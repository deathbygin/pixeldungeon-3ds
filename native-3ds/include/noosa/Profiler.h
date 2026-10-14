#pragma once

#include <3ds.h>
#include <cstdint>

namespace noosa {

class Profiler {
public:
    static void init();

    // Call at start and end of update phase
    static void beginUpdate();
    static void endUpdate();

    // Call at start and end of render phase
    static void beginRender();
    static void endRender();

    // Call once per frame (after C3D_FrameEnd)
    static void endFrame();

    // Metrics accessors
    static float getFps() { return s_fps; }
    static float getFrameTimeMs() { return s_frameTimeMs; }
    static float getUpdateTimeMs() { return s_updateTimeMs; }
    static float getRenderTimeMs() { return s_renderTimeMs; }
    static uint32_t getTilesDrawn() { return s_tilesDrawn; }
    static uint32_t getTilesCulled() { return s_tilesCulled; }

    static void recordTiles(uint32_t drawn, uint32_t culled) {
        s_tilesDrawn = drawn;
        s_tilesCulled = culled;
    }

    // Toggle debug performance overlay
    static bool isOverlayEnabled() { return s_overlayEnabled; }
    static void toggleOverlay() { s_overlayEnabled = !s_overlayEnabled; }
    static void setOverlayEnabled(bool enabled) { s_overlayEnabled = enabled; }

private:
    static u64 s_frameStartTick;
    static u64 s_updateStartTick;
    static u64 s_renderStartTick;

    static float s_frameTimeMs;
    static float s_updateTimeMs;
    static float s_renderTimeMs;
    static float s_fps;

    static uint32_t s_tilesDrawn;
    static uint32_t s_tilesCulled;

    static bool s_overlayEnabled;
    static int s_frameCount;
    static u64 s_lastFpsUpdateTick;
};

} // namespace noosa

