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

    // 1. Selector de historial (Arriba)
    historyCombo = new QComboBox(this);
    historyCombo->addItem("New Connection...");
    mainLayout->addWidget(new QLabel("Saved Connections:"));
    mainLayout->addWidget(historyCombo);

    QFrame* line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // 2. Formulario
    QFormLayout* form = new QFormLayout();
    hostEd = new QLineEdit(this); hostEd->setPlaceholderText("127.0.0.1");
    portEd = new QLineEdit(this); portEd->setText("27017");
    dbEd = new QLineEdit(this);   dbEd->setText("DegorasDB");
    userEd = new QLineEdit(this); userEd->setPlaceholderText("admin");
    passEd = new QLineEdit(this); passEd->setEchoMode(QLineEdit::Password);

    // LO QUE PIDE TU JEFE: SSL
    sslCheck = new QCheckBox("Enable SSL/TLS (OpenSSL)", this);
    sslCheck->setToolTip("Requires OpenSSL libraries installed.\nAdds '?tls=true' to connection string.");

    form->addRow("Host / IP:", hostEd);
    form->addRow("Port:", portEd);
    form->addRow("Database:", dbEd);
    form->addRow("Username:", userEd);
    form->addRow("Password:", passEd);
    form->addRow("", sslCheck);
    mainLayout->addLayout(form);

    // 3. Botones
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText("Connect");
    mainLayout->addWidget(bb);

    connect(bb, &QDialogButtonBox::accepted, this, &ConnectionDialog::on_connectBtn_clicked);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
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

    if(m_params.host.isEmpty()) m_params.host = "localhost";
    if(m_params.port <= 0) m_params.port = 27017;
    if(m_params.dbName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Database name is required.");
        return;
    }

    saveHistory(); // Guardar para la próxima
    accept();
}

void ConnectionDialog::loadHistory()
{
    QSettings settings("DegorasProject", "DBConnections");
    QStringList history = settings.value("historyList").toStringList();

    while(historyCombo->count() > 1) historyCombo->removeItem(1);

    for(const QString& entry : history) {
        historyCombo->addItem(entry);
    }

    // Cargar última usada
    QString last = settings.value("lastUsed").toString();
    int idx = historyCombo->findText(last);
    if(idx != -1) historyCombo->setCurrentIndex(idx);
}

void ConnectionDialog::saveHistory()
{
    // ID único para el historial: user@host
    QString id = QString("%1@%2").arg(m_params.user.isEmpty() ? "anon" : m_params.user, m_params.host);

    QSettings settings("DegorasProject", "DBConnections");
    QStringList history = settings.value("historyList").toStringList();

    if(!history.contains(id)) {
        history.prepend(id);
        if(history.size() > 5) history.removeLast(); // Guardar solo las últimas 5
        settings.setValue("historyList", history);
    }

    settings.setValue("lastUsed", id);

    // Guardar detalles
    settings.beginGroup("Details/" + id);
    settings.setValue("host", m_params.host);
    settings.setValue("port", m_params.port);
    settings.setValue("db", m_params.dbName);
    settings.setValue("user", m_params.user);
    settings.setValue("ssl", m_params.useSSL);
    // NO guardamos contraseña por seguridad básica, o puedes guardarla si es entorno seguro
    settings.endGroup();
}

void ConnectionDialog::on_historyCombo_currentIndexChanged(int index)
{
    if (index <= 0) return; // "New connection..."

    QString id = historyCombo->currentText();
    QSettings settings("DegorasProject", "DBConnections");

    settings.beginGroup("Details/" + id);
    hostEd->setText(settings.value("host").toString());
    portEd->setText(settings.value("port").toString());
    dbEd->setText(settings.value("db").toString());
    userEd->setText(settings.value("user").toString());
    sslCheck->setChecked(settings.value("ssl").toBool());
    passEd->clear(); // Obligar a reintroducir password
    settings.endGroup();
}
