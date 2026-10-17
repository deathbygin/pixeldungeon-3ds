#include "pd/Badges.h"
#include "pd/BadgeBanner.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/Statistics.h"
#include "pd/Dungeon.h"
#include "pd/SaveManager.h"
#include "pd/Rankings.h"
#include "pd/Item.h"
#include "pd/Mob.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace pd {

static std::unordered_set<int> s_globalBadges;
static std::unordered_set<int> s_localBadges;
static bool s_saveNeeded = false;
int Badges::monstersSlainCount = 0;

static const Badges::BadgeInfo s_badgeInfoTable[static_cast<size_t>(Badges::Badge::COUNT)] = {
    { Badges::Badge::MONSTERS_SLAIN_1, "Monster Hunter I", "10 enemies slain", 0, false },
    { Badges::Badge::MONSTERS_SLAIN_2, "Monster Hunter II", "50 enemies slain", 1, false },
    { Badges::Badge::MONSTERS_SLAIN_3, "Monster Hunter III", "150 enemies slain", 2, false },
    { Badges::Badge::MONSTERS_SLAIN_4, "Monster Slayer", "250 enemies slain", 3, false },
    { Badges::Badge::GOLD_COLLECTED_1, "Treasure Seeker I", "100 gold collected", 4, false },
    { Badges::Badge::GOLD_COLLECTED_2, "Treasure Seeker II", "500 gold collected", 5, false },
    { Badges::Badge::GOLD_COLLECTED_3, "Treasure Seeker III", "2500 gold collected", 6, false },
    { Badges::Badge::GOLD_COLLECTED_4, "Treasure Hoarder", "7500 gold collected", 7, false },
    { Badges::Badge::LEVEL_REACHED_1, "Adventurer I", "Level 6 reached", 8, false },
    { Badges::Badge::LEVEL_REACHED_2, "Adventurer II", "Level 12 reached", 9, false },
    { Badges::Badge::LEVEL_REACHED_3, "Adventurer III", "Level 18 reached", 10, false },
    { Badges::Badge::LEVEL_REACHED_4, "Master Adventurer", "Level 24 reached", 11, false },
    { Badges::Badge::ALL_POTIONS_IDENTIFIED, "Alchemist", "All potions identified", 16, false },
    { Badges::Badge::ALL_SCROLLS_IDENTIFIED, "Scholar", "All scrolls identified", 17, false },
    { Badges::Badge::ALL_RINGS_IDENTIFIED, "Jeweler", "All rings identified", 18, false },
    { Badges::Badge::ALL_WANDS_IDENTIFIED, "Sorcerer", "All wands identified", 19, false },
    { Badges::Badge::ALL_ITEMS_IDENTIFIED, "Master of Lore", "All potions, scrolls, rings & wands identified", 35, true },
    { Badges::Badge::BAG_BOUGHT_SEED_POUCH, "", "", -1, false },
    { Badges::Badge::BAG_BOUGHT_SCROLL_HOLDER, "", "", -1, false },
    { Badges::Badge::BAG_BOUGHT_WAND_HOLSTER, "", "", -1, false },
    { Badges::Badge::ALL_BAGS_BOUGHT, "Deep Pockets", "All bags bought", 23, false },
    { Badges::Badge::DEATH_FROM_FIRE, "Cremation", "Death from fire", 24, false },
    { Badges::Badge::DEATH_FROM_POISON, "Envenomed", "Death from poison", 25, false },
    { Badges::Badge::DEATH_FROM_GAS, "Asphyxiation", "Death from toxic gas", 26, false },
    { Badges::Badge::DEATH_FROM_HUNGER, "Starvation", "Death from hunger", 27, false },
    { Badges::Badge::DEATH_FROM_GLYPH, "Fatal Inscription", "Death from an enchantment", 57, false },
    { Badges::Badge::DEATH_FROM_FALLING, "Terminal Velocity", "Death from falling down", 59, false },
    { Badges::Badge::YASD, "YASD", "Death from fire, poison, toxic gas & hunger", 34, true },
    { Badges::Badge::BOSS_SLAIN_1_WARRIOR, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_1_MAGE, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_1_ROGUE, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_1_HUNTRESS, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_1, "Goo Vanquished", "1st boss slain", 12, false },
    { Badges::Badge::BOSS_SLAIN_2, "Tengu Vanquished", "2nd boss slain", 13, false },
    { Badges::Badge::BOSS_SLAIN_3, "DM-300 Vanquished", "3rd boss slain", 14, false },
    { Badges::Badge::BOSS_SLAIN_4, "King Vanquished", "4th boss slain", 15, false },
    { Badges::Badge::BOSS_SLAIN_1_ALL_CLASSES, "Sewer Champion", "1st boss slain by Warrior, Mage, Rogue & Huntress", 32, true },
    { Badges::Badge::BOSS_SLAIN_3_GLADIATOR, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_BERSERKER, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_WARLOCK, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_BATTLEMAGE, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_FREERUNNER, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_ASSASSIN, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_SNIPER, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_WARDEN, "", "", -1, false },
    { Badges::Badge::BOSS_SLAIN_3_ALL_SUBCLASSES, "Subclass Virtuoso", "3rd boss slain by Gladiator, Berserker, Warlock, Battlemage, Freerunner, Assassin, Sniper & Warden", 33, true },
    { Badges::Badge::RING_OF_HAGGLER, "Haggler's Friend", "Ring of Haggler obtained", 20, false },
    { Badges::Badge::RING_OF_THORNS, "Thorns of Retaliation", "Ring of Thorns obtained", 21, false },
    { Badges::Badge::STRENGTH_ATTAINED_1, "Mighty I", "13 points of Strength attained", 40, false },
    { Badges::Badge::STRENGTH_ATTAINED_2, "Mighty II", "15 points of Strength attained", 41, false },
    { Badges::Badge::STRENGTH_ATTAINED_3, "Mighty III", "17 points of Strength attained", 42, false },
    { Badges::Badge::STRENGTH_ATTAINED_4, "Colossus", "19 points of Strength attained", 43, false },
    { Badges::Badge::FOOD_EATEN_1, "Hearty Eater I", "10 pieces of food eaten", 44, false },
    { Badges::Badge::FOOD_EATEN_2, "Hearty Eater II", "20 pieces of food eaten", 45, false },
    { Badges::Badge::FOOD_EATEN_3, "Hearty Eater III", "30 pieces of food eaten", 46, false },
    { Badges::Badge::FOOD_EATEN_4, "Gourmand", "40 pieces of food eaten", 47, false },
    { Badges::Badge::MASTERY_WARRIOR, "", "", -1, false },
    { Badges::Badge::MASTERY_MAGE, "", "", -1, false },
    { Badges::Badge::MASTERY_ROGUE, "", "", -1, false },
    { Badges::Badge::MASTERY_HUNTRESS, "", "", -1, false },
    { Badges::Badge::ITEM_LEVEL_1, "Reinforced", "Item of level 3 acquired", 48, false },
    { Badges::Badge::ITEM_LEVEL_2, "Fortified", "Item of level 6 acquired", 49, false },
    { Badges::Badge::ITEM_LEVEL_3, "Masterwork", "Item of level 9 acquired", 50, false },
    { Badges::Badge::ITEM_LEVEL_4, "Legendary", "Item of level 12 acquired", 51, false },
    { Badges::Badge::RARE_ALBINO, "", "", -1, false },
    { Badges::Badge::RARE_BANDIT, "", "", -1, false },
    { Badges::Badge::RARE_SHIELDED, "", "", -1, false },
    { Badges::Badge::RARE_SENIOR, "", "", -1, false },
    { Badges::Badge::RARE_ACIDIC, "", "", -1, false },
    { Badges::Badge::RARE, "Cryptid Hunter", "All rare monsters slain", 37, true },
    { Badges::Badge::VICTORY_WARRIOR, "", "", -1, false },
    { Badges::Badge::VICTORY_MAGE, "", "", -1, false },
    { Badges::Badge::VICTORY_ROGUE, "", "", -1, false },
    { Badges::Badge::VICTORY_HUNTRESS, "", "", -1, false },
    { Badges::Badge::VICTORY, "Dungeon Conqueror", "Amulet of Yendor obtained", 22, false },
    { Badges::Badge::VICTORY_ALL_CLASSES, "Grand Champion", "Amulet of Yendor obtained by Warrior, Mage, Rogue & Huntress", 36, true },
    { Badges::Badge::MASTERY_COMBO, "Combo Master", "7-hit combo", 56, false },
    { Badges::Badge::POTIONS_COOKED_1, "Brewer I", "3 potions cooked", 52, false },
    { Badges::Badge::POTIONS_COOKED_2, "Brewer II", "6 potions cooked", 53, false },
    { Badges::Badge::POTIONS_COOKED_3, "Brewer III", "9 potions cooked", 54, false },
    { Badges::Badge::POTIONS_COOKED_4, "Master Brewer", "12 potions cooked", 55, false },
    { Badges::Badge::NO_MONSTERS_SLAIN, "Pacifist", "Level completed without killing any monsters", 28, false },
    { Badges::Badge::GRIM_WEAPON, "Grim Reaper", "Monster killed by a Grim weapon", 29, false },
    { Badges::Badge::PIRANHAS, "Fisherman", "6 piranhas killed", 30, false },
    { Badges::Badge::NIGHT_HUNTER, "Night Hunter", "15 monsters killed at nighttime", 58, false },
    { Badges::Badge::GAMES_PLAYED_1, "Persistent I", "10 games played", 60, true },
    { Badges::Badge::GAMES_PLAYED_2, "Persistent II", "100 games played", 61, true },
    { Badges::Badge::GAMES_PLAYED_3, "Persistent III", "500 games played", 62, true },
    { Badges::Badge::GAMES_PLAYED_4, "Dedicated", "2000 games played", 63, true },
    { Badges::Badge::HAPPY_END, "Happy End", "Happy end", 38, false },
    { Badges::Badge::CHAMPION, "Champion", "Challenge won", 39, true },
    { Badges::Badge::SUPPORTER, "Supporter", "Thanks for your support!", 31, true }
};

