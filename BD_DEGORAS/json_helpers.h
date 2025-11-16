#pragma once

#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <string>

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

