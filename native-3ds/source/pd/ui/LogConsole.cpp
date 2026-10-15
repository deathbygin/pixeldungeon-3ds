#include "pd/ui/LogConsole.h"
#include "pd/ui/BitmapFont.h"
#include <algorithm>

namespace pd::ui {

void LogConsole::addMessage(std::string msg, u32 color) {
    if (msg.empty()) return;

    if (m_history.size() >= MAX_ENTRIES) {
        m_history.pop_front();
    }
    m_history.push_back({ std::move(msg), color });
}

void LogConsole::clear() {
    m_history.clear();
}

void LogConsole::draw(C2D_TextBuf textBuf) const {
    (void)textBuf;
    // Sleek recessed dark stone panel with subtle top/bottom borders
    C2D_DrawRectSolid(x, y, 0.30f, width, height, C2D_Color32(0x12, 0x10, 0x16, 0xFF));
    C2D_DrawRectSolid(x + 1.0f, y + 1.0f, 0.31f, width - 2.0f, height - 2.0f, C2D_Color32(0x19, 0x16, 0x22, 0xFF));
    
    // Top highlight and bottom shadow borders
    C2D_DrawRectSolid(x, y, 0.32f, width, 1.0f, C2D_Color32(0x3E, 0x36, 0x48, 0xFF));
    C2D_DrawRectSolid(x, y + height - 1.0f, 0.32f, width, 1.0f, C2D_Color32(0x0A, 0x08, 0x0E, 0xFF));
    C2D_DrawRectSolid(x, y, 0.32f, 1.0f, height, C2D_Color32(0x3E, 0x36, 0x48, 0xFF));
    C2D_DrawRectSolid(x + width - 1.0f, y, 0.32f, 1.0f, height, C2D_Color32(0x0A, 0x08, 0x0E, 0xFF));

    // Render 2 most recent messages with clean baseline and drop shadow
    size_t count = m_history.size();
    size_t startIdx = (count > 2) ? (count - 2) : 0;

    float lineY = y + 7.0f;

    for (size_t i = startIdx; i < count; ++i) {
        const auto& entry = m_history[i];
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, entry.text, x + 8.0f, lineY, 0.35f, entry.color, 0x000000FF);
        lineY += 14.0f;
    }
}

} // namespace pd::ui
