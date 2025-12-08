#pragma once

#include <QString>
#include "LibDegorasSLR/UtilitiesSLR/predictors/predictor_slr_cpf.h"

using namespace dpslr::slr::predictors;
using namespace dpslr::geo::types;

class CPFPredictor {
public:
    CPFPredictor();
    ~CPFPredictor();

    // Configurar coordenadas de la estación (San Fernando)
    void setStationCoordinates(double lat_deg, double lon_deg, double alt_m);

    // Carga el archivo y prepara el predictor
    bool load(const QString& filePath);

    // Calcula el Tiempo de Vuelo (Ida y Vuelta) teórico en Picosegundos
    long long calculateTwoWayTOF(int mjd, double seconds_of_day);

private:
    // Puntero inteligente o instancia directa del predictor de la librería
    std::unique_ptr<PredictorSlrCPF> m_engine;

    // Coordenadas guardadas para reiniciar el predictor
    GeodeticPointDeg m_stationGeodetic;
    GeocentricPoint m_stationGeocentric;
};
