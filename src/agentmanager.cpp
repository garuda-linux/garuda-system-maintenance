#include "agentmanager.h"

#include "forumagent.h"
#include "snapshotagent.h"
#include "updateagent.h"

AgentManager::AgentManager()
{
    timer = new QTimer();
}

void AgentManager::onRoutine(QSettings *settings, bool init)
{
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
    int version = settings.value("application/version", 0).toInt();
    bool init = false;
    if (version <= 1) {
        settings.setValue("application/version", 3);
        init = true;
    } else if (version == 2) {
        settings.setValue("application/version", 3);
        settings.setValue("timestamps/forum", "2025-07-18T00:00:00.000Z");
    }

    ManagerData data { [this, priority_callback]() { priority_callback(getHighestPriorityAgent()->click_priority); }, settings, trayicon };
    agents += new ForumAgent(data);
    agents += new UpdateAgent(data);
    agents += new SnapshotAgent(data);
    connect(timer, &QTimer::timeout, this, std::bind(&AgentManager::onRoutine, this, &settings, false));
    timer->start(15 * 60 * 1000);

    QTimer::singleShot(30000, this, std::bind(&AgentManager::onRoutine, this, &settings, init));
}

AgentManager::~AgentManager()
{
    delete timer;
    for (auto& agent : agents)
        delete agent;
}
