#pragma once

#include "Visual.h"
#include <vector>
#include <algorithm>
#include <memory>

namespace noosa {

class Group : public Visual {
public:
    Group();
    ~Group() override = default;

    void add(Visual* visual);
    void remove(Visual* visual);
    void clear();

    void update(float dt) override;
    void draw(const Camera* camera = nullptr) override;

    // Draw only members targeted at a specific screen
    void drawScreen(ScreenTarget target, const Camera* camera = nullptr);

    size_t count() const { return m_members.size(); }
    Visual* get(size_t index) const {
        return (index < m_members.size()) ? m_members[index] : nullptr;
    }

private:
    std::vector<Visual*> m_members;
};

} // namespace noosa

