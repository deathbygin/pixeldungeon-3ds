#include "pd/Biomes.h"
#include "pd/Patch.h"
#include "pd/Painter.h"
#include "pd/Random.h"
#include "pd/Dungeon.h"
#include "pd/Item.h"
#include "pd/Graph.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace pd {

// =============================================================================
// Sewers (Depths 1-4) & Sewer Boss (Depth 5)
// =============================================================================

SewerLevel::SewerLevel() {
    viewDistance = 8;
}

std::vector<bool> SewerLevel::createWater() {
    return Patch::generate(feeling == Feeling::WATER ? 0.60f : 0.45f, 5);
}

std::vector<bool> SewerLevel::createGrass() {
    return Patch::generate(feeling == Feeling::GRASS ? 0.60f : 0.40f, 4);
}

void SewerLevel::decorate() {
    // Top wall drains
    for (int i = 0; i < WIDTH; ++i) {
        if (map[i] == Terrain::WALL && map[i + WIDTH] == Terrain::WATER && Random::Int(4) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    // Mid wall drains
    for (int i = WIDTH; i < LENGTH - WIDTH; ++i) {
        if (map[i] == Terrain::WALL && map[i - WIDTH] == Terrain::WALL &&
            map[i + WIDTH] == Terrain::WATER && Random::Int(2) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    // Floor moss
    for (int i = WIDTH + 1; i < LENGTH - WIDTH - 1; ++i) {
        if (map[i] == Terrain::EMPTY) {
            int count = (map[i + 1] == Terrain::WALL ? 1 : 0) +
                        (map[i - 1] == Terrain::WALL ? 1 : 0) +
                        (map[i + WIDTH] == Terrain::WALL ? 1 : 0) +
                        (map[i - WIDTH] == Terrain::WALL ? 1 : 0);
            if (Random::Int(16) < count * count) {
                map[i] = Terrain::EMPTY_DECO;
            }
        }
    }

    // Entrance sign
    if (roomEntrance) {
        for (int attempts = 0; attempts < 20; ++attempts) {
            int pos = roomEntrance->random();
            if (pos != entrance && isPassable(pos)) {
                map[pos] = Terrain::SIGN;
                break;
            }
        }
    }
}

SewerBossLevel::SewerBossLevel() {
    viewDistance = 8;
}

std::vector<bool> SewerBossLevel::createWater() {
    return Patch::generate(0.50f, 5);
}

std::vector<bool> SewerBossLevel::createGrass() {
    return Patch::generate(0.40f, 4);
}

void SewerBossLevel::seal() {
    if (entrance != 0) {
        set(entrance, Terrain::WATER_TILES);
        stairs = entrance;
        entrance = 0;
        Sample::playAt(Assets::SND_WATER, stairs);
        GLog::w("Murky water floods the entrance stairs behind you!");
    }
}

void SewerBossLevel::unseal() {
    if (stairs != 0) {
        entrance = stairs;
        stairs = 0;
        set(entrance, Terrain::ENTRANCE);
        GLog::p("The floodwaters recede, revealing the stairs!");
    }
}

bool SewerBossLevel::build() {
    std::fill_n(map, LENGTH, feeling == Feeling::CHASM ? Terrain::CHASM : Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    secretDoors = 0;
    stairs = 0;

    if (!initRooms()) return false;

    int distance = 0;
    int retry = 0;
    int minDistance = static_cast<int>(std::sqrt(rooms.size()));

    do {
        int innerRetry = 0;
        do {
            if (innerRetry++ > 25) return false;
            roomEntrance = Random::element(rooms);
        } while (roomEntrance->width() < 4 || roomEntrance->height() < 4);

        innerRetry = 0;
        do {
            if (innerRetry++ > 25) return false;
            roomExit = Random::element(rooms);
        } while (roomExit == roomEntrance || roomExit->width() < 6 || roomExit->height() < 6 || roomExit->top == 0);

        Graph::buildDistanceMap(rooms, roomExit);
        distance = roomEntrance->distance;

        if (retry++ > 25) return false;
    } while (distance < minDistance || distance >= 1000000);

    roomEntrance->type = Room::Type::ENTRANCE;
    roomExit->type = Room::Type::BOSS_EXIT;

    Graph::buildDistanceMap(rooms, roomExit);
    auto path = Graph::buildPath(rooms, roomEntrance, roomExit);
    if (path.empty()) return false;

    Graph::setPrice(path, roomEntrance->distance);

    Graph::buildDistanceMap(rooms, roomExit);
    path = Graph::buildPath(rooms, roomEntrance, roomExit);
    if (path.empty()) return false;

    auto room = roomEntrance;
    for (const auto& next : path) {
        room->connect(next.get());
        room = next;
    }

    if (roomExit->connected.empty()) {
        return false;
    }
    auto connectedRoom = roomExit->connected.begin()->first;
    if (connectedRoom && roomExit->top == connectedRoom->bottom) {
        return false;
    }

    for (auto& r : rooms) {
        if (r->type == Room::Type::NONE && !r->connected.empty()) {
            r->type = Room::Type::TUNNEL;
        }
    }

    // Rat King chamber candidate
    std::vector<Room*> candidates;
    for (auto* r : roomExit->neigbours) {
        if (roomExit->connected.find(r) == roomExit->connected.end() &&
            (roomExit->left == r->right || roomExit->right == r->left || roomExit->bottom == r->top)) {
            candidates.push_back(r);
        }
    }
    if (!candidates.empty()) {
        auto* kingsRoom = Random::element(candidates);
        kingsRoom->connect(roomExit.get());
        kingsRoom->type = Room::Type::RAT_KING;
    }

    paint();
    paintWater();
    paintGrass();
    placeTraps();
    decorate();

    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WATER) water[i] = true;
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) map[i] = getWaterTile(i);
    }

    buildFlags();
    cleanWalls();

    return true;
}

void SewerBossLevel::decorate() {
    if (roomExit) {
        int start = roomExit->top * WIDTH + roomExit->left + 1;
        int end = start + roomExit->width() - 1;
        for (int i = start; i < end; ++i) {
            if (i != exit) {
                map[i] = Terrain::WALL_DECO;
                map[i + WIDTH] = Terrain::WATER;
            } else {
                map[i + WIDTH] = Terrain::EMPTY;
            }
        }
    }

    if (roomEntrance) {
        for (int attempts = 0; attempts < 20; ++attempts) {
            int pos = roomEntrance->random();
            if (pos != entrance && isPassable(pos)) {
                map[pos] = Terrain::SIGN;
                break;
            }
        }
    }
}

// =============================================================================
// Prison (Depths 6-9) & Prison Boss (Depth 10)
// =============================================================================

PrisonLevel::PrisonLevel() {
    viewDistance = 8;
}

std::vector<bool> PrisonLevel::createWater() {
    return Patch::generate(feeling == Feeling::WATER ? 0.65f : 0.45f, 4);
}

std::vector<bool> PrisonLevel::createGrass() {
    return Patch::generate(feeling == Feeling::GRASS ? 0.60f : 0.40f, 3);
}

void PrisonLevel::assignRoomType() {
    RegularLevel::assignRoomType();
    for (auto& r : rooms) {
        if (r->type == Room::Type::TUNNEL) {
            r->type = Room::Type::PASSAGE;
        }
    }
}

void PrisonLevel::decorate() {
    for (int i = WIDTH + 1; i < LENGTH - WIDTH - 1; ++i) {
        if (map[i] == Terrain::EMPTY) {
            float c = 0.05f;
            if (map[i + 1] == Terrain::WALL && map[i + WIDTH] == Terrain::WALL) c += 0.2f;
            if (map[i - 1] == Terrain::WALL && map[i + WIDTH] == Terrain::WALL) c += 0.2f;
            if (map[i + 1] == Terrain::WALL && map[i - WIDTH] == Terrain::WALL) c += 0.2f;
            if (map[i - 1] == Terrain::WALL && map[i - WIDTH] == Terrain::WALL) c += 0.2f;

            if (Random::Float() < c) {
                map[i] = Terrain::EMPTY_DECO;
            }
        }
    }

    for (int i = 0; i < WIDTH; ++i) {
        if (map[i] == Terrain::WALL &&
            (map[i + WIDTH] == Terrain::EMPTY || map[i + WIDTH] == Terrain::EMPTY_SP) &&
            Random::Int(6) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    for (int i = WIDTH; i < LENGTH - WIDTH; ++i) {
        if (map[i] == Terrain::WALL && map[i - WIDTH] == Terrain::WALL &&
            (map[i + WIDTH] == Terrain::EMPTY || map[i + WIDTH] == Terrain::EMPTY_SP) &&
            Random::Int(3) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    if (roomEntrance) {
        for (int attempts = 0; attempts < 20; ++attempts) {
            int pos = roomEntrance->random();
            if (pos != entrance && isPassable(pos)) {
                map[pos] = Terrain::SIGN;
                break;
            }
        }
    }
}

PrisonBossLevel::PrisonBossLevel() {
    viewDistance = 8;
}

bool PrisonBossLevel::build() {
    std::fill_n(map, LENGTH, feeling == Feeling::CHASM ? Terrain::CHASM : Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    secretDoors = 0;

    if (!initRooms()) return false;

    int distance = 0;
    int retry = 0;

    do {
        if (retry++ > 25) return false;

        int innerRetry = 0;
        do {
            if (innerRetry++ > 25) return false;
            roomEntrance = Random::element(rooms);
        } while (roomEntrance->width() < 4 || roomEntrance->height() < 4);

        innerRetry = 0;
        do {
            if (innerRetry++ > 25) return false;
            roomExit = Random::element(rooms);
        } while (roomExit == roomEntrance || roomExit->width() < 7 || roomExit->height() < 7 || roomExit->top == 0);

        Graph::buildDistanceMap(rooms, roomExit);
        auto p = Graph::buildPath(rooms, roomEntrance, roomExit);
        distance = static_cast<int>(p.size());
    } while (distance < 3 || distance >= 1000000);

    roomEntrance->type = Room::Type::ENTRANCE;
    roomExit->type = Room::Type::BOSS_EXIT;

    auto path = Graph::buildPath(rooms, roomEntrance, roomExit);
    if (path.size() < 2) return false;
    Graph::setPrice(path, roomEntrance->distance);

    Graph::buildDistanceMap(rooms, roomExit);
    path = Graph::buildPath(rooms, roomEntrance, roomExit);

    if (path.size() < 2) return false;
    auto anteroom = path[path.size() - 2];
    anteroom->type = Room::Type::STANDARD;

    auto room = roomEntrance;
    for (const auto& next : path) {
        room->connect(next.get());
        room = next;
    }

    for (auto& r : rooms) {
        if (r->type == Room::Type::NONE && !r->connected.empty()) {
            r->type = Room::Type::PASSAGE;
        }
    }

    paint();

    if (roomExit->connected.empty()) {
        return false;
    }
    auto r = roomExit->connected.begin()->first;
    auto d = roomExit->connected[r];
    if (!d || d->y == roomExit->top) {
        return false;
    }

    paintWater();
    paintGrass();
    placeTraps();

    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WATER) water[i] = true;
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) map[i] = getWaterTile(i);
    }

    buildFlags();
    cleanWalls();

    return true;
}

// =============================================================================
// Caves (Depths 11-14) & Caves Boss (Depth 15)
// =============================================================================

CavesLevel::CavesLevel() {
    viewDistance = 6;
}

std::vector<bool> CavesLevel::createWater() {
    return Patch::generate(feeling == Feeling::WATER ? 0.60f : 0.45f, 6);
}

std::vector<bool> CavesLevel::createGrass() {
    return Patch::generate(feeling == Feeling::GRASS ? 0.55f : 0.35f, 3);
}

void CavesLevel::decorate() {
    // Organic cave rounding: smooth sharp room corners into rock
    for (const auto& room : rooms) {
        if (!room || room->type != Room::Type::STANDARD) continue;
        if (room->width() <= 3 || room->height() <= 3) continue;

        int s = room->square();
        if (Random::Int(s) > 8) {
            int corner = (room->left + 1) + (room->top + 1) * WIDTH;
            if (map[corner - 1] == Terrain::WALL && map[corner - WIDTH] == Terrain::WALL) {
                map[corner] = Terrain::WALL;
            }
        }
        if (Random::Int(s) > 8) {
            int corner = (room->right - 1) + (room->top + 1) * WIDTH;
            if (map[corner + 1] == Terrain::WALL && map[corner - WIDTH] == Terrain::WALL) {
                map[corner] = Terrain::WALL;
            }
        }
        if (Random::Int(s) > 8) {
            int corner = (room->left + 1) + (room->bottom - 1) * WIDTH;
            if (map[corner - 1] == Terrain::WALL && map[corner + WIDTH] == Terrain::WALL) {
                map[corner] = Terrain::WALL;
            }
        }
        if (Random::Int(s) > 8) {
            int corner = (room->right - 1) + (room->bottom - 1) * WIDTH;
            if (map[corner + 1] == Terrain::WALL && map[corner + WIDTH] == Terrain::WALL) {
                map[corner] = Terrain::WALL;
            }
        }

        for (const auto& pair : room->connected) {
            Room* n = pair.first;
            if (n && (n->type == Room::Type::STANDARD || n->type == Room::Type::TUNNEL) && Random::Int(3) == 0) {
                if (pair.second) {
                    map[pair.second->x + pair.second->y * WIDTH] = Terrain::EMPTY_DECO;
                }
            }
        }
    }

    // Cave rubble
    for (int i = WIDTH + 1; i < LENGTH - WIDTH; ++i) {
        if (map[i] == Terrain::EMPTY) {
            int n = (map[i + 1] == Terrain::WALL ? 1 : 0) +
                    (map[i - 1] == Terrain::WALL ? 1 : 0) +
                    (map[i + WIDTH] == Terrain::WALL ? 1 : 0) +
                    (map[i - WIDTH] == Terrain::WALL ? 1 : 0);
            if (Random::Int(6) <= n) {
                map[i] = Terrain::EMPTY_DECO;
            }
        }
    }

    // Gold & mineral veins in rock walls
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WALL && Random::Int(12) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    if (roomEntrance) {
        for (int attempts = 0; attempts < 20; ++attempts) {
            int pos = roomEntrance->random();
            if (pos != entrance && isPassable(pos)) {
                map[pos] = Terrain::SIGN;
                break;
            }
        }
    }
}

CavesBossLevel::CavesBossLevel() {
    viewDistance = 6;
}

bool CavesBossLevel::build() {
    std::fill_n(map, LENGTH, Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    rooms.clear();

    const int ROOM_LEFT   = WIDTH / 2 - 2;
    const int ROOM_RIGHT  = WIDTH / 2 + 2;
    const int ROOM_TOP    = HEIGHT / 2 - 2;
    const int ROOM_BOTTOM = HEIGHT / 2 + 2;

    int topMost = 999999;

    for (int i = 0; i < 8; ++i) {
        int left, right, top, bottom;
        if (Random::Int(2) == 0) {
            left = Random::Int(1, ROOM_LEFT - 3);
            right = ROOM_RIGHT + 3;
        } else {
            left = ROOM_LEFT - 3;
            right = Random::Int(ROOM_RIGHT + 3, WIDTH - 1);
        }
        if (Random::Int(2) == 0) {
            top = Random::Int(2, ROOM_TOP - 3);
            bottom = ROOM_BOTTOM + 3;
        } else {
            top = ROOM_LEFT - 3;
            bottom = Random::Int(ROOM_TOP + 3, HEIGHT - 1);
        }

        Painter::fill(this, left, top, right - left + 1, bottom - top + 1, Terrain::EMPTY);

        if (top < topMost) {
            topMost = top;
            exit = Random::Int(left, right) + (top - 1) * WIDTH;
        }
    }

    map[exit] = Terrain::LOCKED_EXIT;

    // Scattered inactive sparks
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && Random::Int(6) == 0) {
            map[i] = Terrain::INACTIVE_TRAP;
        }
    }

    // Safe sanctuary center
    Painter::fill(this, ROOM_LEFT - 1, ROOM_TOP - 1, ROOM_RIGHT - ROOM_LEFT + 3, ROOM_BOTTOM - ROOM_TOP + 3, Terrain::WALL);
    Painter::fill(this, ROOM_LEFT, ROOM_TOP + 1, ROOM_RIGHT - ROOM_LEFT + 1, ROOM_BOTTOM - ROOM_TOP, Terrain::EMPTY);
    Painter::fill(this, ROOM_LEFT, ROOM_TOP, ROOM_RIGHT - ROOM_LEFT + 1, 1, Terrain::TOXIC_TRAP);

    int arenaDoor = Random::Int(ROOM_LEFT, ROOM_RIGHT) + (ROOM_BOTTOM + 1) * WIDTH;
    map[arenaDoor] = Terrain::DOOR;

    entrance = Random::Int(ROOM_LEFT + 1, ROOM_RIGHT - 1) + Random::Int(ROOM_TOP + 1, ROOM_BOTTOM - 1) * WIDTH;
    map[entrance] = Terrain::ENTRANCE;

    // Water pools
    auto patch = Patch::generate(0.45f, 6);
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && patch[i]) {
            map[i] = Terrain::WATER;
        }
    }

    decorate();

    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WATER) water[i] = true;
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) map[i] = getWaterTile(i);
    }

    buildFlags();
    cleanWalls();

    return true;
}

