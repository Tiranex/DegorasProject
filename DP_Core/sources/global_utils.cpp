#include "global_utils.h"
#include "global_texts.h"
#include "window_message_box.h"

#include <QCoreApplication>
#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QLockFile>
#include <QStyleFactory>
#include <QIcon>
#include <QtMath>
#include <QPluginLoader>
#include <QMenu>

void GlobalUtils::initApp(const QString& app_name, const QString& app_error,
                          const QString& app_config, const QString& icon)
{
    // Variables.
    QString globalconfiglink_filename = FILE_GLOBALCONFIGLINK;
    QString globalconfig_filename = FILE_GLOBALCONFIG;
    QString stationdata_filename = FILE_STATIONDATA;
    QString globalconfiglink_filepath, globalconfig_filepath, appconfig_filepath, stationdata_filepath;
    QDir globalconfiglink_dir, globalconfig_dir, appconfig_dir, stationdata_dir;

    // Init names and icons.
    QApplication::setWindowIcon(QIcon(icon));
    QCoreApplication::setOrganizationName(NAME_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(NAME_ORGANIZATION);
    QCoreApplication::setApplicationName(app_name);

    // Load styles, palette, fonts,...
    QPalette palette;
    QFile file(FILE_STYLESHEET_DEFAULT);
    QString stylesheet;
    if (file.open(QFile::ReadOnly)) {
        stylesheet = QLatin1String(file.readAll());
        file.close();
    }
    QStyle *style = QStyleFactory::create("fusion");
    QApplication::setStyle(style);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::ButtonText, Qt::white);
    QApplication::setPalette(palette);
    QFontDatabase::addApplicationFont(":/fonts/Open_Sans/OpenSans-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Open_Sans/OpenSans-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Open_Sans/OpenSans-ExtraBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Open_Sans/OpenSans-SemiBold.ttf");
    QApplication::setFont(QFont("Open Sans", 8, QFont::Normal));
    qApp->setStyleSheet(stylesheet);

    // Locks to check if other instance of the app is running.
    QLockFile lockFile(QDir::temp().absoluteFilePath(app_name+".lock"));
    if(!lockFile.tryLock(100))
    {
        DegorasInformation::showError(app_error, app_name +
                                                    " is already running.\nAllowed to run only one instance of the application.", "",
                                     DegorasInformation::CRITICAL);
        exit(-1);
    }

    // Check and loads the DEGORAS Project configuration file.
    // Get dir and name of the global config file from the global config path file
    /*globalconfiglink_dir.setPath(QApplication::instance()->applicationDirPath());
    globalconfiglink_dir.cdUp();
    globalconfiglink_filepath = globalconfiglink_dir.path()+'/'+globalconfiglink_filename;
    if(!QFile(globalconfiglink_filepath).exists())
    {
        Degor::showError(app_error,
                                     "Global configuration file link '"+globalconfiglink_filename+"' not found in the "
                                                                                                      "deployment directory (check it clicking 'Show Details...').",
                                     globalconfiglink_dir.path(), SalaraInformation::CRITICAL);
        exit(-1);
    }

    // Get the global config file path
    QSettings salara_globalconfig_path(globalconfiglink_dir.path()+"/"+globalconfiglink_filename, QSettings::IniFormat);
    globalconfig_dir.setPath(salara_globalconfig_path.value("SalaraGlobalConfig/GlobalConfigLink").toString());
    globalconfig_filepath = globalconfig_dir.path()+"/"+globalconfig_filename;
    if(!QFile(globalconfig_filepath).exists())
    {
        SalaraInformation::showError(app_error,
                                     "Global configuration file '"+globalconfig_filename+"' not found in the "
                                                                                             "main directory (check it clicking 'Show Details...').", globalconfig_dir.path(),
                                     SalaraInformation::CRITICAL);
        exit(-1);
    }
    SalaraSettings::instance().setGlobalConfig(globalconfig_filepath);

    // Get APP config file.
    appconfig_dir.setPath(SalaraSettings::instance().getGlobalConfigString("SalaraProjectConfigPaths/SP_ConfigFiles"));
    appconfig_filepath = appconfig_dir.path()+"/"+app_config;

    if(!QFile(appconfig_filepath).exists())
    {
        SalaraInformation::showError(app_error, app_name +
                                                    " App configuration file '"+QString(app_config)+"' not found in the config "
                                                                                                        "directory (check it clicking 'Show Details...').", appconfig_dir.path(),
                                     SalaraInformation::CRITICAL);
        exit(-1);
    }
    SalaraSettings::instance().setApplicationConfig(appconfig_filepath);



    // Create all the paths.
    SalaraSettings::instance().getGlobalConfig()->beginGroup("SalaraProjectDataPaths");
    for(auto&& key : SalaraSettings::instance().getGlobalConfig()->allKeys())
        GlobalUtils::createPath(SalaraSettings::instance().getGlobalConfig()->value(key).toString());
    SalaraSettings::instance().getGlobalConfig()->endGroup();
    SalaraSettings::instance().getGlobalConfig()->beginGroup("SalaraProjectConfigPaths");
    for(auto&& key : SalaraSettings::instance().getGlobalConfig()->allKeys())
        GlobalUtils::createPath(SalaraSettings::instance().getGlobalConfig()->value(key).toString());
    SalaraSettings::instance().getGlobalConfig()->endGroup();
    SalaraSettings::instance().getGlobalConfig()->beginGroup("SalaraProjectPluginPaths");
    for(auto&& key : SalaraSettings::instance().getGlobalConfig()->allKeys())
        GlobalUtils::createPath(SalaraSettings::instance().getGlobalConfig()->value(key).toString());
    SalaraSettings::instance().getGlobalConfig()->endGroup();
    SalaraSettings::instance().getGlobalConfig()->beginGroup("SalaraProjectBackupPaths");
    for(auto&& key : SalaraSettings::instance().getGlobalConfig()->allKeys())
        GlobalUtils::createPath(SalaraSettings::instance().getGlobalConfig()->value(key).toString());
    SalaraSettings::instance().getGlobalConfig()->endGroup();

    // Load the station data.
    stationdata_dir.setPath(SalaraSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_StationData"));
    if(!stationdata_dir.exists(stationdata_filename))
    {
        SalaraInformation::showError(app_error, "Station data file '"+stationdata_filename+"' not found in "
                                                                                               "the data directory (check it clicking 'Show Details...').", stationdata_dir.path(),
                                     SalaraInformation::CRITICAL);
        exit(-1);
    }

    SalaraInformation error = SalaraSettings::instance().setStationData(stationdata_dir.path()+"/"+stationdata_filename);
    if(error.hasError())
    {
        error.showErrors(app_error, SalaraInformation::CRITICAL);
        exit(-1);
    } */
}

