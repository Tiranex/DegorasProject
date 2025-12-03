#include "SpaceObjectDBManager.h"
#include "json_helpers.h"
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <fstream>
#include <sstream>
#include <set>
#include <chrono>
#include <iomanip>

// LOGGING
#include <spdlog/spdlog.h>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;

// --- CONSTRUCTOR ---
SpaceObjectDBManager::SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name)
    : _client(mongocxx::uri{uri_str}),
    _db(_client[db_name]),
    _collection(_db[col_name]),
    _setsCollection(_db["sets"]),
    _groupsCollection(_db["groups"]),
    _versionsCollection(_db["versions"]), // <--- NUEVA COLECCIÓN
    _imageManager(_db)
{
    try {
        _db.run_command(make_document(kvp("ping", 1)));
        spdlog::info("SpaceObjectDBManager connected. Sets, Groups and Versions collections initialized.");
    } catch (const mongocxx::exception& ex) {
        spdlog::critical("Failed to initialize DB: {}", ex.what());
        throw;
    }
}

SpaceObjectDBManager::~SpaceObjectDBManager() {
    spdlog::info("SpaceObjectDBManager disconnected.");
}

// --- GET BY ID ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectById(int64_t id) {
    try {
        auto res = _collection.find_one(make_document(kvp("_id", bsoncxx::types::b_int64{id})));
        return res ? bsoncxxToNjson(res->view()) : nlohmann::json{};
    } catch (...) { return nlohmann::json{}; }
}

// --- GET BY NAME ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectByName(const std::string& name)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("Name", name));
        auto result = _collection.find_one(filter.view());
        return result ? bsoncxxToNjson(result->view()) : nlohmann::json{};
    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in getSpaceObjectByName: {}", ex.what());
        return nlohmann::json{};
    }
}

// --- GET BY PICTURE ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectByPicture(const std::string& picName)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("Picture", picName));
        auto result = _collection.find_one(filter.view());
        return result ? bsoncxxToNjson(result->view()) : nlohmann::json{};
    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in getSpaceObjectByPicture: {}", ex.what());
        return nlohmann::json{};
    }
}

