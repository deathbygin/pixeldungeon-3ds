#include "pd/windows/WndHero.h"
#include "pd/Hero.h"
#include "pd/Buff.h"
#include "pd/HeroSubClass.h"
#include "pd/Dungeon.h"
#include "pd/Assets.h"
#include "noosa/Audio.h"
#include <cstdio>
#include <algorithm>

namespace pd::windows {

void WndHero::show() {
    m_open = true;
    m_tab = Tab::STATS;
}

bool WndHero::update(const pd::ui::TouchController& touch, u32 kDown, pd::Hero* hero) {
    (void)hero;
    if (!m_open) return false;

    // Toggle/Close via B or SELECT
    if (kDown & (KEY_B | KEY_SELECT)) {
        hide();
        return false;
    }

    // Tab switching via D-Pad Left/Right or L/R shoulders
    if (kDown & (KEY_DLEFT | KEY_L)) {
        m_tab = Tab::STATS;
    }
    if (kDown & (KEY_DRIGHT | KEY_R)) {
        m_tab = Tab::BUFFS;
    }

    // Touch handling for tabs
    float tabW = 80.0f;
    float tabH = 20.0f;
    if (touch.clicked(noosa::RectF(m_x + 8.0f, m_y + 6.0f, tabW, tabH))) {
        m_tab = Tab::STATS;
    }
    if (touch.clicked(noosa::RectF(m_x + 92.0f, m_y + 6.0f, tabW, tabH))) {
        m_tab = Tab::BUFFS;
    }

    // Close button at bottom
    float btnW = 100.0f;
    float btnH = 22.0f;
    float btnX = m_x + (m_w - btnW) * 0.5f;
    float btnY = m_y + m_h - 26.0f;
    if (touch.clicked(noosa::RectF(btnX, btnY, btnW, btnH))) {
        hide();
        return false;
    }

    return false;
}

void WndHero::draw(const pd::Hero* hero, C2D_TextBuf textBuf) {
    if (!m_open || !hero) return;

    // Dim background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, 0.70f, 320.0f, 240.0f, C2D_Color32(0x10, 0x14, 0x12, 0xD8));

    // Outer frame
    C2D_DrawRectSolid(m_x, m_y, 0.72f, m_w, m_h, C2D_Color32(0x44, 0x49, 0x46, 0xFF));
    C2D_DrawRectSolid(m_x, m_y, 0.74f, m_w, 1.0f, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(m_x, m_y + m_h - 1.0f, 0.74f, m_w, 1.0f, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(m_x, m_y, 0.74f, 1.0f, m_h, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(m_x + m_w - 1.0f, m_y, 0.74f, 1.0f, m_h, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));

    // Inner shadow border
    C2D_DrawRectSolid(m_x + 1.0f, m_y + 1.0f, 0.74f, m_w - 2.0f, 1.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(m_x + 1.0f, m_y + m_h - 2.0f, 0.74f, m_w - 2.0f, 1.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(m_x + 1.0f, m_y + 1.0f, 0.74f, 1.0f, m_h - 2.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(m_x + m_w - 2.0f, m_y + 1.0f, 0.74f, 1.0f, m_h - 2.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));

    // Tab Headers
    float tabW = 80.0f;
    float tabH = 18.0f;
    u32 statTabBg = (m_tab == Tab::STATS) ? C2D_Color32(0x5a, 0x61, 0x5d, 0xFF) : C2D_Color32(0x33, 0x38, 0x35, 0xFF);
    u32 buffTabBg = (m_tab == Tab::BUFFS) ? C2D_Color32(0x5a, 0x61, 0x5d, 0xFF) : C2D_Color32(0x33, 0x38, 0x35, 0xFF);

    C2D_DrawRectSolid(m_x + 8.0f, m_y + 6.0f, 0.75f, tabW, tabH, statTabBg);
    C2D_DrawRectSolid(m_x + 92.0f, m_y + 6.0f, 0.75f, tabW, tabH, buffTabBg);

    C2D_Text textObj;
    C2D_TextParse(&textObj, textBuf, "STATS");
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_WithColor, m_x + 28.0f, m_y + 8.0f, 0.78f, 0.40f, 0.40f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));

    C2D_TextParse(&textObj, textBuf, "BUFFS");
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_WithColor, m_x + 112.0f, m_y + 8.0f, 0.78f, 0.40f, 0.40f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));

    // Divider line
    C2D_DrawRectSolid(m_x + 4.0f, m_y + 26.0f, 0.75f, m_w - 8.0f, 1.0f, C2D_Color32(0x2d, 0x31, 0x2f, 0xFF));

    float contentY = m_y + 32.0f;

    if (m_tab == Tab::STATS) {
        // Hero Class Title
        char titleBuf[64];
        std::snprintf(titleBuf, sizeof(titleBuf), "LEVEL %d %s", hero->level, hero->className().c_str());
        C2D_TextParse(&textObj, textBuf, titleBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 12.0f, contentY, 0.78f, 0.50f, 0.50f, C2D_Color32(0xff, 0xd7, 0x00, 0xFF));
        contentY += 20.0f;

        // Subclass description if active
        if (hero->subClass != HeroSubClass::None) {
            const char* desc = subClassDesc(hero->subClass);
            C2D_TextParse(&textObj, textBuf, desc);
            C2D_TextOptimize(&textObj);
            C2D_DrawText(&textObj, C2D_WithColor, m_x + 12.0f, contentY, 0.78f, 0.30f, 0.30f, C2D_Color32(0xcc, 0xd0, 0xcc, 0xFF));
            contentY += 28.0f;
        }

        // Stats grid
        char statBuf[128];
        std::snprintf(statBuf, sizeof(statBuf), "Health:          %d / %d", hero->HP, hero->HT);
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        contentY += 15.0f;

        std::snprintf(statBuf, sizeof(statBuf), "Strength:        %d", hero->STR);
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        contentY += 15.0f;

        std::snprintf(statBuf, sizeof(statBuf), "Experience:    %d / %d", hero->exp, hero->maxExp);
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
        contentY += 15.0f;

        std::snprintf(statBuf, sizeof(statBuf), "Gold Pieces:   %d", hero->gold > 0 ? hero->gold : Dungeon::gold);
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xff, 0xd0, 0x33, 0xFF));
        contentY += 15.0f;

