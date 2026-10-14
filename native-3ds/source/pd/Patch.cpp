#include "pd/Patch.h"
#include "pd/Level.h"
#include "pd/Random.h"

namespace pd {

std::vector<bool> Patch::generate(float seed, int nGen) {
    std::vector<bool> cur(Level::LENGTH, false);
    std::vector<bool> off(Level::LENGTH, false);

    for (int i = 0; i < Level::LENGTH; ++i) {
        off[i] = (Random::Float() < seed);
    }

    int w = Level::WIDTH;
    int h = Level::HEIGHT;

    for (int i = 0; i < nGen; ++i) {
        for (int y = 1; y < h - 1; ++y) {
            for (int x = 1; x < w - 1; ++x) {
                int pos = x + y * w;
                int count = 0;
                if (off[pos - w - 1]) count++;
                if (off[pos - w])     count++;
                if (off[pos - w + 1]) count++;
                if (off[pos - 1])     count++;
                if (off[pos + 1])     count++;
                if (off[pos + w - 1]) count++;
                if (off[pos + w])     count++;
                if (off[pos + w + 1]) count++;

                if (!off[pos] && count >= 5) {
                    cur[pos] = true;
                } else if (off[pos] && count >= 4) {
                    cur[pos] = true;
                } else {
                    cur[pos] = false;
                }
            }
        }
        off = cur;
    }

    return off;
}

} // namespace pd

