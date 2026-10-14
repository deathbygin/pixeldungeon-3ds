#pragma once

#include "noosa/Types.h"
#include "noosa/Camera.h"
#include <citro2d.h>
#include <string>

namespace pd {

class FloatingText {
public:
    static constexpr float LIFESPAN = 1.0f;

    bool active = false;
    float time = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    std::string text;
    uint32_t color = 0xFFFFFFFF;

    FloatingText() = default;

    void reset(float worldX, float worldY, const std::string& str, uint32_t col);
    void update(float dt);
    void draw(const noosa::Camera* camera, C2D_TextBuf textBuf) const;
};

} // namespace pd

