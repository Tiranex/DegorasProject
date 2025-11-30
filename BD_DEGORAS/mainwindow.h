#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QKeyEvent>
#include <memory> // Necesario para std::unique_ptr

// --- INCLUDES FALTANTES AÑADIDOS ---
#include <QInputDialog>
#include <QTableWidget>
#include <QListWidget>
#include <QCheckBox>
#include <QRadioButton>

// Tus clases propias
#include "SpaceObjectDBManager.h"
#include "addobjectdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // --- TAB 1: SPACE OBJECTS (Main Management) ---
    void on_addNewObjectSetButton_clicked();
    void on_editObjectButton_clicked();
    void on_deleteObjectSetButton_clicked();
    void on_searchObjectButton_clicked();

    // Filtros y Visualización Tab 1
    void refreshMainTable();
    void on_mainObjectTable_selectionChanged();


    // --- TAB 2: OBSERVATION SETS ---
    void on_createSetButton_clicked();
    void on_deleteSetButton_clicked();
    void on_setsListWidget_itemSelectionChanged();
    void on_assignToSetButton_clicked();
    void on_removeFromSetButton_clicked();

    // --- TAB 3: GROUPS ---
    void on_createGroupButton_clicked();
    void on_deleteGroupButton_clicked();
    void on_groupsListWidget_itemSelectionChanged();
    void on_assignToGroupButton_clicked();
    void on_removeFromGroupButton_clicked();

    // --- MENU DATA ---
    void exportToCSV();
    void importFromJSON();

private:
    Ui::MainWindow *ui;
    std::unique_ptr<SpaceObjectDBManager> dbManager;

    // Ventanas independientes
    std::unique_ptr<AddObjectDialog> m_addDialog;
    std::unique_ptr<AddObjectDialog> m_editDialog;

    // AQUÍ ESTABA EL ERROR: Faltaba #include <QInputDialog> arriba
    std::unique_ptr<QInputDialog> m_searchDialog;

    // En private:
    // Función auxiliar para manejar el click derecho en CUALQUIER tabla
    void handleUniversalContextMenu(const QPoint &pos, QTableWidget* table);

    // Helpers UI
    void setupTables();
    void logMessage(const QString& msg);

    // Helpers Lógica
    void refreshSetListWidget();
    void refreshGroupListWidget();

    void populateMainTable(const std::vector<nlohmann::json>& objects);
    void on_tabWidget_currentChanged(int index);

    // Helper genérico para Tab 2 y 3
    void populateReadOnlyTable(QTableWidget* table, const std::vector<nlohmann::json>& objects);
    QIcon m_iconGreen;
    QIcon m_iconRed;
    QIcon m_iconGray;

    // Helper para inicializarlos
    void initIcons();
};

#endif // MAINWINDOW_H
