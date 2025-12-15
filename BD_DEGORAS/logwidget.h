#pragma once
#include <QWidget>

class QTextEdit; // Forward declaration

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

public slots:
    void appendLog(const QString& text, const QString& level);

private:
    QTextEdit* m_textEdit;
};
