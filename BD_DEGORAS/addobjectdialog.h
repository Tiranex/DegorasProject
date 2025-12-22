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

/**
 * @brief Dialog for creating or editing a Space Object.
 *
 * This dialog handles form validation, image selection (local or DB),
 * and JSON data packaging for the database manager.
 */
class AddObjectDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent The parent widget.
     */
    explicit AddObjectDialog(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~AddObjectDialog();

    /**
     * @brief Populates the "Sets" list widget with available options.
     * @param sets A vector of set names.
     */
    void setAvailableSets(const std::vector<std::string> &sets);

    /**
     * @brief Populates the "Groups" list widget with available options.
     * @param groups A vector of group names.
     */
    void setAvailableGroups(const std::vector<std::string> &groups);

    /**
     * @brief Sets the database manager pointer used for checking image existence.
     * @param dbManager Pointer to the active SpaceObjectDBManager.
     */
    void setDbManager(SpaceObjectDBManager* dbManager);

    /**
     * @brief Collects all data from the UI fields into a JSON object.
     * @return A nlohmann::json object ready for database insertion.
     */
    nlohmann::json getNewObjectData() const;

    /**
     * @brief Returns the path of a locally selected image file.
     * @return The absolute file path, or empty string if using a DB image.
     */
    QString getSelectedImagePath() const;

    /**
     * @brief Loads an existing JSON object into the UI fields for editing.
     * @param objData The JSON object representing a Space Object.
     */
    void loadObjectData(const nlohmann::json& objData);

    /**
     * @brief Configures the dialog for Edit Mode (locks ID field) or Create Mode.
     * @param enable True for Edit Mode, False for Create Mode.
     */
    void setEditMode(bool enable);

    /**
     * @brief Sets a pointer to the current list of objects in memory.
     * Used for live validation of duplicate IDs or names.
     * @param objects Pointer to the vector of existing objects.
     */
    void setExistingObjects(const std::vector<nlohmann::json>* objects);

private slots:
    /**
     * @brief Opens a file dialog to browse for a local image.
     */
    void on_browseImageBtn_clicked();

    /**
     * @brief Validates the form and accepts the dialog if valid.
     */
    void on_saveButton_clicked();

    /**
     * @brief Closes the dialog without saving.
     */
    void on_cancelButton_clicked();

    /**
     * @brief Triggers the plugin manager to auto-fill data based on NORAD ID.
     */
    void on_searchPluginBtn_clicked();

    /**
     * @brief Opens a sub-dialog to select an image already stored in the database.
     */
    void on_selectDBImageBtn_clicked();

private:
    Ui::AddObjectDialog *ui;
    QString m_selectedImagePath;
    SpaceObjectDBManager* m_dbManager = nullptr;
    bool m_isEditMode = false;

    /**
     * @brief Helper to visually style input fields based on validation state.
     * @param widget The widget to style (e.g., QLineEdit).
     * @param state The state string ("error", "warning", or "").
     */
    void setFieldState(QWidget* widget, const QString& state);

    /**
     * @brief Performs live validation of fields as the user types.
     */
    void validateFormLive();

    const std::vector<nlohmann::json>* m_existingObjects = nullptr;
};

#endif // ADDOBJECTDIALOG_H
