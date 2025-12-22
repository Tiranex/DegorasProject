#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <memory>
#include <nlohmann/json.hpp>

#include "interface_spaceobjectsearchengine.h"

/**
 * @brief Singleton class that manages the lifecycle and execution of search engine plugins.
 *
 * @details This manager is responsible for:
 * - Scanning specific directories for dynamic libraries (plugins).
 * - Loading valid `SpaceObjectSearchEngine` interfaces using QPluginLoader.
 * - Aggregating search results from multiple loaded engines.
 * - Bridging data types between Qt (QJsonObject) and the core logic (nlohmann::json).
 */
class PluginManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Access the singleton instance of the PluginManager.
     * @return Reference to the static instance.
     */
    static PluginManager& instance();

    /**
     * @brief Loads plugins from the specified directory.
     * @details If no directory is provided, it looks for a "plugins" folder
     * relative to the application executable. It verifies the IID of loaded plugins
     * to ensure they implement `SpaceObjectSearchEngine`.
     * @param directory Optional custom path to scan.
     */
    void loadPlugins(const QString& directory = "");

    /**
     * @brief Retrieves the names of all successfully loaded plugins.
     * @return A list of plugin names.
     */
    QStringList getLoadedEngines() const;

    /**
     * @brief Queries all enabled plugins for a specific Space Object.
     * @details Iterates through loaded engines. The first engine to return
     * `ErrorEnum::NoError` determines the result.
     * @param noradId The NORAD ID of the object to search for.
     * @return The object data as a JSON object, or an empty JSON if not found.
     */
    nlohmann::json searchSpaceObject(int64_t noradId);

private:
    /**
     * @brief Private constructor for Singleton pattern.
     */
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    QList<SpaceObjectSearchEngine*> m_searchEngines; ///< List of active plugin instances.

    /**
     * @brief Helper to convert Qt JSON objects to nlohmann JSON.
     * @param qObj The Qt JSON object.
     * @return The equivalent nlohmann JSON object.
     */
    nlohmann::json qJsonToNlohmann(const QJsonObject& qObj);
};

#endif // PLUGINMANAGER_H
