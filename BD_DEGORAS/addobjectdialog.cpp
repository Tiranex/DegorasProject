#include "addobjectdialog.h"
#include "ui_addobjectdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <iostream>

AddObjectDialog::AddObjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddObjectDialog)
{
    ui->setupUi(this);

    // Inicializar combos con valores por defecto
    if(ui->lrrCombo->count() == 0) ui->lrrCombo->addItems({"Unknown", "True", "False"});
    if(ui->debrisCombo->count() == 0) ui->debrisCombo->addItems({"Unknown", "True", "False"});

    // Habilitar selección múltiple para Grupos
    ui->setsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
}

AddObjectDialog::~AddObjectDialog()
{
    delete ui;
}


// Añade esto al principio o donde tengas las funciones
void AddObjectDialog::setDbManager(SpaceObjectDBManager* dbManager)
{
    m_dbManager = dbManager;
}


void AddObjectDialog::on_saveButton_clicked()
{
    QString errors;

    // Validar Obligatorios (!)
    if(ui->noradEdit->text().trimmed().isEmpty()) errors += "- Field 'Norad' is required.\n";
    if(ui->nameEdit->text().trimmed().isEmpty()) errors += "- Field 'Name' is required.\n";
    if(ui->aliasEdit->text().trimmed().isEmpty()) errors += "- Field 'Alias' is required.\n";

    // ¡CAMBIO AQUÍ! AÑADIMOS COSPAR A LA LISTA NEGRA DE VACÍOS
    if(ui->cosparEdit->text().trimmed().isEmpty()) errors += "- Field 'COSPAR' is required.\n";

    bool ok;
    ui->noradEdit->text().toLongLong(&ok);
    if(!ok) errors += "- 'Norad' must be a valid integer.\n";

    if(ui->altitudeSpin->value() <= 0) errors += "- Altitude must be greater than 0.\n";
    if(ui->npiSpin->value() <= 0) errors += "- NPI is required.\n";
    if(ui->bsSpin->value() <= 0) errors += "- BinSize (BS) is required.\n";

    if(!errors.isEmpty()) {
        QMessageBox::warning(this, "Invalid Data", "Please correct:\n\n" + errors);
        return;
    }

    if (!m_dbManager) {
        QMessageBox::critical(this, "Error", "No connection to the database.");
        return;
    }

    nlohmann::json newData = getNewObjectData();
    std::string errorDetails;
    bool success = false;
    // DECISIÓN CRÍTICA: ¿Creamos o Actualizamos?
    if (m_isEditMode) {
        // MODO EDICIÓN
        success = m_dbManager->updateSpaceObject(newData, m_selectedImagePath.toStdString(), errorDetails);
    } else {
        // MODO CREACIÓN
        success = m_dbManager->createSpaceObject(newData, m_selectedImagePath.toStdString(), errorDetails);
    }

    if (success) {
        QMessageBox::information(this, "Success", m_isEditMode ? "Object updated." : "Object created.");
        accept();
    } else {
        QMessageBox::critical(this, "Error", "Operation failed.\n\n" + QString::fromStdString(errorDetails));
    }
}

// Cargar grupos desde la BBDD
void AddObjectDialog::setAvailableGroups(const std::set<std::string> &groups)
{
    ui->setsListWidget->clear();
    for(const auto& groupName : groups) {
        ui->setsListWidget->addItem(QString::fromStdString(groupName));
    }

}


// Seleccionar imagen
void AddObjectDialog::on_browseImageBtn_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", QDir::homePath(), "Images (*.jpg *.png *.bmp *.jpeg)");

    if(!filePath.isEmpty()) {
        m_selectedImagePath = filePath;
        QFileInfo fi(filePath);
        ui->imagePathEdit->setText(fi.fileName());
    }
}

QString AddObjectDialog::getSelectedImagePath() const {
    return m_selectedImagePath;
}

// 1. AJUSTE EN LA GENERACIÓN DEL JSON
nlohmann::json AddObjectDialog::getNewObjectData() const
{
    nlohmann::json j;

    // Ayudas (Lambdas)
    auto setStringOrNull = [&](const std::string& key, const QString& value) {
        if (value.trimmed().isEmpty()) j[key] = nullptr;
        else j[key] = value.trimmed().toStdString();
    };

    auto setTristate = [&](const std::string& key, const QString& value) {
        if (value == "True") j[key] = 1;
        else if (value == "False") j[key] = 0;
        else j[key] = nullptr;
    };

    // --- ASIGNACIÓN ---

    // OBLIGATORIOS (Nunca Null)
    try {
        if (ui->noradEdit->text().isEmpty()) j["_id"] = nullptr;
        else j["_id"] = ui->noradEdit->text().toLongLong();
    } catch (...) { j["_id"] = nullptr; }

    // Strings OBLIGATORIOS (Asignación directa, sin setStringOrNull)
    j["NORAD"] = ui->noradEdit->text().trimmed().toStdString();
    j["Name"] = ui->nameEdit->text().trimmed().toStdString();
    j["Abbreviation"] = ui->aliasEdit->text().trimmed().toStdString();

    // ¡CAMBIO AQUÍ! COSPAR AHORA ES OBLIGATORIO
    j["COSPAR"] = ui->cosparEdit->text().trimmed().toStdString();

    // OPCIONALES (Pueden ser Null)
    setStringOrNull("ILRSID", ui->ilrsEdit->text());
    setStringOrNull("SIC", ui->sicEdit->text());

    // Booleanos
    setTristate("LaserRetroReflector", ui->lrrCombo->currentText());
    setTristate("IsDebris", ui->debrisCombo->currentText());
    j["TrackPolicy"] = ui->highPowerCheck->isChecked() ? 1 : 0;

    // Físicos
    j["Altitude"] = ui->altitudeSpin->value();
    j["RadarCrossSection"] = ui->rcsSpin->value();
    j["NormalPointIndicator"] = ui->npiSpin->value();
    j["BinSize"] = ui->bsSpin->value();
    j["Inclination"] = ui->incSpin->value();
    j["CoM"] = ui->comSpin->value();

    // Textos Opcionales
    setStringOrNull("Comments", ui->commentsEdit->toPlainText());
    setStringOrNull("ProviderCPF", ui->cpfEdit->text());
    setStringOrNull("Config", ui->configEdit->text());
    setStringOrNull("Picture", ui->imagePathEdit->text());

    // Grupos
    std::vector<std::string> selectedGroups;
    for(auto item : ui->setsListWidget->selectedItems()) {
        selectedGroups.push_back(item->text().toStdString());
    }
    j["Groups"] = selectedGroups;

    return j;
}

