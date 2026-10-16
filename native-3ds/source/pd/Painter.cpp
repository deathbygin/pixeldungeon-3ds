#include "pd/Painter.h"
#include "pd/Level.h"
#include "pd/Terrain.h"
#include "pd/Random.h"
#include "pd/Dungeon.h"
#include "pd/Item.h"
#include "pd/Mob.h"
#include "pd/Heap.h"
#include "pd/Actor.h"
#include <algorithm>
#include <cstdlib>

namespace pd {

void Painter::set(Level* level, int cell, int value) {
    if (!level) return;
    level->set(cell, value);
}

void Painter::set(Level* level, int x, int y, int value) {
    if (!level) return;
    level->set(x, y, value);
}

void Painter::set(Level* level, Point p, int value) {
    if (!level) return;
    level->set(p.x, p.y, value);
}

void Painter::fill(Level* level, int x, int y, int w, int h, int value) {
    if (!level) return;
    for (int i = y; i < y + h; ++i) {
        for (int j = x; j < x + w; ++j) {
            level->set(j, i, value);
        }
    }
}

void Painter::fill(Level* level, const Rect& rect, int value) {
    fill(level, rect.left, rect.top, rect.width() + 1, rect.height() + 1, value);
}

void Painter::fill(Level* level, const Rect& rect, int m, int value) {
    fill(level, rect.left + m, rect.top + m, rect.width() + 1 - m * 2, rect.height() + 1 - m * 2, value);
}

void Painter::fill(Level* level, const Rect& rect, int l, int t, int r, int b, int value) {
    fill(level, rect.left + l, rect.top + t, rect.width() + 1 - (l + r), rect.height() + 1 - (t + b), value);
}

Point Painter::drawInside(Level* level, const Room& room, Point from, int n, int value) {
    Point step(0, 0);
    if (from.x == room.left) {
        step.set(+1, 0);
    } else if (from.x == room.right) {
        step.set(-1, 0);
    } else if (from.y == room.top) {
        step.set(0, +1);
    } else if (from.y == room.bottom) {
        step.set(0, -1);
    }

    Point p = from;
    p.offset(step);
    for (int i = 0; i < n; ++i) {
        if (value != -1) {
            set(level, p, value);
        }
        p.offset(step);
    }
    return p;
}

void Painter::paintRoom(Level* level, Room* room) {
    if (!room) return;
    switch (room->type) {
    case Room::Type::STANDARD:
        paintStandard(level, room);
        break;
    case Room::Type::TUNNEL:
        paintTunnel(level, room);
        break;
    case Room::Type::PASSAGE:
        paintPassage(level, room);
        break;
    case Room::Type::ENTRANCE:
        paintEntrance(level, room);
        break;
    case Room::Type::EXIT:
        paintExit(level, room);
        break;
    case Room::Type::BOSS_EXIT:
        paintBossExit(level, room);
        break;
    case Room::Type::RAT_KING:
        paintRatKing(level, room);
        break;
    case Room::Type::ARMORY:
        paintArmory(level, room);
        break;
    case Room::Type::LABORATORY:
        paintLaboratory(level, room);
        break;
    case Room::Type::LIBRARY:
        paintLibrary(level, room);
        break;
    case Room::Type::GARDEN:
        paintGarden(level, room);
        break;
    case Room::Type::MAGIC_WELL:
        paintMagicWell(level, room);
        break;
    case Room::Type::POOL:
        paintPool(level, room);
        break;
    case Room::Type::PIT:
        paintPit(level, room);
        break;
    case Room::Type::STORAGE:
        paintStorage(level, room);
        break;
    case Room::Type::STATUE:
        paintStatue(level, room);
        break;
    case Room::Type::TRAPS:
        paintTraps(level, room);
        break;
    case Room::Type::TREASURY:
        paintTreasury(level, room);
        break;
    case Room::Type::VAULT:
        paintVault(level, room);
        break;
    case Room::Type::WEAK_FLOOR:
        paintWeakFloor(level, room);
        break;
    case Room::Type::ALTAR:
        paintAltar(level, room);
        break;
    case Room::Type::CRYPT:
        paintCrypt(level, room);
        break;
    default:
        paintStandard(level, room);
        break;
    }
}

void Painter::paintEntrance(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    for (auto& pair : room->connected) {
        if (pair.second) {
            pair.second->set(Room::Door::Type::REGULAR);
        }
    }

    level->entrance = room->random(1);
    set(level, level->entrance, Terrain::ENTRANCE);
}

void Painter::paintExit(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    for (auto& pair : room->connected) {
        if (pair.second) {
            pair.second->set(Room::Door::Type::REGULAR);
        }
    }

    level->exit = room->random(1);
    set(level, level->exit, Terrain::EXIT);
}

void Painter::paintBossExit(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    for (auto& pair : room->connected) {
        if (pair.second) {
            pair.second->set(Room::Door::Type::REGULAR);
        }
    }

    level->exit = room->top * Level::WIDTH + (room->left + room->right) / 2;
    set(level, level->exit, Terrain::LOCKED_EXIT);
}

static void addRatKingChest(Level* level, int pos, int door) {
    if (pos == door - 1 || pos == door + 1 ||
        pos == door - Level::WIDTH || pos == door + Level::WIDTH) {
        return;
    }
    auto chest = std::make_unique<Heap>(pos, Heap::Type::CHEST);
    chest->drop(std::make_unique<Gold>(Random::IntRange(1, 5)));
    level->heaps.push_back(std::move(chest));
}

void Painter::paintRatKing(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY_SP);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::HIDDEN);
        int door = entrance->x + entrance->y * Level::WIDTH;

        for (int i = room->left + 1; i < room->right; ++i) {
            addRatKingChest(level, (room->top + 1) * Level::WIDTH + i, door);
            addRatKingChest(level, (room->bottom - 1) * Level::WIDTH + i, door);
        }
        for (int i = room->top + 2; i < room->bottom - 1; ++i) {
            addRatKingChest(level, i * Level::WIDTH + room->left + 1, door);
            addRatKingChest(level, i * Level::WIDTH + room->right - 1, door);
        }
    }
}

