#pragma once

#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <string>
#include <QString>
#include <QStringList>
/**
 * @brief Convierte una vista BSON CXX a nlohmann::json (vía Extended JSON).
 * @param view Vista BSON.
 * @return Objeto nlohmann::json.
 */
static inline nlohmann::json bsoncxxToNjson(const bsoncxx::document::view& view)
{
    const std::string s = bsoncxx::to_json(view);
    try
    {
        return nlohmann::json::parse(s);
    }
    catch (...)
    {
        // Devuelve un JSON nulo/vacío si hay error de parseo
        return nlohmann::json{};
    }
}

/**
 * @brief Convierte nlohmann::json a un documento BSON (vía Extended JSON).
 * @param j Objeto JSON.
 * @return BSON document::value (propietario).
 */
static inline bsoncxx::document::value njsonToBsoncxx(const nlohmann::json& j)
{
    const std::string dumped = j.dump();
    return bsoncxx::from_json(dumped);
}

/**
 * @brief Convierte de forma segura un valor JSON a un QString.
 */
QString jsonValueToQString(const nlohmann::json& value)
{
    if (value.is_string()) {
        return QString::fromStdString(value.get<std::string>());
    }
    if (value.is_number()) {
        return QString::number(value.get<double>());
    }
    if (value.is_boolean()) {
        return value.get<bool>() ? "1" : "0";
    }
    if (value.is_null()) {
        return "null";
    }

    // ¡NUEVO! Manejar Arrays (para el campo "Groups")
    if (value.is_array()) {
        QStringList list;
        for (const auto& item : value) {
            if (item.is_string()) {
                list.append(QString::fromStdString(item.get<std::string>()));
            }
        }
        return list.join(", "); // Devuelve "Grupo1, Grupo2"
    }

    return QString();
}
