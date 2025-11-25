#pragma once

#include <QMainWindow>
#include <memory> // For std::unique_ptr
#include <vector>
#include <nlohmann/json.hpp>

#include <QClipboard>
#include <QMenu>
#include <QInputDialog>

#include "gridfsimagemanager.h"

// Forward declarations
namespace Ui {
class MainWindow;
}
class SpaceObjectDBManager;
class AddObjectDialog; // Forward declaration for the dialog

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- Tabs 1 & 2 Slots ---
    void on_mostrarButton_clicked();
    void on_anadirButton_clicked();
    void on_eliminarButton_clicked();
    void on_browseButton_clicked();
    void on_refreshListButton_clicked();
    void onObjectListTableContextMenuRequested(const QPoint &pos);

    // --- Tab 3 "Sets" Slots ---
    void onSetsTableContextMenuRequested(const QPoint &pos);

    // Non-Modal Window Slots (Add/Edit)
    void on_addNewObjectSetButton_clicked();
    void on_editObjectButton_clicked();
    void on_deleteObjectSetButton_clicked(); // Mass deletion

    // Sets UI Interaction Slots
    void on_createGroupButton_clicked();
    void on_deleteGroupButton_clicked();
    void on_refreshListButton_2_clicked();
    void on_assignToGroupButton_clicked();
    void on_removeFromGroupButton_clicked();

    // Filters & Selection Slots
    void on_showAllObjectsCheckBox_toggled(bool checked);
    void on_listWidget_itemSelectionChanged();
    void on_setsObjectTable_selectionChanged();
    void on_searchObjectButton_clicked();

private:
    // Helper function to log messages to the GUI and file
    void logMessage(const QString& msg);

    // Setup table headers and properties
    void setupObjectListTable();

    // Sets Tab Helpers
    void setupSetsObjectTable();
    void refreshGroupList();
    void populateSetsObjectTable(const std::vector<nlohmann::json>& objects);

    // Members
    Ui::MainWindow *ui;
    std::unique_ptr<SpaceObjectDBManager> dbManager;
    QString m_localPicturePath;

    // --- SMART POINTERS FOR NON-MODAL DIALOGS ---
    // These manage the lifecycle of the secondary windows.
    // If the pointer is valid (not null), the window is currently open.
    std::unique_ptr<AddObjectDialog> m_addDialog;
    std::unique_ptr<AddObjectDialog> m_editDialog;

    // NEW: Pointer for the search dialog
    std::unique_ptr<QInputDialog> m_searchDialog;
};
