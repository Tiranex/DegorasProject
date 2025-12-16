#pragma once
#include <QDialog>
#include <QSettings>

// Forward declarations
class QComboBox;
class QLineEdit;
class QCheckBox;

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    // Estructura para devolver los datos limpios al main
    struct ConnectionParams {
        QString host;
        int port;
        QString dbName;
        QString user;
        QString password;
        bool useSSL;
    };

    explicit ConnectionDialog(QWidget *parent = nullptr);
    ~ConnectionDialog();

    ConnectionParams getParams() const;

private slots:
    void on_connectBtn_clicked();
    void on_historyCombo_currentIndexChanged(int index);

private:
    void loadHistory();
    void saveHistory();

    // UI Elements
    QComboBox* historyCombo;
    QLineEdit* hostEd;
    QLineEdit* portEd;
    QLineEdit* dbEd;
    QLineEdit* userEd;
    QLineEdit* passEd;
    QCheckBox* sslCheck;

    ConnectionParams m_params;
};
