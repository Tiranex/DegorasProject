#include "cpf_predictor.h"
#include <QDebug>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid> // Necesario para generar nombres aleatorios

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

// --- VERSIÓN: WRITE-CLOSE-LOAD (Solución al Bloqueo de Windows) ---
bool CPFPredictor::load(const QString& filePath) {
    // Variable para guardar la ruta del archivo temporal y borrarlo luego
    QString tempFilePath;

    try {
        qDebug() << "==================================================";
        qDebug() << "PROCESANDO CPF (Modo Safe-File-Lock):" << filePath;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error: No se puede abrir el archivo original.";
            return false;
        }

        // 1. GENERAR NOMBRE TEMPORAL ÚNICO
        // No usamos QTemporaryFile para evitar que mantenga el archivo abierto.
        tempFilePath = QDir::tempPath() + "/cpf_clean_" + QUuid::createUuid().toString(QUuid::Id128) + ".cpf";

        QFile outFile(tempFilePath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Error: No se puede escribir en temporales.";
            return false;
        }

        QTextStream in(&file);
        QTextStream out(&outFile);

        // Evitar BOM (Byte Order Mark) que a veces confunde a librerías C++
        out.setGenerateByteOrderMark(false);

        bool h1Found = false;
        bool h2Found = false;

        // 2. BARRIDO LÍNEA A LÍNEA
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            // --- FIX H1 ---
            if (line.startsWith("H1")) {
                QRegularExpression h1Regex("^H1\\s+\\w+\\s+\\d+\\s+\\w+\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+([A-Za-z0-9]+).*$", QRegularExpression::MultilineOption);
                QRegularExpressionMatch match = h1Regex.match(line);
                if (match.hasMatch()) {
                    int year = match.captured(1).toInt();
                    int mon  = match.captured(2).toInt();
                    int day  = match.captured(3).toInt();
                    int hour = match.captured(4).toInt();
                    int seq  = match.captured(6).toInt();
                    QString name = match.captured(7).toUpper(); // LAGEOS1

                    out << QString::asprintf("H1 CPF  1 SPC %4d %02d %02d %02d 00 00 %3d %-10s",
                                             year, mon, day, hour, seq, name.toLatin1().constData()) << "\n";
                    h1Found = true;
                }
            }
            // --- FIX H2 ---
            else if (line.startsWith("H2")) {
                QRegularExpression h2Regex("^H2\\s+\\d+\\s+\\d+\\s+(\\d+)\\s+(.*)$");
                QRegularExpressionMatch match = h2Regex.match(line);
                if (match.hasMatch()) {
                    int idNum = match.captured(1).toInt();
                    QString rest = match.captured(2).trimmed();
                    out << QString::asprintf("H2 %8d %10d %s", idNum, idNum, rest.toLatin1().constData()) << "\n";
                    h2Found = true;
                }
            }
            // --- FIX H9 ---
            else if (line.startsWith("H9")) {
                out << "H9\n";
            }
            // --- FIX DATOS (10) ---
            else if (line.startsWith("10") || line.trimmed().startsWith("10")) {
                out << line.simplified() << "\n";
            }
            // Cualquier otra H (H3, H4...) se ignora deliberadamente.
        }

        // 3. CERRAR ARCHIVOS (CRÍTICO)
        file.close();
        outFile.flush();
        outFile.close(); // ¡AQUÍ LIBERAMOS EL BLOQUEO DE WINDOWS!

        if (!h1Found || !h2Found) {
            qDebug() << "ERROR: No se encontraron cabeceras válidas.";
            QFile::remove(tempFilePath); // Limpieza
            return false;
        }

        // 4. CALCULAR COORDENADAS
        toGeocentricWGS84(m_stationGeodetic, m_stationGeocentric);

        qDebug() << "Cargando archivo liberado:" << tempFilePath;

        // 5. CARGAR MOTOR
        // Ahora la librería puede abrir el archivo porque nosotros lo hemos cerrado
        m_engine = std::make_unique<PredictorSlrCPF>(
            tempFilePath.toStdString(),
            m_stationGeodetic,
            m_stationGeocentric
            );

        bool ready = false;
        if (m_engine) {
            ready = m_engine->isReady();
        }

        // 6. LIMPIEZA FINAL
        // Borramos el archivo temporal para no llenar el disco de basura
        QFile::remove(tempFilePath);

        if (ready) {
            qDebug() << "--> EXITO: Archivo procesado correctamente.";
        } else {
            qDebug() << "--> ERROR: El motor no aceptó el archivo.";
        }

        return ready;

    } catch (const std::exception& e) {
        qDebug() << "CRITICAL ERROR:" << e.what();
        // Intentar borrar el temporal si existe
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

    if (error != static_cast<unsigned int>(PredictorSlrCPF::PredictionError::NOT_ERROR)) {
        return 0;
    }

    if (result.instant_data.has_value()) {
        double tof_seconds = static_cast<double>(result.instant_data->tof_2w);
        return static_cast<long long>(tof_seconds * SEC_TO_PS);
    }

    return 0;
}
