/// @file gridfsimagemanager.h
/// @brief Defines the GridFSImageManager class for handling MongoDB GridFS file operations.

#ifndef GRIDFS_IMAGE_MANAGER_H
#define GRIDFS_IMAGE_MANAGER_H

#include <string>
#include <vector>
#include <mongocxx/database.hpp>
#include <mongocxx/gridfs/bucket.hpp>
#include <mongocxx/collection.hpp>

/**
 * @class GridFSImageManager
 * @brief Manages storage, retrieval, deletion, and listing of binary image data using MongoDB's GridFS.
 *
 * This class provides a high-level abstraction over the mongocxx GridFS API.
 * It simplifies operations by using filenames as the primary key for interaction,
 * although internally it handles the necessary GridFS file IDs.
 *
 * @note This class assumes the passed `mongocxx::database` object is already initialized and connected.
 */
class GridFSImageManager {
public:
    /**
     * @brief Constructs the manager and initializes the GridFS bucket.
     *
     * The bucket is created using the provided database connection.
     * By default, it operates on the 'fs' bucket namespace.
     *
     * @param db A reference to an active MongoDB database object.
     */
    explicit GridFSImageManager(mongocxx::database& db);

    /**
     * @brief Uploads an image (or any binary data) to GridFS.
     *
     * This method opens an upload stream and writes the data to a new file in GridFS.
     *
     * @param nameInDB The unique filename to assign in GridFS (e.g., "satellite_01.jpg").
     * @param imageData The raw binary content of the file.
     * @return true if the upload completes successfully, false otherwise.
     */
    bool uploadImage(const std::string& nameInDB, const std::string& imageData);

    /**
     * @brief Downloads an image's data from GridFS given its filename.
     *
     * @param nameInDB The filename of the image to retrieve.
     * @return A std::string containing the binary data of the file. Returns an empty string on failure.
     */
    std::string downloadImageByName(const std::string& nameInDB);

    /**
     * @brief Deletes a file from GridFS using its filename.
     *
     * This operation removes both the file metadata (from `fs.files`) and its chunks (from `fs.chunks`).
     *
     * @param nameInDB The filename of the file to delete.
     * @return true if the file was found and deleted, false otherwise.
     */
    bool deleteImageByName(const std::string& nameInDB);

    /**
     * @brief Checks if a file with the specified name exists in the GridFS bucket.
     *
     * @param filename The name of the file to query.
     * @return true if the file exists, false otherwise.
     */
    bool exists(const std::string& filename);

    /**
     * @brief Retrieves a list of all filenames currently stored in the GridFS bucket.
     *
     * @return A vector of strings containing all filenames found in `fs.files`.
     */
    std::vector<std::string> getAllImageNames();

private:
    /** @name MongoDB Components */
    ///@{
    mongocxx::gridfs::bucket _gridfsBucket;         ///< The GridFS bucket interface for stream operations.
    mongocxx::collection _gridfsFilesCollection;    ///< Direct handle to 'fs.files' for metadata queries.
    ///@}
};

#endif // GRIDFS_IMAGE_MANAGER_H
