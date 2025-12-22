#ifndef QTLOGSINK_H
#define QTLOGSINK_H

#include <spdlog/sinks/base_sink.h>
#include <QObject>
#include <mutex>

class QtLogSinkSignal : public QObject
{
    Q_OBJECT
public:
    virtual ~QtLogSinkSignal() = default;

signals:
    void logReceived(const QString& msg, const QString& level);
};

template<typename Mutex>
class QtLogSink : public QtLogSinkSignal, public spdlog::sinks::base_sink<Mutex>
{
public:
    static QtLogSink<Mutex>& instance() {
        static QtLogSink<Mutex> s_instance;
        return s_instance;
    }

protected:
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


using QtLogSinkMt = QtLogSink<std::mutex>;

#endif
