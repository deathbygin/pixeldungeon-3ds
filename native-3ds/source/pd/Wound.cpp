#include "pd/Wound.h"
#include "pd/Level.h"

namespace pd {

// Wounds live in a static pool, so the sprite is created on first use: loading the
// texture from the constructor would run before main() has mounted RomFS.
Wound::Wound() = default;

void Wound::reset(int cell) {
    if (!sprite) {
        sprite = Effects::get(Effects::Type::WOUND);
        if (!sprite) return;
        sprite->origin.x = sprite->width / 2.0f;
        sprite->origin.y = sprite->height / 2.0f;
    }

    active = true;
    time = TIME_TO_FADE;

    sprite->x = (cell % Level::WIDTH) * 16.0f + 8.0f;
    sprite->y = (cell / Level::WIDTH) * 16.0f + 8.0f;
    sprite->scale.x = 2.0f * SIZE;
    sprite->scale.y = 1.0f * SIZE;
    sprite->alpha = 1.0f;
    sprite->z = 0.76f;
    sprite->visible = true;
}

void Wound::update(float dt) {
    if (!active || !sprite) return;

    time -= dt;
    if (time <= 0.0f) {
        active = false;
        sprite->visible = false;
        return;
    }

    float p = time / TIME_TO_FADE;
    sprite->alpha = p;
    sprite->scale.x = (1.0f + p) * SIZE;
}

void Wound::draw(const noosa::Camera* camera) {
    if (active && sprite && sprite->visible) {
        sprite->draw(camera);
    }
}

} // namespace pd

