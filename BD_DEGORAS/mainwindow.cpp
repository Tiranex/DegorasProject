#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SpaceObjectDBManager.h"
#include "json_helpers.h"
#include "addobjectdialog.h"
#include "QtLogSink.h"

// QT INCLUDES
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QPixmap>
#include <QPainter>
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
#include <QHeaderView>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <vector>
// LOGGING
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#include <QThread>

// STD
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream> // Necesario para stringstream

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

    // 1. UI Setup
    setupTables();
    setupLogTable();
    initIcons();

    // 2. Connections
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
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::on_searchLineEdit_textChanged);

    // Tab Sets
    connect(ui->setsListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::on_setsListWidget_itemSelectionChanged);
    connect(ui->setsViewTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        handleUniversalContextMenu(pos, ui->setsViewTable);
    });

    // Tab Groups
    connect(ui->groupsListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::on_groupsListWidget_itemSelectionChanged);
    connect(ui->groupsViewTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        handleUniversalContextMenu(pos, ui->groupsViewTable);
    });

    // Cambio de Pestaña
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged);

    // Logs Bridge
    auto& sink = QtLogSinkMt::instance();
    connect(&sink, &QtLogSinkMt::logReceived, this, &MainWindow::onLogReceived);
    auto logger = spdlog::default_logger();
    if (logger) logger->sinks().push_back(std::shared_ptr<QtLogSinkMt>(&sink, [](void*){}));

    // --- 3. CONNECT TO DB ---
    const std::string URI = "mongodb://localhost:27017";
    const std::string DB_NAME = "DegorasDB";
    const std::string COLLECTION_NAME = "space_objects";

    try {
        dbManager = std::make_unique<SpaceObjectDBManager>(URI, DB_NAME, COLLECTION_NAME);

        // A) CARGA MASIVA DE OBJETOS
        m_localCache = dbManager->getAllSpaceObjects();

        // ... dentro del constructor ...

        // A) CARGA MASIVA DE OBJETOS
        m_localCache = dbManager->getAllSpaceObjects();

        // B) CARGA DE SETS (Directo Set -> Set)
        // ELIMINAMOS la conversión a vector
        m_localSets = dbManager->getAllUniqueSetNames();

        // C) CARGA DE GRUPOS (Directo Set -> Set)
        m_localGroups = dbManager->getAllUniqueGroupNames();



        logMessage("[Memory] Loaded DB into local cache. Objects: " + QString::number(m_localCache.size()));

        // ... resto del constructor ...

        logMessage("[Memory] Loaded DB into local cache. Objects: " + QString::number(m_localCache.size()));

        refreshMainTable();
        refreshSetListWidget();
        refreshGroupListWidget();

    } catch (const std::exception& e) {
        logMessage("[Fatal] " + QString(e.what()));
        ui->centralwidget->setEnabled(false);
    }

    // 4. Menús
    QMenu *menuData = menuBar()->addMenu("Data");

    QAction *actExport = menuData->addAction("Export to CSV");
    actExport->setShortcut(QKeySequence("Ctrl+E"));
    connect(actExport, &QAction::triggered, this, &MainWindow::exportToCSV);

    QAction *actImport = menuData->addAction("Import from JSON");
    actImport->setShortcut(QKeySequence("Ctrl+I"));
    connect(actImport, &QAction::triggered, this, &MainWindow::importFromJSON);

    // BOTÓN DE COMMIT (Guardar Cambios)
    QAction *actSave = menuData->addAction("Save Changes to DB");
    actSave->setShortcut(QKeySequence("Ctrl+S"));
    connect(actSave, &QAction::triggered, this, &MainWindow::on_saveChangesToDbButton_clicked);
}

MainWindow::~MainWindow() { delete ui; }

// =================================================================
// MEMORY CRUD (Lógica Local - NO TOCA BBDD)
// =================================================================

void MainWindow::refreshMainTable()
{
    // Filtramos sobre m_localCache
    std::vector<nlohmann::json> filtered;

    bool showAll = ui->filterAllRadio->isChecked();
    bool showEnabled = ui->filterEnabledRadio->isChecked();
    bool showDisabled = ui->filterDisabledRadio->isChecked();

    for (const auto& obj : m_localCache) {
        if (showAll) { filtered.push_back(obj); continue; }

        int status = -1;
        if (obj.contains("EnablementPolicy") && !obj["EnablementPolicy"].is_null() && obj["EnablementPolicy"].is_number())
            status = obj["EnablementPolicy"].get<int>();

        if (showEnabled && status == 1) filtered.push_back(obj);
        else if (showDisabled && status == 2) filtered.push_back(obj);
    }

    populateMainTable(filtered);
}

