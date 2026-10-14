#pragma once

#include "Hero.h"
#include "Level.h"
#include "Mob.h"
#include <memory>
#include <vector>

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

    static void init();
    static void newGame();
    static void resetLevel();
    static void nextLevel();
    static void prevLevel();
    static void switchLevel(int newDepth);

    static void spawnMobs(int count = 4);
    static void observe();
    static void updateVisibility() { observe(); }

    // Line of sight and hazard aware pathfinding
    static int findPath(Char* ch, int from, int to, const bool* pass, const bool* visible);
    static int flee(Char* ch, int cur, int from, const bool* pass, const bool* visible);
};

} // namespace pd