// --- CREATE OBJECT ---
bool SpaceObjectDBManager::createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg)
{
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        errorMsg = "";

        // 1. VALIDATION: _id and Name
        if (!objectData.contains("_id") || objectData["_id"].is_null()) {
            errorMsg = "Error: Field '_id' (NORAD) is required and cannot be null.";
            return false;
        }
        int64_t id = objectData["_id"];

        nlohmann::json existing_id = getSpaceObjectById(id);
        if (!existing_id.empty() && !existing_id.is_null()) {
            errorMsg = "An object with NORAD (_id) already exists: " + std::to_string(id);
            return false;
        }

        if (!objectData.contains("Name") || objectData["Name"].is_null()) {
            errorMsg = "Error: Field 'Name' is required.";
            return false;
        }
        std::string name = objectData["Name"];
        if (name.empty()) {
            errorMsg = "Error: Field 'Name' cannot be empty.";
            return false;
        }

        nlohmann::json existing_name = getSpaceObjectByName(name);
        if (!existing_name.empty() && !existing_name.is_null()) {
            errorMsg = "An object with Name already exists: " + name;
            return false;
        }

        // 2. UNIQUENESS VALIDATIONS (Alias, COSPAR, ILRS, SIC)

        if (objectData.contains("Alias") && !objectData["Alias"].is_null()) {
            std::string val = objectData["Alias"];
            auto filter = make_document(kvp("Alias", val));
            if (_collection.count_documents(filter.view()) > 0) {
                errorMsg = "An object with Alias already exists: " + val;
                return false;
            }
        }

        // COSPAR (Required and Unique)
        if (!objectData.contains("COSPAR") || objectData["COSPAR"].is_null()) {
            errorMsg = "Error: Field 'COSPAR' is required.";
            return false;
        }
        std::string cosparVal = objectData["COSPAR"];
        if(cosparVal.empty()) {
            errorMsg = "Error: 'COSPAR' cannot be empty.";
            return false;
        }
        auto filterCospar = make_document(kvp("COSPAR", cosparVal));
        if (_collection.count_documents(filterCospar.view()) > 0) {
            errorMsg = "An object with COSPAR already exists: " + cosparVal;
            return false;
        }

        // ILRSID (Optional but Unique if exists)
        if (objectData.contains("ILRSID") && !objectData["ILRSID"].is_null()) {
            std::string val = objectData["ILRSID"];
            if (!val.empty()) {
                auto filter = make_document(kvp("ILRSID", val));
                if (_collection.count_documents(filter.view()) > 0) {
                    errorMsg = "An object with ILRS ID already exists: " + val;
                    return false;
                }
            }
        }

        // SIC (Optional but Unique if exists)
        if (objectData.contains("SIC") && !objectData["SIC"].is_null()) {
            std::string val = objectData["SIC"];
            if (!val.empty()) {
                auto filter = make_document(kvp("SIC", val));
                if (_collection.count_documents(filter.view()) > 0) {
                    errorMsg = "An object with SIC already exists: " + val;
                    return false;
                }
            }
        }

        // --- 3. IMAGE MANAGEMENT (Lógica de Reutilización) ---
        std::string picName = "";
        if (objectData.contains("Picture") && !objectData["Picture"].is_null()) {
            picName = objectData["Picture"];
        }

        if (!picName.empty()) {
            // CASO A: Subida de nueva imagen (Hay ruta local)
            if (!localPicturePath.empty()) {

                // Verificar si ya existe (Opcional: Podríamos sobrescribir o avisar)
                // En este caso, si el usuario eligió local, subimos y sobrescribimos.

                std::ifstream file(localPicturePath, std::ios::binary);
                if (!file.is_open()) {
                    errorMsg = "Could not read local image file.";
                    return false;
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                if (!_imageManager.uploadImage(picName, buffer.str())) {
                    errorMsg = "Failed to upload image to GridFS.";
                    return false;
                }
            }
            // CASO B: Reutilización (No hay ruta local, pero hay nombre)
            else {
                // Verificamos que la imagen exista realmente en GridFS para no crear enlaces rotos
                if (!_imageManager.exists(picName)) {
                    errorMsg = "Image '" + picName + "' does not exist in Database and no local file was provided.";
                    return false;
                }
                // Si existe, no hacemos nada. El objeto se guardará con el nombre y ya está vinculado.
            }
        }

        // 4. SETS (Upsert)
        if (objectData.contains("Sets") && !objectData["Sets"].is_null() && objectData["Sets"].is_array())
        {
            mongocxx::options::update upsert_opt;
            upsert_opt.upsert(true);

            for (const auto& setNameJson : objectData["Sets"]) {
                if (setNameJson.is_string()) {
                    std::string sName = setNameJson.get<std::string>();
                    if(!sName.empty()){
                        try {
                            // Upsert en la colección 'sets'
                            auto filter = make_document(kvp("name", sName));
                            auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", sName))));
                            _setsCollection.update_one(filter.view(), update.view(), upsert_opt);
                        } catch(...) {}
                    }
                }
            }
        }
        // --- 4B. GROUPS (Upsert en colección 'groups') ---
        if (objectData.contains("Groups") && !objectData["Groups"].is_null() && objectData["Groups"].is_array())
        {
            mongocxx::options::update upsert_opt;
            upsert_opt.upsert(true);

            for (const auto& grpNameJson : objectData["Groups"]) {
                if (grpNameJson.is_string()) {
                    std::string gName = grpNameJson.get<std::string>();
                    if(!gName.empty()){
                        try {
                            auto filter = make_document(kvp("name", gName));
                            auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", gName))));
                            _groupsCollection.update_one(filter.view(), update.view(), upsert_opt);
                        } catch(...) {}
                    }
                }
            }
        }

        // 5. FINAL INSERT
        bsoncxx::document::value bdoc = njsonToBsoncxx(objectData);
        auto result = _collection.insert_one(bdoc.view());
        return result.has_value();

    }
    catch (const nlohmann::json::exception& e) {
        errorMsg = "JSON format error (Possible uncontrolled null field): " + std::string(e.what());
        spdlog::error("[CRASH AVOIDED] {}", errorMsg);
        return false;
    }
    catch (const mongocxx::exception& ex) {
        errorMsg = "Database Error: " + std::string(ex.what());
        spdlog::error("[Mongo Error] {}", errorMsg);
        return false;
    }
    catch (const std::exception& ex) {
        errorMsg = "Generic error: " + std::string(ex.what());
        spdlog::error("Generic error in createSpaceObject: {}", ex.what());
        return false;
    }
}

