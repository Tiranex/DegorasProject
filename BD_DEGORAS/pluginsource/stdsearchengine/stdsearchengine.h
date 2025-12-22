#ifndef STDSEARCHENGINE_H
#define STDSEARCHENGINE_H

#include <QObject>
#include <QNetworkAccessManager>
#include "interface_spaceobjectsearchengine.h"

class StdSearchEngine : public SpaceObjectSearchEngine
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID SpaceObjectSearchEngine_iid FILE "stdsearchengine.json")
    Q_INTERFACES(SpaceObjectSearchEngine)

public:
    StdSearchEngine();
    ~StdSearchEngine() override = default;

    SatelliteSearchEngineError searchSpaceObjectByNorad(const QString& norad, SpaceObject& resultObject) override;

private:
    QNetworkAccessManager* m_networkManager;
};

#endif // STDSEARCHENGINE_H
