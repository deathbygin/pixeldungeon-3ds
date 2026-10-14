#pragma once

#include "noosa/Camera.h"
#include <citro2d.h>

namespace pd {

class CheckedCell {
public:
    static constexpr float LIFESPAN = 0.8f;

    bool active = false;
    float time = 0.0f;
    float cx = 0.0f;
    float cy = 0.0f;

    CheckedCell() = default;

    void reset(int cell);
    void update(float dt);
    void draw(const noosa::Camera* camera) const;
};

} // namespace pd