// --- UPDATE OBJECT ---
bool SpaceObjectDBManager::updateSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg)
{
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        errorMsg = "";

        // 1. GET ID
        if (!objectData.contains("_id") || objectData["_id"].is_null()) {
            errorMsg = "Internal error: The object to edit has no _id.";
            return false;
        }
        int64_t id = objectData["_id"];

        nlohmann::json existing_obj = getSpaceObjectById(id);
        if (existing_obj.empty() || existing_obj.is_null()) {
            errorMsg = "Error: Original object not found with _id: " + std::to_string(id);
            return false;
        }

        // 2. UNIQUENESS VALIDATIONS (IGNORING SELF)
        auto checkUniqueExceptSelf = [&](const std::string& field, const std::string& value, const std::string& fieldNameErr) -> bool {
            auto filter = make_document(
                kvp(field, value),
                kvp("_id", make_document(kvp("$ne", bsoncxx::types::b_int64{id})))
                );
            if (_collection.count_documents(filter.view()) > 0) {
                errorMsg = "Another object already exists with " + fieldNameErr + ": " + value;
                return false;
            }
            return true;
        };

        if (!checkUniqueExceptSelf("Name", objectData["Name"], "Name")) return false;
        if (!checkUniqueExceptSelf("COSPAR", objectData["COSPAR"], "COSPAR")) return false;

        if (objectData.contains("Alias") && !objectData["Alias"].is_null()) {
            if(!checkUniqueExceptSelf("Alias", objectData["Alias"], "Alias")) return false;
        }

        if (objectData.contains("ILRSID") && !objectData["ILRSID"].is_null()) {
            std::string val = objectData["ILRSID"];
            if(!val.empty()) if(!checkUniqueExceptSelf("ILRSID", val, "ILRS ID")) return false;
        }

        if (objectData.contains("SIC") && !objectData["SIC"].is_null()) {
            std::string val = objectData["SIC"];
            if(!val.empty()) if(!checkUniqueExceptSelf("SIC", val, "SIC")) return false;
        }

        // --- 3. IMAGE MANAGEMENT (Lógica de Reutilización) ---
        std::string picName = "";
        if (objectData.contains("Picture") && !objectData["Picture"].is_null()) {
            picName = objectData["Picture"];
        }

        if (!picName.empty()) {
            // CASO A: Subida de nueva imagen (Hay ruta local)
            if (!localPicturePath.empty()) {

                // Verificar si ya existe (Opcional: Podríamos sobrescribir o avisar)
                // En este caso, si el usuario eligió local, subimos y sobrescribimos.

                std::ifstream file(localPicturePath, std::ios::binary);
                if (!file.is_open()) {
                    errorMsg = "Could not read local image file.";
                    return false;
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                if (!_imageManager.uploadImage(picName, buffer.str())) {
                    errorMsg = "Failed to upload image to GridFS.";
                    return false;
                }
            }
            // CASO B: Reutilización (No hay ruta local, pero hay nombre)
            else {
                // Verificamos que la imagen exista realmente en GridFS para no crear enlaces rotos
                if (!_imageManager.exists(picName)) {
                    errorMsg = "Image '" + picName + "' does not exist in Database and no local file was provided.";
                    return false;
                }
                // Si existe, no hacemos nada. El objeto se guardará con el nombre y ya está vinculado.
            }
        }

        // 4. SETS (Upsert)
        if (objectData.contains("Sets") && !objectData["Sets"].is_null() && objectData["Sets"].is_array())
        {
            mongocxx::options::update upsert_opt;
            upsert_opt.upsert(true);

            for (const auto& setNameJson : objectData["Sets"]) {
                if (setNameJson.is_string()) {
                    std::string sName = setNameJson.get<std::string>();
                    if(!sName.empty()){
                        auto filter = make_document(kvp("name", sName));
                        auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", sName))));
                        _setsCollection.update_one(filter.view(), update.view(), upsert_opt);
                    }
                }
            }
        }
        // --- 4B. GROUPS (Upsert en colección 'groups') ---
        if (objectData.contains("Groups") && !objectData["Groups"].is_null() && objectData["Groups"].is_array())
        {
            mongocxx::options::update upsert_opt;
            upsert_opt.upsert(true);

            for (const auto& grpNameJson : objectData["Groups"]) {
                if (grpNameJson.is_string()) {
                    std::string gName = grpNameJson.get<std::string>();
                    if(!gName.empty()){
                        try {
                            auto filter = make_document(kvp("name", gName));
                            auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", gName))));
                            _groupsCollection.update_one(filter.view(), update.view(), upsert_opt);
                        } catch(...) {}
                    }
                }
            }
        }

        // 5. FINAL UPDATE
        bsoncxx::document::value bdoc = njsonToBsoncxx(objectData);
        auto filter = make_document(kvp("_id", bsoncxx::types::b_int64{id}));
        auto result = _collection.replace_one(filter.view(), bdoc.view());

        if(result && result->modified_count() >= 0) return true;
        else {
            errorMsg = "Document was not modified (data might be identical).";
            return true;
        }

    } catch (const std::exception& ex) {
        errorMsg = "Exception in update: " + std::string(ex.what());
        spdlog::error("Exception in updateSpaceObject: {}", ex.what());
        return false;
    }
}

