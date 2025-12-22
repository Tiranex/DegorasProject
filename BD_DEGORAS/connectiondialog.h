#pragma once
#include <QDialog>
#include <QSettings>

class QComboBox;
class QLineEdit;
class QCheckBox;

/**
 * @brief Dialog window for establishing a database connection.
 *
 * This class handles user input for MongoDB connection parameters (Host, Port, User, etc.)
 * and manages a history of previous connections using QSettings.
 */
class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Structure holding all necessary connection parameters.
     */
    struct ConnectionParams {
        QString host;       ///< Hostname or IP address (e.g., "127.0.0.1").
        int port;           ///< Port number (default 27017).
        QString dbName;     ///< Target database name.
        QString user;       ///< Username for authentication (optional).
        QString password;   ///< Password for authentication (optional).
        bool useSSL;        ///< Flag to enable SSL/TLS encryption.
    };

    /**
     * @brief Constructor.
     * @param parent The parent widget.
     */
    explicit ConnectionDialog(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ConnectionDialog();

    /**
     * @brief Retrieves the connection parameters entered by the user.
     * @return A ConnectionParams structure populated with UI data.
     */
    ConnectionParams getParams() const;

private slots:
    /**
     * @brief Validates input, saves history, and accepts the dialog.
     */
    void on_connectBtn_clicked();

    /**
     * @brief Fills the input fields when a history item is selected.
     * @param index The index of the selected item in the combo box.
     */
    void on_historyCombo_currentIndexChanged(int index);

private:
    /**
     * @brief Loads saved connections from the system registry/config (QSettings).
     */
    void loadHistory();

    /**
     * @brief Saves the current connection details to the system registry/config.
     */
    void saveHistory();

    // UI Components
    QComboBox* historyCombo;
    QLineEdit* hostEd;
    QLineEdit* portEd;
    QLineEdit* dbEd;
    QLineEdit* userEd;
    QLineEdit* passEd;
    QCheckBox* sslCheck;

    ConnectionParams m_params;
};
