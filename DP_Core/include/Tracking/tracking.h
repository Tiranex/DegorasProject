#pragma once

#include "../dpcore_global.h"
#include "meteodata.h"
#include "calibration.h"

#include <LibDegorasSLR/ILRS/algorithms/data/statistics_data.h>

#include <QString>
#include <QDateTime>
#include <QJsonObject>

dpslr::ilrs::algorithms::DistStats StatsFromJson(const QJsonObject& o);
QJsonObject StatstoJson(const dpslr::ilrs::algorithms::DistStats &stats);

struct DP_CORE_EXPORT TelescopeData
{
    enum class DirectionFlag
    {
        TRANSMIT_AND_RECEIVE,
        TRANSMIT,
        RECEIVE
    };

    enum class Origin
    {
        UNKNOWN,
        COMPUTED,
        COMMANDED,
        MEASURED_ABSOLUTE
    };

    double time;
    double az;
    double el;
    double az_ofst;
    double el_ofst;
    double az_rate;
    double el_rate;
    DirectionFlag dir;
    Origin origin;

    QJsonObject toJson() const;
    static TelescopeData fromJson();

    explicit TelescopeData();
};

struct DP_CORE_EXPORT Tracking
{

    enum class CalibrationSpan
    {
        PRE = 1,
        POST = 2,
        COMBINED = 3,
        RT = 4
    };

    enum class FilterMode
    {
        RAW = 0,
        MANUAL = 1,
        AUTO = 2
    };

    static inline std::array<QString,3> kFilterModeStrings{
        QStringLiteral("Raw"), QStringLiteral("Manual"), QStringLiteral("Auto")};

    using OrderedCalibrations = std::map<QDateTime, Calibration>;
    using CalibrationsBySpan = std::map<CalibrationSpan, OrderedCalibrations>;

    struct DP_CORE_EXPORT RangeData
    {
        enum class FilterFlag
        {
            UNKNOWN = 0,
            NOISE = 1,
            DATA = 2
        };

        long double start_time;
        double tof_2w;
        double pre_2w;
        double trop_corr_2w;
        double bias;
        FilterFlag flag;

        explicit RangeData();

    };

    QDateTime date_start;
    QDateTime date_end;
    FilterMode filter_mode;
    QString cfg_id;
    QString station_name;
    unsigned int station_id;
    QString obj_name;
    QString obj_norad;
    unsigned int obj_bs;
    std::size_t nshots;
    std::size_t rnshots;
    std::size_t unshots;
    double tror_rfrms;
    double tror_1rms;
    double rf;
    unsigned int release;
    QString ephemeris_file;

    dpslr::ilrs::algorithms::DistStats stats_1rms;
    dpslr::ilrs::algorithms::DistStats stats_rfrms;

    std::vector<MeteoData> meteo_data;

    CalibrationsBySpan cal_data;
    double cal_val_overall;

    std::vector<TelescopeData> telescope_data;

    std::vector<RangeData> ranges;
    std::vector<long double> tA;
    std::vector<long double> tB;
    unsigned int et_precision;

    explicit Tracking();
    Tracking(const Tracking&) = default;
    Tracking(Tracking&&) = default;
    Tracking& operator=(const Tracking&) = default;
    Tracking& operator=(Tracking&&) = default;

};

