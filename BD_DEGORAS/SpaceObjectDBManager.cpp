#include "SpaceObjectDBManager.h"
#include "json_helpers.h" // ¡IMPORTANTE! Para bsoncxxToNjson y njsonToBsoncxx
#include <iostream>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <fstream>
#include <sstream>

#include <bsoncxx/builder/basic/array.hpp>
#include <set>

// --- CONSTRUCTOR ---
SpaceObjectDBManager::SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name)
    : _client(mongocxx::uri{uri_str}),
    _db(_client[db_name]),
    _collection(_db[col_name]),
    // IMPORTANTE: Elimina _gridfsBucket y _gridfsFilesCollection si estaban aquí.
    _groupsCollection(_db["groups"]),
    _imageManager(_db) // Asegúrate de que este miembro se inicialice al final.
{
    try {
        _db.run_command(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1)));
        std::cout << "[Info] SpaceObjectDBManager conectado exitosamente..." << std::endl;
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla al inicializar SpaceObjectDBManager: " << ex.what() << std::endl;
        throw;
    }
}

// --- DESTRUCTOR ---
SpaceObjectDBManager::~SpaceObjectDBManager() {
    std::cout << "[Info] SpaceObjectDBManager desconectado." << std::endl;
}

// --- GET POR ID ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectById(int64_t id)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::types::b_int64{id}));
        auto result = _collection.find_one(filter.view());
        return result ? bsoncxxToNjson(result->view()) : nlohmann::json{};
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getSpaceObjectById: " << ex.what() << std::endl;
        return nlohmann::json{};
    }
}

// --- GET POR NOMBRE ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectByName(const std::string& name)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("Name", name));
        auto result = _collection.find_one(filter.view());
        return result ? bsoncxxToNjson(result->view()) : nlohmann::json{};
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getSpaceObjectByName: " << ex.what() << std::endl;
        return nlohmann::json{};
    }
}

// --- GET POR PICTURE ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectByPicture(const std::string& picName)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("Picture", picName));
        auto result = _collection.find_one(filter.view());
        return result ? bsoncxxToNjson(result->view()) : nlohmann::json{};
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getSpaceObjectByPicture: " << ex.what() << std::endl;
        return nlohmann::json{};
    }
}


// En SpaceObjectDBManager.cpp

