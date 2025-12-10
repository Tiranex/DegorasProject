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

    // En addobjectdialog.h
    void setAvailableSets(const std::vector<std::string> &sets);
    void setAvailableGroups(const std::vector<std::string> &groups);

    // Pasa el puntero del gestor de base de datos
    void setDbManager(SpaceObjectDBManager* dbManager);

    // Genera el JSON con todos los datos de la interfaz
    nlohmann::json getNewObjectData() const;

    // Devuelve la ruta de la imagen seleccionada
    QString getSelectedImagePath() const;

    // Rellena la interfaz con los datos de un objeto existente
    void loadObjectData(const nlohmann::json& objData);

    // Activa el modo edición (bloquea el ID/NORAD)
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

#endif // ADDOBJECTDIALOG_H
