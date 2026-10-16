#include "pd/Wound.h"
#include "pd/Level.h"

namespace pd {

Wound::Wound() {
    sprite = Effects::get(Effects::Type::WOUND);
    if (sprite) {
        sprite->origin.x = sprite->width / 2.0f;
        sprite->origin.y = sprite->height / 2.0f;
    }
}

void Wound::reset(int cell) {
    if (!sprite) return;

    active = true;
    time = TIME_TO_FADE;

    sprite->x = (cell % Level::WIDTH) * 16.0f + 8.0f;
    sprite->y = (cell / Level::WIDTH) * 16.0f + 8.0f;
    sprite->scale.x = 2.0f;
    sprite->scale.y = 1.0f;
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
    sprite->scale.x = 1.0f + p;
}

void Wound::draw(const noosa::Camera* camera) {
    if (active && sprite && sprite->visible) {
        sprite->draw(camera);
    }
}

} // namespace pd

