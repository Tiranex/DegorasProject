#include "GridFSImageManager.h"
#include <iostream>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/exception/exception.hpp>

// --- CONSTRUCTOR ---
GridFSImageManager::GridFSImageManager(mongocxx::database& db)
    : _gridfsBucket(db.gridfs_bucket()),
    _gridfsFilesCollection(db["fs.files"])
{
    // La inicialización ya se hace en la lista de inicialización
}

// --- UPLOAD ---
bool GridFSImageManager::uploadImage(const std::string& nameInDB, const std::string& imageData)
{
    try {
        auto uploader = _gridfsBucket.open_upload_stream(nameInDB);
        uploader.write(
            reinterpret_cast<const std::uint8_t*>(imageData.data()),
            imageData.length()
            );
        uploader.close();
        std::cout << "[Info] Image successfully uploaded to GridFS: " << nameInDB << std::endl;
        return true;
    } catch (const mongocxx::exception &e) {
        std::cerr << "[Error] Failed in uploadImage: " << e.what() << std::endl;
        return false;
    }
}

// --- DOWNLOAD ---
std::string GridFSImageManager::downloadImageByName(const std::string& nameInDB)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("filename", nameInDB));
        auto result = _gridfsFilesCollection.find_one(filter.view());

        if (!result) {
            std::cerr << "[Info] File not found in fs.files: " << nameInDB << std::endl;
            return std::string{};
        }

        bsoncxx::document::view file_view = result->view();

        if (file_view["_id"].type() == bsoncxx::type::k_undefined ||
            file_view["length"].type() == bsoncxx::type::k_undefined) {
            std::cerr << "[Error] File in fs.files is missing _id or length." << std::endl;
            return std::string{};
        }

        auto file_id = file_view["_id"].get_value();
        auto fileSize = file_view["length"].get_int64().value;

        auto downloader = _gridfsBucket.open_download_stream(file_id);
        std::string imageData;
        imageData.resize(fileSize);
        downloader.read(reinterpret_cast<std::uint8_t*>(&imageData[0]), fileSize);

        std::cout << "[Info] Image successfully downloaded from GridFS: " << nameInDB << std::endl;
        return imageData;
    } catch (const mongocxx::exception &e) {
        std::cerr << "[Error] Failed in downloadImageByName: " << e.what() << std::endl;
        return std::string{};
    }
}

// --- DELETE ---
bool GridFSImageManager::deleteImageByName(const std::string& nameInDB)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("filename", nameInDB));
        auto result = _gridfsFilesCollection.find_one(filter.view());

        if (!result) {
            std::cerr << "[Info] File to delete not found: " << nameInDB << std::endl;
            return false;
        }

        auto file_id = result->view()["_id"].get_value();
        _gridfsBucket.delete_file(file_id);
        std::cout << "[Info] Image successfully deleted from GridFS: " << nameInDB << std::endl;
        return true;
    } catch (const mongocxx::exception &e) {
        std::cerr << "[Error] Failed in deleteImageByName: " << e.what() << std::endl;
        return false;
    }
}
