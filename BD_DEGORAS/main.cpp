/**
 * @file main.cpp
 * @brief Entry point for the Degoras Space Object Manager application.
 *
 * Initializes the MongoDB driver instance, Qt application, global logging (spdlog),
 * and loads available plugins before launching the main window.
 */

#include "mainwindow.h"
#include "loggersetup.h"
#include "pluginmanager.h"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include <mongocxx/instance.hpp>
#include <iostream>

/// Global logger name constant used throughout the application.
constexpr std::string_view kLogger1 = "DegorasMainLogger";

/**
 * @brief Main execution function.
 * * 1. Initializes MongoDB C++ Driver.
 * 2. Configures Qt Application style (Dark Fusion Theme).
 * 3. Sets up Asynchronous Logging (Spdlog).
 * 4. Loads Plugins via PluginManager.
 * 5. Launches MainWindow.
 */
int main(int argc, char *argv[])
{
    // The mongocxx::instance constructor initializes the driver.
    // It must be created once and exist for the lifetime of the application.
    mongocxx::instance instance{};

    QApplication a(argc, argv);

    // --- UI THEME SETUP (Dark Fusion) ---
    a.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    a.setPalette(darkPalette);

    // --- LOGGING SETUP ---
    // Ensure logs are stored next to the executable
    std::string logs_dir = getExecutableDir().string() + "/logs";

    SpdlogGlobalConfig gcfg;
    gcfg.queue_size      = 8192;
    gcfg.thread_count    = 1;
    gcfg.flush_interval = std::chrono::seconds{5};
    gcfg.use_flush_every = true;

    SpdlogLogConfig cfg1;
    cfg1.logger_name    = std::string(kLogger1);
    cfg1.file_path      = logs_dir + "/" + std::string(kLogger1) + ".log";
    cfg1.enable_console = true;
    cfg1.enable_file    = true;
    cfg1.set_default    = true; // Can be accessed via spdlog::info() directly
    cfg1.console_level  = spdlog::level::info;
    cfg1.file_level     = spdlog::level::debug;
    cfg1.logger_level   = spdlog::level::trace;
    cfg1.flush_on       = spdlog::level::warn;
    cfg1.use_daily_file = true;

    initSpdlog(gcfg);

    auto global_logger = registerSpdlogLogger(cfg1);
    if (!global_logger)
    {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    spdlog::info("Global logger [{}] initialized.", kLogger1);
    spdlog::info("Starting Degoras Application...");

    // --- PLUGIN LOADING ---
    PluginManager::instance().loadPlugins(); // Scans 'plugins' folder
    spdlog::info("Plugins loaded: {}", PluginManager::instance().getLoadedEngines().join(", ").toStdString());

    int exitCode = 0;

    try {
        MainWindow w;
        w.show();
        exitCode = a.exec();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error at startup: {}", e.what());
        return 1;
    }

    spdlog::info("Shutting down Degoras Application with code: {}", exitCode);
    spdlog::shutdown(); // Flush pending logs

    return exitCode;
}