void Painter::paintStandard(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    for (auto& pair : room->connected) {
        if (pair.second) {
            pair.second->set(Room::Door::Type::REGULAR);
        }
    }

    bool bossLevel = (Dungeon::depth % 5 == 0);
    if (!bossLevel && Random::Int(5) == 0) {
        switch (Random::Int(6)) {
        case 0:
            if (level->feeling != Level::Feeling::GRASS) {
                if (std::min(room->width(), room->height()) >= 4 && std::max(room->width(), room->height()) >= 6) {
                    paintGraveyard(level, room);
                    return;
                }
                break;
            }
            [[fallthrough]];
        case 1:
            if (Dungeon::depth > 1) {
                paintBurned(level, room);
                return;
            }
            break;
        case 2:
            if (std::max(room->width(), room->height()) >= 4) {
                paintStriped(level, room);
                return;
            }
            break;
        case 3:
            if (room->width() >= 6 && room->height() >= 6) {
                paintStudy(level, room);
                return;
            }
            break;
        case 4:
            if (level->feeling != Level::Feeling::WATER) {
                if (room->connected.size() == 2 && room->width() >= 4 && room->height() >= 4) {
                    paintBridge(level, room);
                    return;
                }
                break;
            }
            [[fallthrough]];
        case 5:
            if (!bossLevel && ((Dungeon::depth + 1) % 5 != 0) && std::min(room->width(), room->height()) >= 5) {
                paintFissure(level, room);
                return;
            }
            break;
        }
    }

    fill(level, *room, 1, Terrain::EMPTY);
}

void Painter::paintBurned(Level* level, Room* room) {
    for (int i = room->top + 1; i < room->bottom; ++i) {
        for (int j = room->left + 1; j < room->right; ++j) {
            int t = Terrain::EMBERS;
            switch (Random::Int(5)) {
            case 0:
                t = Terrain::EMPTY;
                break;
            case 1:
                t = Terrain::FIRE_TRAP;
                break;
            case 2:
                t = Terrain::SECRET_FIRE_TRAP;
                break;
            case 3:
                t = Terrain::INACTIVE_TRAP;
                break;
            }
            set(level, j, i, t);
        }
    }
}

void Painter::paintGraveyard(Level* level, Room* room) {
    fill(level, room->left + 1, room->top + 1, room->width() - 1, room->height() - 1, Terrain::GRASS);
    // Floor is grass, graves can be populated during item creation
}

void Painter::paintStriped(Level* level, Room* room) {
    fill(level, room->left + 1, room->top + 1, room->width() - 1, room->height() - 1, Terrain::EMPTY_SP);

    if (room->width() > room->height()) {
        for (int i = room->left + 2; i < room->right; i += 2) {
            fill(level, i, room->top + 1, 1, room->height() - 1, Terrain::HIGH_GRASS);
        }
    } else {
        for (int i = room->top + 2; i < room->bottom; i += 2) {
            fill(level, room->left + 1, i, room->width() - 1, 1, Terrain::HIGH_GRASS);
        }
    }
}

void Painter::paintStudy(Level* level, Room* room) {
    fill(level, room->left + 1, room->top + 1, room->width() - 1, room->height() - 1, Terrain::BOOKSHELF);
    fill(level, room->left + 2, room->top + 2, room->width() - 3, room->height() - 3, Terrain::EMPTY_SP);

    for (const auto& pair : room->connected) {
        if (pair.second) {
            const auto& door = *pair.second;
            if (door.x == room->left) {
                set(level, door.x + 1, door.y, Terrain::EMPTY);
            } else if (door.x == room->right) {
                set(level, door.x - 1, door.y, Terrain::EMPTY);
            } else if (door.y == room->top) {
                set(level, door.x, door.y + 1, Terrain::EMPTY);
            } else if (door.y == room->bottom) {
                set(level, door.x, door.y - 1, Terrain::EMPTY);
            }
        }
    }

    set(level, room->center(), Terrain::PEDESTAL);
}

