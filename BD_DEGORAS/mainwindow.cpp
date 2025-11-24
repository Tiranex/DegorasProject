#include "mainwindow.h"
#include "ui_mainwindow.h"      // Archivo generado por el .ui
#include "SpaceObjectDBManager.h" // Nuestra clase de BBDD
#include "json_helpers.h"       // Nuestras funciones de JSON
#include "addobjectdialog.h"

// Includes de Qt para diálogos, archivos, etc.
#include <QMessageBox> // Para el diálogo de confirmación
#include <QFileDialog> // Para el diálogo "Examinar..."
#include <QDir>        // Para crear/comprobar carpetas
#include <QFile>       // Para copiar archivos
#include <QDebug>      // Para logs de depuración
#include <QPixmap>     // Para mostrar imágenes
#include <QByteArray>  // Para manejar datos binarios
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QInputDialog> // Faltaba este include para QInputDialog

#include <string>
#include <memory>      // Para std::make_unique
#include <vector>
#include <set>         // ¡NUEVO! Para std::set

#include <QGuiApplication> // Para QGuiApplication
#include <QClipboard>      // Para QGuiApplication::clipboard()
#include <QMenu>           // Para QMenu


// Lista completa de cabeceras basada en tu imagen
// NOTA: Estas strings son claves de la BBDD, no se deben traducir o la lógica fallará.
const QStringList g_tableHeaders = {
    "_id", "Name", "Altitude", "Amplification", "BinSize",
    "COSPAR", "Classification", "CounterID", "DetectorID",
    "EnablementPolicy", "ILRSID", "ILRSName", "Inclination",
    "IsDebris", "LaserID", "LaserRetroReflector", "NORAD","Abbreviation" ,
    "NormalPointIndicator", "Picture", "Priority", "ProviderCPF",
    "RadarCrossSection", "SIC", "TrackPolicy"
};

// --- CONSTRUCTOR (¡MODIFICADO!) ---
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //connect(ui->addNewObjectSetButton, &QPushButton::clicked, this, &MainWindow::on_addNewObjectSetButton_clicked);
    // 1. Configurar ambas tablas
    setupObjectListTable(); // Pestaña "Listado"
    setupSetsObjectTable(); // Pestaña "Sets"

    // Conectamos el doble clic en una celda con el botón de "Editar"
    connect(ui->setsObjectTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::on_editObjectButton_clicked);
    // 2. CLIC DERECHO -> MENÚ CONTEXTUAL (Sets)
    connect(ui->setsObjectTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onSetsTableContextMenuRequested);
    // Conexión del botón AÑADIR (La que hiciste antes)
    connect(ui->addNewObjectSetButton, &QPushButton::clicked, this, &MainWindow::on_addNewObjectSetButton_clicked);
    // ¡NUEVO! Conexión del botón EDITAR
    connect(ui->editObjectButton, &QPushButton::clicked, this, &MainWindow::on_editObjectButton_clicked);

    connect(ui->setsObjectTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::on_setsObjectTable_selectionChanged);



    // --- 2. Conectar a la BBDD ---
    const std::string URI = "mongodb://localhost:27017";
    const std::string DB_NAME = "DegorasDB";
    const std::string COLLECTION_NAME = "space_objects";

    try {
        dbManager = std::make_unique<SpaceObjectDBManager>(URI, DB_NAME, COLLECTION_NAME);
        logMessage("[Info] Successfully connected to " + QString::fromStdString(DB_NAME) + "/" + QString::fromStdString(COLLECTION_NAME));
    }
    catch (const std::exception& e)
    {
        logMessage("[Fatal Error] Could not connect to MongoDB: " + QString(e.what()));
        // Deshabilitar todo si falla la BBDD
        ui->mostrarButton->setEnabled(false);
        ui->eliminarButton->setEnabled(false);
        ui->anadirButton->setEnabled(false);
        ui->browseButton->setEnabled(false);
        ui->idLineEdit->setEnabled(false);
        // ¡NUEVO! Deshabilitar también la pestaña "Sets"
        ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tab_3), false);
    }

    // Si la conexión fue exitosa (dbManager no es null)
    if (dbManager) {
        // ¡NUEVO! Cargar la lista de grupos al iniciar
        refreshGroupList();
    }

    // --- 3. Configuración Pestaña 1 ---
    ui->searchByIdRadioButton->setChecked(true);
    ui->imageDisplayLabel->clear();

    // --- 4. Conexiones de Slots ---

    // Pestaña "Listado"
    ui->objectListTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->objectListTable, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::onObjectListTableContextMenuRequested);
    // (on_refreshListButton_clicked se auto-conecta)

    // Pestaña "Sets"
    // ¡CORRECCIÓN! El connect para refreshListButton_2 se elimina
    // Qt lo auto-conectará a 'on_refreshListButton_2_clicked()'

    // 5. Cargar la lista "Listado" una vez al iniciar
    on_refreshListButton_clicked();
}

// --- DESTRUCTOR ---
MainWindow::~MainWindow()
{
    delete ui;
}

// --- FUNCIÓN DE AYUDA PARA LOGS ---
void MainWindow::logMessage(const QString& msg)
{
    ui->resultsTextEdit->append(msg);
    ui->resultsTextEdit->moveCursor(QTextCursor::End);
}


// --- ¡FUNCIÓN 'setup' PESTAÑA "LISTADO"! ---
void MainWindow::setupObjectListTable()
{
    ui->objectListTable->setColumnCount(g_tableHeaders.size());
    ui->objectListTable->setHorizontalHeaderLabels(g_tableHeaders);
    ui->objectListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->objectListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->objectListTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}


// =================================================================
// --- SLOTS (Pestaña 1 y 2 - SIN CAMBIOS, EXCEPTO 'on_anadir') ---
// =================================================================

