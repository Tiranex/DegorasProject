#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SpaceObjectDBManager.h"
#include "json_helpers.h"
#include "addobjectdialog.h" // Required for full class definition

// QT INCLUDES
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QPixmap>
#include <QByteArray>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QInputDialog>
#include <QGuiApplication>
#include <QClipboard>
#include <QMenu>
#include <QDateTime>

// LOGGING
#include <spdlog/spdlog.h>

// STD C++ INCLUDES
#include <string>
#include <memory>
#include <vector>
#include <set>

// Table headers based on DB keys. DO NOT TRANSLATE literal strings.
const QStringList g_tableHeaders = {
    "_id", "Name", "Altitude", "Amplification", "BinSize",
    "COSPAR", "Classification", "CounterID", "DetectorID",
    "EnablementPolicy", "ILRSID", "ILRSName", "Inclination",
    "IsDebris", "LaserID", "LaserRetroReflector", "NORAD","Abbreviation" ,
    "NormalPointIndicator", "Picture", "Priority", "ProviderCPF",
    "RadarCrossSection", "SIC", "TrackPolicy"
};

// --- CONSTRUCTOR ---
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Configure tables
    setupObjectListTable();
    setupSetsObjectTable();

    // 2. Connections (Signals & Slots)
    connect(ui->setsObjectTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::on_editObjectButton_clicked);
    connect(ui->setsObjectTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onSetsTableContextMenuRequested);

    // Connections for non-modal windows

    // Other UI connections
    connect(ui->setsObjectTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::on_setsObjectTable_selectionChanged);
    connect(ui->showAllObjectsCheckBox, &QCheckBox::toggled, this, &MainWindow::on_showAllObjectsCheckBox_toggled);
    connect(ui->listWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::on_listWidget_itemSelectionChanged);
    connect(ui->objectListTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onObjectListTableContextMenuRequested);

    // --- 3. Connect to DB ---
    const std::string URI = "mongodb://localhost:27017";
    const std::string DB_NAME = "DegorasDB";
    const std::string COLLECTION_NAME = "space_objects";

    try {
        dbManager = std::make_unique<SpaceObjectDBManager>(URI, DB_NAME, COLLECTION_NAME);
        logMessage("[Info] Successfully connected to " + QString::fromStdString(DB_NAME) + "/" + QString::fromStdString(COLLECTION_NAME));
    }
    catch (const std::exception& e)
    {
        QString errorMsg = "[Fatal Error] Could not connect to MongoDB: " + QString(e.what());
        logMessage(errorMsg);

        // Disable UI if DB fails
        ui->mostrarButton->setEnabled(false);
        ui->eliminarButton->setEnabled(false);
        ui->anadirButton->setEnabled(false);
        ui->browseButton->setEnabled(false);
        ui->idLineEdit->setEnabled(false);
        ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tab_3), false);
        ui->addNewObjectSetButton->setEnabled(false);
    }

    if (dbManager) {
        refreshGroupList();
    }

    // 4. Initial UI Setup
    ui->searchByIdRadioButton->setChecked(true);
    ui->imageDisplayLabel->clear();

    // Load initial list
    on_refreshListButton_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
    // Unique pointers clean up memory automatically
}

// --- LOGGING HELPER ---
void MainWindow::logMessage(const QString& msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss.zzz");
    QString formattedMsg = QString("[%1] %2").arg(timestamp, msg);

    ui->resultsTextEdit->append(formattedMsg);
    ui->resultsTextEdit->moveCursor(QTextCursor::End);

    if (msg.contains("[Error]") || msg.contains("[Fatal]")) {
        spdlog::error(msg.toStdString());
    }
    else if (msg.contains("[Warn]") || msg.contains("[Warning]")) {
        spdlog::warn(msg.toStdString());
    }
    else {
        spdlog::info(msg.toStdString());
    }
}

