#include "cpf_predictor.h"
#include <QDebug>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>

// --- NAMESPACES ---
using namespace dpbase::timing::dates;
using namespace dpbase::timing::types;
using namespace dpslr::geo::types;
using namespace dpslr::geo::meteo;

// --- CONSTANTES ---
const double SEC_TO_PS = 1.0e12;
const double PI = 3.14159265358979323846;
const double DEG_TO_RAD = PI / 180.0;

// --- FUNCIÓN AUXILIAR MATEMÁTICA ---
void toGeocentricWGS84(const GeodeticPointDeg& geo, GeocentricPoint& outXyz)
{
    const double a = 6378137.0;
    const double f = 1.0 / 298.257223563;
    const double e2 = 2*f - f*f;
    double lat_rad = static_cast<double>(geo.lat) * DEG_TO_RAD;
    double lon_rad = static_cast<double>(geo.lon) * DEG_TO_RAD;
    double alt_m   = static_cast<double>(geo.alt);
    double sin_lat = std::sin(lat_rad);
    double cos_lat = std::cos(lat_rad);
    double N = a / std::sqrt(1.0 - e2 * sin_lat * sin_lat);
    outXyz.x = (N + alt_m) * cos_lat * std::cos(lon_rad);
    outXyz.y = (N + alt_m) * cos_lat * std::sin(lon_rad);
    outXyz.z = (N * (1.0 - e2) + alt_m) * sin_lat;
}

// =========================================================

CPFPredictor::CPFPredictor() {
    setStationCoordinates(36.4624, -6.2062, 197.0);
}

CPFPredictor::~CPFPredictor() {}

void CPFPredictor::setStationCoordinates(double lat_deg, double lon_deg, double alt_m) {
    m_stationGeodetic.lat = lat_deg;
    m_stationGeodetic.lon = lon_deg;
    m_stationGeodetic.alt = alt_m;
}

