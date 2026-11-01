#pragma once

#include "pd/Effects.h"
#include "noosa/Image.h"
#include "noosa/Camera.h"
#include <memory>

namespace pd {

class Wound {
public:
    static constexpr float TIME_TO_FADE = 0.8f;
    // Overall size of the hit slash; 1.0 spans two tiles at the start of the fade.
    static constexpr float SIZE = 0.5f;

    bool active = false;
    float time = 0.0f;
    std::unique_ptr<noosa::Image> sprite;

    Wound();

    void reset(int cell);
    void update(float dt);
    void draw(const noosa::Camera* camera);
};

} // namespace pd

