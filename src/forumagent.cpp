#include "forumagent.h"

#include <KNotifications/KNotification>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// https://bugreports.qt.io/browse/QTBUG-44944
// https://stackoverflow.com/questions/54461719/sort-a-qjsonarray-by-one-of-its-child-elements/54461720
inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

void ForumAgent::onRoutine(bool init)
{
    if (!settings.value("application/notifyforum", true).toBool())
        return;
    auto network_manager = new QNetworkAccessManager(this);
    auto network_reply = network_manager->get(QNetworkRequest(QString("https://forum.garudalinux.org/c/announcements/announcements-maintenance/45.json")));
    QObject::connect(network_reply, &QNetworkReply::finished, this,
        [this, network_reply, network_manager]() {
            if (network_reply->error() == network_reply->NoError && network_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                auto json = QJsonDocument::fromJson(network_reply->readAll()).object();
                auto topics = json["topic_list"].toObject()["topics"].toArray();
                std::sort(topics.begin(), topics.end(), [](const QJsonValue& v1, const QJsonValue& v2) {
                    return v1.toObject()["created_at"].toString() > v2.toObject()["created_at"].toString();
                });
                auto newest = topics[0].toObject();

                auto utctimestamp = newest["created_at"].toString();
                auto last_utc_time = settings.value("timestamps/forum", "").toString();
                if (last_utc_time != "" && utctimestamp > last_utc_time) {
                    auto title = newest["title"].toString();
                    QString messagetext;
                    auto url = "https://forum.garudalinux.org/t/" + QString::number(newest["id"].toInt());
                    if (newest.contains("excerpt")) {
                        auto excerpt = newest["excerpt"].toString();
                        messagetext = url + "\n" + excerpt.left(150).replace("&hellip;", "...");
                        if (excerpt.length() > 150)
                            messagetext += "...";
                    } else {
                        messagetext = "Visit " + url + ".";
                    }
                    showNotification({ url, utctimestamp, title, messagetext });
                }
                if (utctimestamp != last_utc_time)
                    settings.setValue("timestamps/forum", utctimestamp);
            }
            network_manager->deleteLater();
            network_reply->deleteLater();
        });
}

void ForumAgent::showNotification(ForumData data)
{
    KNotification* notification = new KNotification("general", KNotification::Persistent);
    notification->setTitle(tr("New maintenance announcement: ") + data.title);
    notification->setText(data.content);
    notification->setActions({ tr("Open in browser") });
    QString& url = data.url;
    connect(notification, QOverload<unsigned int>::of(&KNotification::activated), [url]() { QDesktopServices::openUrl(url); });
    notification->sendEvent();
}
