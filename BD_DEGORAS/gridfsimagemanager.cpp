#include "GridFSImageManager.h"
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/exception/exception.hpp>
#include <spdlog/spdlog.h>


GridFSImageManager::GridFSImageManager(mongocxx::database& db)
    : _gridfsBucket(db.gridfs_bucket()),
    _gridfsFilesCollection(db["fs.files"])
{
}

bool GridFSImageManager::uploadImage(const std::string& nameInDB, const std::string& imageData)
{
    try {
        auto uploader = _gridfsBucket.open_upload_stream(nameInDB);
        uploader.write(
            reinterpret_cast<const std::uint8_t*>(imageData.data()),
            imageData.length()
            );
        uploader.close();

        spdlog::info("Image successfully uploaded to GridFS: {}", nameInDB);
        return true;
    } catch (const mongocxx::exception &e) {
        spdlog::error("Failed in uploadImage: {}", e.what());
        return false;
    }
}

std::string GridFSImageManager::downloadImageByName(const std::string& nameInDB)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("filename", nameInDB));
        auto result = _gridfsFilesCollection.find_one(filter.view());

        if (!result) {
            spdlog::warn("File not found in fs.files: {}", nameInDB);
            return std::string{};
        }

        bsoncxx::document::view file_view = result->view();

        if (file_view["_id"].type() == bsoncxx::type::k_undefined ||
            file_view["length"].type() == bsoncxx::type::k_undefined) {
            spdlog::error("File in fs.files is missing _id or length.");
            return std::string{};
        }

        auto file_id = file_view["_id"].get_value();
        auto fileSize = file_view["length"].get_int64().value;

        auto downloader = _gridfsBucket.open_download_stream(file_id);
        std::string imageData;
        imageData.resize(fileSize);
        downloader.read(reinterpret_cast<std::uint8_t*>(&imageData[0]), fileSize);

        spdlog::info("Image successfully downloaded from GridFS: {}", nameInDB);
        return imageData;
    } catch (const mongocxx::exception &e) {
        spdlog::error("Failed in downloadImageByName: {}", e.what());
        return std::string{};
    }
}

bool GridFSImageManager::deleteImageByName(const std::string& nameInDB)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("filename", nameInDB));
        auto result = _gridfsFilesCollection.find_one(filter.view());

        if (!result) {
            spdlog::warn("File to delete not found: {}", nameInDB);
            return false;
        }

        auto file_id = result->view()["_id"].get_value();
        _gridfsBucket.delete_file(file_id);

        spdlog::info("Image successfully deleted from GridFS: {}", nameInDB);
        return true;
    } catch (const mongocxx::exception &e) {
        spdlog::error("Failed in deleteImageByName: {}", e.what());
        return false;
    }
}

bool GridFSImageManager::exists(const std::string& filename)
{
    try {
        bsoncxx::builder::basic::document filter{};
        filter.append(bsoncxx::builder::basic::kvp("filename", filename));
        return _gridfsFilesCollection.count_documents(filter.view()) > 0;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> GridFSImageManager::getAllImageNames()
{
    std::vector<std::string> names;
    try {
        auto cursor = _gridfsFilesCollection.find({});
        for (auto&& doc : cursor) {
            if (doc["filename"] && doc["filename"].type() == bsoncxx::type::k_string) {
                names.push_back(std::string(doc["filename"].get_string().value));
            }
        }
    } catch (...) {}
    return names;
}
