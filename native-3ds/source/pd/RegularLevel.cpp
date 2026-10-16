#include "pd/RegularLevel.h"
#include "pd/Graph.h"
#include "pd/Painter.h"
#include "pd/Random.h"
#include "pd/Dungeon.h"
#include "pd/Item.h"
#include <cmath>
#include <unordered_set>

namespace pd {

std::vector<Room::Type> RegularLevel::s_specials;

void RegularLevel::initSpecials() {
    if (s_specials.empty()) {
        s_specials = {
            Room::Type::ARMORY,
            Room::Type::WEAK_FLOOR,
            Room::Type::MAGIC_WELL,
            Room::Type::CRYPT,
            Room::Type::POOL,
            Room::Type::GARDEN,
            Room::Type::LIBRARY,
            Room::Type::TREASURY,
            Room::Type::TRAPS,
            Room::Type::STORAGE,
            Room::Type::STATUE,
            Room::Type::LABORATORY,
            Room::Type::VAULT,
            Room::Type::ALTAR
        };
        for (int i = static_cast<int>(s_specials.size()) - 1; i > 0; --i) {
            int j = Random::Int(i + 1);
            std::swap(s_specials[i], s_specials[j]);
        }
    }
}

bool RegularLevel::build() {
    std::fill_n(map, LENGTH, feeling == Feeling::CHASM ? Terrain::CHASM : Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
    plants.clear();
    wells.clear();
    secretDoors = 0;

    if (!initRooms()) {
        return false;
    }

    int distance = 0;
    int retry = 0;
    int minDistance = static_cast<int>(std::sqrt(rooms.size()));

    do {
        do {
            roomEntrance = Random::element(rooms);
        } while (roomEntrance->width() < 4 || roomEntrance->height() < 4);

        do {
            roomExit = Random::element(rooms);
        } while (roomExit == roomEntrance || roomExit->width() < 4 || roomExit->height() < 4);

        Graph::buildDistanceMap(rooms, roomExit);
        distance = roomEntrance->distance;

        if (retry++ > 10) {
            return false;
        }
    } while (distance < minDistance);

    roomEntrance->type = Room::Type::ENTRANCE;
    roomExit->type = Room::Type::EXIT;

    std::unordered_set<Room*> connected;
    connected.insert(roomEntrance.get());

    Graph::buildDistanceMap(rooms, roomExit);
    auto path = Graph::buildPath(rooms, roomEntrance, roomExit);

    auto room = roomEntrance;
    for (const auto& next : path) {
        room->connect(next.get());
        room = next;
        connected.insert(room.get());
    }

    Graph::setPrice(path, roomEntrance->distance);

    Graph::buildDistanceMap(rooms, roomExit);
    path = Graph::buildPath(rooms, roomEntrance, roomExit);

    room = roomEntrance;
    for (const auto& next : path) {
        room->connect(next.get());
        room = next;
        connected.insert(room.get());
    }

    int nConnected = static_cast<int>(rooms.size() * Random::Float(0.5f, 0.7f));
    std::vector<Room*> connectedVec(connected.begin(), connected.end());
    while (static_cast<int>(connected.size()) < nConnected && !connectedVec.empty()) {
        Room* cr = Random::element(connectedVec);
        if (!cr->neigbours.empty()) {
            Room* or_ = Random::element(cr->neigbours);
            if (connected.find(or_) == connected.end()) {
                cr->connect(or_);
                connected.insert(or_);
                connectedVec.push_back(or_);
            }
        }
    }

    assignRoomType();
    paint();
    paintWater();
    paintGrass();
    placeTraps();
    decorate();

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
    cleanWalls();
    createItems();

    return true;
}

bool RegularLevel::initRooms() {
    rooms.clear();
    split(Rect(0, 0, WIDTH - 1, HEIGHT - 1));

    if (rooms.size() < 8) {
        return false;
    }

    for (size_t i = 0; i < rooms.size() - 1; ++i) {
        for (size_t j = i + 1; j < rooms.size(); ++j) {
            rooms[i]->addNeigbour(rooms[j].get());
        }
    }

    return true;
}

void RegularLevel::split(const Rect& rect) {
    int w = rect.width();
    int h = rect.height();

    if (w > maxRoomSize && h < minRoomSize) {
        int vw = Random::Int(rect.left + 3, rect.right - 3);
        split(Rect(rect.left, rect.top, vw, rect.bottom));
        split(Rect(vw, rect.top, rect.right, rect.bottom));
    } else if (h > maxRoomSize && w < minRoomSize) {
        int vh = Random::Int(rect.top + 3, rect.bottom - 3);
        split(Rect(rect.left, rect.top, rect.right, vh));
        split(Rect(rect.left, vh, rect.right, rect.bottom));
    } else if ((Random::Float() <= ((float)(minRoomSize * minRoomSize) / (float)rect.square()) &&
                w <= maxRoomSize && h <= maxRoomSize) ||
               w < minRoomSize || h < minRoomSize) {
        rooms.push_back(std::make_shared<Room>(rect));
    } else {
        if (Random::Float() < (float)(w - 2) / (float)(w + h - 4)) {
            int vw = Random::Int(rect.left + 3, rect.right - 3);
            split(Rect(rect.left, rect.top, vw, rect.bottom));
            split(Rect(vw, rect.top, rect.right, rect.bottom));
        } else {
            int vh = Random::Int(rect.top + 3, rect.bottom - 3);
            split(Rect(rect.left, rect.top, rect.right, vh));
            split(Rect(rect.left, vh, rect.right, rect.bottom));
        }
    }
}

void RegularLevel::assignRoomType() {
    initSpecials();
    std::vector<Room::Type> specials = s_specials;

    if (Dungeon::depth % 5 == 4 || Dungeon::depth >= 25) {
        auto it = std::find(specials.begin(), specials.end(), Room::Type::WEAK_FLOOR);
        if (it != specials.end()) specials.erase(it);
    }

    int specialRooms = 0;

    for (auto& r : rooms) {
        if (r->type == Room::Type::NONE && r->connected.size() == 1) {
            if (!specials.empty() && r->width() > 3 && r->height() > 3 &&
                Random::Int(specialRooms * specialRooms + 2) == 0) {

                if (pitRoomNeeded) {
                    r->type = Room::Type::PIT;
                    pitRoomNeeded = false;

                    auto removeType = [&](Room::Type t) {
                        auto it = std::find(specials.begin(), specials.end(), t);
                        if (it != specials.end()) specials.erase(it);
                    };
                    removeType(Room::Type::ARMORY);
                    removeType(Room::Type::CRYPT);
                    removeType(Room::Type::LABORATORY);
                    removeType(Room::Type::LIBRARY);
                    removeType(Room::Type::STATUE);
                    removeType(Room::Type::TREASURY);
                    removeType(Room::Type::VAULT);
                    removeType(Room::Type::WEAK_FLOOR);
                } else if (Dungeon::depth % 5 == 2 && std::find(specials.begin(), specials.end(), Room::Type::LABORATORY) != specials.end()) {
                    r->type = Room::Type::LABORATORY;
                } else {
                    int n = static_cast<int>(specials.size());
                    int idx = std::min(Random::Int(n), Random::Int(n));
                    r->type = specials[idx];
                    if (r->type == Room::Type::WEAK_FLOOR) {
                        weakFloorCreated = true;
                    }
                }

                auto itSpec = std::find(s_specials.begin(), s_specials.end(), r->type);
                if (itSpec != s_specials.end()) {
                    Room::Type t = *itSpec;
                    s_specials.erase(itSpec);
                    s_specials.push_back(t);
                }

                auto itCur = std::find(specials.begin(), specials.end(), r->type);
                if (itCur != specials.end()) specials.erase(itCur);
                specialRooms++;
            } else if (Random::Int(2) == 0) {
                std::vector<Room*> nonSpecNeighbors;
                for (Room* n : r->neigbours) {
                    if (n && r->connected.find(n) == r->connected.end() &&
                        n->type != Room::Type::PIT &&
                        std::find(s_specials.begin(), s_specials.end(), n->type) == s_specials.end()) {
                        nonSpecNeighbors.push_back(n);
                    }
                }
                if (nonSpecNeighbors.size() > 1) {
                    r->connect(Random::element(nonSpecNeighbors));
                }
            }
        }
    }

    int count = 0;
    for (auto& r : rooms) {
        if (r->type == Room::Type::NONE) {
            int connections = static_cast<int>(r->connected.size());
            if (connections == 0) {
                // Stays NONE
            } else if (Random::Int(connections * connections) == 0) {
                r->type = Room::Type::STANDARD;
                count++;
            } else {
                r->type = Room::Type::TUNNEL;
            }
        }
    }

    while (count < 4) {
        Room* r = randomRoom(Room::Type::TUNNEL, 1);
        if (r) {
            r->type = Room::Type::STANDARD;
            count++;
        } else {
            break;
        }
    }
}

Room* RegularLevel::randomRoom(Room::Type type, int tries) {
    for (int i = 0; i < tries; ++i) {
        auto r = Random::element(rooms);
        if (r && r->type == type) {
            return r.get();
        }
    }
    return nullptr;
}

void RegularLevel::placeDoors(Room* r) {
    for (auto& pair : r->connected) {
        Room* n = pair.first;
        auto& door = pair.second;
        if (!door) {
            Rect i = r->intersect(*n);
            if (i.width() == 0) {
                door = std::make_shared<Room::Door>(
                    i.left,
                    Random::Int(i.top + 1, i.bottom)
                );
            } else {
                door = std::make_shared<Room::Door>(
                    Random::Int(i.left + 1, i.right),
                    i.top
                );
            }
            n->connected[r] = door;
        }
    }
}

bool RegularLevel::joinRooms(Room* r, Room* n) {
    if (r->type != Room::Type::STANDARD || n->type != Room::Type::STANDARD) {
        return false;
    }

    Rect w = r->intersect(*n);
    if (w.left == w.right) {
        if (w.bottom - w.top < 3) {
            return false;
        }
        if (w.height() == std::max(r->height(), n->height())) {
            return false;
        }
        if (r->width() + n->width() > maxRoomSize) {
            return false;
        }

        w.top += 1;
        w.right++;

        Painter::fill(this, w.left, w.top, 1, w.height(), Terrain::EMPTY);
    } else {
        if (w.right - w.left < 3) {
            return false;
        }
        if (w.width() == std::max(r->width(), n->width())) {
            return false;
        }
        if (r->height() + n->height() > maxRoomSize) {
            return false;
        }

        w.left += 1;
        w.bottom++;

        Painter::fill(this, w.left, w.top, w.width(), 1, Terrain::EMPTY);
    }
    return true;
}

void RegularLevel::paintDoors(Room* r) {
    for (auto& pair : r->connected) {
        Room* n = pair.first;
        auto& d = pair.second;
        if (!d) continue;

        if (joinRooms(r, n)) {
            continue;
        }

        int doorPos = d->x + d->y * WIDTH;

        switch (d->type) {
        case Room::Door::Type::EMPTY:
            map[doorPos] = Terrain::EMPTY;
            break;
        case Room::Door::Type::TUNNEL:
            map[doorPos] = tunnelTile();
            break;
        case Room::Door::Type::REGULAR:
            if (Dungeon::depth <= 1) {
                map[doorPos] = Terrain::DOOR;
            } else {
                bool isSecret = (Dungeon::depth < 6 ? Random::Int(12 - Dungeon::depth) : Random::Int(6)) == 0;
                map[doorPos] = isSecret ? Terrain::SECRET_DOOR : Terrain::DOOR;
                if (isSecret) {
                    secretDoors++;
                }
            }
            break;
        case Room::Door::Type::UNLOCKED:
            map[doorPos] = Terrain::DOOR;
            break;
        case Room::Door::Type::HIDDEN:
            map[doorPos] = Terrain::SECRET_DOOR;
            secretDoors++;
            break;
        case Room::Door::Type::BARRICADE:
            map[doorPos] = (Random::Int(3) == 0) ? Terrain::BOOKSHELF : Terrain::BARRICADE;
            break;
        case Room::Door::Type::LOCKED:
            map[doorPos] = Terrain::LOCKED_DOOR;
            break;
        }
    }
}

void RegularLevel::paint() {
    for (auto& r : rooms) {
        if (r->type != Room::Type::NONE) {
            placeDoors(r.get());
            Painter::paintRoom(this, r.get());
        } else {
            if (feeling == Feeling::CHASM && Random::Int(2) == 0) {
                Painter::fill(this, *r, Terrain::WALL);
            }
        }
    }

    for (auto& r : rooms) {
        paintDoors(r.get());
    }
}

void RegularLevel::placeTraps() {
    int nTraps = (Dungeon::depth <= 1) ? 0 : Random::Int(1, static_cast<int>(rooms.size()) + Dungeon::depth);
    float trapChances[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    for (int i = 0; i < nTraps; ++i) {
        int trapPos = Random::Int(LENGTH);
        if (map[trapPos] == Terrain::EMPTY) {
            int trapType = Random::Chances(trapChances, 9);
            switch (trapType) {
            case 0: map[trapPos] = Terrain::SECRET_FIRE_TRAP; break;
            case 1: map[trapPos] = Terrain::SECRET_PARALYTIC_TRAP; break;
            case 2: map[trapPos] = Terrain::SECRET_TOXIC_TRAP; break;
            case 3: map[trapPos] = Terrain::SECRET_POISON_TRAP; break;
            case 4: map[trapPos] = Terrain::SECRET_ALARM_TRAP; break;
            case 5: map[trapPos] = Terrain::SECRET_LIGHTNING_TRAP; break;
            case 6: map[trapPos] = Terrain::SECRET_GRIPPING_TRAP; break;
            case 7: map[trapPos] = Terrain::SECRET_SUMMONING_TRAP; break;
            case 8: map[trapPos] = Terrain::SECRET_TELEPORTATION_TRAP; break;
            }
        }
    }
}

int RegularLevel::randomDropCell() {
    for (int attempts = 0; attempts < 100; ++attempts) {
        Room* r = randomRoom(Room::Type::STANDARD, 1);
        if (r) {
            int pos = r->random();
            if (isPassable(pos) && map[pos] != Terrain::EMPTY_WELL && getHeap(pos) == nullptr) {
                return pos;
            }
        }
    }
    for (int attempts = 0; attempts < 100; ++attempts) {
        int pos = Random::Int(LENGTH);
        if (isPassable(pos) && map[pos] != Terrain::EMPTY_WELL && getHeap(pos) == nullptr) {
            return pos;
        }
    }
    return entrance;
}

void RegularLevel::createItems() {
    int nItems = 3;
    while (Random::Float() < 0.4f) {
        nItems++;
    }

    for (int i = 0; i < nItems; ++i) {
        int cell = randomDropCell();
        Heap::Type type = Heap::Type::HEAP;
        switch (Random::Int(20)) {
        case 0:
            type = Heap::Type::SKELETON;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            type = Heap::Type::CHEST;
            break;
        case 5:
            type = (Dungeon::depth > 1) ? Heap::Type::MIMIC : Heap::Type::CHEST;
            break;
        default:
            type = Heap::Type::HEAP;
            break;
        }

        float roll = Random::Float();
        std::unique_ptr<Item> item;
        if (roll < 0.25f) {
            item = std::make_unique<Potion>(static_cast<Potion::PotionType>(Random::Int(12)));
        } else if (roll < 0.50f) {
            item = std::make_unique<Scroll>(static_cast<Scroll::ScrollType>(Random::Int(12)));
        } else if (roll < 0.70f) {
            item = std::make_unique<Food>();
        } else if (roll < 0.85f) {
            item = std::make_unique<Gold>(Random::IntRange(10 * Dungeon::depth, 30 * Dungeon::depth));
        } else if (roll < 0.93f) {
            auto w = std::make_unique<Weapon>("Sword", 3, 8 + Dungeon::depth, 10 + Dungeon::depth);
            w->random();
            item = std::move(w);
        } else {
            auto a = std::make_unique<Armor>("Armor", 1 + Dungeon::depth / 5, 10 + Dungeon::depth);
            a->random();
            item = std::move(a);
        }

        Heap* h = drop(std::move(item), cell);
        if (h) h->type = type;
    }

    // Spawn puzzle / special room required items (Keys, Invisibility, Liquid Flame, Levitation, Torch)
    for (auto& itm : itemsToSpawn) {
        int cell = randomDropCell();
        if (itm && itm->name == "Torch") {
            cell = entrance;
        }
        drop(std::move(itm), cell);
    }
    itemsToSpawn.clear();
}

} // namespace pd
