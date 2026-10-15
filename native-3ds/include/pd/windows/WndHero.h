#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include "pd/ui/TouchController.h"

namespace pd {
class Hero;
}

namespace pd::windows {

class WndHero {
public:
    enum class Tab {
        STATS,
        BUFFS
    };

    WndHero() = default;
    ~WndHero() = default;

    void show();
    void hide() { m_open = false; }
    void toggle() { m_open ? hide() : show(); }
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown, pd::Hero* hero);
    void draw(const pd::Hero* hero, C2D_TextBuf textBuf);

private:
    bool m_open = false;
    Tab m_tab = Tab::STATS;

    float m_x = 16.0f;
    float m_y = 16.0f;
    float m_w = 288.0f;
    float m_h = 208.0f;
};

} // namespace pd::windows