void MainWindow::on_addNewObjectSetButton_clicked()
{
    if (m_addDialog) { m_addDialog->activateWindow(); return; }

    m_addDialog = std::make_unique<AddObjectDialog>(nullptr);

    // IMPORTANTE: Pasamos los Sets/Groups de MEMORIA, convirtiéndolos a vector para el diálogo
    std::vector<std::string> availableSets(m_localSets.begin(), m_localSets.end());
    std::vector<std::string> availableGroups(m_localGroups.begin(), m_localGroups.end());

    m_addDialog->setAvailableSets(availableSets);
    m_addDialog->setAvailableGroups(availableGroups);
    m_addDialog->setDbManager(dbManager.get()); // Solo para leer lista de imágenes si es necesario
    m_addDialog->setExistingObjects(&m_localCache);

    connect(m_addDialog.get(), &QDialog::finished, this, [this](int r){
        if (r == QDialog::Accepted) {
            nlohmann::json newObj = m_addDialog->getNewObjectData();

            // 1. Validar duplicado local
            int64_t id = newObj["_id"];
            auto it = std::find_if(m_localCache.begin(), m_localCache.end(),
                                   [id](const nlohmann::json& j){ return j["_id"] == id; });

            if (it != m_localCache.end()) {
                QMessageBox::warning(this, "Error", "ID already exists in local cache.");
            } else {
                // 2. Guardar ruta de imagen temporalmente (Subida diferida)
                QString imgPath = m_addDialog->getSelectedImagePath();
                if(!imgPath.isEmpty()) {
                    newObj["_tempLocalImgPath"] = imgPath.toStdString();
                }

                // 3. Añadir a Memoria
                m_localCache.push_back(newObj);
                setUnsavedChanges(true);
                refreshMainTable();
                logMessage("[Memory] Object added: " + QString::number(id));
            }
        }
        m_addDialog.reset();
    });
    m_addDialog->show();
}

void MainWindow::on_editObjectButton_clicked()
{
    auto sel = ui->mainObjectTable->selectionModel()->selectedRows();
    if (sel.size() != 1) { QMessageBox::warning(this, "Warning", "Select exactly one object."); return; }

    int idCol = g_tableHeaders.indexOf("NORAD");
    int64_t id = ui->mainObjectTable->item(sel.first().row(), idCol)->text().toLongLong();

    // Buscar en Caché
    nlohmann::json obj;
    bool found = false;
    for(const auto& o : m_localCache) {
        if(o["_id"] == id) { obj = o; found = true; break; }
    }

    if(!found) return;

    if (m_editDialog) { m_editDialog->activateWindow(); return; }
    m_editDialog = std::make_unique<AddObjectDialog>(nullptr);

    // USAR DATOS LOCALES
    std::vector<std::string> availableSets(m_localSets.begin(), m_localSets.end());
    std::vector<std::string> availableGroups(m_localGroups.begin(), m_localGroups.end());

    m_editDialog->setAvailableSets(availableSets);
    m_editDialog->setAvailableGroups(availableGroups);
    m_editDialog->setDbManager(dbManager.get());
    m_editDialog->loadObjectData(obj);
    m_editDialog->setEditMode(true);
    m_editDialog->setExistingObjects(&m_localCache);
    connect(m_editDialog.get(), &QDialog::finished, this, [this, id](int r){
        if (r == QDialog::Accepted) {
            nlohmann::json edited = m_editDialog->getNewObjectData();

            // Guardar ruta imagen temporal si cambió
            QString imgPath = m_editDialog->getSelectedImagePath();
            if(!imgPath.isEmpty()) edited["_tempLocalImgPath"] = imgPath.toStdString();

            // Actualizar en Memoria
            for(auto& o : m_localCache) {
                if(o["_id"] == id) {
                    // Mantener campos internos temporales si el diálogo no los devuelve
                    if(o.contains("_tempLocalImgPath") && !edited.contains("_tempLocalImgPath"))
                        edited["_tempLocalImgPath"] = o["_tempLocalImgPath"];
                    o = edited;
                    break;
                }
            }

            setUnsavedChanges(true);
            refreshMainTable();
            // Refrescar otras pestañas por si el objeto cambió de set/grupo
            on_setsListWidget_itemSelectionChanged();
            on_groupsListWidget_itemSelectionChanged();
            logMessage("[Memory] Object updated: " + QString::number(id));
        }
        m_editDialog.reset();
    });
    m_editDialog->show();
}

void MainWindow::on_deleteObjectSetButton_clicked()
{
    auto sel = ui->mainObjectTable->selectionModel()->selectedRows();
    if (sel.empty()) return;
    if (QMessageBox::Yes != QMessageBox::question(this, "Delete", "Delete locally? (Changes lost on Save)")) return;

    int idCol = g_tableHeaders.indexOf("NORAD");
    for (const auto& idx : sel) {
        int64_t id = ui->mainObjectTable->item(idx.row(), idCol)->text().toLongLong();

        // Borrar de Memoria
        m_localCache.erase(std::remove_if(m_localCache.begin(), m_localCache.end(),
                                          [id](const nlohmann::json& j){ return j["_id"] == id; }), m_localCache.end());
    }
    setUnsavedChanges(true);
    refreshMainTable();
    logMessage("[Memory] Objects deleted.");
}

