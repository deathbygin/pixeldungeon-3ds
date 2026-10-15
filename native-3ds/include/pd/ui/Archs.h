#pragma once

#include <citro2d.h>
#include <memory>
#include "noosa/Texture.h"

namespace pd::ui {

/**
 * Animated parallax colonnade background matching Pixel Dungeon's Arcades / Archs.
 * Synchronized scrolling across dual screens with vertical hinge gap continuity.
 */
class Archs {
public:
    Archs();
    ~Archs() = default;

    void update(float dt);

    // Render for Top Screen (400x240)
    void drawTop(float z = 0.05f) const;

    // Render for Bottom Screen (320x240)
    void drawBottom(float z = 0.05f) const;

    bool reversed = false;

    // Synchronized scroll offsets (matching Java static offsB / offsF)
    static float offsB;
    static float offsF;

private:
    std::shared_ptr<noosa::Texture> m_texBg;
    std::shared_ptr<noosa::Texture> m_texFg;

    static constexpr float SCROLL_SPEED = 20.0f; // px/sec
    static constexpr float SCREEN_TOP_W = 400.0f;
    static constexpr float SCREEN_TOP_H = 240.0f;
    static constexpr float SCREEN_BOT_W = 320.0f;
    static constexpr float SCREEN_BOT_H = 240.0f;
    static constexpr float HINGE_GAP    = 40.0f;  // Virtual physical hinge gap
};

} // namespace pd::ui

