#include "pd/scenes/TitleScene.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include <cmath>
#include <algorithm>

namespace pd::scenes {

TitleScene::TitleScene() {
    initTextures();
    layoutButtons();

    // Top Screen Title placement (400x240)
    // Scale banner to 1.5x (192x105) to give substantial presence matching mobile screenshot
    m_bannerScale = 1.4f;
    m_bannerW = 128.0f * m_bannerScale;
    m_bannerH = 70.0f * m_bannerScale;
    m_bannerX = (400.0f - m_bannerW) * 0.5f;
    m_bannerY = (240.0f - m_bannerH) * 0.5f - 4.0f;

    // Torch anchors scaled with the banner horns:
    // Left:  banner.x + 18 * scale, banner.y + 20 * scale
    // Right: banner.x + banner.w - 18 * scale, banner.y + 20 * scale
    m_torchLeft.setPos(m_bannerX + 18.0f * m_bannerScale, m_bannerY + 20.0f * m_bannerScale);
    m_torchRight.setPos(m_bannerX + m_bannerW - 18.0f * m_bannerScale, m_bannerY + 20.0f * m_bannerScale);
}

void TitleScene::initTextures() {
    m_texBanners = noosa::Texture::get(pd::Assets::BANNERS);
    m_texDashboard = noosa::Texture::get(pd::Assets::DASHBOARD);
    m_texIcons = noosa::Texture::get(pd::Assets::ICONS);

    if (m_texBanners && m_texBanners->isLoaded()) {
        float potW = static_cast<float>(m_texBanners->potWidth());
        float potH = static_cast<float>(m_texBanners->potHeight());

        // Banner: 0, 0, 128, 70
        m_subBannerLogo.width = 128;
        m_subBannerLogo.height = 70;
        m_subBannerLogo.left = 0.0f;
        m_subBannerLogo.right = 128.0f / potW;
        m_subBannerLogo.top = 1.0f;
        m_subBannerLogo.bottom = 1.0f - (70.0f / potH);

        // Signs: 0, 161, 128, 218 (h = 57)
        m_subBannerSigns.width = 128;
        m_subBannerSigns.height = 57;
        m_subBannerSigns.left = 0.0f;
        m_subBannerSigns.right = 128.0f / potW;
        m_subBannerSigns.top = 1.0f - (161.0f / potH);
        m_subBannerSigns.bottom = 1.0f - (218.0f / potH);

        m_subBannersInit = true;
    }

    if (m_texDashboard && m_texDashboard->isLoaded() && m_texIcons && m_texIcons->isLoaded()) {
        float potW = static_cast<float>(m_texDashboard->potWidth());
        float potH = static_cast<float>(m_texDashboard->potHeight());

        auto setupDashSub = [&](Tex3DS_SubTexture& sub, int idx) {
            sub.width = 32;
            sub.height = 32;
            sub.left = (idx * 32.0f) / potW;
            sub.right = ((idx + 1) * 32.0f) / potW;
            sub.top = 1.0f;
            sub.bottom = 1.0f - (32.0f / potH);
        };

        setupDashSub(m_subDashPlay, 0);     // 0: Play
        setupDashSub(m_subDashAbout, 1);    // 1: About
        setupDashSub(m_subDashRankings, 2); // 2: Rankings
        setupDashSub(m_subDashBadges, 3);   // 3: Badges

        // Settings gear from icons.png: (30, 0, 46, 16) -> 16x16
        float iconPotW = static_cast<float>(m_texIcons->potWidth());
        float iconPotH = static_cast<float>(m_texIcons->potHeight());
        m_subIconPrefs.width = 16;
        m_subIconPrefs.height = 16;
        m_subIconPrefs.left = 30.0f / iconPotW;
        m_subIconPrefs.right = 46.0f / iconPotW;
        m_subIconPrefs.top = 1.0f;
        m_subIconPrefs.bottom = 1.0f - (16.0f / iconPotH);

        m_subUIInit = true;
    }
}

void TitleScene::layoutButtons() {
    // 2x2 grid centered on bottom screen (320x240)
    // Original Java DashboardItem:
    // SIZE = 48. Image is 32x32 centered, Label is 2px below image.
    // Horizontal spacing: button width 48, gap 32 -> total width: 48 + 32 + 48 = 128
    // Center on 320 screen: (320 - 128) / 2 = 96
    // Col 0: 96, Col 1: 96 + 48 + 32 = 176
    // Vertical spacing: row height 54 (32 img + 2 gap + 12 txt + 8 space)
    // Row 0: Y = 46.0f, Row 1: Y = 122.0f

    float x0 = 96.0f;
    float x1 = 176.0f;
    float y0 = 46.0f;
    float y1 = 122.0f;

    m_btnPlay.x = x0;
    m_btnPlay.y = y0;
    m_btnPlay.w = 48.0f;
    m_btnPlay.h = 48.0f;
    m_btnPlay.label = "Play";
    m_btnPlay.iconIndex = 0;

    m_btnRankings.x = x1;
    m_btnRankings.y = y0;
    m_btnRankings.w = 48.0f;
    m_btnRankings.h = 48.0f;
    m_btnRankings.label = "Rankings";
    m_btnRankings.iconIndex = 2;

    m_btnBadges.x = x0;
    m_btnBadges.y = y1;
    m_btnBadges.w = 48.0f;
    m_btnBadges.h = 48.0f;
    m_btnBadges.label = "Badges";
    m_btnBadges.iconIndex = 3;

    m_btnAbout.x = x1;
    m_btnAbout.y = y1;
    m_btnAbout.w = 48.0f;
    m_btnAbout.h = 48.0f;
    m_btnAbout.label = "About";
    m_btnAbout.iconIndex = 1;

    // Settings Gear in top-right corner flush with top/right margin
    m_btnSettingsX = 320.0f - 24.0f;
    m_btnSettingsY = 4.0f;
    m_btnSettingsW = 20.0f;
    m_btnSettingsH = 20.0f;
}

void TitleScene::update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch) {
    (void)kHeld;
    if (!m_subBannersInit || !m_subUIInit) {
        initTextures();
    }

    // Parallax and torch updates
    m_archs.update(dt);
    m_torchLeft.update(dt);
    m_torchRight.update(dt);

    // Magic signs timer: am = sin(-time)
    m_signsTimer += dt;

    // Physical Hardware Shortcuts
    if (kDown & KEY_START) {
        Sample::play(Assets::SND_CLICK, 1.0f, 0.8f);
        m_pendingAction = TitleAction::PLAY;
        return;
    }
    if (kDown & KEY_SELECT) {
        Sample::play(Assets::SND_CLICK, 1.0f, 0.8f);
        m_pendingAction = TitleAction::SETTINGS;
        return;
    }

    // D-Pad / Circle Pad 2x2 Grid Navigation
    // Grid indexing:
    // [0: Play]     [1: Rankings]
    // [2: Badges]   [3: About]
    if (kDown & (KEY_DLEFT | KEY_CPAD_LEFT)) {
        if (m_selectedGrid % 2 == 1) m_selectedGrid -= 1;
    }
    if (kDown & (KEY_DRIGHT | KEY_CPAD_RIGHT)) {
        if (m_selectedGrid % 2 == 0) m_selectedGrid += 1;
    }
    if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
        if (m_selectedGrid >= 2) m_selectedGrid -= 2;
    }
    if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        if (m_selectedGrid < 2) m_selectedGrid += 2;
    }

    // A Button activates selected grid item
    if (kDown & KEY_A) {
        Sample::play(Assets::SND_CLICK, 1.0f, 0.8f);
        switch (m_selectedGrid) {
            case 0: m_pendingAction = TitleAction::PLAY; break;
            case 1: m_pendingAction = TitleAction::RANKINGS; break;
            case 2: m_pendingAction = TitleAction::BADGES; break;
            case 3: m_pendingAction = TitleAction::ABOUT; break;
        }
        return;
    }

    // Stylus Touch Handling
    auto checkBtn = [&](DashboardButton& btn, TitleAction act, int gridIdx) {
        noosa::RectF r(btn.x - 4.0f, btn.y - 4.0f, btn.w + 8.0f, btn.h + 20.0f);
        if (touch.hitTest(r)) {
            btn.pressed = true;
            m_selectedGrid = gridIdx;
        } else {
            btn.pressed = false;
        }

        if (touch.clicked(r)) {
            Sample::play(Assets::SND_CLICK, 1.0f, 0.8f);
            m_pendingAction = act;
        }
    };

    checkBtn(m_btnPlay, TitleAction::PLAY, 0);
    checkBtn(m_btnRankings, TitleAction::RANKINGS, 1);
    checkBtn(m_btnBadges, TitleAction::BADGES, 2);
    checkBtn(m_btnAbout, TitleAction::ABOUT, 3);

    // Settings Touch Check
    noosa::RectF rSettings(m_btnSettingsX - 4.0f, m_btnSettingsY - 4.0f, m_btnSettingsW + 8.0f, m_btnSettingsH + 8.0f);
    if (touch.hitTest(rSettings)) {
        m_btnSettingsPressed = true;
    } else {
        m_btnSettingsPressed = false;
    }
    if (touch.clicked(rSettings)) {
        Sample::play(Assets::SND_CLICK, 1.0f, 0.8f);
        m_pendingAction = TitleAction::SETTINGS;
    }
}

