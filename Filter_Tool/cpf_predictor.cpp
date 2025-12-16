#include "cpf_predictor.h"
#include <QDebug>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QRegularExpression> // <--- AÑADE ESTO O NO FUNCIONARÁ

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

// --- LOAD CON RECONSTRUCCIÓN DE CABECERA (FIXED-WIDTH FIX) ---
bool CPFPredictor::load(const QString& filePath) {
    try {
        qDebug() << "--------------------------------------------------";
        qDebug() << "Procesando CPF (Modo Uppercase + Strict Align):" << filePath;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        bool modified = false;

        // --- 1. RECONSTRUCCIÓN H1 (AÑADIDO UPPERCASE) ---
        // Regex para capturar datos
        QRegularExpression h1Regex("^H1\\s+\\w+\\s+\\d+\\s+\\w+\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+([A-Za-z0-9]+).*$", QRegularExpression::MultilineOption);

        QRegularExpressionMatch matchH1 = h1Regex.match(content);
        if (matchH1.hasMatch()) {
            int year = matchH1.captured(1).toInt();
            int mon  = matchH1.captured(2).toInt();
            int day  = matchH1.captured(3).toInt();
            int hour = matchH1.captured(4).toInt();
            int seq  = matchH1.captured(6).toInt();
            QString name = matchH1.captured(7);

            // *** CAMBIO CLAVE: .toUpper() ***
            // lageos1 -> LAGEOS1
            name = name.toUpper();

            // Reconstrucción H1 alineada CPF v1
            // Usamos %-10s para alinear a la izquierda
            QString cleanHeader = QString::asprintf("H1 CPF  1 SPC %4d %02d %02d %02d 00 00 %3d %-10s",
                                                    year, mon, day, hour, seq, name.toLatin1().constData());

            content.replace(matchH1.captured(0), cleanHeader);
            modified = true;
            qDebug() << "--> FIX H1: Nombre Satélite convertido a MAYÚSCULAS (" << name << ")";
        }

        // --- 2. RECONSTRUCCIÓN H2 ---
        QRegularExpression h2Regex("^H2\\s+\\d+\\s+\\d+\\s+(\\d+)\\s+(.*)$", QRegularExpression::MultilineOption);

        QRegularExpressionMatch matchH2 = h2Regex.match(content);
        if (matchH2.hasMatch()) {
            QString satID = matchH2.captured(1);
            QString rest  = matchH2.captured(2).trimmed(); // Trim para quitar espacios raros al inicio

            int idNum = satID.toInt();

            // H2 Estándar: ID(8) + SIC(10) + Resto
            // El espacio entre %10d y %s asegura que el año empiece en col 23 (1+2+8+1+10+1 = 23)
            QString newH2 = QString::asprintf("H2 %8d %10d %s",
                                              idNum, idNum, rest.toLatin1().constData());

            content.replace(matchH2.captured(0), newH2);
            modified = true;
            qDebug() << "--> FIX H2: Columnas extra eliminadas.";
        }

        // --- 3. LIMPIEZA GENERAL ---
        if (content.contains("DGF", Qt::CaseInsensitive)) {
            content.replace("DGF", "SPC", Qt::CaseInsensitive);
            modified = true;
        }

        // --- GUARDAR Y CARGAR ---
        QString pathLoading = filePath;
        std::unique_ptr<QTemporaryFile> tempFile;

        if (modified) {
            tempFile = std::make_unique<QTemporaryFile>();
            if (tempFile->open()) {
                QTextStream out(tempFile.get());
                out << content;
                tempFile->flush();
                pathLoading = tempFile->fileName();

                // Debug visual
                tempFile->seek(0);
                qDebug() << "--> H1 FINAL:" << tempFile->readLine();
                qDebug() << "--> H2 FINAL:" << tempFile->readLine();
            } else {
                return false;
            }
        }

        toGeocentricWGS84(m_stationGeodetic, m_stationGeocentric);

        m_engine = std::make_unique<PredictorSlrCPF>(
            pathLoading.toStdString(),
            m_stationGeodetic,
            m_stationGeocentric
            );

        if (!m_engine || !m_engine->isReady()) {
            qDebug() << "--> FALLO: Motor no listo.";
            return false;
        }

        m_engine->setPredictionMode(PredictorSlrBase::PredictionMode::INSTANT_VECTOR);
        m_engine->enableCorrections(true);
        m_engine->setTropoCorrParams(1013.0, 293.0, 0.50, 0.532, WtrVapPressModel::GIACOMO_DAVIS);

        qDebug() << "--> ÉXITO ABSOLUTO: Archivo cargado.";
        return true;

    } catch (const std::exception& e) {
        qDebug() << "CRITICAL ERROR:" << e.what();
        return false;
    }
}
long long CPFPredictor::calculateTwoWayTOF(int mjd, double seconds_of_day) {
    if (!m_engine || !m_engine->isReady()) return 0;

    MJDate date(mjd);
    SoD sod(seconds_of_day);
    MJDateTime time(date, sod);

    PredictionSLR result;
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
