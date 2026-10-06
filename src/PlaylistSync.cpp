#include "PlaylistSync.hpp"
#include <QHash>
#include <QList>
#include <QPointer>
#include <QTimer>

namespace wekde {
namespace {
struct Group {
    QList<PlaylistSync*> clients;
    QString current;
    int index = 0;
    bool stepPending = false;
};
QHash<QString, Group>& groups() {
    static QHash<QString, Group> value;
    return value;
}
}
PlaylistSync::~PlaylistSync() { join({}); }

void PlaylistSync::join(const QString& id) {
    if (m_group == id) return;
    const QString old = m_group;
    m_group.clear();
    auto previous = groups().find(old);
    if (previous != groups().end()) {
        const bool leader = previous->clients.first() == this;
        previous->clients.removeAll(this);
        if (previous->clients.isEmpty()) groups().erase(previous);
        else if (leader) {
            previous->stepPending = false;
            auto* next = previous->clients.first();
            next->m_follower = false;
            emit next->leadershipChanged();
        }
    }
    m_group = id;
    m_follower = false;
    if (!id.isEmpty()) {
        auto& group = groups()[id];
        m_follower = !group.clients.isEmpty();
        group.clients.append(this);
        if (!group.current.isEmpty()) {
            // Join finishes before playback signals reach the QML controller.
            QTimer::singleShot(0, this, [this, id] {
                if (m_group != id) return;
                const auto it = groups().constFind(id);
                if (it != groups().cend() && !it->current.isEmpty())
                    emit picked(it->current, it->index);
            });
        }
    }
    emit leadershipChanged();
}

void PlaylistSync::publish(const QString& workshopId, int index) {
    auto it = groups().find(m_group);
    if (it == groups().end() || it->clients.first() != this) return;
    it->current = workshopId;
    it->index = index;
    // Signal handlers may change group membership; don't iterate a live list.
    QList<QPointer<PlaylistSync>> followers;
    for (auto* client : it->clients.mid(1)) followers.append(client);
    for (const auto& client : followers)
        if (client && client->m_group == m_group) emit client->picked(workshopId, index);
}

void PlaylistSync::stepBy(int delta) {
    auto it = groups().find(m_group);
    if (it == groups().end()) { emit advanceRequested(delta); return; }
    // D-Bus broadcasts to every display; consume that action once per group.
    if (it->stepPending) return;
    it->stepPending = true;
    const QString id = m_group;
    QPointer<PlaylistSync> target = it->clients.first();
    QTimer::singleShot(0, target, [target, id, delta] {
        auto group = groups().find(id);
        if (group == groups().end()) return;
        group->stepPending = false;
        if (target && group->clients.first() == target) emit target->advanceRequested(delta);
    });
}
}
