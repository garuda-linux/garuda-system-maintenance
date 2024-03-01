#ifndef SNAPSHOTAGENT_H
#define SNAPSHOTAGENT_H

#include "baseagent.h"

class SnapshotAgent : public BaseAgent {
    Q_OBJECT
    bool once = false;
    QString text;

    void deleteOld();
    void disableWarnings();

public:
    void onRoutine(bool init) override;
    SnapshotAgent(ManagerData& data)
        : BaseAgent(data)
    {
    }
    void trayIconClicked() override;
};

#endif // SNAPSHOTAGENT_H
