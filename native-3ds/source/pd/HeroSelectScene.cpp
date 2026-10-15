#include "pd/scenes/HeroSelectScene.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Badges.h"
#include <cmath>
#include <algorithm>

namespace pd::scenes {

HeroSelectScene::HeroSelectScene() {
    m_huntressUnlocked = Badges::isUnlocked(Badges::Badge::BOSS_SLAIN_3);
    initTextures();
    layoutUI();
}

void HeroSelectScene::refreshUnlocked() {
    m_huntressUnlocked = Badges::isUnlocked(Badges::Badge::BOSS_SLAIN_3);
    if (!m_huntressUnlocked && m_currentClass == Hero::HeroClass::Huntress) {
        m_currentClass = Hero::HeroClass::Warrior;
    }
}

void HeroSelectScene::initTextures() {
    m_texBanners = noosa::Texture::get(pd::Assets::BANNERS);
    m_texAvatars = noosa::Texture::get(pd::Assets::AVATARS);
    m_texWarrior = noosa::Texture::get(pd::Assets::WARRIOR);
    m_texMage    = noosa::Texture::get(pd::Assets::MAGE);
    m_texRogue   = noosa::Texture::get(pd::Assets::ROGUE);
    m_texHuntress= noosa::Texture::get(pd::Assets::HUNTRESS);

    if (m_texBanners && m_texBanners->isLoaded()) {
        float potW = static_cast<float>(m_texBanners->potWidth());
        float potH = static_cast<float>(m_texBanners->potHeight());

        // SELECT_YOUR_HERO: 0, 140, 128, 161 (h = 21)
        m_subSelectHeroBanner.width = 128;
        m_subSelectHeroBanner.height = 21;
        m_subSelectHeroBanner.left = 0.0f;
        m_subSelectHeroBanner.right = 128.0f / potW;
        m_subSelectHeroBanner.top = 1.0f - (140.0f / potH);
        m_subSelectHeroBanner.bottom = 1.0f - (161.0f / potH);
    }

    if (m_texAvatars && m_texAvatars->isLoaded()) {
        float potW = static_cast<float>(m_texAvatars->potWidth());
        float potH = static_cast<float>(m_texAvatars->potHeight());

        // 24x28 per hero portrait
        for (int i = 0; i < 4; ++i) {
            m_subAvatars[i].width = 24;
            m_subAvatars[i].height = 28;
            m_subAvatars[i].left = (i * 24.0f) / potW;
            m_subAvatars[i].right = ((i + 1) * 24.0f) / potW;
            m_subAvatars[i].top = 1.0f;
            m_subAvatars[i].bottom = 1.0f - (28.0f / potH);
        }
    }

    m_texturesInit = true;
}

void HeroSelectScene::layoutUI() {
    // Bottom Screen (320x240):
    // 4 Portraits arranged horizontally:
    // Tab width 64, height 64. Total width = 4 * 64 + 3 * 8 = 256 + 24 = 280
    // Start X = (320 - 280) / 2 = 20.0f
    // Y = 80.0f
    m_tabs.clear();
    float tabW = 64.0f;
    float tabH = 68.0f;
    float tabGap = 8.0f;
    float startX = 20.0f;
    float startY = 70.0f;

    Hero::HeroClass classes[] = {
        Hero::HeroClass::Warrior,
        Hero::HeroClass::Mage,
        Hero::HeroClass::Rogue,
        Hero::HeroClass::Huntress
    };
    const char* names[] = { "Warrior", "Mage", "Rogue", "Huntress" };

    for (int i = 0; i < 4; ++i) {
        bool locked = (classes[i] == Hero::HeroClass::Huntress && !m_huntressUnlocked);
        m_tabs.push_back({ startX + i * (tabW + tabGap), startY, tabW, tabH, classes[i], names[i], locked });
    }

    // Action buttons at bottom
    m_btnStartX = 180.0f;
    m_btnStartY = 196.0f;
    m_btnStartW = 120.0f;
    m_btnStartH = 26.0f;

    m_btnBackX = 20.0f;
    m_btnBackY = 196.0f;
    m_btnBackW = 90.0f;
    m_btnBackH = 26.0f;
}

void HeroSelectScene::update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch) {
    (void)kHeld;
    m_archs.update(dt);

    // Sprite idle animation frame cycle
    m_animTimer += dt;
    if (m_animTimer >= 0.25f) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % 4;
    }

    // Hardware cycling (Left / Right)
    int curIdx = static_cast<int>(m_currentClass);
    if (kDown & (KEY_DLEFT | KEY_CPAD_LEFT)) {
        curIdx = (curIdx + 3) % 4;
        m_currentClass = static_cast<Hero::HeroClass>(curIdx);
    }
    if (kDown & (KEY_DRIGHT | KEY_CPAD_RIGHT)) {
        curIdx = (curIdx + 1) % 4;
        m_currentClass = static_cast<Hero::HeroClass>(curIdx);
    }

    // Hardware buttons
    if (kDown & KEY_A) {
        if (!(m_currentClass == Hero::HeroClass::Huntress && !m_huntressUnlocked)) {
            m_pendingAction = SelectAction::START_GAME;
        }
        return;
    }
    if (kDown & (KEY_B | KEY_START)) {
        m_pendingAction = SelectAction::BACK_TO_TITLE;
        return;
    }

    // Stylus Touch on Tabs
    for (const auto& tab : m_tabs) {
        noosa::RectF r(tab.x, tab.y, tab.w, tab.h);
        if (touch.clicked(r)) {
            m_currentClass = tab.cl;
        }
    }

    // Touch on Start / Back
    noosa::RectF rStart(m_btnStartX, m_btnStartY, m_btnStartW, m_btnStartH);
    if (touch.clicked(rStart)) {
        if (!(m_currentClass == Hero::HeroClass::Huntress && !m_huntressUnlocked)) {
            m_pendingAction = SelectAction::START_GAME;
        }
    }

    noosa::RectF rBack(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH);
    if (touch.clicked(rBack)) {
        m_pendingAction = SelectAction::BACK_TO_TITLE;
    }
}