void MainWindow::setupObjectListTable()
{
    ui->objectListTable->setColumnCount(g_tableHeaders.size());
    ui->objectListTable->setHorizontalHeaderLabels(g_tableHeaders);
    ui->objectListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->objectListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->objectListTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

// =================================================================
// --- SLOTS (Tabs 1 & 2) ---
// =================================================================

void MainWindow::on_mostrarButton_clicked()
{
    if (!dbManager) return;

    ui->resultsTextEdit->clear();
    ui->imageDisplayLabel->clear();
    ui->mostrar_texto_1->clear();
    ui->mostrar_texto_2->clear();
    ui->mostrar_texto_3->clear();
    ui->mostrar_texto_4->clear();
    ui->mostrar_texto_5->clear();
    ui->mostrar_texto_6->clear();

    logMessage("[Info] Searching...");

    std::string searchText = ui->idLineEdit->text().toStdString();
    nlohmann::json obj;
    bool searchedById = ui->searchByIdRadioButton->isChecked();

    try
    {
        if (searchedById)
        {
            if (searchText.empty()) {
                logMessage("[Error] The _id cannot be empty.");
                return;
            }
            int64_t id = std::stoll(searchText);
            obj = dbManager->getSpaceObjectById(id);
        }
        else
        {
            if (searchText.empty()) {
                logMessage("[Error] The Name cannot be empty.");
                return;
            }
            obj = dbManager->getSpaceObjectByName(searchText);
        }
    }
    catch (const std::exception& e) {
        logMessage("[Error] Exception during search: " + QString(e.what()));
        return;
    }

    if (obj.empty() || obj.is_null()) {
        logMessage("[Info] Object not found.");
        ui->mostrar_texto_1->setText("Status: Not found");
        return;
    }

    logMessage("[OK] Object found.");

    // --- Update UI ---
    if (searchedById)
    {
        if (obj.contains("Name")) ui->mostrar_texto_1->setText("Name: " + jsonValueToQString(obj["Name"]));
        else ui->mostrar_texto_1->setText("Name: Unknown");
    }
    else
    {
        if (obj.contains("_id")) ui->mostrar_texto_1->setText("ID: " + jsonValueToQString(obj["_id"]));
        else ui->mostrar_texto_1->setText("ID: Error");
    }

    if (obj.contains("Altitude")) ui->mostrar_texto_2->setText("Altitude: " + jsonValueToQString(obj["Altitude"]) + " km");
    else ui->mostrar_texto_2->setText("Altitude: -");

    if (obj.contains("Inclination")) ui->mostrar_texto_3->setText("Inclination: " + jsonValueToQString(obj["Inclination"]) + "°");
    else ui->mostrar_texto_3->setText("Inclination: -");

    if (obj.contains("LaserID")) ui->mostrar_texto_4->setText("Laser ID: " + jsonValueToQString(obj["LaserID"]));
    else ui->mostrar_texto_4->setText("Laser ID: N/A");

    if (obj.contains("LaserRetroReflector")) ui->mostrar_texto_5->setText("RetroReflector: " + jsonValueToQString(obj["LaserRetroReflector"]));
    else ui->mostrar_texto_5->setText("RetroReflector: -");

    if (obj.contains("RadarCrossSection")) ui->mostrar_texto_6->setText("RCS: " + jsonValueToQString(obj["RadarCrossSection"]) + " m²");
    else ui->mostrar_texto_6->setText("RCS: -");

    // --- Image Handling ---
    std::string picName = "";
    if (obj.contains("Picture")) {
        picName = obj["Picture"].get<std::string>();
    }

    if (picName.empty() || picName == "\"\"") {
        logMessage("[Info] This object has no assigned image (No photo).");
        ui->imageDisplayLabel->setText("NO PHOTO");
        return;
    }

    std::string imageDataStd = dbManager->getImageManager().downloadImageByName(picName);

    if (imageDataStd.empty()) {
        logMessage("[Warning] The object requests photo '" + QString::fromStdString(picName) + "', but it was not found in GridFS.");
        ui->imageDisplayLabel->setText("File not found");
    }
    else {
        QByteArray imageDataQt(imageDataStd.data(), imageDataStd.length());
        QPixmap pixmap;

        if (pixmap.loadFromData(imageDataQt)) {
            ui->imageDisplayLabel->setPixmap(pixmap.scaled(
                ui->imageDisplayLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                ));
            logMessage("[Info] Image loaded: " + QString::fromStdString(picName));
        } else {
            logMessage("[Error] Data downloaded, but it is not a valid image.");
        }
    }
}

void MainWindow::on_anadirButton_clicked()
{
    if (!dbManager) return;

    std::string id_str = ui->idLineEdit_add->text().toStdString();
    std::string norad_str = ui->noradLineEdit_add->text().toStdString();
    std::string name_str = ui->nameLineEdit_add->text().toStdString();
    std::string class_str = ui->classLineEdit_add->text().toStdString();
    std::string pic_str = ui->pictureLineEdit_add->text().toStdString();

    int64_t id;
    if (id_str.empty()) {
        logMessage("[Error] The _id is required to create an object.");
        return;
    }
    try {
        id = std::stoll(id_str);
    } catch (...) {
        logMessage("[Error] The entered _id is not a valid number.");
        return;
    }
    if (name_str.empty()) {
        logMessage("[Error] The name is required.");
        return;
    }
    if (norad_str.empty()) {
        norad_str = id_str;
        logMessage("[Info] NORAD field empty. Using _id as NORAD.");
    }

    nlohmann::json newObj = {
        {"_id", id},
        {"NORAD", norad_str},
        {"Name", name_str},
        {"Classification", class_str},
        {"Picture", pic_str},
        {"Groups", nlohmann::json::array()}
    };

    logMessage("[Info] Attempting to create object with _id: " + QString::fromStdString(id_str) + "...");

    std::string errorMsg;
    if (dbManager->createSpaceObject(newObj, m_localPicturePath.toStdString(), errorMsg))
    {
        logMessage("[OK] Object successfully created!");
        ui->idLineEdit_add->clear();
        ui->noradLineEdit_add->clear();
        ui->nameLineEdit_add->clear();
        ui->classLineEdit_add->clear();
        ui->pictureLineEdit_add->clear();
        m_localPicturePath.clear();
        refreshGroupList();
    } else {
        logMessage("[Error] Could not create object. Check log.");
    }
}

void MainWindow::on_eliminarButton_clicked()
{
    if (!dbManager) return;

    if (ui->searchByNameRadioButton->isChecked()) {
        logMessage("[Error] Cannot delete by 'Name'.");
        logMessage("[Info] Please search by '_id' to delete.");
        return;
    }

    std::string id_str = ui->idLineEdit->text().toStdString();
    if (id_str.empty()) {
        logMessage("[Error] Enter an _id in the search field.");
        return;
    }

    try
    {
        int64_t id = std::stoll(id_str);
        nlohmann::json obj = dbManager->getSpaceObjectById(id);

        if (obj.empty() || obj.is_null()) {
            logMessage("[Error] No object found with _id: " + QString::fromStdString(id_str));
            return;
        }

        std::string picName = "";
        if (obj.contains("Picture") && !obj["Picture"].get<std::string>().empty()) {
            picName = obj["Picture"].get<std::string>();
        }

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Deletion",
                                      "Are you sure you want to delete the object with _id: " + QString::fromStdString(id_str) + "?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            logMessage("[Info] Deletion cancelled by user.");
            return;
        }

        logMessage("[Info] Deleting object _id: " + QString::fromStdString(id_str) + "...");

        if (dbManager->deleteSpaceObjectById(id)) {
            logMessage("[OK] Object deleted.");
            ui->idLineEdit->clear();
            ui->imageDisplayLabel->clear();

            if (!picName.empty()) {
                logMessage("[Info] Deleting associated image '" + QString::fromStdString(picName) + "' from GridFS...");
                if (dbManager->getImageManager().deleteImageByName(picName)) {
                    logMessage("[OK] GridFS image deleted.");
                } else {
                    logMessage("[Warning] Could not delete GridFS image.");
                }
            }
        } else {
            logMessage("[Error] Could not delete object from DB.");
        }
    }
    catch (...) {
        logMessage("[Error] The entered _id is not a valid number.");
    }
}

