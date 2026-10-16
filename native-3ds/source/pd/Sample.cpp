#include "pd/Sample.h"
#include "pd/Dungeon.h"
#include "pd/Level.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace pd {

Sample Sample::INSTANCE;

static std::unordered_map<std::string, int> s_soundFrameCounts;
static float s_frameTimer = 0.0f;
static constexpr int MAX_CONCURRENT_PER_FRAME = 2;

void Sample::update(float dt) {
    s_frameTimer += dt;
    if (s_frameTimer >= 0.035f) { // ~30 Hz frame reset window
        s_frameTimer = 0.0f;
        s_soundFrameCounts.clear();
    }
}

static bool shouldPlay(const std::string& sound) {
    int& count = s_soundFrameCounts[sound];
    if (count >= MAX_CONCURRENT_PER_FRAME) {
        return false;
    }
    count++;
    return true;
}

int Sample::play(const std::string& sound, float leftVol, float rightVol, float pitch) {
    if (!shouldPlay(sound)) return -1;
    auto sample = noosa::Audio::getSample(sound);
    if (!sample || !sample->isLoaded()) return -1;
    return noosa::Audio::playStereo(sample.get(), leftVol, rightVol, pitch, false);
}

int Sample::play(const std::string& sound, float volume, float pitch) {
    if (!shouldPlay(sound)) return -1;
    auto sample = noosa::Audio::getSample(sound);
    if (!sample || !sample->isLoaded()) return -1;
    return noosa::Audio::play(sample.get(), volume, 0.0f, pitch, false);
}

int Sample::play(const std::string& sound, float volume) {
    return play(sound, volume, 1.0f);
}

int Sample::play(const std::string& sound) {
    return play(sound, 1.0f, 1.0f);
}

int Sample::playAt(const std::string& sound, int cell, float baseVolume, float pitch) {
    if (cell < 0 || cell >= Level::LENGTH) {
        return play(sound, baseVolume, pitch);
    }

    if (!Dungeon::hero) {
        return play(sound, baseVolume, pitch);
    }

    int heroPos = Dungeon::hero->pos;
    int hx = heroPos % Level::WIDTH;
    int hy = heroPos / Level::WIDTH;
    int cx = cell % Level::WIDTH;
    int cy = cell / Level::WIDTH;

    float dx = static_cast<float>(cx - hx);
    float dy = static_cast<float>(cy - hy);
    float dist = std::sqrt(dx * dx + dy * dy);

    // Drop sound if distant (>18 tiles) and not in direct line of sight
    if (dist > 18.0f && !Dungeon::visible[cell]) {
        return -1;
    }

    // Distance attenuation:
    // 0..3 tiles: full volume (1.0)
    // 3..16 tiles: smooth linear falloff to 0.08
    float distFactor = 1.0f;
    if (dist > 3.0f) {
        distFactor = std::clamp(1.0f - (dist - 3.0f) / 14.0f, 0.08f, 1.0f);
    }

    // Wall muffling: if event is outside hero's visible FOV, muffle by 40%
    if (!Dungeon::visible[cell]) {
        distFactor *= 0.60f;
    }

    float finalVol = baseVolume * distFactor;
    if (finalVol < 0.03f) return -1;

    // Stereo panning based on horizontal tile offset
    float pan = std::clamp(dx / 10.0f, -0.85f, 0.85f);

    if (!shouldPlay(sound)) return -1;
    auto sample = noosa::Audio::getSample(sound);
    if (!sample || !sample->isLoaded()) return -1;

    return noosa::Audio::play(sample.get(), finalVol, pan, pitch, false);
}

} // namespace pd