void TitleScene::drawTop(C2D_TextBuf textBuf) {
    (void)textBuf;

    // 1. Draw Colonnade Parallax
    m_archs.drawTop(0.05f);

    // 2. Draw Stone "Pixel Dungeon" Banner (Scaled to give prominent title presence)
    if (m_texBanners && m_texBanners->isLoaded() && m_texBanners->getTex() && m_subBannersInit) {
        C2D_Image img;
        img.tex = m_texBanners->getTex();
        img.subtex = &m_subBannerLogo;

        C2D_DrawParams params;
        params.pos.x = m_bannerX;
        params.pos.y = m_bannerY;
        params.pos.w = m_bannerW;
        params.pos.h = m_bannerH;
        params.center.x = 0.0f;
        params.center.y = 0.0f;
        params.depth = 0.20f;
        params.angle = 0.0f;
        C2D_DrawImage(img, &params, nullptr);

        // 3. Draw Pulsating Rune Signs atop Banner (Additive alpha modulation)
        float signAlpha = std::sin(-m_signsTimer);
        if (signAlpha > 0.0f) {
            img.subtex = &m_subBannerSigns;
            C2D_ImageTint tint;
            u8 a = static_cast<u8>(std::clamp(signAlpha, 0.0f, 1.0f) * 255.0f);
            C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xFF, a), 1.0f);

            C2D_DrawParams signsParams = params;
            signsParams.pos.y = m_bannerY + 1.0f * m_bannerScale;
            signsParams.pos.h = 57.0f * m_bannerScale;
            signsParams.depth = 0.22f;
            C2D_DrawImage(img, &signsParams, &tint);
        }
    }

    // 4. Draw Twin Torches
    m_torchLeft.draw(0.30f);
    m_torchRight.draw(0.30f);
}