void HeroSelectScene::drawTop(C2D_TextBuf textBuf) {
    // 1. Draw Colonnade Parallax
    m_archs.drawTop(0.05f);

    // 2. Translucent Stats & Perks Panel (Top screen center-right)
    float panelX = 140.0f;
    float panelY = 16.0f;
    float panelW = 244.0f;
    float panelH = 208.0f;
    C2D_DrawRectSolid(panelX, panelY, 0.15f, panelW, panelH, C2D_Color32(0x18, 0x14, 0x22, 0xD8));
    C2D_DrawRectSolid(panelX, panelY, 0.16f, panelW, 1.5f, C2D_Color32(0x6e, 0x54, 0x48, 0xFF));
    C2D_DrawRectSolid(panelX, panelY + panelH - 1.5f, 0.16f, panelW, 1.5f, C2D_Color32(0x6e, 0x54, 0x48, 0xFF));
    C2D_DrawRectSolid(panelX, panelY, 0.16f, 1.5f, panelH, C2D_Color32(0x6e, 0x54, 0x48, 0xFF));
    C2D_DrawRectSolid(panelX + panelW - 1.5f, panelY, 0.16f, 1.5f, panelH, C2D_Color32(0x6e, 0x54, 0x48, 0xFF));

    // 3. Class Animated Idle Sprite (Left side: 16..120px)
    std::shared_ptr<noosa::Texture> sprTex = nullptr;
    switch (m_currentClass) {
        case Hero::HeroClass::Warrior:  sprTex = m_texWarrior; break;
        case Hero::HeroClass::Mage:     sprTex = m_texMage; break;
        case Hero::HeroClass::Rogue:    sprTex = m_texRogue; break;
        case Hero::HeroClass::Huntress: sprTex = m_texHuntress; break;
    }

    bool isLocked = (m_currentClass == Hero::HeroClass::Huntress && !m_huntressUnlocked);

    if (sprTex && sprTex->isLoaded() && sprTex->getTex()) {
        float potW = static_cast<float>(sprTex->potWidth());
        float potH = static_cast<float>(sprTex->potHeight());

        // 12x15 frame size, animated frames 0..3
        int frameX = (m_animFrame % 4) * 12;
        Tex3DS_SubTexture subSpr;
        subSpr.width = 12;
        subSpr.height = 15;
        subSpr.left = static_cast<float>(frameX) / potW;
        subSpr.right = static_cast<float>(frameX + 12) / potW;
        subSpr.top = 1.0f;
        subSpr.bottom = 1.0f - (15.0f / potH);

        C2D_Image img;
        img.tex = sprTex->getTex();
        img.subtex = &subSpr;

        // Draw scaled 4x (48x60)
        float sprX = 40.0f;
        float sprY = 80.0f;

        C2D_DrawParams params;
        params.pos.x = sprX;
        params.pos.y = sprY;
        params.pos.w = 48.0f;
        params.pos.h = 60.0f;
        params.center.x = 0.0f;
        params.center.y = 0.0f;
        params.depth = 0.25f;
        params.angle = 0.0f;

        if (isLocked) {
            // Silhouette black
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0x10, 0x10, 0x10, 0xEE), 1.0f);
            C2D_DrawImage(img, &params, &tint);
        } else {
            C2D_DrawImage(img, &params, nullptr);
        }
    }

    // 4. Hero Class Name & Loadout Text
    std::string titleStr;
    std::string hpStr;
    std::vector<std::string> perksList;

    switch (m_currentClass) {
        case Hero::HeroClass::Warrior:
            titleStr = "WARRIOR";
            hpStr = "HP: 20/20   STR: 11";
            perksList = {
                "- Short sword (can be reforged)",
                "- Starts with 8 thrown Darts",
                "- Strength potions identified",
                "- Eating food restores health"
            };
            break;
        case Hero::HeroClass::Mage:
            titleStr = "MAGE";
            hpStr = "HP: 20/20   STR: 10";
            perksList = {
                "- Unique Wand of Magic Missile",
                "- Knuckles melee weapon",
                "- Faster wand recharge rate",
                "- Identify scrolls identified"
            };
            break;
        case Hero::HeroClass::Rogue:
            titleStr = "ROGUE";
            hpStr = "HP: 20/20   STR: 10";
            perksList = {
                "- Starts with Ring of Shadows+1",
                "- Dagger & 8 thrown Darts",
                "- Heightened trap & door detection",
                "- Magic Mapping scrolls identified"
            };
            break;
        case Hero::HeroClass::Huntress:
            titleStr = "HUNTRESS";
            hpStr = isLocked ? "LOCKED CHARACTER" : "HP: 15/15   STR: 10";
            if (isLocked) {
                perksList = {
                    "LOCKED CLASS!",
                    "",
                    "To unlock this character class,",
                    "slay the 3rd boss (DM-300)",
                    "with any other character."
                };
            } else {
                perksList = {
                    "- Unique upgradeable Boomerang",
                    "- Dagger melee backup",
                    "- Thrown weapon strength bonus",
                    "- Senses nearby monsters behind walls"
                };
            }
            break;
    }

    // Title using authentic Font2x
    u32 titleCol = isLocked ? 0xFF5555FF : 0xE4A672FF;
    ui::BitmapFont::draw(ui::BitmapFont::FontType::Font2x, titleStr, panelX + 16.0f, panelY + 12.0f, 0.30f, titleCol);

    // HP / STR Header using authentic Font1x
    u32 hpCol = isLocked ? 0xFFDD44FF : 0x56BB77FF;
    ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, hpStr, panelX + 16.0f, panelY + 34.0f, 0.30f, hpCol);

    // Perks Bullets using authentic Font1x
    float lineY = panelY + 54.0f;
    for (const auto& perk : perksList) {
        u32 col = isLocked ? 0xFFFF44FF : 0xDDDDDDFF;
        ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, perk, panelX + 16.0f, lineY, 0.30f, col);
        lineY += 14.0f;
    }
}

