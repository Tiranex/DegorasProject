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

class SpaceObjectDBManager
{
public:
    SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name);
    ~SpaceObjectDBManager();

    // --- CRUD OBJECTS ---
    nlohmann::json getSpaceObjectById(int64_t id);
    nlohmann::json getSpaceObjectByName(const std::string& name);
    nlohmann::json getSpaceObjectByPicture(const std::string& picName);

    // Statistics
    int64_t getSpaceObjectsCount();

    std::vector<nlohmann::json> getAllSpaceObjects();

    // Create / Update with error messaging
    bool createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg);
    bool updateSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg);

    bool deleteSpaceObjectById(int64_t id);

    // --- SETS MANAGEMENT (Renamed from Groups) ---
    // Estas funciones ahora gestionan la colección "sets" y el campo "Sets"
    std::set<std::string> getAllUniqueSetNames();
    std::vector<nlohmann::json> getSpaceObjectsBySets(const std::set<std::string>& setNames);

    bool addObjectToSet(int64_t objectId, const std::string& setName);
    bool removeObjectFromSet(int64_t objectId, const std::string& setName);

    bool createSet(const std::string& setName);
    bool deleteSet(const std::string& setName);

    // --- GROUPS MANAGEMENT (New) ---
    std::set<std::string> getAllUniqueGroupNames(); // Para llenar la lista de grupos
    bool createGroup(const std::string& groupName); // Crear categoría nueva
    bool deleteGroup(const std::string& groupName);
    std::vector<nlohmann::json> getSpaceObjectsByGroups(const std::set<std::string>& groupNames);
    bool addObjectToGroup(int64_t objectId, const std::string& groupName);
    bool removeObjectFromGroup(int64_t objectId, const std::string& groupName);


    // --- IMAGE MANAGER ACCESS ---
    GridFSImageManager& getImageManager() { return _imageManager; }

private:
    mongocxx::client _client;
    mongocxx::database _db;
    mongocxx::collection _collection;      // space_objects
    mongocxx::collection _setsCollection;  // sets (antes groups)
    mongocxx::collection _groupsCollection;

    GridFSImageManager _imageManager;
};

#endif // SPACEOBJECTDBMANAGER_H
