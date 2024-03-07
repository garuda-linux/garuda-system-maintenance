#include "packageagent.h"

#include <KStatusNotifierItem>
#include <QAction>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QVersionNumber>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

void PackageAgent::onRoutine(bool init)
{
    if (!busy && settings.value("application/updatekeyrings", true).toBool())
        onShouldCheckPackages();
}

PackageAgent::PackageAgent(ManagerData& data)
    : BaseAgent(data)
    , trayicon(data.trayicon)
{
    auto menu = data.trayicon->contextMenu();
    auto actions = menu->actions();
    QAction* keyringAction = new QAction(
        QIcon::fromTheme("update"),
        "Update keyring",
        menu);
    QAction* hotfixAction = new QAction(
        QIcon::fromTheme("update"),
        "Force hotfix update",
        menu);
    connect(keyringAction, &QAction::triggered, this, [this]() {
        if (busy)
            return;
        busy = true;
        updateKeyring(true, false);
    });
    connect(hotfixAction, &QAction::triggered, this, [this]() {
        if (busy)
            return;
        busy = true;
        updateKeyring(false, true);
    });

    menu->insertActions(actions.back(), { keyringAction, hotfixAction });
}

struct Checkupdates_data {
    QString old_version;
    QString new_version;
};

inline static QMap<QString, Checkupdates_data> parseCheckupdates(QString input)
{
    QMap<QString, Checkupdates_data> map;

    auto lines = input.split("\n");
    for (const auto& line : lines) {
        if (line.isEmpty())
            continue;
        auto words = line.split(" ");
        map[words[0]] = { words[1], words[3] };
    }
    return map;
}

void PackageAgent::updateKeyring(bool keyring, bool hotfixes)
{
    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    if (hotfixes) {
        int reply = 0;
        do {
            QMessageBox dlg(QMessageBox::Warning, tr("Hotfix available!"), tr("Important hotfix update found. Do you want to apply the hotfix (recommended)?\nPress the help button to learn more about this hotfix."), QMessageBox::Yes | QMessageBox::No | QMessageBox::Help);
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
    } else if (keyring) {
        trayicon->showMessage(tr("Garuda System Maintenance"), tr("Updating keyrings in the background..."), "garuda-system-maintenance");
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) { onKeyringsInstalled(exitcode, status, process, false); });
        process->start("systemctl", QStringList() << "start"
                                                  << "garuda-system-maintenance@keyring.service");
        return;
    }
    busy = false;
}

void PackageAgent::checkUpdates()
{
    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) { onCheckUpdatesComplete(exitcode, status, process); });
    process->start("checkupdates", QStringList());
}

void PackageAgent::onKeyringsInstalled(int exitcode, QProcess::ExitStatus status, QProcess* process, bool hotfix)
{
    if (status == QProcess::ExitStatus::NormalExit && exitcode == 0) {
        trayicon->showMessage(tr("Garuda System Maintenance"), hotfix ? tr("Hotfix successfully applied!") : tr("Keyrings successfully updated!"), "garuda-system-maintenance");
    } else {
        trayicon->showMessage(tr("Garuda System Maintenance"), hotfix ? tr("Hotfix failed!") : tr("Keyring update failed!"), "garuda-system-maintenance");
    }
    busy = false;
    process->deleteLater();
}

void PackageAgent::onCheckUpdatesComplete(int exitcode, QProcess::ExitStatus status, QProcess* process)
{
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
            busy = false;
    }
    process->deleteLater();
}

void PackageAgent::onShouldCheckPackages()
{
    busy = true;
    auto network_manager = new QNetworkAccessManager(this);
    auto network_reply = network_manager->get(QNetworkRequest(QString("https://garudalinux.org/os/garuda-update/garuda-hotfixes-version")));
    connect(network_reply, &QNetworkReply::finished, this,
        [this, network_manager, network_reply]() {
            if (network_reply->error() == network_reply->NoError && network_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                QString reply = QString(network_reply->readAll());
                auto local = settings.value("timestamps/garuda-hotfixes");
                if (reply != local) {
                    settings.setValue("timestamps/garuda-hotfixes", reply);
                    checkUpdates();
                } else
                    busy = false;
            } else
                busy = false;
            network_reply->deleteLater();
            network_manager->deleteLater();
        });
}
