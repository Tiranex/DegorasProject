#include "mainwindow.h"
#include "ui_mainwindow.h"      // Archivo generado por el .ui
#include "SpaceObjectDBManager.h" // Nuestra clase de BBDD
#include "json_helpers.h"       // Nuestras funciones de JSON

// Includes de Qt para diálogos, archivos, etc.
#include <QMessageBox> // Para el diálogo de confirmación
#include <QFileDialog> // Para el diálogo "Examinar..."
#include <QDir>        // Para crear/comprobar carpetas
#include <QFile>       // Para copiar archivos
#include <QDebug>      // Para logs de depuración
#include <QPixmap>     // Para mostrar imágenes
#include <QByteArray>  // Para manejar datos binarios

#include <string>
#include <memory>      // Para std::make_unique
#include <vector>
#include <set>         // ¡NUEVO! Para std::set

#include <QGuiApplication> // Para QGuiApplication
#include <QClipboard>      // Para QGuiApplication::clipboard()
#include <QMenu>           // Para QMenu


// Lista completa de cabeceras basada en tu imagen
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

    // 1. Configurar ambas tablas
    setupObjectListTable(); // Pestaña "Listado"
    setupSetsObjectTable(); // Pestaña "Sets"

    // --- 2. Conectar a la BBDD ---
    const std::string URI = "mongodb://localhost:27017";
    const std::string DB_NAME = "DegorasDB";
    const std::string COLLECTION_NAME = "space_objects";

    try {
        dbManager = std::make_unique<SpaceObjectDBManager>(URI, DB_NAME, COLLECTION_NAME);
        logMessage("[Info] Conectado exitosamente a " + QString::fromStdString(DB_NAME) + "/" + QString::fromStdString(COLLECTION_NAME));
    }
    catch (const std::exception& e)
    {
        logMessage("[Error Fatal] No se pudo conectar a MongoDB: " + QString(e.what()));
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
void MainWindow::on_mostrarButton_clicked()
{
    if (!dbManager) return;

    nlohmann::json obj;
    std::string searchText = ui->idLineEdit->text().toStdString();

    ui->resultsTextEdit->clear();
    ui->imageDisplayLabel->clear();
    logMessage("[Info] Buscando...");

    try
    {
        if (ui->searchByIdRadioButton->isChecked())
        {
            if (searchText.empty()) {
                logMessage("[Error] El _id no puede estar vacío.");
                return;
            }
            int64_t id = std::stoll(searchText);
            obj = dbManager->getSpaceObjectById(id);
        }
        else
        {
            if (searchText.empty()) {
                logMessage("[Error] El Name no puede estar vacío.");
                return;
            }
            obj = dbManager->getSpaceObjectByName(searchText);
        }
    }
    catch (const std::invalid_argument& e) {
        logMessage("[Error] El _id introducido no es un número válido.");
        return;
    }
    catch (const std::exception& e) {
        logMessage("[Error] Ocurrió un error en la búsqueda: " + QString(e.what()));
        return;
    }

    if (obj.empty() || obj.is_null()) {
        logMessage("[Info] Objeto no encontrado.");
        return;
    }

    logMessage("[OK] Objeto encontrado:");
    logMessage(QString::fromStdString(obj.dump(2)));

    if (obj.contains("Picture")) {
        std::string picName = obj["Picture"];
        if (picName.empty() || picName == "\"\"") {
            logMessage("[Info] Este objeto no tiene imagen asociada ('Picture' está vacío).");
            return;
        }

        logMessage("[Info] Descargando imagen '" + QString::fromStdString(picName) + "' desde GridFS...");
        std::string imageDataStd = dbManager->downloadImageByName(picName);
        if (imageDataStd.empty()) {
            logMessage("[Error Imagen] No se pudo descargar la imagen de GridFS.");
            return;
        }

        QByteArray imageDataQt(imageDataStd.data(), imageDataStd.length());
        QPixmap pixmap;
        if (!pixmap.loadFromData(imageDataQt)) {
            logMessage("[Error Imagen] Los datos descargados de GridFS no son una imagen válida.");
        } else {
            ui->imageDisplayLabel->setPixmap(pixmap.scaled(
                ui->imageDisplayLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                ));
            logMessage("[Info] Imagen '" + QString::fromStdString(picName) + "' cargada desde GridFS.");
        }
    } else {
        logMessage("[Info] Este objeto no tiene campo 'Picture'.");
    }
}


/**
 * @brief Se ejecuta al pulsar el botón "Añadir".
 * (¡MODIFICADO! Añade un array "Groups" vacío)
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
        logMessage("[Error] El _id es obligatorio para crear un objeto.");
        return;
    }
    try {
        id = std::stoll(id_str);
    } catch (...) {
        logMessage("[Error] El _id introducido no es un número válido.");
        return;
    }
    if (name_str.empty()) {
        logMessage("[Error] El nombre es obligatorio para crear un objeto.");
        return;
    }
    if (norad_str.empty()) {
        norad_str = id_str;
        logMessage("[Info] Campo NORAD vacío. Usando _id como NORAD.");
    }

    // 3. Crear el objeto JSON
    nlohmann::json newObj = {
        {"_id", id},
        {"NORAD", norad_str},
        {"Name", name_str},
        {"Classification", class_str},
        {"Picture", pic_str},
        // --- ¡LÍNEA AÑADIDA! ---
        // Añadimos un array de grupos vacío por defecto.
        {"Groups", nlohmann::json::array()}
    };

    logMessage("[Info] Intentando crear objeto con _id: " + QString::fromStdString(id_str) + "...");

    // 4. Intentar insertar en la BBDD
    if (dbManager->createSpaceObject(newObj, m_localPicturePath.toStdString())) {
        logMessage("[OK] ¡Objeto creado exitosamente!");

        // Limpiar campos
        ui->idLineEdit_add->clear();
        ui->noradLineEdit_add->clear();
        ui->nameLineEdit_add->clear();
        ui->classLineEdit_add->clear();
        ui->pictureLineEdit_add->clear();
        m_localPicturePath.clear();

        // ¡NUEVO! Refrescar la lista de grupos, por si hemos añadido uno
        // que no existía (recordar que createSpaceObject hace upsert)
        refreshGroupList();

    } else {
        logMessage("[Error] No se pudo crear el objeto (Revise duplicados de ID, Nombre o Foto).");
    }
}

// ... (on_eliminarButton_clicked - Sin cambios) ...
void MainWindow::on_eliminarButton_clicked()
{
    if (!dbManager) return;

    if (ui->searchByNameRadioButton->isChecked()) {
        logMessage("[Error] No se puede eliminar por 'Name'.");
        logMessage("[Info] Por favor, busca el objeto por '_id' para poder eliminarlo.");
        return;
    }

    std::string id_str = ui->idLineEdit->text().toStdString();
    if (id_str.empty()) {
        logMessage("[Error] Introduce un _id en el campo de búsqueda para eliminar.");
        return;
    }

    try
    {
        int64_t id = std::stoll(id_str);
        nlohmann::json obj = dbManager->getSpaceObjectById(id);

        if (obj.empty() || obj.is_null()) {
            logMessage("[Error] No se encontró ningún objeto con _id: " + QString::fromStdString(id_str));
            return;
        }

        std::string picName = "";
        if (obj.contains("Picture") && !obj["Picture"].get<std::string>().empty()) {
            picName = obj["Picture"].get<std::string>();
        }

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirmar borrado",
                                      "¿Estás seguro de que quieres eliminar el objeto con _id: " + QString::fromStdString(id_str) + "?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            logMessage("[Info] Borrado cancelado por el usuario.");
            return;
        }

        logMessage("[Info] Eliminando objeto con _id: " + QString::fromStdString(id_str) + "...");

        if (dbManager->deleteSpaceObjectById(id)) {
            logMessage("[OK] Objeto eliminado exitosamente.");
            ui->idLineEdit->clear();
            ui->imageDisplayLabel->clear();

            if (!picName.empty()) {
                logMessage("[Info] Eliminando imagen asociada '" + QString::fromStdString(picName) + "' de GridFS...");
                if (dbManager->deleteImageByName(picName)) {
                    logMessage("[OK] Imagen de GridFS eliminada.");
                } else {
                    logMessage("[Warn] No se pudo eliminar la imagen de GridFS.");
                }
            }
        } else {
            logMessage("[Error] No se pudo eliminar el objeto.");
        }
    }
    catch (...) {
        logMessage("[Error] El _id introducido no es un número válido.");
    }
}


// ... (on_browseButton_clicked - Sin cambios) ...
void MainWindow::on_browseButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Seleccionar imagen",
        QDir::homePath(),
        "Imágenes (*.png *.jpg *.jpeg *.bmp)"
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
        logMessage("[Error] No se puede refrescar la lista, no hay conexión a la BBDD.");
        return;
    }

    logMessage("[Info] Refrescando lista de objetos desde la BBDD...");
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
    logMessage("[Info] Lista actualizada. " + QString::number(allObjects.size()) + " objetos cargados.");
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
    QAction *copyJsonAction = contextMenu.addAction("Copiar JSON");
    QAction *selectedAction = contextMenu.exec(ui->objectListTable->mapToGlobal(pos));

    if (selectedAction == copyJsonAction)
    {
        QTableWidgetItem* idItem = ui->objectListTable->item(row, 0);
        if (!idItem) {
            logMessage("[Error] No se pudo encontrar el item _id de la fila seleccionada.");
            return;
        }
        QString idString = idItem->text();

        try {
            int64_t id = idString.toLongLong();
            nlohmann::json obj = dbManager->getSpaceObjectById(id);
            if (obj.empty() || obj.is_null()) {
                logMessage("[Error] No se encontró el objeto con _id " + idString + " en la BBDD.");
                return;
            }
            std::string jsonString = obj.dump(2);
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(jsonString));
            logMessage("[Info] JSON del objeto " + idString + " copiado al portapapeles.");
        } catch (const std::exception& e) {
            logMessage("[Error] El _id '" + idString + "' no es un número válido.");
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
    // Usaremos 3 columnas: ID, Nombre, y Grupos
    ui->setsObjectTable->setColumnCount(3);
    ui->setsObjectTable->setHorizontalHeaderLabels({"_id", "Name", "Groups"});

    // Propiedades de solo lectura y selección de fila única
    ui->setsObjectTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->setsObjectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->setsObjectTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Ajustar columnas
    ui->setsObjectTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Name
    ui->setsObjectTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // Groups
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

    logMessage("[Info] Lista de grupos actualizada.");
}

/**
 * @brief Rellena la tabla 'setsObjectTable' con los objetos dados.
 * (¡VERSIÓN SEGURA CORREGIDA!)
 */
void MainWindow::populateSetsObjectTable(const std::vector<nlohmann::json>& objects)
{
    ui->setsObjectTable->setSortingEnabled(false);
    ui->setsObjectTable->setUpdatesEnabled(false);

    ui->setsObjectTable->setRowCount(0); // Limpiar tabla

    for (const auto& obj : objects) {

        // --- ¡CAMBIO VITAL DE SEGURIDAD! ---
        // Si un objeto no tiene _id, no podemos trabajar con él
        // en esta pestaña (no se puede asignar a grupos). Lo saltamos.
        if (!obj.contains("_id")) {
            continue; // Saltar este objeto y pasar al siguiente
        }

        // Si llegamos aquí, el objeto SÍ tiene _id
        int row = ui->setsObjectTable->rowCount();
        ui->setsObjectTable->insertRow(row);

        // Columna 0: _id (Ahora sabemos que existe)
        // Usamos jsonValueToQString que es seguro para tipos (número, string, etc.)
        QString idStr = jsonValueToQString(obj["_id"]);
        QTableWidgetItem *idItem = new QTableWidgetItem(idStr);
        ui->setsObjectTable->setItem(row, 0, idItem);

        // Columna 1: Name (con valor por defecto)
        QString nameStr = "[Sin Nombre]"; // Valor por defecto
        if (obj.contains("Name")) {
            nameStr = jsonValueToQString(obj["Name"]);
        }
        QTableWidgetItem *nameItem = new QTableWidgetItem(nameStr);
        ui->setsObjectTable->setItem(row, 1, nameItem);

        // Columna 2: Groups (con valor por defecto)
        QString groupsStr = ""; // Vacío por defecto
        if (obj.contains("Groups")) {
            groupsStr = jsonValueToQString(obj["Groups"]);
        }
        QTableWidgetItem *groupsItem = new QTableWidgetItem(groupsStr);
        ui->setsObjectTable->setItem(row, 2, groupsItem);
    }

    ui->setsObjectTable->setUpdatesEnabled(true);
    ui->setsObjectTable->setSortingEnabled(true);

    // Ajustar el tamaño de la columna _id al contenido
    ui->setsObjectTable->resizeColumnToContents(0);
}

/**
 * @brief Slot: Pulsar "Crear Grupo"
 */
void MainWindow::on_createGroupButton_clicked()
{
    if (!dbManager) return;

    QString groupName = ui->newGroupLineEdit->text().trimmed(); // .trimmed() quita espacios
    if (groupName.isEmpty()) {
        QMessageBox::warning(this, "Error", "El nombre del grupo no puede estar vacío.");
        return;
    }

    if (dbManager->crearGrupo(groupName.toStdString())) {
        logMessage("[OK] Grupo '" + groupName + "' creado.");
        ui->newGroupLineEdit->clear();
        refreshGroupList(); // Actualizar la lista
    } else {
        QMessageBox::warning(this, "Error", "No se pudo crear el grupo.\nEs posible que ya exista.");
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
        QMessageBox::warning(this, "Error", "Por favor, selecciona un grupo de la lista para eliminar.");
        return;
    }

    QString groupName = selectedItem->text();

    // 2. Pedir confirmación
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmar borrado",
                                  "¿Estás seguro de que quieres eliminar el grupo '" + groupName + "'?\n\nEsto también lo eliminará de TODOS los objetos que lo contengan.",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        logMessage("[Info] Borrado de grupo cancelado.");
        return;
    }

    // 3. Proceder con el borrado
    if (dbManager->eliminarGrupo(groupName.toStdString())) {
        logMessage("[OK] Grupo '" + groupName + "' eliminado.");
        refreshGroupList(); // Actualizar la lista de grupos
        on_refreshListButton_2_clicked(); // Actualizar la tabla de objetos
    } else {
        QMessageBox::warning(this, "Error", "Ocurrió un error al eliminar el grupo.");
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

    // --- 1. GUARDAR SELECCIÓN (Grupos) ---
    QList<QListWidgetItem*> selectedGroupsItems = ui->listWidget->selectedItems();
    QSet<QString> selectedGroupNames;
    for (QListWidgetItem* item : selectedGroupsItems) {
        selectedGroupNames.insert(item->text());
    }

    // --- 2. GUARDAR SELECCIÓN (Objetos) ---
    auto selectedRowsIndexes = ui->setsObjectTable->selectionModel()->selectedRows();
    QSet<int64_t> selectedObjectIDs;
    for (const QModelIndex& rowIndex : selectedRowsIndexes) {
        if (ui->setsObjectTable->item(rowIndex.row(), 0)) {
            selectedObjectIDs.insert(ui->setsObjectTable->item(rowIndex.row(), 0)->text().toLongLong());
        }
    }

    // --- 3. Lógica de refresco (tu código original) ---
    std::vector<nlohmann::json> objects;

    if (ui->showAllObjectsCheckBox->isChecked())
    {
        logMessage("[Info] Mostrando todos los objetos...");
        objects = dbManager->getAllSpaceObjects();
    }
    else
    {
        std::set<std::string> selectedGroupsForFilter;
        for (const QString& name : selectedGroupNames) {
            selectedGroupsForFilter.insert(name.toStdString());
        }

        if (selectedGroupsForFilter.empty()) {
            logMessage("[Info] Selecciona uno o más grupos para filtrar, o marca 'Mostrar todos los objetos'.");
            ui->setsObjectTable->setRowCount(0);
            objects.clear();
        }
        else
        {
            logMessage("[Info] Filtrando por " + QString::number(selectedGroupsForFilter.size()) + " grupo(s)...");
            objects = dbManager->getSpaceObjectsByGroups(selectedGroupsForFilter);
        }
    }

    // --- 4. Rellenar la tabla (pierde selección de tabla) ---
    populateSetsObjectTable(objects);

    // --- 5. Refrescar lista de grupos (pierde selección de lista Y PONE UNA PROPIA) ---
    refreshGroupList();

    logMessage("[Info] " + QString::number(objects.size()) + " objetos cargados en la tabla 'Sets'.");

    // --- 6. RESTAURAR SELECCIÓN (Grupos) ---
    ui->listWidget->blockSignals(true);

    // --- ¡¡FIX!! ---
    // Limpiamos la selección que 'refreshGroupList' haya podido poner
    ui->listWidget->clearSelection();

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* item = ui->listWidget->item(i);
        if (selectedGroupNames.contains(item->text())) {
            item->setSelected(true);
        }
    }
    ui->listWidget->blockSignals(false);

    // --- 7. RESTAURAR SELECCIÓN (Objetos) ---
    ui->setsObjectTable->blockSignals(true);

    // --- ¡¡FIX!! ---
    // Limpiamos por si acaso 'populateSetsObjectTable' cambiase en el futuro
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
        QMessageBox::warning(this, "Error", "Por favor, selecciona al menos un GRUPO al que quieres añadir los objetos.");
        return;
    }
    if (selectedRowsIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Por favor, selecciona al menos un OBJETO de la tabla que quieres modificar.");
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
            logMessage(QString("[Info] Añadiendo '%1' al grupo '%2'...").arg(objectName).arg(groupName));

            if (dbManager->addObjectToGroup(objectId, groupName.toStdString()))
            {
                successCount++;
            } else {
                failCount++;
                logMessage(QString("[Error] No se pudo añadir '%1' al grupo '%2'.").arg(objectName).arg(groupName));
            }
        }
    }

    // 4. Informar al usuario y refrescar
    logMessage(QString("--- Resumen de asignación: %1 éxito(s), %2 fallo(s). ---").arg(successCount).arg(failCount));

    if (successCount > 0) {
        QString msg = QString("Se realizaron %1 asignaciones con éxito.").arg(successCount);
        if (failCount > 0) {
            msg += QString("\nHUBO %1 ERRORES. Revisa el log.").arg(failCount);
            QMessageBox::warning(this, "Resultado Parcial", msg);
        } else {
            QMessageBox::information(this, "Éxito", msg);
        }

        // --- 5. REFRESCAR ---
        // Simplemente llamamos a la función inteligente. Ella se encargará de todo.
        on_refreshListButton_2_clicked();

    } else {
        QMessageBox::critical(this, "Error", QString("No se pudo realizar ninguna de las %1 asignaciones.").arg(failCount));
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
        QMessageBox::warning(this, "Error", "Por favor, selecciona al menos un GRUPO del que quieres quitar los objetos.");
        return;
    }
    if (selectedRowsIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Por favor, selecciona al menos un OBJETO de la tabla que quieres modificar.");
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
            logMessage(QString("[Info] Quitando '%1' del grupo '%2'...").arg(objectName).arg(groupName));

            if (dbManager->removeObjectFromGroup(objectId, groupName.toStdString()))
            {
                successCount++;
            } else {
                failCount++;
                logMessage(QString("[Error] No se pudo quitar '%1' del grupo '%2'.").arg(objectName).arg(groupName));
            }
        }
    }

    // 4. Informar al usuario y refrescar
    logMessage(QString("--- Resumen de eliminación: %1 éxito(s), %2 fallo(s). ---").arg(successCount).arg(failCount));

    if (successCount > 0) {
        QString msg = QString("Se realizaron %1 eliminaciones de grupo con éxito.").arg(successCount);
        if (failCount > 0) {
            msg += QString("\nHUBO %1 ERRORES. Revisa el log.").arg(failCount);
            QMessageBox::warning(this, "Resultado Parcial", msg);
        } else {
            QMessageBox::information(this, "Éxito", msg);
        }

        // --- 5. REFRESCAR ---
        on_refreshListButton_2_clicked();

    } else {
        QMessageBox::critical(this, "Error", QString("No se pudo realizar ninguna de las %1 eliminaciones.").arg(failCount));
    }
}
