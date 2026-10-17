#pragma once

#include <citro2d.h>
#include <memory>
#include <vector>
#include <string>
#include "pd/ui/Archs.h"
#include "pd/ui/TouchController.h"
#include "pd/Rankings.h"
#include "pd/windows/WndRanking.h"
#include "noosa/Texture.h"

namespace pd::scenes {

enum class RankingsAction {
    NONE,
    BACK_TO_TITLE
};

class RankingsScene {
public:
    RankingsScene();
    ~RankingsScene() = default;

    void update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch);
    void drawTop(C2D_TextBuf textBuf);
    void drawBottom(C2D_TextBuf textBuf);

    RankingsAction getPendingAction() const { return m_pendingAction; }
    void clearAction() { m_pendingAction = RankingsAction::NONE; }
    void refresh();

private:
    RankingsAction m_pendingAction = RankingsAction::NONE;
    int m_selectedRecord = 0;

    pd::ui::Archs m_archs;
    pd::windows::WndRanking m_wndRanking;

    std::shared_ptr<noosa::Texture> m_texItems;
    std::shared_ptr<noosa::Texture> m_texIcons;
    std::shared_ptr<noosa::Texture> m_texWarrior;
    std::shared_ptr<noosa::Texture> m_texMage;
    std::shared_ptr<noosa::Texture> m_texRogue;
    std::shared_ptr<noosa::Texture> m_texHuntress;

    // Scrolling list on Bottom Screen
    float m_scrollY = 0.0f;
    float m_maxScroll = 0.0f;
    bool m_isDragging = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    // Bottom screen chrome & layout
    float m_winX = 10.0f;
    float m_winY = 24.0f;
    float m_winW = 300.0f;
    float m_winH = 176.0f;
    float m_rowH = 28.0f;

    // Action buttons at bottom
    float m_btnInspectX = 14.0f;
    float m_btnInspectY = 206.0f;
    float m_btnInspectW = 120.0f;
    float m_btnInspectH = 24.0f;

    float m_btnBackX = 186.0f;
    float m_btnBackY = 206.0f;
    float m_btnBackW = 120.0f;
    float m_btnBackH = 24.0f;

    void initTextures();
    void drawHeroAvatarTop(float x, float y, Hero::HeroClass cl, int tier);
};

} // namespace pd::scenes

