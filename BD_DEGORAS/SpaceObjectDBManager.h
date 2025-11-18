#pragma once

#include <string>
#include <cstdint> // Para int64_t
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <nlohmann/json.hpp>
#include <mongocxx/gridfs/bucket.hpp>
#include <vector>
#include <set>


// Modulos
#include "gridfsimagemanager.h"


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

    /**
     * @brief (¡NUEVO!) Obtiene *todos* los SpaceObjects de la colección.
     * @return Un vector de objetos nlohmann::json.
     */
    std::vector<nlohmann::json> getAllSpaceObjects();

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


    // =================================================================
    // --- ¡NUEVO! MÉTODOS DE GRUPOS ---
    // =================================================================

    /**
     * @brief Obtiene todos los objetos que pertenecen a CUALQUIERA de los grupos
     * de la lista.
     * @param groupNames Un set de nombres de grupos.
     * @return Un vector de objetos JSON.
     */
    std::vector<nlohmann::json> getSpaceObjectsByGroups(const std::set<std::string>& groupNames);

    /**
     * @brief Obtiene la lista maestra de todos los nombres de grupos únicos
     * desde la colección 'groups'.
     * @return Un set de strings.
     */
    std::set<std::string> getAllUniqueGroupNames();

    /**
     * @brief Añade un tag de grupo a un space_object Y
     * asegura que el grupo exista en la colección 'groups'.
     */
    bool addObjectToGroup(int64_t objectId, const std::string& groupName);

    /**
     * @brief Quita un tag de grupo de un space_object.
     */
    bool removeObjectFromGroup(int64_t objectId, const std::string& groupName);

    /**
     * @brief Crea un nuevo grupo en la colección 'groups'.
     * @param groupName El nombre del nuevo grupo.
     * @return true si se creó, false si ya existía o hubo un error.
     */
    bool crearGrupo(const std::string& groupName);

    /**
     * @brief Elimina un grupo de la colección 'groups' Y de todos los objetos.
     * @param groupName El nombre del grupo a eliminar.
     * @return true si se eliminó, false si hubo un error.
     */
    bool eliminarGrupo(const std::string& groupName);

private:
    mongocxx::client _client;
    mongocxx::database _db;
    mongocxx::collection _collection; // Para 'space_objects'

    // --- Miembros para GridFS (basados en 'name') ---
    mongocxx::gridfs::bucket _gridfsBucket;
    mongocxx::collection _gridfsFilesCollection; // Para buscar en 'fs.files'

    mongocxx::collection _groupsCollection; // Colección "groups"
    GridFSImageManager _imageManager;
};
