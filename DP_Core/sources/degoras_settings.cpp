#include "degoras_settings.h"
#include <QFileInfo>
#include <QDebug> // Replace with Logs (?)

DegorasSettings& DegorasSettings::instance()
{
    static DegorasSettings _instance;
    return _instance;
}

void DegorasSettings::initialize(const QString& filePath)
{
    m_appSettings = std::make_unique<QSettings>(filePath, QSettings::IniFormat);
}

QSettings* DegorasSettings::config()
{
    if(!m_appSettings)
    {
        qWarning() << "Degoras Settings accessed before initialization. Call 'initialize()' in main.";
        return nullptr;
    }
    return m_appSettings.get();
}