void CavesBossLevel::decorate() {
    for (int i = WIDTH + 1; i < LENGTH - WIDTH; ++i) {
        if (map[i] == Terrain::EMPTY) {
            int n = (map[i + 1] == Terrain::WALL ? 1 : 0) +
                    (map[i - 1] == Terrain::WALL ? 1 : 0) +
                    (map[i + WIDTH] == Terrain::WALL ? 1 : 0) +
                    (map[i - WIDTH] == Terrain::WALL ? 1 : 0);
            if (Random::Int(8) <= n) {
                map[i] = Terrain::EMPTY_DECO;
            }
        }
    }
}

// =============================================================================
// City (Depths 16-19, 21) & City Boss (Depth 20)
// =============================================================================

CityLevel::CityLevel() {
    if (Dungeon::depth == 21) {
        viewDistance = std::max(25 - Dungeon::depth, 1);
    } else {
        viewDistance = 8;
    }
}

std::vector<bool> CityLevel::createWater() {
    return Patch::generate(feeling == Feeling::WATER ? 0.65f : 0.45f, 4);
}

std::vector<bool> CityLevel::createGrass() {
    return Patch::generate(feeling == Feeling::GRASS ? 0.60f : 0.40f, 3);
}

void CityLevel::assignRoomType() {
    RegularLevel::assignRoomType();
    for (auto& r : rooms) {
        if (r->type == Room::Type::TUNNEL) {
            r->type = Room::Type::PASSAGE;
        }
    }
}

