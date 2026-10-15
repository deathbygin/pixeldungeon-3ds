#pragma once

#include <cstdint>

namespace pd {

class Preferences {
public:
    static void init();
    static void save();

    // Camera zoom factor on Top Screen: 1.0f (Zoomed Out), 2.0f (Default), 3.0f (Zoomed In)
    static float zoom() { return s_zoom; }
    static void setZoom(float z);

    // Background music toggle
    static bool music() { return s_music; }
    static void setMusic(bool m);

    // Sound effects toggle
    static bool soundFx() { return s_soundFx; }
    static void setSoundFx(bool sfx);

    // Brightness toggle (boosts ambient tile tint and clears fog)
    static bool brightness() { return s_brightness; }
    static void setBrightness(bool b);

private:
    static float s_zoom;
    static bool s_music;
    static bool s_soundFx;
    static bool s_brightness;
};

} // namespace pd

