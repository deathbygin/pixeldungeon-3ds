#include "noosa/Group.h"

namespace noosa {

Group::Group() = default;

void Group::add(Visual* visual) {
    if (visual && std::find(m_members.begin(), m_members.end(), visual) == m_members.end()) {
        m_members.push_back(visual);
    }
}

void Group::remove(Visual* visual) {
    auto it = std::find(m_members.begin(), m_members.end(), visual);
    if (it != m_members.end()) {
        m_members.erase(it);
    }
}

void Group::clear() {
    m_members.clear();
}

void Group::update(float dt) {
    if (!active) return;

    for (Visual* v : m_members) {
        if (v && v->active && v->alive) {
            v->update(dt);
        }
    }
}

void Group::draw(const Camera* camera) {
    if (!visible) return;

    for (Visual* v : m_members) {
        if (v && v->visible && v->alive) {
            v->draw(camera);
        }
    }
}

void Group::drawScreen(ScreenTarget target, const Camera* camera) {
    if (!visible) return;

    for (Visual* v : m_members) {
        if (v && v->visible && v->alive && v->targetScreen == target) {
            v->draw(camera);
        }
    }
}

} // namespace noosa

