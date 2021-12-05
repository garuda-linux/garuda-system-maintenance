#ifndef TRAY_H
#define TRAY_H

#include <QMainWindow>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <KNotifications/KStatusNotifierItem>
#include <QPointer>
#include "settingsdialog.h"

class QSystemTrayIcon;
class QNetworkAccessManager;
class SettingsDialog;
class QFileSystemWatcher;

class Tray : public QMainWindow {
    Q_OBJECT

    KStatusNotifierItem* trayicon = nullptr;
    QFileSystemWatcher* watcher = nullptr;
    void updateKeyring(bool keyring, bool hotfixes = false);
    void checkUpdates();
    void checkPackage(QString package, QNetworkAccessManager *mgr, std::function<void(bool success)> next);
    void showSettings();
    void updateApplicationState();
    bool partialUpgrade();
    bool isSystemCriticallyOutOfDate();
    void launchSystemUpdate();
    bool busy = false;

    QTimer package_timer;
    QTimer forum_timer;
    QSettings settings;

    QPointer<SettingsDialog> dialog = nullptr;
private slots:
    void onKeyringsInstalled(int, QProcess::ExitStatus, QProcess* process, bool hotfix);
    void onCheckUpdatesComplete(int, QProcess::ExitStatus, QProcess* process);
    void onShouldCheckPackages();
    void onCheckForum();
    void onReloadSettings();

public:
    Tray(QWidget* parent = nullptr);
    ~Tray();
};
#endif // TRAY_H
