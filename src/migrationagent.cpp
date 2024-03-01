#include "migrationagent.h"
#include <QDesktopServices>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QUrl>
#include <KNotifications/KStatusNotifierItem>

#define START_SCRIPT "/usr/lib/garuda-system-maintenance/migrate-dr460nized"
#define DR460NIZED_METADATA "/usr/share/sddm/themes/Dr460nized"

void helpArticle()
{
    QDesktopServices::openUrl(QString("https://wiki.garudalinux.org/en/dr460nized-plasma6-migration"));
}

void MigrationAgent::createPrompt(QSettings* migration_data)
{
    bool help;
    do {
        help = false;
        QMessageBox dlg;
        dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        dlg.setWindowTitle(tr("Dr460nized theme update available"));
        dlg.setText(tr("KDE Plasma 6 and the accompanying Dr460nized theme version has been released! Some previous theme elements are unavailable or have changed, therefore re-applying the theme is necessary. Apply update now? (This will remove any custom plasma theme configurations)"));
        auto* cancel = dlg.addButton(QMessageBox::Cancel);
        cancel->setText(tr("Later"));
        auto* apply = dlg.addButton(QMessageBox::Apply);
        auto* disablenotifications = dlg.addButton(QMessageBox::Ignore);
        disablenotifications->setText(tr("Ignore permanently"));
        auto* helpbutton = dlg.addButton(QMessageBox::Help);
        helpbutton->setText(tr("More Info/Details"));
        dlg.exec();
        if (dlg.clickedButton() == apply) {
            auto process2 = new QProcess(this);
            connect(process2, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [process2](int exitcode, QProcess::ExitStatus status) { process2->deleteLater(); });
            process2->start(START_SCRIPT, QStringList() << "true");
            migration_data->setValue("dr460nized", 4);
            click_priority = 0;
        } else if (dlg.clickedButton() == disablenotifications) {
            migration_data->setValue("dr460nized", 3);
            click_priority = 0;
        } else if (dlg.clickedButton() == helpbutton) {
            helpArticle();
            help = true;
        } else {
            click_priority = 2;
        }
    } while (help);
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

void MigrationAgent::trayIconClicked()
{
    onActionClicked();
}

void MigrationAgent::onRoutine(bool init)
{
    if (once)
        return;
    once = true;

    auto migration_data = new QSettings("garuda", "migrations");

    if (init)
        migration_data->setValue("dr460nized", 4);

    if (migration_data->value("dr460nized", 0).toInt() > 2)
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
    if (qgetenv("KDE_SESSION_VERSION") != "6" || !QFile::exists(DR460NIZED_METADATA)) {
        // We don't want the routine to run either.
        once = true;
        return;
    }

    QSettings migration_data("garuda", "migrations");

    if (migration_data.value("dr460nized", 0).toInt() < 4) {
        auto menu = data.trayicon->contextMenu();
        auto actions = menu->actions();
        QAction* applyAction = new QAction(
            QIcon::fromTheme("update"),
            tr("Dr460nized migration"),
            menu);
        connect(applyAction, &QAction::triggered, this, &MigrationAgent::onActionClicked);
        menu->insertAction(actions.back(), applyAction);
    }
}
