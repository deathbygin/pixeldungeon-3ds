#pragma once

#include "pd/ui/TouchController.h"
#include "pd/ui/Chrome.h"
#include <citro2d.h>
#include <memory>
#include <string>
#include <vector>

namespace pd {
class Hero;
class Item;
}

namespace pd::ui {

class WndBag {
public:
    static constexpr int COLS = 4;
    static constexpr int ROWS = 5;
    static constexpr float SLOT_SIZE = 28.0f;
    static constexpr float SLOT_MARGIN = 2.0f;

    static constexpr float WIN_X = 40.0f;
    static constexpr float WIN_Y = 36.0f;
    static constexpr float WIN_W = 240.0f;
    static constexpr float WIN_H = 174.0f;

    static constexpr float GRID_START_X = 60.0f;
    static constexpr float GRID_START_Y = 56.0f;

    static constexpr float SUB_WIN_X = 35.0f;
    static constexpr float SUB_WIN_Y = 38.0f;
    static constexpr float SUB_WIN_W = 250.0f;
    static constexpr float SUB_WIN_H = 160.0f;
    static constexpr float SUB_BTN_Y = 108.0f;
    static constexpr float SUB_BTN_H = 20.0f;
    static constexpr float SUB_BTN_W = (SUB_WIN_W - 24.0f) / 2.0f;

    bool isOpen = false;

    WndBag();
    ~WndBag() = default;

    void open();
    void close();

    // Updates touch and hardware inputs.
    // Returns true if a turn-advancing action occurred.
    // outMessage receives narrative log feedback.
    bool update(pd::Hero* hero, const TouchController& touch, u32 kDown, std::string& outMessage);

    void draw(const pd::Hero* hero, C2D_TextBuf textBuf) const;

    int getSelectedIndex() const { return m_selectedSlot; }
    pd::Item* getSelectedItem(pd::Hero* hero) const;

private:
    int m_selectedSlot = 0;
    bool m_subWindowOpen = false;
    pd::Item* m_subItem = nullptr;
    std::vector<std::string> m_subActions;
    int m_selectedAction = 0;

    std::shared_ptr<noosa::Texture> m_texItems;
    std::shared_ptr<noosa::Texture> m_texIcons;

    mutable Tex3DS_SubTexture m_subItemIcon{};
    mutable Tex3DS_SubTexture m_subTabIcon{};

    void openSubWindow(pd::Item* it, pd::Hero* hero);
    void closeSubWindow();
};

} // namespace pd::ui