void MainWindow::on_browseButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", QDir::homePath(), "Images (*.png *.jpg *.jpeg *.bmp)");

    if (filePath.isEmpty()) {
        m_localPicturePath.clear();
        return;
    }

    QFileInfo fileInfo(filePath);
    m_localPicturePath = filePath;
    ui->pictureLineEdit_add->setText(fileInfo.fileName());
}

void MainWindow::on_refreshListButton_clicked()
{
    if (!dbManager) {
        logMessage("[Error] Cannot refresh, no DB connection.");
        return;
    }

    logMessage("[Info] Refreshing list from DB...");
    ui->objectListTable->setRowCount(0);
    std::vector<nlohmann::json> allObjects = dbManager->getAllSpaceObjects();

    ui->objectListTable->setSortingEnabled(false);
    ui->objectListTable->setUpdatesEnabled(false);

    for (const auto& obj : allObjects) {
        int rowCount = ui->objectListTable->rowCount();
        ui->objectListTable->insertRow(rowCount);

        for (int col = 0; col < g_tableHeaders.size(); ++col) {
            std::string key = g_tableHeaders[col].toStdString();
            nlohmann::json value;
            if (obj.contains(key)) value = obj[key];

            QString stringValue = jsonValueToQString(value);
            QTableWidgetItem *item = new QTableWidgetItem(stringValue);
            ui->objectListTable->setItem(rowCount, col, item);
        }
    }

    ui->objectListTable->setUpdatesEnabled(true);
    ui->objectListTable->setSortingEnabled(true);
    refreshGroupList();
    logMessage("[Info] List updated. " + QString::number(allObjects.size()) + " objects loaded.");
}

void MainWindow::onObjectListTableContextMenuRequested(const QPoint &pos)
{
    QTableWidgetItem *item = ui->objectListTable->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QMenu contextMenu(this);
    QAction *copyJsonAction = contextMenu.addAction("Copy JSON");
    QAction *selectedAction = contextMenu.exec(ui->objectListTable->mapToGlobal(pos));

    if (selectedAction == copyJsonAction)
    {
        QTableWidgetItem* idItem = ui->objectListTable->item(row, 0);
        if (!idItem) {
            logMessage("[Error] Could not find _id for selected row.");
            return;
        }
        QString idString = idItem->text();

        try {
            int64_t id = idString.toLongLong();
            nlohmann::json obj = dbManager->getSpaceObjectById(id);
            if (obj.empty() || obj.is_null()) {
                logMessage("[Error] Object with _id " + idString + " not found in DB.");
                return;
            }
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(obj.dump(2)));
            logMessage("[Info] JSON for object " + idString + " copied to clipboard.");
        } catch (...) {
            logMessage("[Error] _id '" + idString + "' is not numeric.");
        }
    }
}

// =================================================================
// --- TAB "SETS" (Tab 3) ---
// =================================================================

