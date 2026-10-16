#include "pd/Level.h"
#include "pd/RegularLevel.h"
#include "pd/Biomes.h"
#include "pd/ShadowCaster.h"
#include "pd/Random.h"
#include "pd/FX.h"
#include "pd/Dungeon.h"
#include "pd/Char.h"
#include "pd/Mob.h"
#include "pd/Traps.h"
#include "pd/Chasm.h"
#include "pd/Assets.h"
#include "pd/Patch.h"
#include "pd/Item.h"
#include "noosa/Audio.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace pd {

bool Level::pitRoomNeeded = false;
bool Level::weakFloorCreated = false;

std::unique_ptr<Item> Level::itemToSpawnAsPrize() {
    if (itemsToSpawn.empty()) return nullptr;
    if (Random::Int(static_cast<int>(itemsToSpawn.size()) + 1) > 0) {
        size_t idx = Random::Int(static_cast<int>(itemsToSpawn.size()));
        auto item = std::move(itemsToSpawn[idx]);
        itemsToSpawn.erase(itemsToSpawn.begin() + idx);
        return item;
    }
    return nullptr;
}

int Level::pitCell() const {
    for (const auto& r : rooms) {
        if (r && r->type == Room::Type::PIT) {
            return r->random();
        }
    }
    return entrance;
}

Room* Level::room(int pos) const {
    for (const auto& r : rooms) {
        if (r && r->type != Room::Type::NONE && r->inside(pos)) {
            return r.get();
        }
    }
    return nullptr;
}

Level::Level() {
    std::fill_n(map, LENGTH, Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    std::fill_n(passable, LENGTH, false);
    std::fill_n(losBlocking, LENGTH, true);
    std::fill_n(pit, LENGTH, false);
    std::fill_n(secret, LENGTH, false);
    std::fill_n(avoid, LENGTH, false);
    std::fill_n(water, LENGTH, false);
}

int Level::getWaterTile(int pos) const {
    int t = Terrain::WATER_TILES;
    const int neighbours4[4] = { -WIDTH, +1, +WIDTH, -1 };
    for (int j = 0; j < 4; ++j) {
        int n = pos + neighbours4[j];
        if (n >= 0 && n < LENGTH) {
            if (Terrain::isUnstitchable(map[n])) {
                t += 1 << j;
            }
        }
    }
    return t;
}

void Level::buildFlags() {
    for (int i = 0; i < LENGTH; ++i) {
        water[i] = (map[i] == Terrain::WATER || (map[i] >= Terrain::WATER_TILES && map[i] < Terrain::WATER_TILES + 16));
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) {
            map[i] = getWaterTile(i);
        }
        passable[i] = Terrain::isPassable(map[i]);
        losBlocking[i] = Terrain::isLosBlocking(map[i]);
        pit[i] = Terrain::isPit(map[i]);
        secret[i] = Terrain::isSecret(map[i]);
        avoid[i] = Terrain::isAvoid(map[i]);
    }
}

void Level::set(int cell, int tile) {
    if (cell >= 0 && cell < LENGTH) {
        map[cell] = tile;
        passable[cell] = Terrain::isPassable(tile);
        losBlocking[cell] = Terrain::isLosBlocking(tile);
        pit[cell] = Terrain::isPit(tile);
        secret[cell] = Terrain::isSecret(tile);
        avoid[cell] = Terrain::isAvoid(tile);
        water[cell] = (tile == Terrain::WATER || (tile >= Terrain::WATER_TILES && tile < Terrain::WATER_TILES + 16));
    }
}

