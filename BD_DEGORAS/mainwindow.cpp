#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SpaceObjectDBManager.h"
#include "json_helpers.h"
#include "addobjectdialog.h"

// QT INCLUDES
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QPixmap>
#include <QPainter>
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
#include <QKeyEvent>

// LOGGING
#include <spdlog/spdlog.h>

// STD C++ INCLUDES
#include <string>
#include <memory>
#include <vector>
#include <set>

// Table headers based on DB keys.
const QStringList g_tableHeaders = {
    "EnablementPolicy", "NORAD", "Alias", "Name",
    "COSPAR", "Classification", "ILRSID", "Inclination",
    "IsDebris", "LaserRetroReflector", "NormalPointIndicator",
    "Picture", "ProviderCPF", "RadarCrossSection", "SIC", "TrackHighPower",
    "Sets", "Groups"
};

// --- CONSTRUCTOR ---
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Configure tables
    setupTables();
    initIcons(); // Inicializar iconos cacheados

    // 2. Conexiones Manuales (TAB 1)
    connect(ui->mainObjectTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::on_editObjectButton_clicked);
    connect(ui->mainObjectTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        handleUniversalContextMenu(pos, ui->mainObjectTable);
    });
    connect(ui->mainObjectTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::on_mainObjectTable_selectionChanged);

    // Filtros
    connect(ui->filterAllRadio, &QRadioButton::toggled, this, &MainWindow::refreshMainTable);
    connect(ui->filterEnabledRadio, &QRadioButton::toggled, this, &MainWindow::refreshMainTable);
    connect(ui->filterDisabledRadio, &QRadioButton::toggled, this, &MainWindow::refreshMainTable);
    connect(ui->showAllObjectsCheckBox, &QCheckBox::toggled, this, &MainWindow::refreshMainTable);

    // TAB 2: Sets
    connect(ui->setsListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::on_setsListWidget_itemSelectionChanged);
    connect(ui->setsViewTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        handleUniversalContextMenu(pos, ui->setsViewTable);
    });

    // TAB 3: Groups
    connect(ui->groupsListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::on_groupsListWidget_itemSelectionChanged);
    connect(ui->groupsViewTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        handleUniversalContextMenu(pos, ui->groupsViewTable);
    });

    // GENERAL: Cambio de Pestaña
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged);

    // --- 3. Connect to DB ---
    const std::string URI = "mongodb://localhost:27017";
    const std::string DB_NAME = "DegorasDB";
    const std::string COLLECTION_NAME = "space_objects";

    try {
        dbManager = std::make_unique<SpaceObjectDBManager>(URI, DB_NAME, COLLECTION_NAME);
        logMessage("[Info] Successfully connected to DB.");
    }
    catch (const std::exception& e)
    {
        QString errorMsg = "[Fatal Error] Could not connect to MongoDB: " + QString(e.what());
        logMessage(errorMsg);
        ui->centralwidget->setEnabled(false);
    }

    // 4. Carga Inicial
    if (dbManager) {
        refreshMainTable();
        refreshSetListWidget();
        refreshGroupListWidget();

        // Forzar carga de vistas secundarias
        on_setsListWidget_itemSelectionChanged();
        on_groupsListWidget_itemSelectionChanged();
    }

    // 5. Menú Data
    QMenu *menuData = menuBar()->addMenu("Data");

    QAction *actExport = menuData->addAction("Export to CSV");
    actExport->setShortcut(QKeySequence("Ctrl+E"));
    connect(actExport, &QAction::triggered, this, &MainWindow::exportToCSV);

    QAction *actImport = menuData->addAction("Import from JSON");
    actImport->setShortcut(QKeySequence("Ctrl+I"));
    connect(actImport, &QAction::triggered, this, &MainWindow::importFromJSON);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::logMessage(const QString& msg) {
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->resultsTextEdit->append("[" + ts + "] " + msg);
    ui->resultsTextEdit->moveCursor(QTextCursor::End);

    // Spdlog usage preserving English context
    if(msg.contains("[Error]") || msg.contains("[Fatal]")) spdlog::error(msg.toStdString());
    else if(msg.contains("[Warn]")) spdlog::warn(msg.toStdString());
    else spdlog::info(msg.toStdString());
}