void MainWindow::on_searchObjectButton_clicked()
{
    // Búsqueda en memoria
    if (m_searchDialog) { m_searchDialog->activateWindow(); return; }

    m_searchDialog = std::make_unique<QInputDialog>(nullptr);
    m_searchDialog->setWindowTitle("Search in Memory");
    m_searchDialog->setLabelText("Enter NORAD:");
    m_searchDialog->setModal(false);

    QInputDialog* ptr = m_searchDialog.get();
    connect(ptr, &QInputDialog::finished, this, [this, ptr](int r){
        if (r == QDialog::Accepted && !ptr->textValue().isEmpty()) {
            int64_t id = ptr->textValue().toLongLong();

            auto it = std::find_if(m_localCache.begin(), m_localCache.end(),
                                   [id](const nlohmann::json& j){ return j["_id"] == id; });

            if (it != m_localCache.end()) {
                // Reset UI para mostrarlo
                ui->showAllObjectsCheckBox->blockSignals(true);
                ui->showAllObjectsCheckBox->setChecked(false); // Quitar filtro 'all' si afecta visualización
                ui->showAllObjectsCheckBox->blockSignals(false);

                // Forzamos tabla con solo este objeto
                std::vector<nlohmann::json> l; l.push_back(*it);
                populateMainTable(l);

                if(ui->mainObjectTable->rowCount() > 0) {
                    ui->mainObjectTable->selectRow(0);
                    on_mainObjectTable_selectionChanged();
                }
                logMessage("Object found in memory.");
            } else {
                QMessageBox::information(this, "Search", "Not found in local cache.");
            }
        }
        if(m_searchDialog) { m_searchDialog.release()->deleteLater(); }
    });
    m_searchDialog->show();
}

// --- SAVE COMMIT ---

void MainWindow::on_saveChangesToDbButton_clicked()
{
    if (!m_hasUnsavedChanges) {
        QMessageBox::information(this, "Info", "No changes to save.");
        return;
    }
    createDatabaseVersion();
}

void MainWindow::createDatabaseVersion()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Commit Changes");
    dlg.setMinimumWidth(400);

    QVBoxLayout* l = new QVBoxLayout(&dlg);
    QFormLayout* f = new QFormLayout();
    QLineEdit* nameEd = new QLineEdit(&dlg);
    QPlainTextEdit* commEd = new QPlainTextEdit(&dlg);


    // --- NUEVO CHECKBOX ---
    QCheckBox* chkIncremental = new QCheckBox("Save as Incremental (Changes Only)", &dlg);
    chkIncremental->setChecked(true); // Recomendado activarlo por defecto
    chkIncremental->setToolTip("If checked, only modified objects are saved in history.\nUncheck to save a full backup copy.");


    f->addRow("Version Name:", nameEd);
    f->addRow("Comment:", commEd);
    l->addLayout(f);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    l->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted)
    {
        if (nameEd->text().isEmpty()) {
            QMessageBox::warning(this, "Error", "Name required.");
            return;
        }

        std::string err;
        bool ok = false;

        QApplication::setOverrideCursor(Qt::WaitCursor);

        // 1. Procesar Imágenes Pendientes
        bool imagesOk = true;
        for(auto& obj : m_localCache) {
            if (obj.contains("_tempLocalImgPath")) {
                std::string localPath = obj["_tempLocalImgPath"];
                std::string picName = obj.value("Picture", "");
                if (!localPath.empty() && !picName.empty()) {
                    std::ifstream file(localPath, std::ios::binary);
                    if(file.is_open()) {
                        std::stringstream buffer; buffer << file.rdbuf();
                        if (!dbManager->getImageManager().uploadImage(picName, buffer.str())) {
                            imagesOk = false; err = "Failed upload: " + picName; break;
                        }
                    } else { imagesOk = false; err = "File error: " + localPath; break; }
                }
                obj.erase("_tempLocalImgPath");
            }
        }

        if(imagesOk) {
            // 2. PREPARAR SETS/GROUPS PARA DB
            // CORRECCIÓN: Tu DBManager espera std::set, no hace falta convertir a vector.
            // Pasamos m_localSets y m_localGroups directamente.

            // Determinar modo
            SaveMode mode = chkIncremental->isChecked() ? SaveMode::INCREMENTAL : SaveMode::FULL_SNAPSHOT;

            // Llamada actualizada
            ok = dbManager->saveAllAndVersion(m_localCache, m_localSets, m_localGroups,
                                              nameEd->text().toStdString(),
                                              commEd->toPlainText().toStdString(),
                                              mode, // <--- Pasamos el modo
                                              err);
        }
        // 4. GARBAGE COLLECTOR
        if (ok) {
            std::vector<std::string> allDbImages = dbManager->getImageManager().getAllImageNames();
            std::set<std::string> usedImages;
            for(const auto& obj : m_localCache) {
                if(obj.contains("Picture") && !obj["Picture"].is_null()) {
                    std::string p = obj["Picture"];
                    if(!p.empty()) usedImages.insert(p);
                }
            }
            for(const auto& dbImg : allDbImages) {
                if (usedImages.find(dbImg) == usedImages.end()) {
                    dbManager->getImageManager().deleteImageByName(dbImg);
                    spdlog::info("[GC] Deleted orphaned image: {}", dbImg);
                }
            }
        }

        QApplication::restoreOverrideCursor();

        if (ok) {
            setUnsavedChanges(false);
            QMessageBox::information(this, "Success", "Database updated and versioned.");
        } else {
            QMessageBox::critical(this, "Error", QString::fromStdString(err));
        }
    }
}

