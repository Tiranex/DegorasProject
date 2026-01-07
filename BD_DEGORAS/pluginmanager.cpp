#include "pluginmanager.h"
#include <QDir>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QJsonDocument>
#include <spdlog/spdlog.h>

// Static singleton accessor
PluginManager& PluginManager::instance()
{
    static PluginManager s_instance;
    return s_instance;
}

// Scans the filesystem to dynamically load QPlugins
void PluginManager::loadPlugins(const QString& directory)
{
    QDir pluginsDir;

    // Determine the target directory (defaulting to AppDir/plugins)
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

    // Get all files in the directory
    const auto entryList = pluginsDir.entryList(QDir::Files);

    if (entryList.isEmpty()) {
        spdlog::warn(">>>> FOLDER IS EMPTY! No files found in 'plugins' directory.");
    }

    // Iterate through every file and attempt to load it as a Qt Plugin
    for (const QString &fileName : entryList) {
        QString absolutePath = pluginsDir.absoluteFilePath(fileName);
        spdlog::info(">>>> Found file: {}", fileName.toStdString());

        QPluginLoader loader(absolutePath);
        QObject *plugin = loader.instance();

        if (plugin) {
            // Check if the loaded plugin implements the correct Interface (IID)
            auto searchEngine = qobject_cast<SpaceObjectSearchEngine*>(plugin);
            if (searchEngine) {
                // Success: Configure and store the engine
                searchEngine->setFilePath(absolutePath);
                m_searchEngines.append(searchEngine);
                spdlog::info(">>>> SUCCESS! Loaded Plugin: {}", searchEngine->getPluginName().toStdString());
            } else {
                // The DLL loaded, but it is not a SpaceObjectSearchEngine
                spdlog::error(">>>> ERROR: File '{}' loaded but CAST FAILED. IID mismatch?", fileName.toStdString());
                loader.unload();
            }
        } else {
            // The DLL failed to load (dependency missing, not a DLL, etc.)
            spdlog::error(">>>> ERROR LOADING '{}': {}", fileName.toStdString(), loader.errorString().toStdString());
        }
    }
}

// Returns list of loaded plugin names for UI display
QStringList PluginManager::getLoadedEngines() const
{
    QStringList names;
    for(auto* engine : m_searchEngines) {
        names << engine->getPluginName();
    }
    return names;
}

// Performs the search logic by iterating available plugins
nlohmann::json PluginManager::searchSpaceObject(int64_t noradId)
{
    QString noradStr = QString::number(noradId);
    spdlog::info("Searching via plugins for NORAD: {}", noradId);

    for (auto* engine : m_searchEngines) {
        // Skip disabled plugins
        if (!engine->isEnabled()) continue;

        SpaceObject legacyObj;

        // Execute search on the plugin
        auto error = engine->searchSpaceObjectByNorad(noradStr, legacyObj);

        // If found, convert the result and return immediately
        if (error.id == SpaceObjectSearchEngine::ErrorEnum::NoError) {
            spdlog::info("Object found via plugin: {}", engine->getPluginName().toStdString());
            QJsonObject qJson = legacyObj.toJson();
            return qJsonToNlohmann(qJson);
        }
    }

    spdlog::warn("Object {} not found in any loaded plugin.", noradId);
    return nlohmann::json{};
}

// Helper to bridge QJson (used by Plugins) to nlohmann::json (used by Core)
nlohmann::json PluginManager::qJsonToNlohmann(const QJsonObject& qObj)
{
    QJsonDocument doc(qObj);
    // Serialize to string format to allow parsing by nlohmann
    std::string jsonString = doc.toJson(QJsonDocument::Compact).toStdString();
    try {
        return nlohmann::json::parse(jsonString);
    } catch (...) {
        return nlohmann::json{};
    }
}