void MainWindow::setupSetsObjectTable()
{
    QStringList headers = g_tableHeaders;
    headers << "Groups";
    ui->setsObjectTable->setColumnCount(headers.size());
    ui->setsObjectTable->setHorizontalHeaderLabels(headers);
    ui->setsObjectTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->setsObjectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->setsObjectTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->setsObjectTable->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::refreshGroupList()
{
    if (!dbManager) return;

    QString currentGroup;
    if (ui->listWidget->currentItem()) {
        currentGroup = ui->listWidget->currentItem()->text();
    }

    ui->listWidget->clear();
    std::set<std::string> groups = dbManager->getAllUniqueGroupNames();

    QListWidgetItem* itemToSelect = nullptr;
    for (const auto& groupName : groups) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(groupName));
        ui->listWidget->addItem(item);
        if (QString::fromStdString(groupName) == currentGroup) {
            itemToSelect = item;
        }
    }

    if (itemToSelect) {
        ui->listWidget->setCurrentItem(itemToSelect);
    }
    logMessage("[Info] Group list updated.");
}

void MainWindow::populateSetsObjectTable(const std::vector<nlohmann::json>& objects)
{
    ui->setsObjectTable->setSortingEnabled(false);
    ui->setsObjectTable->setUpdatesEnabled(false);
    ui->setsObjectTable->setRowCount(0);

    for (const auto& obj : objects) {
        if (!obj.contains("_id")) continue;

        int row = ui->setsObjectTable->rowCount();
        ui->setsObjectTable->insertRow(row);

        for (int col = 0; col < g_tableHeaders.size(); ++col) {
            std::string key = g_tableHeaders[col].toStdString();
            QString valStr = "";
            if(obj.contains(key)) valStr = jsonValueToQString(obj[key]);
            QTableWidgetItem *item = new QTableWidgetItem(valStr);
            ui->setsObjectTable->setItem(row, col, item);
        }

        int groupCol = g_tableHeaders.size();
        QString groupsStr = "";
        if (obj.contains("Groups") && obj["Groups"].is_array()) {
            QStringList groupsList;
            for (const auto& g : obj["Groups"]) {
                if(g.is_string()) groupsList << QString::fromStdString(g.get<std::string>());
            }
            groupsStr = groupsList.join(", ");
        }
        ui->setsObjectTable->setItem(row, groupCol, new QTableWidgetItem(groupsStr));
    }

    ui->setsObjectTable->setUpdatesEnabled(true);
    ui->setsObjectTable->setSortingEnabled(true);
}

void MainWindow::on_createGroupButton_clicked()
{
    if (!dbManager) return;
    QString groupName = ui->newGroupLineEdit->text().trimmed();
    if (groupName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Group name cannot be empty.");
        return;
    }
    if (dbManager->crearGrupo(groupName.toStdString())) {
        logMessage("[OK] Group '" + groupName + "' created.");
        ui->newGroupLineEdit->clear();
        refreshGroupList();
    } else {
        QMessageBox::warning(this, "Error", "Could not create group.\nIt may already exist.");
    }
}

void MainWindow::on_deleteGroupButton_clicked()
{
    if (!dbManager) return;
    QListWidgetItem* selectedItem = ui->listWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "Error", "Please select a group from the list to delete.");
        return;
    }
    QString groupName = selectedItem->text();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm deletion",
                                  "Are you sure you want to delete group '" + groupName + "'?\n\nThis will remove it from ALL objects.",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        logMessage("[Info] Group deletion cancelled.");
        return;
    }
    if (dbManager->eliminarGrupo(groupName.toStdString())) {
        logMessage("[OK] Group '" + groupName + "' deleted.");
        refreshGroupList();
        on_refreshListButton_2_clicked();
    } else {
        QMessageBox::warning(this, "Error", "An error occurred while deleting the group.");
    }
}

void MainWindow::on_refreshListButton_2_clicked()
{
    if (!dbManager) return;

    QList<QListWidgetItem*> selectedGroupsItems = ui->listWidget->selectedItems();
    QSet<QString> selectedGroupNames;
    for (QListWidgetItem* item : selectedGroupsItems) {
        selectedGroupNames.insert(item->text());
    }

    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();
    QSet<int64_t> selectedObjectIDs;
    for (const QModelIndex& rowIndex : selectedRowsIndexes) {
        if (ui->setsObjectTable->item(rowIndex.row(), 0)) {
            selectedObjectIDs.insert(ui->setsObjectTable->item(rowIndex.row(), 0)->text().toLongLong());
        }
    }

    std::vector<nlohmann::json> objects;
    if (ui->showAllObjectsCheckBox->isChecked()) {
        objects = dbManager->getAllSpaceObjects();
    } else {
        std::set<std::string> selectedGroupsForFilter;
        for (const QString& name : selectedGroupNames) {
            selectedGroupsForFilter.insert(name.toStdString());
        }
        if (!selectedGroupsForFilter.empty()) {
            objects = dbManager->getSpaceObjectsByGroups(selectedGroupsForFilter);
        } else {
            objects.clear();
        }
    }

    populateSetsObjectTable(objects);

    QString infoMsg = "[Info] Table updated. " + QString::number(objects.size()) + " objects visible.";
    if(!ui->showAllObjectsCheckBox->isChecked() && !selectedGroupNames.isEmpty()) {
        infoMsg += " (Filtered by group)";
    }
    logMessage(infoMsg);

    ui->listWidget->blockSignals(true);
    refreshGroupList();
    ui->listWidget->clearSelection();
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* item = ui->listWidget->item(i);
        if (selectedGroupNames.contains(item->text())) {
            item->setSelected(true);
        }
    }
    ui->listWidget->blockSignals(false);

    ui->setsObjectTable->blockSignals(true);
    ui->setsObjectTable->clearSelection();
    for (int row = 0; row < ui->setsObjectTable->rowCount(); ++row) {
        if (ui->setsObjectTable->item(row, 0)) {
            int64_t objectId = ui->setsObjectTable->item(row, 0)->text().toLongLong();
            if (selectedObjectIDs.contains(objectId)) {
                ui->setsObjectTable->selectRow(row);
            }
        }
    }
    ui->setsObjectTable->blockSignals(false);
}

