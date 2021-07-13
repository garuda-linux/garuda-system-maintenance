#include "tray.h"
#include <KNotifications/KNotification>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <functional>
#include <QFile>

constexpr int KEYRING_FIRST_CHECK_TIME = 60 * 1000;
constexpr int KEYRING_CHECK_TIME = 30 * 60 * 1000;

// https://bugreports.qt.io/browse/QTBUG-44944
// https://stackoverflow.com/questions/54461719/sort-a-qjsonarray-by-one-of-its-child-elements/54461720
inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

Tray::Tray(QWidget* parent)
    : QMainWindow(parent)
{
    if (!settings.value("application/firstrun", false).toBool()) {
        settings.setValue("application/firstrun", true);
        settings.setValue("timestamps/forum", QDateTime::currentDateTimeUtc().toString(Qt::DateFormat::ISODateWithMs));
    }

    trayicon = new KStatusNotifierItem(this);
    trayicon->setIconByName("garuda-system-maintenance");

    QMenu* menu = trayicon->contextMenu();

    QAction* keyringAction = new QAction(
        QIcon::fromTheme("update"),
        "Update keyring",
        menu);
    QAction* hotfixAction = new QAction(
        QIcon::fromTheme("update"),
        "Force hotfix update",
        menu);

    menu->addAction(keyringAction);
    menu->addAction(hotfixAction);

    connect(keyringAction, &QAction::triggered, [this]() { updateKeyring(); });
    connect(hotfixAction, &QAction::triggered, [this]() { updateKeyring(true); });

    trayicon->setStatus(KStatusNotifierItem::Passive);
    trayicon->setAssociatedWidget(nullptr);
    connect(trayicon, &KStatusNotifierItem::activateRequested, [this]() {
        if (!dialog) {
            dialog = new SettingsDialog(this);
            connect(dialog, SIGNAL(destroyed()), this, SLOT(onReloadSettings()));
            dialog->show();
        } else
            dialog->deleteLater();
    });

    connect(&package_timer, &QTimer::timeout, this, &Tray::onShouldCheckPackages);
    package_timer.setSingleShot(true);
    connect(&forum_timer, &QTimer::timeout, this, &Tray::onCheckForum);
    forum_timer.setSingleShot(true);

    watcher = new QFileSystemWatcher(this);
    watcher->addPath(settings.fileName());
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        if (!watcher->files().contains(path) && QFile::exists(path))
            watcher->addPath(path);
        onReloadSettings();
    });

    onReloadSettings();
}

void Tray::updateKeyring(bool hotfixes)
{
    if (busy)
        return;
    busy = true;
    package_timer.stop();

    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process, hotfixes](int exitcode, QProcess::ExitStatus status) { onKeyringsInstalled(exitcode, status, process, hotfixes); });

    if (hotfixes) {
        QMessageBox dlg(QMessageBox::Warning, tr("Hotfix available!"), tr("Important hotfix update found. Do you want to apply the hotfix (recommended)?"), QMessageBox::Yes | QMessageBox::No, this);
        dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
        auto reply = dlg.exec();
        if (reply == QMessageBox::Yes) {
            process->start("systemctl", QStringList() << "start"
                                                      << "garuda-system-maintenance@keyring-hotfixes.service");
            return;
        }
    }
    trayicon->showMessage("Garuda System Maintenance", "Updating keyrings in the background...", "garuda-system-maintenance");
    process->start("systemctl", QStringList() << "start"
                                              << "garuda-system-maintenance@keyring.service");
}

void Tray::checkUpdates()
{
    if (busy)
        return;
    busy = true;
    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) { onCheckUpdatesComplete(exitcode, status, process); });
    process->start("checkupdates", QStringList());
}

void Tray::onKeyringsInstalled(int exitcode, QProcess::ExitStatus status, QProcess* process, bool hotfix)
{
    if (status == QProcess::ExitStatus::NormalExit && exitcode == 0) {
        trayicon->showMessage("Garuda System Maintenance", hotfix ? "Hotfix successfully applied!" : "Keyrings successfully updated!", "garuda-system-maintenance");
    } else {
        trayicon->showMessage("Garuda System Maintenance", hotfix ? "Hotfix failed!" : "Keyring update failed!", "garuda-system-maintenance");
    }
    busy = false;
    process->deleteLater();
    package_timer.start(KEYRING_CHECK_TIME);
}