// --- DELETE BY ID ---
bool SpaceObjectDBManager::deleteSpaceObjectById(int64_t id)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::types::b_int64{id}));
        auto result = _collection.delete_one(filter.view());
        return result && result->deleted_count() > 0;
    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in deleteSpaceObjectById: {}", ex.what());
        return false;
    }
}

int64_t SpaceObjectDBManager::getSpaceObjectsCount() { return _collection.count_documents({}); }
std::vector<nlohmann::json> SpaceObjectDBManager::getAllSpaceObjects() {
    std::vector<nlohmann::json> all;
    auto cursor = _collection.find({});
    for(auto&& doc : cursor) all.push_back(bsoncxxToNjson(doc));
    return all;
}

// --- SETS METHODS ---

std::set<std::string> SpaceObjectDBManager::getAllUniqueSetNames()
{
    std::set<std::string> sets;
    try {
        auto cursor = _setsCollection.find({});
        for (auto&& doc : cursor) {
            if (doc["name"] && doc["name"].type() == bsoncxx::type::k_string) {
                sets.insert(std::string(doc["name"].get_string().value));
            }
        }
    } catch (const std::exception& ex) {
        spdlog::error("Failed in getAllUniqueSetNames: {}", ex.what());
    }
    return sets;
}

std::vector<nlohmann::json> SpaceObjectDBManager::getSpaceObjectsBySets(const std::set<std::string>& setNames)
{
    std::vector<nlohmann::json> objects;
    if (setNames.empty()) return objects;

    bsoncxx::builder::basic::array set_array;
    for(const auto& name : setNames) set_array.append(name);

    try {
        // Buscamos en el campo "Sets"
        auto filter = make_document(kvp("Sets", make_document(kvp("$in", set_array))));
        auto cursor = _collection.find(filter.view());
        for (auto&& doc : cursor) objects.push_back(bsoncxxToNjson(doc));
    } catch (...) {}
    return objects;
}


// Antes addObjectToGroup
bool SpaceObjectDBManager::addObjectToSet(int64_t objectId, const std::string& setName)
{
    if (setName.empty()) return false;
    try {
        mongocxx::options::update upsert_opt;
        upsert_opt.upsert(true);
        // 1. Asegurar que el Set existe en la colección 'sets'
        _setsCollection.update_one(make_document(kvp("name", setName)),
                                   make_document(kvp("$setOnInsert", make_document(kvp("name", setName)))),
                                   upsert_opt);

        // 2. Añadir al array "Sets" del objeto
        auto result = _collection.update_one(
            make_document(kvp("_id", bsoncxx::types::b_int64{objectId})),
            make_document(kvp("$addToSet", make_document(kvp("Sets", setName))))
            );
        return (result && result->matched_count() > 0);
    } catch (...) { return false; }
}

// Antes removeObjectFromGroup
bool SpaceObjectDBManager::removeObjectFromSet(int64_t objectId, const std::string& setName)
{
    try {
        // Quitar del array "Sets"
        auto result = _collection.update_one(
            make_document(kvp("_id", bsoncxx::types::b_int64{objectId})),
            make_document(kvp("$pull", make_document(kvp("Sets", setName))))
            );
        return (result && result->matched_count() > 0);
    } catch (...) { return false; }
}

// Antes crearGrupo
bool SpaceObjectDBManager::createSet(const std::string& setName)
{
    if (setName.empty()) return false;
    try {
        if (_setsCollection.count_documents(make_document(kvp("name", setName))) > 0) return false;
        _setsCollection.insert_one(make_document(kvp("name", setName)));
        return true;
    } catch (...) { return false; }
}

