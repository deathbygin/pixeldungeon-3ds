#include "pd/windows/WndDebugMenu.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/Debug.h"
#include "pd/Dungeon.h"
#include "pd/Hero.h"
#include "pd/GLog.h"
#include "pd/Buff.h"
#include <algorithm>
#include <cmath>

namespace pd::windows {

using namespace pd::ui;

WndDebugMenu::WndDebugMenu() {
    m_w = 210.0f;
    m_h = 175.0f;
    m_x = (320.0f - m_w) * 0.5f;
    m_y = (240.0f - m_h) * 0.5f;

    float btnW = m_w - 24.0f;
    float btnH = 22.0f;
    float btnX = m_x + 12.0f;
    float startY = m_y + 32.0f;
    float spacing = 26.0f;

    m_buttons = {
        { "God Mode", DebugMenuAction::TOGGLE_GOD_MODE, btnX, startY, btnW, btnH },
        { "One-Hit Kill", DebugMenuAction::TOGGLE_ONE_HIT_KILL, btnX, startY + spacing, btnW, btnH },
        { "Spawn Items...", DebugMenuAction::OPEN_ITEM_SPAWNER, btnX, startY + spacing * 2, btnW, btnH },
        { "Heal & Cleanse", DebugMenuAction::HEAL_HERO, btnX, startY + spacing * 3, btnW, btnH },
        { "Close", DebugMenuAction::CLOSE, btnX, startY + spacing * 4, btnW, btnH }
    };
}

void WndDebugMenu::show() {
    m_open = true;
    m_selected = 0;
}

void WndDebugMenu::hide() {
    m_open = false;
}

DebugMenuAction WndDebugMenu::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return DebugMenuAction::NONE;

    // D-Pad / Circle pad navigation
    if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
        if (m_selected > 0) m_selected--;
        else m_selected = static_cast<int>(m_buttons.size()) - 1;
        Sample::play(Assets::SND_CLICK);
        return DebugMenuAction::NONE;
    }
    if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        if (m_selected + 1 < static_cast<int>(m_buttons.size())) m_selected++;
        else m_selected = 0;
        Sample::play(Assets::SND_CLICK);
        return DebugMenuAction::NONE;
    }

    // Hardware B button: close
    if (kDown & (KEY_B | KEY_START)) {
        Sample::play(Assets::SND_CLICK);
        hide();
        return DebugMenuAction::CLOSE;
    }

    // Hardware A button: execute selected action
    if (kDown & KEY_A) {
        if (m_selected >= 0 && m_selected < static_cast<int>(m_buttons.size())) {
            DebugMenuAction act = m_buttons[m_selected].action;
            if (act == DebugMenuAction::TOGGLE_GOD_MODE) {
                Debug::godMode = !Debug::godMode;
                Sample::play(Assets::SND_CLICK);
                GLog::p(std::string("[DEBUG] God Mode: ") + (Debug::godMode ? "ON" : "OFF"));
                return DebugMenuAction::TOGGLE_GOD_MODE;
            } else if (act == DebugMenuAction::TOGGLE_ONE_HIT_KILL) {
                Debug::oneHitKill = !Debug::oneHitKill;
                Sample::play(Assets::SND_CLICK);
                GLog::p(std::string("[DEBUG] One-Hit Kill: ") + (Debug::oneHitKill ? "ON" : "OFF"));
                return DebugMenuAction::TOGGLE_ONE_HIT_KILL;
            } else if (act == DebugMenuAction::HEAL_HERO) {
                if (Dungeon::hero) {
                    Dungeon::hero->HP = Dungeon::hero->HT;
                    Dungeon::hero->removeBuff(BuffType::Poison);
                    Dungeon::hero->removeBuff(BuffType::Burning);
                    Dungeon::hero->removeBuff(BuffType::Bleeding);
                    Dungeon::hero->removeBuff(BuffType::Cripple);
                    Dungeon::hero->removeBuff(BuffType::Weakness);
                    Dungeon::hero->removeBuff(BuffType::Ooze);
                    Dungeon::hero->removeBuff(BuffType::Paralysis);
                    Hunger* h = Dungeon::hero->getBuff<Hunger>();
                    if (h) h->satisfy(Hunger::STARVING);
                }
                Sample::play(Assets::SND_DRINK);
                GLog::p("[DEBUG] Hero restored to full vitality!");
                return DebugMenuAction::HEAL_HERO;
            } else if (act == DebugMenuAction::OPEN_ITEM_SPAWNER) {
                Sample::play(Assets::SND_CLICK);
                hide();
                return DebugMenuAction::OPEN_ITEM_SPAWNER;
            } else if (act == DebugMenuAction::CLOSE) {
                Sample::play(Assets::SND_CLICK);
                hide();
                return DebugMenuAction::CLOSE;
            }
        }
    }

    // Touch handling
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const auto& b = m_buttons[i];
        noosa::RectF r(b.x, b.y, b.w, b.h);
        if (touch.clicked(r)) {
            m_selected = static_cast<int>(i);
            if (b.action == DebugMenuAction::TOGGLE_GOD_MODE) {
                Debug::godMode = !Debug::godMode;
                Sample::play(Assets::SND_CLICK);
                GLog::p(std::string("[DEBUG] God Mode: ") + (Debug::godMode ? "ON" : "OFF"));
                return DebugMenuAction::TOGGLE_GOD_MODE;
            } else if (b.action == DebugMenuAction::TOGGLE_ONE_HIT_KILL) {
                Debug::oneHitKill = !Debug::oneHitKill;
                Sample::play(Assets::SND_CLICK);
                GLog::p(std::string("[DEBUG] One-Hit Kill: ") + (Debug::oneHitKill ? "ON" : "OFF"));
                return DebugMenuAction::TOGGLE_ONE_HIT_KILL;
            } else if (b.action == DebugMenuAction::HEAL_HERO) {
                if (Dungeon::hero) {
                    Dungeon::hero->HP = Dungeon::hero->HT;
                    Dungeon::hero->removeBuff(BuffType::Poison);
                    Dungeon::hero->removeBuff(BuffType::Burning);
                    Dungeon::hero->removeBuff(BuffType::Bleeding);
                    Dungeon::hero->removeBuff(BuffType::Cripple);
                    Dungeon::hero->removeBuff(BuffType::Weakness);
                    Dungeon::hero->removeBuff(BuffType::Ooze);
                    Dungeon::hero->removeBuff(BuffType::Paralysis);
                    Hunger* h = Dungeon::hero->getBuff<Hunger>();
                    if (h) h->satisfy(Hunger::STARVING);
                }
                Sample::play(Assets::SND_DRINK);
                GLog::p("[DEBUG] Hero restored to full vitality!");
                return DebugMenuAction::HEAL_HERO;
            } else if (b.action == DebugMenuAction::OPEN_ITEM_SPAWNER) {
                Sample::play(Assets::SND_CLICK);
                hide();
                return DebugMenuAction::OPEN_ITEM_SPAWNER;
            } else if (b.action == DebugMenuAction::CLOSE) {
                Sample::play(Assets::SND_CLICK);
                hide();
                return DebugMenuAction::CLOSE;
            }
        }
    }

    // Tap outside to close
    noosa::RectF winRect(m_x, m_y, m_w, m_h);
    if (touch.justReleased() && !winRect.contains(static_cast<float>(touch.x()), static_cast<float>(touch.y()))) {
        Sample::play(Assets::SND_CLICK);
        hide();
        return DebugMenuAction::CLOSE;
    }

    return DebugMenuAction::NONE;
}

