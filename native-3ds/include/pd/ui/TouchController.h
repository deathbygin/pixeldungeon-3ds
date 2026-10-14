#pragma once

#include <3ds.h>
#include "noosa/Types.h"

namespace pd::ui {

class TouchController {
public:
    TouchController() = default;

    void update(u32 kDown, u32 kHeld, u32 kUp);

    bool isDown() const { return m_isDown; }
    bool justPressed() const { return m_justPressed; }
    bool justReleased() const { return m_justReleased; }

    int x() const { return m_current.px; }
    int y() const { return m_current.py; }

    noosa::PointF pos() const {
        return noosa::PointF(static_cast<float>(m_current.px), static_cast<float>(m_current.py));
    }

    bool hitTest(const noosa::RectF& rect) const {
        return m_isDown && rect.contains(static_cast<float>(m_current.px), static_cast<float>(m_current.py));
    }

    bool clicked(const noosa::RectF& rect) const {
        return m_justReleased && rect.contains(static_cast<float>(m_released.px), static_cast<float>(m_released.py));
    }

    float heldTime() const { return m_heldTime; }

private:
    touchPosition m_current{ 0, 0 };
    touchPosition m_start{ 0, 0 };
    touchPosition m_released{ 0, 0 };

    bool m_isDown = false;
    bool m_justPressed = false;
    bool m_justReleased = false;
    float m_heldTime = 0.0f;
};

} // namespace pd::ui

