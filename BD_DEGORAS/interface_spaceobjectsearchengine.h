#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

// Required for legacy plugin compatibility
#include "class_spaceobject.h"
#include "interface_plugin.h"

/**
 * @brief Interface definition for plugins that search for Space Objects.
 *
 * Plugins implementing this interface query external databases (e.g., Space-Track, CelesTrak)
 * to retrieve orbital and physical data for a specific satellite using its NORAD ID.
 *
 * @note This interface uses the legacy `SpaceObject` class to maintain ABI compatibility
 * with older compiled plugins. The `PluginManager` handles the conversion to modern JSON formats.
 */
class SpaceObjectSearchEngine : public SPPlugin
{
public:
    /**
     * @brief Constructor. Sets the plugin category to SPACE_OBJECT_SEARCH_ENGINE.
     */
    SpaceObjectSearchEngine() : SPPlugin(PluginCategory::SPACE_OBJECT_SEARCH_ENGINE) {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~SpaceObjectSearchEngine() = default;

    /**
     * @brief Error codes returned by search operations.
     */
    enum ErrorEnum
    {
        NoError,            ///< Search completed successfully.
        ConnectionError,    ///< Network or API connection failed.
        ObjectNotFound,     ///< The requested NORAD ID does not exist in the source.
        ObjectDecayed,      ///< The object has re-entered the atmosphere.
        AltitudeIssue       ///< Object altitude is invalid or out of range.
    };

    /**
     * @brief Structure wrapping an error code and a descriptive message.
     */
    struct SatelliteSearchEngineError
    {
        /**
         * @brief Full constructor.
         * @param n Error ID (from ErrorEnum).
         * @param s Descriptive error string.
         */
        SatelliteSearchEngineError(const int& n, const QString s) : id(n), string(s) {}

        /**
         * @brief Default constructor (No Error).
         */
        SatelliteSearchEngineError() : id(ErrorEnum::NoError), string("") {}

        int id;         ///< The numeric error code.
        QString string; ///< The human-readable error message.
    };

    /**
     * @brief Searches for a space object by its NORAD ID.
     *
     * This is a pure virtual function that must be implemented by the specific plugin.
     * It populates the passed `SpaceObject` reference with the found data.
     *
     * @param norad The NORAD ID string to search for.
     * @param object Output parameter. The plugin will fill this object with the retrieved data.
     * @return SatelliteSearchEngineError indicating success or failure.
     */
    virtual SatelliteSearchEngineError searchSpaceObjectByNorad(const QString& norad, SpaceObject& object) = 0;
};

// Qt Interface Identification Macros
QT_BEGIN_NAMESPACE
/**
 * @brief Unique Interface Identifier (IID) for Qt's plugin system.
 * This string MUST match exactly what the plugins export.
 */
#define SpaceObjectSearchEngine_iid "SALARA_PROJECT.Interface_SpaceObjectSearchEngine"
Q_DECLARE_INTERFACE(SpaceObjectSearchEngine, SpaceObjectSearchEngine_iid)
QT_END_NAMESPACE

Q_DECLARE_METATYPE(SpaceObjectSearchEngine*)
