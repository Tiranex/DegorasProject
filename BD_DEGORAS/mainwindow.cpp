#include "mainwindow.h"
#include "ui_mainwindow.h"        // Archivo generado por el .ui
#include "SpaceObjectDBManager.h" // Nuestra clase de BBDD
#include "json_helpers.h"         // Nuestras funciones de JSON

// Includes de Qt para diálogos, archivos, etc.
#include <QMessageBox> // Para el diálogo de confirmación
#include <QFileDialog> // Para el diálogo "Examinar..."
#include <QDir>        // Para crear/comprobar carpetas
#include <QFile>       // Para copiar archivos
#include <QDebug>      // Para logs de depuración
#include <QPixmap>     // Para mostrar imágenes
#include <QByteArray>  // ¡NUEVO INCLUDE! Para manejar datos binarios

#include <string>
#include <memory>     // Para std::make_unique
#include <vector>

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


/**
 * @brief Convierte de forma segura un valor JSON a un QString.
 */
QString jsonValueToQString(const nlohmann::json& value)
{
    if (value.is_string()) {
        return QString::fromStdString(value.get<std::string>());
    }
    if (value.is_number()) {
        // Usamos .get<double>() para coger tanto enteros como decimales
        return QString::number(value.get<double>());
    }
    if (value.is_boolean()) {
        // Mostramos 1 o 0 como en tu imagen, en lugar de "true"/"false"
        return value.get<bool>() ? "1" : "0";
    }
    if (value.is_null()) {
        return "null"; // O QString() si prefieres vacío
    }
    // Para arrays u objetos, podríamos mostrar "[...]" o "{...}"
    // pero por ahora devolvemos vacío si no es un tipo simple.
    return QString();
}



// --- CONSTRUCTOR ---
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
// dbManager se inicializará a nullptr y se creará en el bloque try-catch
{
    ui->setupUi(this);
    //Configuracion de la tabla
    setupObjectListTable();



    // --- 1. Conectar a la BBDD ---
    const std::string URI = "mongodb://localhost:27017";
    const std::string DB_NAME = "DegorasDB";
    const std::string COLLECTION_NAME = "space_objects";

    try {
        // Usamos std::make_unique (corrige el error de antes)
        dbManager = std::make_unique<SpaceObjectDBManager>(URI, DB_NAME, COLLECTION_NAME);

        logMessage("[Info] Conectado exitosamente a " + QString::fromStdString(DB_NAME) + "/" + QString::fromStdString(COLLECTION_NAME));
    }
    catch (const std::exception& e)
    {
        // Si la conexión falla, lo mostramos y deshabilitamos todo
        logMessage("[Error Fatal] No se pudo conectar a MongoDB: " + QString(e.what()));

        // Deshabilitar botones si la BBDD falla
        ui->mostrarButton->setEnabled(false);
        ui->eliminarButton->setEnabled(false);
        ui->anadirButton->setEnabled(false);
        ui->browseButton->setEnabled(false);
        ui->idLineEdit->setEnabled(false);
    }


    // --- 3. Poner "Buscar por _id" como opción por defecto ---
    ui->searchByIdRadioButton->setChecked(true);
    ui->imageDisplayLabel->clear(); // Limpiar el visor de imagen

    ui->objectListTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // 4. Conectar el nuevo botón de refrescar
    connect(ui->refreshListButton, &QPushButton::clicked, this, &MainWindow::on_refreshListButton_clicked);

    // Conectar la señal de clic derecho de la tabla a nuestro nuevo slot
    connect(ui->objectListTable, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::onObjectListTableContextMenuRequested);

    // 5. Cargar la lista una vez al iniciar
    on_refreshListButton_clicked();

}

// --- DESTRUCTOR ---
MainWindow::~MainWindow()
{
    // delete dbManager; // <-- No es necesario, std::unique_ptr lo hace solo
    delete ui;
}

// --- FUNCIÓN DE AYUDA PARA LOGS ---
// (Esta es la implementación de la función que declaramos en el .h)
void MainWindow::logMessage(const QString& msg)
{
    // append() añade el texto y salta a una nueva línea
    ui->resultsTextEdit->append(msg);
    // Mover el cursor al final
    ui->resultsTextEdit->moveCursor(QTextCursor::End);
}


// --- ¡FUNCIÓN 'setup' MODIFICADA! ---
/**
 * @brief Configura las 25 columnas y propiedades de la tabla.
 */
