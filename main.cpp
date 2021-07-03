#include "tray.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("garuda-system-maintenance");
    a.setOrganizationName("garuda");
    Tray t;
    return a.exec();
}
