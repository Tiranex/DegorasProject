# ======================================================================================================================
#  Copyright (C) 2025 Degoras Project Team
#
#  Authors:
#
#      Ángel Vera Herrera       <avera@roa.es> | <angelvh.engr@gmail.com>
#      Jesús Relinque Madroñal
#
#  Licensed under the MIT License.
# =================================T=====================================================================================
#   APP SPACEOBJECT MANAGER - CMAKELIST (Versión QT)
# ======================================================================================================================

# ----------------------------------------------------------------------------------------------------------------------
# BASIC PROJECT CONFIGURATION

# Minimum CMake version required.
cmake_minimum_required(VERSION 3.31)

# Project definition.
project(SpaceObjectManagerApp LANGUAGES CXX)

# For avoid architecture detection warning.
set(CMAKE_SYSTEM_PROCESSOR x86_64 CACHE STRING "")

# Global configurations.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------------------------------------------------------------------------------------------------
# QT CONFIGURATION
# (Necesario para una app gráfica)

set(CMAKE_AUTOMOC ON)    # Ejecuta el "Meta-Object Compiler" automáticamente
set(CMAKE_AUTOUIC ON)    # Ejecuta el "User Interface Compiler" automáticamente
set(CMAKE_AUTORCC ON)    # Ejecuta el "Resource Compiler" automáticamente

# Busca las librerías de Qt (versión 6)
# Añadimos QUIET para que no sea tan ruidoso si lo encuentra rápido
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

# ----------------------------------------------------------------------------------------------------------------------
#  ENVIRONMENT SUMMARY

message(STATUS "===================================================================")
message(STATUS " Degoras Project - APP SPACEOBJECT MANAGER (QT)")
message(STATUS "-------------------------------------------------------------------")
message(STATUS "  CMAKE_SYSTEM_NAME                : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  CMAKE_SYSTEM_PROCESSOR           : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  CMAKE_TOOLCHAIN_FILE             : ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "  CMAKE_GENERATOR                  : ${CMAKE_GENERATOR}")
message(STATUS "  CMAKE_CXX_COMPILER               : ${CMAKE_CXX_COMPILER}")
message(STATUS "  CMAKE_CXX_COMPILER_ID            : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  CMAKE_MAKE_PROGRAM               : ${CMAKE_MAKE_PROGRAM}")
message(STATUS "  CMAKE_FIND_PACKAGE_PREFER_CONFIG : ${CMAKE_FIND_PACKAGE_PREFER_CONFIG}")
message(STATUS "  CMAKE_EXPORT_COMPILE_COMMANDS    : ${CMAKE_EXPORT_COMPILE_COMMANDS}")
message(STATUS "  CMAKE_COLOR_DIAGNOSTICS          : ${CMAKE_COLOR_DIAGNOSTICS}")
message(STATUS "  CMAKE_SOURCE_DIR                 : ${CMAKE_SOURCE_DIR}")
message(STATUS "  CMAKE_BINARY_DIR                 : ${CMAKE_BINARY_DIR}")
message(STATUS "  CMAKE_BUILD_TYPE                 : ${CMAKE_BUILD_TYPE}")
message(STATUS "  VCPKG_TARGET_TRIPLET             : ${VCPKG_TARGET_TRIPLET}")
message(STATUS "  VCPKG_HOST_TRIPLET               : ${VCPKG_HOST_TRIPLET}")
message(STATUS "  VCPKG_MANIFEST_MODE              : ${VCPKG_MANIFEST_MODE}")
message(STATUS "===================================================================")

# ----------------------------------------------------------------------------------------------------------------------
# DEPENDENCIES

# BSON C++
find_package(bsoncxx CONFIG REQUIRED)

# Mongo C++ driver
find_package(mongocxx CONFIG REQUIRED)

#Logs
find_package(spdlog REQUIRED)

# ----------------------------------------------------------------------------------------------------------------------
# BUILD TARGETS

# Definimos una variable con todos nuestros archivos de código
# (El .ui se gestiona automáticamente con CMAKE_AUTOUIC ON)
set(PROJECT_SOURCES
    main.cpp                # Nuevo main para Qt
    mainwindow.h            # Cabecera de la ventana
    mainwindow.cpp          # Lógica de la ventana
    mainwindow.ui
    SpaceObjectDBManager.h  # Reutilizado
    SpaceObjectDBManager.cpp  # Reutilizado
    json_helpers.h          # Reutilizado
    json_helpers.cpp
    gridfsimagemanager.h
    gridfsimagemanager.cpp
)

# Define the main executable target.
add_executable(App_SpaceObjectManager
    ${PROJECT_SOURCES}
    addobjectdialog.h addobjectdialog.cpp addobjectdialog.ui
    loggersetup.h loggersetup.cpp

)

# Link required libraries.
target_link_libraries(App_SpaceObjectManager PRIVATE
    mongo::mongocxx_static
    mongo::bsoncxx_static
    spdlog::spdlog
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Static Mongo and Bson.
target_compile_definitions(App_SpaceObjectManager PRIVATE MONGOCXX_STATIC BSONCXX_STATIC)
#Logs
target_compile_definitions(App_SpaceObjectManager PRIVATE SPDLOG_COMPILED_LIB)

# ----------------------------------------------------------------------------------------------------------------------
# COMPILER CONFIGURATION

# GCC-specific flags.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wpedantic -Wall -Wextra -O0")
    else()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -O3")
    endif()
else()
    message(FATAL_ERROR "Compiler not supported by default.")
endif()

# Static linking for MinGW runtime libs.
if (MINGW)
        target_link_options(App_SpaceObjectManager PRIVATE -static-libgcc -static-libstdc++)
endif()


