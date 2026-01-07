#include "loggersetup.h"
#include <iostream>

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

void initSpdlog(const SpdlogGlobalConfig& cfg)
{
    // Initialize the global thread pool for async logging
    spdlog::init_thread_pool(cfg.queue_size, cfg.thread_count);

    // Set up periodic flushing to ensure logs are written even if a crash occurs later
    if (cfg.use_flush_every)
        spdlog::flush_every(cfg.flush_interval);
}

std::shared_ptr<spdlog::logger> registerSpdlogLogger(const SpdlogLogConfig& cfg)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.reserve(2);

    // 1. Configure Console Sink (Colored Output)
    if (cfg.enable_console)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(cfg.log_pattern);
        console_sink->set_level(cfg.console_level);
        sinks.push_back(console_sink);
    }

    // 2. Configure File Sink (Daily Rotation or Basic Append)
    if (cfg.enable_file)
    {
        spdlog::sink_ptr file_sink;

        if (cfg.use_daily_file)
            // Rotates daily at 00:00
            file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(cfg.file_path, 0, 0);
        else
            // Simple file append
            file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(cfg.file_path, false);

        file_sink->set_pattern(cfg.log_pattern);
        file_sink->set_level(cfg.file_level);
        sinks.push_back(file_sink);
    }

    // Return null if no sinks were configured
    if (sinks.empty())
        return nullptr;

    // 3. Create Async Logger
    auto logger = std::make_shared<spdlog::async_logger>(
        cfg.logger_name,
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),
        cfg.overflow_pol);

    // Global settings for this logger
    logger->set_level(cfg.logger_level);
    logger->flush_on(cfg.flush_on);

    // Register globally so it can be accessed via spdlog::get("name")
    spdlog::register_logger(logger);

    if (cfg.set_default)
        spdlog::set_default_logger(logger);

    return logger;
}

std::filesystem::path getExecutableDir()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    // GetModuleFileName is the Windows API to get the current process path
    DWORD size = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (size == 0 || size == MAX_PATH)
        return std::filesystem::current_path();
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    char buffer[4096];
    // Read the symbolic link /proc/self/exe on Linux
    ssize_t size = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
    if (size <= 0)
        return std::filesystem::current_path();
    buffer[size] = '\0';
    return std::filesystem::path(buffer).parent_path();
#else
    // Fallback for unknown platforms
    return std::filesystem::current_path();
#endif
}
