#include "pd/windows/WndHero.h"
#include "pd/Hero.h"
#include "pd/Buff.h"
#include "pd/HeroSubClass.h"
#include "pd/Dungeon.h"
#include "pd/Assets.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "noosa/Audio.h"
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace pd::windows {

WndHero::WndHero() {
    m_texLargeBuffs = noosa::Texture::get(Assets::BUFFS_LARGE);
}

void WndHero::show() {
    m_open = true;
    m_tab = Tab::STATS;
}

bool WndHero::update(const pd::ui::TouchController& touch, u32 kDown, pd::Hero* hero) {
    (void)hero;
    if (!m_open) return false;

    // Toggle/Close via B, SELECT, or START
    if (kDown & (KEY_B | KEY_SELECT | KEY_START)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        hide();
        return false;
    }

    // Tab switching via D-Pad Left/Right or L/R shoulders
    if (kDown & (KEY_DLEFT | KEY_L)) {
        if (m_tab != Tab::STATS) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
            m_tab = Tab::STATS;
        }
    }
    if (kDown & (KEY_DRIGHT | KEY_R)) {
        if (m_tab != Tab::BUFFS) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
            m_tab = Tab::BUFFS;
        }
    }

    // Touch handling for tabs along top edge
    float tabY = m_y - 13.0f;
    float tabW = 60.0f;
    float tabH = 16.0f;
    float tab1X = m_x + 14.0f;
    float tab2X = tab1X + tabW + 6.0f;

    if (touch.clicked(noosa::RectF(tab1X, tabY, tabW, tabH))) {
        if (m_tab != Tab::STATS) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
            m_tab = Tab::STATS;
        }
    }
    if (touch.clicked(noosa::RectF(tab2X, tabY, tabW, tabH))) {
        if (m_tab != Tab::BUFFS) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
            m_tab = Tab::BUFFS;
        }
    }

    // Close button at bottom
    float btnW = 90.0f;
    float btnH = 20.0f;
    float btnX = std::round(m_x + (m_w - btnW) * 0.5f);
    float btnY = m_y + m_h - 26.0f;
    if (touch.clicked(noosa::RectF(btnX, btnY, btnW, btnH))) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        hide();
        return false;
    }

    return false;
}

