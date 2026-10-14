#pragma once

#include <cstdint>

namespace pd {

class Terrain {
public:
    // Tile IDs matching Pixel Dungeon
    static constexpr int CHASM                  = 0;
    static constexpr int EMPTY                  = 1;
    static constexpr int GRASS                  = 2;
    static constexpr int EMPTY_WELL             = 3;
    static constexpr int WALL                   = 4;
    static constexpr int DOOR                   = 5;
    static constexpr int OPEN_DOOR              = 6;
    static constexpr int ENTRANCE               = 7;
    static constexpr int EXIT                   = 8;
    static constexpr int EMBERS                 = 9;
    static constexpr int LOCKED_DOOR            = 10;
    static constexpr int PEDESTAL               = 11;
    static constexpr int WALL_DECO              = 12;
    static constexpr int BARRICADE              = 13;
    static constexpr int EMPTY_SP               = 14;
    static constexpr int HIGH_GRASS             = 15;
    static constexpr int SECRET_DOOR            = 16;
    static constexpr int TOXIC_TRAP             = 17;
    static constexpr int SECRET_TOXIC_TRAP      = 18;
    static constexpr int FIRE_TRAP              = 19;
    static constexpr int SECRET_FIRE_TRAP       = 20;
    static constexpr int PARALYTIC_TRAP         = 21;
    static constexpr int SECRET_PARALYTIC_TRAP  = 22;
    static constexpr int INACTIVE_TRAP          = 23;
    static constexpr int EMPTY_DECO             = 24;
    static constexpr int LOCKED_EXIT            = 25;
    static constexpr int UNLOCKED_EXIT          = 26;
    static constexpr int POISON_TRAP            = 27;
    static constexpr int SECRET_POISON_TRAP     = 28;
    static constexpr int SIGN                   = 29;
    static constexpr int ALARM_TRAP             = 30;
    static constexpr int SECRET_ALARM_TRAP      = 31;
    static constexpr int LIGHTNING_TRAP         = 32;
    static constexpr int SECRET_LIGHTNING_TRAP  = 33;
    static constexpr int WELL                   = 34;
    static constexpr int STATUE                 = 35;
    static constexpr int STATUE_SP              = 36;
    static constexpr int GRIPPING_TRAP          = 37;
    static constexpr int SECRET_GRIPPING_TRAP   = 38;
    static constexpr int SUMMONING_TRAP         = 39;
    static constexpr int SECRET_SUMMONING_TRAP  = 40;
    static constexpr int BOOKSHELF              = 41;
    static constexpr int ALCHEMY                = 42;
    static constexpr int CHASM_FLOOR            = 43;
    static constexpr int CHASM_FLOOR_SP         = 44;
    static constexpr int CHASM_WALL             = 45;
    static constexpr int CHASM_WATER            = 46;
    static constexpr int WATER_TILES            = 48;
    static constexpr int WATER                  = 63;

    // Bit flags
    static constexpr int PASSABLE        = 0x01;
    static constexpr int LOS_BLOCKING    = 0x02;
    static constexpr int FLAMABLE        = 0x04;
    static constexpr int SECRET          = 0x08;
    static constexpr int SOLID           = 0x10;
    static constexpr int AVOID           = 0x20;
    static constexpr int LIQUID          = 0x40;
    static constexpr int PIT             = 0x80;
    static constexpr int UNSTITCHABLE    = 0x100;

    static int flags(int tile);
    static bool isPassable(int tile)     { return (flags(tile) & PASSABLE) != 0; }
    static bool isLosBlocking(int tile)  { return (flags(tile) & LOS_BLOCKING) != 0; }
    static bool isFlamable(int tile)     { return (flags(tile) & FLAMABLE) != 0; }
    static bool isSecret(int tile)       { return (flags(tile) & SECRET) != 0; }
    static bool isSolid(int tile)        { return (flags(tile) & SOLID) != 0; }
    static bool isAvoid(int tile)        { return (flags(tile) & AVOID) != 0; }
    static bool isPit(int tile)          { return (flags(tile) & PIT) != 0; }
    static bool isUnstitchable(int tile) { return (flags(tile) & UNSTITCHABLE) != 0; }

    static int discover(int tile);
};

} // namespace pd
