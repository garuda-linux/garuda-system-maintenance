#ifndef UPDATEAGENT_H
#define UPDATEAGENT_H

#include "baseagent.h"

class QFileSystemWatcher;

class UpdateAgent : public BaseAgent {
    Q_OBJECT

public:
    void onRoutine() override;
    UpdateAgent(ManagerData& data);
    ~UpdateAgent();
    void trayIconClicked() override;

private:
    QFileSystemWatcher* watcher;
    bool isPartiallyUpgraded();
    bool isSystemCriticallyOutOfDate();
    void launchSystemUpdate();
    void updateApplicationState();
    void onSettingsReloaded() override
    {
        updateApplicationState();
    }
};

#endif // UPDATEAGENT_H
