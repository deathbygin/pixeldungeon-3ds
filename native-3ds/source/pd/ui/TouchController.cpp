#include "pd/ui/TouchController.h"

namespace pd::ui {

void TouchController::update(u32 kDown, u32 kHeld, u32 kUp) {
    m_justPressed = (kDown & KEY_TOUCH) != 0;
    m_isDown = (kHeld & KEY_TOUCH) != 0;
    m_justReleased = (kUp & KEY_TOUCH) != 0;

    if (m_isDown) {
        hidTouchRead(&m_current);
        if (m_justPressed) {
            m_start = m_current;
            m_heldTime = 0.0f;
        } else {
            m_heldTime += 1.0f / 60.0f;
        }
    }

    if (m_justReleased) {
        m_released = m_current;
    }
}

} // namespace pd::ui

