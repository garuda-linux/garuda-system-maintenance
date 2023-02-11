#include "migrationagent.h"
#include <KNotifications/KStatusNotifierItem>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

#define START_SCRIPT "/usr/lib/garuda-system-maintenance/migrate-dr460nized"
#define DR460NIZED_METADATA "/usr/share/plasma/look-and-feel/Dr460nized/metadata.desktop"

void helpArticle()
{
    QDesktopServices::openUrl(QString("https://wiki.garudalinux.org/en/dr460nized-migration"));
}

void MigrationAgent::createPrompt(QSettings* migration_data)
{
    int reply = 0;
    do {
        QMessageBox dlg(QMessageBox::Warning, tr("Dr460nized theme update required"), tr("KDE Plasma 5.27 is incompatible with latte-dock. A complete theme overhaul that tries to stay faithful to the original has been released and needs to be applied to keep your system working as intended. Backups of your old theme files will be created with the .bak extension. Click help to learn more."), QMessageBox::Yes | QMessageBox::No | QMessageBox::Help);
        dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        reply = dlg.exec();
        if (reply == QMessageBox::Yes) {
            auto process2 = new QProcess(this);
            connect(process2, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [process2](int exitcode, QProcess::ExitStatus status) { helpArticle(); process2->deleteLater(); });
            process2->start(START_SCRIPT, QStringList() << "true");
            migration_data->setValue("dr460nized", 2);
        } else if (reply == QMessageBox::No) {
            migration_data->setValue("dr460nized", 1);
        } else if (reply == QMessageBox::Help) {
            helpArticle();
        }
    } while (reply == QMessageBox::Help);
}

void MigrationAgent::onCheckComplete(QSettings* migration_data, QProcess* process, int exitcode, QProcess::ExitStatus status)
{
    process->deleteLater();
    if (status == QProcess::ExitStatus::NormalExit && exitcode == 0) {
        createPrompt(migration_data);
    }
    migration_data->deleteLater();
}

void MigrationAgent::onActionClicked()
{
    QSettings migration_data("garuda", "migrations");
    createPrompt(&migration_data);
}

void MigrationAgent::onRoutine()
{
    if (once)
        return;
    once = true;

    auto migration_data = new QSettings("garuda", "migrations");
    if (migration_data->value("dr460nized", 0).toInt() > 0)
        return;

    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, std::bind(&MigrationAgent::onCheckComplete, this, migration_data, process, std::placeholders::_1, std::placeholders::_2));
    process->start(START_SCRIPT, QStringList());
}

MigrationAgent::MigrationAgent(ManagerData& data)
    : BaseAgent(data)
{
    // This isn't actually dr460nized-next yet
    if (qgetenv("XDG_CURRENT_DESKTOP") != "KDE" || !QFile::exists(DR460NIZED_METADATA)) {
        // We don't want the routine to run either.
        once = true;
        return;
    }

    QSettings migration_data("garuda", "migrations");

    if (migration_data.value("dr460nized", 0).toInt() < 2) {
        auto menu = data.trayicon->contextMenu();
        auto actions = menu->actions();
        QAction* applyAction = new QAction(
            QIcon::fromTheme("update"),
            "Dr460nized migration",
            menu);
        connect(applyAction, &QAction::triggered, this, &MigrationAgent::onActionClicked);
        menu->insertAction(actions.back(), applyAction);
    }
}
