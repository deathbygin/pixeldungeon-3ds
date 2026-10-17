#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace pd {

class Item;
class Mob;

class Badges {
public:
    enum class Badge {
        MONSTERS_SLAIN_1 = 0,
        MONSTERS_SLAIN_2,
        MONSTERS_SLAIN_3,
        MONSTERS_SLAIN_4,
        GOLD_COLLECTED_1,
        GOLD_COLLECTED_2,
        GOLD_COLLECTED_3,
        GOLD_COLLECTED_4,
        LEVEL_REACHED_1,
        LEVEL_REACHED_2,
        LEVEL_REACHED_3,
        LEVEL_REACHED_4,
        ALL_POTIONS_IDENTIFIED,
        ALL_SCROLLS_IDENTIFIED,
        ALL_RINGS_IDENTIFIED,
        ALL_WANDS_IDENTIFIED,
        ALL_ITEMS_IDENTIFIED,
        BAG_BOUGHT_SEED_POUCH,
        BAG_BOUGHT_SCROLL_HOLDER,
        BAG_BOUGHT_WAND_HOLSTER,
        ALL_BAGS_BOUGHT,
        DEATH_FROM_FIRE,
        DEATH_FROM_POISON,
        DEATH_FROM_GAS,
        DEATH_FROM_HUNGER,
        DEATH_FROM_GLYPH,
        DEATH_FROM_FALLING,
        YASD,
        BOSS_SLAIN_1_WARRIOR,
        BOSS_SLAIN_1_MAGE,
        BOSS_SLAIN_1_ROGUE,
        BOSS_SLAIN_1_HUNTRESS,
        BOSS_SLAIN_1,
        BOSS_SLAIN_2,
        BOSS_SLAIN_3,
        BOSS_SLAIN_4,
        BOSS_SLAIN_1_ALL_CLASSES,
        BOSS_SLAIN_3_GLADIATOR,
        BOSS_SLAIN_3_BERSERKER,
        BOSS_SLAIN_3_WARLOCK,
        BOSS_SLAIN_3_BATTLEMAGE,
        BOSS_SLAIN_3_FREERUNNER,
        BOSS_SLAIN_3_ASSASSIN,
        BOSS_SLAIN_3_SNIPER,
        BOSS_SLAIN_3_WARDEN,
        BOSS_SLAIN_3_ALL_SUBCLASSES,
        RING_OF_HAGGLER,
        RING_OF_THORNS,
        STRENGTH_ATTAINED_1,
        STRENGTH_ATTAINED_2,
        STRENGTH_ATTAINED_3,
        STRENGTH_ATTAINED_4,
        FOOD_EATEN_1,
        FOOD_EATEN_2,
        FOOD_EATEN_3,
        FOOD_EATEN_4,
        MASTERY_WARRIOR,
        MASTERY_MAGE,
        MASTERY_ROGUE,
        MASTERY_HUNTRESS,
        ITEM_LEVEL_1,
        ITEM_LEVEL_2,
        ITEM_LEVEL_3,
        ITEM_LEVEL_4,
        RARE_ALBINO,
        RARE_BANDIT,
        RARE_SHIELDED,
        RARE_SENIOR,
        RARE_ACIDIC,
        RARE,
        VICTORY_WARRIOR,
        VICTORY_MAGE,
        VICTORY_ROGUE,
        VICTORY_HUNTRESS,
        VICTORY,
        VICTORY_ALL_CLASSES,
        MASTERY_COMBO,
        POTIONS_COOKED_1,
        POTIONS_COOKED_2,
        POTIONS_COOKED_3,
        POTIONS_COOKED_4,
        NO_MONSTERS_SLAIN,
        GRIM_WEAPON,
        PIRANHAS,
        NIGHT_HUNTER,
        GAMES_PLAYED_1,
        GAMES_PLAYED_2,
        GAMES_PLAYED_3,
        GAMES_PLAYED_4,
        HAPPY_END,
        CHAMPION,
        SUPPORTER,
        COUNT
    };

    struct BadgeInfo {
        Badge badge;
        const char* title;
        const char* description;
        int image; // 0..63 in badges.png, or -1 if internal tracking badge
        bool meta; // true if super badge
    };

    static void init();
    static void loadGlobal();
    static void saveGlobal();
    static void loadLocal();
    static void saveLocal();
    static void reset();

    // Compatibility wrappers
    static void load() { loadGlobal(); }
    static void save() { saveGlobal(); }

    static bool isUnlocked(Badge badge);
    static bool isUnlockedLocal(Badge badge);
    static void unlock(Badge badge);
    static void unlock(Badge badge, const std::string& description);
    static void displayBadge(Badge badge);
    static void disown(Badge badge);

    static const BadgeInfo& getInfo(Badge badge);
    static std::vector<Badge> filtered(bool global);
    static const std::vector<BadgeInfo>& getCatalog();

    // In-game Validation Triggers
    static void validateMonstersSlain();
    static void validateGoldCollected(int totalGold = -1);
    static void validateLevelReached(int heroLevel = -1);
    static void validateStrengthAttained();
    static void validateFoodEaten();
    static void validatePotionsCooked();
    static void validatePiranhasKilled();
    static void validateItemLevelAcquired(Item* item);
    static void validateAllPotionsIdentified();
    static void validateAllScrollsIdentified();
    static void validateAllRingsIdentified();
    static void validateAllWandsIdentified();
    static void validateAllItemsIdentified();
    static void validateAllBagsBought(Item* bag);
    static void validateDeathFromFire();
    static void validateDeathFromPoison();
    static void validateDeathFromGas();
    static void validateDeathFromHunger();
    static void validateDeathFromGlyph();
    static void validateDeathFromFalling();
    static void validateBossSlain(int bossIndex = -1);
    static void validateMastery();
    static void validateMasteryCombo(int count);
    static void validateRingOfHaggler();
    static void validateRingOfThorns();
    static void validateRare(Mob* mob);
    static void validateVictory();
    static void validateNoKilling();
    static void validateGrimWeapon();
    static void validateNightHunter();
    static void validateGamesPlayed();
    static void validateHappyEnd();
    static void validateChampion();
    static void validateSupporter();

    static int monstersSlainCount;
};

} // namespace pd
