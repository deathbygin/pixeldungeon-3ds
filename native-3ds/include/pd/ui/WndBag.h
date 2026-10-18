#pragma once

#include "pd/ui/TouchController.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/WndItem.h"
#include <citro2d.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace pd {
class Hero;
class Item;
}

namespace pd::ui {

class WndBag {
public:
    enum class Mode {
        ALL,
        UNIDENTIFIED,
        UPGRADEABLE,
        ENCHANTABLE,
        WEAPON,
        WAND,
        QUICKSLOT,
        FOR_SALE
    };

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

    bool isOpen = false;

    WndBag();
    ~WndBag() = default;

    void open();
    void openForSelection(Mode mode, const std::string& title, std::function<void(pd::Item*)> listener);
    void close();

    bool isSelectionMode() const { return m_selectionMode != Mode::ALL || m_selectionListener != nullptr; }
    Mode getSelectionMode() const { return m_selectionMode; }
    bool isItemEligible(const pd::Item* it) const;

    // Updates touch and hardware inputs.
    // Returns true if a turn-advancing action occurred.
    // outMessage receives narrative log feedback.
    bool update(pd::Hero* hero, const TouchController& touch, u32 kDown, std::string& outMessage) {
        return update(hero, touch, kDown, 0, outMessage);
    }
    bool update(pd::Hero* hero, const TouchController& touch, u32 kDown, u32 kHeld, std::string& outMessage);

    void draw(const pd::Hero* hero, C2D_TextBuf textBuf) const;

    int getSelectedIndex() const { return m_selectedSlot; }
    pd::Item* getSelectedItem(pd::Hero* hero) const;

private:
    int m_selectedSlot = 0;
    bool m_longPressTriggered = false;
    WndItem m_wndItem;

    Mode m_selectionMode = Mode::ALL;
    std::string m_selectionTitle;
    std::function<void(pd::Item*)> m_selectionListener = nullptr;

    std::shared_ptr<noosa::Texture> m_texItems;
    std::shared_ptr<noosa::Texture> m_texIcons;

    mutable Tex3DS_SubTexture m_subItemIcon{};
    mutable Tex3DS_SubTexture m_subTabIcon{};

    void openSubWindow(pd::Item* it, pd::Hero* hero);
    void closeSubWindow();
};

} // namespace pd::ui
