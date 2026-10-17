#pragma once

#include <citro2d.h>

namespace pd::ui {

// Semantic text color palette constants using Citro2D C2D_Color32 byte order (RGBA).
// Completely eliminates obsolete purple/pink hues across all 3DS screens.
namespace Palette {
    static constexpr u32 COLOR_TEXT_PRIMARY   = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF); // #FFFFFF Pure High-Contrast White
    static constexpr u32 COLOR_TEXT_ACCENT    = C2D_Color32(0xFF, 0xAA, 0x22, 0xFF); // #FFAA22 Warm Orange / Gold Highlight
    static constexpr u32 COLOR_TEXT_MUTED     = C2D_Color32(0x88, 0x88, 0x88, 0xFF); // #888888 Neutral Muted Gray
    static constexpr u32 COLOR_TEXT_WARNING   = C2D_Color32(0xFF, 0x44, 0x44, 0xFF); // #FF4444 Danger / Warning / Cursed Red
    static constexpr u32 COLOR_TEXT_POSITIVE  = C2D_Color32(0x56, 0xBB, 0x77, 0xFF); // #56BB77 Success / Buff / Upgrade Green
    static constexpr u32 COLOR_TEXT_SHADOW    = C2D_Color32(0x00, 0x00, 0x00, 0xFF); // #000000 Solid Black Drop Shadow
}

// Global aliases within pd::ui for direct convenience
using Palette::COLOR_TEXT_PRIMARY;
using Palette::COLOR_TEXT_ACCENT;
using Palette::COLOR_TEXT_MUTED;
using Palette::COLOR_TEXT_WARNING;
using Palette::COLOR_TEXT_POSITIVE;
using Palette::COLOR_TEXT_SHADOW;

} // namespace pd::ui
