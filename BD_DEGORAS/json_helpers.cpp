#include "json_helpers.h"
#include <QStringList>

QString jsonValueToQString(const nlohmann::json& value)
{
    if (value.is_string()) {
        return QString::fromStdString(value.get<std::string>());
    }
    if (value.is_number()) {
        return QString::number(value.get<double>());
    }
    if (value.is_boolean()) {
        // Return 1/0 for boolean to match Qt checkbox logic often used in tables
        return value.get<bool>() ? "1" : "0";
    }
    if (value.is_null()) {
        return "null";
    }
    if (value.is_array()) {
        QStringList list;
        // Iterate through array items
        for (const auto& item : value) {
            // Currently only supports flat arrays of strings (e.g., Groups, Sets)
            if (item.is_string()) {
                list.append(QString::fromStdString(item.get<std::string>()));
            }
        }
        // Join with comma for display in a single table cell
        return list.join(", ");
    }

    // Return empty string for Objects or unknown types to avoid crashes
    return QString();
}
