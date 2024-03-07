#include "updateagent.h"

#include <KNotification>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

#define PARTIAL_UPGRADE_WATCH_FOLDER "/var/lib/garuda"
#define PARTIAL_UPGRADE_PATH PARTIAL_UPGRADE_WATCH_FOLDER "/partial_upgrade"
#define LAST_UPDATE_PATH PARTIAL_UPGRADE_WATCH_FOLDER "/last_update"

void UpdateAgent::onRoutine(bool init)
{
    if (isSystemCriticallyOutOfDate()) {
        if (settings.value("timestamps/systemupdate-alert", QDateTime::fromSecsSinceEpoch(9, Qt::UTC)).toDateTime().daysTo(QDateTime::currentDateTimeUtc()) > 2) {
            settings.setValue("timestamps/systemupdate-alert", QDateTime::currentDateTimeUtc());
            updateApplicationState();
            KNotification* notification = new KNotification("general", KNotification::Persistent);
            notification->setTitle(tr("System out of date"));
            notification->setText(tr("This system has not been updated in a long time.\nRegularly applying system updates on a rolling release distribution is highly encouraged to avoid various issues."));
            KNotificationAction *actionupdate = notification->addAction({ tr("Update system") });
            KNotificationAction *actionignore = notification->addAction({ tr("Disable warnings") });
            connect(actionupdate, &KNotificationAction::activated, this, [this]() { launchSystemUpdate(); });
            connect(actionignore, &KNotificationAction::activated, this, [this]() { settings.setValue("application/partialupgrade", false); });
            notification->sendEvent();
        }
    } else if (settings.contains("timestamps/systemupdate-alert"))
        settings.remove("timestamps/systemupdate-alert");

    updateApplicationState();
}

void UpdateAgent::updateApplicationState()
{
    if (isPartiallyUpgraded() || (isSystemCriticallyOutOfDate() && settings.contains("timestamps/systemupdate-alert"))) {
        click_priority = 5;
    } else {
        click_priority = 0;
    }
}

void UpdateAgent::launchSystemUpdate()
{
    QProcess::startDetached("/usr/lib/garuda/launch-terminal", QStringList() << "garuda-update; read -p 'Press enter to exit'");
}

UpdateAgent::UpdateAgent(ManagerData& data)
    : BaseAgent(data)
{
    watcher = new QFileSystemWatcher(this);
    watcher->addPath(PARTIAL_UPGRADE_WATCH_FOLDER);

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        QTimer::singleShot(1000, this, [this, path]() {
            if (path == PARTIAL_UPGRADE_WATCH_FOLDER) {
                updateApplicationState();
                if (isPartiallyUpgraded()) {
                    KNotification* notification = new KNotification("general", KNotification::Persistent);
                    notification->setTitle("Partial upgrade detected");
                    notification->setText("You performed a \"partial upgrade\". Please fully update your system to prevent system instability.\nPerforming partial upgrades is unsupported.");
                    KNotificationAction *actionupdate = notification->addAction({ tr("Update system") });
                    KNotificationAction *actionlearnmore = notification->addAction({ tr("Learn more") });
                    KNotificationAction *actionignore = notification->addAction({ tr("Disable warnings") });
                    connect(actionupdate, &KNotificationAction::activated, this, [this]() { launchSystemUpdate(); });
                    connect(actionlearnmore, &KNotificationAction::activated, []() { QDesktopServices::openUrl(QUrl("https://wiki.garudalinux.org/en/partial-upgrade")); });
                    connect(actionignore, &KNotificationAction::activated, this, [this]() { settings.setValue("application/partialupgrade", false); });
                    notification->sendEvent();
                }
            }
        });
    });
}

UpdateAgent::~UpdateAgent()
{
    delete watcher;
}

void UpdateAgent::trayIconClicked()
{
    bool outofdate = isSystemCriticallyOutOfDate(), partial = false;
    if (!outofdate)
        partial = isPartiallyUpgraded();
    if (outofdate || partial) {
        QMessageBox dlg;
        dlg.setWindowTitle(outofdate ? tr("System out of date") : tr("Partial upgrade detected"));
        dlg.setText(outofdate ? tr("This system has not been updated in a long time.\nRegularly applying system updates on a rolling release distribution is highly encouraged to avoid various issues.") : tr("You performed a \"partial upgrade\". Please fully update your system to prevent system instability.\nPerforming partial ugprades is unsupported."));

        dlg.addButton(QMessageBox::Cancel);

        auto* apply = dlg.addButton(QMessageBox::Apply);
        apply->setText(tr("Update system"));

        QPushButton* learnmore = nullptr;
        if (partial) {
            learnmore = dlg.addButton(QMessageBox::Help);
            learnmore->setText(tr("Learn more"));
        }

        auto* disablenotifications = dlg.addButton(QMessageBox::Ignore);
        disablenotifications->setText(tr("Ignore permanently"));

        dlg.exec();
        if (dlg.clickedButton() == nullptr)
            return;
        if (dlg.clickedButton() == learnmore) {
            QDesktopServices::openUrl(QString("https://wiki.garudalinux.org/en/partial-upgrade"));
        } else if (dlg.clickedButton() == apply) {
            launchSystemUpdate();
        } else if (dlg.clickedButton() == disablenotifications) {
            if (outofdate)
                settings.setValue("application/outofdate", false);
            else
                settings.setValue("application/partialupgrade", false);
        }
    }
}

bool UpdateAgent::isPartiallyUpgraded()
{
    return settings.value("application/partialupgrade", true).toBool() && QFile::exists(PARTIAL_UPGRADE_PATH);
}

bool UpdateAgent::isSystemCriticallyOutOfDate()
{
    if (!settings.value("application/outofdate", true).toBool())
        return false;

    auto update_file = QFileInfo(LAST_UPDATE_PATH);
    if (!update_file.exists())
        return false;
    auto timestamp = update_file.lastModified().toUTC();
    if (timestamp.daysTo(QDateTime::currentDateTimeUtc()) >= 13) {
        if (settings.contains("timestamps/systemupdate")) {
            if (settings.value("timestamps/systemupdate").toDateTime().daysTo(QDateTime::currentDateTimeUtc()) >= 2)
                return true;
        } else
            settings.setValue("timestamps/systemupdate", QDateTime::currentDateTimeUtc());
    } else {
        settings.remove("timestamps/systemupdate");
    }
    return false;
}