// ... (on_mostrarButton_clicked - Sin cambios) ...
/**
 * @brief Slot que se ejecuta al pulsar el botón "Mostrar".
 */
void MainWindow::on_mostrarButton_clicked()
{
    if (!dbManager) return;

    // 1. LIMPIEZA TOTAL DE LA INTERFAZ
    ui->resultsTextEdit->clear();
    ui->imageDisplayLabel->clear();

    // Limpiamos del 1 al 6
    ui->mostrar_texto_1->clear();
    ui->mostrar_texto_2->clear();
    ui->mostrar_texto_3->clear();
    ui->mostrar_texto_4->clear();
    ui->mostrar_texto_5->clear();
    ui->mostrar_texto_6->clear();

    logMessage("[Info] Searching...");

    std::string searchText = ui->idLineEdit->text().toStdString();
    nlohmann::json obj;
    bool searchedById = ui->searchByIdRadioButton->isChecked(); // Guardamos qué modo usamos

    // 2. BÚSQUEDA EN BASE DE DATOS
    try
    {
        if (searchedById) // Búsqueda por ID
        {
            if (searchText.empty()) {
                logMessage("[Error] The _id cannot be empty.");
                return;
            }
            int64_t id = std::stoll(searchText);
            obj = dbManager->getSpaceObjectById(id);
        }
        else // Búsqueda por NOMBRE
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

    // 3. VERIFICACIÓN
    if (obj.empty() || obj.is_null()) {
        logMessage("[Info] Object not found.");
        ui->mostrar_texto_1->setText("Status: Not found");
        return;
    }

    logMessage("[OK] Object found.");

    // =========================================================
    // 4. ACTUALIZACIÓN DE LA INTERFAZ
    // =========================================================

    // --- CAJA 1: LÓGICA DINÁMICA (NOMBRE vs ID) ---
    if (searchedById)
    {
        // Si busqué por ID -> Muestro el NOMBRE
        if (obj.contains("Name")) {
            ui->mostrar_texto_1->setText("Name: " + jsonValueToQString(obj["Name"]));
        } else {
            ui->mostrar_texto_1->setText("Name: Unknown");
        }
    }
    else
    {
        // Si busqué por NOMBRE -> Muestro el ID
        if (obj.contains("_id")) {
            ui->mostrar_texto_1->setText("ID: " + jsonValueToQString(obj["_id"]));
        } else {
            ui->mostrar_texto_1->setText("ID: Error");
        }
    }

    // --- CAJA 2: ALTITUD ---
    if (obj.contains("Altitude")) {
        ui->mostrar_texto_2->setText("Altitude: " + jsonValueToQString(obj["Altitude"]) + " km");
    } else {
        ui->mostrar_texto_2->setText("Altitude: -");
    }

    // --- CAJA 3: INCLINACIÓN ---
    if (obj.contains("Inclination")) {
        ui->mostrar_texto_3->setText("Inclination: " + jsonValueToQString(obj["Inclination"]) + "°");
    } else {
        ui->mostrar_texto_3->setText("Inclination: -");
    }

    // --- CAJA 4: LASER ID ---
    if (obj.contains("LaserID")) {
        ui->mostrar_texto_4->setText("Laser ID: " + jsonValueToQString(obj["LaserID"]));
    } else {
        ui->mostrar_texto_4->setText("Laser ID: N/A");
    }

    // --- CAJA 5: LASER RETRO REFLECTOR ---
    if (obj.contains("LaserRetroReflector")) {
        // Obtenemos el valor (suponiendo que es 1 o 0 en el JSON)
        QString lrr = jsonValueToQString(obj["LaserRetroReflector"]);
        ui->mostrar_texto_5->setText("RetroReflector: " + lrr);
    } else {
        ui->mostrar_texto_5->setText("RetroReflector: -");
    }

    // --- CAJA 6: RADAR CROSS SECTION (RCS) ---
    if (obj.contains("RadarCrossSection")) {
        // En los satélites, esto suele medirse en metros cuadrados (m²)
        ui->mostrar_texto_6->setText("RCS: " + jsonValueToQString(obj["RadarCrossSection"]) + " m²");
    } else {
        ui->mostrar_texto_6->setText("RCS: -");
    }

    // =========================================================
    // 5. GESTIÓN DE LA IMAGEN (CORREGIDO)
    // =========================================================

    // Variable para guardar el nombre si lo encontramos
    std::string picName = "";

    // 1. Comprobamos si el JSON tiene el campo "Picture"
    if (obj.contains("Picture")) {
        // Si existe, cogemos el valor (aunque sea una cadena vacía)
        picName = obj["Picture"].get<std::string>();
    }

    // 2. Si NO hay campo Picture O el nombre está vacío (""), avisamos
    if (picName.empty() || picName == "\"\"") {
        logMessage("[Info] This object has no assigned image (No photo).");
        // Opcional: Poner un texto en el hueco de la foto para que se note
        ui->imageDisplayLabel->setText("NO PHOTO");
        return; // Salimos, no hay nada que descargar
    }

    // 3. Si llegamos aquí, es que hay un nombre de archivo (ej: "AJISAI.jpg")
    // Intentamos descargarla de GridFS
    std::string imageDataStd = dbManager->getImageManager().downloadImageByName(picName);

    if (imageDataStd.empty()) {
        // Caso: El JSON dice que hay foto, pero el archivo no está en la base de datos
        logMessage("[Warning] The object requests photo '" + QString::fromStdString(picName) + "', but it was not found in GridFS.");
        ui->imageDisplayLabel->setText("File not found");
    }
    else {
        // Caso: Éxito total
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

/**
 * @brief Se ejecuta al pulsar el botón "Añadir" (Pestaña antigua).
 */
void MainWindow::on_anadirButton_clicked()
{
    if (!dbManager) return;

    // 1. Recoger datos
    std::string id_str = ui->idLineEdit_add->text().toStdString();
    std::string norad_str = ui->noradLineEdit_add->text().toStdString();
    std::string name_str = ui->nameLineEdit_add->text().toStdString();
    std::string class_str = ui->classLineEdit_add->text().toStdString();
    std::string pic_str = ui->pictureLineEdit_add->text().toStdString();

    // 2. Validar
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
        logMessage("[Error] The name is required to create an object.");
        return;
    }
    if (norad_str.empty()) {
        norad_str = id_str;
        logMessage("[Info] NORAD field empty. Using _id as NORAD.");
    }

    // 3. Crear el objeto JSON
    nlohmann::json newObj = {
        {"_id", id},
        {"NORAD", norad_str},
        {"Name", name_str},
        {"Classification", class_str},
        {"Picture", pic_str},
        {"Groups", nlohmann::json::array()}
    };

    logMessage("[Info] Attempting to create object with _id: " + QString::fromStdString(id_str) + "...");

    // --- CORRECCIÓN AQUÍ ---
    std::string errorMsg; // 1. Creamos la variable para recibir el error

    // 2. La pasamos como tercer argumento
    if (dbManager->createSpaceObject(newObj, m_localPicturePath.toStdString(), errorMsg))
    {
        logMessage("[OK] Object successfully created!");

        // Limpiar campos
        ui->idLineEdit_add->clear();
        ui->noradLineEdit_add->clear();
        ui->nameLineEdit_add->clear();
        ui->classLineEdit_add->clear();
        ui->pictureLineEdit_add->clear();
        m_localPicturePath.clear();

        refreshGroupList();

    } else {
        // 3. Mostramos el error específico que nos devuelve la BBDD
        logMessage("[Error] Could not create object: " + QString::fromStdString(errorMsg));
    }
}

// ... (on_eliminarButton_clicked - Sin cambios) ...
void MainWindow::on_eliminarButton_clicked()
{
    if (!dbManager) return;

    if (ui->searchByNameRadioButton->isChecked()) {
        logMessage("[Error] Cannot delete by 'Name'.");
        logMessage("[Info] Please search the object by '_id' to delete it.");
        return;
    }

    std::string id_str = ui->idLineEdit->text().toStdString();
    if (id_str.empty()) {
        logMessage("[Error] Enter an _id in the search field to delete.");
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
        reply = QMessageBox::question(this, "Confirm deletion",
                                      "Are you sure you want to delete the object with _id: " + QString::fromStdString(id_str) + "?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            logMessage("[Info] Deletion cancelled by user.");
            return;
        }

        logMessage("[Info] Deleting object with _id: " + QString::fromStdString(id_str) + "...");

        if (dbManager->deleteSpaceObjectById(id)) {
            logMessage("[OK] Object successfully deleted.");
            ui->idLineEdit->clear();
            ui->imageDisplayLabel->clear();

            if (!picName.empty()) {
                logMessage("[Info] Deleting associated image '" + QString::fromStdString(picName) + "' from GridFS...");
                if (dbManager->getImageManager().deleteImageByName(picName)) {
                    logMessage("[OK] GridFS image deleted.");
                } else {
                    logMessage("[Warn] Could not delete image from GridFS.");
                }
            }
        } else {
            logMessage("[Error] Could not delete the object.");
        }
    }
    catch (...) {
        logMessage("[Error] The entered _id is not a valid number.");
    }
}


// ... (on_browseButton_clicked - Sin cambios) ...
void MainWindow::on_browseButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Image",
        QDir::homePath(),
        "Images (*.png *.jpg *.jpeg *.bmp)"
        );

    if (filePath.isEmpty()) {
        m_localPicturePath.clear();
        return;
    }

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    m_localPicturePath = filePath;
    ui->pictureLineEdit_add->setText(fileName);
}


