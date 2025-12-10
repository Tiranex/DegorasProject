/// @file CPFPredictor.h
/// @brief Defines the **CPFPredictor** class, which encapsulates the CPF-based SLR prediction engine.
///
/// This class handles the loading of CPF data files and the calculation of the theoretical
/// Two-Way Time of Flight (TOF) for Satellite Laser Ranging (SLR) measurements.

#pragma once

#include <QString>
#include "LibDegorasSLR/UtilitiesSLR/predictors/predictor_slr_cpf.h"
#include <memory> // Required for std::unique_ptr

// Using declarations to simplify the class interface
using namespace dpslr::slr::predictors;
using namespace dpslr::geo::types;

/**
 * @class CPFPredictor
 * @brief Wrapper class for the PredictorSlrCPF engine used to calculate theoretical Satellite Laser Ranging (SLR) predictions.
 *
 * This class manages the state and configuration necessary to use the CPF
 * prediction engine from the internal library.
 */
class CPFPredictor {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes the internal prediction engine state.
     */
    CPFPredictor();

    /**
     * @brief Destructor.
     */
    ~CPFPredictor();

    // Configurar coordenadas de la estación (San Fernando)
    /**
     * @brief Sets the geodetic coordinates of the ground station.
     *
     * These coordinates are used internally by the prediction engine to calculate the slant range.
     *
     * @param lat_deg Latitude of the station in degrees.
     * @param lon_deg Longitude of the station in degrees.
     * @param alt_m Altitude of the station in meters above the reference ellipsoid.
     */
    void setStationCoordinates(double lat_deg, double lon_deg, double alt_m);

    // Carga el archivo y prepara el predictor
    /**
     * @brief Loads the CPF data file and prepares the prediction engine.
     *
     * This method reads the orbital data from the specified file path, making the predictor ready for TOF calculations.
     *
     * @param filePath The file path to the CPF data file.
     * @return **true** if the file was loaded and the predictor was initialized successfully; **false** otherwise.
     */
    bool load(const QString& filePath);

    // Calcula el Tiempo de Vuelo (Ida y Vuelta) teórico en Picosegundos
    /**
     * @brief Calculates the theoretical Two-Way Time of Flight (TOF) for the given epoch.
     *
     * The TOF is the expected time (round trip) it takes for a laser pulse to travel from the
     * ground station to the satellite and return, calculated in picoseconds.
     *
     * @param mjd Modified Julian Day of the observation epoch.
     * @param seconds_of_day Seconds elapsed since the beginning of the day (MJD).
     * @return The theoretical Two-Way TOF in picoseconds (long long).
     */
    long long calculateTwoWayTOF(int mjd, double seconds_of_day);

private:
    // Puntero inteligente o instancia directa del predictor de la librería
    std::unique_ptr<PredictorSlrCPF> m_engine;  ///< @brief Smart pointer to the core CPF prediction engine.

    // Coordenadas guardadas para reiniciar el predictor
    GeodeticPointDeg m_stationGeodetic; ///< @brief Geodetic coordinates (Lat, Lon, Alt) of the ground station.
    GeocentricPoint m_stationGeocentric; ///< @brief Geocentric (XYZ) coordinates of the ground station, derived from the geodetic coordinates.
};
