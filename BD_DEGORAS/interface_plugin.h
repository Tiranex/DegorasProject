#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDir>
#include <QVariant>
#include <QMap>
#include <QDebug>

class SPPlugin;

enum class PluginCategory : unsigned int
{
    SPACE_OBJECT_SEARCH_ENGINE = 1,
    LASER_SYSTEM_CONTROLLER = 2,
    CPF_DOWNLOAD_ENGINE = 4,
    TLE_DOWNLOAD_ENGINE = 8,
    EXTERNAL_TOOL = 16,
    RGG_CONTROLLER = 32,
    RESULTS_FORMAT = 64,
    RANGE_SOURCE = 128,
    EVENT_TIMER = 256,
    RT_FILTER_SLR = 512,
    METEO_DATA_SOURCE = 1024,
    RESULTS_SENDER = 2048,
    MOUNT_SYSTEM_CONTROLLER = 4096,
    TLE_PROPAGATOR = 8192,
    DOME_SYSTEM_CONTROLLER = 16384
};

Q_DECLARE_FLAGS(PluginCategories, PluginCategory)
Q_DECLARE_OPERATORS_FOR_FLAGS(PluginCategories)

using PluginsMultiMap = QMultiMap<PluginCategory, SPPlugin*>;


extern const QMap<PluginCategory, QString> PluginCategoryEnumMap;
QString pluginCategoryString(SPPlugin*);
QString pluginCategoryString(PluginCategory);
bool checkPluginSpecificCast(SPPlugin*);
class SPPlugin : public QObject
{
    Q_OBJECT
public:

    SPPlugin(PluginCategory category_);

    void setMetaData(const QJsonObject& metadata);
    void setFilePath(const QString& path);
    void setEnabled(bool enabled);
    const QString& getPluginName() const;
    const QString& getPluginShortName() const;
    const QString& getPluginVersion() const;
    const QString& getPluginCopyright() const;
    PluginCategory getPluginCategory() const;

    const QDir& getPluginDir() const;
    const bool& isEnabled() const;
    virtual ~SPPlugin() override = default;

protected:
    const PluginCategory category_;
    QString name;
    QString shortname;
    QString version;
    QString copyright;
    QDir filedir;
    bool enabled;
    QJsonObject metadata;
};

Q_DECLARE_METATYPE(SPPlugin*)
