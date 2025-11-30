#include "addobjectdialog.h"
#include "ui_addobjectdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <iostream>

// LOGGING INCLUDE
#include <spdlog/spdlog.h>

AddObjectDialog::AddObjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddObjectDialog)
{
    ui->setupUi(this);

    // Initialize combos
    if(ui->lrrCombo->count() == 0) ui->lrrCombo->addItems({"Unknown", "True", "False"});
    if(ui->debrisCombo->count() == 0) ui->debrisCombo->addItems({"Unknown", "True", "False"});

    // Enable multi-selection for BOTH lists
    ui->setsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->groupsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    spdlog::debug("AddObjectDialog initialized.");
}

AddObjectDialog::~AddObjectDialog()
{
    delete ui;
}

void AddObjectDialog::setDbManager(SpaceObjectDBManager* dbManager)
{
    m_dbManager = dbManager;
}

// --- LOAD SETS (Observation Lists) ---
void AddObjectDialog::setAvailableSets(const std::set<std::string> &sets)
{
    ui->setsListWidget->clear();
    for(const auto& s : sets) {
        ui->setsListWidget->addItem(QString::fromStdString(s));
    }
    ui->setsListWidget->clearSelection();
}

// --- LOAD GROUPS (Internal Categories) ---
void AddObjectDialog::setAvailableGroups(const std::set<std::string> &groups)
{
    ui->groupsListWidget->clear();
    for(const auto& g : groups) {
        ui->groupsListWidget->addItem(QString::fromStdString(g));
    }
    ui->groupsListWidget->clearSelection();
}

void AddObjectDialog::on_browseImageBtn_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", QDir::homePath(), "Images (*.jpg *.png *.bmp *.jpeg)");

    if(!filePath.isEmpty()) {
        m_selectedImagePath = filePath;
        QFileInfo fi(filePath);
        ui->imagePathEdit->setText(fi.fileName());
        spdlog::info("Image selected: {}", filePath.toStdString());
    }
}

QString AddObjectDialog::getSelectedImagePath() const {
    return m_selectedImagePath;
}

void AddObjectDialog::on_cancelButton_clicked()
{
    spdlog::debug("AddObjectDialog cancelled by user.");
    reject();
}

