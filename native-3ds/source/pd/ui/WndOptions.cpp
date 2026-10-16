#include "pd/ui/WndOptions.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace pd::ui {

WndOptions::WndOptions(std::string title, std::string message,
                       std::vector<std::string> options,
                       OptionCallback onSelect) {
    show(std::move(title), std::move(message), std::move(options), std::move(onSelect));
}

void WndOptions::show(std::string title, std::string message,
                      std::vector<std::string> options,
                      OptionCallback onSelect) {
    m_title = std::move(title);
    m_message = std::move(message);
    m_options = std::move(options);
    m_callback = std::move(onSelect);
    m_selected = 0;
    m_open = true;

    // Proportional modal width matching Pixel Dungeon UI window styling
    m_w = 190.0f;
    m_x = (320.0f - m_w) * 0.5f;

    // Word-wrap message into lines using authentic BitmapFont metrics
    m_messageLines.clear();
    if (!m_message.empty()) {
        float maxMsgW = m_w - 24.0f;
        std::stringstream ss(m_message);
        std::string word;
        std::string currentLine;
        while (ss >> word) {
            float wordW = BitmapFont::width(BitmapFont::FontType::Font1x, word);
            float lineW = currentLine.empty() ? 0.0f : (BitmapFont::width(BitmapFont::FontType::Font1x, currentLine) + 3.0f);
            if (!currentLine.empty() && (lineW + wordW) > maxMsgW) {
                m_messageLines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            m_messageLines.push_back(currentLine);
        }
    }

    float btnH = 22.0f;
    float btnSpacing = 6.0f;
    float totalBtnH = m_options.empty() ? 0.0f : (m_options.size() * btnH + (m_options.size() - 1) * btnSpacing);

    float topPadding = 14.0f;
    float titleH = 10.0f;
    float titleToMsgGap = m_messageLines.empty() ? 0.0f : 8.0f;
    float msgH = m_messageLines.size() * 11.0f;
    float msgToBtnGap = m_options.empty() ? 0.0f : (m_messageLines.empty() ? 8.0f : 12.0f);
    float bottomPadding = 14.0f;

    m_h = topPadding + titleH + titleToMsgGap + msgH + msgToBtnGap + totalBtnH + bottomPadding;
    m_y = (240.0f - m_h) * 0.5f;

    m_buttonLayouts.clear();
    float curY = m_y + topPadding + titleH + titleToMsgGap + msgH + msgToBtnGap;
    float btnW = m_w - 24.0f;
    float btnX = m_x + 12.0f;

    for (size_t i = 0; i < m_options.size(); ++i) {
        m_buttonLayouts.push_back({ btnX, curY, btnW, btnH });
        curY += btnH + btnSpacing;
    }
}

void WndOptions::hide() {
    m_open = false;
}

bool WndOptions::update(const TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Hardware D-pad / Circle Pad up/down navigation
    if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
        if (m_selected > 0) m_selected--;
        else m_selected = static_cast<int>(m_buttonLayouts.size()) - 1;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        return true;
    }
    if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        if (m_selected + 1 < static_cast<int>(m_buttonLayouts.size())) m_selected++;
        else m_selected = 0;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        return true;
    }

    // Hardware confirmation A
    if (kDown & KEY_A) {
        int choice = m_selected;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        if (m_callback) m_callback(choice);
        return true;
    }

    // Hardware cancel B or START
    if (kDown & (KEY_B | KEY_START)) {
        int cancelIdx = (m_options.size() > 1) ? static_cast<int>(m_options.size() - 1) : 0;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        if (m_callback) m_callback(cancelIdx);
        return true;
    }

    // Tap outside window on dark overlay to cancel
    noosa::RectF windowRect(m_x, m_y, m_w, m_h);
    if (touch.justReleased() && !windowRect.contains(touch.pos().x, touch.pos().y)) {
        int cancelIdx = (m_options.size() > 1) ? static_cast<int>(m_options.size() - 1) : 0;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        if (m_callback) m_callback(cancelIdx);
        return true;
    }

    // Stylus Touch handling for option buttons
    for (size_t i = 0; i < m_buttonLayouts.size(); ++i) {
        const auto& b = m_buttonLayouts[i];
        noosa::RectF r(b.x, b.y, b.w, b.h);
        if (touch.hitTest(r)) {
            m_selected = static_cast<int>(i);
        }
        if (touch.clicked(r)) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            hide();
            if (m_callback) m_callback(static_cast<int>(i));
            return true;
        }
    }

    return true; // Absorb all input while modal is active
}

void WndOptions::draw(C2D_TextBuf textBuf, float baseDepth) {
    (void)textBuf;
    if (!m_open) return;

    // 1. Darken background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, baseDepth, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xAA));

    // 2. 9-slice stone window chrome from chrome.png
    Chrome::get(Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, baseDepth + 0.02f);

    // 3. Title text in golden yellow with 1px drop-shadow
    float titleW = BitmapFont::width(BitmapFont::FontType::Font1x, m_title);
    float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
    float titleY = m_y + 14.0f;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, m_title, titleX, titleY, baseDepth + 0.06f, 0xFFEBA4FF, 0x000000FF);

    // 4. Message text lines in readable light grey with 1px drop-shadow
    float curMsgY = titleY + 16.0f;
    for (const auto& line : m_messageLines) {
        float lineW = BitmapFont::width(BitmapFont::FontType::Font1x, line);
        float lineX = std::round(m_x + (m_w - lineW) * 0.5f);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, line, lineX, curMsgY, baseDepth + 0.06f, 0xCACFC2FF, 0x000000FF);
        curMsgY += 11.0f;
    }

    // 5. Option buttons (RedButton 9-slice from chrome.png with BitmapFont labels)
    const auto& btnPatch = Chrome::get(Chrome::Type::BUTTON);
    for (size_t i = 0; i < m_buttonLayouts.size(); ++i) {
        const auto& b = m_buttonLayouts[i];
        bool isSel = (m_selected == static_cast<int>(i));

        float brightness = isSel ? 1.35f : 1.0f;
        btnPatch.draw(b.x, b.y, b.w, b.h, baseDepth + 0.04f, 0xFFFFFFFF, brightness);

        float textW = BitmapFont::width(BitmapFont::FontType::Font1x, m_options[i]);
        float textX = std::round(b.x + (b.w - textW) * 0.5f);
        float textY = std::round(b.y + (b.h - 8.0f) * 0.5f);

        u32 textColor = isSel ? 0xFFEE88FF : 0xFFFFFFFF;
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, m_options[i], textX, textY, baseDepth + 0.06f, textColor, 0x000000FF);
    }
}

} // namespace pd::ui
