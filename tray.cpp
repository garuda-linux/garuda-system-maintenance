#include "tray.h"
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <functional>
#include <QMessageBox>

constexpr int KEYRING_FIRST_CHECK_TIME = 60 * 1000;
constexpr int KEYRING_CHECK_TIME = 60 * 1000;

Tray::Tray(QWidget* parent)
    : QMainWindow(parent)
{
    settings.setValue("app/firstrun", true);

    trayicon = new QSystemTrayIcon(QIcon::fromTheme("garuda-system-maintenance"), this);

    QMenu* menu = new QMenu(this);

    QAction* quitAction = new QAction(
        QIcon::fromTheme("application-exit"),
        "Quit",
        menu);
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
    menu->addSeparator();
    menu->addAction(quitAction);

    connect(quitAction, &QAction::triggered, [this]() { qApp->quit(); });
    connect(keyringAction, &QAction::triggered, [this]() { updateKeyring(); });
    connect(hotfixAction, &QAction::triggered, [this]() { updateKeyring(true); });

    trayicon->setContextMenu(menu);
    trayicon->show();

    connect(&timer, &QTimer::timeout, this, &Tray::onShouldCheckPackages);
    timer.setSingleShot(true);
    timer.start(KEYRING_FIRST_CHECK_TIME);
}

void Tray::updateKeyring(bool hotfixes)
{
    if (busy)
        return;
    busy = true;
    auto process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process, hotfixes](int exitcode, QProcess::ExitStatus status) { onKeyringsInstalled(exitcode, status, process, hotfixes); });

    if (hotfixes)
    {
        QMessageBox dlg(QMessageBox::Warning, tr("Hotfix available!"), tr("Important hotfix update found. Do you want to apply the hotfix (recommended)?"), QMessageBox::Yes | QMessageBox::No, this);
        dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
        auto reply = dlg.exec();
        if (reply == QMessageBox::Yes) {
            process->start("systemctl", QStringList() << "start"
                                                      << "garuda-system-maintenance@keyring-hotfixes.service");
            return;
        }
    }
    trayicon->showMessage("Garuda System Maintenance", "Updating keyrings in the background...");
    process->start("systemctl", QStringList() << "start"
                                              << "garuda-system-maintenance@keyring.service");
}

void Tray::checkUpdates()
{
    if (busy)
        return;
    busy = true;
    auto process = new QProcess(this);
    process->processEnvironment().insert("CHECKUPDATES_DB", "/tmp/garuda-system-maintenance/db");
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitcode, QProcess::ExitStatus status) { onCheckUpdatesComplete(exitcode, status, process); });
    process->start("checkupdates", QStringList());
}

void Tray::onKeyringsInstalled(int exitcode, QProcess::ExitStatus status, QProcess* process, bool hotfix)
{
    if (status == QProcess::ExitStatus::NormalExit && exitcode == 0) {
        trayicon->showMessage("Garuda System Maintenance", hotfix ? "Hotfix successfully applied!" : "Keyrings successfully updated!");
    } else {
        trayicon->showMessage("Garuda System Maintenance", hotfix ? "Hotfix failed!" : "Keyring update failed!");
    }
    busy = false;
    process->deleteLater();
    timer.start(KEYRING_CHECK_TIME);
}

void Tray::onCheckUpdatesComplete(int exitcode, QProcess::ExitStatus status, QProcess* process)
{
    busy = false;
    if (status == QProcess::ExitStatus::NormalExit && exitcode == 0) {
        auto output = process->readAllStandardOutput();
        if (output.contains("garuda-hotfixes"))
            updateKeyring(true);
        else if (output.contains("chaotic-keyring"))
            updateKeyring();
        else
            timer.start(KEYRING_CHECK_TIME);
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
                auto local = settings.value("packages/" + package, "");
                if (reply != local) {
                    settings.setValue("packages/" + package, reply.toString());
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
                    timer.start(KEYRING_CHECK_TIME);
                network_manager->deleteLater();
            });
    });
}

Tray::~Tray()
{
}
