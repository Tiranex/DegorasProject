/***********************************************************************************************************************
 * Copyright (C) 2025 Degoras Project Team
 *
 * Authors:
 * Ángel Vera Herrera
 * <avera@roa.es>
 * <angelvh.engr@gmail.com>
 * Jesús Relinque Madroñal
 *
 * Licensed under the MIT License.
 **********************************************************************************************************************/

#include "LoggerSetup.h"
#include <iostream>

// SPDLOG SINKS
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

// PLATFORM-SPECIFIC
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

void initSpdlog(const SpdlogGlobalConfig& cfg)
{
    // Initialize global async thread pool.
    spdlog::init_thread_pool(cfg.queue_size, cfg.thread_count);

    // Optionally enable periodic flushing for all registered loggers.
    if (cfg.use_flush_every)
        spdlog::flush_every(cfg.flush_interval);
}

std::shared_ptr<spdlog::logger> registerSpdlogLogger(const SpdlogLogConfig& cfg)
{
    // Container.
    std::vector<spdlog::sink_ptr> sinks;
    sinks.reserve(2);

    // Console sink.
    if (cfg.enable_console)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(cfg.log_pattern);
        console_sink->set_level(cfg.console_level);
        sinks.push_back(console_sink);
    }

    // File sink.
    if (cfg.enable_file)
    {
        spdlog::sink_ptr file_sink;

        if (cfg.use_daily_file)
            file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(cfg.file_path, 0, 0);
        else
            file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(cfg.file_path, false);

        // Configure the sink.
        file_sink->set_pattern(cfg.log_pattern);
        file_sink->set_level(cfg.file_level);
        sinks.push_back(file_sink);
    }

    // If no sinks at all do NOT create logger.
    if (sinks.empty())
        return nullptr;

    // Create async logger using the global thread pool.
    auto logger = std::make_shared<spdlog::async_logger>(
        cfg.logger_name,
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),
        cfg.overflow_pol);

    // Set the level and the flush on.
    logger->set_level(cfg.logger_level);
    logger->flush_on(cfg.flush_on);

    // Register logger in spdlog registry.
    spdlog::register_logger(logger);

    // Optionally set as default logger.
    if (cfg.set_default)
        spdlog::set_default_logger(logger);

    // Return the logger.
    return logger;
}

std::filesystem::path getExecutableDir()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    DWORD size = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (size == 0 || size == MAX_PATH)
        return std::filesystem::current_path();
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    char buffer[4096];
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
