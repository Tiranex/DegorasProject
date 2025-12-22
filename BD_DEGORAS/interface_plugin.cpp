#include "interface_plugin.h"                  // <--- RUTA ARREGLADA
#include "interface_spaceobjectsearchengine.h" // <--- RUTA ARREGLADA


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
        result = false;
        break;
    }
    return result;
}

const QMap<PluginCategory, QString> PluginCategoryEnumMap =
{
    {PluginCategory::SPACE_OBJECT_SEARCH_ENGINE, "Space Object Search Engine"},
    {PluginCategory::CPF_DOWNLOAD_ENGINE, "CPF Download Engine"},
    {PluginCategory::TLE_DOWNLOAD_ENGINE, "TLE Download Engine"},
    {PluginCategory::EXTERNAL_TOOL, "External Tool"},
};

QString pluginCategoryString(SPPlugin *plugin)
{
    return pluginCategoryString(plugin->getPluginCategory());
}

QString pluginCategoryString(PluginCategory category)
{
    auto it = PluginCategoryEnumMap.constFind(category);
    return it != PluginCategoryEnumMap.cend() ? it.value() : "";
}
