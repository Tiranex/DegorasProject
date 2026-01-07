#ifndef QTLOGSINK_H
#define QTLOGSINK_H

#include <spdlog/sinks/base_sink.h>
#include <QObject>
#include <mutex>

/**
 * @brief Signal emitter class for QtLogSink.
 * @details Separated from the template class to allow Qt MOC processing.
 */
class QtLogSinkSignal : public QObject
{
    Q_OBJECT
public:
    virtual ~QtLogSinkSignal() = default;

signals:
    /**
     * @brief Signal emitted when a new log message is received.
     * @param msg The formatted log message.
     * @param level The severity level (INFO, WARN, ERROR).
     */
    void logReceived(const QString& msg, const QString& level);
};

/**
 * @brief Custom Spdlog sink that redirects logs to Qt Signals.
 * @details Thread-safe sink that formats spdlog messages and emits them
 * via Qt signals so they can be displayed in UI widgets.
 * @tparam Mutex The mutex type for thread safety (usually std::mutex).
 */
template<typename Mutex>
class QtLogSink : public QtLogSinkSignal, public spdlog::sinks::base_sink<Mutex>
{
public:
    /**
     * @brief Access the singleton instance.
     * @return Reference to the static instance.
     */
    static QtLogSink<Mutex>& instance() {
        static QtLogSink<Mutex> s_instance;
        return s_instance;
    }

protected:
    /**
     * @brief Processes the log message.
     * @details Formats the message using spdlog's pattern and emits the signal.
     * @param msg The raw log message structure.
     */
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        QString qMsg = QString::fromUtf8(formatted.data(), static_cast<int>(formatted.size())).trimmed();

        QString level = "INFO";
        if (msg.level == spdlog::level::err || msg.level == spdlog::level::critical) level = "ERROR";
        else if (msg.level == spdlog::level::warn) level = "WARN";
        emit logReceived(qMsg, level);
    }

    void flush_() override {}
};

/// Alias for the thread-safe version using std::mutex.
using QtLogSinkMt = QtLogSink<std::mutex>;

#endif // QTLOGSINK_H