// 1. GENERAR JSON (GUARDAR)
nlohmann::json AddObjectDialog::getNewObjectData() const
{
    nlohmann::json j;

    // Helpers
    auto setStringOrNull = [&](const std::string& key, const QString& value) {
        if (value.trimmed().isEmpty()) j[key] = nullptr;
        else j[key] = value.trimmed().toStdString();
    };

    auto setTristate = [&](const std::string& key, const QString& value) {
        if (value == "True") j[key] = 1;
        else if (value == "False") j[key] = 0;
        else j[key] = nullptr;
    };

    // --- IDENTIFIERS (Required ! *) ---
    try {
        if (ui->noradEdit->text().isEmpty()) j["_id"] = nullptr;
        else j["_id"] = ui->noradEdit->text().toLongLong();
    } catch (...) { j["_id"] = nullptr; }

    j["NORAD"]  = ui->noradEdit->text().trimmed().toStdString();
    j["Name"]   = ui->nameEdit->text().trimmed().toStdString();
    j["COSPAR"] = ui->cosparEdit->text().trimmed().toStdString();

    // Alias (Antes Abbreviation)
    j["Alias"]  = ui->aliasEdit->text().trimmed().toStdString();

    // Classification (EL CAMPO QUE FALTABA)
    setStringOrNull("Classification", ui->classEdit->text());

    // --- OPTIONAL IDENTIFIERS (*) ---
    setStringOrNull("ILRSID", ui->ilrsEdit->text());
    setStringOrNull("SIC", ui->sicEdit->text());

    // --- BOOLEANS (!) ---
    // Has LRR / Is Debris
    setTristate("LaserRetroReflector", ui->lrrCombo->currentText());
    setTristate("IsDebris", ui->debrisCombo->currentText());

    // Track High Power (Checkbox -> 1/0)
    j["TrackHighPower"] = ui->highPowerCheck->isChecked() ? 1 : 0;

    // (Nota: Si tu BBDD antigua usa "TrackPolicy" con valores 0/1/2, avísame.
    // Aquí estamos guardando "TrackHighPower" como 0/1 según tu lista).

    // --- NUMERIC FIELDS (!) ---
    // Altitude, NPI, BS son obligatorios (validados en on_saveButton_clicked)
    j["Altitude"] = ui->altitudeEdit->text().toDouble();
    j["NormalPointIndicator"] = ui->npiEdit->text().toInt();
    j["BinSize"] = ui->bsEdit->text().toDouble();

    // Opcionales
    j["RadarCrossSection"] = ui->rcsEdit->text().toDouble();
    j["Inclination"] = ui->incEdit->text().toDouble();
    j["CoM"] = ui->comEdit->text().toDouble(); // Center of Mass

    // --- TEXTOS VARIOS ---
    setStringOrNull("Comments", ui->commentsEdit->toPlainText());
    setStringOrNull("ProviderCPF", ui->cpfEdit->text());
    setStringOrNull("Config", ui->configEdit->text()); // Configuraciones (9 chars)
    setStringOrNull("Picture", ui->imagePathEdit->text());

    // Default enablement (Si es nuevo, activado por defecto)
    if (!m_isEditMode) j["EnablementPolicy"] = 1;

    // --- ARRAYS ---

    // 1. SETS (Observation Sets)
    std::vector<std::string> selectedSets;
    for(auto item : ui->setsListWidget->selectedItems()) {
        selectedSets.push_back(item->text().toStdString());
    }
    j["Sets"] = selectedSets;

    // 2. GROUPS (Internal Categories)
    std::vector<std::string> selectedGroups;
    for(auto item : ui->groupsListWidget->selectedItems()) {
        selectedGroups.push_back(item->text().toStdString());
    }
    j["Groups"] = selectedGroups;

    return j;
}

// 2. CARGAR DATOS (EDITAR)
void AddObjectDialog::loadObjectData(const nlohmann::json& obj)
{
    spdlog::debug("Loading object data into form.");

    auto getString = [&](const std::string& key) -> QString {
        if(obj.contains(key) && !obj[key].is_null())
            return QString::fromStdString(obj[key].get<std::string>());
        return "";
    };

    auto getDouble = [&](const std::string& key) -> double {
        if(obj.contains(key) && !obj[key].is_null()) return obj[key];
        return 0.0;
    };

    // Textos
    ui->noradEdit->setText(QString::number(obj.value("_id", 0LL)));
    ui->nameEdit->setText(getString("Name"));
    ui->aliasEdit->setText(getString("Alias"));     // Cargamos "Alias"
    ui->cosparEdit->setText(getString("COSPAR"));
    ui->ilrsEdit->setText(getString("ILRSID"));
    ui->sicEdit->setText(getString("SIC"));

    // Classification (AÑADIDO)
    ui->classEdit->setText(getString("Classification"));

    ui->commentsEdit->setPlainText(getString("Comments"));
    ui->cpfEdit->setText(getString("ProviderCPF"));
    ui->configEdit->setText(getString("Config"));
    ui->imagePathEdit->setText(getString("Picture"));

    // Números
    ui->altitudeEdit->setText(QString::number(getDouble("Altitude")));
    ui->rcsEdit->setText(QString::number(getDouble("RadarCrossSection")));
    ui->npiEdit->setText(QString::number(obj.value("NormalPointIndicator", 0)));
    ui->bsEdit->setText(QString::number(obj.value("BinSize", 0)));
    ui->incEdit->setText(QString::number(getDouble("Inclination")));
    ui->comEdit->setText(QString::number(getDouble("CoM")));

    // Combos/Checks
    if(obj.contains("LaserRetroReflector")) {
        if(obj["LaserRetroReflector"].is_null()) ui->lrrCombo->setCurrentText("Unknown");
        else {
            int val = obj["LaserRetroReflector"];
            ui->lrrCombo->setCurrentText(val == 1 ? "True" : "False");
        }
    }

    if(obj.contains("IsDebris")) {
        if(obj["IsDebris"].is_null()) ui->debrisCombo->setCurrentText("Unknown");
        else {
            int val = obj["IsDebris"];
            ui->debrisCombo->setCurrentText(val == 1 ? "True" : "False");
        }
    }

    if(obj.contains("TrackHighPower") && !obj["TrackHighPower"].is_null()) {
        ui->highPowerCheck->setChecked(obj["TrackHighPower"] == 1);
    }

    // Listas (Sets y Groups)
    ui->setsListWidget->clearSelection();
    if(obj.contains("Sets") && obj["Sets"].is_array()) {
        std::vector<std::string> sets = obj["Sets"];
        for(int i=0; i < ui->setsListWidget->count(); ++i) {
            QListWidgetItem* item = ui->setsListWidget->item(i);
            for(const auto& s : sets) {
                if(item->text().toStdString() == s) { item->setSelected(true); break; }
            }
        }
    }

    ui->groupsListWidget->clearSelection();
    if(obj.contains("Groups") && obj["Groups"].is_array()) {
        std::vector<std::string> groups = obj["Groups"];
        for(int i=0; i < ui->groupsListWidget->count(); ++i) {
            QListWidgetItem* item = ui->groupsListWidget->item(i);
            for(const auto& g : groups) {
                if(item->text().toStdString() == g) { item->setSelected(true); break; }
            }
        }
    }
}