void Tray::onCheckUpdatesComplete(int exitcode, QProcess::ExitStatus status, QProcess* process)
{
    busy = false;
    if (status == QProcess::ExitStatus::NormalExit && exitcode == 0) {
        auto output = process->readAllStandardOutput();
        if (output.contains("garuda-hotfixes") && settings.value("application/updatehotfixes", true).toBool())
            updateKeyring(true);
        else if (output.contains("chaotic-keyring"))
            updateKeyring();
        else
            package_timer.start(KEYRING_CHECK_TIME);
    }
    process->deleteLater();
}

// Hacky code to detect if $package has an update available
void Tray::checkPackage(QString package, QNetworkAccessManager* mgr, std::function<void(bool needsupdate)> next)
{
    auto network_reply = mgr->head(QNetworkRequest("https://builds.garudalinux.org/repos/chaotic-aur/logs/" + package + ".log"));
    connect(network_reply, &QNetworkReply::finished, this,
        [this, network_reply, package, next]() {
            if (network_reply->error() == network_reply->NoError && network_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                auto reply = network_reply->header(QNetworkRequest::LastModifiedHeader);
                auto local = settings.value("timestamps/" + package, "");
                if (reply != local) {
                    settings.setValue("timestamps/" + package, reply.toString());
                    next(true);
                } else
                    next(false);
            } else
                next(false);
            network_reply->deleteLater();
        });
}

void Tray::onShouldCheckPackages()
{
    auto network_manager = new QNetworkAccessManager(this);
    // Check if chaotic keyring has an update via the LastModified header
    checkPackage("chaotic-keyring", network_manager, [network_manager, this](bool needsupdate) {
        if (needsupdate) {
            checkUpdates();
            network_manager->deleteLater();
        } else
            checkPackage("garuda-hotfixes", network_manager, [network_manager, this](bool needsupdate) {
                if (needsupdate) {
                    checkUpdates();
                } else
                    package_timer.start(KEYRING_CHECK_TIME);
                network_manager->deleteLater();
            });
    });
}

void Tray::onCheckForum()
{
    auto network_manager = new QNetworkAccessManager(this);
    auto network_reply = network_manager->get(QNetworkRequest(QString("https://forum.garudalinux.org/c/announcements/announcements-maintenance/45.json")));
    connect(network_reply, &QNetworkReply::finished, this,
        [this, network_reply, network_manager]() {
            if (network_reply->error() == network_reply->NoError && network_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                auto json = QJsonDocument::fromJson(network_reply->readAll()).object();
                auto topics = json["topic_list"].toObject()["topics"].toArray();
                std::sort(topics.begin(), topics.end(), [](const QJsonValue& v1, const QJsonValue& v2) {
                    return v1.toObject()["created_at"].toString() > v2.toObject()["created_at"].toString();
                });
                auto newest = topics[0].toObject();
                auto utctimestamp = newest["created_at"].toString();
                if (utctimestamp <= settings.value("timestamps/forum").toString())
                    return;

                QString messagetext;
                auto url = "https://forum.garudalinux.org/t/" + QString::number(newest["id"].toInt());
                if (newest.contains("excerpt")) {
                    auto excerpt = newest["excerpt"].toString();
                    messagetext = url + "\n" + excerpt.left(150).replace("&hellip;", "...");
                    if (excerpt.length() > 150)
                        messagetext += "...";
                } else {
                    messagetext = "Visit " + url + ".";
                }

                settings.setValue("timestamps/forum", utctimestamp);

                KNotification* notification = new KNotification("forum", KNotification::Persistent);
                notification->setTitle("New maintenance announcement: " + newest["title"].toString());
                notification->setText(messagetext);
                notification->setActions({ "Open in browser" });
                connect(notification, QOverload<unsigned int>::of(&KNotification::activated), [url]() { QDesktopServices::openUrl(url); });
                notification->sendEvent();

                network_manager->deleteLater();
                network_reply->deleteLater();
            }
            forum_timer.start(KEYRING_CHECK_TIME);
        });
}

void Tray::onReloadSettings()
{
    settings.sync();
    if (settings.value("application/updatekeyrings", true).toBool()) {
        if (!package_timer.isActive())
            package_timer.start(KEYRING_FIRST_CHECK_TIME);
    } else
        package_timer.stop();
    if (settings.value("application/notifyforum", true).toBool()) {
        if (!forum_timer.isActive())
            // For now we can just use the same time here, doesn't matter.
            forum_timer.start(KEYRING_FIRST_CHECK_TIME);
    } else
        forum_timer.stop();
}

Tray::~Tray()
{
    trayicon->deleteLater();
    watcher->deleteLater();
}