const Badges::BadgeInfo& Badges::getInfo(Badge badge) {
    size_t idx = static_cast<size_t>(badge);
    if (idx < static_cast<size_t>(Badge::COUNT)) {
        return s_badgeInfoTable[idx];
    }
    static BadgeInfo fallback = { Badge::COUNT, "Unknown", "Unknown badge", -1, false };
    return fallback;
}

static std::string getBadgesPath() {
    return SaveManager::getSaveDirectory() + "badges.dat";
}

void Badges::init() {
    loadGlobal();
}

void Badges::loadGlobal() {
    s_globalBadges.clear();

    std::string path = getBadgesPath();
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        fp = std::fopen("badges.dat", "rb");
    }
    if (!fp) {
        fp = std::fopen("sdmc:/3ds/PixelDungeon/badges.dat", "rb");
    }
    if (!fp) return;

    char magic[4] = { 0 };
    if (std::fread(magic, 1, 4, fp) == 4 && std::memcmp(magic, "PDBG", 4) == 0) {
        uint32_t version = 0;
        std::fread(&version, sizeof(uint32_t), 1, fp);
        uint32_t count = 0;
        std::fread(&count, sizeof(uint32_t), 1, fp);
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t bId = 0;
            if (std::fread(&bId, sizeof(uint32_t), 1, fp) == 1) {
                if (bId < static_cast<uint32_t>(Badge::COUNT)) {
                    s_globalBadges.insert(static_cast<int>(bId));
                }
            }
        }
        std::fread(&monstersSlainCount, sizeof(int), 1, fp);
    } else {
        // Fallback for legacy format (uint32 count + raw byte array)
        std::fseek(fp, 0, SEEK_SET);
        uint32_t count = 0;
        if (std::fread(&count, sizeof(uint32_t), 1, fp) == 1) {
            size_t toRead = std::min(static_cast<size_t>(count), static_cast<size_t>(Badge::COUNT));
            std::vector<uint8_t> buf(toRead, 0);
            std::fread(buf.data(), sizeof(uint8_t), toRead, fp);
            for (size_t i = 0; i < toRead; ++i) {
                if (buf[i]) s_globalBadges.insert(static_cast<int>(i));
            }
            std::fread(&monstersSlainCount, sizeof(int), 1, fp);
        }
    }

    std::fclose(fp);
    s_saveNeeded = false;
}

