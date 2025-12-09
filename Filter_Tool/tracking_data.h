/// @file TrackingData.h
/// @brief Defines the **TrackingData** class, a central data structure for managing
/// and organizing Satellite Laser Ranging (SLR) measurement echoes and noise data.

#pragma once

#include <QString>
#include <QList>
#include <QDate>
#include <QFileInfo>
#include <QSet>

#include <Tracking/trackingfilemanager.h>

/**
 * @class TrackingData
 * @brief Manages data loaded from an SLR tracking file, including raw echoes, noise,
 * and calibration values.
 *
 * This class serves as the primary container for the measurement points
 * and associated metadata used throughout the application's analysis.
 */
class TrackingData
{
public:
    /**
     * @struct Echo
     * @brief Represents a single laser ranging measurement point (an "echo" or a "noise" reading).
     *
     * Stores critical timing, ranging, and angular data for each observed point.
     */
    struct Echo
    {
        /**
         * @brief Constructor for the Echo structure.
         */
        Echo(long long unsigned int t, long long int ft, long long int diff, long long int diff_cm,
             double az, double el, bool v, int mjd):
            time(t), flight_time(ft), difference(diff), difference_cm(diff_cm),
            azimuth(az), elevation(el), valid(v), mjd(mjd){}

        long long unsigned int time;     ///< @brief Time of the event, measured in Nanoseconds.
        long long int flight_time;       ///< @brief Measured two-way Time of Flight (TOF), measured in Picoseconds.
        long long int difference;        ///< @brief Difference (residual) between measured TOF and predicted TOF (units usually ps).
        long long int difference_cm;     ///< @brief Difference (residual) converted to centimeters (cm).
        double azimuth;                  ///< @brief Azimuth angle of the measurement in degrees.
        double elevation;                ///< @brief Elevation angle of the measurement in degrees.
        bool valid;                      ///< @brief Flag indicating if the measurement is considered valid (true) or noise (false).
        int mjd;                         ///< @brief Modified Julian Day of the measurement epoch.
    };

    /**
     * @brief Constructor for the TrackingData container.
     *
     * Loads and initializes the raw data from the specified file path, populating
     * the internal lists of echoes and noise readings.
     *
     * @param path_file The file path to the SLR tracking data file.
     * @param reset_tracing Flag indicating whether to reset any previous tracing/state (defaults to true).
     */
    explicit TrackingData(QString path_file, bool reset_tracing = true);

    /**
     * @brief Destructor.
     *
     * Deletes all dynamically allocated Echo objects stored in the lists.
     */
    ~TrackingData() {
        qDeleteAll(list_echoes);
        qDeleteAll(list_noise);
        // list_all contains pointers from the other two, so we don't delete them again.
        list_echoes.clear();
        list_noise.clear();
        list_all.clear();
    }


    /**
     * @brief Returns the list of measurement points classified as valid echoes.
     * @return Constant reference to the QList of Echo pointers.
     */
    const QList<Echo*>& listEchoes() const {return this->list_echoes;}

    /**
     * @brief Returns the list of measurement points classified as noise.
     * @return Constant reference to the QList of Echo pointers.
     */
    const QList<Echo*>& listNoise() const {return this->list_noise;}

    /**
     * @brief Returns the list containing all measurement points (echoes and noise combined).
     * @return Constant reference to the QList of Echo pointers.
     */
    const QList<Echo*>& listAll() const {return this->list_all;}

    /**
     * @brief Retrieves the calculated mean calibration value.
     * @return The mean calibration value (units usually picoseconds).
     */
    int meanCal() const {return this->mean_cal;}

    QString satel_name; ///< @brief Name of the tracked satellite.
    QString file_name;  ///< @brief Name of the source tracking data file.

    bool et_tracking = false;           ///< @brief Flag indicating if data includes original tracking (event time) information.
    bool et_filtered_tracking = false;  ///< @brief Flag indicating if data includes filtered tracking (event time) information.
    bool dp_tracking = false;           ///< @brief Flag indicating if data includes direct photon tracking information.

    /**
     * @brief Internal structure containing raw data structures or external metadata used during file parsing.
     * * @note The type `Tracking` is assumed to be defined by `trackingfilemanager.h`.
     */
    Tracking data;

private:
    QList<Echo*> list_echoes; ///< @brief Internal list of valid echoes (points).
    QList<Echo*> list_noise;  ///< @brief Internal list of noise points.
    QList<Echo*> list_all;    ///< @brief Internal list containing a union of all points (echoes + noise).
    int mean_cal = 0;         ///< @brief Internal variable storing the mean calibration value.
};
