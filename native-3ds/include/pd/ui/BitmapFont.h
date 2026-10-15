#pragma once

#include <citro2d.h>
#include <string>
#include <memory>
#include <cstdint>

namespace pd::ui {

class BitmapFont {
public:
    enum class FontType {
        Font1x,
        Font2x,
        Font1xInk,
        Font2xInk
    };

    static void init();

    // Draws a single line of text with the exact Pixel Dungeon bitmap font.
    // Supports tracking, spaces, and color tinting.
    static float draw(FontType font, const std::string& text, float x, float y, float z = 0.6f, u32 color = 0xFFFFFFFF, float scale = 1.0f);

    // Draws text with a 1-pixel drop shadow.
    static float drawWithShadow(FontType font, const std::string& text, float x, float y, float z = 0.6f, u32 color = 0xFFFFFFFF, u32 shadowColor = 0x000000FF, float scale = 1.0f);

    // Draws multiline text wrapped to maxWidth.
    static float drawMultiline(FontType font, const std::string& text, float x, float y, float maxWidth, float z = 0.6f, u32 color = 0xFFFFFFFF, float scale = 1.0f);

    // Measures the width of a single line of text.
    static float width(FontType font, const std::string& text, float scale = 1.0f);

    // Height of a single line of text.
    static float height(FontType font, float scale = 1.0f);

    // Line spacing (line height) for multiline text.
    static float lineHeight(FontType font, float scale = 1.0f);
};

} // namespace pd::ui

