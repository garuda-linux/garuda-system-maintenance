#include "tray.h"
#include <KNotifications/KNotification>
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QVersionNumber>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

constexpr int KEYRING_FIRST_CHECK_TIME = 60 * 1000;
constexpr int KEYRING_CHECK_TIME = 30 * 60 * 1000;
#define PARTIAL_UPGRADE_WATCH_FOLDER "/var/lib/garuda"
#define PARTIAL_UPGRADE_PATH PARTIAL_UPGRADE_WATCH_FOLDER "/partial_upgrade"

// https://bugreports.qt.io/browse/QTBUG-44944
// https://stackoverflow.com/questions/54461719/sort-a-qjsonarray-by-one-of-its-child-elements/54461720
inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

class Checkupdates_data {
public:
    QString old_version;
    QString new_version;
};

inline static QMap<QString, Checkupdates_data> parseCheckupdates(QString input)
{
    QMap<QString, Checkupdates_data> map;

    auto lines = input.splitRef("\n");
    for (const auto& line : lines) {
        if (line.isEmpty())
            continue;
        auto words = line.split(" ");
        map[words[0].toString()] = { words[1].toString(), words[3].toString() };
    }
    return map;
}

bool Tray::partialUpgrade()
{
    return settings.value("application/partialupgrade", true).toBool() && QFile::exists(PARTIAL_UPGRADE_PATH);
}

void Tray::showSettings()
{
    if (!dialog) {
        dialog = new SettingsDialog(this);
        connect(dialog, SIGNAL(destroyed()), this, SLOT(onReloadSettings()));
        dialog->show();
    } else
        dialog->deleteLater();
}

void Tray::updateApplicationState()
{
    if (partialUpgrade()) {
        trayicon->setIconByName("garuda-system-maintenance-alert");
        trayicon->setStatus(KStatusNotifierItem::NeedsAttention);
    } else {
        trayicon->setIconByName("garuda-system-maintenance");
        trayicon->setStatus(KStatusNotifierItem::Passive);
    }
}

Tray::Tray(QWidget* parent)
    : QMainWindow(parent)
{
    if (!settings.value("application/firstrun", false).toBool()) {
        settings.setValue("application/firstrun", true);
        settings.setValue("timestamps/forum", QDateTime::currentDateTimeUtc().toString(Qt::DateFormat::ISODateWithMs));
    }

    trayicon = new KStatusNotifierItem(this);
    updateApplicationState();

    QMenu* menu = trayicon->contextMenu();

    QAction* keyringAction = new QAction(
        QIcon::fromTheme("update"),
        "Update keyring",
        menu);
    QAction* hotfixAction = new QAction(
        QIcon::fromTheme("update"),
        "Force hotfix update",
        menu);
    QAction* settingsAction = new QAction(
        QIcon::fromTheme("garuda-system-maintenance"),
        "Settings",
        menu);

    menu->addAction(keyringAction);
    menu->addAction(hotfixAction);
    menu->addSeparator();
    menu->addAction(settingsAction);

    connect(keyringAction, &QAction::triggered, [this]() { updateKeyring(true); });
    connect(hotfixAction, &QAction::triggered, [this]() { updateKeyring(false, true); });
    connect(settingsAction, &QAction::triggered, [this]() { showSettings(); });

    trayicon->setAssociatedWidget(nullptr);
    connect(trayicon, &KStatusNotifierItem::activateRequested, [this]() {
        if (partialUpgrade()) {
            QMessageBox dlg(QMessageBox::Warning, tr("Partial upgrade detected"), tr("You performed a \"partial upgrade\". Please fully update your system to prevent system instability.\nPerforming partial ugprades is unsupported.\nPress help to learn more."), QMessageBox::Ok | QMessageBox::Help, this);
            auto reply = dlg.exec();
            if (reply == QMessageBox::Help) {
                QDesktopServices::openUrl(QString("https://wiki.garudalinux.org/en/partial-upgrade"));
            }
        } else
            showSettings();
    });

    connect(&package_timer, &QTimer::timeout, this, &Tray::onShouldCheckPackages);
    package_timer.setSingleShot(true);
    connect(&forum_timer, &QTimer::timeout, this, &Tray::onCheckForum);
    forum_timer.setSingleShot(true);

    watcher = new QFileSystemWatcher(this);
    watcher->addPath(settings.fileName());
    watcher->addPath(PARTIAL_UPGRADE_WATCH_FOLDER);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        if (!watcher->files().contains(path) && QFile::exists(path))
            watcher->addPath(path);
        if (path == settings.fileName())
            onReloadSettings();
    });
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        QTimer::singleShot(1000, this, [this, path]() {
            updateApplicationState();
            if (path == PARTIAL_UPGRADE_WATCH_FOLDER && partialUpgrade()) {
                KNotification* notification = new KNotification("forum", KNotification::Persistent);
                notification->setTitle("Partial upgrade detected");
                notification->setText("You performed a \"partial upgrade\". Please fully update your system to prevent system instability.\nPerforming partial ugprades is unsupported.");
                notification->setActions({ "Disable warnings", "Learn more" });
                connect(notification, QOverload<unsigned int>::of(&KNotification::activated), [this](unsigned int action) {
                    if (action == 1) {
                        settings.setValue("application/partialupgrade", false);
                    } else {
                        QDesktopServices::openUrl(QUrl("https://wiki.garudalinux.org/en/partial-upgrade"));
                    }
                });
                notification->sendEvent();
            }
        });
    });

    onReloadSettings();
}

