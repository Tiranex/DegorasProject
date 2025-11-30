#include "SpaceObjectDBManager.h"
#include "json_helpers.h"
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <fstream>
#include <sstream>
#include <set>

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
    _groupsCollection(_db["groups"]),    // <--- AHORA APUNTA A "sets"
    _imageManager(_db)
{
    try {
        _db.run_command(make_document(kvp("ping", 1)));
        spdlog::info("SpaceObjectDBManager connected. Sets collection initialized.");
    } catch (const mongocxx::exception& ex) {
        spdlog::critical("Failed to initialize DB: {}", ex.what());
        throw;
    }
}

SpaceObjectDBManager::~SpaceObjectDBManager() {
    spdlog::info("SpaceObjectDBManager disconnected.");
}

// --- GET BY ID ---
nlohmann::json SpaceObjectDBManager::getSpaceObjectById(int64_t id)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::types::b_int64{id}));
        auto result = _collection.find_one(filter.view());
        return result ? bsoncxxToNjson(result->view()) : nlohmann::json{};
    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in getSpaceObjectById: {}", ex.what());
        return nlohmann::json{};
    }
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

        // 3. IMAGE MANAGEMENT
        std::string picName = "";
        if (objectData.contains("Picture") && !objectData["Picture"].is_null()) {
            picName = objectData["Picture"];
        }

        if (!picName.empty()) {
            // A. Uniqueness
            nlohmann::json existing_pic = getSpaceObjectByPicture(picName);
            if (!existing_pic.empty() && !existing_pic.is_null()) {
                errorMsg = "An object is already using image: " + picName;
                return false;
            }
            // B. Local path
            if (localPicturePath.empty()) {
                errorMsg = "Image name specified but no file selected.";
                return false;
            }
            // C. Read file
            std::ifstream file(localPicturePath, std::ios::binary);
            if (!file.is_open()) {
                errorMsg = "Could not read local image file.";
                return false;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string imageData = buffer.str();
            file.close();

            // D. Upload
            if (!_imageManager.uploadImage(picName, imageData)) {
                errorMsg = "Failed to upload image to GridFS.";
                return false;
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

        if (objectData.contains("Abbreviation") && !objectData["Abbreviation"].is_null()) {
            if(!checkUniqueExceptSelf("Abbreviation", objectData["Abbreviation"], "Alias")) return false;
        }

        if (objectData.contains("ILRSID") && !objectData["ILRSID"].is_null()) {
            std::string val = objectData["ILRSID"];
            if(!val.empty()) if(!checkUniqueExceptSelf("ILRSID", val, "ILRS ID")) return false;
        }

        if (objectData.contains("SIC") && !objectData["SIC"].is_null()) {
            std::string val = objectData["SIC"];
            if(!val.empty()) if(!checkUniqueExceptSelf("SIC", val, "SIC")) return false;
        }

        // 3. IMAGE MANAGEMENT
        std::string finalPicName = "";
        if (objectData.contains("Picture") && !objectData["Picture"].is_null()) {
            finalPicName = objectData["Picture"];
        }

        if (!localPicturePath.empty()) {
            auto filterPic = make_document(
                kvp("Picture", finalPicName),
                kvp("_id", make_document(kvp("$ne", bsoncxx::types::b_int64{id})))
                );
            if (_collection.count_documents(filterPic.view()) > 0) {
                errorMsg = "Another object is already using image: " + finalPicName;
                return false;
            }

            std::string oldPic = "";
            if(existing_obj.contains("Picture") && !existing_obj["Picture"].is_null())
                oldPic = existing_obj["Picture"];

            std::ifstream file(localPicturePath, std::ios::binary);
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (!_imageManager.uploadImage(finalPicName, buffer.str())) {
                errorMsg = "Error uploading new image.";
                return false;
            }

            if(!oldPic.empty() && oldPic != finalPicName) {
                _imageManager.deleteImageByName(oldPic);
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

// --- GET ALL ---
std::vector<nlohmann::json> SpaceObjectDBManager::getAllSpaceObjects()
{
    std::vector<nlohmann::json> allObjects;
    try {
        mongocxx::cursor cursor = _collection.find({});
        for (bsoncxx::document::view doc : cursor) {
            allObjects.push_back(bsoncxxToNjson(doc));
        }
    } catch (const mongocxx::exception& ex) {
        spdlog::error("Failed in getAllSpaceObjects: {}", ex.what());
    }
    return allObjects;
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
