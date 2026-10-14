#pragma once

#include "noosa/Types.h"
#include "noosa/Camera.h"
#include <citro2d.h>

namespace pd {

class PixelParticle {
public:
    noosa::PointF pos{ 0.0f, 0.0f };
    noosa::PointF speed{ 0.0f, 0.0f };
    noosa::PointF acc{ 0.0f, 0.0f };

    float size = 2.0f;
    float lifespan = 0.6f;
    float left = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    bool shrinking = true;
    bool active = false;

    PixelParticle() = default;

    void reset(float x, float y, uint32_t col, float sz, float life,
               float vx = 0.0f, float vy = 0.0f, float ax = 0.0f, float ay = 0.0f,
               bool shrink = true);

    void update(float dt);
    void draw(const noosa::Camera* camera) const;
};

} // namespace pd

