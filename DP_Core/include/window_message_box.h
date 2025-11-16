#pragma once

#include <QObject>
#include <QMessageBox>

#include "dpcore_global.h"

class DP_CORE_EXPORT DegorasInformation
{

public:

    enum MessageTypeEnum
    {
        CRITICAL = QMessageBox::Critical,
        WARNING = QMessageBox::Warning,
        INFO = QMessageBox::Information
    };

    typedef QPair<int, QString> ErrorPair;
    typedef QList<ErrorPair> ErrorList;

    DegorasInformation(const ErrorPair& error_pair, const QString& detailed = ""):
        error_list({error_pair}), detailed(detailed){}
    DegorasInformation(const ErrorList& error_list):error_list(error_list){}
    DegorasInformation() = default;
    DegorasInformation(const DegorasInformation&) = default;
    DegorasInformation(DegorasInformation&&) = default;
    DegorasInformation& operator =(DegorasInformation&&) = default;
    DegorasInformation& operator =(const DegorasInformation&) = default;

    inline void append(const DegorasInformation& other) {this->error_list.append(other.error_list);}

    bool containsError(int error_code) const;

    inline bool hasError() const {return  !error_list.isEmpty();}
    inline const ErrorList& getErrors() const {return  error_list;}

    void showErrors(const QString &box_title = "", MessageTypeEnum type = INFO,
                    const QString &error_text = "", QWidget* parent = nullptr) const;

    // Static methods.
    static void showError(const QString &box_title = "", const QString& error ="", const QString& detailed = "",
                          MessageTypeEnum type = INFO, QWidget *parent = nullptr);

    static void showInfo(const QString &box_title = "", const QString& info ="", const QString &detailed = "",
                         QWidget *parent = nullptr);
    static void showWarning(const QString &box_title = "", const QString& warning ="", const QString& detailed = "",
                            QWidget *parent = nullptr);
    static void showCritical(const QString &box_title = "", const QString& warning ="", const QString& detailed = "",
                             QWidget *parent = nullptr);

private:
    ErrorList error_list;
    QString detailed;
};
