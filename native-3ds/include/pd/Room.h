#pragma once

#include <vector>
#include <algorithm>
#include "pd/Random.h"
#include "pd/PathFinder.h"

namespace pd {

class Room {
public:
    enum class Type {
        STANDARD,
        ENTRANCE,
        EXIT,
        TREASURY,
        ARMORY,
        LABORATORY,
        VAULT,
        GARDEN,
        CHASM
    };

    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    Type type = Type::STANDARD;

    Room() = default;
    Room(int l, int t, int r, int b, Type tp = Type::STANDARD)
        : left(l), top(t), right(r), bottom(b), type(tp) {}

    int width() const { return right - left + 1; }
    int height() const { return bottom - top + 1; }

    int centerX() const { return (left + right) / 2; }
    int centerY() const { return (top + bottom) / 2; }

    bool contains(int x, int y) const {
        return x >= left && x <= right && y >= top && y <= bottom;
    }

    bool intersects(const Room& o, int margin = 0) const {
        return !(right + margin < o.left ||
                 left - margin > o.right ||
                 bottom + margin < o.top ||
                 top - margin > o.bottom);
    }

    int random(int m = 0) const {
        int minX = left + 1 + m;
        int maxX = right - m;
        int minY = top + 1 + m;
        int maxY = bottom - m;
        int x = (maxX >= minX) ? Random::IntRange(minX, maxX) : left;
        int y = (maxY >= minY) ? Random::IntRange(minY, maxY) : top;
        return x + y * PathFinder::WIDTH;
    }
    int randomCell() const { return random(0); }
};

} // namespace pd