void MainWindow::on_assignToGroupButton_clicked()
{
    if (!dbManager) return;

    QList<QListWidgetItem*> selectedGroups = ui->listWidget->selectedItems();
    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();

    if (selectedGroups.isEmpty()) {
        QMessageBox::warning(this, "Error", "Select at least one GROUP to assign.");
        return;
    }
    if (selectedRowsIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Select at least one OBJECT from the table.");
        return;
    }

    int successCount = 0;
    int failCount = 0;

    for (const QModelIndex& rowIndex : selectedRowsIndexes) {
        int row = rowIndex.row();
        int64_t objectId = ui->setsObjectTable->item(row, 0)->text().toLongLong();
        QString objectName = ui->setsObjectTable->item(row, 1)->text();

        for (QListWidgetItem* groupItem : selectedGroups) {
            QString groupName = groupItem->text();
            if (dbManager->addObjectToGroup(objectId, groupName.toStdString())) {
                successCount++;
            } else {
                failCount++;
                logMessage(QString("[Error] Could not add '%1' to group '%2'.").arg(objectName).arg(groupName));
            }
        }
    }

    QString msg = QString("Successfully performed %1 assignments.").arg(successCount);
    if (failCount > 0) msg += QString("\nTHERE WERE %1 ERRORS. Check the log.").arg(failCount);

    logMessage(msg);
    if(successCount > 0) on_refreshListButton_2_clicked();
}

void MainWindow::on_removeFromGroupButton_clicked()
{
    if (!dbManager) return;

    QList<QListWidgetItem*> selectedGroups = ui->listWidget->selectedItems();
    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();

    if (selectedGroups.isEmpty()) {
        QMessageBox::warning(this, "Error", "Select at least one GROUP to unassign.");
        return;
    }
    if (selectedRowsIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Select at least one OBJECT from the table.");
        return;
    }

    int successCount = 0;
    int failCount = 0;

    for (const QModelIndex& rowIndex : selectedRowsIndexes) {
        int row = rowIndex.row();
        int64_t objectId = ui->setsObjectTable->item(row, 0)->text().toLongLong();
        QString objectName = ui->setsObjectTable->item(row, 1)->text();

        for (QListWidgetItem* groupItem : selectedGroups) {
            QString groupName = groupItem->text();
            if (dbManager->removeObjectFromGroup(objectId, groupName.toStdString())) {
                successCount++;
            } else {
                failCount++;
                logMessage(QString("[Error] Could not remove '%1' from group '%2'.").arg(objectName).arg(groupName));
            }
        }
    }

    QString msg = QString("Successfully performed %1 group removals.").arg(successCount);
    if (failCount > 0) msg += QString("\nTHERE WERE %1 ERRORS. Check the log.").arg(failCount);

    logMessage(msg);
    if(successCount > 0) on_refreshListButton_2_clicked();
}

/**
 * @brief Opens the "Add Object" dialog in non-modal mode.
 * * Uses std::unique_ptr (m_addDialog) to manage the window instance.
 * * Created with 'nullptr' parent so it behaves as an independent window
 * (Main window can cover it, and vice-versa).
 */
void MainWindow::on_addNewObjectSetButton_clicked()
{
    if (!dbManager) {
        logMessage("[Error] Sin conexión a BBDD.");
        return;
    }

    // 1. CONCURRENCY CHECK
    if (m_addDialog) {
        logMessage("[Info] La ventana 'Añadir Objeto' ya está abierta.");
        m_addDialog->showNormal();
        m_addDialog->raise();
        m_addDialog->activateWindow();
        return;
    }

    // 2. CREATE INSTANCE (INDEPENDENT WINDOW)
    // We pass 'nullptr' instead of 'this'. This breaks the parent-child Z-order,
    // allowing the Main Window to cover this dialog if clicked.
    m_addDialog = std::make_unique<AddObjectDialog>(nullptr);

    // 3. PREPARE DATA
    std::set<std::string> groups = dbManager->getAllUniqueGroupNames();
    m_addDialog->setAvailableGroups(groups);
    m_addDialog->setDbManager(dbManager.get());

    // 4. CONNECT SIGNAL
    connect(m_addDialog.get(), &QDialog::finished, this, [this](int result) {
        if (result == QDialog::Accepted) {
            logMessage("[OK] Nuevo objeto añadido exitosamente.");
            on_refreshListButton_2_clicked();
            on_refreshListButton_clicked();
        } else {
            logMessage("[Info] Operación de añadir cancelada.");
        }
        // CRITICAL: Reset pointer
        m_addDialog.reset();
    });

    // 5. SHOW
    m_addDialog->show();
}

