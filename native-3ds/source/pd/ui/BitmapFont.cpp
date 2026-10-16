#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "noosa/Texture.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace pd::ui {

// Font1x glyphs: offset x, width
static constexpr struct { int16_t x; int16_t w; } GLYPHS_1X[95] = {
    {3, 3}, // [33] '!'
    {7, 5}, // [34] '"'
    {13, 5}, // [35] '#'
    {19, 5}, // [36] '$'
    {25, 5}, // [37] '%'
    {31, 5}, // [38] '&'
    {37, 3}, // [39] '''
    {41, 4}, // [40] '('
    {46, 4}, // [41] ')'
    {51, 5}, // [42] '*'
    {57, 5}, // [43] '+'
    {63, 4}, // [44] ','
    {68, 5}, // [45] '-'
    {74, 3}, // [46] '.'
    {78, 4}, // [47] '/'
    {83, 5}, // [48] '0'
    {89, 4}, // [49] '1'
    {94, 5}, // [50] '2'
    {100, 5}, // [51] '3'
    {106, 5}, // [52] '4'
    {112, 5}, // [53] '5'
    {118, 5}, // [54] '6'
    {124, 5}, // [55] '7'
    {130, 5}, // [56] '8'
    {136, 5}, // [57] '9'
    {142, 3}, // [58] ':'
    {146, 4}, // [59] ';'
    {151, 5}, // [60] '<'
    {157, 5}, // [61] '='
    {163, 5}, // [62] '>'
    {169, 5}, // [63] '?'
    {175, 5}, // [64] '@'
    {181, 5}, // [65] 'A'
    {187, 5}, // [66] 'B'
    {193, 5}, // [67] 'C'
    {199, 5}, // [68] 'D'
    {205, 5}, // [69] 'E'
    {211, 5}, // [70] 'F'
    {217, 5}, // [71] 'G'
    {223, 5}, // [72] 'H'
    {229, 5}, // [73] 'I'
    {235, 5}, // [74] 'J'
    {241, 5}, // [75] 'K'
    {247, 5}, // [76] 'L'
    {253, 7}, // [77] 'M'
    {261, 6}, // [78] 'N'
    {268, 5}, // [79] 'O'
    {274, 5}, // [80] 'P'
    {280, 5}, // [81] 'Q'
    {286, 5}, // [82] 'R'
    {292, 5}, // [83] 'S'
    {298, 5}, // [84] 'T'
    {304, 5}, // [85] 'U'
    {310, 5}, // [86] 'V'
    {316, 7}, // [87] 'W'
    {324, 5}, // [88] 'X'
    {330, 5}, // [89] 'Y'
    {336, 5}, // [90] 'Z'
    {342, 4}, // [91] '['
    {347, 5}, // [92] '\'
    {353, 4}, // [93] ']'
    {358, 5}, // [94] '^'
    {364, 5}, // [95] '_'
    {370, 4}, // [96] '`'
    {375, 5}, // [97] 'a'
    {381, 5}, // [98] 'b'
    {387, 5}, // [99] 'c'
    {393, 5}, // [100] 'd'
    {399, 5}, // [101] 'e'
    {405, 5}, // [102] 'f'
    {411, 5}, // [103] 'g'
    {417, 5}, // [104] 'h'
    {423, 3}, // [105] 'i'
    {427, 4}, // [106] 'j'
    {432, 5}, // [107] 'k'
    {438, 5}, // [108] 'l'
    {444, 7}, // [109] 'm'
    {452, 5}, // [110] 'n'
    {458, 5}, // [111] 'o'
    {464, 5}, // [112] 'p'
    {470, 5}, // [113] 'q'
    {476, 5}, // [114] 'r'
    {482, 5}, // [115] 's'
    {488, 5}, // [116] 't'
    {494, 5}, // [117] 'u'
    {500, 5}, // [118] 'v'
    {506, 7}, // [119] 'w'
    {514, 5}, // [120] 'x'
    {520, 5}, // [121] 'y'
    {526, 5}, // [122] 'z'
    {532, 5}, // [123] '{'
    {538, 3}, // [124] '|'
    {542, 5}, // [125] '}'
    {548, 5}, // [126] '~'
    {554, 5}, // [127] '#'
};