void MainWindow::setUnsavedChanges(bool changed)
{
    m_hasUnsavedChanges = changed;
    QString t = "Degoras Project";
    if (changed) t += " * (Unsaved Changes)";
    this->setWindowTitle(t);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_hasUnsavedChanges) {
        if (QMessageBox::Yes != QMessageBox::question(this, "Unsaved", "Exit without saving?", QMessageBox::Yes|QMessageBox::No)) {
            event->ignore(); return;
        }
    }
    event->accept();
}

// --- ASIGNACIONES EN MEMORIA (Tab 2/3) ---

void MainWindow::on_assignToSetButton_clicked() {
    auto objs = ui->mainObjectTable->selectionModel()->selectedRows();
    auto sets = ui->setsListWidget->selectedItems();
    if(objs.empty() || sets.empty()) return;

    int idCol = g_tableHeaders.indexOf("NORAD");
    for(const auto& idx : objs) {
        int64_t id = ui->mainObjectTable->item(idx.row(), idCol)->text().toLongLong();
        for(auto& o : m_localCache) {
            if(o["_id"] == id) {
                std::vector<std::string> cur = o.value("Sets", std::vector<std::string>());
                for(auto s : sets) {
                    std::string n = s->text().toStdString();
                    if(std::find(cur.begin(), cur.end(), n) == cur.end()) cur.push_back(n);
                }
                o["Sets"] = cur;
            }
        }
    }
    setUnsavedChanges(true);
    refreshMainTable();
    // Forzar refresco de la vista sets si el set asignado está seleccionado
    on_setsListWidget_itemSelectionChanged();
}

void MainWindow::on_removeFromSetButton_clicked() {
    QList<QModelIndex> sel;
    bool fromView = false;
    if(ui->setsViewTable->selectionModel()->hasSelection()) { sel = ui->setsViewTable->selectionModel()->selectedRows(); fromView = true; }
    else { sel = ui->mainObjectTable->selectionModel()->selectedRows(); }

    auto sets = ui->setsListWidget->selectedItems();
    if(sel.empty() || sets.empty()) return;

    int idCol = g_tableHeaders.indexOf("NORAD");
    QTableWidget* table = fromView ? ui->setsViewTable : ui->mainObjectTable;

    for(const auto& idx : sel) {
        int64_t id = table->item(idx.row(), idCol)->text().toLongLong();
        for(auto& o : m_localCache) {
            if(o["_id"] == id && o.contains("Sets")) {
                std::vector<std::string> cur = o["Sets"];
                for(auto s : sets) {
                    std::string n = s->text().toStdString();
                    cur.erase(std::remove(cur.begin(), cur.end(), n), cur.end());
                }
                o["Sets"] = cur;
            }
        }
    }
    setUnsavedChanges(true);
    on_setsListWidget_itemSelectionChanged();
}

void MainWindow::on_assignToGroupButton_clicked() {
    auto objs = ui->mainObjectTable->selectionModel()->selectedRows();
    auto groups = ui->groupsListWidget->selectedItems();
    if(objs.empty() || groups.empty()) return;
    int idCol = g_tableHeaders.indexOf("NORAD");
    for(const auto& idx : objs) {
        int64_t id = ui->mainObjectTable->item(idx.row(), idCol)->text().toLongLong();
        for(auto& o : m_localCache) {
            if(o["_id"] == id) {
                std::vector<std::string> cur = o.value("Groups", std::vector<std::string>());
                for(auto g : groups) {
                    std::string n = g->text().toStdString();
                    if(std::find(cur.begin(), cur.end(), n) == cur.end()) cur.push_back(n);
                }
                o["Groups"] = cur;
            }
        }
    }
    setUnsavedChanges(true);
    refreshMainTable();
    on_groupsListWidget_itemSelectionChanged();
}

void MainWindow::on_removeFromGroupButton_clicked() {
    QList<QModelIndex> sel;
    bool fromView = false;
    if(ui->groupsViewTable->selectionModel()->hasSelection()) { sel = ui->groupsViewTable->selectionModel()->selectedRows(); fromView = true; }
    else { sel = ui->mainObjectTable->selectionModel()->selectedRows(); }
    auto groups = ui->groupsListWidget->selectedItems();
    if(sel.empty() || groups.empty()) return;

    int idCol = g_tableHeaders.indexOf("NORAD");
    QTableWidget* table = fromView ? ui->groupsViewTable : ui->mainObjectTable;

    for(const auto& idx : sel) {
        int64_t id = table->item(idx.row(), idCol)->text().toLongLong();
        for(auto& o : m_localCache) {
            if(o["_id"] == id && o.contains("Groups")) {
                std::vector<std::string> cur = o["Groups"];
                for(auto g : groups) {
                    std::string n = g->text().toStdString();
                    cur.erase(std::remove(cur.begin(), cur.end(), n), cur.end());
                }
                o["Groups"] = cur;
            }
        }
    }
    setUnsavedChanges(true);
    on_groupsListWidget_itemSelectionChanged();
}

