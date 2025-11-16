#pragma once

#include "../dpcore_global.h"
#include "meteodata.h"

#include <LibDegorasBase/Mathematics/units/unit_conversions.h>
#include <LibDegorasSLR/ILRS/algorithms/data/statistics_data.h>

#include <QString>
#include <QDateTime>


struct DP_CORE_EXPORT Calibration
{

    enum class Type
    {
        UNDEFINED,
        NOMINAL,
        EXTERNAL,
        INTERNAL_TELESCOPE,
        INTERNAL_BUILDING,
        BURST_CALIBRATIONS,
        OTHER
    };

    struct DP_CORE_EXPORT RangeData
    {
        enum class FilterFlag
        {
            UNKNOWN = 0,
            NOISE = 1,
            DATA = 2
        };

        long double start_time;
        long double tof_2w;
        FilterFlag flag;

        explicit RangeData();

    };

    QDateTime date_start;
    QString cfg_id;
    QString station_name;
    unsigned int station_id;
    Type type;
    dpbase::math::units::Distance<double> target_dist_2w;
    dpbase::math::units::Distance<double> target_tof_2w;
    double rf;
    std::size_t nshots;
    std::size_t rnshots;
    std::size_t unshots;
    double tror_rfrms;
    double tror_1rms;
    double cal_val_rfrms;
    double cal_val_1rms;

    MeteoData meteo;

    dpslr::ilrs::algorithms::DistStats stats_1rms;
    dpslr::ilrs::algorithms::DistStats stats_rfrms;

    std::vector<RangeData> ranges;
    std::vector<long double> tA;
    std::vector<long double> tB;
    unsigned int et_precision;

    explicit Calibration();

};

