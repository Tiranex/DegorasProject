#include "connectiondialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>

ConnectionDialog::ConnectionDialog(QWidget *parent) : QDialog(parent)
{
    this->setWindowTitle("Connect to Degoras DB");
    this->setModal(true);
    this->setMinimumWidth(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // History Combo Box
    historyCombo = new QComboBox(this);
    // Index 0 is reserved for "New Connection" to clear fields
    historyCombo->addItem("New Connection...");
    mainLayout->addWidget(new QLabel("Saved Connections:"));
    mainLayout->addWidget(historyCombo);

    // Separator Line
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // Form Layout for Inputs
    QFormLayout* form = new QFormLayout();
    hostEd = new QLineEdit(this); hostEd->setPlaceholderText("127.0.0.1");
    portEd = new QLineEdit(this); portEd->setText("27017");
    dbEd = new QLineEdit(this);   dbEd->setText("DegorasDB");
    userEd = new QLineEdit(this); userEd->setPlaceholderText("admin");
    passEd = new QLineEdit(this); passEd->setEchoMode(QLineEdit::Password);

    sslCheck = new QCheckBox("Enable SSL/TLS (OpenSSL)", this);
    sslCheck->setToolTip("Requires OpenSSL libraries installed.\nAdds '?tls=true' to connection string.");

    form->addRow("Host / IP:", hostEd);
    form->addRow("Port:", portEd);
    form->addRow("Database:", dbEd);
    form->addRow("Username:", userEd);
    form->addRow("Password:", passEd);
    form->addRow("", sslCheck);
    mainLayout->addLayout(form);

    // Standard Dialog Buttons (Connect / Cancel)
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText("Connect");
    mainLayout->addWidget(bb);

    // Signal Connections
    connect(bb, &QDialogButtonBox::accepted, this, &ConnectionDialog::on_connectBtn_clicked);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // QOverload is needed because currentIndexChanged has int and QString signatures
    connect(historyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConnectionDialog::on_historyCombo_currentIndexChanged);

    loadHistory();
}

ConnectionDialog::~ConnectionDialog() {}

ConnectionDialog::ConnectionParams ConnectionDialog::getParams() const { return m_params; }

void ConnectionDialog::on_connectBtn_clicked()
{
    m_params.host = hostEd->text().trimmed();
    m_params.port = portEd->text().toInt();
    m_params.dbName = dbEd->text().trimmed();
    m_params.user = userEd->text().trimmed();
    m_params.password = passEd->text();
    m_params.useSSL = sslCheck->isChecked();

    // Set defaults if empty
    if(m_params.host.isEmpty()) m_params.host = "localhost";
    if(m_params.port <= 0) m_params.port = 27017;

    if(m_params.dbName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Database name is required.");
        return;
    }

    saveHistory();
    accept();
}

void ConnectionDialog::loadHistory()
{
    QSettings settings("DegorasProject", "DBConnections");
    QStringList history = settings.value("historyList").toStringList();

    // Clear dynamic items but keep the first one ("New Connection...")
    while(historyCombo->count() > 1) historyCombo->removeItem(1);

    for(const QString& entry : history) {
        historyCombo->addItem(entry);
    }

    // Restore last used selection
    QString last = settings.value("lastUsed").toString();
    int idx = historyCombo->findText(last);
    if(idx != -1) historyCombo->setCurrentIndex(idx);
}

void ConnectionDialog::saveHistory()
{
    // Create a unique identifier string: user@host
    QString id = QString("%1@%2").arg(m_params.user.isEmpty() ? "anon" : m_params.user, m_params.host);

    QSettings settings("DegorasProject", "DBConnections");
    QStringList history = settings.value("historyList").toStringList();

    if(!history.contains(id)) {
        history.prepend(id);
        // Limit history size to keep UI clean
        if(history.size() > 5) history.removeLast();
        settings.setValue("historyList", history);
    }

    settings.setValue("lastUsed", id);
    settings.beginGroup("Details/" + id);
    settings.setValue("host", m_params.host);
    settings.setValue("port", m_params.port);
    settings.setValue("db", m_params.dbName);
    settings.setValue("user", m_params.user);
    settings.setValue("ssl", m_params.useSSL);
    settings.endGroup();
}

void ConnectionDialog::on_historyCombo_currentIndexChanged(int index)
{
    // Index 0 is "New Connection...", just ignore or could implement clear logic
    if (index <= 0) return;

    QString id = historyCombo->currentText();
    QSettings settings("DegorasProject", "DBConnections");

    settings.beginGroup("Details/" + id);
    hostEd->setText(settings.value("host").toString());
    portEd->setText(settings.value("port").toString());
    dbEd->setText(settings.value("db").toString());
    userEd->setText(settings.value("user").toString());
    sslCheck->setChecked(settings.value("ssl").toBool());
    passEd->clear(); // Never store/load passwords for security reasons
    settings.endGroup();
}
