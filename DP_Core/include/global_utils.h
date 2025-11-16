#pragma once

#include <QString>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QDateTime>
#include <QMap>
#include <QPushButton>
#include <QWidget>

#include "dpcore_global.h"

class DP_CORE_EXPORT GlobalUtils
{
public:

    // Typedefs and static containers.
    typedef QHash<QPushButton*, bool> ButtonsState;

    // Static class. Delete constructor.
    GlobalUtils() = delete;

    // Init app functions.
    static void initApp(const QString& app_name, const QString& app_error,
                        const QString& app_config, const QString& icon);

    // Functions to disable and enable sets of buttons.
    static ButtonsState disableInterfaceButtons(QWidget* widget);
    static void restoreInterfaceButtons(QWidget* widget, const ButtonsState& state);

    // Path and directories functions.
    static void createPath(const QString &path);
    static void clearDir(const QString& path);

    // Others.
    static QString toFirstUpper(const QString &string);

};
