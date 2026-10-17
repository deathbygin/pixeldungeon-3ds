#include "pd/windows/WndGame.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"

namespace pd::windows {

WndGame::WndGame() {
    m_w = 160.0f;
    float btnW = m_w - 24.0f;
    float btnH = 22.0f;
    float btnSpacing = 6.0f;

    std::vector<std::pair<std::string, GameAction>> items = {
        { "Return to Game",     GameAction::RESUME },
        { "Settings",           GameAction::SETTINGS },
        { "Main Menu",          GameAction::MAIN_MENU },
        { "Exit Pixel Dungeon", GameAction::EXIT_GAME }
    };

    m_h = 16.0f + items.size() * btnH + (items.size() - 1) * btnSpacing + 16.0f;
    m_x = (320.0f - m_w) * 0.5f;
    m_y = (240.0f - m_h) * 0.5f;

    float curY = m_y + 16.0f;
    for (const auto& item : items) {
        m_buttons.push_back({
            item.first,
            item.second,
            m_x + 12.0f,
            curY,
            btnW,
            btnH
        });
        curY += btnH + btnSpacing;
    }
}

void WndGame::show() {
    m_open = true;
    m_selected = 0;
}

GameAction WndGame::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return GameAction::NONE;

    // B or START acts as "Return to Game"
    if (kDown & (KEY_B | KEY_START)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return GameAction::RESUME;
    }

    // Hardware D-pad / CPAD navigation
    if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
        if (m_selected > 0) m_selected--;
        else m_selected = static_cast<int>(m_buttons.size()) - 1;
        return GameAction::NONE;
    }
    if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        if (m_selected + 1 < static_cast<int>(m_buttons.size())) m_selected++;
        else m_selected = 0;
        return GameAction::NONE;
    }

    // Hardware confirmation A
    if (kDown & KEY_A) {
        if (m_selected >= 0 && m_selected < static_cast<int>(m_buttons.size())) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            GameAction act = m_buttons[m_selected].action;
            if (act == GameAction::RESUME) {
                hide();
            }
            return act;
        }
    }

    // Stylus Touch handling
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const auto& b = m_buttons[i];
        noosa::RectF r(b.x, b.y, b.w, b.h);
        if (touch.hitTest(r)) {
            m_selected = static_cast<int>(i);
        }
        if (touch.clicked(r)) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            GameAction act = b.action;
            if (act == GameAction::RESUME) {
                hide();
            }
            return act;
        }
    }

    return GameAction::NONE;
}

void WndGame::draw(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open) return;

    // 1. Darken background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, 0.80f, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xAA));

    // 2. 9-slice stone window chrome from chrome.png
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, 0.82f);

    // 3. Option buttons styled with RedButton chrome
    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const auto& b = m_buttons[i];
        bool isSel = (m_selected == static_cast<int>(i));

        float brightness = isSel ? 1.35f : 1.0f;
        btnPatch.draw(b.x, b.y, b.w, b.h, 0.84f, 0xFFFFFFFF, brightness);

        float textW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, b.label);
        float textX = std::round(b.x + (b.w - textW) * 0.5f);
        float textY = std::round(b.y + (b.h - 8.0f) * 0.5f);

        u32 textColor = isSel ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY;
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, b.label, textX, textY, 0.86f, textColor, pd::ui::COLOR_TEXT_SHADOW);
    }
}

} // namespace pd::windows