void MainWindow::initIcons()
{
    auto createIcon = [](const QColor& color) -> QIcon {
        QPixmap pix(14, 14);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setBrush(QBrush(color));
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, 0, 14, 14);
        QPen pen(Qt::white); pen.setWidth(2); painter.setPen(pen);
        painter.drawRect(1, 1, 12, 12);
        return QIcon(pix);
    };
    m_iconGreen = createIcon(QColor(0, 180, 0));
    m_iconRed   = createIcon(QColor(200, 0, 0));
    m_iconGray  = createIcon(Qt::gray);
}

void MainWindow::setupTables()
{
    auto configTable = [this](QTableWidget* table) {
        table->setColumnCount(g_tableHeaders.size());
        table->setHorizontalHeaderLabels(g_tableHeaders);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table->setContextMenuPolicy(Qt::CustomContextMenu);
    };

    configTable(ui->mainObjectTable);
    configTable(ui->setsViewTable);
    configTable(ui->groupsViewTable);
}

// --- TAB CHANGE LOGIC ---
void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (!dbManager) return;
    // Index 0 = Space Objects, 1 = Sets, 2 = Groups
    if (index == 0) {
        refreshMainTable();
    }
    else if (index == 1) {
        refreshSetListWidget();
        on_setsListWidget_itemSelectionChanged();
    }
    else if (index == 2) {
        refreshGroupListWidget();
        on_groupsListWidget_itemSelectionChanged();
    }
}

// =================================================================
// TAB 1: SPACE OBJECTS (Main Logic)
// =================================================================

void MainWindow::refreshMainTable()
{
    if (!dbManager) return;

    std::vector<nlohmann::json> objects = dbManager->getAllSpaceObjects();
    std::vector<nlohmann::json> filtered;

    bool showAll = ui->filterAllRadio->isChecked();
    bool showEnabled = ui->filterEnabledRadio->isChecked();
    bool showDisabled = ui->filterDisabledRadio->isChecked();

    for (const auto& obj : objects) {
        if (showAll) { filtered.push_back(obj); continue; }

        int status = -1;
        if (obj.contains("EnablementPolicy") && !obj["EnablementPolicy"].is_null())
            status = obj["EnablementPolicy"].get<int>();

        if (showEnabled && status == 1) filtered.push_back(obj);
        else if (showDisabled && status == 2) filtered.push_back(obj);
    }

    populateMainTable(filtered);
    logMessage("[Tab 1] Table refreshed. " + QString::number(filtered.size()) + " objects.");
}

void MainWindow::populateMainTable(const std::vector<nlohmann::json>& objects)
{
    ui->mainObjectTable->setSortingEnabled(false);
    ui->mainObjectTable->setUpdatesEnabled(false);
    ui->mainObjectTable->setRowCount(0);

    int colEn = g_tableHeaders.indexOf("EnablementPolicy");

    for (const auto& obj : objects) {
        if (!obj.contains("_id")) continue;
        int row = ui->mainObjectTable->rowCount();
        ui->mainObjectTable->insertRow(row);

        for (int col = 0; col < g_tableHeaders.size(); ++col) {
            std::string key = g_tableHeaders[col].toStdString();
            QTableWidgetItem *item = new QTableWidgetItem();

            if (col == colEn) {
                int val = -1;
                // VERSIÓN SEGURA ANTI-CRASH
                if(obj.contains(key) && !obj[key].is_null() && obj[key].is_number()) {
                    val = obj[key].get<int>();
                }
                if (val==1) { item->setIcon(m_iconGreen); item->setText("Enabled"); }
                else if (val==2) { item->setIcon(m_iconRed); item->setText("Disabled"); }
                else { item->setIcon(m_iconGray); item->setText("Unknown"); }
            }
            else if (key == "Sets" || key == "Groups") {
                if (obj.contains(key) && obj[key].is_array()) {
                    QStringList l;
                    for(const auto& el : obj[key]) if(el.is_string()) l << QString::fromStdString(el.get<std::string>());
                    item->setText(l.join(", "));
                }
            } else {
                if(obj.contains(key)) item->setText(jsonValueToQString(obj[key]));
            }
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            ui->mainObjectTable->setItem(row, col, item);
        }
    }
    ui->mainObjectTable->setUpdatesEnabled(true);
    ui->mainObjectTable->setSortingEnabled(true);
}

