#include "pd/ui/Archs.h"
#include "pd/Assets.h"
#include <cmath>

namespace pd::ui {

float Archs::offsB = 0.0f;
float Archs::offsF = 0.0f;

Archs::Archs() {
    m_texBg = noosa::Texture::get(pd::Assets::ARCS_BG);
    m_texFg = noosa::Texture::get(pd::Assets::ARCS_FG);
}

void Archs::update(float dt) {
    float shift = dt * SCROLL_SPEED;
    if (reversed) {
        shift = -shift;
    }

    offsB += shift;
    offsF += shift * 2.0f;

    // Wrap within 128px texture tile
    const float tileH = 128.0f;
    while (offsB >= tileH) offsB -= tileH;
    while (offsB < 0.0f)   offsB += tileH;
    while (offsF >= tileH) offsF -= tileH;
    while (offsF < 0.0f)   offsF += tileH;
}

static void drawTiledLayer(noosa::Texture* tex, float screenW, float screenH,
                           float scrollY, float z, u32 tintColor = 0xFFFFFFFF) {
    if (!tex || !tex->isLoaded() || !tex->getTex()) {
        return;
    }

    const float tileW = static_cast<float>(tex->width() > 0 ? tex->width() : 128);
    const float tileH = static_cast<float>(tex->height() > 0 ? tex->height() : 128);

    // Java Archs.java:
    // arcsBg.offset( arcsBg.texture.width / 4 - (width % arcsBg.texture.width) / 2, 0 );
    float horizOffset = std::floor(tileW / 4.0f - std::fmod(screenW, tileW) / 2.0f);

    float startX = -tileW + horizOffset;
    while (startX < 0.0f) startX += tileW;
    startX -= tileW;

    float wrappedY = std::fmod(scrollY, tileH);
    if (wrappedY < 0.0f) wrappedY += tileH;
    float startY = -tileH + wrappedY;

    // Texture UV mapping for power-of-two texture
    float potW = static_cast<float>(tex->potWidth());
    float potH = static_cast<float>(tex->potHeight());

    Tex3DS_SubTexture subTex;
    subTex.width = static_cast<u16>(tileW);
    subTex.height = static_cast<u16>(tileH);
    subTex.left = 0.0f;
    subTex.right = tileW / potW;
    subTex.top = 1.0f;
    subTex.bottom = 1.0f - (tileH / potH);

    C2D_Image img;
    img.tex = tex->getTex();
    img.subtex = &subTex;

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, tintColor, 1.0f);

    for (float curX = startX; curX < screenW + tileW; curX += tileW) {
        for (float curY = startY; curY < screenH + tileH; curY += tileH) {
            C2D_DrawParams params;
            params.pos.x = curX;
            params.pos.y = curY;
            params.pos.w = tileW;
            params.pos.h = tileH;
            params.center.x = 0.0f;
            params.center.y = 0.0f;
            params.depth = z;
            params.angle = 0.0f;

            if (tintColor != 0xFFFFFFFF) {
                C2D_DrawImage(img, &params, &tint);
            } else {
                C2D_DrawImage(img, &params, nullptr);
            }
        }
    }
}

void Archs::drawTop(float z) const {
    // Top screen starts at global Y = 0
    drawTiledLayer(m_texBg.get(), SCREEN_TOP_W, SCREEN_TOP_H, offsB, z);
    drawTiledLayer(m_texFg.get(), SCREEN_TOP_W, SCREEN_TOP_H, offsF, z + 0.01f);
}

void Archs::drawBottom(float z) const {
    // Bottom screen continuous alignment: Top Screen (240) + Hinge Gap (40) = 280px offset
    const float virtualOffset = SCREEN_TOP_H + HINGE_GAP;
    drawTiledLayer(m_texBg.get(), SCREEN_BOT_W, SCREEN_BOT_H, offsB + virtualOffset, z);
    drawTiledLayer(m_texFg.get(), SCREEN_BOT_W, SCREEN_BOT_H, offsF + virtualOffset * 2.0f, z + 0.01f);
}

} // namespace pd::ui