void AddObjectDialog::setEditMode(bool enable)
{
    m_isEditMode = enable;
    if(enable) {
        this->setWindowTitle("Edit Object");
        ui->noradEdit->setEnabled(false); // ID is immutable
        ui->saveButton->setText("Update Object");
        spdlog::debug("AddObjectDialog set to EDIT mode.");
    } else {
        this->setWindowTitle("Create New Object");
        ui->noradEdit->setEnabled(true);
        ui->saveButton->setText("Save Object");
        spdlog::debug("AddObjectDialog set to CREATE mode.");
    }
}

void AddObjectDialog::on_saveButton_clicked()
{
    QString errors;

    // --- VALIDATIONS ---
    if(ui->noradEdit->text().trimmed().isEmpty()) errors += "- Field 'Norad' is required.\n";
    if(ui->nameEdit->text().trimmed().isEmpty()) errors += "- Field 'Name' is required.\n";
    if(ui->aliasEdit->text().trimmed().isEmpty()) errors += "- Field 'Alias' is required.\n";
    if(ui->cosparEdit->text().trimmed().isEmpty()) errors += "- Field 'COSPAR' is required.\n";

    bool ok;
    ui->noradEdit->text().toLongLong(&ok);
    if(!ok) errors += "- 'Norad' must be a valid integer.\n";

    // Numeric checks (Optional: check if > 0)
    // if(ui->altitudeEdit->text().toDouble() <= 0) errors += "- Altitude > 0 required.\n";

    if(!errors.isEmpty()) {
        spdlog::warn("Validation failed: {}", errors.toStdString());
        QMessageBox::warning(this, "Invalid Data", "Please correct:\n\n" + errors);
        return;
    }

    if (!m_dbManager) {
        QMessageBox::critical(this, "Error", "No connection to database.");
        return;
    }

    // --- SAVE ---
    nlohmann::json newData = getNewObjectData();
    std::string errorDetails;
    bool success = false;

    if (m_isEditMode) {
        success = m_dbManager->updateSpaceObject(newData, m_selectedImagePath.toStdString(), errorDetails);
    } else {
        success = m_dbManager->createSpaceObject(newData, m_selectedImagePath.toStdString(), errorDetails);
    }

    if (success) {
        QMessageBox::information(this, "Success", m_isEditMode ? "Object updated." : "Object created.");
        accept();
    } else {
        spdlog::error("Save failed: {}", errorDetails);
        QMessageBox::critical(this, "Error", "Operation failed.\n\n" + QString::fromStdString(errorDetails));
    }
}
