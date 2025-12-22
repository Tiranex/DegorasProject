#include "logwidget.h"
#include <QVBoxLayout>
#include <QTextEdit>

LogWidget::LogWidget(QWidget *parent) : QWidget(parent)
{
    this->setWindowTitle("System Logs");
    this->resize(800, 400);

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true); // User cannot edit the logs

    // Set a dark theme (Matrix style) for high contrast readability
    m_textEdit->setStyleSheet(
        "background-color: #121212; "
        "color: #00ff00; "
        "font-family: Consolas, Monospace; "
        "font-size: 10pt;"
        );

    layout->addWidget(m_textEdit);
}

LogWidget::~LogWidget()
{
}

void LogWidget::appendLog(const QString& text, const QString& level)
{
    // Default color is Matrix Green for INFO/TRACE
    QString color = "#00ff00";

    // Determine color based on severity string
    if (level.contains("ERR") || level.contains("FATAL")) {
        color = "#ff5555"; // Light Red for Errors
    }
    else if (level.contains("WARN")) {
        color = "#ffff55"; // Light Yellow for Warnings
    }

    // Sanitize the text to prevent HTML injection issues
    // e.g., if the log contains "<" or ">", they are converted to &lt; &gt;
    QString safeText = text.toHtmlEscaped();

    // Format as HTML span to apply color to this specific line
    QString html = QString("<span style='color:%1;'>%2</span>").arg(color, safeText);

    // Append preserves the previous content
    m_textEdit->append(html);
}