// Antes eliminarGrupo
bool SpaceObjectDBManager::deleteSet(const std::string& setName)
{
    try {
        // 1. Borrar de la colección de sets
        _setsCollection.delete_one(make_document(kvp("name", setName)));

        // 2. Borrar la referencia en todos los objetos (campo "Sets")
        _collection.update_many(
            make_document(kvp("Sets", setName)),
            make_document(kvp("$pull", make_document(kvp("Sets", setName))))
            );
        return true;
    } catch (...) { return false; }
}
// =================================================================
// --- MÉTODOS DE GROUPS (NUEVO) ---
// =================================================================

std::set<std::string> SpaceObjectDBManager::getAllUniqueGroupNames()
{
    std::set<std::string> groups;
    try {
        auto cursor = _groupsCollection.find({});
        for (auto&& doc : cursor) {
            if (doc["name"] && doc["name"].type() == bsoncxx::type::k_string) {
                groups.insert(std::string(doc["name"].get_string().value));
            }
        }
    } catch (const std::exception& ex) {
        spdlog::error("Failed in getAllUniqueGroupNames: {}", ex.what());
    }
    return groups;
}

bool SpaceObjectDBManager::createGroup(const std::string& groupName)
{
    if (groupName.empty()) return false;
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        // Verificar si ya existe
        if (_groupsCollection.count_documents(make_document(kvp("name", groupName))) > 0) {
            spdlog::info("Group '{}' already exists.", groupName);
            return false;
        }

        _groupsCollection.insert_one(make_document(kvp("name", groupName)));
        return true;
    } catch (const std::exception& ex) {
        spdlog::error("Failed in createGroup: {}", ex.what());
        return false;
    }
}

bool SpaceObjectDBManager::deleteGroup(const std::string& groupName)
{
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        // 1. Borrar de la colección de grupos
        auto result = _groupsCollection.delete_one(make_document(kvp("name", groupName)));

        if (!result || result->deleted_count() == 0) {
            spdlog::warn("Group '{}' not found for deletion.", groupName);
        }

        // 2. Borrar la referencia en todos los objetos (campo "Groups")
        _collection.update_many(
            make_document(kvp("Groups", groupName)),
            make_document(kvp("$pull", make_document(kvp("Groups", groupName))))
            );

        spdlog::info("Group '{}' deleted and references cleaned.", groupName);
        return true;
    } catch (const std::exception& ex) {
        spdlog::error("Failed in deleteGroup: {}", ex.what());
        return false;
    }
}
// =================================================================
// MÉTODOS DE ENLACE DE GRUPOS (FALTABAN)
// =================================================================

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
        // Buscamos en el campo "Groups" (Array de strings)
        auto filter = make_document(kvp("Groups", make_document(kvp("$in", group_array))));
        mongocxx::cursor cursor = _collection.find(filter.view());

        for (bsoncxx::document::view doc : cursor) {
            objects.push_back(bsoncxxToNjson(doc));
        }
    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in getSpaceObjectsByGroups: {}", ex.what());
    }
    return objects;
}

bool SpaceObjectDBManager::addObjectToGroup(int64_t objectId, const std::string& groupName)
{
    if (groupName.empty()) {
        spdlog::error("Group name cannot be empty.");
        return false;
    }

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        // 1. Asegurar que el Grupo existe en la colección 'groups' (Upsert)
        mongocxx::options::update upsert_options;
        upsert_options.upsert(true);
        auto group_filter = make_document(kvp("name", groupName));
        auto group_update = make_document(kvp("$setOnInsert", make_document(kvp("name", groupName))));
        _groupsCollection.update_one(group_filter.view(), group_update.view(), upsert_options);

        // 2. Añadir al array "Groups" del objeto
        auto obj_filter = make_document(kvp("_id", bsoncxx::types::b_int64{objectId}));
        auto obj_update = make_document(kvp("$addToSet", make_document(kvp("Groups", groupName))));

        auto result = _collection.update_one(obj_filter.view(), obj_update.view());

        if (!result || result->matched_count() == 0) {
            spdlog::error("Object with _id: {} not found to add group.", objectId);
            return false;
        }
        return true;

    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in addObjectToGroup: {}", ex.what());
        return false;
    }
}

bool SpaceObjectDBManager::removeObjectFromGroup(int64_t objectId, const std::string& groupName)
{
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        auto obj_filter = make_document(kvp("_id", bsoncxx::types::b_int64{objectId}));
        // Quitar del array "Groups"
        auto obj_update = make_document(kvp("$pull", make_document(kvp("Groups", groupName))));

        auto result = _collection.update_one(obj_filter.view(), obj_update.view());

        if (!result || result->matched_count() == 0) {
            spdlog::error("Object with _id: {} not found to remove group.", objectId);
            return false;
        }
        return true;

    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in removeObjectFromGroup: {}", ex.what());
        return false;
    }
}

