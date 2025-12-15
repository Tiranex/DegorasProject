#include "shortcutmanager.h"
#include "degoras_settings.h" // Settings class
#include <QKeySequence>
#include <QDebug>

ShortcutManager& ShortcutManager::instance()
{
    static ShortcutManager inst;
    return inst;
}

void ShortcutManager::applyKey(const QString& id, QAction* action, const QString& defaultKey)
{
    // 1. Get the settings instance
    QSettings* cfg = DegorasSettings::instance().config();

    // 2. Look for a saved shortcut. If not found, use defaultKey
    QString savedSeq = cfg->value("Shortcuts/" + id, defaultKey).toString();

    // 3. Apply it to the Qt Action
    action->setShortcut(QKeySequence(savedSeq));

    // Update tooltip to show the user the shortcut
    if (!action->text().isEmpty()) {
        action->setToolTip(action->text() + " (" + savedSeq + ")");
    }
}

void ShortcutManager::registerAction(const QString& id, QAction* action, const QString& defaultKey)
{
    if (!action) return;

    // Store it in our map
    m_registeredActions.insert(id, action);

    // Apply the shortcut logic
    applyKey(id, action, defaultKey);
}

void ShortcutManager::registerButton(const QString& id, QPushButton* button, const QString& defaultKey)
{
    if (!button) return;

    // Workaround: Create a hidden action that lives inside the button
    QAction* proxyAction = new QAction(button);
    connect(proxyAction, &QAction::triggered, button, &QPushButton::animateClick);

    // Call registerAction() with proxyAction, who represents the QPushButton
    registerAction(id, proxyAction, defaultKey);

    // Add the action to the button so it listens to the window context
    // "While this button (or its parent) is active, listen to the specific key associated with 'proxyAction'
    button->addAction(proxyAction);

    // Chain Reaction: keyboard shortcut -> button object notifies action -> action triggered -> button clicked (via connect)
}

void ShortcutManager::setShortcut(const QString& id, const QKeySequence& newSeq)
{
    // 1. Update map
    if (m_registeredActions.contains(id)) {
        QAction* act = m_registeredActions[id];
        act->setShortcut(newSeq);

        // Update tooltip
        QString cleanName = act->text();
        cleanName.remove('&');
        act->setToolTip(cleanName + " (" + newSeq.toString() + ")");
    }

    // 2. Save the change to the .ini file via DegorasSettings
    QSettings* config = DegorasSettings::instance().config();

    config->setValue("Shortcuts/" + id, newSeq.toString());
    config->sync();
}
