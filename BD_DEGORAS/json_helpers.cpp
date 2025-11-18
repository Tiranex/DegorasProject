// --- json_helpers.cpp: CÓDIGO CORREGIDO ---

#include "json_helpers.h"
#include <QStringList> // Necesario para QStringList


// !!! ESTOS CUERPOS FUERON ELIMINADOS:
// static inline nlohmann::json bsoncxxToNjson(...) { ... }
// static inline bsoncxx::document::value njsonToBsoncxx(...) { ... }


/**
 * @brief Convierte de forma segura un valor JSON a un QString.
 * (Esta es la ÚNICA definición que debe estar aquí).
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

    // Manejar Arrays (para el campo "Groups")
    if (value.is_array()) {
        QStringList list;
        for (const auto& item : value) {
            if (item.is_string()) {
                list.append(QString::fromStdString(item.get<std::string>()));
            }
        }
        return list.join(", ");
    }

    return QString();
}
