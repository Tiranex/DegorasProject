#include "SpaceObjectDBManager.h"
#include "json_helpers.h"
#include <iostream>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <fstream>  // Para leer archivos (ifstream)
#include <sstream>  // Para pasar los datos a un string (stringstream)

    // --- CONSTRUCTOR ---
    SpaceObjectDBManager::SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name)
        : _client(mongocxx::uri{uri_str}),
        _db(_client[db_name]),
        _collection(_db[col_name]),
        // --- Lógica de 'name' ---
        _gridfsBucket(_db.gridfs_bucket()),
        _gridfsFilesCollection(_db["fs.files"])
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


    // --- CREAR OBJETO (¡CON VALIDACIÓN Y SUBIDA INTEGRADA!) ---
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

            // 1. VALIDAR UNICIDAD (Como antes)
            nlohmann::json existing_pic = getSpaceObjectByPicture(picName);
            if (!existing_pic.empty() && !existing_pic.is_null()) {
                std::cerr << "[Error] Ya existe un documento con Picture: " << picName << std::endl;
                return false; // Falla validación. NO se sube nada.
            }

            // 2. VALIDACIÓN OK. AHORA, PROCEDER A SUBIR.
            // Verificamos que el usuario seleccionó un archivo (la ruta no está vacía)
            if (localPicturePath.empty()) {
                std::cerr << "[Error] El nombre de imagen es '" << picName << "' pero no se seleccionó ningún archivo (ruta local vacía)." << std::endl;
                return false;
            }

            // 3. Leer el archivo desde la ruta local (usando C++ estándar)
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

            // 4. Intentar la subida a GridFS (usando tu función existente)
            if (!uploadImage(picName, imageData)) {
                std::cerr << "[Error] La validación fue OK, pero falló la subida a GridFS." << std::endl;
                return false; // Falla subida. NO se inserta el documento.
            }

            // Si la subida fue exitosa, la función continúa...
        }
        // Si picName está vacío, o si no estaba vacío y se validó Y subió con éxito,
        // procedemos a insertar el documento.


        // --- 5. INSERCIÓN FINAL DEL DOCUMENTO (como antes) ---
        try {
            bsoncxx::document::value bdoc = njsonToBsoncxx(objectData);
            auto result = _collection.insert_one(bdoc.view());
            return result.has_value(); // Devuelve true si la inserción tuvo resultado
        } catch (const mongocxx::exception& ex) {
            std::cerr << "[Error] Falla en createSpaceObject (insert_one): " << ex.what() << std::endl;

            // ¡MANEJO DE ERROR!
            // Si la inserción del documento falla DESPUÉS de subir la imagen,
            // debemos borrar la imagen que acabamos de subir para evitar huérfanos.
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


    std::vector<nlohmann::json> SpaceObjectDBManager::getAllSpaceObjects()
    {
        std::vector<nlohmann::json> allObjects;

        try {
            // find({}) con un documento vacío significa "obtener todo"
            mongocxx::cursor cursor = _collection.find({});

            // Iteramos sobre todos los documentos encontrados
            for (bsoncxx::document::view doc : cursor) {
                // Usamos la misma función de conversión que los otros métodos
                allObjects.push_back(bsoncxxToNjson(doc));
            }
        } catch (const mongocxx::exception& ex) {
            std::cerr << "[Error] Falla en getAllSpaceObjects: " << ex.what() << std::endl;
            // Devolver un vector vacío en caso de error
        }

        return allObjects;
    }






    // --- MÉTODOS DE GRIDFS (Lógica de 'name') ---

    bool SpaceObjectDBManager::uploadImage(const std::string& nameInDB, const std::string& imageData)
    {
        try {
            auto uploader = _gridfsBucket.open_upload_stream(nameInDB);
            uploader.write(
                reinterpret_cast<const std::uint8_t*>(imageData.data()),
                imageData.length()
            );
            uploader.close();
            std::cout << "[Info] Imagen subida exitosamente a GridFS: " << nameInDB << std::endl;
            return true;
        } catch (const mongocxx::exception &e) {
            std::cerr << "[Error] Falla en uploadImage: " << e.what() << std::endl;
            return false;
        }
    }

    std::string SpaceObjectDBManager::downloadImageByName(const std::string& nameInDB)
    {
        try {
            // --- Lógica de búsqueda manual (compatible con versiones antiguas) ---
            bsoncxx::builder::basic::document filter{};
            filter.append(bsoncxx::builder::basic::kvp("filename", nameInDB));
            auto result = _gridfsFilesCollection.find_one(filter.view());

            if (!result) {
                std::cerr << "[Info] No se encontró el archivo en fs.files: " << nameInDB << std::endl;
                return std::string{};
            }

            bsoncxx::document::view file_view = result->view();

            // Comprobar que los campos existen
            if (file_view["_id"].type() == bsoncxx::type::k_undefined ||
                file_view["length"].type() == bsoncxx::type::k_undefined) {
                 std::cerr << "[Error] El archivo en fs.files no tiene _id o length." << std::endl;
                 return std::string{};
            }

            auto file_id = file_view["_id"].get_value();
            auto fileSize = file_view["length"].get_int64().value;

            auto downloader = _gridfsBucket.open_download_stream(file_id);
            std::string imageData;
            imageData.resize(fileSize);
            downloader.read(reinterpret_cast<std::uint8_t*>(&imageData[0]), fileSize);

            std::cout << "[Info] Imagen descargada exitosamente de GridFS: " << nameInDB << std::endl;
            return imageData;
        } catch (const mongocxx::exception &e) {
            std::cerr << "[Error] Falla en downloadImageByName: " << e.what() << std::endl;
            return std::string{};
        }
    }

    bool SpaceObjectDBManager::deleteImageByName(const std::string& nameInDB)
    {
        try {
            // --- Lógica de búsqueda manual ---
            bsoncxx::builder::basic::document filter{};
            filter.append(bsoncxx::builder::basic::kvp("filename", nameInDB));
            auto result = _gridfsFilesCollection.find_one(filter.view());

            if (!result) {
                std::cerr << "[Info] No se encontró el archivo para borrar: " << nameInDB << std::endl;
                return false; // No es un error, solo no se encontró
            }

            auto file_id = result->view()["_id"].get_value();
            _gridfsBucket.delete_file(file_id);
            std::cout << "[Info] Imagen borrada exitosamente de GridFS: " << nameInDB << std::endl;
            return true;
        } catch (const mongocxx::exception &e) {
            std::cerr << "[Error] Falla en deleteImageByName: " << e.what() << std::endl;
            return false;
        }
    }