/**
 * @brief Opens the "Edit Object" dialog in non-modal mode.
 * * Validates selection and opens an independent window (nullptr parent)
 * to allow free stacking order between Main and Edit windows.
 */
void MainWindow::on_editObjectButton_clicked()
{
    if (!dbManager) return;

    // 1. CONCURRENCY CHECK
    if (m_editDialog) {
        logMessage("[Info] La ventana 'Editar Objeto' ya está abierta. Por favor, ciérrela primero.");
        m_editDialog->showNormal();
        m_editDialog->raise();
        m_editDialog->activateWindow();
        return;
    }

    // 2. VALIDATE SELECTION
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "Por favor, selecciona un objeto para editar.");
        return;
    }
    if (selectedRows.count() > 1) {
        QMessageBox::warning(this, "Aviso", "Solo puedes editar UN objeto a la vez.");
        return;
    }

    // 3. RETRIEVE DATA
    int row = selectedRows.first().row();
    QTableWidgetItem* idItem = ui->setsObjectTable->item(row, 0);
    if (!idItem) return;

    int64_t id = idItem->text().toLongLong();
    nlohmann::json objData = dbManager->getSpaceObjectById(id);

    if (objData.empty()) {
        logMessage("[Error] No se pudo recuperar la información del objeto de la BBDD.");
        return;
    }

    // 4. CREATE DIALOG (INDEPENDENT WINDOW)
    // Pass 'nullptr' so it doesn't stay on top of the Main Window forcedly.
    m_editDialog = std::make_unique<AddObjectDialog>(nullptr);

    m_editDialog->setAvailableGroups(dbManager->getAllUniqueGroupNames());
    m_editDialog->loadObjectData(objData);
    m_editDialog->setEditMode(true);
    m_editDialog->setDbManager(dbManager.get());

    // 5. CONNECT SIGNAL
    connect(m_editDialog.get(), &QDialog::finished, this, [this](int result){
        if (result == QDialog::Accepted) {
            logMessage("[OK] Objeto editado correctamente.");
            on_refreshListButton_2_clicked();
            on_refreshListButton_clicked();
        } else {
            logMessage("[Info] Operación de edición cancelada.");
        }
        // CRITICAL: Reset pointer
        m_editDialog.reset();
    });

    // 6. SHOW
    m_editDialog->show();
}

/**
 * @brief Handles mass deletion of objects from the Sets table.
 * * Iterates over selected rows, retrieves object data to identify associated images,
 * and deletes both the database record and the GridFS image.
 * Uses C++ standard types (std::string) for logic processing.
 */
void MainWindow::on_deleteObjectSetButton_clicked()
{
    if (!dbManager) return;

    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select at least one object to delete.");
        return;
    }

    int count = selectedRows.count();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Deletion",
                                  QString("Are you SURE you want to delete %1 object(s)?\n\nThis action is irreversible.").arg(count),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    logMessage("[Info] Starting mass deletion...");
    int deletedCount = 0;

    for (const auto& idx : selectedRows) {
        int row = idx.row();
        QTableWidgetItem* idItem = ui->setsObjectTable->item(row, 0);

        // Safety check
        if (!idItem) continue;

        bool ok;
        int64_t id = idItem->text().toLongLong(&ok);
        if (!ok) continue;

        // 1. Retrieve object (C++ Logic)
        nlohmann::json obj = dbManager->getSpaceObjectById(id);

        // 2. Extract image name using std::string
        std::string picName = "";
        if (!obj.empty() && obj.contains("Picture") && obj["Picture"].is_string()) {
            picName = obj["Picture"].get<std::string>();
        }

        // 3. Delete from DB
        if (dbManager->deleteSpaceObjectById(id)) {
            deletedCount++;
            // 4. Delete image if exists
            if (!picName.empty()) {
                dbManager->getImageManager().deleteImageByName(picName);
            }
        }
    }

    QMessageBox::information(this, "Success", "Successfully deleted " + QString::number(deletedCount) + " objects.");

    // Refresh interfaces
    on_refreshListButton_2_clicked();
    on_refreshListButton_clicked();
}

