#include "addobjectdialog.h"
#include "ui_addobjectdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QIntValidator>
#include <QDoubleValidator>


#include <spdlog/spdlog.h>

AddObjectDialog::AddObjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddObjectDialog)
{
    ui->setupUi(this);
    this->setStyleSheet(
        "QLineEdit[state='warning'] { border: 2px solid #FBC02D; }"
        "QLineEdit[state='error'] { border: 2px solid #D32F2F; }"
        "QLineEdit { border: 1px solid #606060; border-radius: 2px; }"
        "QLineEdit:read-only { color: #A0A0A0; border: 1px solid #404040; }"
        );

    connect(ui->noradEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);
    connect(ui->nameEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);
    connect(ui->aliasEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);
    connect(ui->cosparEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);
    connect(ui->altitudeEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);
    connect(ui->npiEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);
    connect(ui->bsEdit, &QLineEdit::textChanged, this, &AddObjectDialog::validateFormLive);

    connect(ui->selectDBImageBtn, &QPushButton::clicked, this, &AddObjectDialog::on_selectDBImageBtn_clicked);
    validateFormLive();
    ui->noradEdit->setValidator(new QIntValidator(0, 999999999, this));
    ui->npiEdit->setValidator(new QIntValidator(0, 999, this));

    ui->altitudeEdit->setValidator(new QDoubleValidator(0.0, 100000.0, 4, this));
    ui->rcsEdit->setValidator(new QDoubleValidator(0.0, 10000.0, 4, this));
    ui->bsEdit->setValidator(new QDoubleValidator(0.0, 10000.0, 4, this));
    ui->incEdit->setValidator(new QDoubleValidator(0.0, 360.0, 4, this));
    ui->comEdit->setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));

    if(ui->lrrCombo->count() == 0) ui->lrrCombo->addItems({"Unknown", "True", "False"});
    if(ui->debrisCombo->count() == 0) ui->debrisCombo->addItems({"Unknown", "True", "False"});

    ui->setsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->groupsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    spdlog::debug("AddObjectDialog initialized.");
}

AddObjectDialog::~AddObjectDialog()
{
    delete ui;
}