// ... (on_refreshListButton_clicked - Pestaña "Listado" - Sin cambios) ...
void MainWindow::on_refreshListButton_clicked()
{
    if (!dbManager) {
        logMessage("[Error] Cannot refresh list, no DB connection.");
        return;
    }

    logMessage("[Info] Refreshing object list from DB...");
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
            if (obj.contains(key)) {
                value = obj[key];
            }
            QString stringValue = jsonValueToQString(value);
            QTableWidgetItem *item = new QTableWidgetItem(stringValue);
            ui->objectListTable->setItem(rowCount, col, item);
        }
    }

    ui->objectListTable->setUpdatesEnabled(true);
    ui->objectListTable->setSortingEnabled(true);
    //Tambien refrescamos los grupos
    refreshGroupList();
    logMessage("[Info] List updated. " + QString::number(allObjects.size()) + " objects loaded.");
}

// ... (onObjectListTableContextMenuRequested - Pestaña "Listado" - Sin cambios) ...
void MainWindow::onObjectListTableContextMenuRequested(const QPoint &pos)
{
    QTableWidgetItem *item = ui->objectListTable->itemAt(pos);
    if (!item) {
        return;
    }

    int row = item->row();
    QMenu contextMenu(this);
    QAction *copyJsonAction = contextMenu.addAction("Copy JSON");
    QAction *selectedAction = contextMenu.exec(ui->objectListTable->mapToGlobal(pos));

    if (selectedAction == copyJsonAction)
    {
        QTableWidgetItem* idItem = ui->objectListTable->item(row, 0);
        if (!idItem) {
            logMessage("[Error] Could not find item _id for the selected row.");
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
            std::string jsonString = obj.dump(2);
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(jsonString));
            logMessage("[Info] JSON of object " + idString + " copied to clipboard.");
        } catch (const std::exception& e) {
            logMessage("[Error] The _id '" + idString + "' is not a valid number.");
        }
    }
}


