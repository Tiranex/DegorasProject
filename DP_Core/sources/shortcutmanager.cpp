#include "shortcutmanager.h"
#include "degoras_settings.h" // Settings class
#include <QKeySequence>
#include <QDebug>

ShortcutManager& ShortcutManager::instance() {
    static ShortcutManager inst;
    return inst;
}

void ShortcutManager::registerAction(const QString& id, QAction* action, const QString& defaultKey) {
    if (!action) return;

    // Store it in our map
    m_registeredActions.insert(id, action);

    // Apply the shortcut logic
    applyKey(id, action, defaultKey);
}

void ShortcutManager::registerButton(const QString& id, QPushButton* button, const QString& defaultKey) {
    if (!button) return;

    // TRICK: Create a hidden action that lives inside the button
    QAction* proxyAction = new QAction(button);

    // When the hidden action is triggered (by key), click the button
    connect(proxyAction, &QAction::triggered, button, &QPushButton::animateClick);

    // Now register this proxy action just like a normal one
    registerAction(id, proxyAction, defaultKey);

    // Add the action to the button so it listens to the window context
    button->addAction(proxyAction);
}

void ShortcutManager::applyKey(const QString& id, QAction* action, const QString& defaultKey) {
    // 1. Get the settings instance
    QSettings* cfg = DegorasSettings::instance().config();

    // 2. Look for a saved shortcut. If not found, use defaultKey.
    // We prefix keys with "Shortcuts/" to keep the INI file clean
    QString savedSeq = cfg->value("Shortcuts/" + id, defaultKey).toString();

    // 3. Apply it to the Qt Action
    action->setShortcut(QKeySequence(savedSeq));

    // Optional: Update tooltip to show the user the shortcut (e.g., "Filter [Ctrl+F]")
    if (!action->text().isEmpty()) {
        action->setToolTip(action->text() + " (" + savedSeq + ")");
    }
}
