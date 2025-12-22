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
        return nlohmann::json{};
    }
}

/**
 * @brief Convierte nlohmann::json a un documento BSON (vía Extended JSON).
 */
static inline bsoncxx::document::value njsonToBsoncxx(const nlohmann::json& j)
{
    const std::string dumped = j.dump();
    return bsoncxx::from_json(dumped);
}

/**
 * @brief Convierte de forma segura un valor JSON a un QString.
 * (SOLO DECLARACIÓN, la definición va en json_helpers.cpp)
 */
QString jsonValueToQString(const nlohmann::json& value);
