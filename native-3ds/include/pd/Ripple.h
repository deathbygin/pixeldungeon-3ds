#pragma once

#include "pd/Effects.h"
#include "noosa/Image.h"
#include "noosa/Camera.h"
#include <memory>

namespace pd {

class Ripple {
public:
    static constexpr float TIME_TO_FADE = 0.5f;

    bool active = false;
    float time = 0.0f;
    std::unique_ptr<noosa::Image> sprite;

    Ripple();

    void reset(int cell);
    void update(float dt);
    void draw(const noosa::Camera* camera);
};

} // namespace pd

