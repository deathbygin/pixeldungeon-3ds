#include "pd/ui/StatusBar.h"
#include "pd/Hero.h"
#include "pd/Buff.h"
#include "pd/HeroSubClass.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace pd::ui {

void StatusBar::update(float dt) {
    (void)dt;
}

void StatusBar::draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) const {
    if (!hero) return;

    // Outer panel
    C2D_DrawRectSolid(x, y, 0.3f, width, height, C2D_Color32(0x2b, 0x24, 0x3a, 0xFF));
    C2D_DrawRectSolid(x, y, 0.35f, width, 1.0f, C2D_Color32(0x63, 0x48, 0x3d, 0xFF));
    C2D_DrawRectSolid(x, y + height - 1.0f, 0.35f, width, 1.0f, C2D_Color32(0x63, 0x48, 0x3d, 0xFF));
    C2D_DrawRectSolid(x, y, 0.35f, 1.0f, height, C2D_Color32(0x63, 0x48, 0x3d, 0xFF));
    C2D_DrawRectSolid(x + width - 1.0f, y, 0.35f, 1.0f, height, C2D_Color32(0x63, 0x48, 0x3d, 0xFF));

    // Stats line 1
    char buf[128];
    std::snprintf(buf, sizeof(buf), "LV %d %s  |  Depth %d  |  Gold %d  |  STR %d",
                  hero->level, hero->className().c_str(), depth, hero->gold, hero->STR);
    C2D_Text textObj;
    C2D_TextParse(&textObj, textBuf, buf);
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_WithColor, x + 8.0f, y + 4.0f, 0.45f, 0.38f, 0.38f, C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));

    // HP Bar background
    float barW = width - 16.0f;
    float barH = 12.0f;
    float barX = x + 8.0f;
    float barY = y + 18.0f;
    C2D_DrawRectSolid(barX, barY, 0.4f, barW, barH, C2D_Color32(0x18, 0x14, 0x25, 0xFF));

    // HP Bar fill
    float hpPct = std::clamp(static_cast<float>(hero->HP) / static_cast<float>(hero->HT), 0.0f, 1.0f);
    u32 hpColor = (hpPct > 0.5f) ? C2D_Color32(0x38, 0x9e, 0x38, 0xFF) :
                  (hpPct > 0.25f) ? C2D_Color32(0xd6, 0x9e, 0x2e, 0xFF) : C2D_Color32(0xd6, 0x30, 0x30, 0xFF);

    if (hero->buff(BuffType::Fury)) {
        // Red pulse outline for Berserker Fury
        C2D_DrawRectSolid(barX - 1.0f, barY - 1.0f, 0.43f, barW + 2.0f, barH + 2.0f, C2D_Color32(0xff, 0x22, 0x22, 0xFF));
        hpColor = C2D_Color32(0xff, 0x44, 0x22, 0xFF);
    }
    C2D_DrawRectSolid(barX, barY, 0.45f, barW * hpPct, barH, hpColor);

    // HP Text
    std::snprintf(buf, sizeof(buf), "HP: %d/%d", hero->HP, hero->HT);
    C2D_TextParse(&textObj, textBuf, buf);
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_WithColor, barX + 6.0f, barY + 1.0f, 0.5f, 0.32f, 0.32f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));

    // EXP Bar
    float expY = barY + barH + 3.0f;
    float expH = 6.0f;
    C2D_DrawRectSolid(barX, expY, 0.4f, barW, expH, C2D_Color32(0x18, 0x14, 0x25, 0xFF));
    float expPct = std::clamp(static_cast<float>(hero->exp) / static_cast<float>(hero->maxExp), 0.0f, 1.0f);
    C2D_DrawRectSolid(barX, expY, 0.45f, barW * expPct, expH, C2D_Color32(0x9a, 0x6f, 0xd8, 0xFF));

    // Active Status / Buff Badges
    float badgeX = barX + 54.0f;
    if (hero->buff(BuffType::Fury)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 40.0f, 9.0f, C2D_Color32(0xff, 0x11, 0x11, 0xFF));
        C2D_TextParse(&textObj, textBuf, "FURY");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 4.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0x88, 0xFF));
        badgeX += 44.0f;
    }
    Combo* combo = const_cast<Hero*>(hero)->getBuff<Combo>();
    if (combo && combo->count > 0) {
        std::string cStr = "COMBO " + std::to_string(combo->count) + "x";
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 56.0f, 9.0f, C2D_Color32(0xcc, 0x33, 0x33, 0xFF));
        C2D_TextParse(&textObj, textBuf, cStr.c_str());
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 60.0f;
    }
    if (hero->buff(BuffType::SnipersMark)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 48.0f, 9.0f, C2D_Color32(0x22, 0x88, 0x99, 0xFF));
        C2D_TextParse(&textObj, textBuf, "SNIPER");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 52.0f;
    }
    Barkskin* bark = const_cast<Hero*>(hero)->getBuff<Barkskin>();
    if (bark) {
        std::string bStr = "BARK +" + std::to_string(bark->getLevel());
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 50.0f, 9.0f, C2D_Color32(0x66, 0x88, 0x33, 0xFF));
        C2D_TextParse(&textObj, textBuf, bStr.c_str());
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 54.0f;
    }
    if (hero->buff(BuffType::Shadows)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 50.0f, 9.0f, C2D_Color32(0x44, 0x33, 0x66, 0xFF));
        C2D_TextParse(&textObj, textBuf, "SHADOW");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xdd, 0xcc, 0xff, 0xFF));
        badgeX += 54.0f;
    }
    if (hero->isStarving()) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 52.0f, 9.0f, C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
        C2D_TextParse(&textObj, textBuf, "STARVING");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 56.0f;
    } else if (hero->isHungry()) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 46.0f, 9.0f, C2D_Color32(0xff, 0x88, 0x00, 0xFF));
        C2D_TextParse(&textObj, textBuf, "HUNGRY");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 50.0f;
    }
    if (hero->paralysed) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 54.0f, 9.0f, C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
        C2D_TextParse(&textObj, textBuf, "PARALYSED");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 58.0f;
    }
    if (hero->buff(BuffType::Burning)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 44.0f, 9.0f, C2D_Color32(0xee, 0x77, 0x22, 0xFF));
        C2D_TextParse(&textObj, textBuf, "BURNING");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 48.0f;
    }
    if (hero->buff(BuffType::Poison)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 42.0f, 9.0f, C2D_Color32(0x38, 0x9e, 0x38, 0xFF));
        C2D_TextParse(&textObj, textBuf, "POISON");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 46.0f;
    }
    if (hero->buff(BuffType::Cripple)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 44.0f, 9.0f, C2D_Color32(0x8a, 0x6f, 0x5a, 0xFF));
        C2D_TextParse(&textObj, textBuf, "CRIPPLE");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 2.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 48.0f;
    }
    if (hero->invisible > 0) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 50.0f, 9.0f, C2D_Color32(0x38, 0x6e, 0x9e, 0xFF));
        C2D_TextParse(&textObj, textBuf, "INVIS");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 4.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        badgeX += 54.0f;
    }
    if (hero->buff(BuffType::Ooze)) {
        C2D_DrawRectSolid(badgeX, expY - 1.0f, 0.46f, 40.0f, 9.0f, C2D_Color32(0x30, 0x50, 0x30, 0xFF));
        C2D_TextParse(&textObj, textBuf, "OOZE");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, badgeX + 4.0f, expY - 1.0f, 0.5f, 0.28f, 0.28f, C2D_Color32(0x80, 0xff, 0x80, 0xFF));
    }
}

} // namespace pd::ui

