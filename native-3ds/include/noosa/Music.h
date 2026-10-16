#pragma once

#include <3ds.h>
#include <string>
#include <cstdint>
#include "noosa/minimp3.h"
#include "noosa/minimp3_ex.h"

namespace noosa {

class Music {
public:
    static Music INSTANCE;

    static void init();
    static void exit();

    // Start streaming background music track
    static void play(const std::string& track, bool loop = true);

    // Stop current background music playback
    static void stop();

    // Pause / resume current track
    static void pause();
    static void resume();

    // Set master volume for background music (0.0f to 1.0f)
    static void volume(float vol);
    static float volume() { return s_volume; }

    // Enable / disable music playback
    static void enable(bool en);
    static bool isEnabled() { return s_enabled; }

    // Check if music is active
    static bool isPlaying() { return s_isPlaying && !s_isPaused; }

    // Stream pump - must be called every frame in main loop
    static void update();

    // Name of currently loaded track
    static const std::string& currentTrack() { return s_currentTrack; }

private:
    static constexpr int BGM_CHANNEL = 0;
    static constexpr int BUFFER_COUNT = 3;
    static constexpr int BUFFER_SAMPLES = 16384; // 16384 sample frames (~371ms at 44.1kHz)

    static bool fillBuffer(int bufIndex);

    static bool s_initialized;
    static bool s_enabled;
    static bool s_isPlaying;
    static bool s_isPaused;
    static bool s_loop;
    static float s_volume;
    static std::string s_currentTrack;

    static mp3dec_ex_t s_dec;
    static bool s_decOpen;

    static int16_t* s_bufferMem[BUFFER_COUNT];
    static ndspWaveBuf s_waveBufs[BUFFER_COUNT];
};

} // namespace noosa