// --- CRUD SLOTS (ADD / EDIT / DELETE / SEARCH) ---

void MainWindow::on_addNewObjectSetButton_clicked()
{
    if (!dbManager) return;
    if (m_addDialog) { m_addDialog->activateWindow(); return; }

    m_addDialog = std::make_unique<AddObjectDialog>(nullptr);
    m_addDialog->setAvailableSets(dbManager->getAllUniqueSetNames());
    m_addDialog->setAvailableGroups(dbManager->getAllUniqueGroupNames());
    m_addDialog->setDbManager(dbManager.get());

    connect(m_addDialog.get(), &QDialog::finished, this, [this](int r){
        if (r == QDialog::Accepted) {
            logMessage("[OK] Object created.");
            refreshMainTable();
        }
        m_addDialog.reset();
    });
    m_addDialog->show();
}

void MainWindow::on_editObjectButton_clicked()
{
    if (!dbManager) return;
    auto sel = ui->mainObjectTable->selectionModel()->selectedRows();
    if (sel.size() != 1) {
        QMessageBox::warning(this, "Warning", "Select exactly one object to edit.");
        return;
    }

    int idCol = g_tableHeaders.indexOf("NORAD");
    int64_t id = ui->mainObjectTable->item(sel.first().row(), idCol)->text().toLongLong();
    nlohmann::json obj = dbManager->getSpaceObjectById(id);

    if (m_editDialog) { m_editDialog->activateWindow(); return; }
    m_editDialog = std::make_unique<AddObjectDialog>(nullptr);

    m_editDialog->setAvailableSets(dbManager->getAllUniqueSetNames());
    m_editDialog->setAvailableGroups(dbManager->getAllUniqueGroupNames());
    m_editDialog->loadObjectData(obj);
    m_editDialog->setEditMode(true);
    m_editDialog->setDbManager(dbManager.get());

    connect(m_editDialog.get(), &QDialog::finished, this, [this](int r){
        if (r == QDialog::Accepted) {
            logMessage("[OK] Object updated.");
            refreshMainTable();
            // Refresh secondary tabs in case data changed
            on_setsListWidget_itemSelectionChanged();
            on_groupsListWidget_itemSelectionChanged();
        }
        m_editDialog.reset();
    });
    m_editDialog->show();
}

void MainWindow::on_deleteObjectSetButton_clicked()
{
    if (!dbManager) return;
    auto sel = ui->mainObjectTable->selectionModel()->selectedRows();
    if (sel.empty()) return;

    if (QMessageBox::Yes != QMessageBox::question(this, "Delete", "Delete selected objects?")) return;

    int idCol = g_tableHeaders.indexOf("NORAD");
    for (const auto& idx : sel) {
        int64_t id = ui->mainObjectTable->item(idx.row(), idCol)->text().toLongLong();

        nlohmann::json obj = dbManager->getSpaceObjectById(id);
        if (obj.contains("Picture") && !obj["Picture"].is_null())
            dbManager->getImageManager().deleteImageByName(obj["Picture"].get<std::string>());

        dbManager->deleteSpaceObjectById(id);
    }
    logMessage("[OK] Objects deleted.");
    refreshMainTable();
}