void Level::destroy(int cell) {
    if (cell < 0 || cell >= LENGTH) return;
    int t = map[cell];
    if (t == Terrain::DOOR || t == Terrain::SECRET_DOOR || t == Terrain::LOCKED_DOOR) {
        set(cell, Terrain::EMBERS);
        FX::burst(cell, 0xff7708FF, 6);
    } else if (t == Terrain::HIGH_GRASS || t == Terrain::GRASS) {
        set(cell, Terrain::EMBERS);
        FX::burst(cell, 0xff7708FF, 4);
    } else if (t == Terrain::BARRICADE || t == Terrain::BOOKSHELF) {
        set(cell, Terrain::EMBERS);
        FX::burst(cell, 0xff7708FF, 8);
    } else if (!Terrain::isUnstitchable(t)) {
        set(cell, Terrain::EMBERS);
    } else {
        bool flood = false;
        const int neighbours4[4] = { -WIDTH, +1, +WIDTH, -1 };
        for (int j = 0; j < 4; ++j) {
            int n = cell + neighbours4[j];
            if (n >= 0 && n < LENGTH && water[n]) {
                flood = true;
                break;
            }
        }
        if (flood) {
            set(cell, getWaterTile(cell));
        } else {
            set(cell, Terrain::EMBERS);
        }
    }
}

void Level::trample(int cell, Char* ch) {
    if (cell < 0 || cell >= LENGTH) return;
    if (map[cell] == Terrain::HIGH_GRASS) {
        set(cell, Terrain::GRASS);
        FX::burst(cell, 0x44aa33FF, 6);

        // Warden's barkskin perk
        if (ch && ch == Dungeon::hero.get() && Dungeon::hero->subClass == HeroSubClass::Warden) {
            Barkskin* b = ch->affect<Barkskin>();
            if (b) {
                b->setLevel(ch->HT / 3);
            }
            FX::burst(cell, 0x55aa22FF, 8);
        }

        // Authentic dewdrop drop / collection chance
        int dewChance = 6;
        if (ch && ch == Dungeon::hero.get() && Dungeon::hero->subClass == HeroSubClass::Warden) {
            dewChance = 3; // Much higher chance for Warden
        }
        if (Random::Int(dewChance) == 0) {
            if (ch && ch == Dungeon::hero.get()) {
                int heal = 1 + (Dungeon::depth - 1) / 5 + (Dungeon::hero->heroClass == Hero::HeroClass::Huntress ? 1 : 0);
                ch->HP = std::min(ch->HT, ch->HP + heal);
                FX::floatingText(cell, "+" + std::to_string(heal), 0x38b764FF);
            } else {
                drop(std::make_unique<Dewdrop>(), cell);
            }
        }

        // Warden seed harvest bonus
        if (ch && ch == Dungeon::hero.get() && Dungeon::hero->subClass == HeroSubClass::Warden) {
            if (Random::Int(5) == 0) {
                drop(std::make_unique<PlantSeed>(static_cast<PlantSeed::PlantType>(Random::Int(8))), cell);
            }
        }

        Dungeon::observe();
    }
}

void Level::press(int cell, Char* ch) {
    if (cell < 0 || cell >= LENGTH) return;

    // Flying / levitating characters pass harmlessly over traps and chasms
    if (ch && ch->flying) return;

    if (pit[cell] && ch == Dungeon::hero.get()) {
        Chasm::heroFall(cell);
        return;
    }

    int tile = map[cell];
    if (tile == Terrain::FIRE_TRAP || tile == Terrain::SECRET_FIRE_TRAP ||
        tile == Terrain::TOXIC_TRAP || tile == Terrain::SECRET_TOXIC_TRAP ||
        tile == Terrain::PARALYTIC_TRAP || tile == Terrain::SECRET_PARALYTIC_TRAP ||
        tile == Terrain::POISON_TRAP || tile == Terrain::SECRET_POISON_TRAP ||
        tile == Terrain::ALARM_TRAP || tile == Terrain::SECRET_ALARM_TRAP ||
        tile == Terrain::LIGHTNING_TRAP || tile == Terrain::SECRET_LIGHTNING_TRAP ||
        tile == Terrain::GRIPPING_TRAP || tile == Terrain::SECRET_GRIPPING_TRAP ||
        tile == Terrain::SUMMONING_TRAP || tile == Terrain::SECRET_SUMMONING_TRAP ||
        tile == Terrain::TELEPORTATION_TRAP || tile == Terrain::SECRET_TELEPORTATION_TRAP) {
        Traps::trigger(tile, cell, ch);
    } else if (tile == Terrain::HIGH_GRASS) {
        trample(cell, ch);
    } else if (tile == Terrain::DOOR) {
        set(cell, Terrain::OPEN_DOOR);
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_OPEN).get());
    }
}