bool SpaceObjectDBManager::createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg)
{
    // --- SEGURIDAD ANTI-CRASH ---
    // Envolvemos todo en un try-catch para evitar que el programa se cierre
    // si intentamos leer un null como string.
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        errorMsg = "";

        // --- 1. VALIDACIÓN: _id y Name ---
        if (!objectData.contains("_id") || objectData["_id"].is_null()) {
            errorMsg = "Error: El campo '_id' (NORAD) es obligatorio y no puede ser nulo.";
            return false;
        }
        int64_t id = objectData["_id"];

        nlohmann::json existing_id = getSpaceObjectById(id);
        if (!existing_id.empty() && !existing_id.is_null()) {
            errorMsg = "Ya existe un objeto con el NORAD (_id): " + std::to_string(id);
            return false;
        }

        if (!objectData.contains("Name") || objectData["Name"].is_null()) {
            errorMsg = "Error: El campo 'Name' es obligatorio.";
            return false;
        }
        std::string name = objectData["Name"];
        if (name.empty()) {
            errorMsg = "Error: El campo 'Name' no puede estar vacío.";
            return false;
        }

        nlohmann::json existing_name = getSpaceObjectByName(name);
        if (!existing_name.empty() && !existing_name.is_null()) {
            errorMsg = "Ya existe un objeto con el Nombre: " + name;
            return false;
        }

        // --- 2. VALIDACIONES DE UNICIDAD (Alias, COSPAR, ILRS, SIC) ---

        // Alias (Abbreviation)
        if (objectData.contains("Abbreviation") && !objectData["Abbreviation"].is_null()) {
            std::string val = objectData["Abbreviation"];
            auto filter = make_document(kvp("Abbreviation", val));
            if (_collection.count_documents(filter.view()) > 0) {
                errorMsg = "Ya existe un objeto con el Alias: " + val;
                return false;
            }
        }

        // COSPAR (Obligatorio y Único)
        if (!objectData.contains("COSPAR") || objectData["COSPAR"].is_null()) {
            errorMsg = "Error: El campo COSPAR es obligatorio.";
            return false;
        }
        std::string cosparVal = objectData["COSPAR"];
        if(cosparVal.empty()) {
            errorMsg = "Error: El COSPAR no puede estar vacío.";
            return false;
        }
        auto filterCospar = make_document(kvp("COSPAR", cosparVal));
        if (_collection.count_documents(filterCospar.view()) > 0) {
            errorMsg = "Ya existe un objeto con el COSPAR: " + cosparVal;
            return false;
        }

        // ILRSID (Opcional pero Único si existe)
        if (objectData.contains("ILRSID") && !objectData["ILRSID"].is_null()) {
            std::string val = objectData["ILRSID"];
            if (!val.empty()) { // Solo chequeamos si tiene texto
                auto filter = make_document(kvp("ILRSID", val));
                if (_collection.count_documents(filter.view()) > 0) {
                    errorMsg = "Ya existe un objeto con el ILRS ID: " + val;
                    return false;
                }
            }
        }

        // SIC (Opcional pero Único si existe)
        if (objectData.contains("SIC") && !objectData["SIC"].is_null()) {
            std::string val = objectData["SIC"];
            if (!val.empty()) {
                auto filter = make_document(kvp("SIC", val));
                if (_collection.count_documents(filter.view()) > 0) {
                    errorMsg = "Ya existe un objeto con el SIC: " + val;
                    return false;
                }
            }
        }

        // --- 3. GESTIÓN DE IMAGEN (AQUÍ SOLÍA ESTAR EL CRASH) ---
        std::string picName = "";

        // ¡FIX!: Comprobamos !is_null() antes de asignar
        if (objectData.contains("Picture") && !objectData["Picture"].is_null()) {
            picName = objectData["Picture"];
        }

        if (!picName.empty()) {
            // A. Unicidad
            nlohmann::json existing_pic = getSpaceObjectByPicture(picName);
            if (!existing_pic.empty() && !existing_pic.is_null()) {
                errorMsg = "Ya existe un objeto usando la imagen: " + picName;
                return false;
            }
            // B. Ruta local
            if (localPicturePath.empty()) {
                errorMsg = "Se especificó nombre de imagen pero no se seleccionó archivo.";
                return false;
            }
            // C. Leer archivo
            std::ifstream file(localPicturePath, std::ios::binary);
            if (!file.is_open()) {
                errorMsg = "No se pudo leer el archivo de imagen local.";
                return false;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string imageData = buffer.str();
            file.close();

            // D. Subir
            if (!_imageManager.uploadImage(picName, imageData)) {
                errorMsg = "Falló la subida de la imagen a GridFS.";
                return false;
            }
        }

        // --- 4. GRUPOS (Upsert) ---
        if (objectData.contains("Groups") && !objectData["Groups"].is_null() && objectData["Groups"].is_array()) {
            mongocxx::options::update upsert_options;
            upsert_options.upsert(true);
            for (const auto& groupNameJson : objectData["Groups"]) {
                if (groupNameJson.is_string()) {
                    std::string gName = groupNameJson.get<std::string>();
                    if(!gName.empty()){
                        try {
                            auto filter = make_document(kvp("name", gName));
                            auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", gName))));
                            _groupsCollection.update_one(filter.view(), update.view(), upsert_options);
                        } catch(...) {}
                    }
                }
            }
        }

        // --- 5. INSERCIÓN FINAL ---
        bsoncxx::document::value bdoc = njsonToBsoncxx(objectData);
        auto result = _collection.insert_one(bdoc.view());
        return result.has_value();

    }
    catch (const nlohmann::json::exception& e) {
        // AQUÍ CAPTURAMOS EL CRASH si intentas leer un null como string
        errorMsg = "Error de formato JSON (Posible campo nulo no controlado): " + std::string(e.what());
        std::cerr << "[CRASH EVITADO] " << errorMsg << std::endl;
        return false;
    }
    catch (const mongocxx::exception& ex) {
        errorMsg = "Error de Base de Datos: " + std::string(ex.what());
        std::cerr << "[Mongo Error] " << errorMsg << std::endl;
        return false;
    }
    catch (const std::exception& ex) {
        errorMsg = "Error genérico: " + std::string(ex.what());
        return false;
    }
}