void WndHero::draw(const pd::Hero* hero, C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open || !hero) return;

    float zDim  = 0.70f;
    float zWin  = 0.72f;
    float zText = zWin + 0.04f;

    // 1. Semi-transparent dark background backdrop
    C2D_DrawRectSolid(0.0f, 0.0f, zDim, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xAA));

    // 2. 9-slice stone window chrome from chrome.png
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, zWin);

    // 3. Top Tabs: Stats & Buffs
    float tabY = m_y - 13.0f;
    float tabW = 60.0f;
    float tabH = 16.0f;
    float tab1X = m_x + 14.0f;
    float tab2X = tab1X + tabW + 6.0f;

    bool isStats = (m_tab == Tab::STATS);

    // Tab 1 (Stats)
    const auto& patchTab1 = pd::ui::Chrome::get(isStats ? pd::ui::Chrome::Type::TAB_SELECTED : pd::ui::Chrome::Type::TAB_UNSELECTED);
    patchTab1.draw(tab1X, tabY, tabW, tabH, isStats ? (zWin + 0.02f) : (zWin + 0.01f));
    float sTxtW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Stats");
    float sTxtX = std::round(tab1X + (tabW - sTxtW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Stats", sTxtX, tabY + 4.0f, zWin + 0.04f, isStats ? 0xFFEE88FF : 0x889988FF, 0x000000FF);

    // Tab 2 (Buffs)
    const auto& patchTab2 = pd::ui::Chrome::get(!isStats ? pd::ui::Chrome::Type::TAB_SELECTED : pd::ui::Chrome::Type::TAB_UNSELECTED);
    patchTab2.draw(tab2X, tabY, tabW, tabH, !isStats ? (zWin + 0.02f) : (zWin + 0.01f));
    float bTxtW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Buffs");
    float bTxtX = std::round(tab2X + (tabW - bTxtW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Buffs", bTxtX, tabY + 4.0f, zWin + 0.04f, !isStats ? 0xFFEE88FF : 0x889988FF, 0x000000FF);

    float curY = m_y + 14.0f;

    if (m_tab == Tab::STATS) {
        // Hero Class Title
        std::string title = "LEVEL " + std::to_string(hero->level) + " " + hero->className();
        for (auto& c : title) c = static_cast<char>(std::toupper(c));

        float titleW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, title);
        float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, title, titleX, curY, zText, 0xFFFF44FF, 0x000000FF);
        curY += 14.0f;

        // Subclass description if active
        if (hero->subClass != HeroSubClass::None) {
            std::string desc = subClassDesc(hero->subClass);
            float descW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, desc);
            float descX = std::round(m_x + (m_w - descW) * 0.5f);
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, desc, descX, curY, zText, 0xCACFC2FF, 0x000000FF);
            curY += 12.0f;
        }

        // Horizontal divider
        C2D_DrawRectSolid(m_x + 14.0f, curY, zWin + 0.03f, m_w - 28.0f, 1.0f, C2D_Color32(0x35, 0x30, 0x2A, 0xFF));
        C2D_DrawRectSolid(m_x + 14.0f, curY + 1.0f, zWin + 0.03f, m_w - 28.0f, 1.0f, C2D_Color32(0x5A, 0x55, 0x4D, 0xFF));
        curY += 8.0f;

        // Stats grid
        struct StatRow {
            std::string label;
            std::string value;
            uint32_t valColor;
        };

        std::vector<StatRow> stats = {
            { "Health:",        std::to_string(hero->HP) + " / " + std::to_string(hero->HT), (hero->HP == hero->HT) ? 0x56BB77FF : 0xFFFFFFFF },
            { "Strength:",      std::to_string(hero->STR), 0xFFFFFFFF },
            { "Experience:",    std::to_string(hero->exp) + " / " + std::to_string(hero->maxExp), 0xBBAAFFFF },
            { "Gold Collected:",std::to_string(hero->gold > 0 ? hero->gold : Dungeon::gold), 0xFFD700FF },
            { "Dungeon Depth:", std::to_string(Dungeon::depth), 0x55AAFFFF },
            { "Attack Skill:",  std::to_string(const_cast<Hero*>(hero)->attackSkill(nullptr)), 0xCACFC2FF },
            { "Defense Skill:", std::to_string(const_cast<Hero*>(hero)->defenseSkill(nullptr)), 0xCACFC2FF }
        };

        float leftX = m_x + 22.0f;
        float rightX = m_x + m_w - 22.0f;

        for (const auto& row : stats) {
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, row.label, leftX, curY, zText, 0xCACFC2FF, 0x000000FF);

            float vw = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, row.value);
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, row.value, rightX - vw, curY, zText, row.valColor, 0x000000FF);

            curY += 13.0f;
        }
    } else {
        // Buffs & Status Title
        float titleW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "BUFFS & CONDITIONS");
        float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "BUFFS & CONDITIONS", titleX, curY, zText, 0xFFFF44FF, 0x000000FF);
        curY += 14.0f;

        // Horizontal divider
        C2D_DrawRectSolid(m_x + 14.0f, curY, zWin + 0.03f, m_w - 28.0f, 1.0f, C2D_Color32(0x35, 0x30, 0x2A, 0xFF));
        C2D_DrawRectSolid(m_x + 14.0f, curY + 1.0f, zWin + 0.03f, m_w - 28.0f, 1.0f, C2D_Color32(0x5A, 0x55, 0x4D, 0xFF));
        curY += 8.0f;

        int buffCount = 0;
        int potW = (m_texLargeBuffs && m_texLargeBuffs->isLoaded()) ? m_texLargeBuffs->potWidth() : 256;
        int potH = (m_texLargeBuffs && m_texLargeBuffs->isLoaded()) ? m_texLargeBuffs->potHeight() : 32;
        C3D_Tex* tex = (m_texLargeBuffs && m_texLargeBuffs->isLoaded()) ? m_texLargeBuffs->getTex() : nullptr;

        for (const auto& b : hero->buffs) {
            if (!b) continue;
            std::string bName = b->name();
            std::string bDesc = "";
            uint32_t col = 0xCACFC2FF;
            int iconIndex = -1;

            switch (b->type()) {
                case BuffType::Regeneration:
                    bDesc = "Restores health over time.";
                    col = 0x56BB77FF;
                    iconIndex = 19;
                    break;
                case BuffType::Hunger:
                    bDesc = hero->isStarving() ? "Starving! Taking damage." : (hero->isHungry() ? "Hungry. Need food soon." : "Energized and fed.");
                    col = hero->isStarving() ? 0xEE4444FF : (hero->isHungry() ? 0xFF9933FF : 0x56BB77FF);
                    iconIndex = hero->isStarving() ? 6 : 5;
                    break;
                case BuffType::Fury:
                    bDesc = "Berserker rage: Damage increased!";
                    col = 0xFF3333FF;
                    iconIndex = 18;
                    break;
                case BuffType::Combo: {
                    Combo* cmb = b->asCombo();
                    bDesc = "Gladiator: " + std::to_string(cmb ? cmb->count : 0) + " hits landed in succession.";
                    col = 0xFF7733FF;
                    iconIndex = 17;
                    break;
                }
                case BuffType::Barkskin:
                    bDesc = "Hardened bark absorbs incoming damage.";
                    col = 0x88BB44FF;
                    iconIndex = 24;
                    break;
                case BuffType::Shadows:
                    bDesc = "Stealth concealment reduces detection.";
                    col = 0xAA88DDFF;
                    iconIndex = 13;
                    break;
                case BuffType::SnipersMark:
                    bDesc = "Marked: Next missile ignores armor.";
                    col = 0x33CCDDFF;
                    iconIndex = 27;
                    break;
                case BuffType::Invisibility:
                    bDesc = "Completely invisible to enemies.";
                    col = 0x5599DDFF;
                    iconIndex = 12;
                    break;
                case BuffType::Burning:
                    bDesc = "Engulfed in fire! Taking damage.";
                    col = 0xFF5511FF;
                    iconIndex = 2;
                    break;
                case BuffType::Poison:
                    bDesc = "Deadly venom coursing in veins.";
                    col = 0x33AA44FF;
                    iconIndex = 3;
                    break;
                case BuffType::Paralysis:
                    bDesc = "Paralyzed and unable to act.";
                    col = 0xEE3333FF;
                    iconIndex = 4;
                    break;
                case BuffType::Bleeding:
                    bDesc = "Bleeding out from wounds.";
                    col = 0xDD2222FF;
                    iconIndex = 26;
                    break;
                case BuffType::Cripple:
                    bDesc = "Crippled: Movement speed halved.";
                    col = 0xCC7733FF;
                    iconIndex = 23;
                    break;
                case BuffType::Roots:
                    bDesc = "Rooted in place: Cannot move.";
                    col = 0x997744FF;
                    iconIndex = 11;
                    break;
                case BuffType::Frost:
                    bDesc = "Frozen solid by intense cold.";
                    col = 0x66CCFFFF;
                    iconIndex = 15;
                    break;
                case BuffType::Levitation:
                    bDesc = "Floating gracefully above ground.";
                    col = 0x88EEFFFF;
                    iconIndex = 1;
                    break;
                case BuffType::Light:
                    bDesc = "Illuminating surrounding darkness.";
                    col = 0xFFEE55FF;
                    iconIndex = 22;
                    break;
                default:
                    bDesc = "Active effect.";
                    break;
            }

            float slotX = m_x + 18.0f;

            // Draw 16x16 icon if available
            if (tex && iconIndex >= 0) {
                int colIdx = iconIndex % 16;
                int rowIdx = iconIndex / 16;
                Tex3DS_SubTexture sub;
                sub.width = 16;
                sub.height = 16;
                sub.left = static_cast<float>(colIdx * 16) / static_cast<float>(potW);
                sub.right = static_cast<float>((colIdx + 1) * 16) / static_cast<float>(potW);
                sub.top = 1.0f - (static_cast<float>(rowIdx * 16) / static_cast<float>(potH));
                sub.bottom = 1.0f - (static_cast<float>((rowIdx + 1) * 16) / static_cast<float>(potH));

                C2D_Image img = { tex, &sub };
                C2D_DrawParams p;
                p.pos.x = slotX;
                p.pos.y = curY;
                p.pos.w = 16.0f;
                p.pos.h = 16.0f;
                p.center.x = p.center.y = 0.0f;
                p.depth = zWin + 0.03f;
                p.angle = 0.0f;
                C2D_DrawImage(img, &p, nullptr);

                slotX += 22.0f;
            }

            // Buff Title & short description
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, bName, slotX, curY, zText, col, 0x000000FF);
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, bDesc, slotX, curY + 9.0f, zText, 0xCACFC2FF, 0x000000FF);

            curY += 21.0f;
            buffCount++;
            if (curY > m_y + m_h - 40.0f) break; // Don't overflow close button
        }

        if (buffCount == 0) {
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "No active status effects.", m_x + 24.0f, curY + 16.0f, zText, 0x889988FF, 0x000000FF);
        }
    }

    // Close Button [B] using authentic Chrome::BUTTON
    float btnW = 90.0f;
    float btnH = 20.0f;
    float btnX = std::round(m_x + (m_w - btnW) * 0.5f);
    float btnY = m_y + m_h - 26.0f;

    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);
    btnPatch.draw(btnX, btnY, btnW, btnH, zWin + 0.03f, 0xFFFFFFFF, 1.0f);

    float btnTxtW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Close [B]");
    float btnTxtX = std::round(btnX + (btnW - btnTxtW) * 0.5f);
    float btnTxtY = std::round(btnY + (btnH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Close [B]", btnTxtX, btnTxtY, zWin + 0.05f, 0xFFFFFFFF, 0x000000FF);
}

} // namespace pd::windows
