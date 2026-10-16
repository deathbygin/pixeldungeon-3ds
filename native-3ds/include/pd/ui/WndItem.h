#pragma once

#include "pd/ui/TouchController.h"
#include <citro2d.h>
#include <memory>
#include <string>
#include <vector>

namespace pd {
class Item;
class Hero;
}

namespace noosa {
class Texture;
}

namespace pd::ui {

/**
 * Authentic Pixel Dungeon modal item inspection and action window (WndItem / WndInfoItem).
 * Provides dynamic sizing, strictly bounded multiline description layout, and
 * touch-drag / hardware scrolling with vertical glyph clipping.
 */
class WndItem {
public:
    WndItem();
    ~WndItem() = default;

    // Show modal info window for an item with optional action buttons
    void show(pd::Item* item, pd::Hero* hero, bool allowActions = true);
    void hide();

    bool isOpen() const { return m_open; }
    pd::Item* getItem() const { return m_item; }

    // Returns true if an action that advances the turn was executed.
    // outAction receives the action name executed (e.g. "EQUIP", "DROP", "CLOSE", etc.)
    bool update(pd::Hero* hero, const TouchController& touch, u32 kDown, u32 kHeld, std::string& outAction);

    void draw(C2D_TextBuf textBuf, float baseDepth = 0.82f) const;

private:
    bool m_open = false;
    pd::Item* m_item = nullptr;
    std::vector<std::string> m_actions;
    int m_selectedAction = 0;

    // Window layout geometry
    float m_winX = 30.0f;
    float m_winY = 20.0f;
    float m_winW = 260.0f;
    float m_winH = 200.0f;

    // Header layout
    float m_headerY = 0.0f;
    float m_headerH = 28.0f;
    std::string m_subHeader;

    // Body (Description) layout
    float m_bodyX = 0.0f;
    float m_bodyY = 0.0f;
    float m_bodyW = 0.0f;
    float m_bodyH = 0.0f;
    float m_usableTextW = 0.0f;
    float m_totalTextH = 0.0f;

    // Description text pre-wrapped lines
    std::vector<std::string> m_descLines;

    // Scrolling state
    bool m_isScrollable = false;
    float m_scrollY = 0.0f;
    float m_maxScroll = 0.0f;
    bool m_isDragging = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    // Footer (Buttons) layout
    float m_footerY = 0.0f;
    float m_footerH = 0.0f;
    float m_btnW = 0.0f;
    float m_btnH = 18.0f;
    int m_btnCols = 2;

    std::shared_ptr<noosa::Texture> m_texItems;
    mutable Tex3DS_SubTexture m_subItemIcon{};

    void calculateLayout(pd::Hero* hero);
};

using WndInfoItem = WndItem;

} // namespace pd::ui

