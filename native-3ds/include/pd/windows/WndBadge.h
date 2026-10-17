#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <memory>
#include "pd/Badges.h"
#include "pd/ui/TouchController.h"

namespace pd::windows {

class WndBadge {
public:
    WndBadge();
    ~WndBadge() = default;

    void show(Badges::Badge badge, bool unlocked = true);
    void hide() { m_open = false; }
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    Badges::Badge m_badge = Badges::Badge::COUNT;
    bool m_unlocked = false;

    float m_x = 40.0f;
    float m_y = 35.0f;
    float m_w = 240.0f;
    float m_h = 170.0f;

    std::vector<std::string> m_lines;

    void wrapText(const std::string& text, float maxW);
};

} // namespace pd::windows

