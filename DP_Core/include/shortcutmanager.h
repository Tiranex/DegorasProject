#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#pragma once

#include <QObject>
#include <QMap>
#include <QAction>
#include <QPushButton>
#include <QString>

class ShortcutManager : public QObject {
    Q_OBJECT
public:
    static ShortcutManager& instance();

    // Use this for Menu/Toolbar items
    void registerAction(const QString& id, QAction* action, const QString& defaultKey);

    // Use this for Buttons on the screen
    void registerButton(const QString& id, QPushButton* button, const QString& defaultKey);

private:
    ShortcutManager() = default; // Private constructor

    // Keep track of actions so we can update them later if needed
    QMap<QString, QAction*> m_registeredActions;

    // Helper to apply the key
    void applyKey(const QString& id, QAction* action, const QString& defaultKey);
};

#endif // SHORTCUTMANAGER_H