void Badges::saveGlobal() {
    std::string path = getBadgesPath();
    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp) return;

    const char magic[4] = { 'P', 'D', 'B', 'G' };
    std::fwrite(magic, 1, 4, fp);

    uint32_t version = 1;
    std::fwrite(&version, sizeof(uint32_t), 1, fp);

    uint32_t count = static_cast<uint32_t>(s_globalBadges.size());
    std::fwrite(&count, sizeof(uint32_t), 1, fp);

    for (int bId : s_globalBadges) {
        uint32_t val = static_cast<uint32_t>(bId);
        std::fwrite(&val, sizeof(uint32_t), 1, fp);
    }

    std::fwrite(&monstersSlainCount, sizeof(int), 1, fp);
    std::fclose(fp);
    s_saveNeeded = false;
}

void Badges::loadLocal() {
    s_localBadges.clear();
}

void Badges::saveLocal() {
    // Local run badges preserved in game save bundle if needed
}

void Badges::reset() {
    s_globalBadges.clear();
    s_localBadges.clear();
    monstersSlainCount = 0;
    std::remove(getBadgesPath().c_str());
    std::remove("badges.dat");
    std::remove("sdmc:/3ds/PixelDungeon/badges.dat");
}

bool Badges::isUnlocked(Badge badge) {
    return s_globalBadges.find(static_cast<int>(badge)) != s_globalBadges.end();
}

bool Badges::isUnlockedLocal(Badge badge) {
    return s_localBadges.find(static_cast<int>(badge)) != s_localBadges.end();
}

void Badges::unlock(Badge badge) {
    displayBadge(badge);
}

void Badges::unlock(Badge badge, const std::string& description) {
    (void)description;
    displayBadge(badge);
}

void Badges::disown(Badge badge) {
    s_globalBadges.erase(static_cast<int>(badge));
    saveGlobal();
}

void Badges::displayBadge(Badge badge) {
    if (badge >= Badge::COUNT) return;

    int bId = static_cast<int>(badge);
    const auto& info = getInfo(badge);

    if (s_globalBadges.find(bId) != s_globalBadges.end()) {
        if (!info.meta && info.image >= 0) {
            GLog::h(std::string("Badge endorsed: ") + info.description);
        }
    } else {
        s_globalBadges.insert(bId);
        saveGlobal();

        if (info.image >= 0) {
            if (info.meta) {
                GLog::h(std::string("New super badge: ") + info.description + "!");
            } else {
                GLog::h(std::string("New badge unlocked: ") + info.description + "!");
            }
            BadgeBanner::show(badge);
        }
    }
}

static void leaveBest(std::unordered_set<int>& list, const std::vector<Badges::Badge>& badges) {
    for (int i = static_cast<int>(badges.size()) - 1; i > 0; --i) {
        int targetId = static_cast<int>(badges[i]);
        if (list.find(targetId) != list.end()) {
            for (int j = 0; j < i; ++j) {
                list.erase(static_cast<int>(badges[j]));
            }
            break;
        }
    }
}

