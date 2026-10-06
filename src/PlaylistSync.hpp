#pragma once
#include <QObject>
#include <QString>

namespace wekde {
// Shared across QQmlEngines: a JS library is only shared inside one engine.
class PlaylistSync : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool follower READ follower NOTIFY leadershipChanged)
public:
    explicit PlaylistSync(QObject* parent = nullptr) : QObject(parent) {}
    ~PlaylistSync() override;
    bool follower() const { return m_follower; }
    Q_INVOKABLE void join(const QString& id);
    Q_INVOKABLE void publish(const QString& workshopId, int index);
    Q_INVOKABLE void stepBy(int delta);
signals:
    void picked(const QString& workshopId, int index);
    void leadershipChanged();
    void advanceRequested(int delta);
private:
    QString m_group;
    bool m_follower = false;
};
}
