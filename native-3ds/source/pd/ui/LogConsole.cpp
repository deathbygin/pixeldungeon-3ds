#include "pd/ui/LogConsole.h"
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
    // Background container
    C2D_DrawRectSolid(x, y, 0.3f, width, height, C2D_Color32(0x1a, 0x16, 0x24, 0xFF));
    C2D_DrawRectSolid(x, y, 0.35f, width, 1.0f, C2D_Color32(0x4a, 0x3d, 0x30, 0xFF));
    C2D_DrawRectSolid(x, y + height - 1.0f, 0.35f, width, 1.0f, C2D_Color32(0x4a, 0x3d, 0x30, 0xFF));
    C2D_DrawRectSolid(x, y, 0.35f, 1.0f, height, C2D_Color32(0x4a, 0x3d, 0x30, 0xFF));
    C2D_DrawRectSolid(x + width - 1.0f, y, 0.35f, 1.0f, height, C2D_Color32(0x4a, 0x3d, 0x30, 0xFF));

    // Render up to 3 most recent messages
    size_t count = m_history.size();
    size_t startIdx = (count > 3) ? (count - 3) : 0;

    float lineY = y + 4.0f;
    C2D_Text textObj;

    for (size_t i = startIdx; i < count; ++i) {
        const auto& entry = m_history[i];
        C2D_TextParse(&textObj, textBuf, entry.text.c_str());
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, x + 6.0f, lineY, 0.45f, 0.38f, 0.38f, entry.color);
        lineY += 13.0f;
    }
}

} // namespace pd::ui

