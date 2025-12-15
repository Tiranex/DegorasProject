#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#pragma once

#include <QObject>
#include <QMap>
#include <QAction>
#include <QPushButton>
#include <QString>
#include "dpcore_global.h" // Export macro

class DP_CORE_EXPORT ShortcutManager : public QObject {
    Q_OBJECT
public:
    static ShortcutManager& instance();

    // For Menu/Toolbar items (QActions)
    // 'defaultKey' is used when no key is found in .ini settings file.
    // standard id naming: lowercase + '_' (ex: load_dptr_file)
    void registerAction(const QString& id, QAction* action, const QString& defaultKey);

    // For QPushButtons
    // 'defaultKey' is used when no key is found in .ini settings file.
    // standard id naming: lowercase + '_' (ex: load_dptr_file)
    void registerButton(const QString& id, QPushButton* button, const QString& defaultKey);

    // Shortcut updater for MainWindow toolbar option
    void setShortcut(const QString& id, const QKeySequence& newSeq);

    QList<QString> getRegisteredIds() const { return m_registeredActions.keys(); }
    QAction* getAction(const QString& id) const { return m_registeredActions.value(id); }

private:
    // Default private constructor
    ShortcutManager() = default;
    ShortcutManager(const ShortcutManager&) = default;

    // Map: ID -> QAction&
    QMap<QString, QAction*> m_registeredActions;

    // Helper to apply the key
    void applyKey(const QString& id, QAction* action, const QString& defaultKey);
};

#endif // SHORTCUTMANAGER_H