// --- VERSIÓN: RECONSTRUCCIÓN TOTAL DE ALINEACIÓN ---
bool CPFPredictor::load(const QString& filePath) {
    QString tempFilePath;

    try {
        qDebug() << "==================================================";
        qDebug() << "PROCESANDO CPF (Reconstruccion Columna a Columna):" << filePath;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        QTextStream in(&file);

        // Archivo temporal BINARIO (para controlar los \n)
        tempFilePath = QDir::tempPath() + "/cpf_align_" + QUuid::createUuid().toString(QUuid::Id128) + ".cpf";
        QFile outFile(tempFilePath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            return false;
        }

        bool h1Found = false;
        bool h2Found = false;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            QByteArray lineBytes;
            bool writeLine = false;

            // --- FIX H1: Cabecera Perfecta (Minusculas + 01) ---
            if (line.startsWith("H1")) {
                // Regex para capturar datos
                QRegularExpression h1Regex("^H1\\s+\\w+\\s+\\d+\\s+\\w+\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+([A-Za-z0-9]+).*$", QRegularExpression::MultilineOption);
                QRegularExpressionMatch match = h1Regex.match(line);
                if (match.hasMatch()) {
                    int year = match.captured(1).toInt();
                    int mon  = match.captured(2).toInt();
                    int day  = match.captured(3).toInt();
                    int hour = match.captured(4).toInt();
                    int seq  = match.captured(6).toInt();
                    QString name = match.captured(7).toLower(); // ¡VOLVEMOS A MINÚSCULAS COMO EN EL SGF!

                    // Formato V1 Estándar (Columnas fijas)
                    // H1 CPF 01 SPC ...
                    QString s = QString::asprintf("H1 CPF 01 SPC %4d %02d %02d %02d 00 00 %03d %-10s",
                                                  year, mon, day, hour, seq, name.toLatin1().constData());
                    lineBytes = s.toLatin1();
                    h1Found = true;
                    writeLine = true;
                    qDebug() << "-> H1 FIXED:" << s;
                }
            }
            // --- FIX H2: Cabecera Perfecta ---
            else if (line.startsWith("H2")) {
                QRegularExpression h2Regex("^H2\\s+\\d+\\s+\\d+\\s+(\\d+)\\s+(.*)$");
                QRegularExpressionMatch match = h2Regex.match(line);
                if (match.hasMatch()) {
                    int idNum = match.captured(1).toInt();
                    QString rest = match.captured(2).trimmed();
                    // Columna 24 (Start Year) asegurada por los anchos fijos
                    QString s = QString::asprintf("H2 %8d %10d %s", idNum, idNum, rest.toLatin1().constData());
                    lineBytes = s.toLatin1();
                    h2Found = true;
                    writeLine = true;
                    qDebug() << "-> H2 FIXED:" << s;
                }
            }
            // --- FIX DATA (10): RECONSTRUCCIÓN DE COLUMNAS (CRÍTICO) ---
            else if (line.startsWith("10") || line.trimmed().startsWith("10")) {
                // Regex para leer los números ignorando los espacios locos del DGF
                // Grupos: 1:Dir, 2:MJD, 3:SecOfDay, 4:Leap, 5:X, 6:Y, 7:Z
                QRegularExpression dataRegex("^10\\s+(\\d)\\s+(\\d+)\\s+([\\d\\.]+)\\s+(\\d)\\s+([-\\d\\.]+)\\s+([-\\d\\.]+)\\s+([-\\d\\.]+)");
                QRegularExpressionMatch match = dataRegex.match(line.trimmed());

                if (match.hasMatch()) {
                    int dir = match.captured(1).toInt();
                    int mjd = match.captured(2).toInt();
                    double sod = match.captured(3).toDouble();
                    int leap = match.captured(4).toInt();
                    double x = match.captured(5).toDouble();
                    double y = match.captured(6).toDouble();
                    double z = match.captured(7).toDouble();

                    // REESCRITURA CON ALINEACIÓN ESTÁNDAR ILRS CPF
                    // "10" (col 1-2)
                    // Dir (col 4)
                    // MJD (col 6-10) -> %5d
                    // SOD (col 13-24) -> %12.6f (Aquí estaba el fallo antes, simplified() lo movía a la 11)
                    // Leap (col 26)
                    // X, Y, Z (anchos fijos grandes)

                    QString s = QString::asprintf("10 %1d %5d  %12.6f %1d %16.3f %16.3f %16.3f",
                                                  dir, mjd, sod, leap, x, y, z);

                    lineBytes = s.toLatin1();
                    writeLine = true;
                    // Solo logueamos la primera para no saturar
                    static bool firstDataLogged = false;
                    if (!firstDataLogged) { qDebug() << "-> DATA FIXED:" << s; firstDataLogged = true; }
                }
            }
            // --- H9 ---
            else if (line.startsWith("H9")) {
                lineBytes = "H9";
                writeLine = true;
            }

            // ESCRITURA UNIX (\n)
            if (writeLine) {
                outFile.write(lineBytes);
                outFile.write("\n");
            }
        }

        file.close();
        outFile.close(); // Desbloqueo Windows

        if (!h1Found || !h2Found) {
            qDebug() << "ERROR: Cabeceras no encontradas.";
            QFile::remove(tempFilePath);
            return false;
        }

        // CARGA
        toGeocentricWGS84(m_stationGeodetic, m_stationGeocentric);
        qDebug() << "Cargando archivo realineado:" << tempFilePath;

        m_engine = std::make_unique<PredictorSlrCPF>(
            tempFilePath.toStdString(),
            m_stationGeodetic,
            m_stationGeocentric
            );

        bool ready = false;
        if (m_engine) {
            m_engine->setPredictionMode(PredictorSlrBase::PredictionMode::INSTANT_VECTOR);
            m_engine->enableCorrections(true);
            m_engine->setTropoCorrParams(1013.0, 293.0, 0.50, 0.532, WtrVapPressModel::GIACOMO_DAVIS);
            ready = m_engine->isReady();
        }

        QFile::remove(tempFilePath); // Borrar

        return ready;

    } catch (const std::exception& e) {
        qDebug() << "CRITICAL:" << e.what();
        if (!tempFilePath.isEmpty()) QFile::remove(tempFilePath);
        return false;
    }
}

long long CPFPredictor::calculateTwoWayTOF(int mjd, double seconds_of_day) {
    if (!m_engine || !m_engine->isReady()) return 0;

    MJDate date(mjd);
    SoD sod(seconds_of_day);
    MJDateTime time(date, sod);

    dpslr::slr::predictors::PredictionSLR result;
    auto error = m_engine->predict(time, result);

    if (error != static_cast<unsigned int>(PredictorSlrCPF::PredictionError::NOT_ERROR)) return 0;

    if (result.instant_data.has_value()) {
        double tof_seconds = static_cast<double>(result.instant_data->tof_2w);
        return static_cast<long long>(tof_seconds * SEC_TO_PS);
    }
    return 0;
}
