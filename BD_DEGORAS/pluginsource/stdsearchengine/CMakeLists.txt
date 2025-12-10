cmake_minimum_required(VERSION 3.16)
project(stdsearchengine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# --- CORRECCIÓN: FORZAR VERSIÓN DE QT (VCPKG) ---
# Le decimos a CMake que busque Qt AQUÍ y no en C:/Qt
set(CMAKE_PREFIX_PATH "T:/vcpkg/installed/x64-mingw-dynamic-degoras")

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# --- RUTAS DE ARCHIVOS ---
set(MAIN_PROJECT_DIR "T:/workspace/DegorasProject/Bd_Degoras")

if(NOT EXISTS "${MAIN_PROJECT_DIR}/class_spaceobject.cpp")
    message(FATAL_ERROR "No encuentro class_spaceobject.cpp en: ${MAIN_PROJECT_DIR}")
endif()

set(SOURCES
    stdsearchengine.h
    stdsearchengine.cpp
    "${MAIN_PROJECT_DIR}/class_spaceobject.cpp"
    "${MAIN_PROJECT_DIR}/interface_plugin.cpp"
)

add_library(stdsearchengine SHARED ${SOURCES})

target_link_libraries(stdsearchengine PRIVATE Qt6::Core Qt6::Network)
target_include_directories(stdsearchengine PRIVATE "${MAIN_PROJECT_DIR}")