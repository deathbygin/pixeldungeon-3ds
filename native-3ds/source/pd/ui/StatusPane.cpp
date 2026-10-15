#include "pd/ui/StatusPane.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Hero.h"
#include "pd/Buff.h"
#include "pd/HeroSubClass.h"
#include "pd/Dungeon.h"
#include "pd/Assets.h"
#include "pd/ItemSpriteSheet.h"
#include "noosa/Audio.h"
#include "pd/ui/Chrome.h"
#include <cstdio>
#include <algorithm>

namespace pd::ui {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int fx, int fy, int fw, int fh) {
    sub.width = static_cast<u16>(fw);
    sub.height = static_cast<u16>(fh);
    sub.left = static_cast<float>(fx) / static_cast<float>(potW);
    sub.right = static_cast<float>(fx + fw) / static_cast<float>(potW);
    sub.top = 1.0f - (static_cast<float>(fy) / static_cast<float>(potH));
    sub.bottom = 1.0f - (static_cast<float>(fy + fh) / static_cast<float>(potH));
}

StatusPane::StatusPane() {
    m_texStatus = noosa::Texture::get(Assets::STATUS);
    m_texHp = noosa::Texture::get(Assets::HP_BAR);
    m_texExp = noosa::Texture::get(Assets::XP_BAR);
    m_texBuffs = noosa::Texture::get(Assets::BUFFS_SMALL);
    m_texIcons = noosa::Texture::get(Assets::ICONS);
    m_texItems = noosa::Texture::get(Assets::ITEMS);
}

int StatusPane::update(const TouchController& touch, u32 kDown) {
    (void)kDown;
    // Avatar shield touch area: (0, 0, 40, HEIGHT)
    noosa::RectF avatarRect(0.0f, 0.0f, 40.0f, HEIGHT);
    if (touch.clicked(avatarRect)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return 1; // Toggle Hero Info
    }

    // Menu button touch area: (290, 0, 30, HEIGHT)
    noosa::RectF menuRect(290.0f, 0.0f, 30.0f, HEIGHT);
    if (touch.clicked(menuRect)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return 2; // Open Menu / Options
    }

    return 0;
}

