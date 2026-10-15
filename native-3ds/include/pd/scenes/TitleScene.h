#pragma once

#include <citro2d.h>
#include <memory>
#include <vector>
#include <string>
#include "pd/ui/Archs.h"
#include "pd/effects/Fireball.h"
#include "pd/ui/TouchController.h"
#include "pd/ui/UIButton.h"
#include "noosa/Texture.h"

namespace pd::scenes {

enum class TitleAction {
    NONE,
    PLAY,
    RANKINGS,
    BADGES,
    ABOUT,
    SETTINGS
};

class TitleScene {
public:
    TitleScene();
    ~TitleScene() = default;

    void update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch);
    void drawTop(C2D_TextBuf textBuf);
    void drawBottom(C2D_TextBuf textBuf);

    TitleAction getPendingAction() const { return m_pendingAction; }
    void clearAction() { m_pendingAction = TitleAction::NONE; }

private:
    TitleAction m_pendingAction = TitleAction::NONE;

    // Background & Colonnade
    pd::ui::Archs m_archs;

    // Top Screen Elements
    std::shared_ptr<noosa::Texture> m_texBanners;
    Tex3DS_SubTexture m_subBannerLogo;
    Tex3DS_SubTexture m_subBannerSigns;
    bool m_subBannersInit = false;

    pd::effects::Fireball m_torchLeft;
    pd::effects::Fireball m_torchRight;
    float m_bannerX = 0.0f;
    float m_bannerY = 0.0f;
    float m_bannerScale = 1.0f;
    float m_bannerW = 128.0f;
    float m_bannerH = 70.0f;
    float m_signsTimer = 0.0f;

    // Bottom Screen Elements & Dashboard
    std::shared_ptr<noosa::Texture> m_texDashboard;
    std::shared_ptr<noosa::Texture> m_texIcons;
    Tex3DS_SubTexture m_subDashPlay;
    Tex3DS_SubTexture m_subDashAbout;
    Tex3DS_SubTexture m_subDashRankings;
    Tex3DS_SubTexture m_subDashBadges;
    Tex3DS_SubTexture m_subIconPrefs;
    bool m_subUIInit = false;

    // 2x2 grid navigation
    int m_selectedGrid = 0; // 0: Play, 1: Rankings, 2: Badges, 3: About
    static constexpr int GRID_COLS = 2;
    static constexpr int GRID_ROWS = 2;

    struct DashboardButton {
        float x = 0.0f;
        float y = 0.0f;
        float w = 48.0f;
        float h = 48.0f;
        std::string label;
        int iconIndex = 0; // 0: Play, 1: About, 2: Rankings, 3: Badges
        bool pressed = false;
    };

    DashboardButton m_btnPlay;
    DashboardButton m_btnRankings;
    DashboardButton m_btnBadges;
    DashboardButton m_btnAbout;

    // Settings icon top-right
    float m_btnSettingsX = 0.0f;
    float m_btnSettingsY = 0.0f;
    float m_btnSettingsW = 24.0f;
    float m_btnSettingsH = 24.0f;
    bool m_btnSettingsPressed = false;

    void initTextures();
    void layoutButtons();
};

} // namespace pd::scenes

