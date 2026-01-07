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

/**
 * @brief Defines the strategy for saving version history.
 */
enum class SaveMode {
    FULL_SNAPSHOT, ///< Creates a complete copy of the entire database state.
    INCREMENTAL    ///< Saves only the changes (diff) since the last version.
};

/**
 * @brief Manages all interactions with the MongoDB backend.
 * @details Handles connection pooling, CRUD operations for Space Objects,
 * Sets, Groups, and Version Control. It also delegates image handling to `GridFSImageManager`.
 */
class SpaceObjectDBManager
{
public:
    /**
     * @brief Constructor. Establishes the connection to MongoDB.
     * @param uri_str The MongoDB connection string.
     * @param db_name The target database name.
     * @param col_name The main collection name for space objects.
     */
    SpaceObjectDBManager(const std::string& uri_str, const std::string& db_name, const std::string& col_name);

    ~SpaceObjectDBManager();

    /**
     * @brief Retrieves a specific Space Object by its unique NORAD ID.
     * @param id The NORAD ID.
     * @return The object as JSON, or empty if not found.
     */
    nlohmann::json getSpaceObjectById(int64_t id);

    /**
     * @brief Retrieves a specific Space Object by its name.
     * @param name The name of the object.
     * @return The object as JSON, or empty if not found.
     */
    nlohmann::json getSpaceObjectByName(const std::string& name);

    /**
     * @brief Fetches all Space Objects currently in the database.
     * @return A vector containing all objects.
     */
    std::vector<nlohmann::json> getAllSpaceObjects();

    /**
     * @brief Saves the current memory state to the database and creates a version entry.
     * @details Replaces the active collection with the provided objects and records the change.
     * @param allObjects The complete list of objects currently in memory.
     * @param allSets The complete set of Set names.
     * @param allGroups The complete set of Group names.
     * @param versionName A unique tag for this version.
     * @param comment User comments describing the changes.
     * @param mode The saving strategy (Full vs Incremental).
     * @param[out] errorMsg Buffer to store error description if operation fails.
     * @return True if successful, False otherwise.
     */
    bool saveAllAndVersion(const std::vector<nlohmann::json>& allObjects,
                           const std::set<std::string>& allSets,
                           const std::set<std::string>& allGroups,
                           const std::string& versionName,
                           const std::string& comment,
                           SaveMode mode,
                           std::string& errorMsg);

    /**
     * @brief Creates a simple snapshot of the current DB state in the 'versions' collection.
     * @param versionName The version tag.
     * @param comment User comments.
     * @param[out] errorMsg Buffer for error output.
     * @return True on success.
     */
    bool createVersionSnapshot(const std::string& versionName,
                               const std::string& comment,
                               std::string& errorMsg);

    // --- CRUD Operations ---
    bool createSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg);
    bool updateSpaceObject(const nlohmann::json& objectData, const std::string& localPicturePath, std::string& errorMsg);
    bool deleteSpaceObjectById(int64_t id);

    // --- Sets Management ---
    std::set<std::string> getAllUniqueSetNames();
    std::vector<nlohmann::json> getSpaceObjectsBySets(const std::set<std::string>& setNames);
    bool addObjectToSet(int64_t objectId, const std::string& setName);
    bool removeObjectFromSet(int64_t objectId, const std::string& setName);
    bool createSet(const std::string& setName);
    bool deleteSet(const std::string& setName);

    // --- Groups Management ---
    std::set<std::string> getAllUniqueGroupNames();
    bool createGroup(const std::string& groupName);
    bool deleteGroup(const std::string& groupName);
    std::vector<nlohmann::json> getSpaceObjectsByGroups(const std::set<std::string>& groupNames);
    bool addObjectToGroup(int64_t objectId, const std::string& groupName);
    bool removeObjectFromGroup(int64_t objectId, const std::string& groupName);

    /**
     * @brief Accessor for the GridFS Image Manager.
     * @return Reference to the internal image manager.
     */
    GridFSImageManager& getImageManager() { return _imageManager; }

private:
    mongocxx::client _client;
    mongocxx::database _db;
    mongocxx::collection _collection;
    mongocxx::collection _setsCollection;
    mongocxx::collection _groupsCollection;
    mongocxx::collection _versionsCollection;

    GridFSImageManager _imageManager;

    // Helper for full snapshots
    bool createVersionSnapshotInternal(const std::vector<nlohmann::json>& data,
                                       const std::string& versionName,
                                       const std::string& comment,
                                       std::string& errorMsg);

    // Helper for incremental saving logic
    bool createIncrementalVersion(const std::vector<nlohmann::json>& added,
                                  const std::vector<nlohmann::json>& modified,
                                  const std::vector<int64_t>& deletedIds,
                                  const std::vector<std::string>& setsAdded,
                                  const std::vector<std::string>& setsDeleted,
                                  const std::vector<std::string>& groupsAdded,
                                  const std::vector<std::string>& groupsDeleted,
                                  const std::string& versionName,
                                  const std::string& comment,
                                  std::string& errorMsg);
};

#endif // SPACEOBJECTDBMANAGER_H
