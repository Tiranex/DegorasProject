#pragma once

#include "../dpcore_global.h"

#include <QString>
#include <QJsonObject>
#include <QDateTime>

struct DP_CORE_EXPORT MeteoData
{
    enum class Origin
    {
        MEASURED,
        INTERPOLATED
    };

    Origin origin;
    QDateTime date;
    double temp;                    ///< Surface temperature in Kelvin
    double pressure;                ///< Pressure in mbar
    unsigned int rel_hum;           ///< Relative humidity (%)

    std::optional<double> wind_speed;
    std::optional<double> wind_dir;
    std::optional<unsigned int> visibility;
    std::optional<QString> condition;
    std::optional<double> clarity;
    std::optional<double> seeing;
    std::optional<unsigned int> cloud_cover;
    std::optional<double> sky_temp;

    explicit MeteoData();

    QJsonObject toJson() const;
    static MeteoData fromJson(const QJsonObject& obj);
};
