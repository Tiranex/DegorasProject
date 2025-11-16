#-------------------------------------------------
#
# Project created by QtCreator 2019-02-04T12:21:03
#
#-------------------------------------------------

include ($$_PRO_FILE_PWD_/../DP_Locations.pri)
include ($$_PRO_FILE_PWD_/../DP_ExternalDependencies.pri)

TARGET = DP_Core
DEFINES += SP_CORE_LIBRARY
DESTDIR = $$DP_DEPLOY/lib

QT       += core gui widgets texttospeech multimedia
TEMPLATE = lib
CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x070000


INCLUDEPATH += $$_PRO_FILE_PWD_/resources/includes


SOURCES += \
    sources/class_aboutdialog.cpp\
    sources/class_autoclosedialog.cpp\
    sources/class_aboutpluginsdialog.cpp \
    sources/class_curlmanager.cpp \
    sources/class_curlmanagerfactory.cpp \
    sources/class_pluginssummary.cpp \
    sources/class_salaramainwindowcontroller.cpp \
    sources/class_salarasettings.cpp \
    sources/class_spaceobjectmodel.cpp \
    sources/class_spaceobject.cpp\
    sources/class_spaceobjectdisplaywidget.cpp \
    sources/class_spaceobjectfilemanager.cpp\
    sources/class_spaceobjectsmodelloader.cpp \
    sources/class_treemodel.cpp\
    sources/class_prediction.cpp \
    sources/class_predictionmodel.cpp \
    sources/class_predictionfilemanager.cpp \
    sources/class_cpffilemanager.cpp \
    sources/class_salarainformation.cpp \
    sources/class_globalutils.cpp \
    sources/class_timeprogressdialog.cpp \
    sources/class_passworddialog.cpp \
    sources/global_styles.cpp \
    sources/interface_plugin.cpp \
    sources/class_calibration.cpp \
    sources/class_calibrationfilemanager.cpp \
    sources/class_tracking.cpp \
    sources/class_trackingfilemanager.cpp \
    sources/class_meteodata.cpp \
    sources/class_statsfilemanager.cpp \



HEADERS += \
    includes/class_aboutdialog.h\
    includes/class_autoclosedialog.h \
    includes/class_aboutpluginsdialog.h \
    includes/class_curlmanager.h \
    includes/class_curlmanagerfactory.h \
    includes/class_guiloader.h \
    includes/class_pluginssummary.h \
    includes/class_salaramainwindowcontroller.h \
    includes/class_salarasettings.h\
    includes/class_spaceobject.h\
    includes/class_spaceobjectdisplaywidget.h \
    includes/class_spaceobjectfilemanager.h \
    includes/class_spaceobjectmodel.h\
    includes/class_spaceobjectsmodelloader.h \
    includes/class_treemodel.h\
    includes/global_texts.h\
    includes/interface_cpfdownloadengine.h \
    includes/interface_externaltool.h\
    includes/interface_plugin.h\
    includes/class_prediction.h \
    includes/class_predictionmodel.h \
    includes/class_predictionfilemanager.h \
    includes/class_cpffilemanager.h \
    includes/class_singleton.h \
    includes/class_salarainformation.h \
    includes/class_globalutils.h\
    includes/class_timeprogressdialog.h \
    includes/class_passworddialog.h \
    includes/global_styles.h \
    includes/interface_spaceobjectsearchengine.h \
    includes/interface_tle_propagator.h \
    includes/interface_tledownloadengine.h \
    includes/spcore_global.h \
    includes/class_calibration.h \
    includes/class_calibrationfilemanager.h \
    includes/class_tracking.h \
    includes/class_trackingfilemanager.h \
    includes/class_meteodata.h \
    includes/class_statsfilemanager.h \



RESOURCES += resources/common_resources.qrc

FORMS += \
    forms/form_aboutdialog.ui \
    forms/form_autoclosedialog.ui \
    forms/form_pluginsabouts.ui \
    forms/form_pluginssummary.ui \
    forms/form_spaceobjectdisplaywidget.ui \
    forms/passworddialog.ui \

OTHER_FILES += \
    resources/examples/* \
    resources/libs/* \
    resources/includes/*.h \
    resources/includes/curl/*.h \
    resources/cfg/*.ini \
    resources/cfg/SP_PluginsConfigFiles/*.ini \
    resources/cfg/SP_PluginsConfigFiles/NPOrbit/* \
    resources/cfg/TLE2CPF/tle2cpf.exe \
    resources/cfg/cacert.pem

DISTFILES += \
    resources/cfg/SP_GlobalConfigLink.ini


dep.files += $$_PRO_FILE_PWD_/resources/libs/*.dll
dep.path += $$DP_DEPLOY/bin

INSTALLS += dep


win32-msvc* { # For MSVC
    QMAKE_CXXFLAGS += -openmp
    QMAKE_CXXFLAGS += -D "_CRT_SECURE_NO_WARNINGS"
    QMAKE_CXXFLAGS_RELEASE *= -O2
    LIBS += -L$$_PRO_FILE_PWD_/resources/libs -llibcurl
}
else : win32-g++ { # For GCC
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp
    QMAKE_CXXFLAGS_RELEASE *= -O3
    LIBS += -fopenmp -L$$_PRO_FILE_PWD_/resources/libs -lcurl-x64
}
