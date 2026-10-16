#pragma once

#include "pd/Rect.h"
#include "pd/Random.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

namespace pd {

class Level;

class Room : public Rect {
public:
    enum class Type {
        NONE = 0,
        STANDARD,
        ENTRANCE,
        EXIT,
        BOSS_EXIT,
        TUNNEL,
        PASSAGE,
        SHOP,
        BLACKSMITH,
        TREASURY,
        ARMORY,
        LIBRARY,
        LABORATORY,
        VAULT,
        TRAPS,
        STORAGE,
        MAGIC_WELL,
        GARDEN,
        CRYPT,
        STATUE,
        POOL,
        RAT_KING,
        WEAK_FLOOR,
        PIT,
        ALTAR
    };

    class Door : public Point {
    public:
        enum class Type {
            EMPTY = 0,
            TUNNEL = 1,
            REGULAR = 2,
            UNLOCKED = 3,
            HIDDEN = 4,
            BARRICADE = 5,
            LOCKED = 6
        };
        Type type = Type::EMPTY;

        Door(int x, int y) : Point(x, y) {}

        void set(Type t) {
            if (static_cast<int>(t) > static_cast<int>(type)) {
                type = t;
            }
        }
    };

    Type type = Type::NONE;

    std::vector<Room*> neigbours;
    std::unordered_map<Room*, std::shared_ptr<Door>> connected;

    int distance = 0;
    int price = 1;

    Room() = default;
    Room(int l, int t, int r, int b, Type tp = Type::NONE)
        : Rect(l, t, r, b), type(tp) {}
    explicit Room(const Rect& r, Type tp = Type::NONE)
        : Rect(r), type(tp) {}

    int centerX() const { return (left + right) / 2; }
    int centerY() const { return (top + bottom) / 2; }

    Point center() const {
        return Point(
            (left + right) / 2 + (((right - left) & 1) == 1 ? Random::Int(2) : 0),
            (top + bottom) / 2 + (((bottom - top) & 1) == 1 ? Random::Int(2) : 0)
        );
    }

    int random(int m = 0) const {
        int minX = left + 1 + m;
        int maxX = right - m;
        int minY = top + 1 + m;
        int maxY = bottom - m;
        int x = (maxX > minX) ? Random::Int(minX, maxX) : minX;
        int y = (maxY > minY) ? Random::Int(minY, maxY) : minY;
        return x + y * 32; // Level::WIDTH = 32
    }
    int randomCell() const { return random(0); }

    bool inside(int p) const {
        int x = p % 32;
        int y = p / 32;
        return Rect::inside(x, y);
    }

    void addNeigbour(Room* other) {
        if (!other || other == this) return;
        Rect i = intersect(*other);
        if ((i.width() == 0 && i.height() >= 3) ||
            (i.height() == 0 && i.width() >= 3)) {
            if (std::find(neigbours.begin(), neigbours.end(), other) == neigbours.end()) {
                neigbours.push_back(other);
                other->neigbours.push_back(this);
            }
        }
    }

    void connect(Room* other) {
        if (!other || other == this) return;
        if (connected.find(other) == connected.end()) {
            connected[other] = nullptr;
            other->connected[this] = nullptr;
        }
    }

    std::shared_ptr<Door> entrance() const {
        if (!connected.empty()) {
            return connected.begin()->second;
        }
        return nullptr;
    }
};

} // namespace pd
