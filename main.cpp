#include "tray.h"

#include <QApplication>
#include <QCommandLineParser>

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

    if (cmdline.isSet(settings))
    {
        main = new SettingsDialog;
        main->show();
    }
    else
        main = new Tray;
    return a.exec();
}