void Painter::paintBridge(Level* level, Room* room) {
    bool bossLevel = (Dungeon::depth % 5 == 0);
    fill(level, room->left + 1, room->top + 1, room->width() - 1, room->height() - 1,
         (!bossLevel && ((Dungeon::depth + 1) % 5 != 0) && Random::Int(3) == 0) ?
         Terrain::CHASM : Terrain::WATER);

    Point door1(-1, -1);
    Point door2(-1, -1);
    for (const auto& pair : room->connected) {
        if (pair.second) {
            if (door1.x == -1) {
                door1 = *pair.second;
            } else {
                door2 = *pair.second;
            }
        }
    }

    if (door1.x == -1 || door2.x == -1) return;

    if ((door1.x == room->left && door2.x == room->right) ||
        (door1.x == room->right && door2.x == room->left)) {
        int s = room->width() / 2;
        drawInside(level, *room, door1, s, Terrain::EMPTY_SP);
        drawInside(level, *room, door2, s, Terrain::EMPTY_SP);
        fill(level, room->center().x, std::min(door1.y, door2.y), 1, std::abs(door1.y - door2.y) + 1, Terrain::EMPTY_SP);
    } else if ((door1.y == room->top && door2.y == room->bottom) ||
               (door1.y == room->bottom && door2.y == room->top)) {
        int s = room->height() / 2;
        drawInside(level, *room, door1, s, Terrain::EMPTY_SP);
        drawInside(level, *room, door2, s, Terrain::EMPTY_SP);
        fill(level, std::min(door1.x, door2.x), room->center().y, std::abs(door1.x - door2.x) + 1, 1, Terrain::EMPTY_SP);
    } else if (door1.x == door2.x) {
        fill(level, door1.x == room->left ? room->left + 1 : room->right - 1,
             std::min(door1.y, door2.y), 1, std::abs(door1.y - door2.y) + 1, Terrain::EMPTY_SP);
    } else if (door1.y == door2.y) {
        fill(level, std::min(door1.x, door2.x),
             door1.y == room->top ? room->top + 1 : room->bottom - 1,
             std::abs(door1.x - door2.x) + 1, 1, Terrain::EMPTY_SP);
    } else if (door1.y == room->top || door1.y == room->bottom) {
        drawInside(level, *room, door1, std::abs(door1.y - door2.y), Terrain::EMPTY_SP);
        drawInside(level, *room, door2, std::abs(door1.x - door2.x), Terrain::EMPTY_SP);
    } else if (door1.x == room->left || door1.x == room->right) {
        drawInside(level, *room, door1, std::abs(door1.x - door2.x), Terrain::EMPTY_SP);
        drawInside(level, *room, door2, std::abs(door1.y - door2.y), Terrain::EMPTY_SP);
    }
}

void Painter::paintFissure(Level* level, Room* room) {
    fill(level, room->left + 1, room->top + 1, room->width() - 1, room->height() - 1, Terrain::EMPTY);

    for (int i = room->top + 2; i < room->bottom - 1; ++i) {
        for (int j = room->left + 2; j < room->right - 1; ++j) {
            int v = std::min(i - room->top, room->bottom - i);
            int h = std::min(j - room->left, room->right - j);
            if (std::min(v, h) > 2 || Random::Int(2) == 0) {
                set(level, j, i, Terrain::CHASM);
            }
        }
    }
}

void Painter::paintTunnel(Level* level, Room* room) {
    int floor = level->tunnelTile();
    Point c = room->center();

    if (room->width() > room->height() || (room->width() == room->height() && Random::Int(2) == 0)) {
        int from = room->right - 1;
        int to = room->left + 1;

        for (const auto& pair : room->connected) {
            if (!pair.second) continue;
            const auto& door = *pair.second;
            int step = (door.y < c.y) ? +1 : -1;

            if (door.x == room->left) {
                from = room->left + 1;
                for (int i = door.y; i != c.y; i += step) {
                    set(level, from, i, floor);
                }
            } else if (door.x == room->right) {
                to = room->right - 1;
                for (int i = door.y; i != c.y; i += step) {
                    set(level, to, i, floor);
                }
            } else {
                if (door.x < from) from = door.x;
                if (door.x > to) to = door.x;
                for (int i = door.y + step; i != c.y; i += step) {
                    set(level, door.x, i, floor);
                }
            }
        }

        for (int i = from; i <= to; ++i) {
            set(level, i, c.y, floor);
        }
    } else {
        int from = room->bottom - 1;
        int to = room->top + 1;

        for (const auto& pair : room->connected) {
            if (!pair.second) continue;
            const auto& door = *pair.second;
            int step = (door.x < c.x) ? +1 : -1;

            if (door.y == room->top) {
                from = room->top + 1;
                for (int i = door.x; i != c.x; i += step) {
                    set(level, i, from, floor);
                }
            } else if (door.y == room->bottom) {
                to = room->bottom - 1;
                for (int i = door.x; i != c.x; i += step) {
                    set(level, i, to, floor);
                }
            } else {
                if (door.y < from) from = door.y;
                if (door.y > to) to = door.y;
                for (int i = door.x + step; i != c.x; i += step) {
                    set(level, i, door.y, floor);
                }
            }
        }

        for (int i = from; i <= to; ++i) {
            set(level, c.x, i, floor);
        }
    }

    for (auto& pair : room->connected) {
        if (pair.second) {
            pair.second->set(Room::Door::Type::TUNNEL);
        }
    }
}

