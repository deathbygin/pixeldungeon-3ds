#include "noosa/Audio.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace noosa {

bool Audio::s_initialized = false;
bool Audio::s_soundFxEnabled = true;
bool Audio::s_musicEnabled = true;
ndspWaveBuf Audio::s_waveBufs[Audio::NUM_CHANNELS];
std::unordered_map<std::string, std::shared_ptr<SoundSample>> Audio::s_sampleCache;

// -----------------------------------------------------------------------------
// SoundSample Implementation
// -----------------------------------------------------------------------------

SoundSample::SoundSample() = default;

SoundSample::~SoundSample() {
    freeSample();
}

SoundSample::SoundSample(SoundSample&& other) noexcept {
    m_data = other.m_data;
    m_sampleCount = other.m_sampleCount;
    m_channels = other.m_channels;
    m_sampleRate = other.m_sampleRate;
    m_dataSize = other.m_dataSize;

    other.m_data = nullptr;
    other.m_sampleCount = 0;
    other.m_dataSize = 0;
}

SoundSample& SoundSample::operator=(SoundSample&& other) noexcept {
    if (this != &other) {
        freeSample();
        m_data = other.m_data;
        m_sampleCount = other.m_sampleCount;
        m_channels = other.m_channels;
        m_sampleRate = other.m_sampleRate;
        m_dataSize = other.m_dataSize;

        other.m_data = nullptr;
        other.m_sampleCount = 0;
        other.m_dataSize = 0;
    }
    return *this;
}

void SoundSample::freeSample() {
    if (m_data) {
        linearFree(m_data);
        m_data = nullptr;
    }
    m_sampleCount = 0;
    m_dataSize = 0;
}

bool SoundSample::createFromPCM16(const int16_t* samples, uint32_t sampleCount, int channels, int sampleRate) {
    if (!samples || sampleCount == 0 || channels <= 0 || sampleRate <= 0) {
        return false;
    }

    freeSample();

    m_sampleCount = sampleCount;
    m_channels = channels;
    m_sampleRate = sampleRate;
    m_dataSize = sampleCount * channels * sizeof(int16_t);

    m_data = static_cast<int16_t*>(linearAlloc(m_dataSize));
    if (!m_data) {
        return false;
    }

    std::memcpy(m_data, samples, m_dataSize);
    DSP_FlushDataCache(m_data, m_dataSize);
    return true;
}