void MainWindow::onSetsTableContextMenuRequested(const QPoint &pos)
{
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    QMenu contextMenu(this);
    QString textoCopiar = (selectedRows.count() > 1) ? "Copy JSON (Array)" : "Copy JSON";
    QAction *copyJsonAction = contextMenu.addAction(textoCopiar);
    contextMenu.addSeparator();
    QAction *assignGroupAction = contextMenu.addAction("Assign to Groups...");

    QAction *selectedAction = contextMenu.exec(ui->setsObjectTable->mapToGlobal(pos));

    if (selectedAction == copyJsonAction)
    {
        nlohmann::json finalJson;
        if (selectedRows.count() == 1) {
            int64_t id = ui->setsObjectTable->item(selectedRows.first().row(), 0)->text().toLongLong();
            finalJson = dbManager->getSpaceObjectById(id);
        } else {
            finalJson = nlohmann::json::array();
            for (const auto& idx : selectedRows) {
                int64_t id = ui->setsObjectTable->item(idx.row(), 0)->text().toLongLong();
                nlohmann::json obj = dbManager->getSpaceObjectById(id);
                if (!obj.empty()) finalJson.push_back(obj);
            }
        }
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(QString::fromStdString(finalJson.dump(2)));
        logMessage("[Info] JSON copied to clipboard (" + QString::number(selectedRows.count()) + " objects).");
    }
    else if (selectedAction == assignGroupAction)
    {
        QDialog groupDialog(this);
        groupDialog.setWindowTitle("Select Groups");
        groupDialog.setMinimumWidth(300);
        QVBoxLayout *layout = new QVBoxLayout(&groupDialog);
        layout->addWidget(new QLabel("Select groups to add:", &groupDialog));

        QListWidget *listWidget = new QListWidget(&groupDialog);
        listWidget->setSelectionMode(QAbstractItemView::MultiSelection);
        std::set<std::string> allGroups = dbManager->getAllUniqueGroupNames();
        for (const auto& g : allGroups) listWidget->addItem(QString::fromStdString(g));
        layout->addWidget(listWidget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &groupDialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &groupDialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &groupDialog, &QDialog::reject);
        layout->addWidget(buttonBox);

        if (groupDialog.exec() == QDialog::Accepted) {
            std::vector<std::string> targetGroups;
            for (auto item : listWidget->selectedItems()) targetGroups.push_back(item->text().toStdString());
            if (targetGroups.empty()) return;

            int successCount = 0;
            for (const auto& idx : selectedRows) {
                int64_t objId = ui->setsObjectTable->item(idx.row(), 0)->text().toLongLong();
                for (const auto& groupName : targetGroups) {
                    if (dbManager->addObjectToGroup(objId, groupName)) successCount++;
                }
            }
            logMessage("[OK] Groups assigned.");
            on_refreshListButton_2_clicked();
        }
    }
}

/**
 * @brief Toggles the "Show All Objects" filter.
 * * If checked, clears group selection to show the full dataset.
 */
void MainWindow::on_showAllObjectsCheckBox_toggled(bool checked)
{
    if (checked) {
        ui->listWidget->blockSignals(true);
        ui->listWidget->clearSelection();
        ui->listWidget->blockSignals(false);
        on_refreshListButton_2_clicked();
    }
}

/**
 * @brief Handles selection changes in the Group List.
 * * Automatically unchecks "Show All Objects" if a group is selected
 * and triggers a table refresh.
 */
void MainWindow::on_listWidget_itemSelectionChanged()
{
    if (!ui->listWidget->selectedItems().isEmpty()) {
        if (ui->showAllObjectsCheckBox->isChecked()) {
            ui->showAllObjectsCheckBox->blockSignals(true);
            ui->showAllObjectsCheckBox->setChecked(false);
            ui->showAllObjectsCheckBox->blockSignals(false);
        }
        on_refreshListButton_2_clicked();
    }
}

/**
 * @brief Updates the "Details" panel on the right side of the Sets tab.
 * * Fills the text fields and loads the image from GridFS when a row is selected.
 * Handles cases with multiple selections or no selection by clearing the fields.
 */
