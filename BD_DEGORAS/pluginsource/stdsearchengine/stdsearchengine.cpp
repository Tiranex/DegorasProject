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




    resultObject.setNorad(norad);

    resultObject.setName(satData["OBJECT_NAME"].toString().trimmed());

    if (satData.contains("COUNTRY")) {
        resultObject.setAlias(satData["COUNTRY"].toString());
    }
    resultObject.setCospar(satData["OBJECT_ID"].toString());

    resultObject.setClassification(satData["OBJECT_TYPE"].toString());
    if (satData.contains("RCS")) {
        resultObject.setRadarCrossSection(satData["RCS"].toDouble(0.0));
    }

    resultObject.setInclination(satData["INCLINATION"].toDouble(0.0));
    double apogee = satData["APOGEE"].toDouble(0.0);
    double perigee = satData["PERIGEE"].toDouble(0.0);
    if (apogee > 0 && perigee > 0) {
        int avgAlt = static_cast<int>((apogee + perigee) / 2.0);
        resultObject.setAltitude(avgAlt);
    } else {
        resultObject.setAltitude(0);
    }

    QString objType = satData["OBJECT_TYPE"].toString();
    bool isDebris = (objType != "PAYLOAD");
    resultObject.setIsDebris(isDebris);
    QString launchDate = satData["LAUNCH_DATE"].toString();
    QString launchSite = satData["LAUNCH_SITE"].toString();
    QString status = satData["OPS_STATUS_CODE"].toString(); // +, -, P, D

    QString statusText = "Unknown";
    if (status == "+") statusText = "Active";
    else if (status == "-") statusText = "Inactive";
    else if (status == "D") statusText = "Decayed";

    QString commentText = QString("Status: %1 (%2)\nLaunch Date: %3\nLaunch Site: %4")
                              .arg(statusText, status, launchDate, launchSite);

    resultObject.setComments(commentText);

    resultObject.setPicture("default.png");
    resultObject.setTrackPolicy(SpaceObject::TrackPolicy::TRACK_ALWAYS);
    resultObject.setEnablementPolicy(SpaceObject::EnablementPolicy::ENABLED);

    resultObject.setHasLRR(false);
    resultObject.setInitialized(true);

    return SatelliteSearchEngineError(ErrorEnum::NoError, "");
}