int Painter::xy2p(const Room& room, const Point& xy, int pasWidth, int pasHeight) {
    if (xy.y == room.top) {
        return (xy.x - room.left - 1);
    } else if (xy.x == room.right) {
        return (xy.y - room.top - 1) + pasWidth;
    } else if (xy.y == room.bottom) {
        return (room.right - xy.x - 1) + pasWidth + pasHeight;
    } else {
        if (xy.y == room.top + 1) {
            return 0;
        } else {
            return (room.bottom - xy.y - 1) + pasWidth * 2 + pasHeight;
        }
    }
}

Point Painter::p2xy(const Room& room, int p, int pasWidth, int pasHeight) {
    if (p < pasWidth) {
        return Point(room.left + 1 + p, room.top + 1);
    } else if (p < pasWidth + pasHeight) {
        return Point(room.right - 1, room.top + 1 + (p - pasWidth));
    } else if (p < pasWidth * 2 + pasHeight) {
        return Point(room.right - 1 - (p - (pasWidth + pasHeight)), room.bottom - 1);
    } else {
        return Point(room.left + 1, room.bottom - 1 - (p - (pasWidth * 2 + pasHeight)));
    }
}

void Painter::paintPassage(Level* level, Room* room) {
    int pasWidth = room->width() - 2;
    int pasHeight = room->height() - 2;
    if (pasWidth <= 0 || pasHeight <= 0) {
        paintTunnel(level, room);
        return;
    }

    int floor = level->tunnelTile();

    std::vector<int> joints;
    for (const auto& pair : room->connected) {
        if (pair.second) {
            joints.push_back(xy2p(*room, *pair.second, pasWidth, pasHeight));
        }
    }

    if (joints.empty()) {
        paintTunnel(level, room);
        return;
    }

    std::sort(joints.begin(), joints.end());

    int nJoints = static_cast<int>(joints.size());
    int perimeter = pasWidth * 2 + pasHeight * 2;

    int start = 0;
    int maxD = joints[0] + perimeter - joints[nJoints - 1];
    for (int i = 1; i < nJoints; ++i) {
        int d = joints[i] - joints[i - 1];
        if (d > maxD) {
            maxD = d;
            start = i;
        }
    }

    int end = (start + nJoints - 1) % nJoints;

    int p = joints[start];
    do {
        set(level, p2xy(*room, p, pasWidth, pasHeight), floor);
        p = (p + 1) % perimeter;
    } while (p != joints[end]);

    set(level, p2xy(*room, p, pasWidth, pasHeight), floor);

    for (auto& pair : room->connected) {
        if (pair.second) {
            pair.second->set(Room::Door::Type::TUNNEL);
        }
    }
}

// =============================================================================
// Special Room Painters
// =============================================================================

void Painter::paintArmory(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    auto entrance = room->entrance();
    if (entrance) {
        Point statue(-1, -1);
        if (entrance->x == room->left) {
            statue = Point(room->right - 1, Random::Int(2) == 0 ? room->top + 1 : room->bottom - 1);
        } else if (entrance->x == room->right) {
            statue = Point(room->left + 1, Random::Int(2) == 0 ? room->top + 1 : room->bottom - 1);
        } else if (entrance->y == room->top) {
            statue = Point(Random::Int(2) == 0 ? room->left + 1 : room->right - 1, room->bottom - 1);
        } else if (entrance->y == room->bottom) {
            statue = Point(Random::Int(2) == 0 ? room->left + 1 : room->right - 1, room->top + 1);
        }
        if (statue.x != -1) {
            set(level, statue, Terrain::STATUE);
        }

        entrance->set(Room::Door::Type::LOCKED);
    }

    int n = 3 + (Random::Int(4) == 0 ? 1 : 0);
    for (int i = 0; i < n; ++i) {
        int pos;
        int tries = 0;
        do {
            pos = room->random();
        } while ((level->map[pos] != Terrain::EMPTY || level->getHeap(pos) != nullptr) && ++tries < 50);

        std::unique_ptr<Item> prize;
        if (Random::Int(2) == 0) {
            auto w = std::make_unique<Weapon>("Weapon", 3, 8 + Dungeon::depth, 10 + Dungeon::depth);
            w->random();
            prize = std::move(w);
        } else {
            auto a = std::make_unique<Armor>("Armor", 1 + Dungeon::depth / 5, 10 + Dungeon::depth);
            a->random();
            prize = std::move(a);
        }
        level->drop(std::move(prize), pos);
    }

    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));
}

