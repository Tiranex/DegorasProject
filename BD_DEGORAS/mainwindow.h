#pragma once

#include <QMainWindow>
#include <memory> // Para std::unique_ptr
#include <vector> // Para std::vector
#include <nlohmann/json.hpp> // Para nlohmann::json

#include <QClipboard> // Para el portapapeles
#include <QMenu>      // Para el menú contextual

#include "gridfsimagemanager.h"

// Declaraciones anticipadas (Forward declaration)
namespace Ui {
class MainWindow;
}
class SpaceObjectDBManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT // Macro obligatoria

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- Slots de Pestaña 1 y 2 (Sin cambios) ---
    void on_mostrarButton_clicked();
    void on_anadirButton_clicked();
    void on_eliminarButton_clicked();
    void on_browseButton_clicked();
    void on_refreshListButton_clicked(); // Para la pestaña "Listado"
    void onObjectListTableContextMenuRequested(const QPoint &pos); // Clic derecho en "Listado"

    // --- ¡NUEVO! Slots para la Pestaña "Sets" (`tab_3`) ---

    /**
     * @brief Se llama al pulsar "Crear Grupo".
     */
    void on_createGroupButton_clicked();

    /**
     * @brief Se llama al pulsar "Eliminar Grupo".
     */
    void on_deleteGroupButton_clicked();

    /**
     * @brief Se llama al pulsar "Refresh" (el de la pestaña "Sets").
     * Carga los objetos en la tabla 'setsObjectTable' según el filtro.
     */
    void on_refreshListButton_2_clicked();

    /**
     * @brief Se llama al pulsar "Añadir objeto a grupo".
     */
    void on_assignToGroupButton_clicked();

    /**
     * @brief Se llama al pulsar "Quitar objeto de grupo".
     */
    void on_removeFromGroupButton_clicked();


private:
    /**
     * @brief Función de ayuda para añadir texto al log de la GUI.
     */
    void logMessage(const QString& msg);

    /**
     * @brief Configura la tabla 'objectListTable' (Pestaña "Listado").
     */
    void setupObjectListTable();

    // --- ¡NUEVO! Funciones de ayuda para Pestaña "Sets" ---

    /**
     * @brief Configura la nueva tabla 'setsObjectTable' (Pestaña "Sets").
     */
    void setupSetsObjectTable();

    /**
     * @brief Pide a la BBDD todos los grupos y rellena 'listWidget'.
     */
    void refreshGroupList();

    /**
     * @brief Rellena la tabla 'setsObjectTable' con los objetos dados.
     * @param objects Un vector de JSONs de objetos.
     */
    void populateSetsObjectTable(const std::vector<nlohmann::json>& objects);


    // --- Miembros (Sin cambios) ---
    Ui::MainWindow *ui;
    std::unique_ptr<SpaceObjectDBManager> dbManager;
    QString m_localPicturePath;
};