// =================================================================
// --- ¡NUEVO! SLOTS Y FUNCIONES PARA LA PESTAÑA "SETS" (`tab_3`) ---
// =================================================================

/**
 * @brief Configura la tabla 'setsObjectTable'
 */
void MainWindow::setupSetsObjectTable()
{

    // 1. Preparamos las cabeceras: Todas las normales + "Groups"
    QStringList headers = g_tableHeaders;
    headers << "Groups"; // Añadimos la columna extra al final

    // 2. Configurar columnas
    ui->setsObjectTable->setColumnCount(headers.size());
    ui->setsObjectTable->setHorizontalHeaderLabels(headers);

    // 3. Propiedades visuales
    ui->setsObjectTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Solo lectura
    ui->setsObjectTable->setSelectionBehavior(QAbstractItemView::SelectRows); // Seleccionar fila entera
    ui->setsObjectTable->setSelectionMode(QAbstractItemView::ExtendedSelection); // Permitir selección múltiple (útil para asignar grupos)

    // 4. Habilitar menú contextual (Clic derecho)
    ui->setsObjectTable->setContextMenuPolicy(Qt::CustomContextMenu);
}

/**
 * @brief Pide a la BBDD todos los grupos y rellena 'listWidget'.
 */
void MainWindow::refreshGroupList()
{
    if (!dbManager) return;

    // Guardar el grupo seleccionado actualmente, si hay uno
    QString currentGroup;
    if (ui->listWidget->currentItem()) {
        currentGroup = ui->listWidget->currentItem()->text();
    }

    ui->listWidget->clear();

    // Obtenemos el set de strings de la BBDD
    std::set<std::string> groups = dbManager->getAllUniqueGroupNames();

    QListWidgetItem* itemToSelect = nullptr;
    for (const auto& groupName : groups) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(groupName));
        ui->listWidget->addItem(item);

        // Si este es el que estaba seleccionado, lo guardamos para re-seleccionarlo
        if (QString::fromStdString(groupName) == currentGroup) {
            itemToSelect = item;
        }
    }

    // Re-seleccionar el item
    if (itemToSelect) {
        ui->listWidget->setCurrentItem(itemToSelect);
    }

    logMessage("[Info] Group list updated.");
}

/**
 * @brief Rellena la tabla 'setsObjectTable' con los objetos dados.
 * (¡VERSIÓN SEGURA CORREGIDA!)
 */
void MainWindow::populateSetsObjectTable(const std::vector<nlohmann::json>& objects)
{
    // Desactivar actualizaciones visuales para que vaya rápido
    ui->setsObjectTable->setSortingEnabled(false);
    ui->setsObjectTable->setUpdatesEnabled(false);

    ui->setsObjectTable->setRowCount(0); // Limpiar tabla

    for (const auto& obj : objects) {

        // Seguridad: Si no tiene ID, lo saltamos
        if (!obj.contains("_id")) continue;

        int row = ui->setsObjectTable->rowCount();
        ui->setsObjectTable->insertRow(row);

        // --- 1. Rellenar las columnas normales (basadas en g_tableHeaders) ---
        for (int col = 0; col < g_tableHeaders.size(); ++col) {
            std::string key = g_tableHeaders[col].toStdString();

            // Usamos tu función auxiliar jsonValueToQString
            QString valStr = "";
            if(obj.contains(key)) {
                valStr = jsonValueToQString(obj[key]);
            }

            QTableWidgetItem *item = new QTableWidgetItem(valStr);
            ui->setsObjectTable->setItem(row, col, item);
        }

        // --- 2. Rellenar la columna "Groups" (La última) ---
        int groupCol = g_tableHeaders.size(); // La columna siguiente a la última normal
        QString groupsStr = "";

        if (obj.contains("Groups") && obj["Groups"].is_array()) {
            QStringList groupsList;
            for (const auto& g : obj["Groups"]) {
                if(g.is_string()) {
                    groupsList << QString::fromStdString(g.get<std::string>());
                }
            }
            groupsStr = groupsList.join(", "); // Separados por coma: "Grupo1, Grupo2"
        }

        QTableWidgetItem *groupItem = new QTableWidgetItem(groupsStr);
        ui->setsObjectTable->setItem(row, groupCol, groupItem);
    }

    ui->setsObjectTable->setUpdatesEnabled(true);
    ui->setsObjectTable->setSortingEnabled(true);
}

/**
 * @brief Slot: Pulsar "Crear Grupo"
 */
void MainWindow::on_createGroupButton_clicked()
{
    if (!dbManager) return;

    QString groupName = ui->newGroupLineEdit->text().trimmed(); // .trimmed() quita espacios
    if (groupName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Group name cannot be empty.");
        return;
    }

    if (dbManager->crearGrupo(groupName.toStdString())) {
        logMessage("[OK] Group '" + groupName + "' created.");
        ui->newGroupLineEdit->clear();
        refreshGroupList(); // Actualizar la lista
    } else {
        QMessageBox::warning(this, "Error", "Could not create group.\nIt may already exist.");
    }
}

/**
 * @brief Slot: Pulsar "Eliminar Grupo"
 */
void MainWindow::on_deleteGroupButton_clicked()
{
    if (!dbManager) return;

    // 1. Obtener el grupo seleccionado de la lista
    QListWidgetItem* selectedItem = ui->listWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "Error", "Please select a group from the list to delete.");
        return;
    }

    QString groupName = selectedItem->text();

    // 2. Pedir confirmación
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm deletion",
                                  "Are you sure you want to delete group '" + groupName + "'?\n\nThis will also remove it from ALL objects containing it.",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        logMessage("[Info] Group deletion cancelled.");
        return;
    }

    // 3. Proceder con el borrado
    if (dbManager->eliminarGrupo(groupName.toStdString())) {
        logMessage("[OK] Group '" + groupName + "' deleted.");
        refreshGroupList(); // Actualizar la lista de grupos
        on_refreshListButton_2_clicked(); // Actualizar la tabla de objetos
    } else {
        QMessageBox::warning(this, "Error", "An error occurred while deleting the group.");
    }
}

