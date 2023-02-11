#ifndef TRAY_H
#define TRAY_H

#include "agentmanager.h"
#include "settingsdialog.h"
#include <KNotifications/KStatusNotifierItem>
#include <QMainWindow>

class QFileSystemWatcher;

QT_BEGIN_NAMESPACE
namespace Ui {
class Tray;
}
QT_END_NAMESPACE

class Tray : public QWidget {
    Q_OBJECT

public:
    Tray(QWidget* parent = nullptr);
    ~Tray();

private:
    Ui::Tray* ui;
    KStatusNotifierItem* trayicon;
    SettingsDialog* dialog = nullptr;
    AgentManager manager;
    QSettings settings;
    QFileSystemWatcher* watcher;

    void onReloadSettings();
    void setup();
    void showSettings();
};
#endif // TRAY_H