void Tray::updateKeyring(bool keyring, bool hotfixes)
{
    if (busy)
        return;
    busy = true;
    package_timer.stop();

    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    if (hotfixes) {
        int reply = 0;
        do {
            QMessageBox dlg(QMessageBox::Warning, tr("Hotfix available!"), tr("Important hotfix update found. Do you want to apply the hotfix (recommended)?\nPress the help button to learn more about this hotfix."), QMessageBox::Yes | QMessageBox::No | QMessageBox::Help, this);
            dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
            reply = dlg.exec();
            if (reply == QMessageBox::Yes) {
                connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) { onKeyringsInstalled(exitcode, status, process, true); });
                process->start("systemctl", QStringList() << "start"
                                                          << "garuda-system-maintenance@keyring-hotfixes.service");
                return;
            } else if (reply == QMessageBox::Help) {
                QDesktopServices::openUrl(QString("https://forum.garudalinux.org/current-garuda-hotfix"));
            }
        } while (reply == QMessageBox::Help);
    }
    if (keyring) {
        trayicon->showMessage("Garuda System Maintenance", "Updating keyrings in the background...", "garuda-system-maintenance");
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) { onKeyringsInstalled(exitcode, status, process, false); });
        process->start("systemctl", QStringList() << "start"
                                                  << "garuda-system-maintenance@keyring.service");
    }
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
        auto parsed = parseCheckupdates(QString::fromUtf8(process->readAllStandardOutput()));
        bool keyring = parsed.contains("chaotic-keyring") || parsed.contains("archlinux-keyring");
        bool hotfixes = false;
        if (settings.value("application/updatehotfixes", true).toBool()) {
            auto packageinfo = parsed.find("garuda-hotfixes");
            if (packageinfo != parsed.end()) {
                if (QVersionNumber::commonPrefix(QVersionNumber::fromString(packageinfo->old_version), QVersionNumber::fromString(packageinfo->new_version)).segmentCount() < 2)
                    hotfixes = true;
            }
        }

        if (keyring || hotfixes)
            updateKeyring(keyring, hotfixes);
        else
            package_timer.start(KEYRING_CHECK_TIME);
    } else
        package_timer.start(KEYRING_CHECK_TIME);
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
                if (needsupdate)
                    checkUpdates();
                else
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
    updateApplicationState();
    // There's some weird issue here that should like never matter tho
    // If this is run during checkUpdates, the timer will be started with KEYRING_FIRST_CHECK_TIME instead of waiting for checkUpdates to finish.
    // That's why the busy check is necessary here
    if (settings.value("application/updatekeyrings", true).toBool()) {
        if (!package_timer.isActive() && !busy)
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
