#pragma once

#include <3ds.h>
#include <citro2d.h>

namespace noosa {

// Stereoscopic state for the top screen. Citro2D renders orthographically, so depth is
// produced by shifting each plane horizontally in opposite directions for the two eyes.
class Stereo {
public:
    enum class Eye { Mono, Left, Right };

    // Depth planes, as the left/right disparity in screen pixels at full slider.
    // Negative sits behind the screen, positive in front of it. The whole range spans 18px,
    // under 1 degree of convergence at arm's length even on the larger XL panel.
    static constexpr float PLANE_FLOOR  = -12.0f; // Floor, water, chasms and ground decals
    static constexpr float PLANE_WALL   = -6.0f;  // Walls, doors, tall grass
    static constexpr float PLANE_FOG    = -4.0f;  // Fog of war, floating above the terrain
    static constexpr float PLANE_ENTITY = 0.0f;   // Hero, mobs, loot (sharpest, on the screen itself)
    static constexpr float PLANE_APEX   = 4.0f;   // Peak of airborne arcs and rising particles
    static constexpr float PLANE_UI     = 6.0f;   // Text, banners and overlays: always nearest

    // Largest per-eye shift of any plane; off-screen content within this margin must still draw.
    static constexpr float MAX_EYE_SHIFT = 6.0f;

    // Samples the 3D slider. Call once per frame, before the first eye.
    static void beginFrame();

    static bool isActive() { return s_slider > 0.0f; }

    static void beginEye(Eye eye);
    static void endEye();

    // Moves all following draws onto a plane, snapped to whole pixels so both eyes sample
    // pixel art identically. Each change flushes Citro2D's vertex batch, so call per layer.
    static void setPlane(float plane);

    // As setPlane, without snapping: for the few sprites animating smoothly through depth.
    static void setDepth(float plane);

    // Shift for this eye, for callers placing vertices themselves. Particle pools must use
    // this rather than setDepth, as a flush per particle would stall the GPU.
    static float shift(float plane) { return plane * 0.5f * s_slider * s_eyeSign; }

    // The whole-pixel shift setPlane applies.
    static float snappedShift(float plane);

private:
    static void applyShift(float px);

    static float s_slider;
    static float s_eyeSign;
    static float s_viewShift;
};

} // namespace noosa
