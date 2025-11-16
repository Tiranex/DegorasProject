#pragma once

#include <QMainWindow>
#include <memory> // Para std::unique_ptr


#include <QClipboard> // Para el portapapeles
#include <QMenu>  // Para el menú contextual

// Declaraciones anticipadas (Forward declaration)
// Le decimos a C++ que estas clases existen,
// sin necesidad de incluirlas aquí.
namespace Ui {
class MainWindow;
}
class SpaceObjectDBManager; // Declaración anticipada de nuestra clase

class MainWindow : public QMainWindow
{
    Q_OBJECT // Macro obligatoria para clases de Qt con señales/slots

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Nombres de función generados por Qt a partir
    // de los "objectName" de tu .ui

    // Slot para el botón "Mostrar"
    void on_mostrarButton_clicked();

    // Slot para el botón "Añadir"
    void on_anadirButton_clicked();

    // Slot para el botón "Eliminar"
    void on_eliminarButton_clicked();

    // Slot para el nuevo botón "Examinar..."
    void on_browseButton_clicked();

    /**
     * @brief (¡NUEVO!) Slot para el botón "Refrescar Lista".
     */
    void on_refreshListButton_clicked();


    /**
     * @brief Se llama al hacer clic derecho en la tabla de objetos.
     * @param pos La posición (coordenadas) donde se hizo clic.
     */
    void onObjectListTableContextMenuRequested(const QPoint &pos);


private:
    /**
     * @brief Función de ayuda para añadir texto al log de la GUI.
     * @param msg El mensaje a mostrar.
     */
    void logMessage(const QString& msg); // <-- DECLARACIÓN AÑADIDA

    void setupObjectListTable();

    // Puntero a la clase de la GUI generada por el .ui
    Ui::MainWindow *ui;

    // Puntero inteligente al manager de la BBDD
    std::unique_ptr<SpaceObjectDBManager> dbManager; // <-- USA std::unique_ptr
    QString m_localPicturePath;
};