void Painter::paintLaboratory(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY_SP);

    auto entrance = room->entrance();
    if (entrance) {
        Point pot(-1, -1);
        if (entrance->x == room->left) {
            pot = Point(room->right - 1, Random::Int(2) == 0 ? room->top + 1 : room->bottom - 1);
        } else if (entrance->x == room->right) {
            pot = Point(room->left + 1, Random::Int(2) == 0 ? room->top + 1 : room->bottom - 1);
        } else if (entrance->y == room->top) {
            pot = Point(Random::Int(2) == 0 ? room->left + 1 : room->right - 1, room->bottom - 1);
        } else if (entrance->y == room->bottom) {
            pot = Point(Random::Int(2) == 0 ? room->left + 1 : room->right - 1, room->top + 1);
        }
        if (pot.x != -1) {
            set(level, pot, Terrain::ALCHEMY);
        }

        entrance->set(Room::Door::Type::LOCKED);
    }

    int n = Random::IntRange(2, 3);
    for (int i = 0; i < n; ++i) {
        int pos;
        int tries = 0;
        do {
            pos = room->random();
        } while ((level->map[pos] != Terrain::EMPTY_SP || level->getHeap(pos) != nullptr) && ++tries < 50);

        auto prize = level->itemToSpawnAsPrize();
        if (!prize) {
            prize = std::make_unique<Potion>(static_cast<Potion::PotionType>(Random::Int(12)));
        }
        level->drop(std::move(prize), pos);
    }

    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));
}

void Painter::paintLibrary(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    auto entrance = room->entrance();
    if (entrance) {
        Point a(-1, -1), b(-1, -1);
        if (entrance->x == room->left) {
            a = Point(room->left + 1, entrance->y - 1);
            b = Point(room->left + 1, entrance->y + 1);
            fill(level, room->right - 1, room->top + 1, 1, room->height() - 1, Terrain::BOOKSHELF);
        } else if (entrance->x == room->right) {
            a = Point(room->right - 1, entrance->y - 1);
            b = Point(room->right - 1, entrance->y + 1);
            fill(level, room->left + 1, room->top + 1, 1, room->height() - 1, Terrain::BOOKSHELF);
        } else if (entrance->y == room->top) {
            a = Point(entrance->x + 1, room->top + 1);
            b = Point(entrance->x - 1, room->top + 1);
            fill(level, room->left + 1, room->bottom - 1, room->width() - 1, 1, Terrain::BOOKSHELF);
        } else if (entrance->y == room->bottom) {
            a = Point(entrance->x + 1, room->bottom - 1);
            b = Point(entrance->x - 1, room->bottom - 1);
            fill(level, room->left + 1, room->top + 1, room->width() - 1, 1, Terrain::BOOKSHELF);
        }
        if (a.x != -1 && level->get(a.x, a.y) == Terrain::EMPTY) set(level, a, Terrain::STATUE);
        if (b.x != -1 && level->get(b.x, b.y) == Terrain::EMPTY) set(level, b, Terrain::STATUE);

        entrance->set(Room::Door::Type::LOCKED);
    }

    int n = Random::IntRange(2, 3);
    for (int i = 0; i < n; ++i) {
        int pos;
        int tries = 0;
        do {
            pos = room->random();
        } while ((level->map[pos] != Terrain::EMPTY || level->getHeap(pos) != nullptr) && ++tries < 50);

        auto prize = level->itemToSpawnAsPrize();
        if (!prize) {
            prize = std::make_unique<Scroll>(static_cast<Scroll::ScrollType>(Random::Int(12)));
        }
        level->drop(std::move(prize), pos);
    }

    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));
}

void Painter::paintGarden(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::HIGH_GRASS);
    fill(level, *room, 2, Terrain::GRASS);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::REGULAR);
    }

    int bushes = (Random::Int(5) == 0 ? 2 : 1);
    for (int i = 0; i < bushes; ++i) {
        int pos = room->random();
        set(level, pos, Terrain::GRASS);
        level->drop(std::make_unique<PlantSeed>(PlantSeed::PlantType::SUNGRASS), pos);
    }
}

void Painter::paintMagicWell(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    Point c = room->center();
    set(level, c.x, c.y, Terrain::WELL);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::REGULAR);
    }
}