// =================================================================
// NUEVAS FUNCIONES DE VERSIONADO Y GUARDADO MASIVO
// =================================================================

bool SpaceObjectDBManager::createVersionSnapshot(const std::string& versionName, const std::string& comment, std::string& errorMsg)
{
    try {
        errorMsg = "";

        // 1. Recuperar TODO lo que hay en BBDD ahora mismo
        std::vector<nlohmann::json> currentData = getAllSpaceObjects();

        if (currentData.empty()) {
            // Opcional: Permitir versiones vacías o no. Aquí permitimos pero avisamos.
            spdlog::warn("Creating version snapshot of empty database.");
        }

        // 2. Convertir a BSON Array
        bsoncxx::builder::basic::array objectsArray;
        for (const auto& obj : currentData) {
            objectsArray.append(njsonToBsoncxx(obj));
        }

        // 3. Timestamp
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y-%m-%dT%H:%M:%S");

        // 4. Documento de Versión
        auto versionDoc = make_document(
            kvp("versionName", versionName),
            kvp("comment", comment),
            kvp("timestamp", ss.str()),
            kvp("objectCount", static_cast<int64_t>(currentData.size())),
            kvp("snapshot", objectsArray)
            );

        _versionsCollection.insert_one(versionDoc.view());
        spdlog::info("Version snapshot '{}' created.", versionName);
        return true;

    } catch (const std::exception& ex) {
        errorMsg = ex.what();
        spdlog::error("Failed to create version: {}", errorMsg);
        return false;
    }
}

