#pragma once

#include <citro2d.h>
#include <string>
#include <functional>

namespace pd {

class GLog {
public:
    using LogCallback = std::function<void(const std::string&, u32)>;

    // Palette constants matching original Pixel Dungeon
    static constexpr u32 COLOR_WHITE     = C2D_Color32(0xe4, 0xa6, 0x72, 0xFF); // Default dialogue / event
    static constexpr u32 COLOR_POSITIVE  = C2D_Color32(0x38, 0x9e, 0x38, 0xFF); // Green (success, healing)
    static constexpr u32 COLOR_NEGATIVE  = C2D_Color32(0xd6, 0x30, 0x30, 0xFF); // Red (damage, death, danger)
    static constexpr u32 COLOR_WARNING   = C2D_Color32(0xff, 0x88, 0x00, 0xFF); // Orange (hunger, warning)
    static constexpr u32 COLOR_HIGHLIGHT = C2D_Color32(0xff, 0xff, 0x44, 0xFF); // Yellow (discovery, level up)

    static void setLogger(LogCallback logger);

    // Standard information / default text
    static void i(const std::string& text);

    // Positive / success (green)
    static void p(const std::string& text);

    // Negative / danger / damage (red)
    static void n(const std::string& text);

    // Warning / hunger / caution (orange)
    static void w(const std::string& text);

    // Highlight / discovery / level up (yellow)
    static void h(const std::string& text);

private:
    static LogCallback s_logger;
};

} // namespace pd