void MainWindow::setupObjectListTable()
{

    // Usamos la lista global de cabeceras
    ui->objectListTable->setColumnCount(g_tableHeaders.size());
    ui->objectListTable->setHorizontalHeaderLabels(g_tableHeaders);

    // --- ¡CAMBIO! ---
    // Ya no estiramos una columna. Dejamos que el scroll horizontal
    // haga su trabajo.
    // Opcional: podemos hacer que las columnas se auto-ajusten al contenido
    // al final de la carga, en 'on_refreshListButton_clicked'.

    // Propiedades de solo lectura y selección de fila
    ui->objectListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->objectListTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Habilitar el scroll horizontal
    ui->objectListTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}



// =================================================================
// --- SLOTS (LÓGICA DE BOTONES) ---
// =================================================================

/**
 * @brief Se ejecuta al pulsar el botón "Mostrar".
 * Busca por _id o Name, muestra el JSON y la imagen.
 */
void MainWindow::on_mostrarButton_clicked()
{
    if (!dbManager) return; // No hacer nada si la BBDD no conectó

    nlohmann::json obj;
    std::string searchText = ui->idLineEdit->text().toStdString();

    // 1. Limpiar resultados anteriores
    ui->resultsTextEdit->clear();
    ui->imageDisplayLabel->clear();
    logMessage("[Info] Buscando...");

    // 2. Comprobar qué Radio Button está activo
    try
    {
        if (ui->searchByIdRadioButton->isChecked())
        {
            // --- Búsqueda por _id ---
            if (searchText.empty()) {
                logMessage("[Error] El _id no puede estar vacío.");
                return;
            }
            // Convertir el texto a número
            int64_t id = std::stoll(searchText);
            obj = dbManager->getSpaceObjectById(id);
        }
        else
        {
            // --- Búsqueda por Name ---
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

    // 3. Mostrar resultados (JSON)
    if (obj.empty() || obj.is_null()) {
        logMessage("[Info] Objeto no encontrado.");
        return;
    }

    logMessage("[OK] Objeto encontrado:");
    logMessage(QString::fromStdString(obj.dump(2))); // .dump(2) formatea el JSON

    // 4. Mostrar resultados (Imagen)
    if (obj.contains("Picture")) {
        std::string picName = obj["Picture"];

        if (picName.empty() || picName == "\"\"") {
            logMessage("[Info] Este objeto no tiene imagen asociada ('Picture' está vacío).");
            return;
        }

        // Ya no cargamos desde un archivo local, descargamos de GridFS
        logMessage("[Info] Descargando imagen '" + QString::fromStdString(picName) + "' desde GridFS...");

        // 1. Llamar a nuestro DBManager para descargar los datos
        std::string imageDataStd = dbManager->downloadImageByName(picName);

        if (imageDataStd.empty()) {
            logMessage("[Error Imagen] No se pudo descargar la imagen de GridFS (¿no se encontró?).");
            return;
        }

        // 2. Convertir std::string (C++) a QByteArray (Qt)
        QByteArray imageDataQt(imageDataStd.data(), imageDataStd.length());

        // 3. Cargar el QPixmap desde los datos en memoria
        QPixmap pixmap;
        if (!pixmap.loadFromData(imageDataQt)) {
            // Los datos estaban corruptos o no eran una imagen
            logMessage("[Error Imagen] Los datos descargados de GridFS no son una imagen válida.");
        } else {
            // 4. Mostrar la imagen
            ui->imageDisplayLabel->setPixmap(pixmap.scaled(
                ui->imageDisplayLabel->size(), // Escalar al tamaño del Label
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
 * Recoge los datos de la pestaña "Añadir Objeto" y los inserta.
 */
void MainWindow::on_anadirButton_clicked()
{
    if (!dbManager) return;

    // 1. Recoger datos de los QLineEdit
    std::string id_str = ui->idLineEdit_add->text().toStdString();
    std::string norad_str = ui->noradLineEdit_add->text().toStdString();
    std::string name_str = ui->nameLineEdit_add->text().toStdString();
    std::string class_str = ui->classLineEdit_add->text().toStdString();
    std::string pic_str = ui->pictureLineEdit_add->text().toStdString();

    // 2. Validar el _id
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
    //Valida name no vacio
    if (name_str.empty()) {
        logMessage("[Error] El nombre es obligatorio para crear un objeto.");
        return;
    }

    // <-- ¡CAMBIO 2! Si NORAD está vacío, usar el _id como string.
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
        {"Picture", pic_str} // El nombre de la imagen ya está aquí gracias a "Examinar..."
    };

    logMessage("[Info] Intentando crear objeto con _id: " + QString::fromStdString(id_str) + "...");

    // 4. Intentar insertar en la BBDD
    // --- ¡AQUÍ ESTÁ EL CAMBIO! ---
    // Pasamos el JSON y la ruta local que teníamos guardada.
    if (dbManager->createSpaceObject(newObj, m_localPicturePath.toStdString())) {
        logMessage("[OK] ¡Objeto creado exitosamente!");

        // Limpiar campos
        ui->idLineEdit_add->clear();
        ui->noradLineEdit_add->clear();
        ui->nameLineEdit_add->clear();
        ui->classLineEdit_add->clear();
        ui->pictureLineEdit_add->clear();

        // ¡Importante! Limpiar también la ruta guardada
        m_localPicturePath.clear();

    } else {
        logMessage("[Error] No se pudo crear el objeto (Revise duplicados de ID, Nombre o Foto).");
    }
}

/**
 * @brief Se ejecuta al pulsar el botón "Eliminar".
 * Borra un objeto (solo por _id, por seguridad).
 */
void MainWindow::on_eliminarButton_clicked()
{
    if (!dbManager) return;


    // <-- ¡NUEVO CAMBIO! Comprobar si se está buscando por Nombre.
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

        // --- ¡LÓGICA DE BORRADO MODIFICADA! ---
        // 1. Obtener el objeto *antes* de preguntar
        nlohmann::json obj = dbManager->getSpaceObjectById(id);

        if (obj.empty() || obj.is_null()) {
            logMessage("[Error] No se encontró ningún objeto con _id: " + QString::fromStdString(id_str));
            return;
        }

        // 2. Guardar el nombre de la imagen para borrarla después
        std::string picName = "";
        if (obj.contains("Picture") && !obj["Picture"].get<std::string>().empty()) {
            picName = obj["Picture"].get<std::string>();
        }

        // 3. Diálogo de confirmación.
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirmar borrado",
                                      "¿Estás seguro de que quieres eliminar el objeto con _id: " + QString::fromStdString(id_str) + "?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            logMessage("[Info] Borrado cancelado por el usuario.");
            return;
        }


        logMessage("[Info] Eliminando objeto con _id: " + QString::fromStdString(id_str) + "...");

        // 4. Borrar el objeto JSON
        if (dbManager->deleteSpaceObjectById(id)) {
            logMessage("[OK] Objeto eliminado exitosamente.");
            ui->idLineEdit->clear();
            ui->imageDisplayLabel->clear();

            // 5. ¡NUEVO! Borrar también la imagen de GridFS
            if (!picName.empty()) {
                logMessage("[Info] Eliminando imagen asociada '" + QString::fromStdString(picName) + "' de GridFS...");
                if (dbManager->deleteImageByName(picName)) {
                    logMessage("[OK] Imagen de GridFS eliminada.");
                } else {
                    logMessage("[Warn] No se pudo eliminar la imagen de GridFS (quizás ya no existía o era usada por otro objeto).");
                }
            }
        } else {
            logMessage("[Error] No se pudo eliminar el objeto (error inesperado, ya no existía).");
        }
    }
    catch (...) {
        logMessage("[Error] El _id introducido no es un número válido.");
    }
}

/**
 * @brief Se ejecuta al pulsar el botón "Examinar...".
 * Abre un diálogo, GUARDA LA RUTA y pone el nombre en el QLineEdit.
 * ¡YA NO SUBE A GRIDFS!
 */
void MainWindow::on_browseButton_clicked()
{
    // 1. Abrir diálogo para elegir archivo
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Seleccionar imagen",
        QDir::homePath(),
        "Imágenes (*.png *.jpg *.jpeg *.bmp)"
        );

    if (filePath.isEmpty()) {
        // El usuario canceló. Limpiamos la ruta guardada.
        m_localPicturePath.clear();
        // Opcional: limpiar también el campo de texto
        // ui->pictureLineEdit_add->clear();
        return;
    }

    // 2. Obtener información del archivo
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName(); // ej: "mi_satelite.jpg"

    // 3. ¡AQUÍ ESTÁ EL CAMBIO!
    // Guardamos la RUTA COMPLETA en nuestra variable miembro
    m_localPicturePath = filePath;

    // 4. Ponemos SÓLO EL NOMBRE en el campo de texto
    ui->pictureLineEdit_add->setText(fileName);

    // 5. ¡HEMOS ELIMINADO TODO EL CÓDIGO DE SUBIDA!
    // (Ya no hay file.open, file.readAll, ni dbManager->uploadImage)
}


// --- ¡SLOT 'refresh' MODIFICADO! ---
/**
 * @brief Se ejecuta al pulsar "Refrescar Lista".
 * Llama a la BBDD y llena la QTableWidget con las 25 columnas.
 */
void MainWindow::on_refreshListButton_clicked()
{
    if (!dbManager) {
        logMessage("[Error] No se puede refrescar la lista, no hay conexión a la BBDD.");
        return;
    }

    logMessage("[Info] Refrescando lista de objetos desde la BBDD...");

    // 1. Limpiar la tabla
    ui->objectListTable->setRowCount(0);


    // 2. Obtener todos los objetos
    std::vector<nlohmann::json> allObjects = dbManager->getAllSpaceObjects();

    // Optimización: Deshabilitar actualizaciones visuales y ordenado
    // mientras llenamos la tabla (mucho más rápido)
    ui->objectListTable->setSortingEnabled(false);
    ui->objectListTable->setUpdatesEnabled(false);

    // 3. Llenar la tabla
    for (const auto& obj : allObjects) {

        // Añadir una nueva fila
        int rowCount = ui->objectListTable->rowCount();
        ui->objectListTable->insertRow(rowCount);

        // --- ¡CAMBIO! ---
        // Iteramos por nuestra lista de cabeceras
        for (int col = 0; col < g_tableHeaders.size(); ++col) {

            // 3.1. Obtener la clave (nombre del campo)
            std::string key = g_tableHeaders[col].toStdString();

            // 3.2. Obtener el valor del JSON (de forma segura)
            nlohmann::json value;
            if (obj.contains(key)) {
                value = obj[key];
            }

            // 3.3. Convertir el valor JSON a QString
            QString stringValue = jsonValueToQString(value);

            // 3.4. Crear y poner el item en la tabla
            QTableWidgetItem *item = new QTableWidgetItem(stringValue);
            ui->objectListTable->setItem(rowCount, col, item);
        }
    }

    // 4. Reactivar las actualizaciones y el ordenado
    ui->objectListTable->setUpdatesEnabled(true);
    ui->objectListTable->setSortingEnabled(true);

    // 5. Opcional: ajustar el tamaño de las columnas al contenido
    // Esto puede ser lento si hay miles de filas
    // ui->objectListTable->resizeColumnsToContents();

    logMessage("[Info] Lista actualizada. " + QString::number(allObjects.size()) + " objetos cargados.");
}

/**
 * @brief Slot para el menú contextual (clic derecho) de la tabla.
 */
void MainWindow::onObjectListTableContextMenuRequested(const QPoint &pos)
{
    // 1. Obtener el item (celda) sobre el que se hizo clic
    QTableWidgetItem *item = ui->objectListTable->itemAt(pos);

    // Si el usuario hizo clic en un espacio vacío (no en una fila), no hacer nada
    if (!item) {
        return;
    }

    // 2. Obtener la fila en la que se hizo clic
    int row = item->row();

    // 3. Crear el menú
    QMenu contextMenu(this);
    QAction *copyJsonAction = contextMenu.addAction("Copiar JSON");

    // 4. Mostrar el menú donde el usuario hizo clic y esperar su selección
    //    (mapToGlobal convierte las coordenadas locales del widget a globales de la pantalla)
    QAction *selectedAction = contextMenu.exec(ui->objectListTable->mapToGlobal(pos));

    // 5. Si el usuario seleccionó "Copiar JSON"
    if (selectedAction == copyJsonAction)
    {
        // 6. Obtener el _id de la fila seleccionada (siempre está en la columna 0)
        QTableWidgetItem* idItem = ui->objectListTable->item(row, 0);
        if (!idItem) {
            logMessage("[Error] No se pudo encontrar el item _id de la fila seleccionada.");
            return;
        }

        QString idString = idItem->text();

        try {
            // 7. Convertir el _id (QString) a número (int64_t)
            int64_t id = idString.toLongLong();

            // 8. ¡Usar el DBManager para obtener el JSON completo!
            nlohmann::json obj = dbManager->getSpaceObjectById(id);

            if (obj.empty() || obj.is_null()) {
                logMessage("[Error] No se encontró el objeto con _id " + idString + " en la BBDD.");
                return;
            }

            // 9. Convertir el JSON a un string formateado (con 2 espacios de indentación)
            std::string jsonString = obj.dump(2);

            // 10. Copiar al portapapeles
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(jsonString));

            logMessage("[Info] JSON del objeto " + idString + " copiado al portapapeles.");

        } catch (const std::exception& e) {
            logMessage("[Error] El _id '" + idString + "' no es un número válido.");
        }
    }
}





