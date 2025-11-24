#include "cpf_predictor.h"
#include <cmath>
#include <QDebug>


const double C_LIGHT = 299792458.0;

using namespace dpslr::ilrs::cpf;

CPFPredictor::CPFPredictor() : m_cpfLib(1.0f) { // Inicializa con versión dummy
    // Coordenadas San Fernando (ROA) por defecto
    st_x = 5105445.163;
    st_y = -555145.748;
    st_z = 3769803.483;
}

CPFPredictor::~CPFPredictor() {}

void CPFPredictor::setStationCoordinates(double x, double y, double z) {
    st_x = x; st_y = y; st_z = z;
}

bool CPFPredictor::load(const QString& filePath) {

    auto error = m_cpfLib.openCPFFile(filePath.toStdString(), CPF::OpenOptionEnum::ALL_DATA);

    if (error != CPF::ReadFileErrorEnum::NOT_ERROR &&
        error != CPF::ReadFileErrorEnum::HEADER_LOAD_WARNING) {
        qDebug() << "Error loading CPF with LibDegoras. Error Code:" << static_cast<int>(error);
        return false;
    }

    return m_cpfLib.hasData();
}


double CPFPredictor::interpolateLagrange(double target_s, int start_idx, int comp) {

    const auto& records = m_cpfLib.getData().positionRecords();

    double result = 0.0;
    int max_size = static_cast<int>(records.size());
    int end_idx = (start_idx + 8 > max_size) ? max_size : start_idx + 8;

    for (int i = start_idx; i < end_idx; i++) {
        // En LibDegoras: geo_pos.x es comp 0, y es 1, z es 2
        double val_i = (comp == 0) ? records[i].geo_pos.x :
                           (comp == 1) ? records[i].geo_pos.y :
                           records[i].geo_pos.z;

        double term = val_i;

        for (int j = start_idx; j < end_idx; j++) {
            if (i != j) {
                double num = target_s - records[j].sod; // sod = Seconds of Day
                double den = records[i].sod - records[j].sod;
                term *= (num / den);
            }
        }
        result += term;
    }
    return result;
}

long long CPFPredictor::calculateTwoWayTOF(int shot_mjd, unsigned long long shot_time_ns) {

    const auto& records = m_cpfLib.getData().positionRecords();
    if (records.empty()) return 0;

    double shot_seconds = shot_time_ns * 1.0e-9;
    int idx = -1;



    int limit = static_cast<int>(records.size()) - 9;
    for (int i = 0; i < limit; ++i) {

        if (records[i].mjd == shot_mjd &&
            records[i].sod <= shot_seconds &&
            records[i+1].sod > shot_seconds) {

            idx = i - 3; // Centrar Lagrange
            if (idx < 0) idx = 0;
            break;
        }

        // Manejo básico de cruce de día (Si el disparo es al día siguiente del registro actual)
        if (shot_mjd > records[i].mjd) {
            // Lógica más compleja requerida para cruce exacto,
            // por ahora buscamos coincidencia exacta de día.
        }
    }

    if (idx == -1) return 0; // No encontrado en este CPF


    double sat_x = interpolateLagrange(shot_seconds, idx, 0);
    double sat_y = interpolateLagrange(shot_seconds, idx, 1);
    double sat_z = interpolateLagrange(shot_seconds, idx, 2);

    double dx = sat_x - st_x;
    double dy = sat_y - st_y;
    double dz = sat_z - st_z;

    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    double tof = (2.0 * dist) / C_LIGHT;

    return static_cast<long long>(tof * 1.0e12); // Picosegundos
}