void WndDebugMenu::draw(C2D_TextBuf textBuf, float baseDepth) {
    (void)textBuf;
    if (!m_open) return;

    // 1. Dim background
    C2D_DrawRectSolid(0.0f, 0.0f, baseDepth, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xBB));

    // 2. 9-slice stone window chrome
    Chrome::get(Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, baseDepth + 0.02f);

    // 3. Title text in warm orange / gold
    std::string title = "DEVELOPER MENU";
    float titleW = BitmapFont::width(BitmapFont::FontType::Font1x, title);
    float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, title, titleX, m_y + 12.0f, baseDepth + 0.06f, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);

    // 4. Buttons
    const auto& btnPatch = Chrome::get(Chrome::Type::BUTTON);
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const auto& b = m_buttons[i];
        bool isSel = (m_selected == static_cast<int>(i));

        float brightness = isSel ? 1.35f : 1.0f;
        btnPatch.draw(b.x, b.y, b.w, b.h, baseDepth + 0.04f, 0xFFFFFFFF, brightness);

        std::string displayLabel = b.label;
        u32 textColor = isSel ? COLOR_TEXT_ACCENT : COLOR_TEXT_PRIMARY;

        if (b.action == DebugMenuAction::TOGGLE_GOD_MODE) {
            displayLabel = std::string("God Mode: ") + (Debug::godMode ? "[ ON ]" : "[ OFF ]");
            if (Debug::godMode) {
                textColor = COLOR_TEXT_POSITIVE;
            }
        } else if (b.action == DebugMenuAction::TOGGLE_ONE_HIT_KILL) {
            displayLabel = std::string("One-Hit Kill: ") + (Debug::oneHitKill ? "[ ON ]" : "[ OFF ]");
            if (Debug::oneHitKill) {
                textColor = COLOR_TEXT_POSITIVE;
            }
        } else if (b.action == DebugMenuAction::CLOSE) {
            textColor = isSel ? COLOR_TEXT_ACCENT : COLOR_TEXT_PRIMARY;
        }

        float textW = BitmapFont::width(BitmapFont::FontType::Font1x, displayLabel);
        float textX = std::round(b.x + (b.w - textW) * 0.5f);
        float textY = std::round(b.y + (b.h - 8.0f) * 0.5f);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, displayLabel, textX, textY, baseDepth + 0.06f, textColor, COLOR_TEXT_SHADOW);
    }
}

} // namespace pd::windows