bool SoundSample::synthesizeTone(float frequency, float duration, float volume, int sampleRate) {
    if (frequency <= 0.0f || duration <= 0.0f || sampleRate <= 0) {
        return false;
    }

    freeSample();

    m_channels = 1;
    m_sampleRate = sampleRate;
    m_sampleCount = static_cast<uint32_t>(duration * sampleRate);
    m_dataSize = m_sampleCount * sizeof(int16_t);

    m_data = static_cast<int16_t*>(linearAlloc(m_dataSize));
    if (!m_data) {
        return false;
    }

    float vol = std::clamp(volume, 0.0f, 1.0f);
    float twoPiF = 2.0f * 3.14159265f * frequency;

    for (uint32_t i = 0; i < m_sampleCount; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        // Simple attack and decay envelope to prevent audio popping
        float envelope = 1.0f;
        float fadeTime = 0.01f;
        if (t < fadeTime) {
            envelope = t / fadeTime;
        } else if (duration - t < fadeTime) {
            envelope = (duration - t) / fadeTime;
        }

        float sample = std::sin(twoPiF * t) * vol * envelope;
        m_data[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    DSP_FlushDataCache(m_data, m_dataSize);
    return true;
}

bool SoundSample::loadWAV(const char* filepath) {
    if (!filepath) return false;

    FILE* fp = std::fopen(filepath, "rb");
    if (!fp) return false;

    char chunkId[4];
    uint32_t chunkSize = 0;

    // RIFF header
    if (std::fread(chunkId, 1, 4, fp) != 4 || std::memcmp(chunkId, "RIFF", 4) != 0) {
        std::fclose(fp);
        return false;
    }
    if (std::fread(&chunkSize, 4, 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (std::fread(chunkId, 1, 4, fp) != 4 || std::memcmp(chunkId, "WAVE", 4) != 0) {
        std::fclose(fp);
        return false;
    }

    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint8_t* rawData = nullptr;
    uint32_t dataBytes = 0;

    while (std::fread(chunkId, 1, 4, fp) == 4 && std::fread(&chunkSize, 4, 1, fp) == 1) {
        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            std::fread(&audioFormat, 2, 1, fp);
            std::fread(&numChannels, 2, 1, fp);
            std::fread(&sampleRate, 4, 1, fp);
            uint32_t byteRate;
            uint16_t blockAlign;
            std::fread(&byteRate, 4, 1, fp);
            std::fread(&blockAlign, 2, 1, fp);
            std::fread(&bitsPerSample, 2, 1, fp);
            if (chunkSize > 16) {
                std::fseek(fp, chunkSize - 16, SEEK_CUR);
            }
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataBytes = chunkSize;
            rawData = new uint8_t[dataBytes];
            std::fread(rawData, 1, dataBytes, fp);
            break;
        } else {
            std::fseek(fp, chunkSize, SEEK_CUR);
        }
    }

    std::fclose(fp);

    if (!rawData || audioFormat != 1 || (bitsPerSample != 16 && bitsPerSample != 8)) {
        delete[] rawData;
        return false;
    }

    freeSample();

    m_channels = numChannels;
    m_sampleRate = sampleRate;

    if (bitsPerSample == 16) {
        m_sampleCount = dataBytes / (numChannels * 2);
        m_dataSize = dataBytes;
        m_data = static_cast<int16_t*>(linearAlloc(m_dataSize));
        if (m_data) {
            std::memcpy(m_data, rawData, m_dataSize);
        }
    } else {
        // Convert 8-bit unsigned PCM to 16-bit signed PCM
        m_sampleCount = dataBytes / numChannels;
        m_dataSize = m_sampleCount * numChannels * sizeof(int16_t);
        m_data = static_cast<int16_t*>(linearAlloc(m_dataSize));
        if (m_data) {
            for (uint32_t i = 0; i < m_sampleCount * numChannels; ++i) {
                m_data[i] = static_cast<int16_t>((static_cast<int32_t>(rawData[i]) - 128) << 8);
            }
        }
    }

    delete[] rawData;

    if (!m_data) {
        return false;
    }

    DSP_FlushDataCache(m_data, m_dataSize);
    return true;
}

int SoundSample::play(float volume, float pan, bool loop) {
    return Audio::play(this, volume, pan, loop);
}

// -----------------------------------------------------------------------------
// Audio Subsystem Implementation
// -----------------------------------------------------------------------------

bool Audio::init() {
    if (s_initialized) return true;

    Result res = ndspInit();
    if (R_FAILED(res)) {
        return false;
    }

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    for (int i = 0; i < NUM_CHANNELS; ++i) {
        ndspChnReset(i);
        ndspChnSetInterp(i, NDSP_INTERP_LINEAR);
        std::memset(&s_waveBufs[i], 0, sizeof(ndspWaveBuf));
    }

    s_initialized = true;
    return true;
}

void Audio::exit() {
    if (!s_initialized) return;

    stopAll();
    clearCache();
    ndspExit();
    s_initialized = false;
}

void Audio::stopAll() {
    if (!s_initialized) return;

    for (int i = 0; i < NUM_CHANNELS; ++i) {
        ndspChnReset(i);
    }
}

int Audio::allocateChannel() {
    static int nextChannel = 0;

    // First search for an idle channel
    for (int i = 0; i < NUM_CHANNELS; ++i) {
        if (!ndspChnIsPlaying(i)) {
            return i;
        }
    }

    // Fallback: round-robin channel stealing
    int ch = nextChannel;
    nextChannel = (nextChannel + 1) % NUM_CHANNELS;
    ndspChnReset(ch);
    return ch;
}

void Audio::setSoundFxEnabled(bool enabled) {
    s_soundFxEnabled = enabled;
    if (!enabled) {
        stopAll();
    }
}

void Audio::setMusicEnabled(bool enabled) {
    s_musicEnabled = enabled;
}

int Audio::play(SoundSample* sample, float volume, float pan, bool loop) {
    if (!s_initialized || !s_soundFxEnabled || !sample || !sample->isLoaded()) {
        return -1;
    }

    int ch = allocateChannel();
    if (ch < 0) return -1;

    ndspChnReset(ch);
    ndspChnSetRate(ch, sample->sampleRate());
    ndspChnSetFormat(ch, sample->channels() == 1 ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);

    // Compute left and right stereo mix gains
    float clampedVol = std::clamp(volume, 0.0f, 1.0f);
    float clampedPan = std::clamp(pan, -1.0f, 1.0f);
    float leftGain = clampedVol * std::min(1.0f, 1.0f - clampedPan);
    float rightGain = clampedVol * std::min(1.0f, 1.0f + clampedPan);

    float mix[12] = { 0.0f };
    mix[0] = leftGain;
    mix[1] = rightGain;
    ndspChnSetMix(ch, mix);

    ndspWaveBuf& waveBuf = s_waveBufs[ch];
    std::memset(&waveBuf, 0, sizeof(ndspWaveBuf));
    waveBuf.data_pcm16 = sample->data();
    waveBuf.nsamples = sample->sampleCount();
    waveBuf.looping = loop;
    waveBuf.status = NDSP_WBUF_FREE;
    DSP_FlushDataCache(&waveBuf, sizeof(ndspWaveBuf));

    ndspChnWaveBufAdd(ch, &waveBuf);
    return ch;
}

std::shared_ptr<SoundSample> Audio::getSample(const std::string& key) {
    auto it = s_sampleCache.find(key);
    if (it != s_sampleCache.end()) {
        return it->second;
    }
    auto sample = std::make_shared<SoundSample>();
    s_sampleCache[key] = sample;
    return sample;
}

void Audio::clearCache() {
    s_sampleCache.clear();
}

} // namespace noosa