void GlobalUtils::createPath(const QString& path)
{
    if(!path.isEmpty() && !QDir(path).exists())
        QDir().mkdir(path);
}


void GlobalUtils::clearDir(const QString& path)
{
    QDir dir(path);
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
    foreach( QString dirItem, dir.entryList() )
        dir.remove( dirItem );
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    foreach( QString dirItem, dir.entryList() )
    {
        QDir subDir( dir.absoluteFilePath( dirItem ) );
        subDir.removeRecursively();
    }
}


QString GlobalUtils::toFirstUpper(const QString& string)
{
    QStringList splitter = string.simplified().trimmed().split(" ");
    QString final;
    for(auto&& str : splitter)
    {
        str = str.toLower();
        str[0] = str[0].toUpper();
    }
    final = splitter.join(" ");
    return final;
}

GlobalUtils::ButtonsState GlobalUtils::disableInterfaceButtons(QWidget *widget)
{
    GlobalUtils::ButtonsState buttons_state;
    QList<QPushButton *> list_sources_buttons = widget->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
    for(QPushButton* button : list_sources_buttons)
    {
        buttons_state.insert(button, button->isEnabled());
        button->setEnabled(false);
    }
    return  buttons_state;
}


void GlobalUtils::restoreInterfaceButtons(QWidget *widget, const GlobalUtils::ButtonsState& state)
{
    QList<QPushButton *> list_sources_buttons = widget->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
    for(QPushButton* button : list_sources_buttons)
        button->setEnabled(state[button]);
}


