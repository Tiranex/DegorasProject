#include "window_message_box.h"
#include "global_texts.h"


bool DegorasInformation::containsError(int error_code) const
{
    auto it = std::find_if(this->error_list.begin(), this->error_list.end(),
                           [error_code](const ErrorPair& error_pair)
                           {
                               return error_pair.first == error_code;
                           });
    return it != this->error_list.end();
}


void DegorasInformation::showErrors(const QString& box_title, MessageTypeEnum type,
                                   const QString& error_text, QWidget *parent) const
{
    if(this->error_list.size()==1)
    {
        QString error = error_list.first().second;
        QMessageBox messagebox(static_cast<QMessageBox::Icon>(type), box_title, error,
                               QMessageBox::StandardButton::Ok, parent);
        messagebox.setDetailedText(this->detailed);
        messagebox.exec();
    }
    else if(this->error_list.size()>1)
    {
        QString detailed, error_title;

        error_title = error_text.isEmpty() ? TEXT_ERRORS_GENERIC : error_text;

        QMessageBox messagebox(static_cast<QMessageBox::Icon>(type), box_title, error_title,
                               QMessageBox::StandardButton::Ok, parent);
        for (const auto& error : error_list)
            detailed += error.second+"\n\n";
        detailed.chop(2);
        messagebox.setDetailedText(detailed);
        messagebox.exec();
    }
}

void DegorasInformation::showError(const QString &box_title, const QString &error, const QString &detailed_text,
                                  DegorasInformation::MessageTypeEnum type, QWidget *parent)
{
    QMessageBox messagebox(static_cast<QMessageBox::Icon>(type), box_title, error,
                           QMessageBox::StandardButton::Ok, parent);
    if(!detailed_text.isEmpty())
        messagebox.setDetailedText(detailed_text);
    messagebox.exec();
}

void DegorasInformation::showInfo(const QString &box_title, const QString &info, const QString& detailed, QWidget *parent)
{
    DegorasInformation::showError(box_title, info, detailed, INFO, parent);
}

void DegorasInformation::showWarning(const QString &box_title, const QString &warning, const QString &detailed, QWidget *parent)
{
    DegorasInformation::showError(box_title, warning, detailed, WARNING, parent);
}

void DegorasInformation::showCritical(const QString &box_title, const QString &warning, const QString &detailed, QWidget *parent)
{
    DegorasInformation::showError(box_title, warning, detailed, CRITICAL, parent);
}
