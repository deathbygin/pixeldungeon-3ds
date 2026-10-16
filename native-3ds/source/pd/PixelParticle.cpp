#include "pd/PixelParticle.h"
#include <algorithm>

namespace pd {

void PixelParticle::reset(float x, float y, uint32_t col, float sz, float life,
                          float vx, float vy, float ax, float ay, bool shrink) {
    pos.x = x;
    pos.y = y;
    speed.x = vx;
    speed.y = vy;
    acc.x = ax;
    acc.y = ay;
    color = col;
    size = sz;
    lifespan = (life > 0.0f) ? life : 0.6f;
    left = lifespan;
    shrinking = shrink;
    active = true;
}

void PixelParticle::update(float dt) {
    if (!active) return;

    left -= dt;
    if (left <= 0.0f) {
        active = false;
        return;
    }

    speed.x += acc.x * dt;
    speed.y += acc.y * dt;
    pos.x += speed.x * dt;
    pos.y += speed.y * dt;
}

void PixelParticle::draw(const noosa::Camera* camera) const {
    if (!active || left <= 0.0f) return;

    float p = left / lifespan;
    float curSize = shrinking ? (size * p) : size;
    if (curSize < 1.0f) curSize = 1.0f;

    float drawX = pos.x;
    float drawY = pos.y;
    float zoom = 1.0f;

    if (camera) {
        camera->worldToScreen(pos.x, pos.y, drawX, drawY);
        zoom = camera->getZoom();
        curSize *= zoom;
    }

    // Authentic Pixel Dungeon alpha envelope: rapid fade in, sustained bright body, gentle fade out
    float alphaFactor = (p > 0.85f) ? ((1.0f - p) * 6.67f) : ((p < 0.20f) ? (p * 5.0f) : 1.0f);
    uint8_t a = static_cast<uint8_t>(std::clamp(alphaFactor * 255.0f, 0.0f, 255.0f));
    uint8_t r = static_cast<uint8_t>((color >> 24) & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 16) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 8) & 0xFF);

    u32 finalColor = C2D_Color32(r, g, b, a);
    // Depth 0.78f ensures particles render in front of characters (0.60f) and Fog of War (0.65f)
    C2D_DrawRectSolid(drawX - curSize * 0.5f, drawY - curSize * 0.5f, 0.78f, curSize, curSize, finalColor);
}

} // namespace pd

