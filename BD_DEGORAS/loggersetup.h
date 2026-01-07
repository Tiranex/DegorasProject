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
 * and the periodic flushing policy used by all async loggers.
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

    std::size_t queue_size;              ///< Global async thread pool queue size (power of 2 recommended).
    std::size_t thread_count;            ///< Global async thread pool worker thread count.
    std::chrono::seconds flush_interval; ///< Interval used for spdlog::flush_every().
    bool use_flush_every;                ///< Enable periodic flushing with flush_every().
};

/**
 * @brief Per-logger configuration for spdlog asynchronous loggers.
 *
 * This structure holds settings for each individual logger instance, including
 * sink types (file/console), log levels, and formatting patterns.
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

    std::string logger_name;                         ///< Unique name for the logger (key in spdlog registry).
    std::string file_path;                           ///< Full path to the log file.
    std::string log_pattern;                         ///< Formatting pattern for log messages.
    bool enable_console;                             ///< Enable logging to standard output (stdout/stderr).
    bool enable_file;                                ///< Enable logging to a file.
    bool set_default;                                ///< If true, sets this logger as the global default logger.
    spdlog::level::level_enum console_level;         ///< Minimum log level for the console sink.
    spdlog::level::level_enum file_level;            ///< Minimum log level for the file sink.
    spdlog::level::level_enum logger_level;          ///< Global minimum log level accepted by the logger.
    spdlog::level::level_enum flush_on;              ///< Severity level that triggers an immediate flush to disk.
    spdlog::async_overflow_policy overflow_pol;      ///< Policy when the message queue is full (block vs drop).
    bool use_daily_file;                             ///< True for daily rotation, False for simple append mode.
};

// FUNCTION DECLARATIONS

/**
 * @brief Initializes the global spdlog async thread pool.
 *
 * This must be called once at application startup before creating any async loggers.
 *
 * @param cfg Global configuration structure.
 */
void initSpdlog(const SpdlogGlobalConfig& cfg);

/**
 * @brief Creates and registers a new asynchronous spdlog logger.
 *
 * Configures sinks (file, console) based on the provided configuration
 * and registers the logger with the spdlog global registry.
 *
 * @param cfg Per-logger configuration structure.
 * @return std::shared_ptr<spdlog::logger> Shared pointer to the created logger, or nullptr on failure.
 */
std::shared_ptr<spdlog::logger> registerSpdlogLogger(const SpdlogLogConfig& cfg);

/**
 * @brief Retrieves the absolute directory path of the current executable.
 *
 * Used to construct reliable paths for log files and plugins relative to the binary location.
 *
 * @return std::filesystem::path Path to the executable's directory.
 */
std::filesystem::path getExecutableDir();

#endif // LOGGER_SETUP_H