bool SpaceObjectDBManager::saveAllAndVersion(const std::vector<nlohmann::json>& allObjects,
                                             const std::set<std::string>& allSets,
                                             const std::set<std::string>& allGroups,
                                             const std::string& versionName,
                                             const std::string& comment,
                                             SaveMode mode,
                                             std::string& errorMsg)
{
    try {
        // =================================================================================
        // 1. ANÁLISIS DE DIFERENCIAS (DIFF)
        // =================================================================================

        // A. DIFERENCIAS DE OBJETOS
        std::vector<nlohmann::json> diffAdded;
        std::vector<nlohmann::json> diffModified;
        std::vector<int64_t> diffDeleted;

        std::map<int64_t, nlohmann::json> oldDbState;
        auto currentDbObjects = getAllSpaceObjects();
        for(const auto& obj : currentDbObjects) {
            if(obj.contains("_id") && obj["_id"].is_number()) {
                oldDbState[obj["_id"].get<int64_t>()] = obj;
            }
        }

        // B. DIFERENCIAS DE SETS
        std::vector<std::string> setsAdded, setsDeleted;
        std::set<std::string> oldSets = getAllUniqueSetNames(); // Leemos BD actual

        // Calcular Sets Added (Están en memoria, no en BD)
        for(const auto& s : allSets) {
            if(oldSets.find(s) == oldSets.end()) setsAdded.push_back(s);
        }
        // Calcular Sets Deleted (Estaban en BD, no en memoria)
        for(const auto& s : oldSets) {
            if(allSets.find(s) == allSets.end()) setsDeleted.push_back(s);
        }

        // C. DIFERENCIAS DE GROUPS
        std::vector<std::string> groupsAdded, groupsDeleted;
        std::set<std::string> oldGroups = getAllUniqueGroupNames(); // Leemos BD actual

        // Calcular Groups Added
        for(const auto& g : allGroups) {
            if(oldGroups.find(g) == oldGroups.end()) groupsAdded.push_back(g);
        }
        // Calcular Groups Deleted
        for(const auto& g : oldGroups) {
            if(allGroups.find(g) == allGroups.end()) groupsDeleted.push_back(g);
        }

        spdlog::info("[DIFF START] Objects: {} vs {}, Sets: {} vs {}, Groups: {} vs {}",
                     oldDbState.size(), allObjects.size(),
                     oldSets.size(), allSets.size(),
                     oldGroups.size(), allGroups.size());

        // =================================================================================
        // 2. DOBLE COMPROBACIÓN DE SEGURIDAD (INTEGRADO)
        //    Verificamos que no haya duplicados internos y calculamos las diferencias.
        // =================================================================================

        std::set<int64_t> checkIds;
        std::set<std::string> checkNames;
        std::set<std::string> checkAliases;
        std::set<std::string> checkCospars;

        for(const auto& newObj : allObjects) {
            // --- A. VALIDACIONES DE SEGURIDAD (Tus checks de duplicados) ---

            // ID (NORAD) - Crítico
            if(!newObj.contains("_id") || newObj["_id"].is_null()) {
                errorMsg = "Critical: Object without _id found in save list.";
                return false;
            }
            int64_t id = newObj["_id"].get<int64_t>(); // Asegurar tipo

            if(checkIds.count(id)) {
                errorMsg = "Critical: Duplicate NORAD ID in save list: " + std::to_string(id);
                return false;
            }
            checkIds.insert(id);

            // NAME - Crítico
            if(newObj.contains("Name") && newObj["Name"].is_string()) {
                std::string n = newObj["Name"];
                if(!n.empty()) {
                    if(checkNames.count(n)) {
                        errorMsg = "Critical: Duplicate Name in save list: " + n;
                        return false;
                    }
                    checkNames.insert(n);
                }
            }

            // ALIAS - Estricto
            if(newObj.contains("Alias") && newObj["Alias"].is_string()) {
                std::string a = newObj["Alias"];
                if(!a.empty()) {
                    if(checkAliases.count(a)) {
                        errorMsg = "Critical: Duplicate Alias in save list: " + a;
                        return false;
                    }
                    checkAliases.insert(a);
                }
            }

            // COSPAR - Estricto
            if(newObj.contains("COSPAR") && newObj["COSPAR"].is_string()) {
                std::string c = newObj["COSPAR"];
                if(!c.empty()) {
                    if(checkCospars.count(c)) {
                        errorMsg = "Critical: Duplicate COSPAR in save list: " + c;
                        return false;
                    }
                    checkCospars.insert(c);
                }
            }

            // --- CÁLCULO DIFF OBJETOS ---
            if (oldDbState.find(id) == oldDbState.end()) {
                diffAdded.push_back(newObj);
            } else {
                if (oldDbState[id] != newObj) diffModified.push_back(newObj);
                oldDbState.erase(id);
            }
        }

        // Detectar eliminados
        for(const auto& kv : oldDbState) diffDeleted.push_back(kv.first);

        // =================================================================================
        // 3. APLICAR CAMBIOS (COMMIT)
        // =================================================================================

        // Reemplazar OBJETOS
        _collection.drop();
        if (!allObjects.empty()) {
            std::vector<bsoncxx::document::value> docs;
            docs.reserve(allObjects.size());
            for(const auto& j : allObjects) docs.push_back(njsonToBsoncxx(j));
            _collection.insert_many(docs);
        }

        // Reemplazar SETS
        _setsCollection.drop();
        if (!allSets.empty()) {
            std::vector<bsoncxx::document::value> setDocs;
            setDocs.reserve(allSets.size());
            for(const auto& s : allSets) setDocs.push_back(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", s)));
            _setsCollection.insert_many(setDocs);
        }

        // Reemplazar GROUPS
        _groupsCollection.drop();
        if (!allGroups.empty()) {
            std::vector<bsoncxx::document::value> groupDocs;
            groupDocs.reserve(allGroups.size());
            for(const auto& g : allGroups) groupDocs.push_back(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", g)));
            _groupsCollection.insert_many(groupDocs);
        }

        // =================================================================================
        // 4. REGISTRAR VERSIÓN
        // =================================================================================

        if (mode == SaveMode::FULL_SNAPSHOT) {
            return createVersionSnapshotInternal(allObjects, versionName, comment, errorMsg);
        }
        else {
            // Pasamos todas las listas de diferencias a la nueva función
            return createIncrementalVersion(diffAdded, diffModified, diffDeleted,
                                            setsAdded, setsDeleted,
                                            groupsAdded, groupsDeleted,
                                            versionName, comment, errorMsg);
        }

    } catch (const std::exception& e) {
        errorMsg = "Critical error during bulk save: " + std::string(e.what());
        spdlog::critical(errorMsg);
        return false;
    }
}
// NUEVA FUNCIÓN PRIVADA PARA GUARDAR DELTAS
// SpaceObjectDBManager.cpp

// Actualiza la firma en el .cpp (recuerda cambiarla también en el .h)
bool SpaceObjectDBManager::createIncrementalVersion(const std::vector<nlohmann::json>& added,
                                                    const std::vector<nlohmann::json>& modified,
                                                    const std::vector<int64_t>& deletedIds,
                                                    // NUEVOS PARÁMETROS:
                                                    const std::vector<std::string>& setsAdded,
                                                    const std::vector<std::string>& setsDeleted,
                                                    const std::vector<std::string>& groupsAdded,
                                                    const std::vector<std::string>& groupsDeleted,
                                                    // ------------------
                                                    const std::string& versionName,
                                                    const std::string& comment,
                                                    std::string& errorMsg)
{
    try {
        using bsoncxx::builder::basic::make_document;
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_array;

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y-%m-%dT%H:%M:%S");

        // Construir arrays BSON para objetos
        bsoncxx::builder::basic::array arrAdded;
        for(const auto& j : added) arrAdded.append(njsonToBsoncxx(j));

        bsoncxx::builder::basic::array arrModified;
        for(const auto& j : modified) arrModified.append(njsonToBsoncxx(j));

        bsoncxx::builder::basic::array arrDeleted;
        for(const auto& id : deletedIds) arrDeleted.append(id);

        // --- NUEVO: Construir arrays BSON para Sets/Groups ---
        bsoncxx::builder::basic::array arrSetsAdded, arrSetsDeleted;
        for(const auto& s : setsAdded) arrSetsAdded.append(s);
        for(const auto& s : setsDeleted) arrSetsDeleted.append(s);

        bsoncxx::builder::basic::array arrGroupsAdded, arrGroupsDeleted;
        for(const auto& g : groupsAdded) arrGroupsAdded.append(g);
        for(const auto& g : groupsDeleted) arrGroupsDeleted.append(g);

        // Documento de versión optimizado
        auto versionDoc = make_document(
            kvp("versionName", versionName),
            kvp("comment", comment),
            kvp("timestamp", ss.str()),
            kvp("type", "incremental"),
            kvp("changes", make_document(
                               // Contadores Objeto
                               kvp("objects", make_document(
                                                  kvp("addedCount", static_cast<int64_t>(added.size())),
                                                  kvp("modifiedCount", static_cast<int64_t>(modified.size())),
                                                  kvp("deletedCount", static_cast<int64_t>(deletedIds.size())),
                                                  kvp("added", arrAdded),
                                                  kvp("modified", arrModified),
                                                  kvp("deleted", arrDeleted)
                                                  )),
                               // Contadores Sets
                               kvp("sets", make_document(
                                               kvp("added", arrSetsAdded),
                                               kvp("deleted", arrSetsDeleted)
                                               )),
                               // Contadores Groups
                               kvp("groups", make_document(
                                                 kvp("added", arrGroupsAdded),
                                                 kvp("deleted", arrGroupsDeleted)
                                                 ))
                               ))
            );

        _versionsCollection.insert_one(versionDoc.view());
        spdlog::info("Incremental version '{}' created. (Sets: +{}/-{}, Groups: +{}/-{})",
                     versionName, setsAdded.size(), setsDeleted.size(), groupsAdded.size(), groupsDeleted.size());
        return true;

    } catch (const std::exception& ex) {
        errorMsg = ex.what();
        return false;
    }
}
bool SpaceObjectDBManager::createVersionSnapshotInternal(const std::vector<nlohmann::json>& data,
                                                         const std::string& versionName,
                                                         const std::string& comment,
                                                         std::string& errorMsg)
{
    try {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;
        using bsoncxx::builder::basic::make_array;

        // 1. Timestamp actual
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y-%m-%dT%H:%M:%S");

        // 2. Convertir el vector de JSON a BSON Array
        // Esto es mucho más eficiente que leer de la BBDD porque ya tenemos los datos en RAM
        bsoncxx::builder::basic::array objectsArray;
        for (const auto& obj : data) {
            objectsArray.append(njsonToBsoncxx(obj));
        }

        // 3. Crear el documento de versión
        auto versionDoc = make_document(
            kvp("versionName", versionName),
            kvp("comment", comment),
            kvp("timestamp", ss.str()),
            kvp("type", "full_snapshot"), // Marcamos que es completa
            kvp("objectCount", static_cast<int64_t>(data.size())),
            kvp("snapshot", objectsArray)
            );

        // 4. Insertar en la colección de versiones
        _versionsCollection.insert_one(versionDoc.view());

        spdlog::info("Full Version snapshot '{}' created successfully.", versionName);
        return true;

    } catch (const std::exception& ex) {
        errorMsg = ex.what();
        spdlog::error("Failed to create version snapshot: {}", errorMsg);
        return false;
    }
}