// Font2x glyphs: offset x, width
static constexpr struct { int16_t x; int16_t w; } GLYPHS_2X[95] = {
    {6, 3}, // [33] '!'
    {10, 5}, // [34] '"'
    {16, 8}, // [35] '#'
    {25, 7}, // [36] '$'
    {33, 8}, // [37] '%'
    {42, 8}, // [38] '&'
    {51, 3}, // [39] '''
    {55, 5}, // [40] '('
    {61, 5}, // [41] ')'
    {67, 8}, // [42] '*'
    {76, 7}, // [43] '+'
    {84, 4}, // [44] ','
    {89, 7}, // [45] '-'
    {97, 3}, // [46] '.'
    {101, 7}, // [47] '/'
    {109, 8}, // [48] '0'
    {118, 5}, // [49] '1'
    {124, 8}, // [50] '2'
    {133, 8}, // [51] '3'
    {142, 8}, // [52] '4'
    {151, 8}, // [53] '5'
    {160, 8}, // [54] '6'
    {169, 8}, // [55] '7'
    {178, 8}, // [56] '8'
    {187, 8}, // [57] '9'
    {196, 3}, // [58] ':'
    {200, 4}, // [59] ';'
    {205, 7}, // [60] '<'
    {213, 7}, // [61] '='
    {221, 7}, // [62] '>'
    {229, 7}, // [63] '?'
    {237, 8}, // [64] '@'
    {246, 8}, // [65] 'A'
    {255, 8}, // [66] 'B'
    {264, 8}, // [67] 'C'
    {273, 8}, // [68] 'D'
    {282, 8}, // [69] 'E'
    {291, 8}, // [70] 'F'
    {300, 8}, // [71] 'G'
    {309, 8}, // [72] 'H'
    {318, 3}, // [73] 'I'
    {322, 7}, // [74] 'J'
    {330, 8}, // [75] 'K'
    {339, 7}, // [76] 'L'
    {347, 9}, // [77] 'M'
    {357, 8}, // [78] 'N'
    {366, 8}, // [79] 'O'
    {375, 8}, // [80] 'P'
    {384, 8}, // [81] 'Q'
    {393, 8}, // [82] 'R'
    {402, 8}, // [83] 'S'
    {411, 7}, // [84] 'T'
    {419, 8}, // [85] 'U'
    {428, 7}, // [86] 'V'
    {436, 9}, // [87] 'W'
    {446, 7}, // [88] 'X'
    {454, 7}, // [89] 'Y'
    {462, 8}, // [90] 'Z'
    {471, 5}, // [91] '['
    {477, 10}, // [92] '\'
    {488, 5}, // [93] ']'
    {494, 7}, // [94] '^'
    {502, 7}, // [95] '_'
    {510, 5}, // [96] '`'
    {516, 7}, // [97] 'a'
    {524, 7}, // [98] 'b'
    {532, 7}, // [99] 'c'
    {540, 7}, // [100] 'd'
    {548, 7}, // [101] 'e'
    {556, 6}, // [102] 'f'
    {563, 7}, // [103] 'g'
    {571, 7}, // [104] 'h'
    {579, 3}, // [105] 'i'
    {583, 4}, // [106] 'j'
    {588, 7}, // [107] 'k'
    {596, 4}, // [108] 'l'
    {601, 9}, // [109] 'm'
    {611, 7}, // [110] 'n'
    {619, 7}, // [111] 'o'
    {627, 7}, // [112] 'p'
    {635, 7}, // [113] 'q'
    {643, 6}, // [114] 'r'
    {650, 7}, // [115] 's'
    {658, 7}, // [116] 't'
    {666, 7}, // [117] 'u'
    {674, 7}, // [118] 'v'
    {682, 9}, // [119] 'w'
    {692, 7}, // [120] 'x'
    {700, 7}, // [121] 'y'
    {708, 7}, // [122] 'z'
    {716, 6}, // [123] '{'
    {723, 3}, // [124] '|'
    {727, 6}, // [125] '}'
    {734, 8}, // [126] '~'
    {743, 8}, // [127] '#'
};

static std::shared_ptr<noosa::Texture> s_tex1x;
static std::shared_ptr<noosa::Texture> s_tex2x;
static std::shared_ptr<noosa::Texture> s_tex1xInk;
static std::shared_ptr<noosa::Texture> s_tex2xInk;

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int fx, int fy, int fw, int fh) {
    sub.width = static_cast<u16>(fw);
    sub.height = static_cast<u16>(fh);
    sub.left = static_cast<float>(fx) / static_cast<float>(potW);
    sub.right = static_cast<float>(fx + fw) / static_cast<float>(potW);
    sub.top = 1.0f - (static_cast<float>(fy) / static_cast<float>(potH));
    sub.bottom = 1.0f - (static_cast<float>(fy + fh) / static_cast<float>(potH));
}