// =================================================================
// HELPERS & MENUS
// =================================================================

void MainWindow::logMessage(const QString& msg) {
    if(msg.contains("[Error]") || msg.contains("[Fatal]")) spdlog::error(msg.toStdString());
    else if(msg.contains("[Warn]")) spdlog::warn(msg.toStdString());
    else spdlog::info(msg.toStdString());
}

void MainWindow::onLogReceived(const QString& msg, const QString& level)
{
    if (!ui->logTable) return;
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString pid = QString::number(QCoreApplication::applicationPid());
    QString threadId = QString::number((quint64)QThread::currentThreadId());
    QColor textColor = QColor(Qt::white);
    if (level == "ERROR") textColor = QColor("#FF5252");
    else if (level == "WARN") textColor = QColor("#FFD740");

    int row = ui->logTable->rowCount();
    ui->logTable->insertRow(row);
    auto createItem = [&](const QString& text) {
        QTableWidgetItem* item = new QTableWidgetItem(text);
        item->setForeground(textColor);
        return item;
    };
    ui->logTable->setItem(row, 0, createItem(timestamp));
    ui->logTable->setItem(row, 1, createItem(pid));
    ui->logTable->setItem(row, 2, createItem(threadId));
    ui->logTable->setItem(row, 3, createItem(level));
    ui->logTable->setItem(row, 4, createItem(msg));
    ui->logTable->scrollToBottom();
}

