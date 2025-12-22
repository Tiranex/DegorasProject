#include "pluginmanager.h"
#include <QDir>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QJsonDocument>
#include <spdlog/spdlog.h>

PluginManager& PluginManager::instance()
{
    static PluginManager s_instance;
    return s_instance;
}

void PluginManager::loadPlugins(const QString& directory)
{
    QDir pluginsDir;
    if (directory.isEmpty()) {
        pluginsDir = QDir(QCoreApplication::applicationDirPath());
        if (!pluginsDir.cd("plugins")) {
            spdlog::warn("Plugins directory NOT found at: {}/plugins", QCoreApplication::applicationDirPath().toStdString());
            return;
        }
    } else {
        pluginsDir = QDir(directory);
    }

    spdlog::info(">>>> SCANNING FOR PLUGINS IN: {}", pluginsDir.absolutePath().toStdString());

    const auto entryList = pluginsDir.entryList(QDir::Files);
    
    if (entryList.isEmpty()) {
        spdlog::warn(">>>> FOLDER IS EMPTY! No files found in 'plugins' directory.");
    }

    for (const QString &fileName : entryList) {
        QString absolutePath = pluginsDir.absoluteFilePath(fileName);
        spdlog::info(">>>> Found file: {}", fileName.toStdString());

        QPluginLoader loader(absolutePath);
        QObject *plugin = loader.instance();

        if (plugin) {
            auto searchEngine = qobject_cast<SpaceObjectSearchEngine*>(plugin);
            if (searchEngine) {
                searchEngine->setFilePath(absolutePath);
                m_searchEngines.append(searchEngine);
                spdlog::info(">>>> SUCCESS! Loaded Plugin: {}", searchEngine->getPluginName().toStdString());
            } else {
                spdlog::error(">>>> ERROR: File '{}' loaded but CAST FAILED. IID mismatch?", fileName.toStdString());
                loader.unload();
            }
        } else {
            spdlog::error(">>>> ERROR LOADING '{}': {}", fileName.toStdString(), loader.errorString().toStdString());
        }
    }
}

QStringList PluginManager::getLoadedEngines() const
{
    QStringList names;
    for(auto* engine : m_searchEngines) {
        names << engine->getPluginName();
    }
    return names;
}

nlohmann::json PluginManager::searchSpaceObject(int64_t noradId)
{
    QString noradStr = QString::number(noradId);
    spdlog::info("Searching via plugins for NORAD: {}", noradId);

    for (auto* engine : m_searchEngines) {
        if (!engine->isEnabled()) continue;

        SpaceObject legacyObj;

        auto error = engine->searchSpaceObjectByNorad(noradStr, legacyObj);

        if (error.id == SpaceObjectSearchEngine::ErrorEnum::NoError) {
            spdlog::info("Object found via plugin: {}", engine->getPluginName().toStdString());
            QJsonObject qJson = legacyObj.toJson();
            return qJsonToNlohmann(qJson);
        }
    }

    spdlog::warn("Object {} not found in any loaded plugin.", noradId);
    return nlohmann::json{};
}

nlohmann::json PluginManager::qJsonToNlohmann(const QJsonObject& qObj)
{

    QJsonDocument doc(qObj);
    std::string jsonString = doc.toJson(QJsonDocument::Compact).toStdString();
    try {
        return nlohmann::json::parse(jsonString);
    } catch (...) {
        return nlohmann::json{};
    }
}
