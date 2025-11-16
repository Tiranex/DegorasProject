#include "degoras_settings.h"

#include <QFileInfo>


DegorasInformation DegorasSettings::setGlobalConfig(const QString &path)
{
    // Clear the settings.
    delete this->settings_global;

    this->settings_global = new QSettings(path, QSettings::IniFormat);
    QFileInfo info(path);
    this->settings_global_path.setPath(info.absolutePath());
    this->settings_global_filename = info.fileName();
    // TODO check for errors.
    return DegorasInformation();
}

DegorasInformation DegorasSettings::setApplicationConfig(const QString &path)
{
    // Clear the settings.
    delete this->settings_application;

    this->settings_application = new QSettings(path, QSettings::IniFormat);
    QFileInfo info(path);
    this->settings_application_path.setPath(info.absolutePath());
    this->settings_application_filename = info.fileName();

    // TODO check for errors.
    return DegorasInformation();
}

DegorasInformation DegorasSettings::setStationData(const QString &path)
{
    // Variables.
    double x, y, z, latitude, longitude, altitude;
    QString station_name, station_short_name, station_code;
    QFileInfo info(path);

    // Clear the data.
    delete this->data_station;

    // Open json station file.
    QFile stationdatafile(path);
    if(!stationdatafile.open(QIODevice::ReadOnly | QIODevice::Text))
        return DegorasInformation({ErrorEnum::FILE_NOT_OPEN, "Station data json file not open."}, path);
    QString stationdatafile_string = stationdatafile.readAll().simplified();
    stationdatafile.close();
    QJsonDocument stationdatafile_jsondocument = QJsonDocument::fromJson(stationdatafile_string.toUtf8());
    if (stationdatafile_jsondocument.isNull())
        return DegorasInformation({ErrorEnum::INVALID_JSON, "Station data json file invalid."}, path);
    QJsonArray array_xyz =stationdatafile_jsondocument["StationXYZ"].toArray();
    QJsonArray array_coor=stationdatafile_jsondocument["StationCoordinates"].toArray();
    if(array_coor.size()!=3)
        return DegorasInformation({ErrorEnum::INVALID_COORDINATES, "Station coordinates invalid."}, path);
    if(array_xyz.size()!=3)
        return DegorasInformation({ErrorEnum::INVALID_XYZ, "Station XYZ invalid."}, path);;
    x = array_xyz[0].toDouble();
    y = array_xyz[1].toDouble();
    z = array_xyz[2].toDouble();
    latitude = array_coor[0].toDouble()*3.14/180.0; // TODO() Sustituir por dpslr
    longitude = array_coor[1].toDouble()*3.14/180.0;
    altitude = array_coor[2].toDouble();

    QList<double> list_xyz {x,y,z};
    QList<double> list_coor {latitude,longitude,altitude};

    station_name = stationdatafile_jsondocument["StationName"].toString();
    station_short_name = stationdatafile_jsondocument["StationShortName"].toString();
    station_code = stationdatafile_jsondocument["StationCode"].toString();

    // Store the data.
    this->data_station = new QSettings;
    this->data_station->beginGroup("StationData");
    this->data_station->setValue("StationName", station_name);
    this->data_station->setValue("StationShortName", station_short_name);
    this->data_station->setValue("StationCode", station_code);
    this->data_station->setValue("StationXYZ", QVariant::fromValue(list_xyz));
    this->data_station->setValue("StationCoordinates", QVariant::fromValue(list_coor));
    this->data_station->endGroup();
    this->data_station_path.setPath(info.absolutePath());
    this->data_station_filename = info.fileName();

    return DegorasInformation();
}

DegorasSettings &DegorasSettings::instance()
{
    static DegorasSettings degoras_settings;
    return  degoras_settings;
}

DegorasSettings::~DegorasSettings()
{
    delete settings_global;
    delete settings_application;
    delete data_station;
}

