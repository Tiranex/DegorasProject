#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

#include "class_spaceobject.h"
#include "interface_plugin.h"

class SpaceObjectSearchEngine : public SPPlugin
{
public:

    SpaceObjectSearchEngine():SPPlugin(PluginCategory::SPACE_OBJECT_SEARCH_ENGINE){}
    virtual ~SpaceObjectSearchEngine() = default;

    enum ErrorEnum
    {
        NoError,
        ConnectionError,
        ObjectNotFound,
        ObjectDecayed,
        AltitudeIssue
    };

    struct SatelliteSearchEngineError
    {
        SatelliteSearchEngineError(const int& n, const QString s):id(n),string(s){}
        SatelliteSearchEngineError():id(ErrorEnum::NoError),string(""){}
        int id;
        QString string;
    };

    virtual SatelliteSearchEngineError searchSpaceObjectByNorad(const QString& norad, SpaceObject&) = 0;
};

QT_BEGIN_NAMESPACE
#define SpaceObjectSearchEngine_iid "SALARA_PROJECT.Interface_SpaceObjectSearchEngine"
Q_DECLARE_INTERFACE(SpaceObjectSearchEngine, SpaceObjectSearchEngine_iid)
QT_END_NAMESPACE

Q_DECLARE_METATYPE(SpaceObjectSearchEngine*)
