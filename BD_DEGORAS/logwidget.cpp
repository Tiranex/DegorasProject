#include "logwidget.h"
#include <QVBoxLayout>
#include <QTextEdit>

LogWidget::LogWidget(QWidget *parent) : QWidget(parent)
{
    this->setWindowTitle("System Logs");
    this->resize(800, 400);

    QVBoxLayout* layout = new QVBoxLayout(this);
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    // Estilo "Hacker": Fondo oscuro, letra verde
    m_textEdit->setStyleSheet("background-color: #121212; color: #00ff00; font-family: Consolas, Monospace; font-size: 10pt;");
    layout->addWidget(m_textEdit);
}

LogWidget::~LogWidget()
{
}

void LogWidget::appendLog(const QString& text, const QString& level)
{
    QString color = "#00ff00"; // Por defecto VERDE (Info)

    // Elegir color según el nivel
    if (level.contains("ERR") || level.contains("FATAL")) {
        color = "#ff5555"; // ROJO claro
    }
    else if (level.contains("WARN")) {
        color = "#ffff55"; // AMARILLO claro
    }

    // Convertimos caracteres especiales a HTML para no romper el formato
    QString safeText = text.toHtmlEscaped();

    // Insertamos como HTML con el color incrustado
    // <pre> mantiene los espacios y saltos de línea originales
    QString html = QString("<span style='color:%1;'>%2</span>").arg(color, safeText);

    m_textEdit->append(html);
}
