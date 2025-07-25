#include "snapshotagent.h"

#include <KNotification>
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

void SnapshotAgent::onRoutine(bool init)
{
    if (once || !settings.value("application/oldsnapshot", true).toBool())
        return;
    once = true;

    QProcess* process = new QProcess(this);
    process->start("pkexec", { "/usr/bin/snapper-tools", "find-old" });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) {
        process->deleteLater();
        if (exitcode == 0) {
            text = QString::fromUtf8(process->readAllStandardOutput());
            qDebug() << text;
            KNotification* notification = new KNotification("general", KNotification::Persistent);
            notification->setTitle(tr("Old snapshots/backups found"));
            notification->setText(tr("Old snapshots or snapshot restore backups have been found that are using up disk space.\n"));
            KNotificationAction *actiondelete = notification->addAction({ tr("View and delete") });
            KNotificationAction *actionignore = notification->addAction({ tr("Disable notifications") });
            connect(actiondelete, &KNotificationAction::activated, this, [this]() {
                // KNotification doesn't like it if you create a blocking message box right away. So we just push it back onto the event loop.. I guess.
                QTimer::singleShot(0, this, [this]() {
                    QMessageBox dlg;
                    dlg.setWindowTitle(tr("Old snapshots"));
                    dlg.setText(text);
                    dlg.addButton(QMessageBox::Cancel);
                    auto* apply = dlg.addButton(QMessageBox::Apply);
                    apply->setText(tr("Delete"));
                    dlg.exec();
                    if (dlg.clickedButton() == apply) {
                        deleteOld();
                    }
                });
            });
            connect(actionignore, &KNotificationAction::activated, this, [this]() { disableWarnings(); });
            notification->sendEvent();
            click_priority = 1;
        }
    });
}

void SnapshotAgent::disableWarnings()
{
    settings.setValue("application/oldsnapshot", false);
    click_priority = 0;
}

void SnapshotAgent::deleteOld()
{
    QProcess::startDetached("pkexec", { "/usr/bin/snapper-tools", "delete-old" });
    text.clear();
    click_priority = 0;
}

void SnapshotAgent::trayIconClicked()
{
    QMessageBox dlg;
    dlg.setWindowTitle(tr("Old snapshots/backups found!"));
    dlg.setText(text);
    dlg.addButton(QMessageBox::Cancel);
    auto* apply = dlg.addButton(QMessageBox::Apply);
    apply->setText(tr("Delete"));
    auto* disablenotifications = dlg.addButton(QMessageBox::Ignore);
    disablenotifications->setText(tr("Ignore permanently"));
    dlg.exec();
    if (dlg.clickedButton() == nullptr)
        return;
    if (dlg.clickedButton() == apply) {
        deleteOld();
    } else if (dlg.clickedButton() == disablenotifications) {
        disableWarnings();
    }
}
