#include "stdsearchengine.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>
#include <QDateTime>

StdSearchEngine::StdSearchEngine()
{
    QJsonObject meta;
    QJsonObject data;
    data["Name"] = "CelesTrak Catalog Engine (UI Synced)";
    data["ShortName"] = "CelesTrak";
    data["Version"] = "1.5.0";
    data["Copyright"] = "Degoras Project 2025";
    meta["MetaData"] = data;
    this->setMetaData(meta);

    m_networkManager = new QNetworkAccessManager(this);
    this->setEnabled(true);
}

SpaceObjectSearchEngine::SatelliteSearchEngineError StdSearchEngine::searchSpaceObjectByNorad(const QString& norad, SpaceObject& resultObject)
{
    // Usamos el catálogo 'satcat'
    QUrl url(QString("https://celestrak.org/satcat/records.php?CATNR=%1&FORMAT=json").arg(norad));
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errStr = reply->errorString();
        reply->deleteLater();
        return SatelliteSearchEngineError(ErrorEnum::ConnectionError, "Network Error: " + errStr);
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty()) {
        return SatelliteSearchEngineError(ErrorEnum::ObjectNotFound, "Object not found in CelesTrak Catalog.");
    }

    QJsonObject satData = doc.array().first().toObject();

    // =========================================================================
    // MAPEO 1:1 CON LA INTERFAZ GRÁFICA (UI)
    // =========================================================================

    // 1. Norad
    resultObject.setNorad(norad);

    // 2. Name
    resultObject.setName(satData["OBJECT_NAME"].toString().trimmed());

    // 3. Alias (Ahora existe setAlias en el .h)
    // Ponemos el País/Dueño aquí, es lo más útil
    if (satData.contains("COUNTRY")) {
        resultObject.setAlias(satData["COUNTRY"].toString());
    }

    // 4. COSPAR
    resultObject.setCospar(satData["OBJECT_ID"].toString());

    // 5. Classification
    resultObject.setClassification(satData["OBJECT_TYPE"].toString());

    // 6. RCS (Radar Cross Section)
    if (satData.contains("RCS")) {
        resultObject.setRadarCrossSection(satData["RCS"].toDouble(0.0));
    }

    // 7. Inc (Inclination)
    resultObject.setInclination(satData["INCLINATION"].toDouble(0.0));

    // 8. Altitude (Tu clase pide 'int', hacemos conversión)
    double apogee = satData["APOGEE"].toDouble(0.0);
    double perigee = satData["PERIGEE"].toDouble(0.0);
    if (apogee > 0 && perigee > 0) {
        int avgAlt = static_cast<int>((apogee + perigee) / 2.0);
        resultObject.setAltitude(avgAlt);
    } else {
        resultObject.setAltitude(0);
    }

    // 9. Is Debris
    QString objType = satData["OBJECT_TYPE"].toString();
    bool isDebris = (objType != "PAYLOAD");
    resultObject.setIsDebris(isDebris);

    // 10. Comments (Ahora existe setComments en el .h)
    // Construimos un texto rico con la info de lanzamiento y estado
    QString launchDate = satData["LAUNCH_DATE"].toString();
    QString launchSite = satData["LAUNCH_SITE"].toString();
    QString status = satData["OPS_STATUS_CODE"].toString(); // +, -, P, D

    // Desciframos el estado para que sea legible en el cuadro de texto
    QString statusText = "Unknown";
    if (status == "+") statusText = "Active";
    else if (status == "-") statusText = "Inactive";
    else if (status == "D") statusText = "Decayed";

    QString commentText = QString("Status: %1 (%2)\nLaunch Date: %3\nLaunch Site: %4")
                              .arg(statusText, status, launchDate, launchSite);

    resultObject.setComments(commentText);


    // =========================================================================
    // VALORES POR DEFECTO PARA LA UI
    // =========================================================================
    resultObject.setPicture("default.png");
    resultObject.setTrackPolicy(SpaceObject::TrackPolicy::TRACK_ALWAYS);
    resultObject.setEnablementPolicy(SpaceObject::EnablementPolicy::ENABLED);

    // Estos campos no los da CelesTrak, los ponemos vacíos o por defecto
    resultObject.setHasLRR(false);
    resultObject.setInitialized(true);

    return SatelliteSearchEngineError(ErrorEnum::NoError, "");
}
