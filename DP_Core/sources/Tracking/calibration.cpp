#include "Tracking/calibration.h"

Calibration::RangeData::RangeData():
    start_time(0.0),
    tof_2w(0.0),
    flag(Calibration::RangeData::FilterFlag::UNKNOWN)
{

}

Calibration::Calibration() :
    type(Calibration::Type::UNDEFINED),
    target_dist_2w(0., decltype(target_dist_2w)::Unit::METRES),
    target_tof_2w(0., decltype(target_tof_2w)::Unit::LIGHT_PS),
    rf(0.),
    nshots(0),
    rnshots(0),
    tror_rfrms(0.),
    tror_1rms(0.),
    cal_val_rfrms(0.),
    cal_val_1rms(0.),
    stats_1rms({0,0,0,0.,0.,0.,0.,0.,0.}),
    stats_rfrms({0,0,0,0.,0.,0.,0.,0.,0.}),
    et_precision(0)

{

}