void CityLevel::decorate() {
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && Random::Int(10) == 0) {
            map[i] = Terrain::EMPTY_DECO;
        } else if (map[i] == Terrain::WALL && Random::Int(8) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    if (roomEntrance) {
        for (int attempts = 0; attempts < 20; ++attempts) {
            int pos = roomEntrance->random();
            if (pos != entrance && isPassable(pos)) {
                map[pos] = Terrain::SIGN;
                break;
            }
        }
    }
}

CityBossLevel::CityBossLevel() {
    viewDistance = 8;
}

bool CityBossLevel::build() {
    std::fill_n(map, LENGTH, Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    rooms.clear();

    const int TOP = 2;
    const int HALL_WIDTH = 7;
    const int HALL_HEIGHT = 15;
    const int CHAMBER_HEIGHT = 3;

    const int LEFT = (WIDTH - HALL_WIDTH) / 2;
    const int CENTER = LEFT + HALL_WIDTH / 2;

    Painter::fill(this, LEFT, TOP, HALL_WIDTH, HALL_HEIGHT, Terrain::EMPTY);
    Painter::fill(this, CENTER, TOP, 1, HALL_HEIGHT, Terrain::EMPTY_SP);

    int y = TOP + 1;
    while (y < TOP + HALL_HEIGHT) {
        map[y * WIDTH + CENTER - 2] = Terrain::STATUE_SP;
        map[y * WIDTH + CENTER + 2] = Terrain::STATUE_SP;
        y += 2;
    }

    int leftPedestal = (TOP + HALL_HEIGHT / 2) * WIDTH + CENTER - 2;
    int rightPedestal = (TOP + HALL_HEIGHT / 2) * WIDTH + CENTER + 2;
    map[leftPedestal] = map[rightPedestal] = Terrain::PEDESTAL;
    for (int i = leftPedestal + 1; i < rightPedestal; ++i) {
        map[i] = Terrain::EMPTY_SP;
    }

    exit = (TOP - 1) * WIDTH + CENTER;
    map[exit] = Terrain::LOCKED_EXIT;

    int arenaDoor = (TOP + HALL_HEIGHT) * WIDTH + CENTER;
    map[arenaDoor] = Terrain::DOOR;

    Painter::fill(this, LEFT, TOP + HALL_HEIGHT + 1, HALL_WIDTH, CHAMBER_HEIGHT, Terrain::EMPTY);
    Painter::fill(this, LEFT, TOP + HALL_HEIGHT + 1, 1, CHAMBER_HEIGHT, Terrain::BOOKSHELF);
    Painter::fill(this, LEFT + HALL_WIDTH - 1, TOP + HALL_HEIGHT + 1, 1, CHAMBER_HEIGHT, Terrain::BOOKSHELF);

    entrance = (TOP + HALL_HEIGHT + 2 + Random::Int(CHAMBER_HEIGHT - 1)) * WIDTH + LEFT + (1 + Random::Int(HALL_WIDTH - 2));
    map[entrance] = Terrain::ENTRANCE;

    decorate();

    buildFlags();
    cleanWalls();

    return true;
}

void CityBossLevel::decorate() {
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && Random::Int(10) == 0) {
            map[i] = Terrain::EMPTY_DECO;
        } else if (map[i] == Terrain::WALL && Random::Int(8) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }
}