void MainWindow::on_searchObjectButton_clicked()
{
    if (!dbManager) return;
    if (m_searchDialog) { m_searchDialog->activateWindow(); return; }

    m_searchDialog = std::make_unique<QInputDialog>(nullptr);
    m_searchDialog->setWindowTitle("Search");
    m_searchDialog->setLabelText("Enter NORAD:");
    m_searchDialog->setModal(false);

    QInputDialog* ptr = m_searchDialog.get();
    connect(ptr, &QInputDialog::finished, this, [this, ptr](int r){
        if (r == QDialog::Accepted && !ptr->textValue().isEmpty()) {
            int64_t id = 0;
            bool ok = false;
            try { id = ptr->textValue().toLongLong(&ok); } catch (...) {}

            if(!ok) QMessageBox::warning(this, "Error", "ID must be numeric.");
            else {
                nlohmann::json obj = dbManager->getSpaceObjectById(id);
                if (!obj.empty()) {
                    // Reset filters
                    ui->showAllObjectsCheckBox->blockSignals(true);
                    ui->showAllObjectsCheckBox->setChecked(false);
                    ui->showAllObjectsCheckBox->blockSignals(false);

                    std::vector<nlohmann::json> l; l.push_back(obj);
                    populateMainTable(l);

                    if(ui->mainObjectTable->rowCount() > 0) {
                        ui->mainObjectTable->selectRow(0);
                        on_mainObjectTable_selectionChanged();
                    }
                    logMessage("Object found.");
                } else {
                    QMessageBox::information(this, "Search", "Not found.");
                }
            }
        }
        if(m_searchDialog) { m_searchDialog.release()->deleteLater(); }
    });
    m_searchDialog->show();
}

// --- PANEL LATERAL ---
void MainWindow::on_mainObjectTable_selectionChanged()
{
    auto sel = ui->mainObjectTable->selectionModel()->selectedRows();
    if (sel.size() != 1) {
        ui->setsDetailImageLabel->clear();
        ui->setsDetailNorad->clear();
        ui->setsDetailName->clear();
        ui->setsDetailAlias->clear();
        ui->setsDetailAltitude->clear();
        ui->setsDetailLrr->clear();
        ui->setsDetailComments->clear();
        return;
    }

    int row = sel.first().row();
    int idCol = g_tableHeaders.indexOf("NORAD");
    QString idStr = ui->mainObjectTable->item(row, idCol)->text();
    int64_t id = idStr.toLongLong();

    if (!dbManager) return;
    nlohmann::json obj = dbManager->getSpaceObjectById(id);
    if (obj.empty() || obj.is_null()) return;

    // --- HELPER SEGURO PARA ENTEROS ---
    // Si el campo no existe, es null o no es número, devuelve el valor por defecto (-1)
    auto getIntSafe = [&](const std::string& key, int defValue) -> int {
        if (obj.contains(key) && !obj[key].is_null() && obj[key].is_number()) {
            return obj[key].get<int>();
        }
        return defValue;
    };

    // --- HELPER SEGURO PARA STRINGS ---
    auto getStr = [&](const std::string& key) {
        if (obj.contains(key) && !obj[key].is_null() && obj[key].is_string()) {
            return QString::fromStdString(obj[key].get<std::string>());
        }
        return QString("-");
    };

    // Rellenar campos
    ui->setsDetailNorad->setText(QString::number(id));
    ui->setsDetailName->setText(getStr("Name"));
    ui->setsDetailAlias->setText(getStr("Alias"));
    ui->setsDetailComments->setText(getStr("Comments"));

    // Altitud (Double)
    if (obj.contains("Altitude") && obj["Altitude"].is_number())
        ui->setsDetailAltitude->setText(QString::number(obj["Altitude"].get<double>()));
    else
        ui->setsDetailAltitude->setText("-");

    // Laser Retro Reflector (Usando el helper seguro)
    int lrrVal = getIntSafe("LaserRetroReflector", -1); // -1 = Unknown

    if (lrrVal == 1) ui->setsDetailLrr->setText("Yes");
    else if (lrrVal == 0) ui->setsDetailLrr->setText("No");
    else ui->setsDetailLrr->setText("Unknown");


    // Imagen
    std::string picName = "";
    if (obj.contains("Picture") && !obj["Picture"].is_null() && obj["Picture"].is_string()) {
        picName = obj["Picture"].get<std::string>();
    }

    ui->setsDetailImageLabel->clear();
    if (!picName.empty()) {
        std::string imgData = dbManager->getImageManager().downloadImageByName(picName);
        QPixmap pix;
        if (!imgData.empty() && pix.loadFromData(QByteArray(imgData.data(), imgData.length())))
            ui->setsDetailImageLabel->setPixmap(pix.scaled(200, 200, Qt::KeepAspectRatio));
        else
            ui->setsDetailImageLabel->setText("Image Error");
    } else {
        ui->setsDetailImageLabel->setText("No Image");
    }
}

