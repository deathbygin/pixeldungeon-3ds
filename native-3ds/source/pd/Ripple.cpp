#include "pd/Ripple.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"

namespace pd {

Ripple::Ripple() {
    sprite = Effects::get(Effects::Type::RIPPLE);
    if (sprite) {
        sprite->origin.x = sprite->width / 2.0f;
        sprite->origin.y = sprite->height / 2.0f;
    }
}

void Ripple::reset(int cell) {
    if (!sprite) return;

    active = true;
    time = TIME_TO_FADE;

    sprite->x = (cell % Level::WIDTH) * 16.0f + 8.0f;
    sprite->y = (cell / Level::WIDTH) * 16.0f + 8.0f;
    sprite->scale.x = 0.0f;
    sprite->scale.y = 0.0f;
    sprite->alpha = 1.0f;
    sprite->z = 0.27f;
    sprite->visible = true;

    uint32_t col = Dungeon::waterColor();
    uint8_t r = static_cast<uint8_t>((col >> 24) & 0xFF);
    uint8_t g = static_cast<uint8_t>((col >> 16) & 0xFF);
    uint8_t b = static_cast<uint8_t>((col >> 8) & 0xFF);
    sprite->tint = noosa::Color(r, g, b, 255);
}

void Ripple::update(float dt) {
    if (!active || !sprite) return;

    time -= dt;
    if (time <= 0.0f) {
        active = false;
        sprite->visible = false;
        return;
    }

    float p = time / TIME_TO_FADE;
    float s = 1.0f - p;
    sprite->scale.x = s;
    sprite->scale.y = s;
    sprite->alpha = p;
}

void Ripple::draw(const noosa::Camera* camera) {
    if (active && sprite && sprite->visible) {
        sprite->draw(camera);
    }
}

} // namespace pd

