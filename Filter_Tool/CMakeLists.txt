cmake_minimum_required(VERSION 3.16)
project(DegorasProject VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define Paths to Dependencies ---
# Using set() is fine for local development.

find_package(unofficial-qwt CONFIG REQUIRED)

# --- Find Packages ---
# Find Qt and your other libraries. We do NOT find Qwt here because we set it manually.
find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Charts)
find_package(LibDegorasBase CONFIG REQUIRED)
find_package(LibDegorasSLR CONFIG REQUIRED)
find_package(LibNovasCpp CONFIG REQUIRED)

# --- Project Setup ---
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appDegorasProject
    main.cpp
)

# NOTE: You list main.cpp in both qt_add_executable and qt_add_qml_module.
# This is usually not necessary. The QML module will compile the sources and
# they will be linked into the final executable.
qt_add_qml_module(appDegorasProject
    URI DegorasProject
    QML_FILES
        Main.qml
    SOURCES
        class_mainwindow.cpp
        class_mainwindow.h
RESOURCES form_mainwindow.ui
SOURCES class_mainwindow.cpp class_mainwindow.h errorplot.cpp errorplot.h histogramplot.cpp histogramplot.h main.cpp plot.cpp plot.h qwt_slrplot_picker.cpp qwt_slrplot_picker.h tracking_data.cpp tracking_data.h
RESOURCES CMakeLists.txt form_mainwindow.ui
SOURCES cpf_predictor.h
SOURCES cpf_predictor.cpp
)

set_target_properties(appDegorasProject PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)
target_include_directories(appDegorasProject PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${_autogen_dir}/include)

# --- Link Libraries (Corrected) ---
target_link_libraries(appDegorasProject
    PRIVATE
    Qt6::Quick
    Qt6::Charts
    Qt6::Widgets
    LibDegorasBase::LibDegorasBase
    LibDegorasSLR::LibDegorasSLR
    LibNovasCpp::LibNovasCpp
    DP_Core
    unofficial::qwt::qwt
)

# --- Include Directories ---
# This must come after the target is defined.
target_include_directories(appDegorasProject PRIVATE ${QWT_INCLUDE_DIR})

# --- Add Subdirectory ---


# --- Installation and Post-Build Steps ---
include(GNUInstallDirs)
install(TARGETS appDegorasProject
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wpedantic -Wall -Wextra -O0 -fopenmp")
    else()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -O3 -fopenmp")
    endif()
else()
    message(FATAL_ERROR "Compiler not supported by default.")
endif()

# Static linking for MinGW runtime libs.
if (MINGW)
        target_link_options(appDegorasProject PRIVATE -static-libgcc -static-libstdc++)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_QWT_BIN_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    set(_QWT_DLL "${_QWT_BIN_DIR}/qwt.dll")

    if(EXISTS "${_QWT_DLL}")
        add_custom_command(TARGET appDegorasProject POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "[DEGORAS] Copying Qwt runtime library (Debug build)..."
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_QWT_DLL}"
                "$<TARGET_FILE_DIR:appDegorasProject>/qwt.dll"
            VERBATIM
        )
    else()
        message(WARNING "[DEGORAS] qwt.dll not found in ${_QWT_BIN_DIR}")
    endif()
endif()

if(NOT DEFINED _VCPKG_INSTALLED_DIR)
    set(_VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed")
endif()

set(_QT_CONF_PATH "$<TARGET_FILE_DIR:appDegorasProject>/qt.conf")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET appDegorasProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[DEGORAS] Generating qt.conf at: ${_QT_CONF_PATH}"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${_QT_CONF_PATH}"
        COMMAND ${CMAKE_COMMAND} -E echo "[Paths]" > "${_QT_CONF_PATH}"
        COMMAND ${CMAKE_COMMAND} -E echo "Prefix=${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}" >> "${_QT_CONF_PATH}"
        COMMAND ${CMAKE_COMMAND} -E echo "Plugins=Qt6/plugins" >> "${_QT_CONF_PATH}"
        COMMAND ${CMAKE_COMMAND} -E echo "Libraries=lib" >> "${_QT_CONF_PATH}"
        COMMAND ${CMAKE_COMMAND} -E echo "Binaries=bin" >> "${_QT_CONF_PATH}"
        VERBATIM
    )
else()
    message(STATUS "[DEGORAS] qt.conf generation disabled for Debug build")
endif()
