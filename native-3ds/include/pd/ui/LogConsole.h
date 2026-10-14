#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <deque>

namespace pd::ui {

class LogConsole {
public:
    float x = 8.0f;
    float y = 168.0f;
    float width = 304.0f;
    float height = 48.0f;

    LogConsole() = default;

    void addMessage(std::string msg, u32 color = C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));
    void clear();

    void draw(C2D_TextBuf textBuf) const;

private:
    struct Entry {
        std::string text;
        u32 color;
    };

    static constexpr size_t MAX_ENTRIES = 30;
    std::deque<Entry> m_history;
};

} // namespace pd::ui

