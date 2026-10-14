#pragma once

#include "Types.h"
#include <citro2d.h>

namespace noosa {

class Camera;

class Visual {
public:
    Visual();
    virtual ~Visual() = default;

    // Transform properties
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.5f; // Citro2D depth layer (0.0 to 1.0)
    float width = 0.0f;
    float height = 0.0f;

    PointF scale{ 1.0f, 1.0f };
    PointF origin{ 0.0f, 0.0f };
    float angle = 0.0f; // radians

    // Appearance
    float alpha = 1.0f;
    Color tint{ 255, 255, 255, 255 };

    // Screen destination
    ScreenTarget targetScreen = ScreenTarget::Top;

    // Lifecycle flags
    bool visible = true;
    bool active = true;
    bool alive = true;

    // Center coordinates
    PointF center() const {
        return PointF(x + (width * 0.5f), y + (height * 0.5f));
    }

    void setPos(float px, float py) {
        x = px;
        y = py;
    }

    void setSize(float w, float h) {
        width = w;
        height = h;
    }

    virtual void kill() {
        alive = false;
        active = false;
        visible = false;
    }

    virtual void revive() {
        alive = true;
        active = true;
        visible = true;
    }

    virtual void update(float dt) {
        (void)dt;
    }

    virtual void draw(const Camera* camera = nullptr) {
        (void)camera;
    }
};

} // namespace noosa

