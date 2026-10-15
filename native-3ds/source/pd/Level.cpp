#include "pd/Level.h"
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
#include "noosa/Audio.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace pd {

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

void Level::digRoom(const Room& r) {
    for (int y = r.top; y <= r.bottom; ++y) {
        for (int x = r.left; x <= r.right; ++x) {
            map[y * WIDTH + x] = Terrain::EMPTY;
        }
    }
}

void Level::digCorridor(int x1, int y1, int x2, int y2) {
    int cx = x1;
    int cy = y1;

    // Dig horizontally then vertically
    while (cx != x2) {
        if (map[cy * WIDTH + cx] == Terrain::WALL) {
            map[cy * WIDTH + cx] = Terrain::EMPTY;
        }
        cx += (x2 > cx) ? 1 : -1;
    }
    while (cy != y2) {
        if (map[cy * WIDTH + cx] == Terrain::WALL) {
            map[cy * WIDTH + cx] = Terrain::EMPTY;
        }
        cy += (y2 > cy) ? 1 : -1;
    }
    if (map[cy * WIDTH + cx] == Terrain::WALL) {
        map[cy * WIDTH + cx] = Terrain::EMPTY;
    }
}

bool Level::build() {
    std::fill_n(map, LENGTH, Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    rooms.clear();
    heaps.clear();

    const int targetRooms = Random::IntRange(6, 9);
    int attempts = 0;

    // Room partitioning
    while (static_cast<int>(rooms.size()) < targetRooms && attempts < 150) {
        attempts++;
        int rw = Random::IntRange(4, 7);
        int rh = Random::IntRange(4, 7);
        int rx = Random::IntRange(1, WIDTH - rw - 2);
        int ry = Random::IntRange(1, HEIGHT - rh - 2);

        Room candidate(rx, ry, rx + rw - 1, ry + rh - 1);

        bool overlaps = false;
        for (const auto& r : rooms) {
            if (candidate.intersects(r, 1)) {
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            rooms.push_back(candidate);
            digRoom(candidate);
        }
    }

    if (rooms.size() < 3) return false;

    // Connect rooms sequentially
    for (size_t i = 0; i < rooms.size() - 1; ++i) {
        digCorridor(rooms[i].centerX(), rooms[i].centerY(),
                    rooms[i + 1].centerX(), rooms[i + 1].centerY());
    }
    // Add cycle connection between first and middle room
    digCorridor(rooms[0].centerX(), rooms[0].centerY(),
                rooms[rooms.size() / 2].centerX(), rooms[rooms.size() / 2].centerY());

    // Place entrance and exit
    rooms[0].type = Room::Type::ENTRANCE;
    entrance = rooms[0].centerY() * WIDTH + rooms[0].centerX();
    map[entrance] = Terrain::ENTRANCE;

    rooms.back().type = Room::Type::EXIT;
    exit = rooms.back().centerY() * WIDTH + rooms.back().centerX();
    map[exit] = Terrain::EXIT;

    // Identify doorway transitions (empty tile adjacent to wall on 2 opposite sides)
    for (int y = 1; y < HEIGHT - 1; ++y) {
        for (int x = 1; x < WIDTH - 1; ++x) {
            int c = y * WIDTH + x;
            if (map[c] == Terrain::EMPTY) {
                bool horizontalWalls = (map[c - 1] == Terrain::WALL && map[c + 1] == Terrain::WALL &&
                                        map[c - WIDTH] == Terrain::EMPTY && map[c + WIDTH] == Terrain::EMPTY);
                bool verticalWalls   = (map[c - WIDTH] == Terrain::WALL && map[c + WIDTH] == Terrain::WALL &&
                                        map[c - 1] == Terrain::EMPTY && map[c + 1] == Terrain::EMPTY);

                if (horizontalWalls || verticalWalls) {
                    if (Random::Float() < 0.65f) {
                        map[c] = (Random::Float() < 0.15f) ? Terrain::SECRET_DOOR : Terrain::DOOR;
                    }
                }
            }
        }
    }

    // Procedural organic lakes and pools using cellular automaton Patch
    std::vector<bool> lake = Patch::generate(0.45f, 5);
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && lake[i] && i != entrance && i != exit) {
            map[i] = Terrain::WATER;
        }
    }

    // Procedural organic meadows and high grass using cellular automaton Patch
    std::vector<bool> grass = Patch::generate(0.40f, 4);
    const int neighbours8[8] = { +1, -1, +WIDTH, -WIDTH, +1+WIDTH, +1-WIDTH, -1+WIDTH, -1-WIDTH };
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && grass[i] && i != entrance && i != exit) {
            int count = 0;
            for (int j = 0; j < 8; ++j) {
                int n = i + neighbours8[j];
                if (n >= 0 && n < LENGTH && grass[n]) {
                    count++;
                }
            }
            map[i] = (Random::Float() < static_cast<float>(count) / 12.0f) ? Terrain::HIGH_GRASS : Terrain::GRASS;
        }
    }

    // Scatter traps (both visible and secret traps)
    for (size_t i = 1; i < rooms.size() - 1; ++i) {
        const auto& r = rooms[i];
        if (Random::Float() < 0.60f) {
            int trapCount = Random::IntRange(1, 2);
            for (int t = 0; t < trapCount; ++t) {
                int tx = Random::IntRange(r.left + 1, r.right - 1);
                int ty = Random::IntRange(r.top + 1, r.bottom - 1);
                int tc = ty * WIDTH + tx;
                if (map[tc] == Terrain::EMPTY && tc != entrance && tc != exit) {
                    int trapTypes[] = {
                        Terrain::FIRE_TRAP, Terrain::TOXIC_TRAP, Terrain::PARALYTIC_TRAP,
                        Terrain::POISON_TRAP, Terrain::ALARM_TRAP, Terrain::LIGHTNING_TRAP,
                        Terrain::GRIPPING_TRAP, Terrain::SUMMONING_TRAP, Terrain::TELEPORTATION_TRAP
                    };
                    int secretTypes[] = {
                        Terrain::SECRET_FIRE_TRAP, Terrain::SECRET_TOXIC_TRAP, Terrain::SECRET_PARALYTIC_TRAP,
                        Terrain::SECRET_POISON_TRAP, Terrain::SECRET_ALARM_TRAP, Terrain::SECRET_LIGHTNING_TRAP,
                        Terrain::SECRET_GRIPPING_TRAP, Terrain::SECRET_SUMMONING_TRAP, Terrain::SECRET_TELEPORTATION_TRAP
                    };
                    int idx = Random::Int(9);
                    bool isSec = (Random::Float() < 0.70f);
                    map[tc] = isSec ? secretTypes[idx] : trapTypes[idx];
                }
            }
        }
    }

    // Resolve water shore tiles matching Java Level.java
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WATER) {
            water[i] = true;
        }
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) {
            map[i] = getWaterTile(i);
        }
    }

    buildFlags();

    // Authentic special room generation (e.g. Vault room with crystal chests or locked chest and golden key)
    bool vaultGenerated = false;
    for (size_t i = 1; i < rooms.size() - 1; ++i) {
        if (!vaultGenerated && Random::Float() < 0.35f) {
            rooms[i].type = Room::Type::VAULT;
            vaultGenerated = true;

            int c = rooms[i].centerY() * WIDTH + rooms[i].centerX();
            // In Vault room: 2 Crystal Chests with different prizes (or 1 Locked Chest) + Golden Key hidden in the level
            if (Random::Int(2) == 0) {
                // Crystal chests choice
                auto chest1 = std::make_unique<Heap>(c, Heap::Type::CRYSTAL_CHEST);
                auto w = std::make_unique<Weapon>("Battle Axe", 5, 20, 16);
                w->random();
                chest1->drop(std::move(w));
                heaps.push_back(std::move(chest1));

                // Adjacent crystal chest
                int c2 = c + 1;
                if (!isPassable(c2)) c2 = c - 1;
                if (isPassable(c2)) {
                    auto chest2 = std::make_unique<Heap>(c2, Heap::Type::CRYSTAL_CHEST);
                    auto a = std::make_unique<Armor>("Scale Armor", 4, 16);
                    a->random();
                    chest2->drop(std::move(a));
                    heaps.push_back(std::move(chest2));
                }
            } else {
                auto lChest = std::make_unique<Heap>(c, Heap::Type::LOCKED_CHEST);
                auto w = std::make_unique<Weapon>("Longsword", 4, 15, 14);
                w->random();
                lChest->drop(std::move(w));
                heaps.push_back(std::move(lChest));
            }

            // Spawn the required Golden Key in a different room
            size_t keyRoomIdx = (i == 1) ? (rooms.size() - 2) : 1;
            int keyCell = rooms[keyRoomIdx].centerY() * WIDTH + rooms[keyRoomIdx].centerX() + 1;
            if (!isPassable(keyCell)) keyCell = rooms[keyRoomIdx].centerY() * WIDTH + rooms[keyRoomIdx].centerX();
            drop(std::make_unique<GoldenKey>(Dungeon::depth), keyCell);
            continue;
        }

        const auto& r = rooms[i];
        int cell = r.centerY() * WIDTH + r.centerX();
        if (isPassable(cell) && !getHeap(cell)) {
            float roll = Random::Float();
            if (roll < 0.25f) {
                // Regular unlocked chest (does not need a key)
                auto chest = std::make_unique<Heap>(cell, Heap::Type::CHEST);
                if (Random::Float() < 0.5f) {
                    chest->drop(std::make_unique<Potion>(Potion::PotionType::HEALING));
                } else {
                    chest->drop(std::make_unique<Scroll>(Scroll::ScrollType::UPGRADE));
                }
                heaps.push_back(std::move(chest));
            } else if (roll < 0.45f) {
                // Skeletal remains containing unidentified weapon or armor
                auto skel = std::make_unique<Heap>(cell, Heap::Type::SKELETON);
                if (Random::Float() < 0.5f) {
                    auto w = std::make_unique<Weapon>("Shortsword", 3, 8, 11);
                    w->random();
                    skel->drop(std::move(w));
                } else {
                    auto a = std::make_unique<Armor>("Leather Armor", 2, 12);
                    a->random();
                    skel->drop(std::move(a));
                }
                heaps.push_back(std::move(skel));
            } else if (roll < 0.75f) {
                auto ground = std::make_unique<Heap>(cell, Heap::Type::HEAP);
                float lootRoll = Random::Float();
                if (lootRoll < 0.35f) {
                    ground->drop(std::make_unique<Food>());
                } else if (lootRoll < 0.55f) {
                    ground->drop(std::make_unique<Potion>(Potion::PotionType::STRENGTH));
                } else if (lootRoll < 0.75f) {
                    ground->drop(std::make_unique<Scroll>(Scroll::ScrollType::MAGIC_MAPPING));
                } else {
                    // Ground equipment drop (unidentified, possibly cursed)
                    if (Random::Float() < 0.5f) {
                        auto w = std::make_unique<Weapon>("Mace", 3, 10, 12);
                        w->random();
                        ground->drop(std::move(w));
                    } else {
                        auto a = std::make_unique<Armor>("Mail Armor", 3, 14);
                        a->random();
                        ground->drop(std::move(a));
                    }
                }
                heaps.push_back(std::move(ground));
            }
        }
    }

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
            int c = rooms[r].randomCell();
            if (isPassable(c)) {
                return c;
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

} // namespace pd