// =============================================================================
// Halls (Depths 22-24), Halls Boss (Depth 25) & Last (Depth 26)
// =============================================================================

HallsLevel::HallsLevel() {
    minRoomSize = 6;
    viewDistance = std::max(25 - Dungeon::depth, 1);
    addItemToSpawn(std::make_unique<Torch>());
}

std::vector<bool> HallsLevel::createWater() {
    return Patch::generate(feeling == Feeling::WATER ? 0.55f : 0.40f, 6);
}

std::vector<bool> HallsLevel::createGrass() {
    return Patch::generate(feeling == Feeling::GRASS ? 0.55f : 0.30f, 3);
}

void HallsLevel::decorate() {
    const int neighbours8[8] = { +1, -1, +WIDTH, -WIDTH, +1+WIDTH, +1-WIDTH, -1+WIDTH, -1-WIDTH };
    for (int i = WIDTH + 1; i < LENGTH - WIDTH - 1; ++i) {
        if (map[i] == Terrain::EMPTY) {
            int count = 0;
            for (int j = 0; j < 8; ++j) {
                if (isPassable(i + neighbours8[j])) count++;
            }
            if (Random::Int(80) < count) {
                map[i] = Terrain::EMPTY_DECO;
            }
        } else if (map[i] == Terrain::WALL && map[i - 1] != Terrain::WALL_DECO &&
                   map[i - WIDTH] != Terrain::WALL_DECO && Random::Int(20) == 0) {
            map[i] = Terrain::WALL_DECO;
        }
    }

    if (roomEntrance) {
        for (int attempts = 0; attempts < 20; ++attempts) {
            int pos = roomEntrance->random();
            if (pos != entrance && isPassable(pos)) {
                map[pos] = Terrain::SIGN;
                break;
            }
        }
    }
}

