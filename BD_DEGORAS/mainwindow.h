#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QKeyEvent>
#include <QCloseEvent> // Necesario para detectar el cierre
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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;


    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_addNewObjectSetButton_clicked();
    void on_editObjectButton_clicked();
    void on_deleteObjectSetButton_clicked();
    void refreshMainTable();
    void on_mainObjectTable_selectionChanged();
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

    void exportToCSV();
    void importFromJSON();
    // Search Slots
    void on_LineEditSpaceObjects_textChanged(const QString &arg1);
    void on_searchLineEditSet_textChanged(const QString &arg1);
    void on_searchLineEditGroups_textChanged(const QString &arg1);


    void on_saveChangesToDbButton_clicked();
    void createDatabaseVersion();

    void onLogReceived(const QString& msg, const QString& level);
    void on_actionShowLogs_triggered();

private slots:

private:
    bool m_dirtyMain = true;
    bool m_dirtySets = true;
    bool m_dirtyGroups = true;
    Ui::MainWindow *ui;
    std::unique_ptr<SpaceObjectDBManager> dbManager;

    std::unique_ptr<AddObjectDialog> m_addDialog;
    std::unique_ptr<AddObjectDialog> m_editDialog;
    std::unique_ptr<QInputDialog> m_searchDialog;

    std::vector<nlohmann::json> m_localCache;
    std::set<std::string> m_localSets;
    std::set<std::string> m_localGroups;
    bool m_hasUnsavedChanges = false;


    void handleUniversalContextMenu(const QPoint &pos, QTableWidget* table);
    void setupTables();
    void setupLogTable();
    void logMessage(const QString& msg);

    void refreshSetListWidget();
    void refreshGroupListWidget();

    void populateMainTable(const std::vector<nlohmann::json>& objects);
    void populateReadOnlyTable(QTableWidget* table, const std::vector<nlohmann::json>& objects);
    void on_tabWidget_currentChanged(int index);


    QIcon m_iconGreen;
    QIcon m_iconRed;
    QIcon m_iconGray;
    void initIcons();


    void setUnsavedChanges(bool changed);


    void applyTableFilter(QTableWidget* table, const QString& text);
    LogWidget* m_logWidget = nullptr;
};

#endif
