#include "pd/windows/WndBadge.h"
#include "pd/BadgeBanner.h"
#include "pd/Assets.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Sample.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace pd::windows {

WndBadge::WndBadge() {
}

void WndBadge::show(Badges::Badge badge, bool unlocked) {
    m_badge = badge;
    m_unlocked = unlocked;
    m_open = true;

    const auto& info = Badges::getInfo(m_badge);
    wrapText(info.description, m_w - 32.0f);
}

void WndBadge::wrapText(const std::string& text, float maxW) {
    m_lines.clear();
    std::stringstream textStream(text);
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
            float wordW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, word);
            float spaceW = 4.0f;
            float lineW = currentLine.empty() ? 0.0f : (pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, currentLine) + spaceW);
            if (!currentLine.empty() && (lineW + wordW) > maxW) {
                m_lines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) {
                    currentLine += " ";
                }
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            m_lines.push_back(currentLine);
        }
    }
}

bool WndBadge::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Close via B, A, or START
    if (kDown & (KEY_B | KEY_A | KEY_START)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return true;
    }

    // Close button bounds
    float btnW = 80.0f;
    float btnH = 20.0f;
    float btnX = std::round(m_x + (m_w - btnW) * 0.5f);
    float btnY = m_y + m_h - 26.0f;
    noosa::RectF rBtn(btnX, btnY, btnW, btnH);

    if (touch.clicked(rBtn)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return true;
    }

    // Tapping outside window also closes
    noosa::RectF rWin(m_x, m_y, m_w, m_h);
    if (touch.justReleased() && !rWin.contains(static_cast<float>(touch.x()), static_cast<float>(touch.y()))) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return true;
    }

    return true;
}

void WndBadge::draw(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open || m_badge >= Badges::Badge::COUNT) return;

    float zDim = 0.85f;
    float zWin = 0.87f;
    float zContent = 0.90f;
    float zText = 0.92f;

    // 1. Darken background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, zDim, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xB0));

    // 2. 9-slice stone window chrome
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, zWin);

    const auto& info = Badges::getInfo(m_badge);

    // 3. Medal Icon (32x32)
    float iconX = std::round(m_x + (m_w - 32.0f) * 0.5f);
    float iconY = m_y + 12.0f;
    BadgeBanner::drawBadge(info.image, iconX, iconY, 2.0f, 1.0f, !m_unlocked, zContent);

    // 4. Badge Title
    float titleW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, info.title);
    float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
    float titleY = iconY + 36.0f;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, info.title, titleX, titleY, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    // 5. Unlock Status
    std::string statusStr = m_unlocked ? "[UNLOCKED]" : "[LOCKED]";
    u32 statusCol = m_unlocked ? pd::ui::COLOR_TEXT_POSITIVE : pd::ui::COLOR_TEXT_MUTED;
    float statW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, statusStr);
    float statX = std::round(m_x + (m_w - statW) * 0.5f);
    float statY = titleY + 12.0f;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, statusStr, statX, statY, zText, statusCol, pd::ui::COLOR_TEXT_SHADOW);

    // 6. Divider line
    float divY = statY + 13.0f;
    C2D_DrawRectSolid(m_x + 16.0f, divY, zContent, m_w - 32.0f, 1.0f, C2D_Color32(0x4A, 0x4D, 0x44, 0xFF));

    // 7. Multiline Description
    float lineY = divY + 6.0f;
    for (const auto& line : m_lines) {
        float lineW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, line);
        float lineX = std::round(m_x + (m_w - lineW) * 0.5f);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, line, lineX, lineY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
        lineY += 10.0f;
    }

    // 8. Close Button
    float btnW = 80.0f;
    float btnH = 20.0f;
    float btnX = std::round(m_x + (m_w - btnW) * 0.5f);
    float btnY = m_y + m_h - 26.0f;
    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);
    btnPatch.draw(btnX, btnY, btnW, btnH, zContent, 0xFFFFFFFF, 1.0f);

    float closeW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Close");
    float closeX = std::round(btnX + (btnW - closeW) * 0.5f);
    float closeY = std::round(btnY + (btnH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Close", closeX, closeY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
}

} // namespace pd::windows

