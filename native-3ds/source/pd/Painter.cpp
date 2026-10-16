#include "pd/Painter.h"
#include "pd/Level.h"
#include "pd/Terrain.h"
#include "pd/Random.h"
#include "pd/Dungeon.h"
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

} // namespace pd

