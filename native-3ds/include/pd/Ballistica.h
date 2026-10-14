#pragma once

#include "Level.h"
#include "Actor.h"
#include <algorithm>
#include <cmath>

namespace pd {

class Ballistica {
public:
    static inline int trace[Level::WIDTH > Level::HEIGHT ? Level::WIDTH : Level::HEIGHT];
    static inline int distance = 0;

    static int cast(int from, int to, bool magic, bool hitChars) {
        int w = Level::WIDTH;

        int x0 = from % w;
        int x1 = to % w;
        int y0 = from / w;
        int y1 = to / w;

        int dx = x1 - x0;
        int dy = y1 - y0;

        int stepX = dx > 0 ? +1 : -1;
        int stepY = dy > 0 ? +1 : -1;

        dx = std::abs(dx);
        dy = std::abs(dy);

        int stepA;
        int stepB;
        int dA;
        int dB;

        if (dx > dy) {
            stepA = stepX;
            stepB = stepY * w;
            dA = dx;
            dB = dy;
        } else {
            stepA = stepY * w;
            stepB = stepX;
            dA = dy;
            dB = dx;
        }

        distance = 1;
        trace[0] = from;

        int cell = from;
        int err = dA / 2;

        Level* level = Dungeon::level.get();
        if (!level) return to;

        while (cell != to || magic) {
            cell += stepA;
            err += dB;
            if (err >= dA) {
                err -= dA;
                cell += stepB;
            }

            if (cell < 0 || cell >= Level::LENGTH) {
                return trace[std::max(0, distance - 1)];
            }

            trace[distance++] = cell;

            if (!level->passable[cell] && !level->avoid[cell]) {
                if (--distance > 0) {
                    return trace[distance - 1];
                }
                return from;
            }

            if (level->losBlocking[cell] || (hitChars && Actor::findChar(cell) != nullptr)) {
                return cell;
            }
        }

        trace[distance++] = cell;
        return to;
    }
};

} // namespace pd

