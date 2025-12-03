#ifndef SPACEOBJECTDBMANAGER_H
#define SPACEOBJECTDBMANAGER_H

#include <string>
#include <vector>
#include <set>
#include <memory>

// MongoDB Driver
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/pool.hpp>

// JSON Library
#include <nlohmann/json.hpp>

// GridFS
#include "GridFSImageManager.h"

// TIPO DE GUARDADO
enum class SaveMode {
    FULL_SNAPSHOT,  // Guarda copia completa (Lo que hacíamos hasta ahora)
    INCREMENTAL     // Guarda solo las diferencias (Delta)
};

class SpaceObjectDBManager
{
public:
    SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name);
    ~SpaceObjectDBManager();

    // --- CRUD OBJECTS (Individual) ---
    // Estas se mantienen para lecturas puntuales o validaciones
    nlohmann::json getSpaceObjectById(int64_t id);
    nlohmann::json getSpaceObjectByName(const std::string& name);
    nlohmann::json getSpaceObjectByPicture(const std::string& picName);

    // Statistics
    int64_t getSpaceObjectsCount();

    std::vector<nlohmann::json> getAllSpaceObjects();

    // --- NUEVO: VERSIONADO Y GUARDADO MASIVO (Bulk Save) ---

    /**
     * @brief Reemplaza TODA la colección actual con la lista proporcionada (desde memoria)
     * y crea una entrada en el historial de versiones.
     * @param allObjects Vector con todos los objetos que hay en memoria actualmente.
     * @param versionName Nombre/Etiqueta de la versión.
     * @param comment Comentario del usuario.
     * @param errorMsg Mensaje de error si falla.
     */
    bool saveAllAndVersion(const std::vector<nlohmann::json>& allObjects,
                           const std::set<std::string>& allSets,
                           const std::set<std::string>& allGroups,
                           const std::string& versionName,
                           const std::string& comment,
                           SaveMode mode, // <--- NUEVO PARAMETRO
                           std::string& errorMsg);
    /**
     * @brief Crea solo una instantánea (snapshot) del estado actual de la BBDD en la colección 'versions'.
     */
    bool createVersionSnapshot(const std::string& versionName,
                               const std::string& comment,
                               std::string& errorMsg);


    // --- MÉTODOS ANTIGUOS DE ESCRITURA DIRECTA ---
    // (Se mantienen por compatibilidad si quieres usarlos,
    // pero en el nuevo modo "Memoria" usaremos saveAllAndVersion principalmente)
    bool createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg);
    bool updateSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg);
    bool deleteSpaceObjectById(int64_t id);

    // --- SETS MANAGEMENT ---
    std::set<std::string> getAllUniqueSetNames();
    std::vector<nlohmann::json> getSpaceObjectsBySets(const std::set<std::string>& setNames);
    bool addObjectToSet(int64_t objectId, const std::string& setName);
    bool removeObjectFromSet(int64_t objectId, const std::string& setName);
    bool createSet(const std::string& setName);
    bool deleteSet(const std::string& setName);

    // --- GROUPS MANAGEMENT ---
    std::set<std::string> getAllUniqueGroupNames();
    bool createGroup(const std::string& groupName);
    bool deleteGroup(const std::string& groupName);
    std::vector<nlohmann::json> getSpaceObjectsByGroups(const std::set<std::string>& groupNames);
    bool addObjectToGroup(int64_t objectId, const std::string& groupName);
    bool removeObjectFromGroup(int64_t objectId, const std::string& groupName);

    // --- IMAGE MANAGER ACCESS ---
    GridFSImageManager& getImageManager() { return _imageManager; }

private:
    mongocxx::client _client;
    mongocxx::database _db;
    mongocxx::collection _collection;         // space_objects
    mongocxx::collection _setsCollection;     // sets
    mongocxx::collection _groupsCollection;   // groups
    mongocxx::collection _versionsCollection; // <--- NUEVA: versions

    GridFSImageManager _imageManager;
    bool createVersionSnapshotInternal(const std::vector<nlohmann::json>& data,
                                       const std::string& versionName,
                                       const std::string& comment,
                                       std::string& errorMsg);

    bool createIncrementalVersion(const std::vector<nlohmann::json>& added,
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
                                                        std::string& errorMsg);
};

#endif // SPACEOBJECTDBMANAGER_H