//MODIFICA UN SPACEOBJECT SELECCIONADO
//MANTIENE LOS DATOS EN LA PESTAÑA EMERGENTE Y LOS MODIFICA
bool SpaceObjectDBManager::updateSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg)
{
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        errorMsg = "";

        // 1. OBTENER EL ID (Es la clave para saber a quién actualizar)
        if (!objectData.contains("_id") || objectData["_id"].is_null()) {
            errorMsg = "Error interno: El objeto a editar no tiene _id.";
            return false;
        }
        int64_t id = objectData["_id"];

        // Verificar que el objeto existe realmente
        nlohmann::json existing_obj = getSpaceObjectById(id);
        if (existing_obj.empty() || existing_obj.is_null()) {
            errorMsg = "Error: No se encontró el objeto original con _id: " + std::to_string(id);
            return false;
        }

        // =================================================================
        // 2. VALIDACIONES DE UNICIDAD (IGNORANDO AL PROPIO OBJETO)
        // =================================================================
        // La lógica es: Busco si alguien tiene ese Nombre/COSPAR...
        // Si lo encuentro Y su _id NO es el mío -> ERROR (es un duplicado).

        // Helper Lambda para validar unicidad excluyendo self
        auto checkUniqueExceptSelf = [&](const std::string& field, const std::string& value, const std::string& fieldNameErr) -> bool {
            auto filter = make_document(
                kvp(field, value),
                kvp("_id", make_document(kvp("$ne", bsoncxx::types::b_int64{id}))) // $ne = Not Equal
                );
            if (_collection.count_documents(filter.view()) > 0) {
                errorMsg = "Ya existe OTRO objeto con el " + fieldNameErr + ": " + value;
                return false;
            }
            return true;
        };

        // Validar Name
        if (!checkUniqueExceptSelf("Name", objectData["Name"], "Nombre")) return false;

        // Validar COSPAR
        if (!checkUniqueExceptSelf("COSPAR", objectData["COSPAR"], "COSPAR")) return false;

        // Validar Alias (Si existe)
        if (objectData.contains("Abbreviation") && !objectData["Abbreviation"].is_null()) {
            if(!checkUniqueExceptSelf("Abbreviation", objectData["Abbreviation"], "Alias")) return false;
        }

        // Validar ILRSID (Si existe)
        if (objectData.contains("ILRSID") && !objectData["ILRSID"].is_null()) {
            std::string val = objectData["ILRSID"];
            if(!val.empty()) if(!checkUniqueExceptSelf("ILRSID", val, "ILRS ID")) return false;
        }

        // Validar SIC (Si existe)
        if (objectData.contains("SIC") && !objectData["SIC"].is_null()) {
            std::string val = objectData["SIC"];
            if(!val.empty()) if(!checkUniqueExceptSelf("SIC", val, "SIC")) return false;
        }


        // =================================================================
        // 3. GESTIÓN DE IMAGEN (GridFS)
        // =================================================================
        // Lógica: Si el usuario seleccionó una foto NUEVA (localPicturePath no vacío),
        // borramos la vieja (si había) y subimos la nueva.
        // Si no seleccionó foto nueva, mantenemos la referencia actual.

        std::string finalPicName = "";
        if (objectData.contains("Picture") && !objectData["Picture"].is_null()) {
            finalPicName = objectData["Picture"];
        }

        if (!localPicturePath.empty()) {
            // A. El usuario quiere cambiar la foto. Validar nombre nuevo.
            auto filterPic = make_document(
                kvp("Picture", finalPicName),
                kvp("_id", make_document(kvp("$ne", bsoncxx::types::b_int64{id})))
                );
            if (_collection.count_documents(filterPic.view()) > 0) {
                errorMsg = "Ya existe otro objeto usando la imagen: " + finalPicName;
                return false;
            }

            // B. Borrar foto antigua de GridFS (si tenía una diferente)
            std::string oldPic = "";
            if(existing_obj.contains("Picture") && !existing_obj["Picture"].is_null())
                oldPic = existing_obj["Picture"];

            // Subir nueva
            std::ifstream file(localPicturePath, std::ios::binary);
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (!_imageManager.uploadImage(finalPicName, buffer.str())) {
                errorMsg = "Error al subir la nueva imagen.";
                return false;
            }

            // Si subida OK, borramos la vieja si era distinta
            if(!oldPic.empty() && oldPic != finalPicName) {
                _imageManager.deleteImageByName(oldPic);
            }
        }
        else {
            // No hay foto nueva local. Mantenemos la que viene en el JSON
            // (que debería ser la misma que ya tenía, a menos que quieras borrarla)
        }


        // =================================================================
        // 4. ACTUALIZAR GRUPOS (Upsert de nombres en colección groups)
        // =================================================================
        if (objectData.contains("Groups") && !objectData["Groups"].is_null() && objectData["Groups"].is_array()) {
            mongocxx::options::update upsert_options;
            upsert_options.upsert(true);
            for (const auto& groupNameJson : objectData["Groups"]) {
                if (groupNameJson.is_string()) {
                    std::string gName = groupNameJson.get<std::string>();
                    if(!gName.empty()){
                        try {
                            auto filter = make_document(kvp("name", gName));
                            auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", gName))));
                            _groupsCollection.update_one(filter.view(), update.view(), upsert_options);
                        } catch(...) {}
                    }
                }
            }
        }

        // =================================================================
        // 5. UPDATE FINAL (Reemplazar documento)
        // =================================================================
        // Usamos replace_one para sobrescribir todo el documento con los nuevos datos
        bsoncxx::document::value bdoc = njsonToBsoncxx(objectData);

        auto filter = make_document(kvp("_id", bsoncxx::types::b_int64{id}));
        auto result = _collection.replace_one(filter.view(), bdoc.view());

        if(result && result->modified_count() >= 0) return true;
        else {
            errorMsg = "No se modificó el documento (quizás los datos eran idénticos).";
            return true; // Consideramos éxito si no hubo error, aunque no cambiara nada
        }

    } catch (const std::exception& ex) {
        errorMsg = "Excepción en update: " + std::string(ex.what());
        return false;
    }
}







