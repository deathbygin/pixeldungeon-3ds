#pragma once

#include <citro2d.h>
#include <string>

namespace pd {
class Hero;
}

namespace pd::ui {

class StatusBar {
public:
    float x = 8.0f;
    float y = 24.0f;
    float width = 304.0f;
    float height = 48.0f;

    StatusBar() = default;

    void update(float dt);
    void draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) const;

private:
    float m_displayedHpPct = 1.0f;
};

} // namespace pd::ui

