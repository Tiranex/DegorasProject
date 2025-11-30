#ifndef ADDOBJECTDIALOG_H
#define ADDOBJECTDIALOG_H

#include <QDialog>
#include <set>
#include <string>
#include <vector>
#include "json_helpers.h"
#include "SpaceObjectDBManager.h"

namespace Ui {
class AddObjectDialog;
}

class AddObjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddObjectDialog(QWidget *parent = nullptr);
    ~AddObjectDialog();

    // Carga la lista de GRUPOS (Categorías internas)
    void setAvailableGroups(const std::set<std::string>& groups);

    // Carga la lista de SETS (Listas de observación)
    void setAvailableSets(const std::set<std::string>& sets);

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

private slots:
    void on_browseImageBtn_clicked();
    void on_saveButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::AddObjectDialog *ui;
    QString m_selectedImagePath;
    SpaceObjectDBManager* m_dbManager = nullptr;
    bool m_isEditMode = false;
};

#endif // ADDOBJECTDIALOG_H