HallsBossLevel::HallsBossLevel() {
    viewDistance = 3;
}

bool HallsBossLevel::build() {
    std::fill_n(map, LENGTH, Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    rooms.clear();

    const int ROOM_LEFT   = WIDTH / 2 - 1;
    const int ROOM_RIGHT  = WIDTH / 2 + 1;
    const int ROOM_TOP    = HEIGHT / 2 - 1;
    const int ROOM_BOTTOM = HEIGHT / 2 + 1;

    for (int i = 0; i < 5; ++i) {
        int top = Random::Int(2, ROOM_TOP);
        int bottom = Random::Int(ROOM_BOTTOM + 1, 23);
        Painter::fill(this, 2 + i * 4, top, 4, bottom - top + 1, Terrain::EMPTY);

        if (i == 2) {
            exit = (i * 4 + 3) + (top - 1) * WIDTH;
        }

        for (int j = 0; j < 4; ++j) {
            if (Random::Int(2) == 0) {
                int y = Random::Int(top + 1, bottom);
                map[i * 4 + j + y * WIDTH] = Terrain::STATUE_SP;
            }
        }
    }

    map[exit] = Terrain::LOCKED_EXIT;

    Painter::fill(this, ROOM_LEFT - 1, ROOM_TOP - 1, ROOM_RIGHT - ROOM_LEFT + 3, ROOM_BOTTOM - ROOM_TOP + 3, Terrain::WALL);
    Painter::fill(this, ROOM_LEFT, ROOM_TOP, ROOM_RIGHT - ROOM_LEFT + 1, ROOM_BOTTOM - ROOM_TOP + 1, Terrain::EMPTY);

    entrance = Random::Int(ROOM_LEFT + 1, ROOM_RIGHT) + Random::Int(ROOM_TOP + 1, ROOM_BOTTOM) * WIDTH;
    map[entrance] = Terrain::ENTRANCE;

    auto patch = Patch::generate(0.45f, 6);
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && patch[i]) {
            map[i] = Terrain::WATER;
        }
    }

    decorate();

    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WATER) water[i] = true;
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) map[i] = getWaterTile(i);
    }

    buildFlags();
    cleanWalls();

    return true;
}