void HeroSelectScene::drawBottom(C2D_TextBuf textBuf) {
    // 1. Draw Colonnade Parallax continuation
    m_archs.drawBottom(0.05f);

    // 2. Banner: "SELECT YOUR HERO"
    if (m_texBanners && m_texBanners->isLoaded() && m_texBanners->getTex()) {
        C2D_Image img;
        img.tex = m_texBanners->getTex();
        img.subtex = &m_subSelectHeroBanner;

        // Centered horizontally: (320 - 128 * 1.25) / 2 = 80.0f
        float bW = 128.0f * 1.25f;
        float bH = 21.0f * 1.25f;
        C2D_DrawParams params;
        params.pos.x = (320.0f - bW) * 0.5f;
        params.pos.y = 16.0f;
        params.pos.w = bW;
        params.pos.h = bH;
        params.center.x = 0.0f;
        params.center.y = 0.0f;
        params.depth = 0.20f;
        params.angle = 0.0f;
        C2D_DrawImage(img, &params, nullptr);
    }

    // 3. 4 Hero Portrait Tabs
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        bool isSelected = (m_currentClass == tab.cl);

        u32 tabBg = isSelected ? C2D_Color32(0x56, 0x30, 0x64, 0xEE) : C2D_Color32(0x22, 0x1d, 0x2a, 0xCC);
        u32 tabBorder = isSelected ? C2D_Color32(0xe4, 0xa6, 0x72, 0xFF) : C2D_Color32(0x50, 0x42, 0x58, 0xAA);

        C2D_DrawRectSolid(tab.x, tab.y, 0.15f, tab.w, tab.h, tabBg);
        C2D_DrawRectSolid(tab.x, tab.y, 0.16f, tab.w, 1.5f, tabBorder);
        C2D_DrawRectSolid(tab.x, tab.y + tab.h - 1.5f, 0.16f, tab.w, 1.5f, tabBorder);
        C2D_DrawRectSolid(tab.x, tab.y, 0.16f, 1.5f, tab.h, tabBorder);
        C2D_DrawRectSolid(tab.x + tab.w - 1.5f, tab.y, 0.16f, 1.5f, tab.h, tabBorder);

        // Portrait Icon (24x28 centered in 64x64)
        if (m_texAvatars && m_texAvatars->isLoaded() && m_texAvatars->getTex()) {
            C2D_Image img;
            img.tex = m_texAvatars->getTex();
            img.subtex = &m_subAvatars[i];

            C2D_DrawParams params;
            params.pos.x = tab.x + (tab.w - 24.0f) * 0.5f;
            params.pos.y = tab.y + 6.0f;
            params.pos.w = 24.0f;
            params.pos.h = 28.0f;
            params.center.x = 0.0f;
            params.center.y = 0.0f;
            params.depth = 0.20f;
            params.angle = 0.0f;

            if (tab.locked) {
                C2D_ImageTint tint;
                C2D_PlainImageTint(&tint, C2D_Color32(0x20, 0x20, 0x20, 0xEE), 1.0f);
                C2D_DrawImage(img, &params, &tint);
            } else {
                C2D_DrawImage(img, &params, nullptr);
            }
        }

        // Name underneath using authentic Font1x
        float nW = ui::BitmapFont::width(ui::BitmapFont::FontType::Font1x, tab.name);
        float nameX = tab.x + (tab.w - nW) * 0.5f;
        float nameY = tab.y + 38.0f;

        u32 nameCol = tab.locked ? 0x888888FF : (isSelected ? 0xFFFF88FF : 0xCACFC2FF);
        ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, tab.name, nameX, nameY, 0.25f, nameCol);

        if (tab.locked) {
            std::string lockStr = "[LOCKED]";
            float lW = ui::BitmapFont::width(ui::BitmapFont::FontType::Font1x, lockStr);
            ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, lockStr, tab.x + (tab.w - lW) * 0.5f, tab.y + 48.0f, 0.26f, 0xFF4444FF);
        }
    }

    // 4. Action Buttons (Back [B] and Start Game [A])
    // Back Button
    C2D_DrawRectSolid(m_btnBackX, m_btnBackY, 0.20f, m_btnBackW, m_btnBackH, C2D_Color32(0x44, 0x22, 0x22, 0xEE));
    C2D_DrawRectSolid(m_btnBackX, m_btnBackY, 0.21f, m_btnBackW, 1.0f, C2D_Color32(0x88, 0x44, 0x44, 0xFF));
    C2D_DrawRectSolid(m_btnBackX, m_btnBackY + m_btnBackH - 1.0f, 0.21f, m_btnBackW, 1.0f, C2D_Color32(0x88, 0x44, 0x44, 0xFF));
    C2D_DrawRectSolid(m_btnBackX, m_btnBackY, 0.21f, 1.0f, m_btnBackH, C2D_Color32(0x88, 0x44, 0x44, 0xFF));
    C2D_DrawRectSolid(m_btnBackX + m_btnBackW - 1.0f, m_btnBackY, 0.21f, 1.0f, m_btnBackH, C2D_Color32(0x88, 0x44, 0x44, 0xFF));

    std::string backStr = "< Back [B]";
    float bW = ui::BitmapFont::width(ui::BitmapFont::FontType::Font1x, backStr);
    ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, backStr, m_btnBackX + (m_btnBackW - bW) * 0.5f, m_btnBackY + 7.0f, 0.25f, 0xEEDDDDFF);

    // Start Button
    bool isCurLocked = (m_currentClass == Hero::HeroClass::Huntress && !m_huntressUnlocked);
    u32 startBg = isCurLocked ? C2D_Color32(0x33, 0x33, 0x33, 0xCC) : C2D_Color32(0x2d, 0x66, 0x3d, 0xEE);
    u32 startBorder = isCurLocked ? C2D_Color32(0x55, 0x55, 0x55, 0xFF) : C2D_Color32(0x56, 0xbb, 0x77, 0xFF);

    C2D_DrawRectSolid(m_btnStartX, m_btnStartY, 0.20f, m_btnStartW, m_btnStartH, startBg);
    C2D_DrawRectSolid(m_btnStartX, m_btnStartY, 0.21f, m_btnStartW, 1.0f, startBorder);
    C2D_DrawRectSolid(m_btnStartX, m_btnStartY + m_btnStartH - 1.0f, 0.21f, m_btnStartW, 1.0f, startBorder);
    C2D_DrawRectSolid(m_btnStartX, m_btnStartY, 0.21f, 1.0f, m_btnStartH, startBorder);
    C2D_DrawRectSolid(m_btnStartX + m_btnStartW - 1.0f, m_btnStartY, 0.21f, 1.0f, m_btnStartH, startBorder);

    std::string startStr = isCurLocked ? "Locked" : "Start Game [A]";
    float sW = ui::BitmapFont::width(ui::BitmapFont::FontType::Font1x, startStr);
    u32 startCol = isCurLocked ? 0x888888FF : 0xFFFFFFFF;
    ui::BitmapFont::draw(ui::BitmapFont::FontType::Font1x, startStr, m_btnStartX + (m_btnStartW - sW) * 0.5f, m_btnStartY + 7.0f, 0.25f, startCol);
}

} // namespace pd::scenes