/**
 * @brief Slot: Pulsar "Refresh" (Pestaña "Sets")
 *
 * ¡VERSIÓN CORREGIDA!
 * Esta función ahora limpia explícitamente las selecciones ANTES de
 * restaurarlas para evitar conflictos con las funciones de refresco.
 */
void MainWindow::on_refreshListButton_2_clicked()
{
    if (!dbManager) return;

    // --- 1. GUARDAR SELECCIÓN ACTUAL ---
    // Guardamos qué grupos estaban seleccionados antes de refrescar
    QList<QListWidgetItem*> selectedGroupsItems = ui->listWidget->selectedItems();
    QSet<QString> selectedGroupNames;
    for (QListWidgetItem* item : selectedGroupsItems) {
        selectedGroupNames.insert(item->text());
    }

    // Guardamos qué objetos estaban seleccionados en la tabla
    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();
    QSet<int64_t> selectedObjectIDs;
    for (const QModelIndex& rowIndex : selectedRowsIndexes) {
        if (ui->setsObjectTable->item(rowIndex.row(), 0)) {
            selectedObjectIDs.insert(ui->setsObjectTable->item(rowIndex.row(), 0)->text().toLongLong());
        }
    }

    // --- 2. CONSULTA A BASE DE DATOS ---
    std::vector<nlohmann::json> objects;

    if (ui->showAllObjectsCheckBox->isChecked())
    {
        objects = dbManager->getAllSpaceObjects();
    }
    else
    {
        // Si "Mostrar todo" está apagado, filtramos por los grupos seleccionados
        std::set<std::string> selectedGroupsForFilter;
        for (const QString& name : selectedGroupNames) {
            selectedGroupsForFilter.insert(name.toStdString());
        }

        if (!selectedGroupsForFilter.empty()) {
            objects = dbManager->getSpaceObjectsByGroups(selectedGroupsForFilter);
        } else {
            // Si no hay grupos seleccionados y el check está apagado, no mostramos nada
            objects.clear();
        }
    }

    // --- 3. REFRESCAR TABLA (Esto es seguro) ---
    populateSetsObjectTable(objects);

    QString infoMsg = "[Info] Table updated. " + QString::number(objects.size()) + " objects visible.";
    if(!ui->showAllObjectsCheckBox->isChecked() && !selectedGroupNames.isEmpty()) {
        infoMsg += " (Filtered by group)";
    }
    logMessage(infoMsg);


    // --- 4. REFRESCAR LISTA DE GRUPOS (¡AQUÍ ESTABA EL CRASH!) ---

    // !!! ACTIVAMOS EL SILENCIADOR (IMPORTANTE) !!!
    // Esto evita que al limpiar la lista se dispare el evento 'itemSelectionChanged' y entremos en bucle.
    ui->listWidget->blockSignals(true);

    refreshGroupList(); // Borra y rellena la lista

    // Restaurar selección de Grupos
    ui->listWidget->clearSelection();
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* item = ui->listWidget->item(i);
        if (selectedGroupNames.contains(item->text())) {
            item->setSelected(true);
        }
    }

    // !!! QUITAMOS EL SILENCIADOR !!!
    // Ahora la lista vuelve a funcionar normal para que el usuario pueda clicar.
    ui->listWidget->blockSignals(false);


    // --- 5. RESTAURAR SELECCIÓN DE TABLA ---
    ui->setsObjectTable->blockSignals(true); // Silenciamos tabla por si acaso
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

/**
 * @brief Slot: Pulsar "Añadir objeto a grupo"
 * (Versión simplificada: la lógica de refresco la hace on_refreshListButton_2_clicked)
 */
void MainWindow::on_assignToGroupButton_clicked()
{
    if (!dbManager) return;

    // 1. Obtener selecciones
    QList<QListWidgetItem*> selectedGroups = ui->listWidget->selectedItems();
    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();

    // 2. Validar selecciones
    if (selectedGroups.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select at least one GROUP to add objects to.");
        return;
    }
    if (selectedRowsIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select at least one OBJECT from the table.");
        return;
    }

    // 3. Ejecutar la acción (Bucle anidado)
    int successCount = 0;
    int failCount = 0;

    for (const QModelIndex& rowIndex : selectedRowsIndexes)
    {
        int row = rowIndex.row();
        int64_t objectId = ui->setsObjectTable->item(row, 0)->text().toLongLong();
        QString objectName = ui->setsObjectTable->item(row, 1)->text();

        for (QListWidgetItem* groupItem : selectedGroups)
        {
            QString groupName = groupItem->text();
            logMessage(QString("[Info] Adding '%1' to group '%2'...").arg(objectName).arg(groupName));

            if (dbManager->addObjectToGroup(objectId, groupName.toStdString()))
            {
                successCount++;
            } else {
                failCount++;
                logMessage(QString("[Error] Could not add '%1' to group '%2'.").arg(objectName).arg(groupName));
            }
        }
    }

    // 4. Informar al usuario y refrescar
    logMessage(QString("--- Assignment summary: %1 success(es), %2 failure(s). ---").arg(successCount).arg(failCount));

    if (successCount > 0) {
        QString msg = QString("Successfully performed %1 assignments.").arg(successCount);
        if (failCount > 0) {
            msg += QString("\nTHERE WERE %1 ERRORS. Check the log.").arg(failCount);
            QMessageBox::warning(this, "Partial Result", msg);
        } else {
            QMessageBox::information(this, "Success", msg);
        }

        // --- 5. REFRESCAR ---
        // Simplemente llamamos a la función inteligente. Ella se encargará de todo.
        on_refreshListButton_2_clicked();

    } else {
        QMessageBox::critical(this, "Error", QString("Could not perform any of the %1 assignments.").arg(failCount));
    }
}