void BitmapFont::init() {
    if (!s_tex1x) s_tex1x = noosa::Texture::get(Assets::FONTS1X);
    if (!s_tex2x) s_tex2x = noosa::Texture::get(Assets::FONTS2X);
    if (!s_tex1xInk) s_tex1xInk = noosa::Texture::get(Assets::FONTS1X_INK);
    if (!s_tex2xInk) s_tex2xInk = noosa::Texture::get(Assets::FONTS2X_INK);
}

float BitmapFont::height(FontType font, float scale) {
    bool is1x = (font == FontType::Font1x || font == FontType::Font1xInk);
    return (is1x ? 8.0f : 16.0f) * scale;
}

float BitmapFont::lineHeight(FontType font, float scale) {
    bool is1x = (font == FontType::Font1x || font == FontType::Font1xInk);
    return (is1x ? 9.0f : 17.0f) * scale;
}

float BitmapFont::width(FontType font, const std::string& text, float scale) {
    bool is1x = (font == FontType::Font1x || font == FontType::Font1xInk);
    float curW = 0.0f;
    float maxW = 0.0f;
    float tracking = 0.0f * scale;
    float spaceW = (is1x ? 4.0f : 6.0f) * scale;

    for (char c : text) {
        if (c == '\n') {
            maxW = std::max(maxW, curW);
            curW = 0.0f;
            continue;
        }
        if (c == ' ') {
            curW += spaceW;
            continue;
        }
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 33 && uc <= 127) {
            int idx = uc - 33;
            float gw = (is1x ? GLYPHS_1X[idx].w : GLYPHS_2X[idx].w) * scale;
            curW += gw + tracking;
        }
    }
    return std::max(maxW, curW);
}

float BitmapFont::draw(FontType font, const std::string& text, float x, float y, float z, u32 color, float scale) {
    init();
    std::shared_ptr<noosa::Texture> tex;
    switch (font) {
        case FontType::Font1x:    tex = s_tex1x; break;
        case FontType::Font2x:    tex = s_tex2x; break;
        case FontType::Font1xInk: tex = (s_tex1xInk && s_tex1xInk->isLoaded()) ? s_tex1xInk : s_tex1x; break;
        case FontType::Font2xInk: tex = (s_tex2xInk && s_tex2xInk->isLoaded()) ? s_tex2xInk : s_tex2x; break;
    }
    if (!tex || !tex->isLoaded()) return x;

    int potW = tex->potWidth();
    int potH = tex->potHeight();
    C3D_Tex* rawTex = tex->getTex();

    bool is1x = (font == FontType::Font1x || font == FontType::Font1xInk);
    float curX = std::floor(x);
    float drawY = std::floor(y);
    float tracking = 0.0f * scale;
    float spaceW = (is1x ? 4.0f : 6.0f) * scale;
    int glyphH = (is1x ? 8 : 16);
    int glyphSrcY = 0;

    C2D_ImageTint tint;
    bool hasTint = (color != 0xFFFFFFFF);
    if (hasTint) {
        C2D_PlainImageTint(&tint, color, 1.0f);
    }

    Tex3DS_SubTexture sub;
    for (char c : text) {
        if (c == '\n') {
            break; // Single line draw ignores newline
        }
        if (c == ' ') {
            curX += spaceW;
            continue;
        }
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 33 && uc <= 127) {
            int idx = uc - 33;
            int gx = (is1x ? GLYPHS_1X[idx].x : GLYPHS_2X[idx].x);
            int gw = (is1x ? GLYPHS_1X[idx].w : GLYPHS_2X[idx].w);

            setupSubTex(sub, potW, potH, gx, glyphSrcY, gw, glyphH);
            C2D_Image img = { rawTex, &sub };

            C2D_DrawParams p;
            p.pos.x = std::floor(curX);
            p.pos.y = drawY;
            p.pos.w = gw * scale;
            p.pos.h = glyphH * scale;
            p.center.x = 0.0f;
            p.center.y = 0.0f;
            p.depth = z;
            p.angle = 0.0f;

            C2D_DrawImage(img, &p, hasTint ? &tint : nullptr);

            curX += (gw * scale) + tracking;
        }
    }

    return curX;
}

float BitmapFont::drawWithShadow(FontType font, const std::string& text, float x, float y, float z, u32 color, u32 shadowColor, float scale) {
    draw(font, text, x + 1.0f, y + 1.0f, z - 0.001f, shadowColor, scale);
    return draw(font, text, x, y, z, color, scale);
}