std::vector<Badges::Badge> Badges::filtered(bool global) {
    std::unordered_set<int> filteredSet = global ? s_globalBadges : s_localBadges;

    // Remove internal tracking and non-image badges
    for (auto it = filteredSet.begin(); it != filteredSet.end(); ) {
        const auto& info = getInfo(static_cast<Badge>(*it));
        if ((!global && info.meta) || info.image == -1) {
            it = filteredSet.erase(it);
        } else {
            ++it;
        }
    }

    leaveBest(filteredSet, { Badge::MONSTERS_SLAIN_1, Badge::MONSTERS_SLAIN_2, Badge::MONSTERS_SLAIN_3, Badge::MONSTERS_SLAIN_4 });
    leaveBest(filteredSet, { Badge::GOLD_COLLECTED_1, Badge::GOLD_COLLECTED_2, Badge::GOLD_COLLECTED_3, Badge::GOLD_COLLECTED_4 });
    leaveBest(filteredSet, { Badge::BOSS_SLAIN_1, Badge::BOSS_SLAIN_2, Badge::BOSS_SLAIN_3, Badge::BOSS_SLAIN_4 });
    leaveBest(filteredSet, { Badge::LEVEL_REACHED_1, Badge::LEVEL_REACHED_2, Badge::LEVEL_REACHED_3, Badge::LEVEL_REACHED_4 });
    leaveBest(filteredSet, { Badge::STRENGTH_ATTAINED_1, Badge::STRENGTH_ATTAINED_2, Badge::STRENGTH_ATTAINED_3, Badge::STRENGTH_ATTAINED_4 });
    leaveBest(filteredSet, { Badge::FOOD_EATEN_1, Badge::FOOD_EATEN_2, Badge::FOOD_EATEN_3, Badge::FOOD_EATEN_4 });
    leaveBest(filteredSet, { Badge::ITEM_LEVEL_1, Badge::ITEM_LEVEL_2, Badge::ITEM_LEVEL_3, Badge::ITEM_LEVEL_4 });
    leaveBest(filteredSet, { Badge::POTIONS_COOKED_1, Badge::POTIONS_COOKED_2, Badge::POTIONS_COOKED_3, Badge::POTIONS_COOKED_4 });
    leaveBest(filteredSet, { Badge::BOSS_SLAIN_1_ALL_CLASSES, Badge::BOSS_SLAIN_3_ALL_SUBCLASSES });
    leaveBest(filteredSet, { Badge::DEATH_FROM_FIRE, Badge::YASD });
    leaveBest(filteredSet, { Badge::DEATH_FROM_GAS, Badge::YASD });
    leaveBest(filteredSet, { Badge::DEATH_FROM_HUNGER, Badge::YASD });
    leaveBest(filteredSet, { Badge::DEATH_FROM_POISON, Badge::YASD });
    leaveBest(filteredSet, { Badge::ALL_POTIONS_IDENTIFIED, Badge::ALL_ITEMS_IDENTIFIED });
    leaveBest(filteredSet, { Badge::ALL_SCROLLS_IDENTIFIED, Badge::ALL_ITEMS_IDENTIFIED });
    leaveBest(filteredSet, { Badge::ALL_RINGS_IDENTIFIED, Badge::ALL_ITEMS_IDENTIFIED });
    leaveBest(filteredSet, { Badge::ALL_WANDS_IDENTIFIED, Badge::ALL_ITEMS_IDENTIFIED });
    leaveBest(filteredSet, { Badge::VICTORY, Badge::VICTORY_ALL_CLASSES });
    leaveBest(filteredSet, { Badge::VICTORY, Badge::HAPPY_END });
    leaveBest(filteredSet, { Badge::VICTORY, Badge::CHAMPION });
    leaveBest(filteredSet, { Badge::GAMES_PLAYED_1, Badge::GAMES_PLAYED_2, Badge::GAMES_PLAYED_3, Badge::GAMES_PLAYED_4 });

    std::vector<Badge> result;
    for (int bId : filteredSet) {
        result.push_back(static_cast<Badge>(bId));
    }
    std::sort(result.begin(), result.end(), [](Badge a, Badge b) {
        return static_cast<int>(a) < static_cast<int>(b);
    });
    return result;
}

const std::vector<Badges::BadgeInfo>& Badges::getCatalog() {
    static std::vector<BadgeInfo> catalog;
    if (catalog.empty()) {
        const Badge distinctTracks[] = {
            Badge::MONSTERS_SLAIN_1,
            Badge::GOLD_COLLECTED_1,
            Badge::LEVEL_REACHED_1,
            Badge::STRENGTH_ATTAINED_1,
            Badge::FOOD_EATEN_1,
            Badge::ITEM_LEVEL_1,
            Badge::POTIONS_COOKED_1,
            Badge::GAMES_PLAYED_1,
            Badge::BOSS_SLAIN_1,
            Badge::BOSS_SLAIN_2,
            Badge::BOSS_SLAIN_3,
            Badge::BOSS_SLAIN_4,
            Badge::VICTORY,
            Badge::HAPPY_END,
            Badge::CHAMPION,
            Badge::BOSS_SLAIN_1_ALL_CLASSES,
            Badge::BOSS_SLAIN_3_ALL_SUBCLASSES,
            Badge::VICTORY_ALL_CLASSES,
            Badge::ALL_POTIONS_IDENTIFIED,
            Badge::ALL_SCROLLS_IDENTIFIED,
            Badge::ALL_RINGS_IDENTIFIED,
            Badge::ALL_WANDS_IDENTIFIED,
            Badge::ALL_ITEMS_IDENTIFIED,
            Badge::ALL_BAGS_BOUGHT,
            Badge::RING_OF_HAGGLER,
            Badge::RING_OF_THORNS,
            Badge::RARE,
            Badge::MASTERY_COMBO,
            Badge::NO_MONSTERS_SLAIN,
            Badge::GRIM_WEAPON,
            Badge::PIRANHAS,
            Badge::NIGHT_HUNTER,
            Badge::DEATH_FROM_FIRE,
            Badge::DEATH_FROM_POISON,
            Badge::DEATH_FROM_GAS,
            Badge::DEATH_FROM_HUNGER,
            Badge::YASD,
            Badge::DEATH_FROM_GLYPH,
            Badge::DEATH_FROM_FALLING,
            Badge::SUPPORTER
        };
        for (Badge b : distinctTracks) {
            catalog.push_back(getInfo(b));
        }
    }
    return catalog;
}