/**
 * @brief Slot: Pulsar "Quitar objeto de grupo"
 * (Versión simplificada: la lógica de refresco la hace on_refreshListButton_2_clicked)
 */
void MainWindow::on_removeFromGroupButton_clicked()
{
    if (!dbManager) return;

    // 1. Obtener selecciones
    QList<QListWidgetItem*> selectedGroups = ui->listWidget->selectedItems();
    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();

    // 2. Validar selecciones
    if (selectedGroups.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select at least one GROUP to remove objects from.");
        return;
    }
    if (selectedRowsIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select at least one OBJECT from the table.");
        return;
    }

    // 3. Ejecutar la acción (Bucle anidado)
    int successCount = 0;
    int failCount = 0;

    for (const QModelIndex& rowIndex : selectedRowsIndexes)
    {
        int row = rowIndex.row();
        int64_t objectId = ui->setsObjectTable->item(row, 0)->text().toLongLong();
        QString objectName = ui->setsObjectTable->item(row, 1)->text();

        for (QListWidgetItem* groupItem : selectedGroups)
        {
            QString groupName = groupItem->text();
            logMessage(QString("[Info] Removing '%1' from group '%2'...").arg(objectName).arg(groupName));

            if (dbManager->removeObjectFromGroup(objectId, groupName.toStdString()))
            {
                successCount++;
            } else {
                failCount++;
                logMessage(QString("[Error] Could not remove '%1' from group '%2'.").arg(objectName).arg(groupName));
            }
        }
    }

    // 4. Informar al usuario y refrescar
    logMessage(QString("--- Removal summary: %1 success(es), %2 failure(s). ---").arg(successCount).arg(failCount));

    if (successCount > 0) {
        QString msg = QString("Successfully performed %1 group removals.").arg(successCount);
        if (failCount > 0) {
            msg += QString("\nTHERE WERE %1 ERRORS. Check the log.").arg(failCount);
            QMessageBox::warning(this, "Partial Result", msg);
        } else {
            QMessageBox::information(this, "Success", msg);
        }

        // --- 5. REFRESCAR ---
        on_refreshListButton_2_clicked();

    } else {
        QMessageBox::critical(this, "Error", QString("Could not perform any of the %1 removals.").arg(failCount));
    }
}

/**
 * @brief Botón "Añadir Nuevo Objeto (Popup)" en la pestaña Sets.
 */
void MainWindow::on_addNewObjectSetButton_clicked()
{
    if (!dbManager) {
        logMessage("[Error] No connection to database.");
        return;
    }

    AddObjectDialog dialog(this);

    // 1. Configuramos el diálogo
    std::set<std::string> groups = dbManager->getAllUniqueGroupNames();
    dialog.setAvailableGroups(groups);

    // ¡IMPORTANTE! Pasamos el puntero del dbManager al diálogo
    // Usamos .get() porque dbManager es un std::unique_ptr
    dialog.setDbManager(dbManager.get());

    // 2. Ejecutamos
    // Si devuelve Accepted es que todo fue bien (se guardó dentro del diálogo)
    if (dialog.exec() == QDialog::Accepted)
    {
        logMessage("[OK] New object added.");

        // 3. Solo refrescamos las tablas
        on_refreshListButton_2_clicked();
        on_refreshListButton_clicked();
    }
    else
    {
        // Si devuelve Rejected (botón Cancelar o X)
        logMessage("[Info] Operation cancelled.");
    }
}

