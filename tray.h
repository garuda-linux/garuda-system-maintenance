#ifndef TRAY_H
#define TRAY_H

#include <QMainWindow>
#include <QProcess>
#include <QSettings>
#include <QTimer>

class QSystemTrayIcon;
class QNetworkAccessManager;

class Tray : public QMainWindow {
    Q_OBJECT

    QSystemTrayIcon* trayicon = nullptr;
    void updateKeyring(bool hotfixes = false);
    void checkUpdates();
    void checkPackage(QString package, QNetworkAccessManager *mgr, std::function<void(bool success)> next);
    bool busy = false;

    QTimer timer;
    QSettings settings;

private slots:
    void onKeyringsInstalled(int, QProcess::ExitStatus, QProcess* process, bool hotfix);
    void onCheckUpdatesComplete(int, QProcess::ExitStatus, QProcess* process);
    void onShouldCheckPackages();

public:
    Tray(QWidget* parent = nullptr);
    ~Tray();
};
#endif // TRAY_H
