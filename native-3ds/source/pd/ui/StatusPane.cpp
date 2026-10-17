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
    // Avatar shield touch area: (X_POS, Y_POS, 26, HEIGHT)
    noosa::RectF avatarRect(X_POS, Y_POS, 26.0f, HEIGHT);
    m_avatarPressed = touch.hitTest(avatarRect);
    if (touch.clicked(avatarRect)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return 1; // Toggle Hero Info
    }

    // Depth / stairs icon touch area: (X_POS + 144, Y_POS, 44, HEIGHT)
    noosa::RectF depthRect(X_POS + 144.0f, Y_POS, 44.0f, HEIGHT);
    if (touch.clicked(depthRect)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return 3; // Stairs / Depth info
    }

    // Gold coins icon touch area: (X_POS + 190, Y_POS, 85, HEIGHT) - Secret Developer / Debug Menu
    noosa::RectF goldRect(X_POS + 190.0f, Y_POS, 85.0f, HEIGHT);
    if (touch.clicked(goldRect)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return 4; // Open Developer / Debug Menu
    }

    // Menu button touch area: (X_POS + WIDTH - 30, Y_POS, 30, HEIGHT)
    noosa::RectF menuRect(X_POS + WIDTH - 30.0f, Y_POS, 30.0f, HEIGHT);
    m_menuPressed = touch.hitTest(menuRect);
    if (touch.clicked(menuRect)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return 2; // Open Menu / Options
    }

    return 0;
}

const pd::Buff* StatusPane::clickedBuff(const pd::Hero* hero, const TouchController& touch) const {
    if (!hero) return nullptr;

    float bx = X_POS + 26.0f;
    float by = Y_POS + 19.0f;

    for (const pd::Buff* b : hero->buffs) {
        if (!b) continue;
        int iconIdx = b->icon();
        if (iconIdx < 0) continue;

        noosa::RectF hitRect(bx - 1.0f, by - 1.0f, 10.0f, 10.0f);
        if (touch.clicked(hitRect)) {
            return b;
        }
        bx += 10.0f;
        if (bx > X_POS + 135.0f) break;
    }
    return nullptr;
}

void StatusPane::draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) const {
    (void)textBuf;
    if (!hero) return;

    float zHeader = 0.30f;
    float zContent = 0.35f;
    float zText = 0.40f;

    // 1. Raised 9-slice stone chrome container (matching Hero Stats, Minimap & Log)
    Chrome::get(Chrome::Type::TOAST).draw(X_POS, Y_POS, WIDTH, HEIGHT, zHeader);

    // 2. Hero Avatar Sprite seated cleanly inside left margin (x=X_POS + 6, y=Y_POS + 4, 16x16)
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
        pHero.pos.x = X_POS + 6.0f; pHero.pos.y = Y_POS + 4.0f;
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

    // Hero level text badge directly below avatar
    std::string lvlStr = std::to_string(hero->level);
    float lvlW = BitmapFont::width(BitmapFont::FontType::Font1x, lvlStr);
    float lvlX = std::round(X_POS + 14.0f - (lvlW * 0.5f));
    float lvlY = Y_POS + 20.0f;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, lvlStr, lvlX, lvlY, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

    // 3. Two-tone HP Bar (x: X_POS + 26, y: Y_POS + 5, w: 114, h: 12)
    float barX = X_POS + 26.0f;
    float barY = Y_POS + 5.0f;
    float barW = 114.0f;
    float barH = 12.0f;
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
    float hpTextY = barY + 2.0f;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, hpBuf, hpTextX, hpTextY, zText, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

    // 4. Status Badges Tray (x: X_POS + 26, y: Y_POS + 19)
    if (m_texBuffs && m_texBuffs->isLoaded()) {
        int potW = m_texBuffs->potWidth();
        int potH = m_texBuffs->potHeight();
        C3D_Tex* bTex = m_texBuffs->getTex();

        float bx = barX;
        float by = Y_POS + 19.0f;

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

        for (const pd::Buff* b : hero->buffs) {
            if (!b) continue;
            int iconIdx = b->icon();
            if (iconIdx >= 0 && bx <= (barX + barW - 8.0f)) {
                drawBuffIcon(iconIdx);
            }
        }
    }

    // 5. Dungeon Info Cluster
    // Depth: Staircase icon + text
    if (m_texIcons && m_texIcons->isLoaded()) {
        int potW = m_texIcons->potWidth();
        int potH = m_texIcons->potHeight();
        C3D_Tex* iTex = m_texIcons->getTex();

        setupSubTex(m_subDepthIcon, potW, potH, 45, 12, 9, 8);
        C2D_Image imgDepth = { iTex, &m_subDepthIcon };
        C2D_DrawParams pD;
        pD.pos.x = X_POS + 148.0f; pD.pos.y = Y_POS + 10.0f; pD.pos.w = 11.0f; pD.pos.h = 10.0f;
        pD.center.x = pD.center.y = 0.0f; pD.depth = zContent; pD.angle = 0.0f;
        C2D_DrawImage(imgDepth, &pD, nullptr);

        char depthBuf[16];
        std::snprintf(depthBuf, sizeof(depthBuf), "%d", depth);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, depthBuf, X_POS + 162.0f, Y_POS + 11.0f, zText, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
    }

    // Gold: Coin pile icon + numeric count
    if (m_texItems && m_texItems->isLoaded()) {
        int potW = m_texItems->potWidth();
        int potH = m_texItems->potHeight();
        setupSubTex(m_subGoldIcon, potW, potH, (ItemSpriteSheet::GOLD % 8) * 16, (ItemSpriteSheet::GOLD / 8) * 16, 16, 16);
        C2D_Image imgGold = { m_texItems->getTex(), &m_subGoldIcon };
        C2D_DrawParams pGold;
        pGold.pos.x = X_POS + 192.0f; pGold.pos.y = Y_POS + 8.0f; pGold.pos.w = 14.0f; pGold.pos.h = 14.0f;
        pGold.center.x = pGold.center.y = 0.0f; pGold.depth = zContent; pGold.angle = 0.0f;
        C2D_DrawImage(imgGold, &pGold, nullptr);

        char goldBuf[32];
        std::snprintf(goldBuf, sizeof(goldBuf), "%d", hero->gold);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, goldBuf, X_POS + 209.0f, Y_POS + 11.0f, zText, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
    }

    // 6. Burger Menu Button (x: X_POS + WIDTH - 22, y: Y_POS + 9, w: 14, h: 13)
    if (m_texStatus && m_texStatus->isLoaded()) {
        int potW = m_texStatus->potWidth();
        int potH = m_texStatus->potHeight();
        setupSubTex(m_subBtnMenu, potW, potH, 114, 3, 12, 11);
        C2D_Image imgMenu = { m_texStatus->getTex(), &m_subBtnMenu };
        C2D_DrawParams pMenu;
        float shift = m_menuPressed ? 1.0f : 0.0f;
        pMenu.pos.x = X_POS + WIDTH - 22.0f;
        pMenu.pos.y = Y_POS + 9.0f + shift;
        pMenu.pos.w = 14.0f;
        pMenu.pos.h = 13.0f;
        pMenu.center.x = pMenu.center.y = 0.0f;
        pMenu.depth = zContent + 0.01f;
        pMenu.angle = 0.0f;

        if (m_menuPressed) {
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xEE, 0xAA, 0xFF), 0.5f);
            C2D_DrawImage(imgMenu, &pMenu, &tint);
        } else {
            C2D_DrawImage(imgMenu, &pMenu, nullptr);
        }
    }
}

} // namespace pd::ui