void AddObjectDialog::on_cancelButton_clicked()
{
    reject();
}

// --- NUEVA FUNCIÓN: CARGAR DATOS ---
void AddObjectDialog::loadObjectData(const nlohmann::json& obj)
{
    // Helper para no crashear con nulos
    auto getString = [&](const std::string& key) -> QString {
        if(obj.contains(key) && !obj[key].is_null())
            return QString::fromStdString(obj[key].get<std::string>());
        return "";
    };

    // Helper para números
    auto getDouble = [&](const std::string& key) -> double {
        if(obj.contains(key) && !obj[key].is_null()) return obj[key];
        return 0.0;
    };

    // 1. Rellenar Textos
    ui->noradEdit->setText(QString::number(obj.value("_id", 0LL))); // _id es int64
    ui->nameEdit->setText(getString("Name"));
    ui->aliasEdit->setText(getString("Abbreviation"));
    ui->cosparEdit->setText(getString("COSPAR"));
    ui->ilrsEdit->setText(getString("ILRSID"));
    ui->sicEdit->setText(getString("SIC"));

    ui->commentsEdit->setPlainText(getString("Comments"));
    ui->cpfEdit->setText(getString("ProviderCPF"));
    ui->configEdit->setText(getString("Config"));
    ui->imagePathEdit->setText(getString("Picture")); // Mostramos nombre de foto

    // 2. Rellenar Números
    ui->altitudeSpin->setValue(getDouble("Altitude"));
    ui->rcsSpin->setValue(getDouble("RadarCrossSection"));
    ui->npiSpin->setValue(obj.value("NormalPointIndicator", 0));
    ui->bsSpin->setValue(obj.value("BinSize", 0));
    ui->incSpin->setValue(getDouble("Inclination"));
    ui->comSpin->setValue(getDouble("CoM"));

    // 3. Rellenar Combos/Checks
    // LRR
    if(obj.contains("LaserRetroReflector")) {
        if(obj["LaserRetroReflector"].is_null()) ui->lrrCombo->setCurrentText("Unknown");
        else {
            // Asumimos que guardamos 1/0, convertimos a string "True"/"False" para el combo
            int val = obj["LaserRetroReflector"];
            ui->lrrCombo->setCurrentText(val == 1 ? "True" : "False");
        }
    }

    // Debris
    if(obj.contains("IsDebris")) {
        if(obj["IsDebris"].is_null()) ui->debrisCombo->setCurrentText("Unknown");
        else {
            int val = obj["IsDebris"];
            ui->debrisCombo->setCurrentText(val == 1 ? "True" : "False");
        }
    }

    // TrackPolicy
    if(obj.contains("TrackPolicy") && !obj["TrackPolicy"].is_null()) {
        ui->highPowerCheck->setChecked(obj["TrackPolicy"] == 1);
    }

    // 4. SELECCIONAR GRUPOS
    // Primero limpiamos selección
    ui->setsListWidget->clearSelection();

    if(obj.contains("Groups") && obj["Groups"].is_array()) {
        std::vector<std::string> groups = obj["Groups"];
        // Recorremos la lista visual y seleccionamos los que coincidan
        for(int i=0; i < ui->setsListWidget->count(); ++i) {
            QListWidgetItem* item = ui->setsListWidget->item(i);
            // Si el texto del item está en el vector groups del JSON
            for(const auto& g : groups) {
                if(item->text().toStdString() == g) {
                    item->setSelected(true);
                    break;
                }
            }
        }
    }
}

// --- NUEVA FUNCIÓN: MODO EDICIÓN ---
void AddObjectDialog::setEditMode(bool enable)
{
    m_isEditMode = enable;
    if(enable) {
        this->setWindowTitle("Edit Object");
        // ID / NORAD NO SE PUEDE TOCAR AL EDITAR (Es la clave primaria)
        ui->noradEdit->setEnabled(false);
        ui->saveButton->setText("Update Object");
    } else {
        this->setWindowTitle("Create New Object");
        ui->noradEdit->setEnabled(true);
        ui->saveButton->setText("Save Object");
    }
}
