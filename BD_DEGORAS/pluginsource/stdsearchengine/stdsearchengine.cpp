#include "stdsearchengine.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

StdSearchEngine::StdSearchEngine()
{
    // Definimos los metadatos que esperaba el sistema legacy
    QJsonObject meta;
    QJsonObject data;
    data["Name"] = "CelesTrak Standard Engine";
    data["ShortName"] = "CelesTrak";
    data["Version"] = "1.0.0";
    data["Copyright"] = "Degoras Project 2025";
    meta["MetaData"] = data;
    this->setMetaData(meta);

    // Inicializamos network y estado
    m_networkManager = new QNetworkAccessManager(this);
    this->setEnabled(true);
}

SpaceObjectSearchEngine::SatelliteSearchEngineError StdSearchEngine::searchSpaceObjectByNorad(const QString& norad, SpaceObject& resultObject)
{
    // 1. Construir la URL de CelesTrak (Formato JSON GP)
    // Devuelve datos orbitales completos dado un NORAD ID
    QUrl url(QString("https://celestrak.org/NORAD/elements/gp.php?CATNR=%1&FORMAT=json").arg(norad));
    QNetworkRequest request(url);

    // 2. Realizar petición SÍNCRONA (esperamos respuesta)
    // Usamos QEventLoop porque la interfaz legacy espera un return directo, no señales.
    QNetworkReply* reply = m_networkManager->get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(); // Bloquea aquí hasta que internet responda

    // 3. Comprobar errores de red
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return SatelliteSearchEngineError(ErrorEnum::ConnectionError, "Network Error: " + reply->errorString());
    }

    // 4. Parsear JSON
    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty()) {
        return SatelliteSearchEngineError(ErrorEnum::ObjectNotFound, "Object not found or invalid JSON.");
    }

    // CelesTrak devuelve un array, cogemos el primero
    QJsonObject satData = doc.array().first().toObject();

    // 5. Rellenar el objeto SpaceObject (Legacy)
    // Mapeamos los campos del JSON a tu clase C++
    resultObject.setNorad(norad);
    resultObject.setName(satData["OBJECT_NAME"].toString());
    resultObject.setCospar(satData["OBJECT_ID"].toString()); // Ejemplo: 1998-067A
    resultObject.setInclination(satData["INCLINATION"].toDouble());
    resultObject.setRadarCrossSection(satData["RCS"].toDouble()); // A veces viene, a veces no

    // Ajustes por defecto requeridos por la UI
    resultObject.setPicture("default.png");
    resultObject.setTrackPolicy(SpaceObject::TrackPolicy::TRACK_ALWAYS);
    resultObject.setEnablementPolicy(SpaceObject::EnablementPolicy::ENABLED);
    
    // Si quisieras usar N2YO, solo cambiarías la URL arriba y el mapeo de campos aquí.
    
    return SatelliteSearchEngineError(ErrorEnum::NoError, "");
}