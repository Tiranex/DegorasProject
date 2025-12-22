
#include "LoggerSetup.h"
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
    spdlog::init_thread_pool(cfg.queue_size, cfg.thread_count);

    if (cfg.use_flush_every)
        spdlog::flush_every(cfg.flush_interval);
}

std::shared_ptr<spdlog::logger> registerSpdlogLogger(const SpdlogLogConfig& cfg)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.reserve(2);

    if (cfg.enable_console)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(cfg.log_pattern);
        console_sink->set_level(cfg.console_level);
        sinks.push_back(console_sink);
    }

    if (cfg.enable_file)
    {
        spdlog::sink_ptr file_sink;

        if (cfg.use_daily_file)
            file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(cfg.file_path, 0, 0);
        else
            file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(cfg.file_path, false);

        file_sink->set_pattern(cfg.log_pattern);
        file_sink->set_level(cfg.file_level);
        sinks.push_back(file_sink);
    }

    if (sinks.empty())
        return nullptr;

    auto logger = std::make_shared<spdlog::async_logger>(
        cfg.logger_name,
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),
        cfg.overflow_pol);

    logger->set_level(cfg.logger_level);
    logger->flush_on(cfg.flush_on);

    spdlog::register_logger(logger);

    if (cfg.set_default)
        spdlog::set_default_logger(logger);

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
