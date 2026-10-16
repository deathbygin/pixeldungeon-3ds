#pragma once

#include "Hero.h"
#include "Level.h"
#include "Mob.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace pd {

class FogOfWar;

class Dungeon {
public:
    static std::unique_ptr<Hero> hero;
    static std::unique_ptr<Level> level;
    static std::vector<std::unique_ptr<Mob>> mobs;
    static std::unique_ptr<FogOfWar> fog;

    static bool visible[Level::LENGTH];

    static int depth;
    static int gold;
    static int deepestFloor;
    static bool nightMode;

    static bool bossLevel(int d = -1);

    static void init();
    static void newGame();
    static void resetLevel();
    static void nextLevel();
    static void prevLevel();
    static void switchLevel(int newDepth);
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
