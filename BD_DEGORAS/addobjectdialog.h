#ifndef ADDOBJECTDIALOG_H
#define ADDOBJECTDIALOG_H

#include <QDialog>
#include <set>
#include <string>
#include <vector>
#include "json_helpers.h" // Asegúrate de que detecta tu librería JSON
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

    // Función para pasarle los grupos existentes (usamos std::set para coincidir con tu DBManager)
    void setAvailableGroups(const std::set<std::string>& groups);

    // ¡NUEVO! Función para pasarle el puntero del gestor de base de datos
    void setDbManager(SpaceObjectDBManager* dbManager);

    // Función que devuelve el JSON listo para guardar
    nlohmann::json getNewObjectData() const;

    // Función que devuelve la ruta de la imagen seleccionada (para subirla luego)
    QString getSelectedImagePath() const;

    // Carga los datos de un JSON en las cajas de texto
    void loadObjectData(const nlohmann::json& objData);

    // Activa el modo edición (bloquea el ID para que no se toque)
    void setEditMode(bool enable);

private slots:
    void on_browseImageBtn_clicked(); // Botón examinar imagen
    void on_saveButton_clicked();     // Botón guardar
    void on_cancelButton_clicked();   // Botón cancelar

private:
    Ui::AddObjectDialog *ui;
    QString m_selectedImagePath; // Variable interna para guardar la ruta del archivo
    SpaceObjectDBManager* m_dbManager = nullptr;
    bool m_isEditMode = false; // Bandera para saber qué hacer al guardar
};

#endif // ADDOBJECTDIALOG_H
