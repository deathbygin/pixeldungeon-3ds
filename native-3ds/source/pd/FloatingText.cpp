#include "pd/FloatingText.h"
#include <algorithm>

namespace pd {

void FloatingText::reset(float worldX, float worldY, const std::string& str, uint32_t col) {
    x = worldX;
    y = worldY - 4.0f; // Position slightly above target head
    text = str;
    color = col;
    time = LIFESPAN;
    active = true;
}

void FloatingText::update(float dt) {
    if (!active) return;

    time -= dt;
    if (time <= 0.0f) {
        active = false;
        return;
    }

    // Float upward by 16 world pixels over 1.0 second
    y -= 16.0f * dt;
}

void FloatingText::draw(const noosa::Camera* camera, C2D_TextBuf textBuf) const {
    if (!active || time <= 0.0f || text.empty() || !textBuf) return;

    float drawX = x;
    float drawY = y;

    if (camera) {
        camera->worldToScreen(x, y, drawX, drawY);
    }

    float p = time / LIFESPAN;
    float alphaFactor = (p > 0.5f) ? 1.0f : (p * 2.0f);

    uint8_t a = static_cast<uint8_t>(std::clamp(alphaFactor * 255.0f, 0.0f, 255.0f));
    uint8_t r = static_cast<uint8_t>((color >> 24) & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 16) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 8) & 0xFF);
    u32 finalColor = C2D_Color32(r, g, b, a);

    C2D_Text textObj;
    C2D_TextParse(&textObj, textBuf, text.c_str());
    C2D_TextOptimize(&textObj);

    // Draw with slight shadow for readability
    u32 shadowColor = C2D_Color32(0, 0, 0, a / 2);
    C2D_DrawText(&textObj, C2D_WithColor, drawX + 1.0f, drawY + 1.0f, 0.88f, 0.45f, 0.45f, shadowColor);
    C2D_DrawText(&textObj, C2D_WithColor, drawX, drawY, 0.89f, 0.45f, 0.45f, finalColor);
}

} // namespace pd

