#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDir>
#include <QVariant>
#include <QMap>
#include <QDebug>

// BORRADO: #include "spcore_global.h"

class SPPlugin;

// Enum Categories
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

// Map for getting the category strings.
// BORRADO: SP_CORE_EXPORT
extern const QMap<PluginCategory, QString> PluginCategoryEnumMap;
// BORRADO: SP_CORE_EXPORT
QString pluginCategoryString(SPPlugin*);
// BORRADO: SP_CORE_EXPORT
QString pluginCategoryString(PluginCategory);
// BORRADO: SP_CORE_EXPORT
bool checkPluginSpecificCast(SPPlugin*);

// BORRADO: SP_CORE_EXPORT
class SPPlugin : public QObject
{
    Q_OBJECT
public:

    SPPlugin(PluginCategory category_);

    // Non virtual methods.
    void setMetaData(const QJsonObject& metadata);
    void setFilePath(const QString& path);
    void setEnabled(bool enabled);

    // About the plugin.
    const QString& getPluginName() const;
    const QString& getPluginShortName() const;
    const QString& getPluginVersion() const;
    const QString& getPluginCopyright() const;
    PluginCategory getPluginCategory() const;

    const QDir& getPluginDir() const;
    const bool& isEnabled() const;

    // Destructor.
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
