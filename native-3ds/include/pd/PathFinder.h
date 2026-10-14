#pragma once

#include <cstdint>
#include <climits>

namespace pd {

class PathFinder {
public:
    static constexpr int WIDTH = 32;
    static constexpr int HEIGHT = 32;
    static constexpr int LENGTH = WIDTH * HEIGHT; // 1024

    static constexpr int NEIGHBOURS4[4] = { -WIDTH, 1, WIDTH, -1 };
    static constexpr int NEIGHBOURS8[8] = {
        -WIDTH, -WIDTH + 1, 1, WIDTH + 1,
        WIDTH, WIDTH - 1, -1, -WIDTH - 1
    };

    static int distance[LENGTH];

    // BFS distance map builder from a starting cell
    static void buildDistanceMap(int from, const bool* passable, int maxDist = 32);

    // Get immediate next step towards destination (returns target cell or -1 if unreachable)
    static int getStep(int from, int to, const bool* passable);

    // Get next step fleeing away from threat
    static int getStepBack(int cur, int from, const bool* passable);

    // Chebyshev distance (diagonal movement allowed, metric used by Pixel Dungeon)
    static int distanceChebyshev(int a, int b) {
        int ax = a % WIDTH, ay = a / WIDTH;
        int bx = b % WIDTH, by = b / WIDTH;
        int dx = (ax > bx) ? (ax - bx) : (bx - ax);
        int dy = (ay > by) ? (ay - by) : (by - ay);
        return (dx > dy) ? dx : dy;
    }

    // Manhattan distance
    static int distanceManhattan(int a, int b) {
        int ax = a % WIDTH, ay = a / WIDTH;
        int bx = b % WIDTH, by = b / WIDTH;
        int dx = (ax > bx) ? (ax - bx) : (bx - ax);
        int dy = (ay > by) ? (ay - by) : (by - ay);
        return dx + dy;
    }
};

} // namespace pd

