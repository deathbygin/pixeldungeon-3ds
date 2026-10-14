#include "noosa/Profiler.h"
#include <algorithm>

namespace noosa {

// Old 3DS ARM11 clock: ~268.123480 MHz (ticks per millisecond)
static constexpr double TICKS_PER_MSEC = 268123.48;

u64 Profiler::s_frameStartTick = 0;
u64 Profiler::s_updateStartTick = 0;
u64 Profiler::s_renderStartTick = 0;

float Profiler::s_frameTimeMs = 16.66f;
float Profiler::s_updateTimeMs = 1.0f;
float Profiler::s_renderTimeMs = 15.0f;
float Profiler::s_fps = 60.0f;

uint32_t Profiler::s_tilesDrawn = 0;
uint32_t Profiler::s_tilesCulled = 0;

bool Profiler::s_overlayEnabled = false;
int Profiler::s_frameCount = 0;
u64 Profiler::s_lastFpsUpdateTick = 0;

void Profiler::init() {
    s_frameStartTick = svcGetSystemTick();
    s_lastFpsUpdateTick = s_frameStartTick;
    s_frameCount = 0;
    s_fps = 60.0f;
    s_frameTimeMs = 16.66f;
    s_updateTimeMs = 1.0f;
    s_renderTimeMs = 15.0f;
    s_tilesDrawn = 0;
    s_tilesCulled = 0;
    s_overlayEnabled = false;
}

void Profiler::beginUpdate() {
    s_updateStartTick = svcGetSystemTick();
}

void Profiler::endUpdate() {
    u64 now = svcGetSystemTick();
    float elapsed = static_cast<float>((now - s_updateStartTick) / TICKS_PER_MSEC);
    s_updateTimeMs = s_updateTimeMs * 0.9f + elapsed * 0.1f;
}

void Profiler::beginRender() {
    s_renderStartTick = svcGetSystemTick();
}

void Profiler::endRender() {
    u64 now = svcGetSystemTick();
    float elapsed = static_cast<float>((now - s_renderStartTick) / TICKS_PER_MSEC);
    s_renderTimeMs = s_renderTimeMs * 0.9f + elapsed * 0.1f;
}

void Profiler::endFrame() {
    u64 now = svcGetSystemTick();
    float frameMs = static_cast<float>((now - s_frameStartTick) / TICKS_PER_MSEC);
    s_frameTimeMs = s_frameTimeMs * 0.9f + frameMs * 0.1f;
    s_frameStartTick = now;

    s_frameCount++;
    double elapsedSinceFps = (now - s_lastFpsUpdateTick) / TICKS_PER_MSEC;
    if (elapsedSinceFps >= 500.0) { // Update FPS every 500 ms
        float currentFps = static_cast<float>((s_frameCount * 1000.0) / elapsedSinceFps);
        s_fps = s_fps * 0.5f + currentFps * 0.5f;
        s_frameCount = 0;
        s_lastFpsUpdateTick = now;
    }
}

} // namespace noosa

