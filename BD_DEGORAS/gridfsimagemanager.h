#ifndef GRIDFS_IMAGE_MANAGER_H
#define GRIDFS_IMAGE_MANAGER_H

#include <string>
#include <mongocxx/database.hpp>
#include <mongocxx/gridfs/bucket.hpp>
#include <mongocxx/collection.hpp>

class GridFSImageManager {
public:
    // Constructor que recibe la base de datos para inicializar el bucket
    explicit GridFSImageManager(mongocxx::database& db);

    // Métodos movidos desde SpaceObjectDBManager
    bool uploadImage(const std::string& nameInDB, const std::string& imageData);
    std::string downloadImageByName(const std::string& nameInDB);
    bool deleteImageByName(const std::string& nameInDB);

    // Verificar si existe un archivo
    bool exists(const std::string& filename);
    // Listar todas las imágenes
    std::vector<std::string> getAllImageNames();

private:
    mongocxx::gridfs::bucket _gridfsBucket;
    mongocxx::collection _gridfsFilesCollection;
};


#endif // GRIDFS_IMAGE_MANAGER_H