void MainWindow::on_editObjectButton_clicked()
{
    if (!dbManager) return;

    // 1. Obtener filas seleccionadas
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();

    // --- PROTECCIÓN CONTRA CRASH ---
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select an object to edit.");
        return;
    }

    if (selectedRows.count() > 1) {
        QMessageBox::warning(this, "Warning", "You can only edit ONE object at a time.\nPlease select only one.");
        return;
    }
    // -------------------------------

    // 2. Obtener ID
    int row = selectedRows.first().row();
    QString idStr = ui->setsObjectTable->item(row, 0)->text();
    int64_t id = idStr.toLongLong();

    // 3. Recuperar objeto
    nlohmann::json objData = dbManager->getSpaceObjectById(id);
    if (objData.empty()) return;

    // 4. Abrir diálogo
    AddObjectDialog dialog(this);
    dialog.setAvailableGroups(dbManager->getAllUniqueGroupNames());
    dialog.loadObjectData(objData);
    dialog.setEditMode(true);
    dialog.setDbManager(dbManager.get());

    if (dialog.exec() == QDialog::Accepted) {
        logMessage("[OK] Object edited successfully.");
        on_refreshListButton_2_clicked();
        on_refreshListButton_clicked();
    }
}
void MainWindow::onSetsTableContextMenuRequested(const QPoint &pos)
{
    // 1. Obtener filas seleccionadas
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    // 2. Crear Menú
    QMenu contextMenu(this);

    // Acción 1: Copiar JSON
    QString textoCopiar = (selectedRows.count() > 1) ? "Copy JSON (Object Array)" : "Copy JSON";
    QAction *copyJsonAction = contextMenu.addAction(textoCopiar);

    contextMenu.addSeparator(); // Una rayita separadora

    // Acción 2: Asignar a Grupo
    QAction *assignGroupAction = contextMenu.addAction("Assign to Groups...");

    // 3. Ejecutar menú y esperar clic
    QAction *selectedAction = contextMenu.exec(ui->setsObjectTable->mapToGlobal(pos));

    // --- LÓGICA: COPIAR JSON (MULTIPLE) ---
    if (selectedAction == copyJsonAction)
    {
        nlohmann::json finalJson;

        if (selectedRows.count() == 1) {
            // Caso simple: Un solo objeto
            int64_t id = ui->setsObjectTable->item(selectedRows.first().row(), 0)->text().toLongLong();
            finalJson = dbManager->getSpaceObjectById(id);
        } else {
            // Caso múltiple: Crear un Array
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

    // --- LÓGICA: ASIGNAR A GRUPOS (VENTANA EMERGENTE) ---
    else if (selectedAction == assignGroupAction)
    {
        // A. Crear una mini ventana (Dialog) al vuelo
        QDialog groupDialog(this);
        groupDialog.setWindowTitle("Select Groups");
        groupDialog.setMinimumWidth(300);
        QVBoxLayout *layout = new QVBoxLayout(&groupDialog);

        layout->addWidget(new QLabel("Select groups to add:", &groupDialog));

        // B. Lista de Grupos
        QListWidget *listWidget = new QListWidget(&groupDialog);
        listWidget->setSelectionMode(QAbstractItemView::MultiSelection); // Permitir seleccionar varios

        // Cargar grupos desde BD
        std::set<std::string> allGroups = dbManager->getAllUniqueGroupNames();
        for (const auto& g : allGroups) {
            listWidget->addItem(QString::fromStdString(g));
        }
        layout->addWidget(listWidget);

        // C. Botones Aceptar/Cancelar
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &groupDialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &groupDialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &groupDialog, &QDialog::reject);
        layout->addWidget(buttonBox);

        // D. Mostrar y esperar
        if (groupDialog.exec() == QDialog::Accepted) {
            // Recoger grupos seleccionados
            std::vector<std::string> targetGroups;
            for (auto item : listWidget->selectedItems()) {
                targetGroups.push_back(item->text().toStdString());
            }

            if (targetGroups.empty()) return;

            // E. Aplicar a TODOS los objetos seleccionados en la tabla
            int successCount = 0;
            for (const auto& idx : selectedRows) {
                int64_t objId = ui->setsObjectTable->item(idx.row(), 0)->text().toLongLong();

                for (const auto& groupName : targetGroups) {
                    if (dbManager->addObjectToGroup(objId, groupName)) {
                        successCount++;
                    }
                }
            }

            logMessage("[OK] Group assignments added.");
            QMessageBox::information(this, "Success", "Groups assigned successfully.");

            // Refrescar para ver los cambios en la columna "Groups"
            on_refreshListButton_2_clicked();
        }
    }
}
void MainWindow::on_deleteObjectSetButton_clicked()
{
    if (!dbManager) return;

    // 1. Obtener filas seleccionadas
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select at least one object to delete.");
        return;
    }

    int count = selectedRows.count();

    // 2. PEDIR CONFIRMACIÓN (Solo una vez para todo el grupo)

    QMessageBox::StandardButton reply;
    QString msg = QString("Are you SURE you want to delete %1 object(s)?\n\n"
                          "This action is irreversible and will also remove their associated images.")
                      .arg(count);

    reply = QMessageBox::question(this, "Confirm Mass Deletion", msg,
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    // 3. PROCESO DE BORRADO
    logMessage("[Info] Starting mass deletion of " + QString::number(count) + " objects...");

    int deletedCount = 0;
    int errorCount = 0;

    for (const auto& idx : selectedRows) {
        int row = idx.row();

        // Obtener ID de la columna 0
        QString idStr = ui->setsObjectTable->item(row, 0)->text();
        int64_t id = idStr.toLongLong();

        // A) Recuperar info del objeto PRIMERO (para saber qué foto borrar)
        // Esto es necesario porque si borramos el objeto primero, perdemos el nombre de la foto.
        nlohmann::json obj = dbManager->getSpaceObjectById(id);
        std::string picName = "";

        if (!obj.empty() && obj.contains("Picture") && !obj["Picture"].is_null()) {
            picName = obj["Picture"].get<std::string>();
        }

        // B) Borrar Objeto de la BBDD
        if (dbManager->deleteSpaceObjectById(id)) {
            deletedCount++;

            // C) Borrar Foto de GridFS (Si tenía)
            if (!picName.empty()) {
                if(dbManager->getImageManager().deleteImageByName(picName)) {
                    logMessage("[Info] Image '" + QString::fromStdString(picName) + "' deleted.");
                }
            }
        } else {
            errorCount++;
            logMessage("[Error] Could not delete object with ID: " + idStr);
        }
    }

    // 4. RESULTADO FINAL
    if (errorCount == 0) {
        QMessageBox::information(this, "Success", "Successfully deleted " + QString::number(deletedCount) + " objects.");
    } else {
        QMessageBox::warning(this, "Partial Result",
                             QString("Deleted %1 objects.\nThere were %2 errors.")
                                 .arg(deletedCount).arg(errorCount));
    }

    // 5. REFRESCAR TABLAS
    on_refreshListButton_2_clicked(); // Refresh Sets
    on_refreshListButton_clicked();   // Refresh Listado original
}


// Slot: Cuando tocas el checkbox "Mostrar Todo"
void MainWindow::on_showAllObjectsCheckBox_toggled(bool checked)
{
    if (checked) {
        // Si activas "Mostrar todo", limpiamos la selección de grupos visualmente
        // Usamos blockSignals MOMENTÁNEAMENTE para que limpiar no dispare un refresh doble
        ui->listWidget->blockSignals(true);
        ui->listWidget->clearSelection();
        ui->listWidget->blockSignals(false);

        // Refrescamos
        on_refreshListButton_2_clicked();
    }
}

// Slot: Cuando seleccionas un grupo en la lista
void MainWindow::on_listWidget_itemSelectionChanged()
{
    // Solo actuamos si el usuario ha seleccionado algo
    if (!ui->listWidget->selectedItems().isEmpty()) {

        // Si "Mostrar todos" estaba marcado, lo desmarcamos
        if (ui->showAllObjectsCheckBox->isChecked()) {
            // Silenciamos el check para que al desmarcarse no intente refrescar por su cuenta
            ui->showAllObjectsCheckBox->blockSignals(true);
            ui->showAllObjectsCheckBox->setChecked(false);
            ui->showAllObjectsCheckBox->blockSignals(false);
        }

        // Llamamos al refresh (que ahora está protegido contra bucles)
        //  on_refreshListButton_2_clicked();
    }
}
// En mainwindow.cpp

void MainWindow::on_setsObjectTable_selectionChanged()
{
    // 1. Obtener la selección actual
    auto selectedRows = ui->setsObjectTable->selectionModel()->selectedRows();

    // Si no hay nada seleccionado o hay más de uno, limpiamos y salimos
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

    // 2. Obtener el ID de la fila seleccionada (Columna 0)
    int row = selectedRows.first().row();
    QString idStr = ui->setsObjectTable->item(row, 0)->text();
    int64_t id = idStr.toLongLong();

    // 3. Buscar datos en BBDD
    if (!dbManager) return;
    nlohmann::json obj = dbManager->getSpaceObjectById(id);

    if (obj.empty() || obj.is_null()) return;

    // 4. Rellenar Textos (Usando helpers para evitar fallos con nulos)
    // Helper rápido para sacar strings
    auto getStr = [&](const std::string& key) {
        if (obj.contains(key) && !obj[key].is_null())
            return QString::fromStdString(obj[key].get<std::string>());
        return QString("-");
    };

    ui->setsDetailNorad->setText(QString::number(id));
    ui->setsDetailName->setText(getStr("Name"));
    ui->setsDetailAlias->setText(getStr("Abbreviation")); // Alias es Abbreviation
    ui->setsDetailComments->setText(getStr("Comments"));

    // Altitud (Numérico)
    if (obj.contains("Altitude") && !obj["Altitude"].is_null()) {
        double alt = obj["Altitude"];
        ui->setsDetailAltitude->setText(QString::number(alt) + " km");
    } else {
        ui->setsDetailAltitude->setText("-");
    }

    // Has LRR (1/0/Null -> Sí/No/Unknown)
    if (obj.contains("LaserRetroReflector")) {
        if (obj["LaserRetroReflector"].is_null()) ui->setsDetailLrr->setText("Unknown");
        else {
            int val = obj["LaserRetroReflector"];
            ui->setsDetailLrr->setText(val == 1 ? "Yes" : "No");
        }
    } else {
        ui->setsDetailLrr->setText("-");
    }

    // 5. Cargar Imagen
    ui->setsDetailImageLabel->clear();
    std::string picName = "";
    if (obj.contains("Picture") && !obj["Picture"].is_null()) {
        picName = obj["Picture"].get<std::string>();
    }

    if (!picName.empty()) {
        // Descargar de GridFS
        std::string imgData = dbManager->getImageManager().downloadImageByName(picName);
        if (!imgData.empty()) {
            QByteArray bytes(imgData.data(), imgData.length());
            QPixmap pix;
            if (pix.loadFromData(bytes)) {
                // Escalar imagen manteniendo proporción
                ui->setsDetailImageLabel->setPixmap(pix.scaled(
                    ui->setsDetailImageLabel->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
            } else {
                ui->setsDetailImageLabel->setText("Format error");
            }
        } else {
            ui->setsDetailImageLabel->setText("Img not found");
        }
    } else {
        ui->setsDetailImageLabel->setText("No image");
    }
}
void MainWindow::on_searchObjectButton_clicked()
{
    if (!dbManager) return;

    // 1. ABRIR POPUP PARA PEDIR ID
    bool ok;
    QString text = QInputDialog::getText(this, "Search Object",
                                         "Enter the ID (Norad) of the object:",
                                         QLineEdit::Normal, "", &ok);

    // Si el usuario da a Cancelar o no escribe nada, salimos
    if (!ok || text.isEmpty()) return;

    // 2. BUSCAR EN BBDD
    int64_t id = 0;
    try {
        id = text.toLongLong();
    } catch (...) {
        QMessageBox::warning(this, "Error", "ID must be numeric.");
        return;
    }

    nlohmann::json obj = dbManager->getSpaceObjectById(id);

    // 3. SI NO LO ENCUENTRA
    if (obj.empty() || obj.is_null()) {
        QMessageBox::information(this, "No results", "No object found with that ID.");
        return;
    }

    // 4. SI LO ENCUENTRA -> MODIFICAR LA INTERFAZ

    // A) Deseleccionar "Mostrar todos" SIN disparar el refresco
    ui->showAllObjectsCheckBox->blockSignals(true);
    ui->showAllObjectsCheckBox->setChecked(false);
    ui->showAllObjectsCheckBox->blockSignals(false);

    // B) Deseleccionar Grupos de la lista SIN disparar el refresco
    ui->listWidget->blockSignals(true);
    ui->listWidget->clearSelection();
    ui->listWidget->blockSignals(false);

    // C) Mostrar SOLO el objeto encontrado en la tabla
    std::vector<nlohmann::json> singleObjList;
    singleObjList.push_back(obj);

    // Usamos tu función existente para pintar la tabla
    populateSetsObjectTable(singleObjList);

    // D) Seleccionar la fila automáticamente
    if (ui->setsObjectTable->rowCount() > 0) {
        ui->setsObjectTable->selectRow(0); // Seleccionamos la primera (y única) fila

        // Opcional: Forzar que se muestren los detalles a la derecha
        // (Llamamos manualmente a la función que se activa al hacer click)
        on_setsObjectTable_selectionChanged();
    }

    logMessage("[Info] Search finished. Object " + text + " found.");
}
