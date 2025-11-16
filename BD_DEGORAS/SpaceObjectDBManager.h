#pragma once

#include <string>
#include <cstdint> // Para int64_t
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <nlohmann/json.hpp>
#include <mongocxx/gridfs/bucket.hpp>
#include <vector>



    class SpaceObjectDBManager {
    public:
        SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name);
        ~SpaceObjectDBManager();

        // --- Métodos de SpaceObject ---
        nlohmann::json getSpaceObjectById(int64_t id);
        nlohmann::json getSpaceObjectByName(const std::string& name);

        /**
         * @brief (NUEVO) Obtiene un SpaceObject por su campo Picture (case-sensitive).
         * @param picName El nombre (string) de la imagen.
         * @return Un nlohmann::json con los datos.
         */
        nlohmann::json getSpaceObjectByPicture(const std::string& picName);

        bool createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath);
        bool deleteSpaceObjectById(int64_t id);

        // --- MÉTODOS DE GRIDFS (basados en 'name') ---

        /**
         * @brief Sube datos binarios a GridFS.
         * @return true si la subida fue exitosa, false en caso contrario.
         */
        bool uploadImage(const std::string& nameInDB, const std::string& imageData);

        /**
         * @brief Descarga datos binarios desde GridFS usando su nombre.
         * @return Un std::string con los datos binarios.
         */
        std::string downloadImageByName(const std::string& nameInDB);

        /**
         * @brief Borra un archivo de GridFS por su nombre.
         * @return true si el borrado fue exitoso, false en caso contrario.
         */
        bool deleteImageByName(const std::string& nameInDB);

        /**
         * @brief (¡NUEVO!) Obtiene *todos* los SpaceObjects de la colección.
         * @return Un vector de objetos nlohmann::json.
         */
        std::vector<nlohmann::json> getAllSpaceObjects();

    private:
        mongocxx::client _client;
        mongocxx::database _db;
        mongocxx::collection _collection; // Para 'space_objects'

        // --- Miembros para GridFS (basados en 'name') ---
        mongocxx::gridfs::bucket _gridfsBucket;
        mongocxx::collection _gridfsFilesCollection; // Para buscar en 'fs.files'
    };