// --- BORRAR POR ID (Mismo código) ---
bool SpaceObjectDBManager::deleteSpaceObjectById(int64_t id)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::types::b_int64{id}));
        auto result = _collection.delete_one(filter.view());
        return result && result->deleted_count() > 0;
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en deleteSpaceObjectById: " << ex.what() << std::endl;
        return false;
    }
}

// --- GET ALL (Mismo código) ---
std::vector<nlohmann::json> SpaceObjectDBManager::getAllSpaceObjects()
{
    std::vector<nlohmann::json> allObjects;

    try {
        mongocxx::cursor cursor = _collection.find({});

        for (bsoncxx::document::view doc : cursor) {
            allObjects.push_back(bsoncxxToNjson(doc));
        }
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getAllSpaceObjects: " << ex.what() << std::endl;
    }

    return allObjects;
}


// --- MÉTODOS DE GRIDFS DELEGADOS (DEBES BORRARLOS) ---
// Comentaste esta sección, ¡asegúrate de que esté completamente eliminada!
// Si la dejas comentada, no causa el error, pero la limpieza es total.


// =================================================================
// --- ¡NUEVO! MÉTODOS DE GRUPOS (Mismo código) ---
// =================================================================

std::set<std::string> SpaceObjectDBManager::getAllUniqueGroupNames()
{
    std::set<std::string> groups;
    try {
        mongocxx::cursor cursor = _groupsCollection.find({});
        for (bsoncxx::document::view doc : cursor) {
            if (doc["name"] && doc["name"].type() == bsoncxx::type::k_string)
            {
                std::string_view name_view = doc["name"].get_string().value;
                groups.insert(std::string(name_view));
            }
        }
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getAllUniqueGroupNames: " << ex.what() << std::endl;
    }
    return groups;
}

