#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <memory>
#include <nlohmann/json.hpp>


#include "interface_spaceobjectsearchengine.h"

class PluginManager : public QObject
{
    Q_OBJECT
public:
    static PluginManager& instance();

    void loadPlugins(const QString& directory = "");

    QStringList getLoadedEngines() const;
    nlohmann::json searchSpaceObject(int64_t noradId);

private:
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    QList<SpaceObjectSearchEngine*> m_searchEngines;
    nlohmann::json qJsonToNlohmann(const QJsonObject& qObj);
};

#endif
