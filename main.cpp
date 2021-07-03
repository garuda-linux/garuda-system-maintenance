#include "tray.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("garuda-system-maintenance");
    a.setOrganizationName("garuda");
    a.setApplicationDisplayName("Garuda System Maintenance");
    Tray t;
    return a.exec();
}
