#pragma once

#include <citro2d.h>
#include <memory>
#include <vector>
#include <string>
#include "pd/ui/Archs.h"
#include "pd/ui/TouchController.h"
#include "pd/Badges.h"
#include "pd/windows/WndBadge.h"

namespace pd::scenes {

enum class BadgesAction {
    NONE,
    BACK_TO_TITLE
};

class BadgesScene {
public:
    BadgesScene();
    ~BadgesScene() = default;

    void update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch);
    void drawTop(C2D_TextBuf textBuf);
    void drawBottom(C2D_TextBuf textBuf);

    BadgesAction getPendingAction() const { return m_pendingAction; }
    void clearAction() { m_pendingAction = BadgesAction::NONE; }
    void refresh();

private:
    BadgesAction m_pendingAction = BadgesAction::NONE;
    int m_selectedIndex = 0;

    struct GridItem {
        Badges::Badge badge;
        bool unlocked = false;
        int image = 0;
        std::string title;
        std::string description;
    };
    std::vector<GridItem> m_items;

    pd::ui::Archs m_archs;
    pd::windows::WndBadge m_wndBadge;

    // Scrolling grid on Bottom Screen
    float m_scrollY = 0.0f;
    float m_maxScroll = 0.0f;
    bool m_isDragging = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    // Bottom screen chrome & grid layout
    static constexpr int COLS = 6;
    static constexpr int VISIBLE_ROWS = 5;
    float m_winX = 10.0f;
    float m_winY = 24.0f;
    float m_winW = 300.0f;
    float m_winH = 176.0f;
    float m_cellW = 43.0f;
    float m_cellH = 28.0f;
    float m_spacingX = 3.0f;
    float m_spacingY = 2.0f;
    float m_gridPaddingX = 13.0f;
    float m_gridPaddingY = 14.0f;

    // Action buttons at bottom
    float m_btnInspectX = 14.0f;
    float m_btnInspectY = 208.0f;
    float m_btnInspectW = 140.0f;
    float m_btnInspectH = 24.0f;

    float m_btnBackX = 166.0f;
    float m_btnBackY = 208.0f;
    float m_btnBackW = 140.0f;
    float m_btnBackH = 24.0f;

    void ensureVisible(int index);
    void wrapText(const std::string& text, float maxW, std::vector<std::string>& outLines);
};

} // namespace pd::scenes

