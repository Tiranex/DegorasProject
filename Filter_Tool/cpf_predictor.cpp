#include "cpf_predictor.h"
#include <QDebug>
#include <cmath>

// --- NAMESPACES (Vital para evitar el error 'no member named dpbase') ---
using namespace dpbase::timing::dates;
using namespace dpbase::timing::types;
using namespace dpslr::geo::types;
using namespace dpslr::geo::meteo;

// Constantes
const double SEC_TO_PS = 1.0e12;
const double PI = 3.14159265358979323846;
const double DEG_TO_RAD = PI / 180.0;

// --- FUNCIÓN AUXILIAR (Privada en el .cpp) ---
// Calcula X, Y, Z a partir de Lat, Lon, Alt
void toGeocentricWGS84(const GeodeticPointDeg& geo, GeocentricPoint& outXyz)
{
    const double a = 6378137.0;             // Radio mayor
    const double f = 1.0 / 298.257223563;   // Aplanamiento
    const double e2 = 2*f - f*f;            // Excentricidad

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
    // Coordenadas por defecto San Fernando
    setStationCoordinates(36.4624, -6.2062, 197.0);
}

CPFPredictor::~CPFPredictor() {}

void CPFPredictor::setStationCoordinates(double lat_deg, double lon_deg, double alt_m) {
    // Guardamos en las variables de TU .h
    m_stationGeodetic.lat = lat_deg;
    m_stationGeodetic.lon = lon_deg;
    m_stationGeodetic.alt = alt_m;
}

bool CPFPredictor::load(const QString& filePath) {
    try {
        // 1. CALCULAMOS LA GEOCÉNTRICA (X, Y, Z)
        // Esto rellena m_stationGeocentric automáticamente usando las matemáticas
        toGeocentricWGS84(m_stationGeodetic, m_stationGeocentric);

        // 2. INICIALIZAMOS EL MOTOR
        m_engine = std::make_unique<PredictorSlrCPF>(
            filePath.toStdString(),
            m_stationGeodetic,
            m_stationGeocentric
            );

        // 3. CONFIGURACIÓN FÍSICA
        m_engine->setPredictionMode(PredictorSlrBase::PredictionMode::INSTANT_VECTOR);
        m_engine->enableCorrections(true);
        m_engine->setTropoCorrParams(1013.0, 293.0, 0.50, 0.532, WtrVapPressModel::GIACOMO_DAVIS);

        return m_engine->isReady();

    } catch (const std::exception& e) {
        qDebug() << "Error loading CPF:" << e.what();
        return false;
    }
}

long long CPFPredictor::calculateTwoWayTOF(int mjd, double seconds_of_day) {
    if (!m_engine || !m_engine->isReady()) return 0;

    // Crear tiempo
    MJDate date(mjd);
    SoD sod(seconds_of_day);
    MJDateTime time(date, sod);

    PredictionSLR result;
    auto error = m_engine->predict(time, result);

    // CORRECCIÓN DEL ERROR ROJO: Casteamos el Enum a int
    if (error != static_cast<unsigned int>(PredictorSlrCPF::PredictionError::NOT_ERROR)) {
        return 0;
    }

    if (result.instant_data.has_value()) {
        double tof_seconds = static_cast<double>(result.instant_data->tof_2w);
        return static_cast<long long>(tof_seconds * SEC_TO_PS);
    }

    return 0;
}
