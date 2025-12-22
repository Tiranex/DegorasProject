#ifndef ADDOBJECTDIALOG_H
#define ADDOBJECTDIALOG_H

#include <QDialog>
#include <set>
#include <string>
#include <vector>
#include "json_helpers.h"
#include "SpaceObjectDBManager.h"
#include "pluginmanager.h"
#include <QIntValidator>
#include <QDoubleValidator>

namespace Ui {
class AddObjectDialog;
}

class AddObjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddObjectDialog(QWidget *parent = nullptr);
    ~AddObjectDialog();

    void setAvailableSets(const std::vector<std::string> &sets);
    void setAvailableGroups(const std::vector<std::string> &groups);
    void setDbManager(SpaceObjectDBManager* dbManager);
    nlohmann::json getNewObjectData() const;
    QString getSelectedImagePath() const;
    void loadObjectData(const nlohmann::json& objData);
    void setEditMode(bool enable);
    void on_selectDBImageBtn_clicked();
    void setExistingObjects(const std::vector<nlohmann::json>* objects);

private slots:
    void on_browseImageBtn_clicked();
    void on_saveButton_clicked();
    void on_cancelButton_clicked();
    void on_searchPluginBtn_clicked();

private:
    Ui::AddObjectDialog *ui;
    QString m_selectedImagePath;
    SpaceObjectDBManager* m_dbManager = nullptr;
    bool m_isEditMode = false;

    void setFieldState(QWidget* widget, const QString& state); // state: "error", "warning", ""
    void validateFormLive();
    int m_storedEnablement = 1;
    const std::vector<nlohmann::json>* m_existingObjects = nullptr;
};

#endif
