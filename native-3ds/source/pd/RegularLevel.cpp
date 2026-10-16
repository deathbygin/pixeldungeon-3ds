#include "pd/RegularLevel.h"
#include "pd/Graph.h"
#include "pd/Painter.h"
#include "pd/Random.h"
#include "pd/Dungeon.h"
#include "pd/Item.h"
#include <cmath>
#include <unordered_set>

namespace pd {

bool RegularLevel::build() {
    std::fill_n(map, LENGTH, feeling == Feeling::CHASM ? Terrain::CHASM : Terrain::WALL);
    std::fill_n(visited, LENGTH, false);
    std::fill_n(fieldOfView, LENGTH, false);
    heaps.clear();
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

void RegularLevel::createItems() {
    for (size_t i = 0; i < rooms.size(); ++i) {
        const auto& r = rooms[i];
        if (!r || r->type != Room::Type::STANDARD) continue;

        int cell = r->centerY() * WIDTH + r->centerX();
        if (isPassable(cell) && !getHeap(cell)) {
            float roll = Random::Float();
            if (roll < 0.25f) {
                auto chest = std::make_unique<Heap>(cell, Heap::Type::CHEST);
                if (Random::Float() < 0.5f) {
                    chest->drop(std::make_unique<Potion>(Potion::PotionType::HEALING));
                } else {
                    chest->drop(std::make_unique<Scroll>(Scroll::ScrollType::UPGRADE));
                }
                heaps.push_back(std::move(chest));
            } else if (roll < 0.45f) {
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
}

} // namespace pd
