#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <memory>
#include <nlohmann/json.hpp>

// Includes Legacy
#include "interface_spaceobjectsearchengine.h"

class PluginManager : public QObject
{
    Q_OBJECT
public:
    static PluginManager& instance();

    // Carga plugins desde una ruta (o "plugins/" por defecto)
    void loadPlugins(const QString& directory = "");

    // Devuelve los nombres de los motores cargados
    QStringList getLoadedEngines() const;

    // LA FUNCIÓN MÁGICA: Busca usando los plugins y devuelve JSON moderno
    // Retorna un JSON vacío {} si no encuentra nada.
    nlohmann::json searchSpaceObject(int64_t noradId);

private:
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Lista de plugins cargados y válidos
    QList<SpaceObjectSearchEngine*> m_searchEngines;

    // Helper para convertir QJsonObject (Legacy) -> nlohmann::json (Moderno)
    nlohmann::json qJsonToNlohmann(const QJsonObject& qObj);
};

#endif // PLUGINMANAGER_H
