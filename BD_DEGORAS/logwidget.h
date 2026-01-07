#pragma once
#include <QWidget>

class QTextEdit;

/**
 * @brief A standalone window/widget for displaying colored system logs.
 *
 * This widget provides a read-only text area that visualizes log messages
 * in real-time. It supports color coding based on the log severity level
 * (e.g., Red for Errors, Yellow for Warnings).
 *
 * It is designed to be connected to the QtLogSink via signals and slots.
 */
class LogWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent The parent widget (usually nullptr for a standalone window).
     */
    explicit LogWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~LogWidget();

public slots:
    /**
     * @brief Appends a new log message to the display.
     *
     * @param text The raw log message content.
     * @param level The log severity level string (e.g., "INFO", "WARN", "ERROR").
     * This determines the color of the text.
     */
    void appendLog(const QString& text, const QString& level);

private:
    QTextEdit* m_textEdit; ///< The text area widget used to display the logs.
};
