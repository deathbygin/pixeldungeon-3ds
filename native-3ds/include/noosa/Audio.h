#pragma once

#include <3ds.h>
#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace noosa {

class SoundSample {
public:
    SoundSample();
    ~SoundSample();

    SoundSample(const SoundSample&) = delete;
    SoundSample& operator=(const SoundSample&) = delete;
    SoundSample(SoundSample&& other) noexcept;
    SoundSample& operator=(SoundSample&& other) noexcept;

    // Load standard PCM16 WAV from file (RomFS or SD)
    bool loadWAV(const char* filepath);

    // Load and decode original MP3 from file (RomFS or SD)
    bool loadMP3(const char* filepath);

    // Create from raw PCM16 samples (allocates in 3DS linear memory)
    bool createFromPCM16(const int16_t* samples, uint32_t sampleCount, int channels, int sampleRate);

    // Synthesize a tone (e.g. square/sine wave sound effect)
    bool synthesizeTone(float frequency, float duration, float volume = 0.5f, int sampleRate = 22050);

    // Play sound sample on an available NDSP channel
    // Volume: 0.0 to 1.0, Pan: -1.0 (left) to 1.0 (right), Pitch: 1.0 = normal
    int play(float volume = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool loop = false);

    void freeSample();

    bool isLoaded() const { return m_data != nullptr; }
    int16_t* data() const { return m_data; }
    int channels() const { return m_channels; }
    int sampleRate() const { return m_sampleRate; }
    uint32_t sampleCount() const { return m_sampleCount; }

private:
    int16_t* m_data = nullptr; // Allocated with linearAlloc
    uint32_t m_sampleCount = 0;
    int m_channels = 1;
    int m_sampleRate = 22050;
    uint32_t m_dataSize = 0;
};

class Audio {
public:
    static bool init();
    static void exit();
    static bool isInitialized() { return s_initialized; }

    // Sound effect playback helper
    static int play(SoundSample* sample, float volume = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool loop = false);
    static int playStereo(SoundSample* sample, float leftVol, float rightVol, float pitch = 1.0f, bool loop = false);

    // Stop all audio playback
    static void stopAll();

    // Cache
    static std::shared_ptr<SoundSample> getSample(const std::string& key);
    static void clearCache();

    // Sound FX and Music toggles
    static void setSoundFxEnabled(bool enabled);
    static bool isSoundFxEnabled() { return s_soundFxEnabled; }
    static void setMusicEnabled(bool enabled);
    static bool isMusicEnabled() { return s_musicEnabled; }

    // Allocate an NDSP channel (0..NUM_CHANNELS-1)
    static int allocateChannel();

private:
    static constexpr int NUM_CHANNELS = 16;
    static bool s_initialized;
    static bool s_soundFxEnabled;
    static bool s_musicEnabled;
    static ndspWaveBuf s_waveBufs[NUM_CHANNELS];
    static std::unordered_map<std::string, std::shared_ptr<SoundSample>> s_sampleCache;
};

} // namespace noosa