void MainWindow::initIcons()
{
    auto createIcon = [](const QColor& color) -> QIcon {
        QPixmap pix(14, 14); pix.fill(Qt::transparent); QPainter painter(&pix);
        painter.setBrush(QBrush(color)); painter.setPen(Qt::NoPen); painter.drawRect(0, 0, 14, 14);
        QPen pen(Qt::white); pen.setWidth(2); painter.setPen(pen); painter.drawRect(1, 1, 12, 12);
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
    ui->mainObjectTable->setSortingEnabled(true);
    configTable(ui->mainObjectTable);
    configTable(ui->setsViewTable);
    configTable(ui->groupsViewTable);
}

void MainWindow::setupLogTable()
{
    QStringList headers = {"Time", "PID", "Thread", "Level", "Message"};
    ui->logTable->setColumnCount(headers.size());
    ui->logTable->setHorizontalHeaderLabels(headers);
    ui->logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->logTable->verticalHeader()->setVisible(false);
    ui->logTable->setColumnWidth(0, 140);
    ui->logTable->setColumnWidth(1, 60);
    ui->logTable->setColumnWidth(2, 80);
    ui->logTable->setColumnWidth(3, 80);
    ui->logTable->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (!dbManager) return;
    // En las pestañas 1 y 2, los ListWidgets ya se mantienen actualizados en memoria
    // Solo forzamos la vista de tabla para reflejar cambios
    if (index == 0) {
        refreshMainTable();
    } else if (index == 1) {
        on_setsListWidget_itemSelectionChanged();
    } else if (index == 2) {
        on_groupsListWidget_itemSelectionChanged();
    }
}

// --- SELECCIONES & PANELES ---

void MainWindow::on_mainObjectTable_selectionChanged()
{
    auto sel = ui->mainObjectTable->selectionModel()->selectedRows();
    if (sel.size() != 1) {
        ui->setsDetailImageLabel->clear();
        ui->setsDetailNorad->clear();
        ui->setsDetailName->clear();
        return;
    }

    int row = sel.first().row();
    int idCol = g_tableHeaders.indexOf("NORAD");
    int64_t id = ui->mainObjectTable->item(row, idCol)->text().toLongLong();

    // Buscar en MEMORIA
    nlohmann::json obj;
    for(const auto& o : m_localCache) if(o["_id"] == id) { obj = o; break; }
    if(obj.empty()) return;

    auto getStr = [&](const std::string& key) {
        if (obj.contains(key) && !obj[key].is_null() && obj[key].is_string())
            return QString::fromStdString(obj[key].get<std::string>());
        return QString("-");
    };
    auto getIntSafe = [&](const std::string& key, int def) -> int {
        if (obj.contains(key) && !obj[key].is_null() && obj[key].is_number()) return obj[key].get<int>();
        return def;
    };

    ui->setsDetailNorad->setText(QString::number(id));
    ui->setsDetailName->setText(getStr("Name"));
    ui->setsDetailAlias->setText(getStr("Alias"));
    ui->setsDetailComments->setText(getStr("Comments"));

    if (obj.contains("Altitude") && obj["Altitude"].is_number())
        ui->setsDetailAltitude->setText(QString::number(obj["Altitude"].get<double>()));
    else ui->setsDetailAltitude->setText("-");

    int lrrVal = getIntSafe("LaserRetroReflector", -1);
    if (lrrVal == 1) ui->setsDetailLrr->setText("Yes");
    else if (lrrVal == 0) ui->setsDetailLrr->setText("No");
    else ui->setsDetailLrr->setText("Unknown");

    // Imagen: Prioridad temporal (la que acabamos de cargar pero no guardado)
    std::string picName = "";
    if (obj.contains("Picture") && !obj["Picture"].is_null()) picName = obj["Picture"];

    ui->setsDetailImageLabel->clear();

    if (obj.contains("_tempLocalImgPath")) {
        QPixmap pix;
        if(pix.load(QString::fromStdString(obj["_tempLocalImgPath"]))) {
            ui->setsDetailImageLabel->setPixmap(pix.scaled(200, 200, Qt::KeepAspectRatio));
            return;
        }
    }

    // Si no es temporal, intentamos descargarla de DB
    if (!picName.empty()) {
        std::string imgData = dbManager->getImageManager().downloadImageByName(picName);
        QPixmap pix;
        if (pix.loadFromData(QByteArray(imgData.data(), imgData.length())))
            ui->setsDetailImageLabel->setPixmap(pix.scaled(200, 200, Qt::KeepAspectRatio));
        else ui->setsDetailImageLabel->setText("Image Error");
    } else {
        ui->setsDetailImageLabel->setText("No Image");
    }
}

// --- REFRESH LISTAS AUXILIARES ---

void MainWindow::refreshSetListWidget()
{
    // Leer de MEMORIA (std::set m_localSets)
    ui->setsListWidget->clear();
    for(const auto& s : m_localSets) {
        ui->setsListWidget->addItem(QString::fromStdString(s));
    }
}

void MainWindow::refreshGroupListWidget()
{
    // Leer de MEMORIA (std::set m_localGroups)
    ui->groupsListWidget->clear();
    for(const auto& g : m_localGroups) {
        ui->groupsListWidget->addItem(QString::fromStdString(g));
    }
}

void MainWindow::on_setsListWidget_itemSelectionChanged()
{
    if (ui->setsListWidget->selectedItems().isEmpty()) {
        populateReadOnlyTable(ui->setsViewTable, m_localCache); // Mostrar todo si vacio
        return;
    }
    std::set<std::string> sets;
    for(auto i : ui->setsListWidget->selectedItems()) sets.insert(i->text().toStdString());

    // Filtro en Memoria
    std::vector<nlohmann::json> filtered;
    for(const auto& obj : m_localCache) {
        if(obj.contains("Sets") && obj["Sets"].is_array()) {
            for(const auto& s : obj["Sets"]) {
                if(sets.count(s.get<std::string>())) { filtered.push_back(obj); break; }
            }
        }
    }
    populateReadOnlyTable(ui->setsViewTable, filtered);
}

void MainWindow::on_groupsListWidget_itemSelectionChanged()
{
    if (ui->groupsListWidget->selectedItems().isEmpty()) {
        populateReadOnlyTable(ui->groupsViewTable, m_localCache); // Todo
        return;
    }
    std::set<std::string> groups;
    for(auto i : ui->groupsListWidget->selectedItems()) groups.insert(i->text().toStdString());

    // Filter MEMORY
    std::vector<nlohmann::json> filtered;
    for(const auto& obj : m_localCache) {
        if(obj.contains("Groups") && obj["Groups"].is_array()) {
            for(const auto& g : obj["Groups"]) {
                if(groups.count(g.get<std::string>())) { filtered.push_back(obj); break; }
            }
        }
    }
    populateReadOnlyTable(ui->groupsViewTable, filtered);
}


// --- CREAR/BORRAR SETS/GRUPOS (Metadata en Memoria) ---

void MainWindow::on_createSetButton_clicked() {
    QString name = ui->newSetLineEdit->text().trimmed();
    if(name.isEmpty()) return;

    // Validar si existe en local
    if (m_localSets.count(name.toStdString())) {
        QMessageBox::warning(this, "Error", "Set already exists locally.");
        return;
    }

    // Añadir a memoria
    m_localSets.insert(name.toStdString());
    refreshSetListWidget();
    ui->newSetLineEdit->clear();
    setUnsavedChanges(true);
    logMessage("[Memory] Set created: " + name);
}

void MainWindow::on_deleteSetButton_clicked() {
    if(ui->setsListWidget->selectedItems().isEmpty()) return;
    QString name = ui->setsListWidget->currentItem()->text();

    if(QMessageBox::Yes != QMessageBox::question(this, "Delete", "Delete Set '" + name + "' locally?")) return;

    // 1. Borrar de la lista de Sets
    m_localSets.erase(name.toStdString());

    // 2. Borrar de TODOS los objetos en memoria (Cascada)
    for(auto& obj : m_localCache) {
        if(obj.contains("Sets") && obj["Sets"].is_array()) {
            std::vector<std::string> current = obj["Sets"];
            auto it = std::remove(current.begin(), current.end(), name.toStdString());
            if (it != current.end()) {
                current.erase(it, current.end());
                obj["Sets"] = current;
            }
        }
    }

    refreshSetListWidget();
    ui->setsViewTable->setRowCount(0);
    refreshMainTable(); // Actualizar columna Sets en tabla principal
    setUnsavedChanges(true);
    logMessage("[Memory] Set deleted: " + name);
}

void MainWindow::on_createGroupButton_clicked()
{
    QString name = ui->newGroupLineEdit->text().trimmed();

    if(name.isEmpty()) {
        QMessageBox::warning(this, "Error", "Group name cannot be empty.");
        return;
    }

    // 1. Validar si ya existe en memoria
    if (m_localGroups.count(name.toStdString())) {
        QMessageBox::warning(this, "Error", "Group already exists locally.");
        return;
    }

    // 2. Añadir a memoria
    m_localGroups.insert(name.toStdString());

    // 3. Actualizar interfaz y estado
    refreshGroupListWidget();
    ui->newGroupLineEdit->clear();
    setUnsavedChanges(true);
    logMessage("[Memory] Group created: " + name);
}

void MainWindow::on_deleteGroupButton_clicked()
{
    if(ui->groupsListWidget->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a Group to delete.");
        return;
    }

    QString name = ui->groupsListWidget->currentItem()->text();

    if(QMessageBox::Yes != QMessageBox::question(this, "Delete",
                                                  "Delete Group '" + name + "' locally?\n\nThis will remove it from ALL objects in memory."))
    {
        return;
    }

    std::string stdName = name.toStdString();

    // 1. Borrar de la lista de definiciones de Grupos (en Memoria)
    m_localGroups.erase(stdName);

    // 2. Borrado en Cascada: Quitar este grupo de TODOS los objetos en memoria
    int objectsAffected = 0;
    for(auto& obj : m_localCache) {
        if(obj.contains("Groups") && obj["Groups"].is_array()) {
            std::vector<std::string> current = obj["Groups"];
            auto it = std::remove(current.begin(), current.end(), stdName);
            if (it != current.end()) {
                current.erase(it, current.end());
                obj["Groups"] = current;
                objectsAffected++;
            }
        }
    }

    refreshGroupListWidget();
    ui->groupsViewTable->setRowCount(0);
    refreshMainTable();

    setUnsavedChanges(true);
    logMessage("[Memory] Group deleted: " + name + " (Removed from " + QString::number(objectsAffected) + " objects)");
}


// --- RESTO DE HELPERS (Log, Setup, InitIcons, Populate) ---

void MainWindow::populateMainTable(const std::vector<nlohmann::json>& objects)
{
    ui->mainObjectTable->setSortingEnabled(false);
    ui->mainObjectTable->setUpdatesEnabled(false);
    ui->mainObjectTable->setRowCount(0);
    int colEn = g_tableHeaders.indexOf("EnablementPolicy");
    int countTotal = objects.size();
    int countEnabled = 0;

    for (const auto& obj : objects) {
        if (!obj.contains("_id")) continue;
        if (colEn != -1 && obj.contains("EnablementPolicy") && !obj["EnablementPolicy"].is_null())
            if (obj["EnablementPolicy"].get<int>() == 1) countEnabled++;

        int row = ui->mainObjectTable->rowCount();
        ui->mainObjectTable->insertRow(row);
        for (int col = 0; col < g_tableHeaders.size(); ++col) {
            std::string key = g_tableHeaders[col].toStdString();
            QTableWidgetItem *item = new QTableWidgetItem();
            if (col == colEn) {
                int val = -1;
                if(obj.contains(key) && !obj[key].is_null() && obj[key].is_number()) val = obj[key].get<int>();
                if(val==1) { item->setIcon(m_iconGreen); item->setText("Enabled"); }
                else if(val==2) { item->setIcon(m_iconRed); item->setText("Disabled"); }
                else { item->setIcon(m_iconGray); item->setText("Unknown"); }
            }
            else if (key == "Sets" || key == "Groups") {
                if (obj.contains(key) && obj[key].is_array()) {
                    QStringList l;
                    for(const auto& s : obj[key]) if(s.is_string()) l << QString::fromStdString(s.get<std::string>());
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
    if (ui->lblCountTotal)   ui->lblCountTotal->setText("Total (Memory): " + QString::number(countTotal));
    if (ui->lblCountVisible) ui->lblCountVisible->setText("Visible: " + QString::number(countTotal));
    if (ui->lblCountEnabled) ui->lblCountEnabled->setText("Enabled: " + QString::number(countEnabled));
}

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
                if(obj.contains(k) && !obj[k].is_null() && obj[k].is_number()) val = obj[k].get<int>();
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

void MainWindow::handleUniversalContextMenu(const QPoint &pos, QTableWidget* table) {
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
            for(const auto& o : m_localCache) if(o["_id"] == id) { jArr.push_back(o); break; }
        }
        QGuiApplication::clipboard()->setText(QString::fromStdString(jArr.dump(2)));
        logMessage("JSON copied.");
    }
    else if (act == assignSet) {
        // Dialogo de selección de Sets (DESDE MEMORIA m_localSets)
        QDialog dlg(this); dlg.setWindowTitle("Select Sets (Memory)");
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        QListWidget *lw = new QListWidget(&dlg); lw->setSelectionMode(QAbstractItemView::MultiSelection);

        // Cargar sets desde memoria
        for(const auto& s : m_localSets) lw->addItem(QString::fromStdString(s));

        l->addWidget(lw);
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        l->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if(dlg.exec() == QDialog::Accepted) {
            for(const auto& row : selectedRows) {
                int64_t id = table->item(row.row(), idCol)->text().toLongLong();
                for(auto& o : m_localCache) {
                    if(o["_id"] == id) {
                        std::vector<std::string> cur = o.value("Sets", std::vector<std::string>());
                        for(auto s : lw->selectedItems()) {
                            std::string n = s->text().toStdString();
                            if(std::find(cur.begin(), cur.end(), n) == cur.end()) cur.push_back(n);
                        }
                        o["Sets"] = cur;
                    }
                }
            }
            setUnsavedChanges(true);
            refreshMainTable();
            on_setsListWidget_itemSelectionChanged();
        }
    }
    else if (act == assignGroup) {
        // Dialogo de selección de Groups (DESDE MEMORIA m_localGroups)
        QDialog dlg(this); dlg.setWindowTitle("Select Groups (Memory)");
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        QListWidget *lw = new QListWidget(&dlg); lw->setSelectionMode(QAbstractItemView::MultiSelection);

        for(const auto& g : m_localGroups) lw->addItem(QString::fromStdString(g));

        l->addWidget(lw);
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        l->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if(dlg.exec() == QDialog::Accepted) {
            for(const auto& row : selectedRows) {
                int64_t id = table->item(row.row(), idCol)->text().toLongLong();
                for(auto& o : m_localCache) {
                    if(o["_id"] == id) {
                        std::vector<std::string> cur = o.value("Groups", std::vector<std::string>());
                        for(auto g : lw->selectedItems()) {
                            std::string n = g->text().toStdString();
                            if(std::find(cur.begin(), cur.end(), n) == cur.end()) cur.push_back(n);
                        }
                        o["Groups"] = cur;
                    }
                }
            }
            setUnsavedChanges(true);
            refreshMainTable();
            on_groupsListWidget_itemSelectionChanged();
        }
    }
}

void MainWindow::on_searchLineEdit_textChanged(const QString &text) {
    QRegularExpression regex(text, QRegularExpression::CaseInsensitiveOption);
    if (!regex.isValid()) return;
    int index = ui->tabWidget->currentIndex();
    QTableWidget* t = (index==0)?ui->mainObjectTable : (index==1)?ui->setsViewTable : ui->groupsViewTable;
    int visibleCount = 0;
    for(int i=0; i<t->rowCount(); ++i) {
        bool m = false;
        if(text.isEmpty()) m=true;
        else for(int j=1; j<5; ++j) if(t->item(i,j)->text().contains(regex)) { m=true; break; }
        t->setRowHidden(i, !m);
        if(m) visibleCount++;
    }
    if(ui->lblCountVisible) ui->lblCountVisible->setText("Visible: " + QString::number(visibleCount));
}

void MainWindow::exportToCSV() {
    QString fn = QFileDialog::getSaveFileName(this, "Export", QDir::homePath(), "CSV (*.csv)");
    if(fn.isEmpty()) return;
    QFile f(fn); if(!f.open(QIODevice::WriteOnly)) return;
    QTextStream ts(&f);
    for(int i=0; i<ui->mainObjectTable->columnCount(); ++i) ts << ui->mainObjectTable->horizontalHeaderItem(i)->text() << ";";
    ts << "\n";
    for(int i=0; i<ui->mainObjectTable->rowCount(); ++i) {
        for(int j=0; j<ui->mainObjectTable->columnCount(); ++j) {
            QTableWidgetItem* it = ui->mainObjectTable->item(i,j);
            QString t = it ? it->text() : "";
            if(t.contains(";") || t.contains("\n")) t = "\"" + t + "\"";
            ts << t << ";";
        }
        ts << "\n";
    }
    f.close();
    logMessage("Exported.");
}

void MainWindow::importFromJSON() {
    QString fn = QFileDialog::getOpenFileName(this, "Import", QDir::homePath(), "JSON (*.json)");
    if(fn.isEmpty()) return;
    QFile f(fn); if(!f.open(QIODevice::ReadOnly)) return;
    try {
        auto j = nlohmann::json::parse(f.readAll());
        std::vector<nlohmann::json> list;
        if(j.is_array()) for(const auto& o : j) list.push_back(o); else list.push_back(j);

        QDir dir = QFileInfo(fn).absoluteDir();
        int ok=0;
        for(auto& obj : list) {
            // Check dup in memory
            int64_t id = obj.value("_id", -1LL);
            auto it = std::find_if(m_localCache.begin(), m_localCache.end(), [&](const auto& o){ return o["_id"] == id; });
            if(it != m_localCache.end()) continue;

            // Temp image path
            if(obj.contains("Picture") && !obj["Picture"].is_null()) {
                QString p = dir.filePath(QString::fromStdString(obj["Picture"]));
                if(QFile::exists(p)) obj["_tempLocalImgPath"] = p.toStdString();
            }
            m_localCache.push_back(obj);
            ok++;
        }
        setUnsavedChanges(true);
        refreshMainTable();
        logMessage("Imported " + QString::number(ok) + " objects to memory.");
    } catch (...) {}
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if(ui->mainObjectTable->hasFocus()) {
        if(event->key() == Qt::Key_Delete) on_deleteObjectSetButton_clicked();
        else if(event->key() == Qt::Key_Enter) on_editObjectButton_clicked();
    }
    QMainWindow::keyPressEvent(event);
}
