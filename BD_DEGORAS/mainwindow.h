#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QKeyEvent>
#include <QCloseEvent>
#include <memory>
#include <QRegularExpression>
#include <QInputDialog>
#include <QTableWidget>
#include <QListWidget>
#include <QCheckBox>
#include <QRadioButton>

#include "SpaceObjectDBManager.h"
#include "addobjectdialog.h"

class LogWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief The main application window for the Space Object Manager.
 *
 * @details This class acts as the central controller for the UI. It manages:
 * - The interaction between the user and the local in-memory cache (`m_localCache`).
 * - Displaying data in `QTableWidget`s with filtering and sorting capabilities.
 * - CRUD operations via `AddObjectDialog`.
 * - Organization of objects into logical "Sets" and "Groups".
 * - Synchronization with the backend database (`SpaceObjectDBManager`).
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the MainWindow.
     * @param parent The parent widget (usually nullptr for the main window).
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor. Cleans up UI resources and smart pointers.
     */
    ~MainWindow();

protected:
    /**
     * @brief Handles key press events for global shortcuts.
     * @details Specifically listens for:
     * - `Delete`: To remove selected objects/sets/groups.
     * - `Enter`: To edit the selected object.
     * @param event The key event parameters.
     */
    void keyPressEvent(QKeyEvent *event) override;

    /**
     * @brief Intercepts the window close event.
     * @details Checks `m_hasUnsavedChanges`. If true, prompts the user to save, discard, or cancel.
     * @param event The close event parameters.
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    // --- MAIN TABLE OPERATIONS ---

    /**
     * @brief Opens the dialog to add a new space object.
     */
    void on_addNewObjectSetButton_clicked();

    /**
     * @brief Opens the dialog to edit the currently selected space object.
     */
    void on_editObjectButton_clicked();

    /**
     * @brief Deletes the currently selected space object(s) from the local cache.
     */
    void on_deleteObjectSetButton_clicked();

    /**
     * @brief Re-renders the main table based on the current `m_localCache`.
     */
    void refreshMainTable();

    /**
     * @brief Updates the "Detail View" sidebar when the selection changes in the main table.
     */
    void on_mainObjectTable_selectionChanged();

    // --- SETS MANAGEMENT ---

    /**
     * @brief Creates a new custom Set tag.
     */
    void on_createSetButton_clicked();

    /**
     * @brief Deletes an existing Set tag and removes it from all associated objects.
     */
    void on_deleteSetButton_clicked();

    /**
     * @brief Filters the "Sets View" table based on the selected Set in the list.
     */
    void on_setsListWidget_itemSelectionChanged();

    /**
     * @brief Assigns the selected object(s) to the currently selected Set.
     */
    void on_assignToSetButton_clicked();

    /**
     * @brief Removes the selected object(s) from the currently selected Set.
     */
    void on_removeFromSetButton_clicked();

    // --- GROUPS MANAGEMENT ---

    /**
     * @brief Creates a new custom Group tag.
     */
    void on_createGroupButton_clicked();

    /**
     * @brief Deletes an existing Group tag and removes it from all associated objects.
     */
    void on_deleteGroupButton_clicked();

    /**
     * @brief Filters the "Groups View" table based on the selected Group in the list.
     */
    void on_groupsListWidget_itemSelectionChanged();

    /**
     * @brief Assigns the selected object(s) to the currently selected Group.
     */
    void on_assignToGroupButton_clicked();

    /**
     * @brief Removes the selected object(s) from the currently selected Group.
     */
    void on_removeFromGroupButton_clicked();

    // --- DATA IMPORT/EXPORT ---

    /**
     * @brief Exports the currently visible table data to a CSV file.
     */
    void exportToCSV();

    /**
     * @brief Imports objects from a JSON file into the local cache.
     */
    void importFromJSON();

    // --- SEARCH & FILTERING ---

    /**
     * @brief Live filter for the Main Object Table.
     * @param arg1 The search string (supports Regex).
     */
    void on_LineEditSpaceObjects_textChanged(const QString &arg1);

    /**
     * @brief Live filter for the Sets Table.
     * @param arg1 The search string.
     */
    void on_searchLineEditSet_textChanged(const QString &arg1);

    /**
     * @brief Live filter for the Groups Table.
     * @param arg1 The search string.
     */
    void on_searchLineEditGroups_textChanged(const QString &arg1);

    // --- DATABASE & VERSIONING ---

    /**
     * @brief Commits the local cache (`m_localCache`) to the persistent database.
     */
    void on_saveChangesToDbButton_clicked();

    /**
     * @brief Creates a version snapshot of the current database state.
     */
    void createDatabaseVersion();

    // --- LOGGING ---

    /**
     * @brief Slot to receive log messages from the logger thread/worker.
     * @param msg The log message.
     * @param level The severity level (Info, Warning, Error).
     */
    void onLogReceived(const QString& msg, const QString& level);

    /**
     * @brief Toggles the visibility of the Log Widget window.
     */
    void on_actionShowLogs_triggered();

