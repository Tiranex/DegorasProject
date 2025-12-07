#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QKeyEvent>
#include <QCloseEvent> // Necesario para detectar el cierre
#include <memory>
#include <QRegularExpression>

// UI Includes
#include <QInputDialog>
#include <QTableWidget>
#include <QListWidget>
#include <QCheckBox>
#include <QRadioButton>

// Clases propias
#include "SpaceObjectDBManager.h"
#include "addobjectdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

    // --- NUEVO: Detectar cierre de la ventana para avisar de cambios no guardados ---
    void closeEvent(QCloseEvent *event) override;

private slots:
    // --- TAB 1: SPACE OBJECTS ---
    void on_addNewObjectSetButton_clicked();
    void on_editObjectButton_clicked();
    void on_deleteObjectSetButton_clicked();

    // Visualización
    void refreshMainTable(); // Ahora leerá de m_localCache
    void on_mainObjectTable_selectionChanged();

    // --- TAB 2 & 3 (Sets / Groups) ---
    void on_createSetButton_clicked();
    void on_deleteSetButton_clicked();
    void on_setsListWidget_itemSelectionChanged();
    void on_assignToSetButton_clicked();
    void on_removeFromSetButton_clicked();

    void on_createGroupButton_clicked();
    void on_deleteGroupButton_clicked();
    void on_groupsListWidget_itemSelectionChanged();
    void on_assignToGroupButton_clicked();
    void on_removeFromGroupButton_clicked();

    // --- DATA MENU & SEARCH ---
    void exportToCSV();
    void importFromJSON();
    // Search Slots
    void on_LineEditSpaceObjects_textChanged(const QString &arg1); // Tab 1
    void on_searchLineEditSet_textChanged(const QString &arg1);    // Tab 2
    void on_searchLineEditGroups_textChanged(const QString &arg1); // Tab 3

    // --- NUEVO: SAVE / VERSIONING ---
    void on_saveChangesToDbButton_clicked(); // Botón "Commit"
    void createDatabaseVersion(); // Función auxiliar para crear el snapshot

    // --- LOGGING ---
    void onLogReceived(const QString& msg, const QString& level);

private slots:
    // Slot para context menu
    // (Nota: ya estaba declarado como lambda o función en cpp, si lo tienes en .h mantenlo)
    // void onMainTableContextMenuRequested(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    std::unique_ptr<SpaceObjectDBManager> dbManager;

    // Ventanas
    std::unique_ptr<AddObjectDialog> m_addDialog;
    std::unique_ptr<AddObjectDialog> m_editDialog;
    std::unique_ptr<QInputDialog> m_searchDialog;

    // --- NUEVO: MEMORIA Y ESTADO ---
    std::vector<nlohmann::json> m_localCache;
    std::set<std::string> m_localSets;   // <--- NUEVO: Sets en memoria
    std::set<std::string> m_localGroups;
    bool m_hasUnsavedChanges = false;         // ¿Hay cambios sin subir a BBDD?

    // Helpers
    void handleUniversalContextMenu(const QPoint &pos, QTableWidget* table);
    void setupTables();
    void setupLogTable();
    void logMessage(const QString& msg);

    void refreshSetListWidget();
    void refreshGroupListWidget();

    void populateMainTable(const std::vector<nlohmann::json>& objects);
    void populateReadOnlyTable(QTableWidget* table, const std::vector<nlohmann::json>& objects);
    void on_tabWidget_currentChanged(int index);

    // Iconos
    QIcon m_iconGreen;
    QIcon m_iconRed;
    QIcon m_iconGray;
    void initIcons();

    // Helper para marcar la UI como "Sucia" (Cambios pendientes)
    void setUnsavedChanges(bool changed);

    // Helper
    void applyTableFilter(QTableWidget* table, const QString& text);
};

#endif // MAINWINDOW_H