void HallsBossLevel::decorate() {
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && Random::Int(10) == 0) {
            map[i] = Terrain::EMPTY_DECO;
        }
    }
}

LastLevel::LastLevel() {
    viewDistance = 8;
}

bool LastLevel::build() {
    std::fill_n(map, LENGTH, Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    rooms.clear();

    const int SIZE = 7;

    Painter::fill(this, 1, 1, SIZE, SIZE, Terrain::WATER);
    Painter::fill(this, 2, 2, SIZE - 2, SIZE - 2, Terrain::EMPTY);
    Painter::fill(this, SIZE / 2, SIZE / 2, 3, 3, Terrain::EMPTY_SP);

    entrance = SIZE * WIDTH + SIZE / 2 + 1;
    map[entrance] = Terrain::ENTRANCE;

    exit = entrance - WIDTH * SIZE;
    map[exit] = Terrain::LOCKED_EXIT;

    int pedestal = (SIZE / 2 + 1) * (WIDTH + 1);
    map[pedestal] = Terrain::PEDESTAL;
    map[pedestal - 1] = map[pedestal + 1] = Terrain::STATUE_SP;

    drop(std::make_unique<Amulet>(), pedestal);

    feeling = Feeling::NONE;

    decorate();

    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::WATER) water[i] = true;
    }
    for (int i = 0; i < LENGTH; ++i) {
        if (water[i]) map[i] = getWaterTile(i);
    }

    buildFlags();
    cleanWalls();

    return true;
}

void LastLevel::decorate() {
    for (int i = 0; i < LENGTH; ++i) {
        if (map[i] == Terrain::EMPTY && Random::Int(10) == 0) {
            map[i] = Terrain::EMPTY_DECO;
        }
    }
}

} // namespace pd
