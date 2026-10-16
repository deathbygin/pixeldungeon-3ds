#include "noosa/Music.h"
#include <cstring>
#include <algorithm>

namespace noosa {

Music Music::INSTANCE;

bool Music::s_initialized = false;
bool Music::s_enabled = true;
bool Music::s_isPlaying = false;
bool Music::s_isPaused = false;
bool Music::s_loop = true;
float Music::s_volume = 1.0f;
std::string Music::s_currentTrack = "";

mp3dec_ex_t Music::s_dec{};
bool Music::s_decOpen = false;

int16_t* Music::s_bufferMem[Music::BUFFER_COUNT] = { nullptr };
ndspWaveBuf Music::s_waveBufs[Music::BUFFER_COUNT]{};

void Music::init() {
    if (s_initialized) return;

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        // 16384 frames * 2 channels * 2 bytes = 65536 bytes
        uint32_t bufSize = BUFFER_SAMPLES * 2 * sizeof(int16_t);
        s_bufferMem[i] = static_cast<int16_t*>(linearAlloc(bufSize));
        std::memset(&s_waveBufs[i], 0, sizeof(ndspWaveBuf));
    }

    s_initialized = true;
}

void Music::exit() {
    if (!s_initialized) return;

    stop();

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (s_bufferMem[i]) {
            linearFree(s_bufferMem[i]);
            s_bufferMem[i] = nullptr;
        }
    }

    s_initialized = false;
}

void Music::play(const std::string& track, bool loop) {
    if (!s_initialized) {
        init();
    }

    if (track.empty()) {
        stop();
        return;
    }

    // If this exact track is already active, keep it playing without interrupting
    if (s_currentTrack == track && (s_isPlaying || s_isPaused)) {
        if (s_isPaused && s_enabled) {
            resume();
        }
        return;
    }

    stop();

    s_currentTrack = track;
    s_loop = loop;

    if (!s_enabled) {
        return;
    }

    // Open MP3 file stream
    if (mp3dec_ex_open(&s_dec, track.c_str(), MP3D_SEEK_TO_BYTE | MP3D_DO_NOT_SCAN) != 0) {
        s_currentTrack.clear();
        return;
    }
    s_decOpen = true;

    // Reset and configure NDSP channel 0
    ndspChnReset(BGM_CHANNEL);
    ndspChnSetInterp(BGM_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(BGM_CHANNEL, static_cast<float>(s_dec.info.hz > 0 ? s_dec.info.hz : 44100));
    ndspChnSetFormat(BGM_CHANNEL, s_dec.info.channels == 1 ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);

    float mix[12] = { 0.0f };
    mix[0] = s_volume;
    mix[1] = s_volume;
    ndspChnSetMix(BGM_CHANNEL, mix);

    // Prime all ring buffers
    int queued = 0;
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (fillBuffer(i)) {
            queued++;
        }
    }

    if (queued > 0) {
        s_isPlaying = true;
        s_isPaused = false;
    } else {
        stop();
    }
}

bool Music::fillBuffer(int bufIndex) {
    if (!s_decOpen || !s_bufferMem[bufIndex]) return false;

    size_t channels = s_dec.info.channels > 0 ? s_dec.info.channels : 2;
    size_t targetSamples = BUFFER_SAMPLES * channels;
    size_t samplesRead = 0;
    int16_t* dst = s_bufferMem[bufIndex];

    while (samplesRead < targetSamples) {
        size_t toRead = targetSamples - samplesRead;
        size_t r = mp3dec_ex_read(&s_dec, dst + samplesRead, toRead);
        if (r == 0) {
            if (s_loop) {
                if (mp3dec_ex_seek(&s_dec, 0) != 0) break;
                size_t r2 = mp3dec_ex_read(&s_dec, dst + samplesRead, toRead);
                if (r2 == 0) break;
                samplesRead += r2;
            } else {
                break;
            }
        } else {
            samplesRead += r;
        }
    }

    if (samplesRead == 0) {
        return false;
    }

    // Zero-fill any unread tail if track ends without loop
    if (samplesRead < targetSamples) {
        std::memset(dst + samplesRead, 0, (targetSamples - samplesRead) * sizeof(int16_t));
    }

    DSP_FlushDataCache(dst, targetSamples * sizeof(int16_t));

    ndspWaveBuf& wb = s_waveBufs[bufIndex];
    std::memset(&wb, 0, sizeof(ndspWaveBuf));
    wb.data_pcm16 = dst;
    wb.nsamples = static_cast<uint32_t>(samplesRead / channels);
    wb.looping = false; // Looping handled seamlessly by streaming decoder seek
    wb.status = NDSP_WBUF_FREE;
    DSP_FlushDataCache(&wb, sizeof(ndspWaveBuf));

    ndspChnWaveBufAdd(BGM_CHANNEL, &wb);
    return true;
}

void Music::update() {
    if (!s_initialized || !s_isPlaying || s_isPaused || !s_enabled || !s_decOpen) return;

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        if (s_waveBufs[i].status == NDSP_WBUF_DONE) {
            if (!fillBuffer(i)) {
                if (!s_loop) {
                    bool anyPlaying = false;
                    for (int j = 0; j < BUFFER_COUNT; ++j) {
                        if (s_waveBufs[j].status != NDSP_WBUF_DONE && s_waveBufs[j].status != NDSP_WBUF_FREE) {
                            anyPlaying = true;
                            break;
                        }
                    }
                    if (!anyPlaying) {
                        stop();
                    }
                }
            }
        }
    }
}

void Music::stop() {
    if (s_decOpen) {
        mp3dec_ex_close(&s_dec);
        s_decOpen = false;
    }
    ndspChnReset(BGM_CHANNEL);
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        s_waveBufs[i].status = NDSP_WBUF_FREE;
    }
    s_isPlaying = false;
    s_isPaused = false;
}

void Music::pause() {
    if (s_isPlaying && !s_isPaused) {
        ndspChnSetPaused(BGM_CHANNEL, true);
        s_isPaused = true;
    }
}

void Music::resume() {
    if (s_isPlaying && s_isPaused) {
        ndspChnSetPaused(BGM_CHANNEL, false);
        s_isPaused = false;
    }
}

void Music::volume(float vol) {
    s_volume = std::clamp(vol, 0.0f, 1.0f);
    if (s_isPlaying && !s_isPaused && s_enabled) {
        float mix[12] = { 0.0f };
        mix[0] = s_volume;
        mix[1] = s_volume;
        ndspChnSetMix(BGM_CHANNEL, mix);
    }
}

void Music::enable(bool en) {
    if (s_enabled == en) return;
    s_enabled = en;
    if (!s_enabled) {
        if (s_isPlaying) {
            ndspChnReset(BGM_CHANNEL);
        }
    } else {
        if (!s_currentTrack.empty()) {
            std::string t = s_currentTrack;
            s_currentTrack.clear();
            play(t, s_loop);
        }
    }
}

} // namespace noosa