void Painter::paintPool(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::WATER);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::REGULAR);

        int x = -1, y = -1;
        if (entrance->x == room->left) {
            x = room->right - 1;
            y = room->top + room->height() / 2;
        } else if (entrance->x == room->right) {
            x = room->left + 1;
            y = room->top + room->height() / 2;
        } else if (entrance->y == room->top) {
            x = room->left + room->width() / 2;
            y = room->bottom - 1;
        } else if (entrance->y == room->bottom) {
            x = room->left + room->width() / 2;
            y = room->top + 1;
        }

        int pos = x + y * Level::WIDTH;
        set(level, pos, Terrain::PEDESTAL);

        auto prize = level->itemToSpawnAsPrize();
        if (!prize) {
            auto w = std::make_unique<Weapon>("Weapon", 4, 10 + Dungeon::depth, 12 + Dungeon::depth);
            w->random();
            prize = std::move(w);
        }

        Heap* h = level->drop(std::move(prize), pos);
        if (h && Random::Int(3) == 0) {
            h->type = Heap::Type::CHEST;
        }
    }

    level->addItemToSpawn(std::make_unique<Potion>(Potion::PotionType::INVISIBILITY));

    // Spawn 3 Giant Piranhas in water
    for (int i = 0; i < 3; ++i) {
        int pPos;
        int tries = 0;
        do {
            pPos = room->random();
        } while ((level->map[pPos] != Terrain::WATER || Actor::findChar(pPos) != nullptr) && ++tries < 50);

        if (level->map[pPos] == Terrain::WATER && Actor::findChar(pPos) == nullptr) {
            auto piranha = std::make_unique<Piranha>();
            piranha->place(pPos);
            Actor::occupyCell(piranha.get());
            Actor::add(piranha.get());
            Dungeon::mobs.push_back(std::move(piranha));
        }
    }
}

void Painter::paintPit(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::LOCKED);

        Point well(-1, -1);
        if (entrance->x == room->left) {
            well = Point(room->right - 1, Random::Int(2) == 0 ? room->top + 1 : room->bottom - 1);
        } else if (entrance->x == room->right) {
            well = Point(room->left + 1, Random::Int(2) == 0 ? room->top + 1 : room->bottom - 1);
        } else if (entrance->y == room->top) {
            well = Point(Random::Int(2) == 0 ? room->left + 1 : room->right - 1, room->bottom - 1);
        } else if (entrance->y == room->bottom) {
            well = Point(Random::Int(2) == 0 ? room->left + 1 : room->right - 1, room->top + 1);
        }
        if (well.x != -1) {
            set(level, well, Terrain::EMPTY_WELL);
        }
    }

    int remains = room->random();
    int tries = 0;
    while (level->map[remains] == Terrain::EMPTY_WELL && ++tries < 50) {
        remains = room->random();
    }

    Heap* skel = level->drop(std::make_unique<IronKey>(Dungeon::depth), remains);
    if (skel) {
        skel->type = Heap::Type::SKELETON;
        if (Random::Int(2) == 0) {
            auto w = std::make_unique<Weapon>("Sword", 3, 8 + Dungeon::depth, 11 + Dungeon::depth);
            w->random();
            skel->drop(std::move(w));
        } else {
            auto a = std::make_unique<Armor>("Armor", 2, 12 + Dungeon::depth);
            a->random();
            skel->drop(std::move(a));
        }
        skel->drop(std::make_unique<Potion>(Potion::PotionType::HEALING));
    }
}

void Painter::paintStorage(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY_SP);

    int n = Random::IntRange(3, 4);
    for (int i = 0; i < n; ++i) {
        int pos;
        int tries = 0;
        do {
            pos = room->random();
        } while ((level->map[pos] != Terrain::EMPTY_SP || level->getHeap(pos) != nullptr) && ++tries < 50);

        auto prize = level->itemToSpawnAsPrize();
        if (!prize) {
            float r = Random::Float();
            if (r < 0.35f) {
                prize = std::make_unique<Food>();
            } else if (r < 0.65f) {
                prize = std::make_unique<Potion>(static_cast<Potion::PotionType>(Random::Int(12)));
            } else if (r < 0.85f) {
                prize = std::make_unique<Scroll>(static_cast<Scroll::ScrollType>(Random::Int(12)));
            } else {
                prize = std::make_unique<Gold>(Random::IntRange(10, 35));
            }
        }
        level->drop(std::move(prize), pos);
    }

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::BARRICADE);
    }

    level->addItemToSpawn(std::make_unique<Potion>(Potion::PotionType::LIQUID_FLAME));
}

