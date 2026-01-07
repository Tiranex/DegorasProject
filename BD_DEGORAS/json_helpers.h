#pragma once

#include <nlohmann/json.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <string>
#include <QString>
#include <QStringList>

/**
 * @brief Converts a BSON document view (MongoDB) to a nlohmann::json object.
 *
 * This utility bridges the gap between the MongoDB C++ driver (which uses BSON)
 * and the application logic (which uses nlohmann::json).
 * It works by serializing BSON to an "Extended JSON" string and re-parsing it.
 *
 * @param view The BSON view returned by a MongoDB query.
 * @return nlohmann::json The equivalent JSON object. Returns empty on failure.
 */
static inline nlohmann::json bsoncxxToNjson(const bsoncxx::document::view& view)
{
    // Convert binary BSON -> String JSON
    const std::string s = bsoncxx::to_json(view);
    try
    {
        // Parse String JSON -> nlohmann::json object
        return nlohmann::json::parse(s);
    }
    catch (...)
    {
        return nlohmann::json{};
    }
}

/**
 * @brief Converts a nlohmann::json object back to a BSON document value.
 *
 * Used when saving application data back to MongoDB.
 *
 * @param j The JSON object to convert.
 * @return bsoncxx::document::value A binary BSON document ready for insertion.
 */
static inline bsoncxx::document::value njsonToBsoncxx(const nlohmann::json& j)
{
    const std::string dumped = j.dump();
    return bsoncxx::from_json(dumped);
}

/**
 * @brief robustly converts a JSON value into a human-readable QString.
 *
 * This function handles various JSON types (String, Number, Bool, Array)
 * to ensure they can be displayed safely in UI widgets like QTableWidget.
 *
 * @param value The JSON value to convert.
 * @return QString representation of the value.
 */
QString jsonValueToQString(const nlohmann::json& value);
