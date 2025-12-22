#include "interface_plugin.h"
#include "interface_spaceobjectsearchengine.h"

SPPlugin::SPPlugin(PluginCategory category) : category_(category) {}

void SPPlugin::setMetaData(const QJsonObject &metadata)
{
    if(metadata.contains("MetaData") && metadata["MetaData"].isObject()){
        QJsonObject md = metadata["MetaData"].toObject();
        this->name = md["Name"].toString();
        this->shortname = md["ShortName"].toString();
        this->version = md["Version"].toString();
        this->copyright = md["Copyright"].toString();
    }
    this->metadata = metadata;
}

void SPPlugin::setFilePath(const QString &path){ this->filedir.setPath(path); }
void SPPlugin::setEnabled(bool enabled){ this->enabled = enabled; }
const QString &SPPlugin::getPluginName() const { return this->name; }
const QString &SPPlugin::getPluginShortName() const { return this->shortname; }
const QString &SPPlugin::getPluginVersion() const { return this->version; }
const QString &SPPlugin::getPluginCopyright() const { return this->copyright; }
PluginCategory SPPlugin::getPluginCategory() const { return this->category_; }
const QDir &SPPlugin::getPluginDir() const { return this->filedir; }
const bool &SPPlugin::isEnabled() const { return this->enabled; }

bool checkPluginSpecificCast(SPPlugin* plugin)
{
    bool result = false;
    switch (plugin->getPluginCategory())
    {
    case PluginCategory::SPACE_OBJECT_SEARCH_ENGINE:
        result = qobject_cast<SpaceObjectSearchEngine*>(plugin);
        break;
    default:
        // Other categories would require their specific includes and casts here
        result = false;
        break;
    }
    return result;
}

// Complete mapping of all PluginCategory enum values to human-readable strings.
const QMap<PluginCategory, QString> PluginCategoryEnumMap =
    {
        {PluginCategory::SPACE_OBJECT_SEARCH_ENGINE, "Space Object Search Engine"},
        {PluginCategory::LASER_SYSTEM_CONTROLLER,    "Laser System Controller"},
        {PluginCategory::CPF_DOWNLOAD_ENGINE,        "CPF Download Engine"},
        {PluginCategory::TLE_DOWNLOAD_ENGINE,        "TLE Download Engine"},
        {PluginCategory::EXTERNAL_TOOL,              "External Tool"},
        {PluginCategory::RGG_CONTROLLER,             "RGG Controller"},
        {PluginCategory::RESULTS_FORMAT,             "Results Format"},
        {PluginCategory::RANGE_SOURCE,               "Range Source"},
        {PluginCategory::EVENT_TIMER,                "Event Timer"},
        {PluginCategory::RT_FILTER_SLR,              "Real-Time Filter (SLR)"},
        {PluginCategory::METEO_DATA_SOURCE,          "Meteo Data Source"},
        {PluginCategory::RESULTS_SENDER,             "Results Sender"},
        {PluginCategory::MOUNT_SYSTEM_CONTROLLER,    "Mount System Controller"},
        {PluginCategory::TLE_PROPAGATOR,             "TLE Propagator"},
        {PluginCategory::DOME_SYSTEM_CONTROLLER,     "Dome System Controller"}
};

QString pluginCategoryString(SPPlugin *plugin)
{
    return pluginCategoryString(plugin->getPluginCategory());
}

QString pluginCategoryString(PluginCategory category)
{
    auto it = PluginCategoryEnumMap.constFind(category);
    return it != PluginCategoryEnumMap.cend() ? it.value() : "Unknown Category";
}
