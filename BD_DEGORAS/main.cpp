#include "mainwindow.h"
#include "LoggerSetup.h"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include <mongocxx/instance.hpp>
#include <iostream>
#include "pluginmanager.h"


constexpr std::string_view kLogger1 = "DegorasMainLogger";

int main(int argc, char *argv[])
{

    mongocxx::instance instance{};


    QApplication a(argc, argv);

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

    std::string logs_dir = getExecutableDir().string() + "/logs";
    SpdlogGlobalConfig gcfg;
    gcfg.queue_size     = 8192;
    gcfg.thread_count   = 1;
    gcfg.flush_interval = std::chrono::seconds{5};
    gcfg.use_flush_every = true;

    SpdlogLogConfig cfg1;
    cfg1.logger_name    = std::string(kLogger1);
    cfg1.file_path = logs_dir + "/" + std::string(kLogger1) + ".log";
    cfg1.enable_console = true;
    cfg1.enable_file    = true;
    cfg1.set_default    = true;
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

    PluginManager::instance().loadPlugins(); // Busca en la carpeta /plugins junto al .exe
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
    spdlog::shutdown();

    return exitCode;
}
