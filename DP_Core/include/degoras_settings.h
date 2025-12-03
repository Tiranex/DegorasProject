#pragma once

#include <QSettings>
#include <QString>
#include <memory> // unique_ptr
#include "dpcore_global.h" // Export macro

class DP_CORE_EXPORT DegorasSettings {
public:
    // Singleton
    static DegorasSettings& instance();
    // Call in main
    void initialize(const QString& filePath);
    // Access QSettings object
    QSettings* config();

    inline QString getGlobalConfigString(const QString& key) const { return "hola"; } // Legacy code does not build without it
private:
    // Smart pointer
    std::unique_ptr<QSettings> m_appSettings;

    // Constructors and operators to private to enforce singleton
    DegorasSettings() = default;
    DegorasSettings(const DegorasSettings&) = delete;
    DegorasSettings& operator =(const DegorasSettings&) = delete;
    DegorasSettings(const DegorasSettings&&) = delete;
    DegorasSettings& operator =(const DegorasSettings&&) = delete;
};