void StatusPane::draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) const {
    (void)textBuf;
    if (!hero) return;

    float zHeader = 0.50f;
    float zContent = 0.52f;
    float zText = 0.55f;

    // 1. Header base strip (y: 0..32, w: 320, h: 33)
    C2D_DrawRectSolid(0.0f, 0.0f, zHeader, WIDTH, HEIGHT, C2D_Color32(0x1B, 0x18, 0x22, 0xFF));

    // 2. Corner Shield Frame from status_pane.png
    if (m_texStatus && m_texStatus->isLoaded()) {
        int potW = m_texStatus->potWidth();
        int potH = m_texStatus->potHeight();
        C3D_Tex* tex = m_texStatus->getTex();

        // Top horizontal border runner from x=35 to x=320
        setupSubTex(m_subTopBar, potW, potH, 35, 0, 45, 4);
        C2D_Image imgTopBar = { tex, &m_subTopBar };
        C2D_DrawParams pTop;
        pTop.pos.x = 35.0f; pTop.pos.y = 0.0f;
        pTop.pos.w = 285.0f; pTop.pos.h = 4.0f;
        pTop.center.x = pTop.center.y = 0.0f; pTop.depth = zHeader + 0.01f; pTop.angle = 0.0f;
        C2D_DrawImage(imgTopBar, &pTop, nullptr);

        // Shield frame at (0, 0, 35, 35) - 1:1 pixel mapping with complete corner notch badge
        setupSubTex(m_subShield, potW, potH, 0, 0, 35, 35);
        C2D_Image imgShield = { tex, &m_subShield };
        C2D_DrawParams pShield;
        pShield.pos.x = 0.0f; pShield.pos.y = 0.0f;
        pShield.pos.w = 35.0f; pShield.pos.h = 35.0f;
        pShield.center.x = pShield.center.y = 0.0f; pShield.depth = zHeader + 0.01f; pShield.angle = 0.0f;
        C2D_DrawImage(imgShield, &pShield, nullptr);
    }

    // 3. Hero Avatar Sprite seated cleanly inside recessed notch (x=8, y=5, 16x16)
    std::string heroSheet = Assets::WARRIOR;
    if (hero->heroClass == Hero::HeroClass::Mage) heroSheet = Assets::MAGE;
    else if (hero->heroClass == Hero::HeroClass::Rogue) heroSheet = Assets::ROGUE;
    else if (hero->heroClass == Hero::HeroClass::Huntress) heroSheet = Assets::HUNTRESS;

    auto texHero = noosa::Texture::get(heroSheet);
    if (texHero && texHero->isLoaded()) {
        Tex3DS_SubTexture subHero;
        setupSubTex(subHero, texHero->potWidth(), texHero->potHeight(), 0, 0, 12, 15);
        C2D_Image imgHero = { texHero->getTex(), &subHero };
        C2D_DrawParams pHero;
        pHero.pos.x = 8.0f; pHero.pos.y = 5.0f;
        pHero.pos.w = 16.0f; pHero.pos.h = 16.0f;
        pHero.center.x = pHero.center.y = 0.0f; pHero.depth = zContent; pHero.angle = 0.0f;

        float hpRatio = static_cast<float>(hero->HP) / std::max(1, hero->HT);
        if (hpRatio < 0.25f) {
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0xff, 0x66, 0x66, 0xFF), 0.6f);
            C2D_DrawImage(imgHero, &pHero, &tint);
        } else {
            C2D_DrawImage(imgHero, &pHero, nullptr);
        }
    }

    // Hero level text badge seated centered directly inside dedicated corner notch (center x=27.0, y=28.0)
    std::string lvlStr = std::to_string(hero->level);
    float lvlW = BitmapFont::width(BitmapFont::FontType::Font1x, lvlStr);
    float lvlX = std::round(27.0f - (lvlW * 0.5f));
    float lvlY = 25.0f;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, lvlStr, lvlX, lvlY, zText, 0xFFEBA4FF, 0x000000FF);

    // 4. Two-tone HP Bar (x: 44, y: 4, w: 140, h: 14)
    float barX = 44.0f;
    float barY = 4.0f;
    float barW = 140.0f;
    float barH = 14.0f;
    float hpPct = std::clamp(static_cast<float>(hero->HP) / std::max(1, hero->HT), 0.0f, 1.0f);

    // 1px Outer dark recessed border
    C2D_DrawRectSolid(barX - 1.0f, barY - 1.0f, zContent - 0.02f, barW + 2.0f, barH + 2.0f, C2D_Color32(0x14, 0x10, 0x18, 0xFF));
    // Dark red background
    C2D_DrawRectSolid(barX, barY, zContent - 0.01f, barW, barH, C2D_Color32(0x3D, 0x11, 0x11, 0xFF));
    // Bright red current fill
    float fillW = std::floor(barW * hpPct);
    if (fillW > 0.0f) {
        u32 hpFillCol = hero->buff(BuffType::Fury) ? C2D_Color32(0xFF, 0x11, 0x11, 0xFF) : C2D_Color32(0xC9, 0x22, 0x22, 0xFF);
        C2D_DrawRectSolid(barX, barY, zContent, fillW, barH, hpFillCol);
    }

    // Centered HP ratio text with crisp 1px drop-shadow
    char hpBuf[32];
    std::snprintf(hpBuf, sizeof(hpBuf), "%d/%d", hero->HP, hero->HT);
    float hpTextW = BitmapFont::width(BitmapFont::FontType::Font1x, hpBuf);
    float hpTextX = std::floor(barX + (barW - hpTextW) * 0.5f);
    float hpTextY = 7.0f;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, hpBuf, hpTextX, hpTextY, zText, 0xFFFFFFFF, 0x000000FF);

    // 5. Status Badges Tray (x: 44, y: 20)
    if (m_texBuffs && m_texBuffs->isLoaded()) {
        int potW = m_texBuffs->potWidth();
        int potH = m_texBuffs->potHeight();
        C3D_Tex* bTex = m_texBuffs->getTex();

        float bx = 44.0f;
        float by = 21.0f;

        auto drawBuffIcon = [&](int buffIdx) {
            int sx = (buffIdx % 16) * 7;
            int sy = (buffIdx / 16) * 7;
            setupSubTex(m_subBuffIcon, potW, potH, sx, sy, 7, 7);
            C2D_Image imgB = { bTex, &m_subBuffIcon };
            C2D_DrawParams pB;
            pB.pos.x = bx; pB.pos.y = by; pB.pos.w = 8.0f; pB.pos.h = 8.0f;
            pB.center.x = pB.center.y = 0.0f; pB.depth = zContent; pB.angle = 0.0f;
            C2D_DrawImage(imgB, &pB, nullptr);
            bx += 10.0f;
        };

        if (hero->isStarving()) drawBuffIcon(6); // STARVATION
        else if (hero->isHungry()) drawBuffIcon(5); // HUNGER
        if (hero->buff(BuffType::Poison)) drawBuffIcon(3);
        if (hero->buff(BuffType::Bleeding)) drawBuffIcon(26);
        if (hero->buff(BuffType::Burning)) drawBuffIcon(2);
        if (hero->buff(BuffType::Paralysis)) drawBuffIcon(4);
        if (hero->buff(BuffType::Fury)) drawBuffIcon(18);
        if (hero->buff(BuffType::Combo)) drawBuffIcon(17);
        if (hero->buff(BuffType::SnipersMark)) drawBuffIcon(27);
        if (hero->buff(BuffType::Barkskin)) drawBuffIcon(24);
        if (hero->buff(BuffType::Shadows)) drawBuffIcon(13);
        if (hero->buff(BuffType::Invisibility)) drawBuffIcon(12);
        if (hero->buff(BuffType::Regeneration)) drawBuffIcon(19);
        if (hero->buff(BuffType::Roots)) drawBuffIcon(11);
        if (hero->buff(BuffType::Cripple)) drawBuffIcon(23);
    }

    // 6. Dungeon Info Cluster (x: 190 to 290, y: 4..28)
    // Depth: Staircase icon + text
    if (m_texIcons && m_texIcons->isLoaded()) {
        int potW = m_texIcons->potWidth();
        int potH = m_texIcons->potHeight();
        C3D_Tex* iTex = m_texIcons->getTex();

        setupSubTex(m_subDepthIcon, potW, potH, 45, 12, 9, 8);
        C2D_Image imgDepth = { iTex, &m_subDepthIcon };
        C2D_DrawParams pD;
        pD.pos.x = 192.0f; pD.pos.y = 8.0f; pD.pos.w = 11.0f; pD.pos.h = 10.0f;
        pD.center.x = pD.center.y = 0.0f; pD.depth = zContent; pD.angle = 0.0f;
        C2D_DrawImage(imgDepth, &pD, nullptr);

        char depthBuf[16];
        std::snprintf(depthBuf, sizeof(depthBuf), "%d", depth);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, depthBuf, 206.0f, 9.0f, zText, 0xCACFC2FF, 0x000000FF);
    }

    // Gold: Coin pile icon + numeric count
    if (m_texItems && m_texItems->isLoaded()) {
        int potW = m_texItems->potWidth();
        int potH = m_texItems->potHeight();
        setupSubTex(m_subGoldIcon, potW, potH, (ItemSpriteSheet::GOLD % 8) * 16, (ItemSpriteSheet::GOLD / 8) * 16, 16, 16);
        C2D_Image imgGold = { m_texItems->getTex(), &m_subGoldIcon };
        C2D_DrawParams pGold;
        pGold.pos.x = 238.0f; pGold.pos.y = 6.0f; pGold.pos.w = 14.0f; pGold.pos.h = 14.0f;
        pGold.center.x = pGold.center.y = 0.0f; pGold.depth = zContent; pGold.angle = 0.0f;
        C2D_DrawImage(imgGold, &pGold, nullptr);

        char goldBuf[32];
        std::snprintf(goldBuf, sizeof(goldBuf), "%d", hero->gold);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, goldBuf, 255.0f, 9.0f, zText, 0xE4A672FF, 0x000000FF);
    }

    // 7. Menu Button (x: 294, y: 4, w: 22, h: 22)
    Chrome::get(Chrome::Type::BUTTON).draw(294.0f, 4.0f, 22.0f, 22.0f, zContent);
    if (m_texStatus && m_texStatus->isLoaded()) {
        int potW = m_texStatus->potWidth();
        int potH = m_texStatus->potHeight();
        setupSubTex(m_subBtnMenu, potW, potH, 114, 3, 12, 11);
        C2D_Image imgMenu = { m_texStatus->getTex(), &m_subBtnMenu };
        C2D_DrawParams pMenu;
        pMenu.pos.x = 299.0f; pMenu.pos.y = 9.0f; pMenu.pos.w = 12.0f; pMenu.pos.h = 11.0f;
        pMenu.center.x = pMenu.center.y = 0.0f; pMenu.depth = zContent + 0.01f; pMenu.angle = 0.0f;
        C2D_DrawImage(imgMenu, &pMenu, nullptr);
    }

    // 8. Bottom divider border & EXP Progress line at y: 32
    C2D_DrawRectSolid(0.0f, 32.0f, zHeader + 0.01f, WIDTH, 1.0f, C2D_Color32(0x30, 0x2C, 0x3E, 0xFF));
    float expPct = std::clamp(static_cast<float>(hero->exp) / std::max(1, hero->maxExp), 0.0f, 1.0f);
    if (expPct > 0.0f) {
        C2D_DrawRectSolid(0.0f, 32.0f, zContent, WIDTH * expPct, 1.0f, C2D_Color32(0x90, 0x40, 0xD0, 0xFF));
    }
}

} // namespace pd::ui
