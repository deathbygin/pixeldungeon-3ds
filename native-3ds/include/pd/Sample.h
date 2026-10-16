#pragma once

#include "noosa/Audio.h"
#include <string>

namespace pd {

class Sample {
public:
    static Sample INSTANCE;

    // Direct playback matching Java Sample.INSTANCE.play
    static int play(const std::string& sound, float leftVol, float rightVol, float pitch);
    static int play(const std::string& sound, float volume, float pitch);
    static int play(const std::string& sound, float volume);
    static int play(const std::string& sound);

    // Spatial sound with distance attenuation and stereo panning relative to Hero's cell
    static int playAt(const std::string& sound, int cell, float baseVolume = 1.0f, float pitch = 1.0f);

    // Call once per frame in main loop to reset frame concurrency caps
    static void update(float dt);
};

} // namespace pd
