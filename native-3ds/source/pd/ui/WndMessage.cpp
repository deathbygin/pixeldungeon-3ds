#include "pd/ui/WndMessage.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "noosa/Audio.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace pd::ui {

WndMessage::WndMessage(std::string text, DismissCallback onDismiss) {
    show(std::move(text), std::move(onDismiss));
}

void WndMessage::show(std::string text, DismissCallback onDismiss) {
    m_text = std::move(text);
    m_callback = std::move(onDismiss);
    m_open = true;

    // Fixed comfortable modal width on 320x240 screen
    m_w = 230.0f;
    m_x = std::round((320.0f - m_w) * 0.5f);

    float maxLineW = m_w - 28.0f; // 14px padding each side

    // Word wrap text into lines with newline support
    m_lines.clear();
    std::stringstream textStream(m_text);
    std::string paragraph;
    while (std::getline(textStream, paragraph)) {
        if (paragraph.empty()) {
            m_lines.push_back("");
            continue;
        }

        std::stringstream ss(paragraph);
        std::string word;
        std::string currentLine;
        while (ss >> word) {
            float wordW = BitmapFont::width(BitmapFont::FontType::Font1x, word);
            float spaceW = 3.0f;
            float lineW = currentLine.empty() ? 0.0f : (BitmapFont::width(BitmapFont::FontType::Font1x, currentLine) + spaceW);

            if (!currentLine.empty() && (lineW + wordW) > maxLineW) {
                m_lines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            m_lines.push_back(currentLine);
        }
    }

    if (m_lines.empty()) {
        m_lines.push_back("");
    }

    float lineH = 12.0f;
    float topPadding = 16.0f;
    float bottomPadding = 16.0f;
    m_h = topPadding + m_lines.size() * lineH + bottomPadding;
    // Clamp min and max height
    m_h = std::clamp(m_h, 60.0f, 210.0f);
    m_y = std::round((240.0f - m_h) * 0.5f);
}

void WndMessage::hide() {
    m_open = false;
}

bool WndMessage::update(const TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Dismiss on hardware button press (A, B, X, Y, START, SELECT)
    if (kDown & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_START | KEY_SELECT)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        hide();
        if (m_callback) m_callback();
        return true;
    }

    // Dismiss on stylus tap anywhere
    if (touch.justReleased()) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        hide();
        if (m_callback) m_callback();
        return true;
    }

    return true; // Modal consumes all input while open
}

void WndMessage::draw(C2D_TextBuf textBuf, float baseDepth) {
    (void)textBuf;
    if (!m_open) return;

    // 1. Dark background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, baseDepth, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xAA));

    // 2. 9-slice stone window chrome from chrome.png
    Chrome::get(Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, baseDepth + 0.02f);

    // 3. Narrative / message text lines in readable light grey with 1px black drop-shadow
    float curY = m_y + 16.0f;
    float lineH = 12.0f;
    for (const auto& line : m_lines) {
        if (!line.empty()) {
            float lineW = BitmapFont::width(BitmapFont::FontType::Font1x, line);
            float lineX = std::round(m_x + (m_w - lineW) * 0.5f);
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, line, lineX, curY, baseDepth + 0.06f, 0xCACFC2FF, 0x000000FF);
        }
        curY += lineH;
    }
}

} // namespace pd::ui