void AddObjectDialog::setFieldState(QWidget* widget, const QString& state)
{
    widget->setProperty("state", state);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

void AddObjectDialog::setDbManager(SpaceObjectDBManager* dbManager)
{
    m_dbManager = dbManager;
}

void AddObjectDialog::on_saveButton_clicked()
{
    QString errors;
    setFieldState(ui->noradEdit, "");
    setFieldState(ui->nameEdit, "");
    setFieldState(ui->aliasEdit, "");
    setFieldState(ui->cosparEdit, "");
    setFieldState(ui->ilrsEdit, "");
    setFieldState(ui->sicEdit, "");
    setFieldState(ui->altitudeEdit, "");
    setFieldState(ui->npiEdit, "");
    setFieldState(ui->bsEdit, "");
    int64_t currentId = -1;
    if (!ui->noradEdit->text().isEmpty()) currentId = ui->noradEdit->text().toLongLong();

    auto checkDuplicate = [&](const std::string& fieldName, const QString& uiValue, QLineEdit* widget) -> bool {
        if (uiValue.trimmed().isEmpty()) return false;
        if (!m_existingObjects) return false;

        std::string valToCheck = uiValue.trimmed().toStdString();

        for (const auto& obj : *m_existingObjects) {
            if (m_isEditMode) {
                if (obj.contains("_id") && obj["_id"] == currentId) continue;
            }
            if (obj.contains(fieldName) && !obj[fieldName].is_null()) {
                std::string objVal;
                if (obj[fieldName].is_string()) objVal = obj[fieldName].get<std::string>();
                else if (obj[fieldName].is_number()) objVal = std::to_string(obj[fieldName].get<int64_t>());

                if (objVal == valToCheck) {
                    setFieldState(widget, "error");
                    return true;
                }
            }
        }
        return false;
    };

    if (ui->noradEdit->text().isEmpty()) {
        errors += "- Field 'Norad' is required.\n";
        setFieldState(ui->noradEdit, "error");
    }
    else if (!m_isEditMode && checkDuplicate("_id", ui->noradEdit->text(), ui->noradEdit)) {
        errors += "- NORAD ID already exists in memory.\n";
    }
    if (ui->nameEdit->text().isEmpty()) {
        errors += "- Field 'Name' is required.\n";
        setFieldState(ui->nameEdit, "error");
    } else if (checkDuplicate("Name", ui->nameEdit->text(), ui->nameEdit)) {
        errors += "- Name already exists in memory.\n";
    }
    if (ui->aliasEdit->text().isEmpty()) {
        errors += "- Field 'Alias' is required.\n";
        setFieldState(ui->aliasEdit, "error");
    } else if (checkDuplicate("Alias", ui->aliasEdit->text(), ui->aliasEdit)) {
        errors += "- Alias already exists in memory.\n";
    }
    if (ui->cosparEdit->text().isEmpty()) {
        errors += "- Field 'COSPAR' is required.\n";
        setFieldState(ui->cosparEdit, "error");
    } else if (checkDuplicate("COSPAR", ui->cosparEdit->text(), ui->cosparEdit)) {
        errors += "- COSPAR already exists in memory.\n";
    }
    if (checkDuplicate("ILRSID", ui->ilrsEdit->text(), ui->ilrsEdit)) {
        errors += "- ILRS ID already exists in memory.\n";
    }
    if (checkDuplicate("SIC", ui->sicEdit->text(), ui->sicEdit)) {
        errors += "- SIC already exists in memory.\n";
    }
    if(ui->altitudeEdit->text().toDouble() <= 0) {
        errors += "- Altitude > 0 required.\n";
        setFieldState(ui->altitudeEdit, "error");
    }
    if(ui->npiEdit->text().toInt() <= 0) {
        errors += "- NPI > 0 required.\n";
        setFieldState(ui->npiEdit, "error");
    }
    if(ui->bsEdit->text().toDouble() <= 0) {
        errors += "- BinSize > 0 required.\n";
        setFieldState(ui->bsEdit, "error");
    }
    if(!errors.isEmpty()) {
        spdlog::warn("Validation failed in Dialog: {}", errors.toStdString());
        QMessageBox::warning(this, "Invalid Data", "Please correct the fields marked in RED:\n\n" + errors);
        return;
    }
    spdlog::info("Object data validated successfully.");
    accept();
}

void AddObjectDialog::setAvailableSets(const std::vector<std::string> &sets)
{
    ui->setsListWidget->clear();
    for(const auto& s : sets) ui->setsListWidget->addItem(QString::fromStdString(s));
    ui->setsListWidget->clearSelection();
}

void AddObjectDialog::setAvailableGroups(const std::vector<std::string> &groups)
{
    ui->groupsListWidget->clear();
    for(const auto& g : groups) ui->groupsListWidget->addItem(QString::fromStdString(g));
    ui->groupsListWidget->clearSelection();
}

void AddObjectDialog::on_browseImageBtn_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", QDir::homePath(), "Images (*.jpg *.png *.bmp *.jpeg)");

    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    QString fileName = fi.fileName();
    std::string nameStd = fileName.toStdString();
    if (m_dbManager && m_dbManager->getImageManager().exists(nameStd)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Image Exists",
                                      "The image '" + fileName + "' already exists in the database.\n\n"
                                                                 "Do you want to use the existing image? (No upload will be performed)",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            return;
        }
        else {
            m_selectedImagePath = "";
            ui->imagePathEdit->setText(fileName);
            return;
        }
    }
    m_selectedImagePath = filePath;
    ui->imagePathEdit->setText(fileName);
}

