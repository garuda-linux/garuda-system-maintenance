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

void AgentManager::onRoutine()
{
    for (auto& agent : agents)
        agent->onRoutine();
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
    connect(timer, &QTimer::timeout, this, &AgentManager::onRoutine);
    timer->start(15 * 60 * 1000);
    QTimer::singleShot(30000, this, &AgentManager::onRoutine);
}

AgentManager::~AgentManager()
{
    delete timer;
    for (auto& agent : agents)
        delete agent;
}
