#include "pd/CheckedCell.h"
#include "pd/Level.h"
#include <algorithm>

namespace pd {

void CheckedCell::reset(int cell) {
    active = true;
    time = LIFESPAN;
    cx = (cell % Level::WIDTH) * 16.0f + 8.0f;
    cy = (cell / Level::WIDTH) * 16.0f + 8.0f;
}

void CheckedCell::update(float dt) {
    if (!active) return;

    time -= dt;
    if (time <= 0.0f) {
        active = false;
    }
}

void CheckedCell::draw(const noosa::Camera* camera) const {
    if (!active || time <= 0.0f) return;

    float p = time / LIFESPAN;
    float currentSize = 16.0f * p;
    float half = currentSize * 0.5f;

    float drawX = cx - half;
    float drawY = cy - half;

    if (camera) {
        float scrCX, scrCY;
        camera->worldToScreen(cx, cy, scrCX, scrCY);
        float zoom = camera->getZoom();
        currentSize *= zoom;
        half = currentSize * 0.5f;
        drawX = scrCX - half;
        drawY = scrCY - half;
    }

    uint8_t a = static_cast<uint8_t>(std::clamp(p * 200.0f, 0.0f, 255.0f));
    u32 color = C2D_Color32(0x55, 0xaa, 0xff, a);
    C2D_DrawRectSolid(drawX, drawY, 0.4f, currentSize, currentSize, color);
}

} // namespace pd