void AddObjectDialog::on_selectDBImageBtn_clicked()
{
    if (!m_dbManager) return;

    std::vector<std::string> images = m_dbManager->getImageManager().getAllImageNames();
    if (images.empty()) {
        QMessageBox::information(this, "Info", "No images found in the database.");
        return;
    }
    std::sort(images.begin(), images.end());

    QDialog dlg(this);
    dlg.setWindowTitle("Select Image from DB");
    dlg.setMinimumSize(400, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel("Filter images:", &dlg));
    QLineEdit *searchBox = new QLineEdit(&dlg);
    searchBox->setPlaceholderText("Type to search...");
    layout->addWidget(searchBox);

    QListWidget *listWidget = new QListWidget(&dlg);
    for(const auto& img : images) {
        listWidget->addItem(QString::fromStdString(img));
    }
    layout->addWidget(listWidget);

    QObject::connect(searchBox, &QLineEdit::textChanged, listWidget, [listWidget](const QString &text){
        for(int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem *item = listWidget->item(i);
            bool match = item->text().contains(text, Qt::CaseInsensitive);
            item->setHidden(!match);
        }
    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        if (!listWidget->selectedItems().isEmpty()) {
            QString selectedName = listWidget->selectedItems().first()->text();
            ui->imagePathEdit->setText(selectedName);
            m_selectedImagePath.clear(); // Reutilización
        }
    }
}

QString AddObjectDialog::getSelectedImagePath() const { return m_selectedImagePath; }

void AddObjectDialog::on_cancelButton_clicked() { reject(); }

nlohmann::json AddObjectDialog::getNewObjectData() const
{
    nlohmann::json j;

    auto setStringOrNull = [&](const std::string& key, const QString& value) {
        if (value.trimmed().isEmpty()) j[key] = nullptr;
        else j[key] = value.trimmed().toStdString();
    };
    auto setTristate = [&](const std::string& key, const QString& value) {
        if (value == "True") j[key] = 1; else if (value == "False") j[key] = 0; else j[key] = nullptr;
    };

    try {
        if (ui->noradEdit->text().isEmpty()) j["_id"] = nullptr;
        else j["_id"] = ui->noradEdit->text().toLongLong();
    } catch (...) { j["_id"] = nullptr; }

    j["NORAD"] = ui->noradEdit->text().trimmed().toStdString();
    j["Name"] = ui->nameEdit->text().trimmed().toStdString();
    j["Alias"] = ui->aliasEdit->text().trimmed().toStdString();
    j["COSPAR"] = ui->cosparEdit->text().trimmed().toStdString();

    setStringOrNull("Classification", ui->classEdit->text());
    setStringOrNull("ILRSID", ui->ilrsEdit->text());
    setStringOrNull("SIC", ui->sicEdit->text());

    setTristate("LaserRetroReflector", ui->lrrCombo->currentText());
    setTristate("IsDebris", ui->debrisCombo->currentText());
    j["TrackHighPower"] = ui->highPowerCheck->isChecked() ? 1 : 0;

    j["Altitude"] = ui->altitudeEdit->text().toDouble();
    j["RadarCrossSection"] = ui->rcsEdit->text().toDouble();
    j["NormalPointIndicator"] = ui->npiEdit->text().toInt();
    j["BinSize"] = ui->bsEdit->text().toDouble();
    j["Inclination"] = ui->incEdit->text().toDouble();
    j["CoM"] = ui->comEdit->text().toDouble();

    setStringOrNull("Comments", ui->commentsEdit->toPlainText());
    setStringOrNull("ProviderCPF", ui->cpfEdit->text());
    setStringOrNull("Config", ui->configEdit->text());
    setStringOrNull("Picture", ui->imagePathEdit->text());

    std::vector<std::string> selectedSets;
    for(auto item : ui->setsListWidget->selectedItems()) {
        selectedSets.push_back(item->text().toStdString());
    }
    j["Sets"] = selectedSets;
    std::vector<std::string> selectedGroups;
    for(auto item : ui->groupsListWidget->selectedItems()) {
        selectedGroups.push_back(item->text().toStdString());
    }
    j["Groups"] = selectedGroups;
    j["EnablementPolicy"] = ui->policyCombo->currentIndex();
    return j;
}

void AddObjectDialog::loadObjectData(const nlohmann::json& obj)
{
    auto getString = [&](const std::string& key) -> QString {
        if(obj.contains(key) && !obj[key].is_null()) return QString::fromStdString(obj[key].get<std::string>());
        return "";
    };
    auto getDouble = [&](const std::string& key) -> double {
        if(obj.contains(key) && !obj[key].is_null()) return obj[key];
        return 0.0;
    };

    if (ui->noradEdit->text().isEmpty()) {
        ui->noradEdit->setText(QString::number(obj.value("_id", 0LL)));
    }
    ui->nameEdit->setText(getString("Name"));
    ui->aliasEdit->setText(getString("Alias"));
    ui->cosparEdit->setText(getString("COSPAR"));
    ui->ilrsEdit->setText(getString("ILRSID"));
    ui->sicEdit->setText(getString("SIC"));
    ui->classEdit->setText(getString("Classification"));

    ui->commentsEdit->setPlainText(getString("Comments"));
    ui->cpfEdit->setText(getString("ProviderCPF"));
    ui->configEdit->setText(getString("Config"));
    ui->imagePathEdit->setText(getString("Picture"));

    ui->altitudeEdit->setText(QString::number(getDouble("Altitude")));
    ui->rcsEdit->setText(QString::number(getDouble("RadarCrossSection")));
    ui->npiEdit->setText(QString::number(obj.value("NormalPointIndicator", 0)));
    ui->bsEdit->setText(QString::number(obj.value("BinSize", 0)));
    ui->incEdit->setText(QString::number(getDouble("Inclination")));
    ui->comEdit->setText(QString::number(getDouble("CoM")));

    if(obj.contains("LaserRetroReflector")) {
        if(obj["LaserRetroReflector"].is_null()) ui->lrrCombo->setCurrentText("Unknown");
        else ui->lrrCombo->setCurrentText(obj["LaserRetroReflector"] == 1 ? "True" : "False");
    }
    if(obj.contains("IsDebris")) {
        if(obj["IsDebris"].is_null()) ui->debrisCombo->setCurrentText("Unknown");
        else ui->debrisCombo->setCurrentText(obj["IsDebris"] == 1 ? "True" : "False");
    }
    if(obj.contains("TrackHighPower") && !obj["TrackHighPower"].is_null()) {
        ui->highPowerCheck->setChecked(obj["TrackHighPower"] == 1);
    }
    if (obj.contains("EnablementPolicy") && obj["EnablementPolicy"].is_number()) {
        int val = obj["EnablementPolicy"].get<int>();
        if (val >= 0 && val < ui->policyCombo->count()) {
            ui->policyCombo->setCurrentIndex(val);
        }
    }
    ui->setsListWidget->clearSelection();
    if(obj.contains("Sets") && obj["Sets"].is_array()) {
        std::vector<std::string> sets = obj["Sets"];
        for(int i=0; i < ui->setsListWidget->count(); ++i) {
            for(const auto& s : sets) {
                if(ui->setsListWidget->item(i)->text().toStdString() == s) {
                    ui->setsListWidget->item(i)->setSelected(true);
                }
            }
        }
    }

    ui->groupsListWidget->clearSelection();
    if(obj.contains("Groups") && obj["Groups"].is_array()) {
        std::vector<std::string> groups = obj["Groups"];
        for(int i=0; i < ui->groupsListWidget->count(); ++i) {
            for(const auto& g : groups) {
                if(ui->groupsListWidget->item(i)->text().toStdString() == g) {
                    ui->groupsListWidget->item(i)->setSelected(true);
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
    } else {
        this->setWindowTitle("Create New Object");
        ui->noradEdit->setEnabled(true);
        ui->saveButton->setText("Save Object");
    }
}

void AddObjectDialog::validateFormLive()
{
    auto checkRequired = [&](QLineEdit* widget) {
        if (widget->text().trimmed().isEmpty()) setFieldState(widget, "warning");
        else setFieldState(widget, "");
    };

    auto checkNumeric = [&](QLineEdit* widget) {
        if (widget->text().isEmpty() || widget->text().toDouble() <= 0) setFieldState(widget, "warning");
        else setFieldState(widget, "");
    };

    checkRequired(ui->noradEdit);
    checkRequired(ui->nameEdit);
    checkRequired(ui->aliasEdit);
    checkRequired(ui->cosparEdit);
    checkNumeric(ui->altitudeEdit);
    checkNumeric(ui->npiEdit);
    checkNumeric(ui->bsEdit);
}

void AddObjectDialog::setExistingObjects(const std::vector<nlohmann::json>* objects) {
    m_existingObjects = objects;
}

void AddObjectDialog::on_searchPluginBtn_clicked()
{
    QString noradStr = ui->noradEdit->text().trimmed();
    if (noradStr.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a NORAD ID first.");
        return;
    }

    int64_t noradId = noradStr.toLongLong();
    this->setCursor(Qt::WaitCursor);
    ui->searchPluginBtn->setEnabled(false);
    nlohmann::json result = PluginManager::instance().searchSpaceObject(noradId);
    this->setCursor(Qt::ArrowCursor);
    ui->searchPluginBtn->setEnabled(true);
    if (result.empty()) {
        QMessageBox::information(this, "Not Found",
                                 "Object not found in any loaded plugin.\n\n"
                                 "Check that you have the DLLs in the 'plugins' folder.");
    } else {
        loadObjectData(result);
        validateFormLive();
        spdlog::info("Form auto-filled via plugin for NORAD: {}", noradId);
        QMessageBox::information(this, "Success", "Object data loaded from plugin!");
    }
}
