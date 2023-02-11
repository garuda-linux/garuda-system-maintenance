#ifndef PACKAGEAGENT_H
#define PACKAGEAGENT_H

#include "baseagent.h"
#include <QProcess>

class QNetworkAccessManager;

class PackageAgent : public BaseAgent {
    Q_OBJECT
    void showNotification();
    KStatusNotifierItem* trayicon;

public:
    void onRoutine() override;
    PackageAgent(ManagerData& data)
        : BaseAgent(data)
        , trayicon(data.trayicon)
    {
    }

private:
    bool busy = false;

    void updateKeyring(bool keyring, bool hotfixes);
    void checkUpdates();
    void onKeyringsInstalled(int exitcode, QProcess::ExitStatus status, QProcess* process, bool hotfix);
    void onShouldCheckPackages();
    void onCheckUpdatesComplete(int exitcode, QProcess::ExitStatus status, QProcess* process);
};

#endif // PACKAGEAGENT_H
