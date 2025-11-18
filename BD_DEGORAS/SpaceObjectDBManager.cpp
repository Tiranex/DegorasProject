#include "SpaceObjectDBManager.h"
#include "json_helpers.h" // ¡IMPORTANTE! Para bsoncxxToNjson y njsonToBsoncxx
#include <iostream>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <fstream>  // Para leer archivos (ifstream)
#include <sstream>  // Para pasar los datos a un string (stringstream)

#include <bsoncxx/builder/basic/array.hpp>
#include <set>

// --- CONSTRUCTOR ---
SpaceObjectDBManager::SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name)
    : _client(mongocxx::uri{uri_str}),
    _db(_client[db_name]),
    _collection(_db[col_name]),
    // --- Lógica de 'name' ---
    _gridfsBucket(_db.gridfs_bucket()),
    _gridfsFilesCollection(_db["fs.files"]),
    _groupsCollection(_db["groups"]),
    _imageManager(_db)
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

// --- ¡NUEVA IMPLEMENTACIÓN! ---
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


// --- CREAR OBJETO (¡VERSIÓN MODIFICADA CON LÓGICA DE GRUPOS!) ---
bool SpaceObjectDBManager::createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath)
{
    // --- Comprobación 1: _id (Sin cambios) ---
    if (!objectData.contains("_id")) {
        std::cerr << "[Error] El JSON para crear no contiene el campo '_id'." << std::endl;
        return false;
    }
    int64_t id = objectData["_id"];
    nlohmann::json existing_id = getSpaceObjectById(id);
    if (!existing_id.empty() && !existing_id.is_null()) {
        std::cerr << "[Error] Ya existe un documento con _id: " << id << std::endl;
        return false;
    }

    // --- Comprobación 2: Name (Sin cambios) ---
    if (!objectData.contains("Name")) {
        std::cerr << "[Error] El JSON para crear no contiene el campo 'Name'." << std::endl;
        return false;
    }
    std::string name = objectData["Name"];
    if (name.empty()) {
        std::cerr << "[Error] El campo 'Name' no puede estar vacío." << std::endl;
        return false;
    }
    nlohmann::json existing_name = getSpaceObjectByName(name);
    if (!existing_name.empty() && !existing_name.is_null()) {
        std::cerr << "[Error] Ya existe un documento con Name: " << name << std::endl;
        return false;
    }

    // --- ¡NUEVA Comprobación 3: Picture (Lógica mejorada)! ---
    std::string picName = "";
    if (objectData.contains("Picture")) {
        picName = objectData["Picture"];
    }

    // Solo validamos y subimos si picName NO está vacío
    if (!picName.empty()) {

        // 1. VALIDAR UNICIDAD
        nlohmann::json existing_pic = getSpaceObjectByPicture(picName);
        if (!existing_pic.empty() && !existing_pic.is_null()) {
            std::cerr << "[Error] Ya existe un documento con Picture: " << picName << std::endl;
            return false; // Falla validación. NO se sube nada.
        }

        // 2. VALIDACIÓN OK. AHORA, PROCEDER A SUBIR.
        if (localPicturePath.empty()) {
            std::cerr << "[Error] El nombre de imagen es '" << picName << "' pero no se seleccionó ningún archivo (ruta local vacía)." << std::endl;
            return false;
        }

        // 3. Leer el archivo desde la ruta local
        std::ifstream file(localPicturePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[Error] No se pudo abrir el archivo local: " << localPicturePath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string imageData = buffer.str();
        file.close();

        if (imageData.empty()) {
            std::cerr << "[Error] El archivo local estaba vacío o no se pudo leer: " << localPicturePath << std::endl;
            return false;
        }

        // 4. Intentar la subida a GridFS
        if (!uploadImage(picName, imageData)) {
            std::cerr << "[Error] La validación fue OK, pero falló la subida a GridFS." << std::endl;
            return false; // Falla subida. NO se inserta el documento.
        }
    }

    // --- ¡NUEVA Comprobación 4: Grupos! ---
    // Antes de insertar el objeto, nos aseguramos de que los grupos
    // que trae en su array "Groups" estén registrados en la _groupsCollection.
    if (objectData.contains("Groups") && objectData["Groups"].is_array())
    {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        mongocxx::options::update upsert_options;
        upsert_options.upsert(true);

        try {
            for (const auto& groupNameJson : objectData["Groups"]) {
                if (groupNameJson.is_string()) {
                    std::string groupName = groupNameJson.get<std::string>();
                    if (!groupName.empty()) {
                        // "Upsert": si no existe, lo crea.
                        auto filter = make_document(kvp("name", groupName));
                        auto update = make_document(kvp("$setOnInsert", make_document(kvp("name", groupName))));

                        _groupsCollection.update_one(filter.view(), update.view(), upsert_options);
                    }
                }
            }
        } catch (const mongocxx::exception& ex) {
            std::cerr << "[Error] Falla al hacer upsert de grupos en _groupsCollection: " << ex.what() << std::endl;
        }
    }
    // --- Fin de la Comprobación 4 ---


    // --- 5. INSERCIÓN FINAL DEL DOCUMENTO (como antes) ---
    try {
        bsoncxx::document::value bdoc = njsonToBsoncxx(objectData);
        auto result = _collection.insert_one(bdoc.view());
        return result.has_value(); // Devuelve true si la inserción tuvo resultado
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en createSpaceObject (insert_one): " << ex.what() << std::endl;

        // ¡MANEJO DE ERROR!
        if (!picName.empty()) {
            std::cerr << "[Info] Deshaciendo... eliminando archivo de GridFS: " << picName << std::endl;
            deleteImageByName(picName); // Llama a tu función de borrado
        }
        return false;
    }
}

// --- BORRAR POR ID ---
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
        std::cerr << "[Error] Falla en getAllSpaceObjects: " << ex.what() << std::endl;
    }

    return allObjects;
}


