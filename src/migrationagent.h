#ifndef MIGRATIONAGENT_H
#define MIGRATIONAGENT_H

#include "baseagent.h"
#include <QProcess>

class MigrationAgent : public BaseAgent {
    Q_OBJECT
    bool once = false;

    void onCheckComplete(QSettings* migration_data, QProcess* process, int exitcode, QProcess::ExitStatus status);

public:
    void onRoutine() override;
    MigrationAgent(ManagerData& data)
        : BaseAgent(data)
    {
    }
};

#endif // MIGRATIONAGENT_H
