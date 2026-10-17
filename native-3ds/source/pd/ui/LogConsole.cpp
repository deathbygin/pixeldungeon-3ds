#include "pd/ui/LogConsole.h"
#include "pd/ui/BitmapFont.h"
#include "pd/ui/Chrome.h"
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
    float zLog = 0.30f;
    float zText = 0.35f;

    // 1. Raised 9-slice stone chrome container (matching Hero Stats & Minimap)
    Chrome::get(Chrome::Type::TOAST).draw(x, y, width, height, zLog);

    // 2. Render wrapped lines strictly inside inner container body
    float textX = x + 8.0f;
    float textMaxW = width - 16.0f; // 296px
    float lineH = 12.0f;

    struct FormattedLine {
        std::string text;
        u32 color;
    };
    std::vector<FormattedLine> linesToDraw;

    // Collect from most recent entries backwards until 3 display lines are filled
    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it) {
        auto wrapped = BitmapFont::splitMultiline(BitmapFont::FontType::Font1x, it->text, textMaxW);
        std::vector<FormattedLine> entryLines;
        entryLines.reserve(wrapped.size());
        for (auto& s : wrapped) {
            entryLines.push_back({ std::move(s), it->color });
        }

        // Prepend these lines so they remain in chronological order
        linesToDraw.insert(linesToDraw.begin(), entryLines.begin(), entryLines.end());
        if (linesToDraw.size() >= 3) {
            if (linesToDraw.size() > 3) {
                // Keep only the 3 most recent lines
                linesToDraw.erase(linesToDraw.begin(), linesToDraw.begin() + (linesToDraw.size() - 3));
            }
            break;
        }
    }

    float lineY = y + 7.0f;
    for (const auto& line : linesToDraw) {
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, line.text, textX, lineY, zText, line.color, 0x000000FF);
        lineY += lineH;
    }
}

} // namespace pd::ui
