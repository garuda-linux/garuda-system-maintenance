#include "tray.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLockFile>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("garuda-system-maintenance");
    a.setOrganizationName("garuda");
    a.setApplicationDisplayName("Garuda System Maintenance");

    QCommandLineParser cmdline;
    QCommandLineOption settings( "settings",
                                    "Open Garuda System Maintenance settings" );
    cmdline.addOption( settings );
    cmdline.process( a );

    QWidget *main;
    QLockFile lock(QDir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)).absoluteFilePath("garuda-system-maintenance.lock"));

    if (cmdline.isSet(settings))
    {
        main = new SettingsDialog;
        main->show();
    }
    else
    {
        if (lock.tryLock(1000))
            main = new Tray;
        else
            return 1;
    }
    return a.exec();
}