// -----------------------------------------------------------------------------
// Validation Handlers
// -----------------------------------------------------------------------------
void Badges::validateMonstersSlain() {
    monstersSlainCount++;
    Statistics::enemiesSlain = monstersSlainCount;

    Badge badge = Badge::COUNT;
    if (!isUnlockedLocal(Badge::MONSTERS_SLAIN_1) && monstersSlainCount >= 10) {
        badge = Badge::MONSTERS_SLAIN_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::MONSTERS_SLAIN_2) && monstersSlainCount >= 50) {
        badge = Badge::MONSTERS_SLAIN_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::MONSTERS_SLAIN_3) && monstersSlainCount >= 150) {
        badge = Badge::MONSTERS_SLAIN_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::MONSTERS_SLAIN_4) && monstersSlainCount >= 250) {
        badge = Badge::MONSTERS_SLAIN_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validateGoldCollected(int totalGold) {
    int g = (totalGold >= 0) ? totalGold : Statistics::goldCollected;
    Badge badge = Badge::COUNT;
    if (!isUnlockedLocal(Badge::GOLD_COLLECTED_1) && g >= 100) {
        badge = Badge::GOLD_COLLECTED_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::GOLD_COLLECTED_2) && g >= 500) {
        badge = Badge::GOLD_COLLECTED_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::GOLD_COLLECTED_3) && g >= 2500) {
        badge = Badge::GOLD_COLLECTED_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::GOLD_COLLECTED_4) && g >= 7500) {
        badge = Badge::GOLD_COLLECTED_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validateLevelReached(int heroLevel) {
    int lvl = (heroLevel >= 0) ? heroLevel : (Dungeon::hero ? Dungeon::hero->level : 1);
    Badge badge = Badge::COUNT;
    if (!isUnlockedLocal(Badge::LEVEL_REACHED_1) && lvl >= 6) {
        badge = Badge::LEVEL_REACHED_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::LEVEL_REACHED_2) && lvl >= 12) {
        badge = Badge::LEVEL_REACHED_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::LEVEL_REACHED_3) && lvl >= 18) {
        badge = Badge::LEVEL_REACHED_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::LEVEL_REACHED_4) && lvl >= 24) {
        badge = Badge::LEVEL_REACHED_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validateStrengthAttained() {
    if (!Dungeon::hero) return;
    int str = Dungeon::hero->STR;
    Badge badge = Badge::COUNT;
    if (!isUnlockedLocal(Badge::STRENGTH_ATTAINED_1) && str >= 13) {
        badge = Badge::STRENGTH_ATTAINED_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::STRENGTH_ATTAINED_2) && str >= 15) {
        badge = Badge::STRENGTH_ATTAINED_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::STRENGTH_ATTAINED_3) && str >= 17) {
        badge = Badge::STRENGTH_ATTAINED_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::STRENGTH_ATTAINED_4) && str >= 19) {
        badge = Badge::STRENGTH_ATTAINED_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validateFoodEaten() {
    Badge badge = Badge::COUNT;
    if (!isUnlockedLocal(Badge::FOOD_EATEN_1) && Statistics::foodEaten >= 10) {
        badge = Badge::FOOD_EATEN_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::FOOD_EATEN_2) && Statistics::foodEaten >= 20) {
        badge = Badge::FOOD_EATEN_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::FOOD_EATEN_3) && Statistics::foodEaten >= 30) {
        badge = Badge::FOOD_EATEN_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::FOOD_EATEN_4) && Statistics::foodEaten >= 40) {
        badge = Badge::FOOD_EATEN_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validatePotionsCooked() {
    Badge badge = Badge::COUNT;
    if (!isUnlockedLocal(Badge::POTIONS_COOKED_1) && Statistics::potionsCooked >= 3) {
        badge = Badge::POTIONS_COOKED_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::POTIONS_COOKED_2) && Statistics::potionsCooked >= 6) {
        badge = Badge::POTIONS_COOKED_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::POTIONS_COOKED_3) && Statistics::potionsCooked >= 9) {
        badge = Badge::POTIONS_COOKED_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::POTIONS_COOKED_4) && Statistics::potionsCooked >= 12) {
        badge = Badge::POTIONS_COOKED_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validatePiranhasKilled() {
    if (!isUnlockedLocal(Badge::PIRANHAS) && Statistics::piranhasKilled >= 6) {
        Badge badge = Badge::PIRANHAS;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateItemLevelAcquired(Item* item) {
    if (!item || !item->levelKnown) return;

    Badge badge = Badge::COUNT;
    int lvl = item->level;
    if (!isUnlockedLocal(Badge::ITEM_LEVEL_1) && lvl >= 3) {
        badge = Badge::ITEM_LEVEL_1;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::ITEM_LEVEL_2) && lvl >= 6) {
        badge = Badge::ITEM_LEVEL_2;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::ITEM_LEVEL_3) && lvl >= 9) {
        badge = Badge::ITEM_LEVEL_3;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (!isUnlockedLocal(Badge::ITEM_LEVEL_4) && lvl >= 12) {
        badge = Badge::ITEM_LEVEL_4;
        s_localBadges.insert(static_cast<int>(badge));
    }
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validateAllPotionsIdentified() {
    if (Dungeon::hero && Dungeon::hero->isAlive() && !isUnlockedLocal(Badge::ALL_POTIONS_IDENTIFIED)) {
        if (Potion::allKnown()) {
            Badge badge = Badge::ALL_POTIONS_IDENTIFIED;
            s_localBadges.insert(static_cast<int>(badge));
            displayBadge(badge);
            validateAllItemsIdentified();
        }
    }
}

void Badges::validateAllScrollsIdentified() {
    if (Dungeon::hero && Dungeon::hero->isAlive() && !isUnlockedLocal(Badge::ALL_SCROLLS_IDENTIFIED)) {
        if (Scroll::allKnown()) {
            Badge badge = Badge::ALL_SCROLLS_IDENTIFIED;
            s_localBadges.insert(static_cast<int>(badge));
            displayBadge(badge);
            validateAllItemsIdentified();
        }
    }
}

void Badges::validateAllRingsIdentified() {
    if (Dungeon::hero && Dungeon::hero->isAlive() && !isUnlockedLocal(Badge::ALL_RINGS_IDENTIFIED)) {
        if (Ring::allKnown()) {
            Badge badge = Badge::ALL_RINGS_IDENTIFIED;
            s_localBadges.insert(static_cast<int>(badge));
            displayBadge(badge);
            validateAllItemsIdentified();
        }
    }
}

void Badges::validateAllWandsIdentified() {
    if (Dungeon::hero && Dungeon::hero->isAlive() && !isUnlockedLocal(Badge::ALL_WANDS_IDENTIFIED)) {
        if (Wand::allKnown()) {
            Badge badge = Badge::ALL_WANDS_IDENTIFIED;
            s_localBadges.insert(static_cast<int>(badge));
            displayBadge(badge);
            validateAllItemsIdentified();
        }
    }
}

void Badges::validateAllItemsIdentified() {
    if (!isUnlocked(Badge::ALL_ITEMS_IDENTIFIED) &&
        isUnlocked(Badge::ALL_POTIONS_IDENTIFIED) &&
        isUnlocked(Badge::ALL_SCROLLS_IDENTIFIED) &&
        isUnlocked(Badge::ALL_RINGS_IDENTIFIED) &&
        isUnlocked(Badge::ALL_WANDS_IDENTIFIED)) {
        displayBadge(Badge::ALL_ITEMS_IDENTIFIED);
    }
}

void Badges::validateAllBagsBought(Item* bag) {
    if (!bag) return;
    std::string bName = bag->name;
    std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
    Badge badge = Badge::COUNT;
    if (bName.find("seed") != std::string::npos || bName.find("pouch") != std::string::npos) badge = Badge::BAG_BOUGHT_SEED_POUCH;
    else if (bName.find("scroll") != std::string::npos || bName.find("holder") != std::string::npos) badge = Badge::BAG_BOUGHT_SCROLL_HOLDER;
    else if (bName.find("wand") != std::string::npos || bName.find("holster") != std::string::npos) badge = Badge::BAG_BOUGHT_WAND_HOLSTER;

    if (badge != Badge::COUNT) {
        s_localBadges.insert(static_cast<int>(badge));
        if (!isUnlockedLocal(Badge::ALL_BAGS_BOUGHT) &&
            isUnlockedLocal(Badge::BAG_BOUGHT_SEED_POUCH) &&
            isUnlockedLocal(Badge::BAG_BOUGHT_SCROLL_HOLDER) &&
            isUnlockedLocal(Badge::BAG_BOUGHT_WAND_HOLSTER)) {
            Badge all = Badge::ALL_BAGS_BOUGHT;
            s_localBadges.insert(static_cast<int>(all));
            displayBadge(all);
        }
    }
}

static void validateYASD() {
    if (Badges::isUnlocked(Badges::Badge::DEATH_FROM_FIRE) &&
        Badges::isUnlocked(Badges::Badge::DEATH_FROM_POISON) &&
        Badges::isUnlocked(Badges::Badge::DEATH_FROM_GAS) &&
        Badges::isUnlocked(Badges::Badge::DEATH_FROM_HUNGER)) {
        Badges::displayBadge(Badges::Badge::YASD);
    }
}

void Badges::validateDeathFromFire() {
    s_localBadges.insert(static_cast<int>(Badge::DEATH_FROM_FIRE));
    displayBadge(Badge::DEATH_FROM_FIRE);
    validateYASD();
}

void Badges::validateDeathFromPoison() {
    s_localBadges.insert(static_cast<int>(Badge::DEATH_FROM_POISON));
    displayBadge(Badge::DEATH_FROM_POISON);
    validateYASD();
}

void Badges::validateDeathFromGas() {
    s_localBadges.insert(static_cast<int>(Badge::DEATH_FROM_GAS));
    displayBadge(Badge::DEATH_FROM_GAS);
    validateYASD();
}

void Badges::validateDeathFromHunger() {
    s_localBadges.insert(static_cast<int>(Badge::DEATH_FROM_HUNGER));
    displayBadge(Badge::DEATH_FROM_HUNGER);
    validateYASD();
}

void Badges::validateDeathFromGlyph() {
    s_localBadges.insert(static_cast<int>(Badge::DEATH_FROM_GLYPH));
    displayBadge(Badge::DEATH_FROM_GLYPH);
}

void Badges::validateDeathFromFalling() {
    s_localBadges.insert(static_cast<int>(Badge::DEATH_FROM_FALLING));
    displayBadge(Badge::DEATH_FROM_FALLING);
}

void Badges::validateBossSlain(int bossIndex) {
    Badge badge = Badge::COUNT;
    int idx = bossIndex;
    if (idx < 0) {
        if (Dungeon::depth == 5) idx = 1;
        else if (Dungeon::depth == 10) idx = 2;
        else if (Dungeon::depth == 15) idx = 3;
        else if (Dungeon::depth == 20) idx = 4;
        else if (Dungeon::depth == 25) idx = 5;
    }

    switch (idx) {
        case 1: badge = Badge::BOSS_SLAIN_1; break;
        case 2: badge = Badge::BOSS_SLAIN_2; break;
        case 3: badge = Badge::BOSS_SLAIN_3; break;
        case 4: badge = Badge::BOSS_SLAIN_4; break;
        default: break;
    }

    if (badge != Badge::COUNT) {
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);

        if (badge == Badge::BOSS_SLAIN_1 && Dungeon::hero) {
            Badge classBadge = Badge::COUNT;
            switch (Dungeon::hero->heroClass) {
                case Hero::HeroClass::Warrior: classBadge = Badge::BOSS_SLAIN_1_WARRIOR; break;
                case Hero::HeroClass::Mage:    classBadge = Badge::BOSS_SLAIN_1_MAGE; break;
                case Hero::HeroClass::Rogue:   classBadge = Badge::BOSS_SLAIN_1_ROGUE; break;
                case Hero::HeroClass::Huntress:classBadge = Badge::BOSS_SLAIN_1_HUNTRESS; break;
            }
            if (classBadge != Badge::COUNT) {
                s_localBadges.insert(static_cast<int>(classBadge));
                s_globalBadges.insert(static_cast<int>(classBadge));
                saveGlobal();

                if (isUnlocked(Badge::BOSS_SLAIN_1_WARRIOR) &&
                    isUnlocked(Badge::BOSS_SLAIN_1_MAGE) &&
                    isUnlocked(Badge::BOSS_SLAIN_1_ROGUE) &&
                    isUnlocked(Badge::BOSS_SLAIN_1_HUNTRESS)) {
                    displayBadge(Badge::BOSS_SLAIN_1_ALL_CLASSES);
                }
            }
        } else if (badge == Badge::BOSS_SLAIN_3 && Dungeon::hero) {
            Badge subBadge = Badge::COUNT;
            switch (Dungeon::hero->subClass) {
                case HeroSubClass::Gladiator:  subBadge = Badge::BOSS_SLAIN_3_GLADIATOR; break;
                case HeroSubClass::Berserker:  subBadge = Badge::BOSS_SLAIN_3_BERSERKER; break;
                case HeroSubClass::Warlock:    subBadge = Badge::BOSS_SLAIN_3_WARLOCK; break;
                case HeroSubClass::Battlemage: subBadge = Badge::BOSS_SLAIN_3_BATTLEMAGE; break;
                case HeroSubClass::Freerunner: subBadge = Badge::BOSS_SLAIN_3_FREERUNNER; break;
                case HeroSubClass::Assassin:   subBadge = Badge::BOSS_SLAIN_3_ASSASSIN; break;
                case HeroSubClass::Sniper:     subBadge = Badge::BOSS_SLAIN_3_SNIPER; break;
                case HeroSubClass::Warden:     subBadge = Badge::BOSS_SLAIN_3_WARDEN; break;
                default: break;
            }
            if (subBadge != Badge::COUNT) {
                s_localBadges.insert(static_cast<int>(subBadge));
                s_globalBadges.insert(static_cast<int>(subBadge));
                saveGlobal();

                if (isUnlocked(Badge::BOSS_SLAIN_3_GLADIATOR) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_BERSERKER) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_WARLOCK) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_BATTLEMAGE) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_FREERUNNER) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_ASSASSIN) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_SNIPER) &&
                    isUnlocked(Badge::BOSS_SLAIN_3_WARDEN)) {
                    displayBadge(Badge::BOSS_SLAIN_3_ALL_SUBCLASSES);
                }
            }
        }
    }
}

