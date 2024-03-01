#include "agentmanager.h"

#include "forumagent.h"
#include "migrationagent.h"
#include "packageagent.h"
#include "snapshotagent.h"
#include "updateagent.h"

AgentManager::AgentManager()
{
    timer = new QTimer();
}

void AgentManager::onRoutine(QSettings *settings)
{
    bool init = false;
    if (settings->value("application/version", 0) == 1) {
        settings->setValue("application/version", 2);
        init = true;
    }
    for (auto& agent : agents)
        agent->onRoutine(init);
}

BaseAgent* AgentManager::getHighestPriorityAgent()
{
    auto tempVector = agents;
    std::sort(tempVector.begin(), tempVector.end(), [](BaseAgent* a, BaseAgent* b) {
        return a->click_priority > b->click_priority;
    });
    return tempVector[0];
}

bool AgentManager::onTrayIconClick()
{
    auto highest = getHighestPriorityAgent();
    if (highest->click_priority > 0) {
        highest->trayIconClicked();
        return true;
    }
    return false;
}

void AgentManager::onSettingsReloaded()
{
    for (auto& agent : agents)
        agent->onSettingsReloaded();
}

void AgentManager::init(QSettings& settings, KStatusNotifierItem* trayicon, std::function<void(int)> priority_callback)
{
    ManagerData data { [this, priority_callback]() { priority_callback(getHighestPriorityAgent()->click_priority); }, settings, trayicon };
    agents += new ForumAgent(data);
    agents += new PackageAgent(data);
    agents += new UpdateAgent(data);
    agents += new SnapshotAgent(data);
    agents += new MigrationAgent(data);
    connect(timer, &QTimer::timeout, this, std::bind(&AgentManager::onRoutine, this, &settings));
    timer->start(15 * 60 * 1000);
    QTimer::singleShot(30000, this, std::bind(&AgentManager::onRoutine, this, &settings));
}

AgentManager::~AgentManager()
{
    delete timer;
    for (auto& agent : agents)
        delete agent;
}