        std::snprintf(statBuf, sizeof(statBuf), "Dungeon Depth: %d", Dungeon::depth);
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xaa, 0xdd, 0xff, 0xFF));
        contentY += 15.0f;

        std::snprintf(statBuf, sizeof(statBuf), "Attack Skill:    %d", const_cast<Hero*>(hero)->attackSkill(nullptr));
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xcc, 0xcc, 0xcc, 0xFF));
        contentY += 15.0f;

        std::snprintf(statBuf, sizeof(statBuf), "Defense Skill:  %d", const_cast<Hero*>(hero)->defenseSkill(nullptr));
        C2D_TextParse(&textObj, textBuf, statBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 16.0f, contentY, 0.78f, 0.38f, 0.38f, C2D_Color32(0xcc, 0xcc, 0xcc, 0xFF));
    } else {
        // Buffs & Effects Tab
        C2D_TextParse(&textObj, textBuf, "ACTIVE BUFFS & CONDITIONS");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, m_x + 12.0f, contentY, 0.78f, 0.44f, 0.44f, C2D_Color32(0xff, 0xd7, 0x00, 0xFF));
        contentY += 18.0f;

        int buffCount = 0;
        for (const auto& b : hero->buffs) {
            if (!b) continue;
            std::string bName = b->name();
            std::string bDesc = "";
            u32 col = C2D_Color32(0xdd, 0xdd, 0xdd, 0xFF);

            switch (b->type()) {
                case BuffType::Regeneration:
                    bDesc = "Slowly restores health over time.";
                    col = C2D_Color32(0x56, 0xbb, 0x77, 0xFF);
                    break;
                case BuffType::Hunger:
                    bDesc = hero->isStarving() ? "Starving! Taking damage from starvation." : (hero->isHungry() ? "Hungry. Will start starving soon." : "Well-fed and energized.");
                    col = hero->isStarving() ? C2D_Color32(0xee, 0x44, 0x44, 0xFF) : (hero->isHungry() ? C2D_Color32(0xff, 0x99, 0x33, 0xFF) : C2D_Color32(0x56, 0xbb, 0x77, 0xFF));
                    break;
                case BuffType::Fury:
                    bDesc = "Berserker rage: Damage increased by +50%!";
                    col = C2D_Color32(0xff, 0x33, 0x33, 0xFF);
                    break;
                case BuffType::Combo: {
                    Combo* cmb = b->asCombo();
                    bDesc = "Gladiator combo: " + std::to_string(cmb ? cmb->count : 0) + " hits landed in succession.";
                    col = C2D_Color32(0xff, 0x77, 0x33, 0xFF);
                    break;
                }
                case BuffType::Barkskin:
                    bDesc = "Natural hardened bark absorbing incoming physical damage.";
                    col = C2D_Color32(0x88, 0xbb, 0x44, 0xFF);
                    break;
                case BuffType::Shadows:
                    bDesc = "Stealth concealment reducing mob detection radius.";
                    col = C2D_Color32(0xaa, 0x88, 0xdd, 0xFF);
                    break;
                case BuffType::SnipersMark:
                    bDesc = "Target marked: Armor completely ignored on next missile hit.";
                    col = C2D_Color32(0x33, 0xcc, 0xdd, 0xFF);
                    break;
                case BuffType::Invisibility:
                    bDesc = "Completely invisible to enemies until attacking.";
                    col = C2D_Color32(0x55, 0x99, 0xdd, 0xFF);
                    break;
                case BuffType::Burning:
                    bDesc = "Engulfed in flames! Taking continuous fire damage.";
                    col = C2D_Color32(0xff, 0x55, 0x11, 0xFF);
                    break;
                case BuffType::Poison:
                    bDesc = "Deadly venom coursing through veins.";
                    col = C2D_Color32(0x33, 0xaa, 0x44, 0xFF);
                    break;
                case BuffType::Paralysis:
                    bDesc = "Completely immobilized and unable to act.";
                    col = C2D_Color32(0xee, 0x33, 0x33, 0xFF);
                    break;
                default:
                    bDesc = "Active effect.";
                    break;
            }

            std::string line = "- " + bName + ": " + bDesc;
            C2D_TextParse(&textObj, textBuf, line.c_str());
            C2D_TextOptimize(&textObj);
            C2D_DrawText(&textObj, C2D_WithColor, m_x + 14.0f, contentY, 0.78f, 0.32f, 0.32f, col);
            contentY += 14.0f;
            buffCount++;
        }

        if (buffCount == 0) {
            C2D_TextParse(&textObj, textBuf, "No active status effects.");
            C2D_TextOptimize(&textObj);
            C2D_DrawText(&textObj, C2D_WithColor, m_x + 14.0f, contentY, 0.78f, 0.36f, 0.36f, C2D_Color32(0x88, 0x88, 0x88, 0xFF));
        }
    }

    // Close Button [B]
    float btnW = 100.0f;
    float btnH = 20.0f;
    float btnX = m_x + (m_w - btnW) * 0.5f;
    float btnY = m_y + m_h - 24.0f;
    C2D_DrawRectSolid(btnX, btnY, 0.76f, btnW, btnH, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(btnX, btnY, 0.77f, btnW, 1.0f, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(btnX, btnY + btnH - 1.0f, 0.77f, btnW, 1.0f, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(btnX, btnY, 0.77f, 1.0f, btnH, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(btnX + btnW - 1.0f, btnY, 0.77f, 1.0f, btnH, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));

    C2D_TextParse(&textObj, textBuf, "CLOSE [B]");
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_WithColor, btnX + 18.0f, btnY + 3.0f, 0.78f, 0.38f, 0.38f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
}

} // namespace pd::windows