// =================================================================
// TAB 2: SETS
// =================================================================

void MainWindow::refreshSetListWidget()
{
    if (!dbManager) return;

    QString current = ui->setsListWidget->currentItem() ? ui->setsListWidget->currentItem()->text() : "";

    ui->setsListWidget->clear();
    for(const auto& s : dbManager->getAllUniqueSetNames()) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(s));
        ui->setsListWidget->addItem(item);
        if(s == current.toStdString()) ui->setsListWidget->setCurrentItem(item);
    }
}

void MainWindow::on_setsListWidget_itemSelectionChanged()
{
    if (ui->setsListWidget->selectedItems().isEmpty()) {
        populateReadOnlyTable(ui->setsViewTable, dbManager->getAllSpaceObjects());
        return;
    }
    std::set<std::string> sets;
    for(auto i : ui->setsListWidget->selectedItems()) sets.insert(i->text().toStdString());

    populateReadOnlyTable(ui->setsViewTable, dbManager->getSpaceObjectsBySets(sets));
}

void MainWindow::on_createSetButton_clicked() {
    QString name = ui->newSetLineEdit->text().trimmed();
    if(!name.isEmpty() && dbManager->createSet(name.toStdString())) {
        refreshSetListWidget();
        ui->newSetLineEdit->clear();
        logMessage("Set created: " + name);
    }
}

void MainWindow::on_deleteSetButton_clicked() {
    if(ui->setsListWidget->selectedItems().isEmpty()) return;
    QString name = ui->setsListWidget->currentItem()->text();

    if(QMessageBox::Yes != QMessageBox::question(this, "Delete", "Delete Set '" + name + "'?")) return;

    if(dbManager->deleteSet(name.toStdString())) {
        refreshSetListWidget();
        on_setsListWidget_itemSelectionChanged();
        refreshMainTable();
        logMessage("Set deleted: " + name);
    }
}

void MainWindow::on_assignToSetButton_clicked() {
    // Logic: Assign selection from Main Table OR Sets View Table?
    // Requirement: "In all tabs I want to select objects and add them to sets/groups"
    // So we check if the current view table has selection, otherwise main table.

    QList<QModelIndex> selectedObjs;
    bool fromView = false;

    if(ui->setsViewTable->selectionModel()->hasSelection()) {
        selectedObjs = ui->setsViewTable->selectionModel()->selectedRows();
        fromView = true;
    } else {
        selectedObjs = ui->mainObjectTable->selectionModel()->selectedRows();
    }

    auto sets = ui->setsListWidget->selectedItems();
    if(selectedObjs.empty() || sets.empty()) {
        QMessageBox::warning(this, "Info", "Select objects (in table) and destination Sets (in list).");
        return;
    }

    int idCol = g_tableHeaders.indexOf("NORAD");
    int count = 0;
    QTableWidget* table = fromView ? ui->setsViewTable : ui->mainObjectTable;

    for(const auto& idx : selectedObjs) {
        int64_t id = table->item(idx.row(), idCol)->text().toLongLong();
        for(auto s : sets) if(dbManager->addObjectToSet(id, s->text().toStdString())) count++;
    }
    logMessage("Assigned " + QString::number(count) + " objects to sets.");
    if(fromView) on_setsListWidget_itemSelectionChanged(); // Refresh view
    else refreshMainTable();
}

