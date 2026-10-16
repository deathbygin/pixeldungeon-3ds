#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <functional>
#include "pd/ui/TouchController.h"

namespace pd::windows {

enum class DebugMenuAction {
    NONE,
    TOGGLE_GOD_MODE,
    TOGGLE_ONE_HIT_KILL,
    OPEN_ITEM_SPAWNER,
    HEAL_HERO,
    CLOSE
};

class WndDebugMenu {
public:
    WndDebugMenu();
    ~WndDebugMenu() = default;

    void show();
    void hide();
    bool isOpen() const { return m_open; }

    DebugMenuAction update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf, float baseDepth = 0.85f);

private:
    bool m_open = false;
    int m_selected = 0;

    float m_x = 55.0f;
    float m_y = 35.0f;
    float m_w = 210.0f;
    float m_h = 175.0f;

    struct ButtonItem {
        std::string label;
        DebugMenuAction action;
        float x;
        float y;
        float w;
        float h;
    };
    std::vector<ButtonItem> m_buttons;
};

} // namespace pd::windows

