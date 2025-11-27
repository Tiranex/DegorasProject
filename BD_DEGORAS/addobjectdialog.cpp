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

    // Initialize combos with default values
    if(ui->lrrCombo->count() == 0) ui->lrrCombo->addItems({"Unknown", "True", "False"});
    if(ui->debrisCombo->count() == 0) ui->debrisCombo->addItems({"Unknown", "True", "False"});

    // Enable multi-selection for Groups
    ui->setsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

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

void AddObjectDialog::on_saveButton_clicked()
{
    QString errors;

    // Validate Required Fields
    if(ui->noradEdit->text().trimmed().isEmpty()) errors += "- Field 'Norad' is required.\n";
    if(ui->nameEdit->text().trimmed().isEmpty()) errors += "- Field 'Name' is required.\n";
    if(ui->aliasEdit->text().trimmed().isEmpty()) errors += "- Field 'Alias' is required.\n";
    if(ui->cosparEdit->text().trimmed().isEmpty()) errors += "- Field 'COSPAR' is required.\n";

    bool ok;
    ui->noradEdit->text().toLongLong(&ok);
    if(!ok) errors += "- 'Norad' must be a valid integer.\n";

    // --- FIX: Convert text to numbers for validation ---
    if(ui->altitudeEdit->text().toDouble() <= 0) errors += "- Altitude greater than 0 is required.\n";
    if(ui->npiEdit->text().toInt() <= 0) errors += "- NPI greater than 0 is required.\n";
    if(ui->bsEdit->text().toDouble() <= 0) errors += "- BinSize (BS) greater than 0 is required.\n";

    if(!errors.isEmpty()) {
        spdlog::warn("AddObjectDialog validation failed:\n{}", errors.toStdString());
        QMessageBox::warning(this, "Invalid Data", "Please correct:\n\n" + errors);
        return;
    }

    if (!m_dbManager) {
        spdlog::error("AddObjectDialog: No database connection available.");
        QMessageBox::critical(this, "Error", "No connection to the database.");
        return;
    }

    nlohmann::json newData = getNewObjectData();
    std::string errorDetails;
    bool success = false;

    // Create or Update?
    if (m_isEditMode) {
        spdlog::info("Attempting to update object with ID: {}", newData["_id"].dump());
        success = m_dbManager->updateSpaceObject(newData, m_selectedImagePath.toStdString(), errorDetails);
    } else {
        spdlog::info("Attempting to create new object with ID: {}", newData["_id"].dump());
        success = m_dbManager->createSpaceObject(newData, m_selectedImagePath.toStdString(), errorDetails);
    }

    if (success) {
        spdlog::info("Operation successful. Object saved.");
        QMessageBox::information(this, "Success", m_isEditMode ? "Object updated." : "Object created.");
        accept();
    } else {
        spdlog::error("Operation failed. Reason: {}", errorDetails);
        QMessageBox::critical(this, "Error", "Operation failed.\n\n" + QString::fromStdString(errorDetails));
    }
}