void MainWindow::on_removeFromSetButton_clicked() {
    // Same logic for removal
    QList<QModelIndex> selectedObjs;
    bool fromView = false;

    if(ui->setsViewTable->selectionModel()->hasSelection()) {
        selectedObjs = ui->setsViewTable->selectionModel()->selectedRows();
        fromView = true;
    } else {
        selectedObjs = ui->mainObjectTable->selectionModel()->selectedRows();
    }

    auto sets = ui->setsListWidget->selectedItems();
    if(selectedObjs.empty() || sets.empty()) {
        QMessageBox::warning(this, "Info", "Select objects and Sets to unassign.");
        return;
    }

    int idCol = g_tableHeaders.indexOf("NORAD");
    QTableWidget* table = fromView ? ui->setsViewTable : ui->mainObjectTable;

    for(const auto& idx : selectedObjs) {
        int64_t id = table->item(idx.row(), idCol)->text().toLongLong();
        for(auto s : sets) dbManager->removeObjectFromSet(id, s->text().toStdString());
    }
    logMessage("Removed from sets.");
    if(fromView) on_setsListWidget_itemSelectionChanged();
    else refreshMainTable();
}

// =================================================================
// TAB 3: GROUPS
// =================================================================

void MainWindow::refreshGroupListWidget()
{
    if (!dbManager) return;
    QString current = ui->groupsListWidget->currentItem() ? ui->groupsListWidget->currentItem()->text() : "";

    ui->groupsListWidget->clear();
    for(const auto& g : dbManager->getAllUniqueGroupNames()) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(g));
        ui->groupsListWidget->addItem(item);
        if(g == current.toStdString()) ui->groupsListWidget->setCurrentItem(item);
    }
}

void MainWindow::on_groupsListWidget_itemSelectionChanged()
{
    if (ui->groupsListWidget->selectedItems().isEmpty()) {
        populateReadOnlyTable(ui->groupsViewTable, dbManager->getAllSpaceObjects());
        return;
    }
    std::set<std::string> groups;
    for(auto i : ui->groupsListWidget->selectedItems()) groups.insert(i->text().toStdString());

    populateReadOnlyTable(ui->groupsViewTable, dbManager->getSpaceObjectsByGroups(groups));
}

void MainWindow::on_createGroupButton_clicked() {
    QString name = ui->newGroupLineEdit->text().trimmed();
    if(!name.isEmpty() && dbManager->createGroup(name.toStdString())) {
        refreshGroupListWidget();
        logMessage("Group created: " + name);
        ui->newGroupLineEdit->clear();
    }
}

void MainWindow::on_deleteGroupButton_clicked() {
    if(ui->groupsListWidget->selectedItems().isEmpty()) return;
    QString name = ui->groupsListWidget->currentItem()->text();

    if(QMessageBox::Yes != QMessageBox::question(this, "Delete", "Delete Group '" + name + "'?")) return;

    if(dbManager->deleteGroup(name.toStdString())) {
        refreshGroupListWidget();
        on_groupsListWidget_itemSelectionChanged();
        refreshMainTable();
        logMessage("Group deleted: " + name);
    }
}

void MainWindow::on_assignToGroupButton_clicked() {
    // Logic: Assign selection from Main Table OR Groups View Table
    QList<QModelIndex> selectedObjs;
    bool fromView = false;

    if(ui->groupsViewTable->selectionModel()->hasSelection()) {
        selectedObjs = ui->groupsViewTable->selectionModel()->selectedRows();
        fromView = true;
    } else {
        selectedObjs = ui->mainObjectTable->selectionModel()->selectedRows();
    }

    auto groups = ui->groupsListWidget->selectedItems();
    if(selectedObjs.empty() || groups.empty()) {
        QMessageBox::warning(this, "Info", "Select objects and Groups.");
        return;
    }

    int idCol = g_tableHeaders.indexOf("NORAD");
    QTableWidget* table = fromView ? ui->groupsViewTable : ui->mainObjectTable;
    int count = 0;

    for(const auto& idx : selectedObjs) {
        int64_t id = table->item(idx.row(), idCol)->text().toLongLong();
        for(auto g : groups) {
            if(dbManager->addObjectToGroup(id, g->text().toStdString())) count++;
        }
    }
    logMessage("Assigned " + QString::number(count) + " objects to groups.");
    if(fromView) on_groupsListWidget_itemSelectionChanged();
    else refreshMainTable();
}