void Painter::paintStatue(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    int cx = room->centerX();
    int cy = room->centerY();

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::LOCKED);

        if (entrance->x == room->left) {
            fill(level, room->right - 1, room->top + 1, 1, room->height() - 1, Terrain::STATUE);
            cx = room->right - 2;
        } else if (entrance->x == room->right) {
            fill(level, room->left + 1, room->top + 1, 1, room->height() - 1, Terrain::STATUE);
            cx = room->left + 2;
        } else if (entrance->y == room->top) {
            fill(level, room->left + 1, room->bottom - 1, room->width() - 1, 1, Terrain::STATUE);
            cy = room->bottom - 2;
        } else if (entrance->y == room->bottom) {
            fill(level, room->left + 1, room->top + 1, room->width() - 1, 1, Terrain::STATUE);
            cy = room->top + 2;
        }
    }

    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));

    int sPos = cx + cy * Level::WIDTH;
    auto statueMob = std::make_unique<Statue>();
    statueMob->place(sPos);
    Actor::occupyCell(statueMob.get());
    Actor::add(statueMob.get());
    Dungeon::mobs.push_back(std::move(statueMob));
}

void Painter::paintTraps(Level* level, Room* room) {
    int trapChoices[] = { Terrain::TOXIC_TRAP, Terrain::PARALYTIC_TRAP, Terrain::CHASM };
    int trapType = trapChoices[Random::Int(3)];

    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, trapType);

    auto entrance = room->entrance();
    int x = -1, y = -1;
    int lastRow = (trapType == Terrain::CHASM) ? Terrain::CHASM : Terrain::EMPTY;

    if (entrance) {
        entrance->set(Room::Door::Type::REGULAR);

        if (entrance->x == room->left) {
            x = room->right - 1;
            y = room->top + room->height() / 2;
            fill(level, x, room->top + 1, 1, room->height() - 1, lastRow);
        } else if (entrance->x == room->right) {
            x = room->left + 1;
            y = room->top + room->height() / 2;
            fill(level, x, room->top + 1, 1, room->height() - 1, lastRow);
        } else if (entrance->y == room->top) {
            x = room->left + room->width() / 2;
            y = room->bottom - 1;
            fill(level, room->left + 1, y, room->width() - 1, 1, lastRow);
        } else if (entrance->y == room->bottom) {
            x = room->left + room->width() / 2;
            y = room->top + 1;
            fill(level, room->left + 1, y, room->width() - 1, 1, lastRow);
        }
    }

    int pos = x + y * Level::WIDTH;
    if (Random::Int(3) == 0) {
        if (lastRow == Terrain::CHASM) set(level, pos, Terrain::EMPTY);
        auto prize = std::make_unique<Weapon>("Weapon", 4, 12, 13);
        prize->random();
        Heap* h = level->drop(std::move(prize), pos);
        if (h) h->type = Heap::Type::CHEST;
    } else {
        set(level, pos, Terrain::PEDESTAL);
        auto prize = std::make_unique<Armor>("Armor", 3, 14);
        prize->random();
        level->drop(std::move(prize), pos);
    }

    level->addItemToSpawn(std::make_unique<Potion>(Potion::PotionType::LEVITATION));
}

void Painter::paintTreasury(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    set(level, room->center(), Terrain::STATUE);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::LOCKED);
    }

    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));

    Heap::Type heapType = (Random::Int(2) == 0) ? Heap::Type::CHEST : Heap::Type::HEAP;
    int n = Random::IntRange(2, 3);
    for (int i = 0; i < n; ++i) {
        int pos;
        int tries = 0;
        do {
            pos = room->random();
        } while ((level->map[pos] != Terrain::EMPTY || level->getHeap(pos) != nullptr) && ++tries < 50);

        Heap* h = level->drop(std::make_unique<Gold>(Random::IntRange(15 * Dungeon::depth, 40 * Dungeon::depth)), pos);
        if (h) {
            h->type = (i == 0 && heapType == Heap::Type::CHEST) ? Heap::Type::MIMIC : heapType;
        }
    }

    if (heapType == Heap::Type::HEAP) {
        for (int i = 0; i < 6; ++i) {
            int pos = room->random();
            if (level->map[pos] == Terrain::EMPTY) {
                level->drop(std::make_unique<Gold>(Random::IntRange(1, 5)), pos);
            }
        }
    }
}

