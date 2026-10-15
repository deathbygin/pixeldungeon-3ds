#include "pd/Preferences.h"
#include "pd/SaveManager.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"
#include <cstdio>
#include <algorithm>

namespace pd {

static constexpr uint32_t SETTINGS_MAGIC = 0x54455350; // 'PSET'
static constexpr uint32_t SETTINGS_VERSION = 1;

#pragma pack(push, 1)
struct SettingsFile {
    uint32_t magic;
    uint32_t version;
    float zoom;
    uint8_t music;
    uint8_t soundFx;
    uint8_t brightness;
    uint8_t pad;
};
#pragma pack(pop)

float Preferences::s_zoom = 2.0f;
bool Preferences::s_music = true;
bool Preferences::s_soundFx = true;
bool Preferences::s_brightness = false;

void Preferences::init() {
    std::string path = SaveManager::getSaveDirectory() + "settings.dat";
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (fp) {
        SettingsFile data{};
        size_t read = std::fread(&data, sizeof(SettingsFile), 1, fp);
        std::fclose(fp);

        if (read == 1 && data.magic == SETTINGS_MAGIC && data.version == SETTINGS_VERSION) {
            s_zoom = std::clamp(data.zoom, 1.0f, 3.0f);
            s_music = (data.music != 0);
            s_soundFx = (data.soundFx != 0);
            s_brightness = (data.brightness != 0);
        }
    }

    noosa::Audio::setSoundFxEnabled(s_soundFx);
    noosa::Audio::setMusicEnabled(s_music);

    if (noosa::Camera::main) {
        noosa::Camera::main->setZoom(s_zoom);
    }
}

void Preferences::save() {
    std::string path = SaveManager::getSaveDirectory() + "settings.dat";
    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp) return;

    SettingsFile data{};
    data.magic = SETTINGS_MAGIC;
    data.version = SETTINGS_VERSION;
    data.zoom = s_zoom;
    data.music = s_music ? 1 : 0;
    data.soundFx = s_soundFx ? 1 : 0;
    data.brightness = s_brightness ? 1 : 0;
    data.pad = 0;

    std::fwrite(&data, sizeof(SettingsFile), 1, fp);
    std::fclose(fp);
}

void Preferences::setZoom(float z) {
    s_zoom = std::clamp(z, 1.0f, 3.0f);
    if (noosa::Camera::main) {
        noosa::Camera::main->setZoom(s_zoom);
        // Re-center to update boundary clamping
        auto c = noosa::Camera::main->getCenter();
        noosa::Camera::main->setCenter(c.x, c.y);
    }
    save();
}

void Preferences::setMusic(bool m) {
    s_music = m;
    noosa::Audio::setMusicEnabled(m);
    save();
}

void Preferences::setSoundFx(bool sfx) {
    s_soundFx = sfx;
    noosa::Audio::setSoundFxEnabled(sfx);
    save();
}

void Preferences::setBrightness(bool b) {
    s_brightness = b;
    save();
}

} // namespace pd

