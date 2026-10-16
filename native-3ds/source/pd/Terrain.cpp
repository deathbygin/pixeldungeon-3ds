#include "pd/Terrain.h"

namespace pd {

int Terrain::flags(int tile) {
    if (tile >= WATER_TILES && tile < WATER_TILES + 16) {
        return PASSABLE | LIQUID | UNSTITCHABLE;
    }

    switch (tile) {
        case EMPTY:
        case EMPTY_DECO:
        case EMBERS:
        case UNLOCKED_EXIT:
            return PASSABLE;

        case EMPTY_SP:
            return PASSABLE | UNSTITCHABLE;

        case GRASS:
            return PASSABLE | FLAMABLE;

        case HIGH_GRASS:
            return PASSABLE | LOS_BLOCKING | FLAMABLE;

        case WATER:
            return PASSABLE | LIQUID | UNSTITCHABLE;

        case DOOR:
            return PASSABLE | LOS_BLOCKING | SOLID | FLAMABLE | UNSTITCHABLE;

        case OPEN_DOOR:
            return PASSABLE | FLAMABLE | UNSTITCHABLE;

        case LOCKED_DOOR:
            return SOLID | LOS_BLOCKING | FLAMABLE | UNSTITCHABLE;

        case SECRET_DOOR:
            return SOLID | LOS_BLOCKING | SECRET | UNSTITCHABLE;

        case ENTRANCE:
        case EXIT:
            return PASSABLE;

        case WALL:
        case WALL_DECO:
            return SOLID | LOS_BLOCKING | UNSTITCHABLE;

        case STATUE:
            return SOLID;

        case STATUE_SP:
            return SOLID | UNSTITCHABLE;

        case BARRICADE:
            return SOLID | LOS_BLOCKING | FLAMABLE;

        case BOOKSHELF:
            return SOLID | LOS_BLOCKING | FLAMABLE | UNSTITCHABLE;

        case PEDESTAL:
            return PASSABLE | UNSTITCHABLE;

        case SIGN:
            return PASSABLE | FLAMABLE;

        case EMPTY_WELL:
        case ALCHEMY:
            return PASSABLE;

        case LOCKED_EXIT:
            return SOLID;

        case WELL:
            return AVOID;

        case CHASM:
        case CHASM_FLOOR:
        case CHASM_FLOOR_SP:
        case CHASM_WALL:
        case CHASM_WATER:
            return AVOID | PIT | UNSTITCHABLE;

        case TOXIC_TRAP:
        case FIRE_TRAP:
        case PARALYTIC_TRAP:
        case POISON_TRAP:
        case ALARM_TRAP:
        case LIGHTNING_TRAP:
        case GRIPPING_TRAP:
        case SUMMONING_TRAP:
        case TELEPORTATION_TRAP:
            return PASSABLE | AVOID;

        case SECRET_TOXIC_TRAP:
        case SECRET_FIRE_TRAP:
        case SECRET_PARALYTIC_TRAP:
        case SECRET_POISON_TRAP:
        case SECRET_ALARM_TRAP:
        case SECRET_LIGHTNING_TRAP:
        case SECRET_GRIPPING_TRAP:
        case SECRET_SUMMONING_TRAP:
        case SECRET_TELEPORTATION_TRAP:
            return PASSABLE | SECRET;

        case INACTIVE_TRAP:
            return PASSABLE;

        default:
            return 0;
    }
}

int Terrain::discover(int tile) {
    switch (tile) {
        case SECRET_DOOR:
            return DOOR;
        case SECRET_FIRE_TRAP:
            return FIRE_TRAP;
        case SECRET_PARALYTIC_TRAP:
            return PARALYTIC_TRAP;
        case SECRET_TOXIC_TRAP:
            return TOXIC_TRAP;
        case SECRET_POISON_TRAP:
            return POISON_TRAP;
        case SECRET_ALARM_TRAP:
            return ALARM_TRAP;
        case SECRET_LIGHTNING_TRAP:
            return LIGHTNING_TRAP;
        case SECRET_GRIPPING_TRAP:
            return GRIPPING_TRAP;
        case SECRET_SUMMONING_TRAP:
            return SUMMONING_TRAP;
        case SECRET_TELEPORTATION_TRAP:
            return TELEPORTATION_TRAP;
        default:
            return tile;
    }
}

} // namespace pd