void Level::mobPress(Mob* mob) {
    if (!mob) return;
    if (mob->flying) return; // Flying creatures pass harmlessly over traps and chasms
    int cell = mob->pos;
    if (cell < 0 || cell >= LENGTH) return;

    if (pit[cell]) {
        Chasm::mobFall(mob);
        return;
    }

    int tile = map[cell];
    if (tile == Terrain::FIRE_TRAP || tile == Terrain::TOXIC_TRAP ||
        tile == Terrain::PARALYTIC_TRAP || tile == Terrain::POISON_TRAP ||
        tile == Terrain::ALARM_TRAP || tile == Terrain::LIGHTNING_TRAP ||
        tile == Terrain::GRIPPING_TRAP || tile == Terrain::SUMMONING_TRAP ||
        tile == Terrain::TELEPORTATION_TRAP) {
        Traps::trigger(tile, cell, mob);
    } else if (tile == Terrain::HIGH_GRASS) {
        trample(cell, mob);
    } else if (tile == Terrain::DOOR) {
        set(cell, Terrain::OPEN_DOOR);
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_OPEN).get());
    }
}

Heap* Level::getHeap(int cell) const {
    for (const auto& h : heaps) {
        if (h && h->pos == cell) {
            return h.get();
        }
    }
    return nullptr;
}

Heap* Level::drop(std::unique_ptr<Item> item, int cell) {
    if (!item || cell < 0 || cell >= LENGTH) return nullptr;

    if (map[cell] == Terrain::CHASM || pit[cell]) {
        Dungeon::dropToChasm(std::move(item));
        return nullptr;
    }

    Heap* h = getHeap(cell);
    if (!h) {
        auto newHeap = std::make_unique<Heap>(cell, Heap::Type::HEAP);
        h = newHeap.get();
        heaps.push_back(std::move(newHeap));
    }
    h->drop(std::move(item));

    press(cell, nullptr);

    return h;
}

void Level::removeHeap(Heap* heap) {
    if (!heap) return;
    auto it = std::find_if(heaps.begin(), heaps.end(),
                           [heap](const std::unique_ptr<Heap>& h) { return h.get() == heap; });
    if (it != heaps.end()) {
        heaps.erase(it);
    }
}

std::unique_ptr<Level> Level::create(int depth) {
    std::unique_ptr<Level> level = nullptr;
    switch (depth) {
    case 1:
    case 2:
    case 3:
    case 4:
        level = std::make_unique<SewerLevel>();
        break;
    case 5:
        level = std::make_unique<SewerBossLevel>();
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        level = std::make_unique<PrisonLevel>();
        break;
    case 10:
        level = std::make_unique<PrisonBossLevel>();
        break;
    case 11:
    case 12:
    case 13:
    case 14:
        level = std::make_unique<CavesLevel>();
        break;
    case 15:
        level = std::make_unique<CavesBossLevel>();
        break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 21:
        level = std::make_unique<CityLevel>();
        break;
    case 20:
        level = std::make_unique<CityBossLevel>();
        break;
    case 22:
    case 23:
    case 24:
        level = std::make_unique<HallsLevel>();
        break;
    case 25:
        level = std::make_unique<HallsBossLevel>();
        break;
    case 26:
        level = std::make_unique<LastLevel>();
        break;
    default:
        level = std::make_unique<RegularLevel>();
        break;
    }

    if (level && !Dungeon::bossLevel(depth) && depth > 1 && depth != 26) {
        switch (Random::Int(10)) {
        case 0:
            if (!Dungeon::bossLevel(depth + 1)) {
                level->feeling = Level::Feeling::CHASM;
            }
            break;
        case 1:
            level->feeling = Level::Feeling::WATER;
            break;
        case 2:
            level->feeling = Level::Feeling::GRASS;
            break;
        case 3:
            level->feeling = Level::Feeling::DARK;
            break;
        default:
            level->feeling = Level::Feeling::NONE;
            break;
        }

        if (level->feeling == Level::Feeling::DARK) {
            level->viewDistance = 3;
            level->addItemToSpawn(std::make_unique<Torch>());
        }
    }

    return level;
}

