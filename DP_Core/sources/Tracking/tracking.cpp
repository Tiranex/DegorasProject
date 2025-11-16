#include "Tracking/tracking.h"

/* Exportador de Stats */
const QString kIter = QStringLiteral("iter");
const QString kAPtn = QStringLiteral("andata");
const QString kRPtn = QStringLiteral("rndata");
const QString kMean = QStringLiteral("mean");
const QString kRMS = QStringLiteral("rms");
const QString kSkew = QStringLiteral("skew");
const QString kKurt = QStringLiteral("kurt");
const QString kPeak = QStringLiteral("peak");
const QString kARate = QStringLiteral("ror");

dpslr::ilrs::algorithms::DistStats StatsFromJson(const QJsonObject& o)
{
    dpslr::ilrs::algorithms::DistStats stats;
    stats.iter = o[kIter].toInt();
    stats.aptn = o[kAPtn].toInt();
    stats.rptn = o[kRPtn].toInt();
    stats.mean = o[kMean].toDouble();
    stats.rms = o[kRMS].toDouble();
    stats.skew = o[kSkew].toDouble();
    stats.kurt = o[kKurt].toDouble();
    stats.peak = o[kPeak].toDouble();
    stats.arate = o[kARate].toDouble();

    return stats;
}

QJsonObject StatstoJson(const dpslr::ilrs::algorithms::DistStats &stats)
{
    QJsonObject o;
    o.insert(kIter, static_cast<int>(stats.iter));
    o.insert(kAPtn, static_cast<int>(stats.aptn));
    o.insert(kRPtn, static_cast<int>(stats.rptn));
    o.insert(kMean, static_cast<double>(stats.mean));
    o.insert(kRMS, static_cast<double>(stats.rms));
    o.insert(kSkew, static_cast<double>(stats.skew));
    o.insert(kKurt, static_cast<double>(stats.kurt));
    o.insert(kPeak, static_cast<double>(stats.peak));
    o.insert(kARate, static_cast<double>(stats.arate));
    return o;
}


Tracking::RangeData::RangeData():
    start_time(0.0),
    tof_2w(0.0),
    pre_2w(0.0),
    trop_corr_2w(0.0),
    bias(0.0),
    flag(Tracking::RangeData::FilterFlag::UNKNOWN)
{

}

Tracking::Tracking() :
    filter_mode(FilterMode::RAW),
    obj_bs(0),
    nshots(0),
    rnshots(0),
    unshots(0),
    tror_rfrms(0.0),
    tror_1rms(0.0),
    rf(0.),
    release(0),
    cal_val_overall(0.0),
    et_precision(0)

{

}

QJsonObject TelescopeData::toJson() const
{
    // TODO
    return {};
}

TelescopeData TelescopeData::fromJson()
{
    //TODO
    return TelescopeData();
}

TelescopeData::TelescopeData() :
    time(0.0),
    az(0.0),
    el(0.0),
    az_ofst(0.0),
    el_ofst(0.0),
    az_rate(0.0),
    el_rate(0.0),
    dir(TelescopeData::DirectionFlag::TRANSMIT_AND_RECEIVE),
    origin(TelescopeData::Origin::UNKNOWN)
{

}
