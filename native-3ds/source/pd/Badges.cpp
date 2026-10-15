#include "pd/Badges.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "noosa/Audio.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace pd {

static bool s_unlocked[static_cast<size_t>(Badges::Badge::COUNT)] = { false };
int Badges::monstersSlainCount = 0;

static const char* getBadgesFilePath() {
#ifdef __3DS__
    return "sdmc:/3ds/PixelDungeon/badges.dat";
#else
    return "badges.dat";
#endif
}

void Badges::init() {
    load();
}

void Badges::load() {
    FILE* fp = std::fopen(getBadgesFilePath(), "rb");
    if (!fp) return;

    uint32_t count = 0;
    if (std::fread(&count, sizeof(uint32_t), 1, fp) == 1) {
        size_t toRead = std::min(static_cast<size_t>(count), static_cast<size_t>(Badge::COUNT));
        uint8_t buf[static_cast<size_t>(Badge::COUNT)];
        std::fread(buf, sizeof(uint8_t), toRead, fp);
        for (size_t i = 0; i < toRead; ++i) {
            s_unlocked[i] = (buf[i] != 0);
        }
    }

    std::fread(&monstersSlainCount, sizeof(int), 1, fp);
    std::fclose(fp);
}

void Badges::save() {
    FILE* fp = std::fopen(getBadgesFilePath(), "wb");
    if (!fp) return;

    uint32_t count = static_cast<uint32_t>(Badge::COUNT);
    std::fwrite(&count, sizeof(uint32_t), 1, fp);

    uint8_t buf[static_cast<size_t>(Badge::COUNT)];
    for (size_t i = 0; i < static_cast<size_t>(Badge::COUNT); ++i) {
        buf[i] = s_unlocked[i] ? 1 : 0;
    }
    std::fwrite(buf, sizeof(uint8_t), count, fp);
    std::fwrite(&monstersSlainCount, sizeof(int), 1, fp);
    std::fclose(fp);
}

void Badges::reset() {
    for (size_t i = 0; i < static_cast<size_t>(Badge::COUNT); ++i) {
        s_unlocked[i] = false;
    }
    monstersSlainCount = 0;
    std::remove(getBadgesFilePath());
    std::remove("badges.dat");
}

bool Badges::isUnlocked(Badge badge) {
    size_t idx = static_cast<size_t>(badge);
    if (idx < static_cast<size_t>(Badge::COUNT)) {
        return s_unlocked[idx];
    }
    return false;
}

void Badges::unlock(Badge badge, const std::string& description) {
    size_t idx = static_cast<size_t>(badge);
    if (idx >= static_cast<size_t>(Badge::COUNT)) return;

    if (!s_unlocked[idx]) {
        s_unlocked[idx] = true;
        GLog::h("BADGE UNLOCKED: " + description + "!");
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_BADGE).get());
        save();
    }
}

void Badges::validateMonstersSlain() {
    monstersSlainCount++;
    if (monstersSlainCount >= 10)  unlock(Badge::MONSTERS_SLAIN_1, "10 enemies slain");
    if (monstersSlainCount >= 50)  unlock(Badge::MONSTERS_SLAIN_2, "50 enemies slain");
    if (monstersSlainCount >= 150) unlock(Badge::MONSTERS_SLAIN_3, "150 enemies slain");
    if (monstersSlainCount >= 250) unlock(Badge::MONSTERS_SLAIN_4, "250 enemies slain");
}

void Badges::validateGoldCollected(int totalGold) {
    if (totalGold >= 100)  unlock(Badge::GOLD_COLLECTED_1, "100 gold collected");
    if (totalGold >= 500)  unlock(Badge::GOLD_COLLECTED_2, "500 gold collected");
    if (totalGold >= 2500) unlock(Badge::GOLD_COLLECTED_3, "2500 gold collected");
    if (totalGold >= 7500) unlock(Badge::GOLD_COLLECTED_4, "7500 gold collected");
}

void Badges::validateLevelReached(int heroLevel) {
    if (heroLevel >= 6)  unlock(Badge::LEVEL_REACHED_1, "Level 6 reached");
    if (heroLevel >= 12) unlock(Badge::LEVEL_REACHED_2, "Level 12 reached");
    if (heroLevel >= 18) unlock(Badge::LEVEL_REACHED_3, "Level 18 reached");
    if (heroLevel >= 24) unlock(Badge::LEVEL_REACHED_4, "Level 24 reached");
}

void Badges::validateBossSlain(int bossIndex) {
    if (bossIndex == 1) unlock(Badge::BOSS_SLAIN_1, "1st boss slain (Goo)");
    else if (bossIndex == 2) unlock(Badge::BOSS_SLAIN_2, "2nd boss slain (Tengu)");
}

void Badges::validateDeathFromFalling() {
    unlock(Badge::DEATH_FROM_FALLING, "Death from falling down");
}

void Badges::validateDeathFromFire() {
    unlock(Badge::DEATH_FROM_FIRE, "Death from fire");
}

void Badges::validateDeathFromPoison() {
    unlock(Badge::DEATH_FROM_POISON, "Death from poison");
}

void Badges::validateDeathFromGas() {
    unlock(Badge::DEATH_FROM_GAS, "Death from toxic gas");
}

void Badges::validateDeathFromHunger() {
    unlock(Badge::DEATH_FROM_HUNGER, "Death from hunger");
}

} // namespace pd

