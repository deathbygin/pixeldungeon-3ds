#include "pd/PathFinder.h"
#include <algorithm>
#include <cstring>

namespace pd {

int PathFinder::distance[PathFinder::LENGTH];
static int s_queue[PathFinder::LENGTH];

void PathFinder::buildDistanceMap(int from, const bool* passable, int maxDist) {
    for (int i = 0; i < LENGTH; ++i) {
        distance[i] = INT_MAX;
    }

    if (from < 0 || from >= LENGTH) return;

    distance[from] = 0;
    int head = 0;
    int tail = 0;
    s_queue[tail++] = from;

    while (head < tail) {
        int cur = s_queue[head++];
        int curDist = distance[cur];

        if (curDist >= maxDist) continue;

        int cx = cur % WIDTH;
        int cy = cur / WIDTH;

        for (int n = 0; n < 8; ++n) {
            int step = NEIGHBOURS8[n];
            int npos = cur + step;

            if (npos < 0 || npos >= LENGTH) continue;

            int nx = npos % WIDTH;
            int ny = npos / WIDTH;

            // Reject horizontal wrapping across grid borders
            if (std::abs(nx - cx) > 1 || std::abs(ny - cy) > 1) continue;

            if (passable && !passable[npos]) continue;

            if (distance[npos] > curDist + 1) {
                distance[npos] = curDist + 1;
                s_queue[tail++] = npos;
            }
        }
    }
}

int PathFinder::getStep(int from, int to, const bool* passable) {
    if (from == to || from < 0 || from >= LENGTH || to < 0 || to >= LENGTH) {
        return -1;
    }

    // Direct neighbor check
    int fx = from % WIDTH, fy = from / WIDTH;
    int tx = to % WIDTH, ty = to / WIDTH;
    if (std::abs(fx - tx) <= 1 && std::abs(fy - ty) <= 1) {
        if (!passable || passable[to]) return to;
    }

    // Run BFS backwards from destination to populate distance gradient
    buildDistanceMap(to, passable, 32);

    if (distance[from] == INT_MAX) {
        return -1; // Unreachable
    }

    int bestStep = -1;
    int minDistance = distance[from];

    for (int n = 0; n < 8; ++n) {
        int npos = from + NEIGHBOURS8[n];
        if (npos < 0 || npos >= LENGTH) continue;

        int nx = npos % WIDTH;
        int ny = npos / WIDTH;
        if (std::abs(nx - fx) > 1 || std::abs(ny - fy) > 1) continue;

        if (passable && !passable[npos] && npos != to) continue;

        if (distance[npos] < minDistance) {
            minDistance = distance[npos];
            bestStep = npos;
        }
    }

    return bestStep;
}

int PathFinder::getStepBack(int cur, int from, const bool* passable) {
    if (cur < 0 || cur >= LENGTH || from < 0 || from >= LENGTH) {
        return -1;
    }

    buildDistanceMap(from, passable, 16);

    int cx = cur % WIDTH, cy = cur / WIDTH;
    int bestStep = -1;
    int maxDistance = distance[cur];

    for (int n = 0; n < 8; ++n) {
        int npos = cur + NEIGHBOURS8[n];
        if (npos < 0 || npos >= LENGTH) continue;

        int nx = npos % WIDTH;
        int ny = npos / WIDTH;
        if (std::abs(nx - cx) > 1 || std::abs(ny - cy) > 1) continue;

        if (passable && !passable[npos]) continue;

        if (distance[npos] > maxDistance && distance[npos] != INT_MAX) {
            maxDistance = distance[npos];
            bestStep = npos;
        }
    }

    return bestStep;
}

} // namespace pd

