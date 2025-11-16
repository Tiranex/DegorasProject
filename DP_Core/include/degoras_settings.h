#pragma once

#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QList>
#include <QVariant>
#include <QMetaType>
#include <QTime>

#include "dpcore_global.h"
#include "window_message_box.h"

Q_DECLARE_METATYPE(QList<double>)

class DP_CORE_EXPORT DegorasSettings
{
public:

    enum ErrorEnum
    {
        NOT_ERROR,
        FILE_NOT_OPEN,
        INVALID_JSON,
        INVALID_COORDINATES,
        INVALID_XYZ
    };

    // Seters.
    DegorasInformation setGlobalConfig(const QString& path);
    DegorasInformation setApplicationConfig(const QString& path);
    DegorasInformation setStationData(const QString& path);

    // Observers for settings.
    inline QSettings* getGlobalConfig() const {return (this->settings_global);}
    inline QSettings* getApplicationConfig() const {return (this->settings_application);}
    inline QVariant getGlobalConfigValue(const QString& key) const {return this->settings_global->value(key);}
    inline QVariant getAppConfigValue(const QString& key) const {return this->settings_application->value(key);}
    inline QString getGlobalConfigString(const QString& key) const {return this->settings_global->value(key).toString();}
    inline QString getAppConfigString(const QString& key) const {return this->settings_application->value(key).toString();}
    inline bool getGlobalConfigBool(const QString& key) const {return this->settings_global->value(key).toBool();}
    inline bool getAppConfigBool(const QString& key) const {return this->settings_application->value(key).toBool();}
    inline double getGlobalConfigDouble(const QString& key) const {return this->settings_global->value(key).toDouble();}
    inline double getAppConfigDouble(const QString& key) const {return this->settings_application->value(key).toDouble();}
    inline int getGlobalConfigInt(const QString& key) const {return this->settings_global->value(key).toInt();}
    inline int getAppConfigInt(const QString& key) const {return this->settings_application->value(key).toInt();}
    inline QTime getGlobalConfigTime(const QString& key) const {
        return QTime::fromString(this->settings_global->value(key).toString(), "hh:mm");}
    inline QTime getAppConfigTime(const QString& key) const {
        return QTime::fromString(this->settings_application->value(key).toString(), "hh:mm");}

    // Observers for data.
    inline QSettings* getStationData() const {return (this->data_station);}
    inline QVariant getStationDataValue(const QString& key) const {return (this->data_station->value(key));}
    inline QString getStationDataString(const QString& key) const {return this->data_station->value(key).toString();}
    inline bool getStationDataBool(const QString& key) const {return this->data_station->value(key).toBool();}
    inline double getStationDataDouble(const QString& key) const {return this->data_station->value(key).toDouble();}
    inline int getStationDataInt(const QString& key) const {return this->data_station->value(key).toInt();}

    // Observers for path and filepath.
    inline const QDir& getGlobalConfigPath() const {return (this->settings_global_path);}
    inline const QDir& getApplicationConfigPath() const {return (this->settings_application_path);}
    inline const QDir& getStationDataPath() const {return (this->data_station_path);}
    inline const QString getGlobalConfigFilePath() const
    {return (this->settings_global_path.path()+'/'+settings_global_filename);}
    inline const QString getApplicationConfigFilePath() const
    {return (this->settings_application_path.path()+'/'+settings_application_filename);}
    inline const QString getStationDataFilePath() const
    {return (this->data_station_path.path()+'/'+data_station_filename);}

    // Singleton instance.
    static DegorasSettings& instance();

protected:
    QSettings* settings_global;
    QSettings* settings_application;
    QSettings* data_station;
    QDir settings_global_path;
    QDir settings_application_path;
    QDir data_station_path;
    QString settings_global_filename;
    QString settings_application_filename;
    QString data_station_filename;

private:
    DegorasSettings() = default;
    ~DegorasSettings();
    DegorasSettings(const DegorasSettings&) = delete;
    DegorasSettings& operator=(const DegorasSettings&) = delete;
    DegorasSettings(DegorasSettings&&) = delete;
    DegorasSettings& operator=(DegorasSettings&&) = delete;
};
