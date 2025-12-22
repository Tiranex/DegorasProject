# ======================================================================================================================
#  Copyright (C) 2025 Degoras Project Team
#
#  Authors:
#
#      Ángel Vera Herrera       <avera@roa.es> | <angelvh.engr@gmail.com>
#      Jesús Relinque Madroñal
#
#  Licensed under the MIT License.
# =================================T====================================================================================
#   APP SPACEOBJECT MANAGER - CMAKELIST (Versión QT)
# ======================================================================================================================

# ----------------------------------------------------------------------------------------------------------------------
# BASIC PROJECT CONFIGURATION

cmake_minimum_required(VERSION 3.31)
project(SpaceObjectManagerApp LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bd_Degoras")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bd_Degoras")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bd_Degoras")

set(CMAKE_SYSTEM_PROCESSOR x86_64 CACHE STRING "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)



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


find_package(bsoncxx CONFIG REQUIRED)

find_package(mongocxx CONFIG REQUIRED)

find_package(spdlog REQUIRED)

# ----------------------------------------------------------------------------------------------------------------------

set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    SpaceObjectDBManager.h
    SpaceObjectDBManager.cpp
    json_helpers.h
    json_helpers.cpp
    gridfsimagemanager.h
    gridfsimagemanager.cpp

    interface_plugin.h
    interface_plugin.cpp
    interface_spaceobjectsearchengine.h
    class_spaceobject.h
    class_spaceobject.cpp

    pluginmanager.h
    pluginmanager.cpp
)


add_executable(App_SpaceObjectManager
    ${PROJECT_SOURCES}
    addobjectdialog.h addobjectdialog.cpp addobjectdialog.ui
    loggersetup.h loggersetup.cpp
    qtlogsink.h
    logwidget.h logwidget.cpp
    connectiondialog.h connectiondialog.cpp
)

target_link_libraries(App_SpaceObjectManager PRIVATE
    mongo::mongocxx_static
    mongo::bsoncxx_static
    spdlog::spdlog
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


target_compile_definitions(App_SpaceObjectManager PRIVATE MONGOCXX_STATIC BSONCXX_STATIC)
target_compile_definitions(App_SpaceObjectManager PRIVATE SPDLOG_COMPILED_LIB)

# ----------------------------------------------------------------------------------------------------------------------
# COMPILER CONFIGURATION


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wpedantic -Wall -Wextra -O0")
    else()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -O3")
    endif()
else()
    message(FATAL_ERROR "Compiler not supported by default.")
endif()


if (MINGW)
        target_link_options(App_SpaceObjectManager PRIVATE -static-libgcc -static-libstdc++)
endif()

# ==============================================================================
#  AÑADIR PLUGIN: STANDARD SEARCH ENGINE
# ==============================================================================

set(PLUGIN_FILES
    pluginsource/stdsearchengine/stdsearchengine.h
    pluginsource/stdsearchengine/stdsearchengine.cpp
    class_spaceobject.cpp
    interface_plugin.cpp
)

add_library(stdsearchengine SHARED ${PLUGIN_FILES})
target_link_libraries(stdsearchengine PRIVATE Qt::Core Qt::Network)
target_include_directories(stdsearchengine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(stdsearchengine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bd_Degoras/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bd_Degoras/plugins"
)

add_dependencies(App_SpaceObjectManager stdsearchengine)


if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    add_custom_command(TARGET App_SpaceObjectManager POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                --dir "${CMAKE_BINARY_DIR}/Bd_Degoras"
                "$<TARGET_FILE:App_SpaceObjectManager>"
        COMMENT "Setting the release."
    )
endif()
