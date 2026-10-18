#pragma once

#include "Hero.h"
#include "Level.h"
#include "Mob.h"
#include "LevelTransition.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace pd {

class FogOfWar;

class Dungeon {
public:
    static LevelTransition pendingTransition;

    static std::unique_ptr<Hero> hero;
    static std::unique_ptr<Level> level;
    static std::vector<std::unique_ptr<Mob>> mobs;
    static std::unique_ptr<FogOfWar> fog;

    static bool visible[Level::LENGTH];

    static int depth;
    static int gold;
    static int deepestFloor;
    static bool nightMode;

    static int scrollsOfUpgrade;
    static int scrollsOfEnchantment;
    static int potionOfStrength;
    static bool dewVial;

    static bool souNeeded();
    static bool soeNeeded();
    static bool posNeeded();

    static bool bossLevel(int d = -1);
    static bool shopOnLevel(int d = -1);
    static uint32_t waterColor(int d = -1);

    static std::string resultDescription;
    static void fail(const std::string& desc = "");
    static void win(const std::string& desc = "");

    static void init();
    static void newGame();
    static void resetLevel();
    static void nextLevel();
    static void prevLevel();
    static void switchLevel(int newDepth, bool isFall = false);
    static void announceLevelEntrance();

    static void spawnMobs(int count = 4);
    static std::unique_ptr<Mob> createMobForDepth(int depth);
    static void observe();
    static void updateVisibility() { observe(); }

    static void dropToChasm(std::unique_ptr<Item> item);
    static void placeChasmItems();

    // Line of sight and hazard aware pathfinding
    static int findPath(Char* ch, int from, int to, const bool* pass, const bool* visible);
    static int flee(Char* ch, int cur, int from, const bool* pass, const bool* visible);

    static std::unordered_map<int, std::vector<std::unique_ptr<Item>>> droppedItems;
};

} // namespace pd
