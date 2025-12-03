#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include "class_mainwindow.h"

#include "degoras_settings.h"
#include <QDir>
#include <QDebug>
#include <QStandardPaths> // OPTIONAL

// Assuming these are available from your project
#include <global_utils.h>

int main( int argc, char **argv )
{
    QApplication a( argc, argv );

    // Initialize DegorasSettings -----------------

    QString configFilePath;

    // --- DEFAULT OPTION: .ini files in Project Directory (Portable) ---

    // WARNING: Make sure to have read/write permissions in folder
    QString appDir = QCoreApplication::applicationDirPath(); // Will be /FilterTool PREGUNTAR
    QDir portableDir(appDir);

    if(!portableDir.exists("config"))
    {
        portableDir.mkpath("config");
    }

    configFilePath = portableDir.filePath("config/settings.ini");

    // --- ALTERNATIVE: .ini files in StandardPath (Single User Config) ---
/*
    QString singleUserPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir singleUserDir(singleUserPath);

    if(!singleUserDir.exists())
    {
        singleUserDir.mkpath(".");
    }

    configFilePath = singleUserDir.filePath("settings.ini");
*/
    // --------------------------------------------
    // Initilize singleton
    DegorasSettings::instance().initialize(configFilePath);
    // --------------------------------------------




    // Initialize your application utilities if needed
    //GlobalUtils::initApp("Filter Tool", "Filter Tool", NAME_SPACEOBJECTSMANAGERCONFIGFILE, ICON_SPACEOBJECTSMANAGER);

    // Set dark theme style
    a.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    // Apply palette to the application
    a.setPalette(darkPalette);

    MainWindow w;
    w.show();

    return a.exec();
}