// --- MÉTODOS DE GRIDFS DELEGADOS ---

// bool SpaceObjectDBManager::uploadImage(const std::string& nameInDB, const std::string& imageData) {
//     return _imageManager.uploadImage(nameInDB, imageData);
// }

// std::string SpaceObjectDBManager::downloadImageByName(const std::string& nameInDB) {
//     return _imageManager.downloadImageByName(nameInDB);
// }

// bool SpaceObjectDBManager::deleteImageByName(const std::string& nameInDB) {
//     return _imageManager.deleteImageByName(nameInDB);
// }


// =================================================================
// --- ¡NUEVO! MÉTODOS DE GRUPOS ---
// =================================================================

/**
 * @brief Obtiene la lista maestra de todos los nombres de grupos únicos
 * desde la colección 'groups'.
 */
std::set<std::string> SpaceObjectDBManager::getAllUniqueGroupNames()
{
    std::set<std::string> groups;
    try {
        mongocxx::cursor cursor = _groupsCollection.find({});

        for (bsoncxx::document::view doc : cursor) {

            // 1. CAMBIO: Comprobar que "name" existe y es del tipo k_string
            if (doc["name"] && doc["name"].type() == bsoncxx::type::k_string)
            {
                // 2. CAMBIO: Usar get_string() y construir un std::string desde el string_view
                std::string_view name_view = doc["name"].get_string().value;
                groups.insert(std::string(name_view));
            }
        }
    } catch (const mongocxx::exception& ex) {
        std::cerr << "[Error] Falla en getAllUniqueGroupNames: " << ex.what() << std::endl;
    }
    return groups;
}
/**
 * @brief Obtiene todos los objetos que pertenecen a CUALQUIERA de los grupos
 * de la lista.
 */
std::vector<nlohmann::json> SpaceObjectDBManager::getSpaceObjectsByGroups(const std::set<std::string>& groupNames)
{
    std::vector<nlohmann::json> objects;

    if (groupNames.empty()) {
        return objects;
    }

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


/**
 * @brief Añade un tag de grupo a un space_object Y
 * asegura que el grupo exista en la colección 'groups'.
 */
bool SpaceObjectDBManager::addObjectToGroup(int64_t objectId, const std::string& groupName)
{
    if (groupName.empty()) {
        std::cerr << "[Error] El nombre del grupo no puede estar vacío." << std::endl;
        return false;
    }

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        // 1. Asegurar que el grupo exista en la colección 'groups' (Upsert)
        mongocxx::options::update upsert_options;
        upsert_options.upsert(true);
        auto group_filter = make_document(kvp("name", groupName));
        auto group_update = make_document(kvp("$setOnInsert", make_document(kvp("name", groupName))));
        _groupsCollection.update_one(group_filter.view(), group_update.view(), upsert_options);

        // 2. Añadir el grupo al array del objeto (usando $addToSet)
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


/**
 * @brief Quita un tag de grupo de un space_object.
 */
bool SpaceObjectDBManager::removeObjectFromGroup(int64_t objectId, const std::string& groupName)
{
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        // 1. Quitar el grupo del array del objeto (usando $pull)
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
        // 1. Verificar si ya existe
        auto filter = make_document(kvp("name", groupName));
        if (_groupsCollection.find_one(filter.view())) {
            std::cerr << "[Info] El grupo '" << groupName << "' ya existe." << std::endl;
            return false;
        }

        // 2. Si no existe, crearlo
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
        // 1. Eliminar el grupo de la colección 'groups'
        auto filter_group = make_document(kvp("name", groupName));
        auto result_group = _groupsCollection.delete_one(filter_group.view());

        if (!result_group || result_group->deleted_count() == 0) {
            std::cerr << "[Info] No se encontró el grupo '" << groupName << "' para eliminar." << std::endl;
            // Continuamos igualmente para limpiar referencias en objetos
        }

        // 2. ¡IMPORTANTE! Eliminar el nombre del grupo del array "Groups"
        //    en TODOS los documentos de la colección principal ('_collection')
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