void MainWindow::on_removeFromGroupButton_clicked() {
    QList<QModelIndex> selectedObjs;
    bool fromView = false;
    if(ui->groupsViewTable->selectionModel()->hasSelection()) {
        selectedObjs = ui->groupsViewTable->selectionModel()->selectedRows();
        fromView = true;
    } else {
        selectedObjs = ui->mainObjectTable->selectionModel()->selectedRows();
    }
    auto groups = ui->groupsListWidget->selectedItems();
    if(selectedObjs.empty() || groups.empty()) return;

    int idCol = g_tableHeaders.indexOf("NORAD");
    QTableWidget* table = fromView ? ui->groupsViewTable : ui->mainObjectTable;

    for(const auto& idx : selectedObjs) {
        int64_t id = table->item(idx.row(), idCol)->text().toLongLong();
        for(auto g : groups) dbManager->removeObjectFromGroup(id, g->text().toStdString());
    }
    logMessage("Removed objects from groups.");
    if(fromView) on_groupsListWidget_itemSelectionChanged();
    else refreshMainTable();
}

// =================================================================
// HELPERS & MENUS
// =================================================================

void MainWindow::populateReadOnlyTable(QTableWidget* table, const std::vector<nlohmann::json>& objects)
{
    table->setSortingEnabled(false);
    table->setRowCount(0);
    int colEn = g_tableHeaders.indexOf("EnablementPolicy");

    for(const auto& obj : objects) {
        if (!obj.contains("_id")) continue;
        int row = table->rowCount();
        table->insertRow(row);
        for(int col=0; col<g_tableHeaders.size(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem();
            std::string k = g_tableHeaders[col].toStdString();

            if(col==colEn) {
                int val = -1;
                if(obj.contains(k) && !obj[k].is_null()) val = obj[k].get<int>();
                if(val==1) { item->setIcon(m_iconGreen); item->setText("Enabled"); }
                else if(val==2) { item->setIcon(m_iconRed); item->setText("Disabled"); }
                else { item->setIcon(m_iconGray); item->setText("Unknown"); }
            } else if (k == "Sets" || k == "Groups") {
                if (obj.contains(k) && obj[k].is_array()) {
                    QStringList l;
                    for(const auto& s : obj[k]) if(s.is_string()) l << QString::fromStdString(s.get<std::string>());
                    item->setText(l.join(", "));
                }
            } else {
                if(obj.contains(k)) item->setText(jsonValueToQString(obj[k]));
            }
            table->setItem(row, col, item);
        }
    }
    table->setSortingEnabled(true);
}

void MainWindow::handleUniversalContextMenu(const QPoint &pos, QTableWidget* table)
{
    auto selectedRows = table->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    QMenu contextMenu(this);
    QAction *copyJson = contextMenu.addAction("Copy JSON");
    contextMenu.addSeparator();
    QAction *assignSet = contextMenu.addAction("Assign to Sets...");
    QAction *assignGroup = contextMenu.addAction("Assign to Groups...");

    QAction *act = contextMenu.exec(table->mapToGlobal(pos));
    int idCol = g_tableHeaders.indexOf("NORAD");

    if (act == copyJson) {
        nlohmann::json jArr = nlohmann::json::array();
        for(const auto& i : selectedRows) {
            int64_t id = table->item(i.row(), idCol)->text().toLongLong();
            jArr.push_back(dbManager->getSpaceObjectById(id));
        }
        QGuiApplication::clipboard()->setText(QString::fromStdString(jArr.dump(2)));
        logMessage("JSON copied.");
    }
    else if (act == assignSet) {
        QDialog dlg(this); dlg.setWindowTitle("Select Sets");
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        QListWidget *lw = new QListWidget(&dlg); lw->setSelectionMode(QAbstractItemView::MultiSelection);
        for(const auto& s : dbManager->getAllUniqueSetNames()) lw->addItem(QString::fromStdString(s));
        l->addWidget(lw);
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        l->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if(dlg.exec() == QDialog::Accepted) {
            for(const auto& row : selectedRows) {
                int64_t id = table->item(row.row(), idCol)->text().toLongLong();
                for(auto s : lw->selectedItems()) dbManager->addObjectToSet(id, s->text().toStdString());
            }
            logMessage("Sets assigned.");
            if(table == ui->setsViewTable) on_setsListWidget_itemSelectionChanged();
            else refreshMainTable();
        }
    }
    else if (act == assignGroup) {
        QDialog dlg(this); dlg.setWindowTitle("Select Groups");
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        QListWidget *lw = new QListWidget(&dlg); lw->setSelectionMode(QAbstractItemView::MultiSelection);
        for(const auto& g : dbManager->getAllUniqueGroupNames()) lw->addItem(QString::fromStdString(g));
        l->addWidget(lw);
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        l->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if(dlg.exec() == QDialog::Accepted) {
            for(const auto& row : selectedRows) {
                int64_t id = table->item(row.row(), idCol)->text().toLongLong();
                for(auto g : lw->selectedItems()) dbManager->addObjectToGroup(id, g->text().toStdString());
            }
            logMessage("Groups assigned.");
            if(table == ui->groupsViewTable) on_groupsListWidget_itemSelectionChanged();
            else refreshMainTable();
        }
    }
}

void MainWindow::exportToCSV()
{
    QString fn = QFileDialog::getSaveFileName(this, "Export", QDir::homePath(), "CSV (*.csv)");
    if(fn.isEmpty()) return;
    QFile f(fn);
    if(!f.open(QIODevice::WriteOnly)) return;
    QTextStream ts(&f);

    QStringList h;
    for(int i=0; i<ui->mainObjectTable->columnCount(); ++i) h << ui->mainObjectTable->horizontalHeaderItem(i)->text();
    ts << h.join(";") << "\n";

    for(int i=0; i<ui->mainObjectTable->rowCount(); ++i) {
        QStringList l;
        for(int j=0; j<ui->mainObjectTable->columnCount(); ++j) {
            QString t = ui->mainObjectTable->item(i,j)->text();
            if(t.contains(";") || t.contains("\n")) t = "\"" + t + "\"";
            l << t;
        }
        ts << l.join(";") << "\n";
    }
    f.close();
    logMessage("Exported to " + fn);
}

void MainWindow::importFromJSON()
{
    QString fn = QFileDialog::getOpenFileName(this, "Import", QDir::homePath(), "JSON (*.json)");
    if(fn.isEmpty()) return;

    QFile f(fn);
    if(!f.open(QIODevice::ReadOnly)) return;

    try {
        auto j = nlohmann::json::parse(f.readAll());
        std::vector<nlohmann::json> list;
        if(j.is_array()) for(const auto& o : j) list.push_back(o);
        else list.push_back(j);

        QDir dir = QFileInfo(fn).absoluteDir();
        int ok=0;
        for(const auto& obj : list) {
            std::string err;
            std::string imgPath = "";
            if(obj.contains("Picture") && !obj["Picture"].is_null()) {
                QString p = dir.filePath(QString::fromStdString(obj["Picture"]));
                if(QFile::exists(p)) imgPath = p.toStdString();
            }
            if(dbManager->createSpaceObject(obj, imgPath, err)) ok++;
        }
        logMessage("Imported " + QString::number(ok) + " objects.");
        refreshMainTable();
        refreshSetListWidget();
        refreshGroupListWidget();
    } catch(...) {
        QMessageBox::critical(this, "Error", "Invalid JSON");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->mainObjectTable->hasFocus()) {
        if (event->key() == Qt::Key_Delete) {
            on_deleteObjectSetButton_clicked();
        } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            on_editObjectButton_clicked();
        }
    }
    QMainWindow::keyPressEvent(event);
}



