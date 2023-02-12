#include "tray.h"
#include "./ui_tray.h"
#include <QFile>
#include <QFileSystemWatcher>

Tray::Tray(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Tray)
{
    setup();
}

Tray::~Tray()
{
    delete ui;
}

void Tray::setup()
{
    trayicon = new KStatusNotifierItem(this);
    trayicon->setIconByName("garuda-system-maintenance");

    QMenu* menu = trayicon->contextMenu();

    QAction* settingsAction = new QAction(
        QIcon::fromTheme("garuda-system-maintenance"),
        "Settings",
        menu);

    // menu->addAction(keyringAction);
    // menu->addAction(hotfixAction);
    // menu->addSeparator();
    menu->addAction(settingsAction);

    // connect(keyringAction, &QAction::triggered, [this]() { updateKeyring(true); });
    // connect(hotfixAction, &QAction::triggered, [this]() { updateKeyring(false, true); });
    connect(settingsAction, &QAction::triggered, this, [this]() { showSettings(); });

    trayicon->setAssociatedWidget(nullptr);
    connect(trayicon, &KStatusNotifierItem::activateRequested, this, [this]() {
        if (!manager.onTrayIconClick())
            showSettings();
    });

    watcher = new QFileSystemWatcher(this);
    watcher->addPath(settings.fileName());
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        if (!watcher->files().contains(path) && QFile::exists(path))
            watcher->addPath(path);
        if (path == settings.fileName())
            onReloadSettings();
    });

    manager.init(settings, trayicon, [this](int priority) {
        if (priority > 0) {
            trayicon->setStatus(KStatusNotifierItem::NeedsAttention);
            trayicon->setIconByName("garuda-system-maintenance-alert");
        } else {
            trayicon->setStatus(KStatusNotifierItem::Passive);
            trayicon->setIconByName("garuda-system-maintenance");
        }
    });
}

void Tray::showSettings()
{
    if (!dialog) {
        dialog = new SettingsDialog(this);
        connect(dialog, &QWidget::destroyed, this, [this]() {
            dialog = nullptr;
            // Handled by the inotify watcher
            // onReloadSettings();
        });
        dialog->show();
    } else {
        dialog->deleteLater();
    }
}

void Tray::onReloadSettings()
{
    settings.sync();
    manager.onSettingsReloaded();
}