std::vector<nlohmann::json> SpaceObjectDBManager::getSpaceObjectsByGroups(const std::set<std::string>& groupNames)
{
    std::vector<nlohmann::json> objects;
    if (groupNames.empty()) return objects;

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::make_array;

    bsoncxx::builder::basic::array group_array;
    for(const auto& name : groupNames) {
        group_array.append(name);
    }

    try {
        auto filter = make_document(kvp("Groups", make_document(kvp("$in", group_array))));
        mongocxx::cursor cursor = _collection.find(filter.view());

        for (bsoncxx::document::view doc : cursor) {
            objects.push_back(bsoncxxToNjson(doc));
        }
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getSpaceObjectsByGroups: " << ex.what() << std::endl;
    }
    return objects;
}


bool SpaceObjectDBManager::addObjectToGroup(int64_t objectId, const std::string& groupName)
{
    if (groupName.empty()) {
        std::cerr << "[Error] El nombre del grupo no puede estar vacío." << std::endl;
        return false;
    }

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        mongocxx::options::update upsert_options;
        upsert_options.upsert(true);
        auto group_filter = make_document(kvp("name", groupName));
        auto group_update = make_document(kvp("$setOnInsert", make_document(kvp("name", groupName))));
        _groupsCollection.update_one(group_filter.view(), group_update.view(), upsert_options);

        auto obj_filter = make_document(kvp("_id", bsoncxx::types::b_int64{objectId}));
        auto obj_update = make_document(kvp("$addToSet", make_document(kvp("Groups", groupName))));

        auto result = _collection.update_one(obj_filter.view(), obj_update.view());

        if (!result || result->matched_count() == 0) {
            std::cerr << "[Error] No se encontró el objeto con _id: " << objectId << " para añadir grupo." << std::endl;
            return false;
        }

        return true;

    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en addObjectToGroup: " << ex.what() << std::endl;
        return false;
    }
}


bool SpaceObjectDBManager::removeObjectFromGroup(int64_t objectId, const std::string& groupName)
{
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        auto obj_filter = make_document(kvp("_id", bsoncxx::types::b_int64{objectId}));
        auto obj_update = make_document(kvp("$pull", make_document(kvp("Groups", groupName))));

        auto result = _collection.update_one(obj_filter.view(), obj_update.view());

        if (!result || result->matched_count() == 0) {
            std::cerr << "[Error] No se encontró el objeto con _id: " << objectId << " para quitar grupo." << std::endl;
            return false;
        }

        return true;

    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en removeObjectFromGroup: " << ex.what() << std::endl;
        return false;
    }
}

bool SpaceObjectDBManager::crearGrupo(const std::string& groupName)
{
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    if (groupName.empty()) {
        std::cerr << "[Error] El nombre del grupo no puede estar vacío." << std::endl;
        return false;
    }

    try {
        auto filter = make_document(kvp("name", groupName));
        if (_groupsCollection.find_one(filter.view())) {
            std::cerr << "[Info] El grupo '" << groupName << "' ya existe." << std::endl;
            return false;
        }

        auto doc = make_document(kvp("name", groupName));
        auto result = _groupsCollection.insert_one(doc.view());
        return result.has_value();

    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en crearGrupo: " << ex.what() << std::endl;
        return false;
    }
}

bool SpaceObjectDBManager::eliminarGrupo(const std::string& groupName)
{
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        auto filter_group = make_document(kvp("name", groupName));
        auto result_group = _groupsCollection.delete_one(filter_group.view());

        if (!result_group || result_group->deleted_count() == 0) {
            std::cerr << "[Info] No se encontró el grupo '" << groupName << "' para eliminar." << std::endl;
        }

        auto filter_objects = make_document(kvp("Groups", groupName));
        auto update_objects = make_document(kvp("$pull", make_document(kvp("Groups", groupName))));

        _collection.update_many(filter_objects.view(), update_objects.view());

        std::cout << "[Info] Grupo '" << groupName << "' eliminado y referencias limpiadas." << std::endl;
        return true;

    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en eliminarGrupo: " << ex.what() << std::endl;
        return false;
    }
}