void Level::cleanWalls() {
    const int neighbours9[9] = { 0, +1, -1, +WIDTH, -WIDTH, +1+WIDTH, +1-WIDTH, -1+WIDTH, -1-WIDTH };
    for (int i = 0; i < LENGTH; ++i) {
        bool d = false;
        for (int j = 0; j < 9; ++j) {
            int n = i + neighbours9[j];
            if (n >= 0 && n < LENGTH && map[n] != Terrain::WALL && map[n] != Terrain::WALL_DECO) {
                d = true;
                break;
            }
        }
        if (d) {
            d = false;
            for (int j = 0; j < 9; ++j) {
                int n = i + neighbours9[j];
                if (n >= 0 && n < LENGTH && !pit[n]) {
                    d = true;
                    break;
                }
            }
        }
    }
}

std::vector<bool> Level::createWater() {
    return Patch::generate(feeling == Feeling::WATER ? 0.60f : 0.45f, 5);
}

std::vector<bool> Level::createGrass() {
    return Patch::generate(feeling == Feeling::GRASS ? 0.60f : 0.40f, 4);
}

void Level::paintWater() {
    auto lake = createWater();
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && lake[i]) {
            map[i] = Terrain::WATER;
        }
    }
}

void Level::paintGrass() {
    auto gr = createGrass();
    if (feeling == Feeling::GRASS) {
        for (const auto& room : rooms) {
            if (room && room->type != Room::Type::NONE &&
                room->type != Room::Type::PASSAGE && room->type != Room::Type::TUNNEL) {
                gr[(room->left + 1) + (room->top + 1) * WIDTH] = true;
                gr[(room->right - 1) + (room->top + 1) * WIDTH] = true;
                gr[(room->left + 1) + (room->bottom - 1) * WIDTH] = true;
                gr[(room->right - 1) + (room->bottom - 1) * WIDTH] = true;
            }
        }
    }

    const int neighbours8[8] = { +1, -1, +WIDTH, -WIDTH, +1+WIDTH, +1-WIDTH, -1+WIDTH, -1-WIDTH };
    for (int i = WIDTH + 1; i < LENGTH - WIDTH - 1; ++i) {
        if (map[i] == Terrain::EMPTY && gr[i]) {
            int count = 1;
            for (int j = 0; j < 8; ++j) {
                int n = i + neighbours8[j];
                if (n >= 0 && n < LENGTH && gr[n]) {
                    count++;
                }
            }
            map[i] = (Random::Float() < static_cast<float>(count) / 12.0f) ? Terrain::HIGH_GRASS : Terrain::GRASS;
        }
    }
}

bool Level::build() {
    RegularLevel reg;
    reg.feeling = feeling;
    int tries = 0;
    while (!reg.build()) {
        if (++tries > 20) return false;
    }
    std::memcpy(map, reg.map, sizeof(map));
    entrance = reg.entrance;
    exit = reg.exit;
    rooms = std::move(reg.rooms);
    heaps = std::move(reg.heaps);
    secretDoors = reg.secretDoors;
    feeling = reg.feeling;

    buildFlags();
    cleanWalls();

    return true;
}

void Level::updateFieldOfView(int centerCell, int distance) {
    if (centerCell < 0 || centerCell >= LENGTH) return;

    int cx = cellX(centerCell);
    int cy = cellY(centerCell);

    ShadowCaster::castShadow(cx, cy, fieldOfView, distance, losBlocking);

    // Any cell in field of view becomes visited
    for (int i = 0; i < LENGTH; ++i) {
        if (fieldOfView[i]) {
            visited[i] = true;
        }
    }
}

int Level::randomDestination() const {
    if (!rooms.empty()) {
        for (int attempts = 0; attempts < 50; ++attempts) {
            size_t r = Random::Int(static_cast<int>(rooms.size()));
            if (rooms[r] && rooms[r]->type != Room::Type::NONE) {
                int c = rooms[r]->random();
                if (isPassable(c)) {
                    return c;
                }
            }
        }
    }
    for (int attempts = 0; attempts < 100; ++attempts) {
        int c = Random::Int(LENGTH);
        if (isPassable(c)) {
            return c;
        }
    }
    return entrance;
}

