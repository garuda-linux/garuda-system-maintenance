#ifndef FORUMAGENT_H
#define FORUMAGENT_H

#include "baseagent.h"

struct ForumData {
    QString url;
    QString utctimestamp;
    QString title;
    QString content;
};

class ForumAgent : public BaseAgent {
    Q_OBJECT
    void showNotification(ForumData data);

public:
    void onRoutine() override;
    ForumAgent(ManagerData& data)
        : BaseAgent(data)
    {
    }
};

#endif // FORUMAGENT_H
