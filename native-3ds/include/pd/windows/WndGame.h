#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include "pd/ui/TouchController.h"

namespace pd::windows {

enum class GameAction {
    NONE,
    RESUME,
    SETTINGS,
    RANKINGS,
    START_NEW_GAME,
    MAIN_MENU,
    EXIT_GAME
};

class WndGame {
public:
    WndGame();
    ~WndGame() = default;

    void show();
    void hide() { m_open = false; }
    bool isOpen() const { return m_open; }

    GameAction update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    int m_selected = 0;

    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_w = 160.0f;
    float m_h = 146.0f;

    struct ButtonItem {
        std::string label;
        GameAction action;
        float x;
        float y;
        float w;
        float h;
    };
    std::vector<ButtonItem> m_buttons;
};

} // namespace pd::windows

