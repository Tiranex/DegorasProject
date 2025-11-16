#include "window_about_dialog.h"
#include "forms/ui_form_about_dialog.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QDebug> // For logging errors

// Define resource paths here or in a central header
#define SALARA_LICENSE_PATH ":/salara_license.txt"
#define SALARA_ABOUT_PATH   ":/salara_about.json"
#define SALARA_LOGO_PATH    ":/salara_logo.png"


AboutDialog::AboutDialog(const QString& license_resource_path, const QString& about_resource_path,
                         const QIcon& icon, QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // --- Set License text with error handling ---
    QFile licenseFile(license_resource_path);
    if (!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open license file:" << license_resource_path;
        this->ui->pte_license->setPlainText("License information is not available.");
    } else {
        this->ui->pte_license->setPlainText(licenseFile.readAll());
        licenseFile.close();
    }


    // --- Set About text with error handling ---
    QFile aboutFile(about_resource_path);
    if (!aboutFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open about file:" << about_resource_path;
    } else {
        QJsonParseError parseError;
        QJsonDocument aboutDoc = QJsonDocument::fromJson(aboutFile.readAll(), &parseError);
        aboutFile.close();

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Failed to parse about JSON:" << parseError.errorString();
        } else if (aboutDoc.isObject() && aboutDoc.object().contains("About")) {
            QJsonObject aboutObject = aboutDoc["About"].toObject();

            // Use a C++11 range-based for loop for better readability
            for (const QString& key : aboutObject.keys()) {
                // Using value() is slightly safer than operator[]
                QJsonValue val = aboutObject.value(key);

                if (key == "DisplayName") {
                    this->setWindowTitle(val.toString("About")); // Provide a default
                } else {
                    // Create and add labels to the form layout
                    auto* keyLabel = new QLabel(key, this);
                    auto* valueLabel = new QLabel(val.toString(), this);
                    // Make the value selectable by mouse for easy copy-pasting
                    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
                    this->ui->fl_about->addRow(keyLabel, valueLabel);
                }
            }
        } else {
            qWarning() << "About JSON is missing the root 'About' object.";
        }
    }


    // --- Set icon ---
    this->ui->lb_logo->setPixmap(icon.pixmap(QSize(200, 200)));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::aboutSalaraProject(QWidget *parent)
{
    // The dialog is created on the stack and will be destroyed when it goes out of scope.
    AboutDialog aboutDialog(SALARA_LICENSE_PATH, SALARA_ABOUT_PATH, QIcon(SALARA_LOGO_PATH), parent);
    // about_salara.setEnabled(true); // This call is redundant and can be removed.
    aboutDialog.exec();
}