private:
    // --- DIRTY FLAGS FOR CACHING ---
    // These boolean flags are used to prevent unnecessary UI redraws when switching tabs.
    // If a table hasn't changed since the user last looked at it, we don't rebuild it.
    bool m_dirtyMain = true;   ///< Needs refresh: Main Object Table
    bool m_dirtySets = true;   ///< Needs refresh: Sets View Table
    bool m_dirtyGroups = true; ///< Needs refresh: Groups View Table

    Ui::MainWindow *ui;

    // --- CORE MANAGERS ---
    std::unique_ptr<SpaceObjectDBManager> dbManager; ///< Handles backend DB connection (SQLite/MongoDB).
    std::unique_ptr<AddObjectDialog> m_addDialog;    ///< Dialog instance for creating objects.
    std::unique_ptr<AddObjectDialog> m_editDialog;   ///< Dialog instance for editing objects.
    std::unique_ptr<QInputDialog> m_searchDialog;    ///< Generic input dialog.
    LogWidget* m_logWidget = nullptr;                ///< Separate window for system logs.

    // --- IN-MEMORY DATA CACHE ---
    /**
     * @brief The primary data store for the application while running.
     * @details This vector holds the JSON representation of every Space Object loaded.
     * Operations (Add/Edit/Delete) modify this vector first. Changes are only
     * persisted to disk when the user explicitly saves.
     */
    std::vector<nlohmann::json> m_localCache;

    std::set<std::string> m_localSets;   ///< Unique list of all available Set names.
    std::set<std::string> m_localGroups; ///< Unique list of all available Group names.

    bool m_hasUnsavedChanges = false; ///< Tracks if `m_localCache` differs from the database.

    // --- ICON CACHE ---
    // Pre-rendered icons to avoid recreating QPixmaps for every table cell.
    QIcon m_iconGreen; ///< Enabled status.
    QIcon m_iconRed;   ///< Disabled status.
    QIcon m_iconGray;  ///< Unknown status.

    // --- HELPERS ---

    /**
     * @brief Displays a context menu (Right-Click) for table rows.
     * @details Allows actions like "Copy JSON", "Assign to Set", etc.
     */
    void handleUniversalContextMenu(const QPoint &pos, QTableWidget* table);

    void setupTables();
    void setupLogTable();
    void logMessage(const QString& msg);
    void refreshSetListWidget();
    void refreshGroupListWidget();
    void populateMainTable(const std::vector<nlohmann::json>& objects);
    void populateReadOnlyTable(QTableWidget* table, const std::vector<nlohmann::json>& objects);
    void on_tabWidget_currentChanged(int index);
    void initIcons();

    /**
     * @brief Updates the UI state to reflect unsaved changes.
     * @details Enables/Disables the "Save" button and updates the window title.
     * @param changed True if there are unsaved modifications.
     */
    void setUnsavedChanges(bool changed);

    /**
     * @brief Generic helper to filter rows in any QTableWidget.
     * @param table The table to filter.
     * @param text The text/regex to match against.
     */
    void applyTableFilter(QTableWidget* table, const QString& text);
};

#endif // MAINWINDOW_H
