#ifndef AGENTMANAGER_H
#define AGENTMANAGER_H

#include "baseagent.h"
#include <QObject>
#include <QTimer>

class KStatusNotifierItem;

class AgentManager : public QObject {
    Q_OBJECT
    QVector<BaseAgent*> agents;
    QTimer* timer;

public:
    AgentManager();
    void onRoutine();
    bool onTrayIconClick();
    void onSettingsReloaded();
    void init(QSettings& settings, KStatusNotifierItem* trayicon, std::function<void(int)> priority_callback);
    ~AgentManager();

private:
    BaseAgent* getHighestPriorityAgent();
};

#endif // AGENTMANAGER_H
