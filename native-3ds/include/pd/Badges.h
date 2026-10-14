#pragma once

#include <string>

namespace pd {

class Badges {
public:
    enum class Badge {
        MONSTERS_SLAIN_1 = 0, // 10 enemies slain
        MONSTERS_SLAIN_2 = 1, // 50 enemies slain
        MONSTERS_SLAIN_3 = 2, // 150 enemies slain
        MONSTERS_SLAIN_4 = 3, // 250 enemies slain
        GOLD_COLLECTED_1 = 4, // 100 gold collected
        GOLD_COLLECTED_2 = 5, // 500 gold collected
        GOLD_COLLECTED_3 = 6, // 2500 gold collected
        GOLD_COLLECTED_4 = 7, // 7500 gold collected
        LEVEL_REACHED_1  = 8, // Level 6 reached
        LEVEL_REACHED_2  = 9, // Level 12 reached
        LEVEL_REACHED_3  = 10, // Level 18 reached
        LEVEL_REACHED_4  = 11, // Level 24 reached
        BOSS_SLAIN_1     = 12, // 1st boss slain (Goo)
        BOSS_SLAIN_2     = 13, // 2nd boss slain (Tengu)
        DEATH_FROM_FIRE  = 14,
        DEATH_FROM_POISON= 15,
        DEATH_FROM_GAS   = 16,
        DEATH_FROM_HUNGER= 17,
        DEATH_FROM_FALLING= 18,
        ALL_ITEMS_IDENTIFIED = 19,
        COUNT
    };

    static void init();
    static void load();
    static void save();

    static bool isUnlocked(Badge badge);
    static void unlock(Badge badge, const std::string& description);

    // Validation triggers
    static void validateMonstersSlain();
    static void validateGoldCollected(int totalGold);
    static void validateLevelReached(int heroLevel);
    static void validateBossSlain(int bossIndex);
    static void validateDeathFromFalling();
    static void validateDeathFromFire();
    static void validateDeathFromPoison();
    static void validateDeathFromGas();
    static void validateDeathFromHunger();

    static int monstersSlainCount;
};

} // namespace pd