void AddObjectDialog::setAvailableGroups(const std::set<std::string> &groups)
{
    ui->setsListWidget->clear();
    for(const auto& groupName : groups) {
        ui->setsListWidget->addItem(QString::fromStdString(groupName));
    }
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

    // --- ASSIGNMENT ---

    // Required (Never Null)
    try {
        if (ui->noradEdit->text().isEmpty()) j["_id"] = nullptr;
        else j["_id"] = ui->noradEdit->text().toLongLong();
    } catch (...) { j["_id"] = nullptr; }

    j["NORAD"] = ui->noradEdit->text().trimmed().toStdString();
    j["Name"] = ui->nameEdit->text().trimmed().toStdString();
    j["Abbreviation"] = ui->aliasEdit->text().trimmed().toStdString();
    j["COSPAR"] = ui->cosparEdit->text().trimmed().toStdString();
    j["EnablementPolicy"] = 1;

    // Optional Strings
    setStringOrNull("ILRSID", ui->ilrsEdit->text());
    setStringOrNull("SIC", ui->sicEdit->text());

    // Booleans
    setTristate("LaserRetroReflector", ui->lrrCombo->currentText());
    setTristate("IsDebris", ui->debrisCombo->currentText());
    j["TrackPolicy"] = ui->highPowerCheck->isChecked() ? 1 : 0;

    // --- FIX: Convert QLineEdit text to numbers ---
    // toDouble() returns 0.0 if conversion fails, which is handled by validation
    j["Altitude"] = ui->altitudeEdit->text().toDouble();
    j["RadarCrossSection"] = ui->rcsEdit->text().toDouble();
    j["NormalPointIndicator"] = ui->npiEdit->text().toInt();
    j["BinSize"] = ui->bsEdit->text().toDouble();
    j["Inclination"] = ui->incEdit->text().toDouble();
    j["CoM"] = ui->comEdit->text().toDouble();

    // Optional Texts
    setStringOrNull("Comments", ui->commentsEdit->toPlainText());
    setStringOrNull("ProviderCPF", ui->cpfEdit->text());
    setStringOrNull("Config", ui->configEdit->text());
    setStringOrNull("Picture", ui->imagePathEdit->text());

    // Groups
    std::vector<std::string> selectedGroups;
    for(auto item : ui->setsListWidget->selectedItems()) {
        selectedGroups.push_back(item->text().toStdString());
    }
    j["Groups"] = selectedGroups;

    return j;
}

void AddObjectDialog::on_cancelButton_clicked()
{
    spdlog::debug("AddObjectDialog cancelled by user.");
    reject();
}

// --- LOAD DATA ---
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

    // 1. Fill Texts
    ui->noradEdit->setText(QString::number(obj.value("_id", 0LL)));
    ui->nameEdit->setText(getString("Name"));
    ui->aliasEdit->setText(getString("Abbreviation"));
    ui->cosparEdit->setText(getString("COSPAR"));
    ui->ilrsEdit->setText(getString("ILRSID"));
    ui->sicEdit->setText(getString("SIC"));

    ui->commentsEdit->setPlainText(getString("Comments"));
    ui->cpfEdit->setText(getString("ProviderCPF"));
    ui->configEdit->setText(getString("Config"));
    ui->imagePathEdit->setText(getString("Picture"));

    // --- FIX: Convert Numbers to String for QLineEdit ---
    ui->altitudeEdit->setText(QString::number(getDouble("Altitude")));
    ui->rcsEdit->setText(QString::number(getDouble("RadarCrossSection")));
    ui->npiEdit->setText(QString::number(obj.value("NormalPointIndicator", 0)));
    ui->bsEdit->setText(QString::number(obj.value("BinSize", 0)));
    ui->incEdit->setText(QString::number(getDouble("Inclination")));
    ui->comEdit->setText(QString::number(getDouble("CoM")));

    // 3. Fill Combos/Checks
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

    if(obj.contains("TrackPolicy") && !obj["TrackPolicy"].is_null()) {
        ui->highPowerCheck->setChecked(obj["TrackPolicy"] == 1);
    }

    // 4. Select Groups
    ui->setsListWidget->clearSelection();
    if(obj.contains("Groups") && obj["Groups"].is_array()) {
        std::vector<std::string> groups = obj["Groups"];
        for(int i=0; i < ui->setsListWidget->count(); ++i) {
            QListWidgetItem* item = ui->setsListWidget->item(i);
            for(const auto& g : groups) {
                if(item->text().toStdString() == g) {
                    item->setSelected(true);
                    break;
                }
            }
        }
    }
}

void AddObjectDialog::setEditMode(bool enable)
{
    m_isEditMode = enable;
    if(enable) {
        this->setWindowTitle("Edit Object");
        ui->noradEdit->setEnabled(false);
        ui->saveButton->setText("Update Object");
        spdlog::debug("AddObjectDialog set to EDIT mode.");
    } else {
        this->setWindowTitle("Create New Object");
        ui->noradEdit->setEnabled(true);
        ui->saveButton->setText("Save Object");
        spdlog::debug("AddObjectDialog set to CREATE mode.");
    }
}
