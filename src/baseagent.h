#ifndef BASEAGENT_H
#define BASEAGENT_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <optional>

class KStatusNotifierItem;

class ClickPriorityWrapper {
    int priority = 0;
    std::function<void()> callback;

protected:
    ClickPriorityWrapper(std::function<void()> callback)
        : callback(callback)
    {
    }

public:
    const int operator=(int other)
    {
        priority = other;
        callback();
        return priority;
    }
    operator const int()
    {
        return priority;
    }
    friend class BaseAgent;
};

struct ManagerData {
    std::function<void()> priority_callback;
    QSettings& settings;
    KStatusNotifierItem* trayicon;
};

class BaseAgent : public QObject {
    Q_OBJECT

protected:
    QSettings& settings;
    BaseAgent(ManagerData& data)
        : settings(data.settings)
        , click_priority(data.priority_callback)
    {
    }

public:
    virtual void trayIconClicked()
    {
        return;
    };
    virtual void onRoutine(bool init) = 0;
    virtual void onSettingsReloaded() {};
    ClickPriorityWrapper click_priority;
    virtual ~BaseAgent() {};
};

#endif // BASEAGENT_H