void Painter::paintVault(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY_SP);
    fill(level, *room, 2, Terrain::EMPTY);

    Point c = room->center();
    int cPos = c.x + c.y * Level::WIDTH;

    switch (Random::Int(3)) {
    case 0: {
        auto w = std::make_unique<Weapon>("Enchanted Weapon", 4, 12, 13);
        w->random();
        Heap* h = level->drop(std::move(w), cPos);
        if (h) h->type = Heap::Type::LOCKED_CHEST;
        level->addItemToSpawn(std::make_unique<GoldenKey>(Dungeon::depth));
        break;
    }
    case 1: {
        auto w = std::make_unique<Weapon>("Crystal Weapon", 4, 14, 14);
        w->random();
        auto a = std::make_unique<Armor>("Crystal Armor", 3, 14);
        a->random();
        Heap* h1 = level->drop(std::move(w), cPos);
        if (h1) h1->type = Heap::Type::CRYSTAL_CHEST;
        const int n8[8] = { +1, -1, +Level::WIDTH, -Level::WIDTH, +1+Level::WIDTH, +1-Level::WIDTH, -1+Level::WIDTH, -1-Level::WIDTH };
        int cPos2 = cPos + n8[Random::Int(8)];
        Heap* h2 = level->drop(std::move(a), cPos2);
        if (h2) h2->type = Heap::Type::CRYSTAL_CHEST;
        level->addItemToSpawn(std::make_unique<GoldenKey>(Dungeon::depth));
        break;
    }
    case 2: {
        set(level, cPos, Terrain::PEDESTAL);
        auto w = std::make_unique<Weapon>("Sacred Weapon", 4, 14, 14);
        w->random();
        level->drop(std::move(w), cPos);
        break;
    }
    }

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::LOCKED);
    }
    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));
}

void Painter::paintWeakFloor(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::CHASM);

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::REGULAR);

        if (entrance->x == room->left) {
            for (int i = room->top + 1; i < room->bottom; ++i) {
                drawInside(level, *room, Point(room->left, i), Random::IntRange(1, room->width() - 2), Terrain::EMPTY_SP);
            }
        } else if (entrance->x == room->right) {
            for (int i = room->top + 1; i < room->bottom; ++i) {
                drawInside(level, *room, Point(room->right, i), Random::IntRange(1, room->width() - 2), Terrain::EMPTY_SP);
            }
        } else if (entrance->y == room->top) {
            for (int i = room->left + 1; i < room->right; ++i) {
                drawInside(level, *room, Point(i, room->top), Random::IntRange(1, room->height() - 2), Terrain::EMPTY_SP);
            }
        } else if (entrance->y == room->bottom) {
            for (int i = room->left + 1; i < room->right; ++i) {
                drawInside(level, *room, Point(i, room->bottom), Random::IntRange(1, room->height() - 2), Terrain::EMPTY_SP);
            }
        }
    }
}

void Painter::paintAltar(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::CHASM);

    Point c = room->center();
    auto entrance = room->entrance();
    if (entrance) {
        if (entrance->x == room->left || entrance->x == room->right) {
            Point p = drawInside(level, *room, *entrance, std::abs(entrance->x - c.x) - 2, Terrain::EMPTY_SP);
            for (; p.y != c.y; p.y += (p.y < c.y ? 1 : -1)) {
                set(level, p, Terrain::EMPTY_SP);
            }
        } else {
            Point p = drawInside(level, *room, *entrance, std::abs(entrance->y - c.y) - 2, Terrain::EMPTY_SP);
            for (; p.x != c.x; p.x += (p.x < c.x ? 1 : -1)) {
                set(level, p, Terrain::EMPTY_SP);
            }
        }
        entrance->set(Room::Door::Type::EMPTY);
    }

    fill(level, c.x - 1, c.y - 1, 3, 3, Terrain::EMBERS);
    set(level, c, Terrain::PEDESTAL);
}

void Painter::paintCrypt(Level* level, Room* room) {
    fill(level, *room, Terrain::WALL);
    fill(level, *room, 1, Terrain::EMPTY);

    Point c = room->center();
    int cx = c.x, cy = c.y;

    auto entrance = room->entrance();
    if (entrance) {
        entrance->set(Room::Door::Type::LOCKED);

        if (entrance->x == room->left) {
            set(level, Point(room->right - 1, room->top + 1), Terrain::STATUE);
            set(level, Point(room->right - 1, room->bottom - 1), Terrain::STATUE);
            cx = room->right - 2;
        } else if (entrance->x == room->right) {
            set(level, Point(room->left + 1, room->top + 1), Terrain::STATUE);
            set(level, Point(room->left + 1, room->bottom - 1), Terrain::STATUE);
            cx = room->left + 2;
        } else if (entrance->y == room->top) {
            set(level, Point(room->left + 1, room->bottom - 1), Terrain::STATUE);
            set(level, Point(room->right - 1, room->bottom - 1), Terrain::STATUE);
            cy = room->bottom - 2;
        } else if (entrance->y == room->bottom) {
            set(level, Point(room->left + 1, room->top + 1), Terrain::STATUE);
            set(level, Point(room->right - 1, room->top + 1), Terrain::STATUE);
            cy = room->top + 2;
        }
    }

    level->addItemToSpawn(std::make_unique<IronKey>(Dungeon::depth));

    auto a = std::make_unique<Armor>("Ancient Armor", 3, 14);
    a->random();
    Heap* tomb = level->drop(std::move(a), cx + cy * Level::WIDTH);
    if (tomb) {
        tomb->type = Heap::Type::TOMB;
    }
}

} // namespace pd

