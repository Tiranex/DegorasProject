#include "Tracking/meteodata.h"

const QString kDateStartKey = QStringLiteral("date");
const QString kOriginKey = QStringLiteral("origin");
const QString kTempKey = QStringLiteral("temp");
const QString kPressKey = QStringLiteral("pres");
const QString kRHKey = QStringLiteral("rh");
const QString kWSKey = QStringLiteral("ws");
const QString kWDKey = QStringLiteral("wdir");
const QString kVisKey = QStringLiteral("vis");
const QString kCondKey = QStringLiteral("cond");
const QString kClarityKey = QStringLiteral("clarity");
const QString kSeeingKey = QStringLiteral("seeing");
const QString kCloudKey = QStringLiteral("cloud_cover");
const QString kSkyTempKey = QStringLiteral("sky_temp");

MeteoData::MeteoData() :
    origin(MeteoData::Origin::MEASURED),
    temp(0.),
    pressure(0.),
    rel_hum(0.)
{

}

QJsonObject MeteoData::toJson() const
{
    QJsonObject obj;
    obj.insert(kOriginKey, static_cast<int>(this->origin));
    obj.insert(kDateStartKey, this->date.toString(Qt::ISODateWithMs));
    obj.insert(kTempKey, this->temp);
    obj.insert(kPressKey, this->pressure);
    obj.insert(kRHKey, static_cast<int>(this->rel_hum));
    obj.insert(kWSKey, this->wind_speed ? this->wind_speed.value() : QJsonValue());
    obj.insert(kWDKey, this->wind_dir ? this->wind_dir.value() : QJsonValue());
    obj.insert(kVisKey, this->visibility ? static_cast<int>(this->visibility.value()) : QJsonValue());
    obj.insert(kCondKey, this->condition ? this->condition.value() : QJsonValue());
    obj.insert(kClarityKey, this->clarity ? this->clarity.value() : QJsonValue());
    obj.insert(kSeeingKey, this->seeing ? this->seeing.value() : QJsonValue());
    obj.insert(kCloudKey, this->cloud_cover ? static_cast<int>(this->cloud_cover.value()) : QJsonValue());
    obj.insert(kSkyTempKey, this->sky_temp ? this->sky_temp.value() : QJsonValue());

    return obj;
}

MeteoData MeteoData::fromJson(const QJsonObject &obj)
{
    MeteoData data;
    data.origin = static_cast<MeteoData::Origin>(obj[kOriginKey].toInt());
    data.date = QDateTime::fromString(obj[kDateStartKey].toString(), Qt::ISODateWithMs);
    data.temp = obj[kTempKey].toDouble();
    data.pressure = obj[kPressKey].toDouble();
    data.rel_hum = obj[kRHKey].toInt();
    data.wind_speed = obj[kWSKey].isDouble() ? obj[kWSKey].toDouble() : decltype(data.wind_speed)();
    data.wind_dir = obj[kWDKey].isDouble() ? obj[kWDKey].toDouble() : decltype(data.wind_dir)();
    data.visibility = obj[kVisKey].isDouble() ? obj[kVisKey].toInt() : decltype(data.visibility)();
    data.condition = obj[kCondKey].isString() ? obj[kCondKey].toString() : decltype(data.condition)();
    data.clarity = obj[kClarityKey].isDouble() ? obj[kClarityKey].toDouble() : decltype(data.clarity)();
    data.seeing = obj[kSeeingKey].isDouble() ? obj[kSeeingKey].toDouble() : decltype(data.seeing)();
    data.cloud_cover = obj[kCloudKey].isDouble() ? obj[kCloudKey].toInt() : decltype(data.cloud_cover)();
    data.sky_temp = obj[kSkyTempKey].isDouble() ? obj[kSkyTempKey].toDouble() : decltype(data.sky_temp)();

    return data;
}