std::vector<std::string> BitmapFont::splitMultiline(FontType font, const std::string& text, float maxWidth, float scale) {
    std::vector<std::string> lines;
    std::stringstream textStream(text);
    std::string paragraph;
    while (std::getline(textStream, paragraph)) {
        if (paragraph.empty()) {
            lines.push_back("");
            continue;
        }

        std::stringstream ss(paragraph);
        std::string word;
        std::string currentLine;
        while (ss >> word) {
            float wordW = width(font, word, scale);
            float spaceW = (font == FontType::Font1x || font == FontType::Font1xInk ? 4.0f : 6.0f) * scale;
            float lineW = currentLine.empty() ? 0.0f : (width(font, currentLine, scale) + spaceW);

            if (!currentLine.empty() && (lineW + wordW) > maxWidth) {
                lines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}

float BitmapFont::drawClipped(FontType font, const std::string& text, float x, float y, float clipTop, float clipBottom, float z, u32 color, float scale) {
    if (clipBottom <= clipTop) return x;
    bool is1x = (font == FontType::Font1x || font == FontType::Font1xInk);
    float nominalH = (is1x ? 8.0f : 16.0f) * scale;
    if (y + nominalH <= clipTop || y >= clipBottom) {
        return x; // Completely outside vertical clipping range
    }

    init();
    std::shared_ptr<noosa::Texture> tex;
    switch (font) {
        case FontType::Font1x:    tex = s_tex1x; break;
        case FontType::Font2x:    tex = s_tex2x; break;
        case FontType::Font1xInk: tex = (s_tex1xInk && s_tex1xInk->isLoaded()) ? s_tex1xInk : s_tex1x; break;
        case FontType::Font2xInk: tex = (s_tex2xInk && s_tex2xInk->isLoaded()) ? s_tex2xInk : s_tex2x; break;
    }
    if (!tex || !tex->isLoaded()) return x;

    int potW = tex->potWidth();
    int potH = tex->potHeight();
    C3D_Tex* rawTex = tex->getTex();

    float curX = std::floor(x);
    float tracking = 0.0f * scale;
    float spaceW = (is1x ? 4.0f : 6.0f) * scale;
    int baseGlyphH = (is1x ? 8 : 16);

    C2D_ImageTint tint;
    bool hasTint = (color != 0xFFFFFFFF);
    if (hasTint) {
        C2D_PlainImageTint(&tint, color, 1.0f);
    }

    Tex3DS_SubTexture sub;
    for (char c : text) {
        if (c == '\n') break;
        if (c == ' ') {
            curX += spaceW;
            continue;
        }
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 33 && uc <= 127) {
            int idx = uc - 33;
            int gx = (is1x ? GLYPHS_1X[idx].x : GLYPHS_2X[idx].x);
            int gw = (is1x ? GLYPHS_1X[idx].w : GLYPHS_2X[idx].w);

            float drawY = y;
            int glyphH = baseGlyphH;
            int glyphSrcY = 0;

            if (drawY < clipTop) {
                float topCut = (clipTop - drawY) / scale;
                int cutPx = static_cast<int>(std::ceil(topCut));
                if (cutPx >= glyphH) continue;
                glyphSrcY += cutPx;
                glyphH -= cutPx;
                drawY += cutPx * scale;
            }
            if (drawY + glyphH * scale > clipBottom) {
                float botCut = ((drawY + glyphH * scale) - clipBottom) / scale;
                int cutPx = static_cast<int>(std::ceil(botCut));
                if (cutPx >= glyphH) continue;
                glyphH -= cutPx;
            }

            if (glyphH > 0 && gw > 0) {
                setupSubTex(sub, potW, potH, gx, glyphSrcY, gw, glyphH);
                C2D_Image img = { rawTex, &sub };

                C2D_DrawParams p;
                p.pos.x = std::floor(curX);
                p.pos.y = std::floor(drawY);
                p.pos.w = gw * scale;
                p.pos.h = glyphH * scale;
                p.center.x = 0.0f;
                p.center.y = 0.0f;
                p.depth = z;
                p.angle = 0.0f;

                C2D_DrawImage(img, &p, hasTint ? &tint : nullptr);
            }

            curX += (gw * scale) + tracking;
        }
    }

    return curX;
}

float BitmapFont::drawMultiline(FontType font, const std::string& text, float x, float y, float maxWidth, float z, u32 color, float scale) {
    auto lines = splitMultiline(font, text, maxWidth, scale);
    float lineH = lineHeight(font, scale);
    float curY = y;
    for (const auto& line : lines) {
        if (!line.empty()) {
            draw(font, line, x, curY, z, color, scale);
        }
        curY += lineH;
    }
    return curY;
}

} // namespace pd::ui