void Badges::validateMastery() {
    if (!Dungeon::hero) return;
    Badge badge = Badge::COUNT;
    switch (Dungeon::hero->heroClass) {
        case Hero::HeroClass::Warrior:  badge = Badge::MASTERY_WARRIOR; break;
        case Hero::HeroClass::Mage:     badge = Badge::MASTERY_MAGE; break;
        case Hero::HeroClass::Rogue:    badge = Badge::MASTERY_ROGUE; break;
        case Hero::HeroClass::Huntress: badge = Badge::MASTERY_HUNTRESS; break;
    }
    if (badge != Badge::COUNT) {
        s_globalBadges.insert(static_cast<int>(badge));
        saveGlobal();
    }
}

void Badges::validateMasteryCombo(int count) {
    if (!isUnlockedLocal(Badge::MASTERY_COMBO) && count >= 7) {
        Badge badge = Badge::MASTERY_COMBO;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateRingOfHaggler() {
    if (!isUnlockedLocal(Badge::RING_OF_HAGGLER)) {
        Badge badge = Badge::RING_OF_HAGGLER;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateRingOfThorns() {
    if (!isUnlockedLocal(Badge::RING_OF_THORNS)) {
        Badge badge = Badge::RING_OF_THORNS;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateRare(Mob* mob) {
    if (!mob) return;
    std::string mName = mob->name;
    std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
    Badge b = Badge::COUNT;
    if (mName.find("albino") != std::string::npos) b = Badge::RARE_ALBINO;
    else if (mName.find("bandit") != std::string::npos) b = Badge::RARE_BANDIT;
    else if (mName.find("shielded") != std::string::npos) b = Badge::RARE_SHIELDED;
    else if (mName.find("senior") != std::string::npos) b = Badge::RARE_SENIOR;
    else if (mName.find("acidic") != std::string::npos) b = Badge::RARE_ACIDIC;

    if (b != Badge::COUNT) {
        s_globalBadges.insert(static_cast<int>(b));
        saveGlobal();

        if (isUnlocked(Badge::RARE_ALBINO) &&
            isUnlocked(Badge::RARE_BANDIT) &&
            isUnlocked(Badge::RARE_SHIELDED) &&
            isUnlocked(Badge::RARE_SENIOR) &&
            isUnlocked(Badge::RARE_ACIDIC)) {
            displayBadge(Badge::RARE);
        }
    }
}

void Badges::validateVictory() {
    displayBadge(Badge::VICTORY);

    if (Dungeon::hero) {
        Badge classBadge = Badge::COUNT;
        switch (Dungeon::hero->heroClass) {
            case Hero::HeroClass::Warrior:  classBadge = Badge::VICTORY_WARRIOR; break;
            case Hero::HeroClass::Mage:     classBadge = Badge::VICTORY_MAGE; break;
            case Hero::HeroClass::Rogue:    classBadge = Badge::VICTORY_ROGUE; break;
            case Hero::HeroClass::Huntress: classBadge = Badge::VICTORY_HUNTRESS; break;
        }
        if (classBadge != Badge::COUNT) {
            s_localBadges.insert(static_cast<int>(classBadge));
            s_globalBadges.insert(static_cast<int>(classBadge));
            saveGlobal();

            if (isUnlocked(Badge::VICTORY_WARRIOR) &&
                isUnlocked(Badge::VICTORY_MAGE) &&
                isUnlocked(Badge::VICTORY_ROGUE) &&
                isUnlocked(Badge::VICTORY_HUNTRESS)) {
                displayBadge(Badge::VICTORY_ALL_CLASSES);
            }
        }
    }
}

void Badges::validateNoKilling() {
    if (!isUnlockedLocal(Badge::NO_MONSTERS_SLAIN) && Statistics::completedWithNoKilling) {
        Badge badge = Badge::NO_MONSTERS_SLAIN;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateGrimWeapon() {
    if (!isUnlockedLocal(Badge::GRIM_WEAPON)) {
        Badge badge = Badge::GRIM_WEAPON;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateNightHunter() {
    if (!isUnlockedLocal(Badge::NIGHT_HUNTER) && Statistics::nightHunt >= 15) {
        Badge badge = Badge::NIGHT_HUNTER;
        s_localBadges.insert(static_cast<int>(badge));
        displayBadge(badge);
    }
}

void Badges::validateGamesPlayed() {
    int total = static_cast<int>(Rankings::instance().records.size());
    Badge badge = Badge::COUNT;
    if (total >= 10)   badge = Badge::GAMES_PLAYED_1;
    if (total >= 100)  badge = Badge::GAMES_PLAYED_2;
    if (total >= 500)  badge = Badge::GAMES_PLAYED_3;
    if (total >= 2000) badge = Badge::GAMES_PLAYED_4;
    if (badge != Badge::COUNT) displayBadge(badge);
}

void Badges::validateHappyEnd() {
    displayBadge(Badge::HAPPY_END);
}

void Badges::validateChampion() {
    displayBadge(Badge::CHAMPION);
}

void Badges::validateSupporter() {
    s_globalBadges.insert(static_cast<int>(Badge::SUPPORTER));
    saveGlobal();
    displayBadge(Badge::SUPPORTER);
}

} // namespace pd
