/// @file gridfs_image_manager.h
/// @brief Defines the **GridFSImageManager** class for handling file operations (upload, download, delete, list)
/// using MongoDB's GridFS feature.

#ifndef GRIDFS_IMAGE_MANAGER_H
#define GRIDFS_IMAGE_MANAGER_H

#include <string>
#include <vector> // Required for std::vector in getAllImageNames
#include <mongocxx/database.hpp>
#include <mongocxx/gridfs/bucket.hpp>
#include <mongocxx/collection.hpp>

/**
 * @class GridFSImageManager
 * @brief Manages storage and retrieval of binary data (like images) using MongoDB's GridFS.
 *
 * This class abstracts the complexities of the mongocxx GridFS API, providing simple methods
 * to handle files based on their names. It utilizes a mongocxx::gridfs::bucket
 * for all file operations.
 *
 * @note This class assumes the required MongoDB connection and database initialization
 * have been handled externally.
 */
class GridFSImageManager {
public:
    /**
     * @brief Constructor that initializes the GridFS bucket.
     *
     * The bucket is instantiated using the provided mongocxx::database object,
     * which handles the connection to the 'fs' collection namespace by default.
     *
     * @param db A reference to the initialized MongoDB database object.
     */
    explicit GridFSImageManager(mongocxx::database& db);

    /**
     * @brief Uploads image data to GridFS.
     *
     * Stores the raw image data as a file in the GridFS bucket under the given name.
     *
     * @param nameInDB The name under which the file will be stored in GridFS (e.g., "satellite_image_01").
     * @param imageData The raw binary data of the image (as a string or binary buffer).
     * @return **true** if the upload was successful; **false** otherwise.
     */
    bool uploadImage(const std::string& nameInDB, const std::string& imageData);

    /**
     * @brief Downloads image data from GridFS by its filename.
     *
     * Retrieves the stored binary data associated with the given filename.
     *
     * @param nameInDB The name of the file to retrieve from GridFS.
     * @return The raw binary data of the image as a std::string; returns an empty string if the file is not found or if an error occurs.
     */
    std::string downloadImageByName(const std::string& nameInDB);

    /**
     * @brief Deletes a file from GridFS by its filename.
     *
     * Locates and deletes both the file chunks and the file entry from the GridFS collections.
     *
     * @param nameInDB The name of the file to delete from GridFS.
     * @return **true** if the deletion was successful; **false** otherwise (e.g., file not found).
     */
    bool deleteImageByName(const std::string& nameInDB);

    /**
     * @brief Checks if a file with the given name exists in GridFS.
     *
     * @param filename The name of the file to check for existence.
     * @return **true** if the file exists; **false** otherwise.
     */
    bool exists(const std::string& filename);

    /**
     * @brief Retrieves a list of all filenames stored in GridFS.
     *
     * This operation typically queries the 'fs.files' collection in MongoDB.
     *
     * @return std::vector<std::string> A vector containing the names of all files/images stored.
     */
    std::vector<std::string> getAllImageNames();

private:
    /** @name GridFS Components */
    ///@{
    mongocxx::gridfs::bucket _gridfsBucket; ///< The primary GridFS interface for I/O operations.
    mongocxx::collection _gridfsFilesCollection; ///< A handle to the underlying 'fs.files' collection for custom queries (e.g., exists, listing).
    ///@}
};


#endif // GRIDFS_IMAGE_MANAGER_H