void MainWindow::on_setsObjectTable_selectionChanged()
{
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();

    // Clear details if no selection or multiple items selected
    if (selectedRows.isEmpty() || selectedRows.count() > 1) {
        ui->setsDetailImageLabel->clear();
        ui->setsDetailImageLabel->setText("No selection");
        ui->setsDetailNorad->clear();
        ui->setsDetailName->clear();
        ui->setsDetailAlias->clear();
        ui->setsDetailAltitude->clear();
        ui->setsDetailLrr->clear();
        ui->setsDetailComments->clear();
        return;
    }

    // Get ID safely
    int row = selectedRows.first().row();
    QTableWidgetItem* idItem = ui->setsObjectTable->item(row, 0);
    if (!idItem) return;

    int64_t id = idItem->text().toLongLong();

    if (!dbManager) return;

    // Retrieve Data (C++ Logic)
    nlohmann::json obj = dbManager->getSpaceObjectById(id);
    if (obj.empty() || obj.is_null()) return;

    // Helper Lambda for safe string extraction
    auto getStr = [&](const std::string& key) {
        if (obj.contains(key) && obj[key].is_string()) {
            return QString::fromStdString(obj[key].get<std::string>());
        }
        return QString("-");
    };

    // Update UI Fields
    ui->setsDetailNorad->setText(QString::number(id));
    ui->setsDetailName->setText(getStr("Name"));
    ui->setsDetailAlias->setText(getStr("Abbreviation"));
    ui->setsDetailComments->setText(getStr("Comments"));

    // Handle Numeric Fields
    if (obj.contains("Altitude") && obj["Altitude"].is_number()) {
        ui->setsDetailAltitude->setText(QString::number(obj["Altitude"].get<double>()) + " km");
    } else {
        ui->setsDetailAltitude->setText("-");
    }

    if (obj.contains("LaserRetroReflector")) {
        // Handle boolean logic (0/1 or true/false)
        if (obj["LaserRetroReflector"].is_null()) {
            ui->setsDetailLrr->setText("Unknown");
        } else {
            int lrrVal = obj["LaserRetroReflector"].get<int>(); // Assuming it stores 0 or 1
            ui->setsDetailLrr->setText(lrrVal == 1 ? "Yes" : "No");
        }
    } else {
        ui->setsDetailLrr->setText("-");
    }

    // Handle Image (GridFS)
    std::string picName = "";
    if (obj.contains("Picture") && obj["Picture"].is_string()) {
        picName = obj["Picture"].get<std::string>();
    }

    ui->setsDetailImageLabel->clear();
    if (!picName.empty()) {
        // Retrieve binary data
        std::string imgData = dbManager->getImageManager().downloadImageByName(picName);

        QPixmap pix;
        if (!imgData.empty() && pix.loadFromData(QByteArray(imgData.data(), imgData.length()))) {
            ui->setsDetailImageLabel->setPixmap(pix.scaled(
                ui->setsDetailImageLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                ));
        } else {
            ui->setsDetailImageLabel->setText("Image Error");
        }
    } else {
        ui->setsDetailImageLabel->setText("No Image");
    }
}

/**
 * @brief Search for an object by ID using a non-modal input dialog.
 * * Opens an independent window (QInputDialog).
 * * Validates input asynchronously and updates the table if found.
 */
void MainWindow::on_searchObjectButton_clicked()
{
    if (!dbManager) return;

    // 1. CONCURRENCY CHECK
    if (m_searchDialog) {
        logMessage("[Info] The search window is already open.");
        m_searchDialog->showNormal();
        m_searchDialog->raise();
        m_searchDialog->activateWindow();
        return;
    }

    // 2. CREATE INSTANCE (Independent Window -> nullptr parent)
    m_searchDialog = std::make_unique<QInputDialog>(nullptr);

    // Configure dialog
    m_searchDialog->setWindowTitle("Search Object");
    m_searchDialog->setLabelText("Enter ID (Norad):");
    m_searchDialog->setInputMode(QInputDialog::TextInput);
    m_searchDialog->setModal(false); // Explicitly non-modal

    // 3. CONNECT SIGNAL
    // Use a local pointer 'dlg' inside the lambda for safety
    QInputDialog* dlg = m_searchDialog.get();

    connect(dlg, &QInputDialog::finished, this, [this, dlg](int result) {
        if (result == QDialog::Accepted) {
            QString text = dlg->textValue(); // Use local pointer

            if (!text.isEmpty()) {
                int64_t id = 0;
                bool conversionOk = false;
                try {
                    id = text.toLongLong(&conversionOk);
                } catch (...) {}

                if (!conversionOk) {
                    QMessageBox::warning(this, "Error", "ID must be numeric.");
                }
                else {
                    nlohmann::json obj = dbManager->getSpaceObjectById(id);
                    if (obj.empty() || obj.is_null()) {
                        QMessageBox::information(this, "No results", "No object found with that ID.");
                    } else {
                        // Reset UI Filters
                        ui->showAllObjectsCheckBox->blockSignals(true);
                        ui->showAllObjectsCheckBox->setChecked(false);
                        ui->showAllObjectsCheckBox->blockSignals(false);

                        ui->listWidget->blockSignals(true);
                        ui->listWidget->clearSelection();
                        ui->listWidget->blockSignals(false);

                        // Populate table
                        std::vector<nlohmann::json> singleObjList;
                        singleObjList.push_back(obj);
                        populateSetsObjectTable(singleObjList);

                        if (ui->setsObjectTable->rowCount() > 0) {
                            ui->setsObjectTable->selectRow(0);
                            on_setsObjectTable_selectionChanged();
                        }
                        logMessage("[Info] Search finished. Object " + text + " found.");
                    }
                }
            }
        } else {
            logMessage("[Info] Search cancelled.");
        }

        // --- FIX ANTICRASH ---
        // 1. Release the raw pointer from the unique_ptr (m_searchDialog becomes null)
        if (m_searchDialog) {
            QInputDialog* rawPtr = m_searchDialog.release();
            // 2. Tell Qt to delete the window when safe (not immediately)
            rawPtr->deleteLater();
        }
    });

    // 4. SHOW
    m_searchDialog->show();
}