const char* Level::tileName(int tile) {
    if (tile >= Terrain::WATER_TILES) {
        return "Water";
    }
    switch (tile) {
        case Terrain::CHASM: return "Chasm";
        case Terrain::EMPTY:
        case Terrain::EMPTY_SP:
        case Terrain::EMPTY_DECO:
            return "Floor";
        case Terrain::GRASS: return "Grass";
        case Terrain::WATER: return "Water";
        case Terrain::WALL:
        case Terrain::WALL_DECO:
            return "Wall";
        case Terrain::DOOR: return "Closed door";
        case Terrain::OPEN_DOOR: return "Open door";
        case Terrain::ENTRANCE: return "Depth entrance";
        case Terrain::EXIT: return "Depth exit";
        case Terrain::EMBERS: return "Embers";
        case Terrain::LOCKED_DOOR: return "Locked door";
        case Terrain::PEDESTAL: return "Pedestal";
        case Terrain::BARRICADE: return "Barricade";
        case Terrain::HIGH_GRASS: return "High grass";
        case Terrain::LOCKED_EXIT: return "Locked depth exit";
        case Terrain::UNLOCKED_EXIT: return "Unlocked depth exit";
        case Terrain::SIGN: return "Sign";
        case Terrain::WELL: return "Well";
        case Terrain::EMPTY_WELL: return "Empty well";
        case Terrain::STATUE:
        case Terrain::STATUE_SP: return "Statue";
        case Terrain::TOXIC_TRAP: return "Toxic gas trap";
        case Terrain::FIRE_TRAP: return "Fire trap";
        case Terrain::PARALYTIC_TRAP: return "Paralytic gas trap";
        case Terrain::POISON_TRAP: return "Poison dart trap";
        case Terrain::ALARM_TRAP: return "Alarm trap";
        case Terrain::LIGHTNING_TRAP: return "Lightning trap";
        case Terrain::GRIPPING_TRAP: return "Gripping trap";
        case Terrain::SUMMONING_TRAP: return "Summoning trap";
        case Terrain::INACTIVE_TRAP: return "Triggered trap";
        case Terrain::BOOKSHELF: return "Bookshelf";
        case Terrain::ALCHEMY: return "Alchemy pot";
        default: return "???";
    }
}

const char* Level::tileDesc(int tile) {
    switch (tile) {
        case Terrain::CHASM: return "You can't see the bottom.";
        case Terrain::WATER: return "In case of burning step into the water to extinguish the fire.";
        case Terrain::ENTRANCE: return "Stairs lead up to the upper depth.";
        case Terrain::EXIT:
        case Terrain::UNLOCKED_EXIT: return "Stairs lead down to the lower depth.";
        case Terrain::EMBERS: return "Embers cover the floor.";
        case Terrain::HIGH_GRASS: return "Dense vegetation blocks the view.";
        case Terrain::LOCKED_DOOR: return "This door is locked, you need a matching key to unlock it.";
        case Terrain::LOCKED_EXIT: return "Heavy bars block the stairs leading down.";
        case Terrain::BARRICADE: return "The wooden barricade is firmly set but has dried over the years. Might it burn?";
        case Terrain::SIGN: return "You can't read the text from here.";
        case Terrain::TOXIC_TRAP:
        case Terrain::FIRE_TRAP:
        case Terrain::PARALYTIC_TRAP:
        case Terrain::POISON_TRAP:
        case Terrain::ALARM_TRAP:
        case Terrain::LIGHTNING_TRAP:
        case Terrain::GRIPPING_TRAP:
        case Terrain::SUMMONING_TRAP:
            return "Stepping onto a hidden pressure plate will activate the trap.";
        case Terrain::INACTIVE_TRAP: return "The trap has been triggered before and it's not dangerous anymore.";
        case Terrain::STATUE:
        case Terrain::STATUE_SP: return "Someone wanted to adorn this place, but failed, obviously.";
        case Terrain::ALCHEMY: return "Drop some seeds here to cook a potion.";
        case Terrain::EMPTY_WELL: return "The well has run dry.";
        default:
            if (tile >= Terrain::WATER_TILES) return "In case of burning step into the water to extinguish the fire.";
            return "";
    }
}

} // namespace pd
