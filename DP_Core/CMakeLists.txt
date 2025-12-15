cmake_minimum_required(VERSION 3.16)

project(DP_Core LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(LibDegorasBase CONFIG REQUIRED)
find_package(LibDegorasSLR CONFIG REQUIRED)
find_package(LibNovasCpp CONFIG REQUIRED)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)


# Header files (.h)
set(DP_CORE_HEADERS
    include/dpcore_global.h
    include/degoras_settings.h
    include/global_texts.h
    include/global_utils.h
    include/window_about_dialog.h
    include/window_message_box.h
    include/algorithms.h
    include/Tracking/calibration.h
    include/Tracking/calibrationfilemanager.h
    include/Tracking/meteodata.h
    include/Tracking/tracking.h
    include/Tracking/trackingfilemanager.h
    include/datafilter.h
    include/shortcutmanager.h
)

# Source files (.cpp)
set(DP_CORE_SOURCES
    sources/degoras_settings.cpp
    sources/global_utils.cpp
    sources/window_about_dialog.cpp
    sources/window_message_box.cpp
    sources/algorithms.cpp
    sources/Tracking/calibration.cpp
    sources/Tracking/calibrationfilemanager.cpp
    sources/Tracking/meteodata.cpp
    sources/Tracking/tracking.cpp
    sources/Tracking/trackingfilemanager.cpp
    sources/datafilter.cpp
    sources/shortcutmanager.cpp
)

# Qt Designer UI files (.ui)
set(DP_CORE_UI_FILES
    sources/forms/filetablewidget.ui
    sources/forms/form_about_dialog.ui
)

# --- Define the library target ---

# Create the library. With CMAKE_AUTOUIC enabled, it will handle the UI files.
add_library(DP_Core SHARED
    ${DP_CORE_HEADERS}
    ${DP_CORE_SOURCES}
    ${DP_CORE_UI_FILES}
)

target_link_libraries(DP_Core PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt6::Widgets LibDegorasBase::LibDegorasBase LibDegorasSLR::LibDegorasSLR
  LibNovasCpp::LibNovasCpp)

target_include_directories(DP_Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(DP_Core PRIVATE DP_CORE_LIBRARY)
