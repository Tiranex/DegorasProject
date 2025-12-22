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

#ifndef LOGGER_SETUP_H
#define LOGGER_SETUP_H

// STD INCLUDES
#include <string>
#include <chrono>
#include <memory>
#include <filesystem>
#include <vector>

// SPDLOG INCLUDES
#include <spdlog/spdlog.h>
#include <spdlog/async.h>

/**
 * @brief Global configuration for spdlog asynchronous logging.
 *
 * This structure holds settings that affect the shared async thread pool
 * and the periodic flushing policy.
 */
struct SpdlogGlobalConfig
{
    /**
     * @brief Default constructor initializing recommended values.
     */
    SpdlogGlobalConfig() noexcept :
        queue_size(8192),
        thread_count(1),
        flush_interval(std::chrono::seconds{3}),
        use_flush_every(true)
    {}

    std::size_t queue_size;              ///< Global async thread pool queue size.
    std::size_t thread_count;            ///< Global async thread pool worker thread count.
    std::chrono::seconds flush_interval; ///< Interval used for spdlog::flush_every().
    bool use_flush_every;                ///< Enable periodic flushing with flush_every().
};

/**
 * @brief Per-logger configuration for spdlog asynchronous loggers.
 *
 * This structure holds settings for each individual logger: sinks, levels
 * and overflow policy.
 */
struct SpdlogLogConfig
{
    /**
     * @brief Default constructor initializing recommended values.
     */
    SpdlogLogConfig() noexcept :
        logger_name(std::string()),
        file_path(std::string()),
        log_pattern("[%Y-%m-%dT%H:%M:%S.%f][%P][%t][%^%L%$] %v"),
        enable_console(true),
        enable_file(false),
        set_default(false),
        console_level(spdlog::level::info),
        file_level(spdlog::level::debug),
        logger_level(spdlog::level::trace),
        flush_on(spdlog::level::warn),
        overflow_pol(spdlog::async_overflow_policy::overrun_oldest),
        use_daily_file(true)
    {}

    std::string logger_name;                     ///< Logger name (used in spdlog registry).
    std::string file_path;                       ///< Path to the log file (daily or basic sink).
    std::string log_pattern;                     ///< Pattern for the logs.
    bool enable_console;                         ///< Enable console sink.
    bool enable_file;                            ///< Enable file sink.
    bool set_default;                            ///< Set this logger as the global default logger.
    spdlog::level::level_enum console_level;     ///< Minimum log level for console sink.
    spdlog::level::level_enum file_level;        ///< Minimum log level for file sink.
    spdlog::level::level_enum logger_level;      ///< Minimum log level accepted by the logger.
    spdlog::level::level_enum flush_on;          ///< Force flush when log >= this level.
    spdlog::async_overflow_policy overflow_pol;  ///< Overflow handling when queue is full.
    bool use_daily_file;                         ///< Use daily_file_sink_mt (true) or basic_file_sink_mt (false).
};

// FUNCTION DECLARATIONS

/**
 * @brief Initialize global spdlog async thread pool and time behavior.
 *
 * @param cfg Global configuration for async logging.
 */
void initSpdlog(const SpdlogGlobalConfig& cfg);

/**
 * @brief Create and register an asynchronous spdlog logger using SpdlogLogConfig.
 *
 * Uses the global async thread pool initialized by initSpdlog().
 *
 * @param cfg Per-logger configuration structure.
 * @return std::shared_ptr<spdlog::logger> The created logger, or nullptr if no sinks are enabled.
 */
std::shared_ptr<spdlog::logger> registerSpdlogLogger(const SpdlogLogConfig& cfg);

/**
 * @brief Get the directory of the current executable.
 * @return std::filesystem::path Path to the executable directory.
 */
std::filesystem::path getExecutableDir();

#endif
