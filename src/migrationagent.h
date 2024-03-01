#ifndef MIGRATIONAGENT_H
#define MIGRATIONAGENT_H

#include "baseagent.h"
#include <QProcess>

class MigrationAgent : public BaseAgent {
    Q_OBJECT
    bool once = false;

    void onCheckComplete(QSettings* migration_data, QProcess* process, int exitcode, QProcess::ExitStatus status);
    void onActionClicked();
    void createPrompt(QSettings* migration_data);

public:
    void onRoutine(bool init) override;
    MigrationAgent(ManagerData& data);
    void trayIconClicked() override;
};

#endif // MIGRATIONAGENT_H