void TitleScene::drawBottom(C2D_TextBuf textBuf) {
    (void)textBuf;
    // 1. Draw Colonnade Parallax continuation
    m_archs.drawBottom(0.05f);

    // 2. Draw Settings Gear Button (Top-Right, no box, clean icon directly on background)
    if (m_texIcons && m_texIcons->isLoaded() && m_texIcons->getTex() && m_subUIInit) {
        C2D_Image img;
        img.tex = m_texIcons->getTex();
        img.subtex = &m_subIconPrefs;

        C2D_DrawParams params;
        params.pos.x = m_btnSettingsX;
        params.pos.y = m_btnSettingsY;
        params.pos.w = 16.0f;
        params.pos.h = 16.0f;
        params.center.x = 0.0f;
        params.center.y = 0.0f;
        params.depth = 0.20f;
        params.angle = 0.0f;

        if (m_btnSettingsPressed) {
            // Brightness 1.5x on touch
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), 1.0f);
            C2D_DrawImage(img, &params, &tint);
        } else {
            C2D_DrawImage(img, &params, nullptr);
        }
    }

    // 3. Draw 2x2 Dashboard Buttons matching source Pixel Dungeon screenshot
    // Pure icon + text directly over the parallax arches (no artificial chrome box),
    // with subtle selection reticle when navigating via D-Pad / Circle Pad.
    auto drawDashBtn = [&](const DashboardButton& btn, Tex3DS_SubTexture& sub, bool isFocused) {
        // Icon inside button (32x32)
        if (m_texDashboard && m_texDashboard->isLoaded() && m_texDashboard->getTex()) {
            C2D_Image img;
            img.tex = m_texDashboard->getTex();
            img.subtex = &sub;

            float iconX = btn.x + (btn.w - 32.0f) * 0.5f;
            float iconY = btn.y;

            // Highlight glow/reticle when focused by hardware keys
            if (isFocused) {
                // Subtle four-corner bracket indicator
                u32 focusCol = C2D_Color32(0xe4, 0xa6, 0x72, 0xCC);
                float bX = iconX - 3.0f;
                float bY = iconY - 3.0f;
                float bW = 38.0f;
                float bH = 38.0f;
                float clen = 6.0f;
                // Top-left
                C2D_DrawRectSolid(bX, bY, 0.18f, clen, 1.5f, focusCol);
                C2D_DrawRectSolid(bX, bY, 0.18f, 1.5f, clen, focusCol);
                // Top-right
                C2D_DrawRectSolid(bX + bW - clen, bY, 0.18f, clen, 1.5f, focusCol);
                C2D_DrawRectSolid(bX + bW - 1.5f, bY, 0.18f, 1.5f, clen, focusCol);
                // Bottom-left
                C2D_DrawRectSolid(bX, bY + bH - 1.5f, 0.18f, clen, 1.5f, focusCol);
                C2D_DrawRectSolid(bX, bY + bH - clen, 0.18f, 1.5f, clen, focusCol);
                // Bottom-right
                C2D_DrawRectSolid(bX + bW - clen, bY + bH - 1.5f, 0.18f, clen, 1.5f, focusCol);
                C2D_DrawRectSolid(bX + bW - 1.5f, bY + bH - clen, 0.18f, 1.5f, clen, focusCol);
            }

            C2D_DrawParams params;
            params.pos.x = iconX;
            params.pos.y = iconY;
            params.pos.w = 32.0f;
            params.pos.h = 32.0f;
            params.center.x = 0.0f;
            params.center.y = 0.0f;
            params.depth = 0.20f;
            params.angle = 0.0f;

            if (btn.pressed) {
                // Brightness 1.5x on touch
                C2D_ImageTint tint;
                C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xCC, 0xFF), 1.0f);
                C2D_DrawImage(img, &params, &tint);
            } else {
                C2D_DrawImage(img, &params, nullptr);
            }
        }

        // Text label underneath using authentic Pixel Dungeon Font2x
        float textW = ui::BitmapFont::width(ui::BitmapFont::FontType::Font2x, btn.label);
        float labelX = btn.x + (btn.w - textW) * 0.5f;
        float labelY = btn.y + 34.0f;

        u32 textCol = isFocused ? ui::COLOR_TEXT_ACCENT : ui::COLOR_TEXT_PRIMARY;
        ui::BitmapFont::draw(ui::BitmapFont::FontType::Font2x, btn.label, labelX, labelY, 0.25f, textCol);
    };

    drawDashBtn(m_btnPlay, m_subDashPlay, m_selectedGrid == 0);
    drawDashBtn(m_btnRankings, m_subDashRankings, m_selectedGrid == 1);
    drawDashBtn(m_btnBadges, m_subDashBadges, m_selectedGrid == 2);
    drawDashBtn(m_btnAbout, m_subDashAbout, m_selectedGrid == 3);

    // 4. Version string: bottom-right corner ("v 1.0-alpha") using authentic Font1x in muted gray
    std::string verStr = "v 1.0-alpha";
    float vW = ui::BitmapFont::width(ui::BitmapFont::FontType::Font1x, verStr);
    ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, verStr, 320.0f - vW - 4.0f, 240.0f - 10.0f, 0.25f, ui::COLOR_TEXT_MUTED);
}

} // namespace pd::scenes

