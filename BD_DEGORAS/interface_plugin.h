#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDir>
#include <QVariant>
#include <QMap>
#include <QDebug>

class SPPlugin;

/**
 * @brief Categorizes the type of functionality provided by a plugin.
 *
 * This enum uses bit flags (unsigned int) to allow for potential future
 * combinations, although currently used as distinct types.
 */
enum class PluginCategory : unsigned int
{
    SPACE_OBJECT_SEARCH_ENGINE = 1,      ///< Search for satellites/objects (e.g., Space-Track, CelesTrak).
    LASER_SYSTEM_CONTROLLER = 2,         ///< Hardware control for laser systems.
    CPF_DOWNLOAD_ENGINE = 4,             ///< Downloads Consolidated Prediction Format files.
    TLE_DOWNLOAD_ENGINE = 8,             ///< Downloads Two-Line Element sets.
    EXTERNAL_TOOL = 16,                  ///< General external utility.
    RGG_CONTROLLER = 32,                 ///< Range Gate Generator controller.
    RESULTS_FORMAT = 64,                 ///< Formatter for observation results.
    RANGE_SOURCE = 128,                  ///< Source of range data.
    EVENT_TIMER = 256,                   ///< Precision timing hardware controller.
    RT_FILTER_SLR = 512,                 ///< Real-time filter for Satellite Laser Ranging.
    METEO_DATA_SOURCE = 1024,            ///< Weather station or data source.
    RESULTS_SENDER = 2048,               ///< Transmits results to a remote server.
    MOUNT_SYSTEM_CONTROLLER = 4096,      ///< Telescope mount controller.
    TLE_PROPAGATOR = 8192,               ///< Propagates TLEs to calculate positions.
    DOME_SYSTEM_CONTROLLER = 16384       ///< Observatory dome controller.
};

Q_DECLARE_FLAGS(PluginCategories, PluginCategory)
Q_DECLARE_OPERATORS_FOR_FLAGS(PluginCategories)

/** @brief Alias for a MultiMap storing plugins by their category. */
using PluginsMultiMap = QMultiMap<PluginCategory, SPPlugin*>;

extern const QMap<PluginCategory, QString> PluginCategoryEnumMap;

/**
 * @brief Helper to get the string representation of a plugin's category.
 */
QString pluginCategoryString(SPPlugin*);

/**
 * @brief Helper to get the string representation of a category enum.
 */
QString pluginCategoryString(PluginCategory);

/**
 * @brief Helper to verify if a plugin instance can be cast to its specific interface.
 * @param plugin The generic plugin pointer.
 * @return true if the cast is valid, false otherwise.
 */
bool checkPluginSpecificCast(SPPlugin*);

/**
 * @brief Base class for all loadable plugins in the system.
 *
 * Provides metadata management (name, version, author), file path tracking,
 * and enablement state. All specific plugin interfaces must inherit from this.
 */
class SPPlugin : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor.
     * @param category_ The specific category this plugin belongs to.
     */
    SPPlugin(PluginCategory category_);

    /**
     * @brief Parses and sets plugin metadata from a JSON object.
     * @param metadata JSON containing keys like "Name", "Version", "Copyright".
     */
    void setMetaData(const QJsonObject& metadata);

    /**
     * @brief Sets the absolute file path where the plugin binary resides.
     * @param path File path string.
     */
    void setFilePath(const QString& path);

    /**
     * @brief Enables or disables the plugin.
     * @param enabled True to enable, False to disable.
     */
    void setEnabled(bool enabled);

    // Getters
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
