#include "pd/ShadowCaster.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace pd {

static int s_rounding[ShadowCaster::MAX_DISTANCE + 1][ShadowCaster::MAX_DISTANCE + 1] = { { 0 } };
static bool s_initialized = false;
static int s_distance = 0;
static const int* s_limits = nullptr;
static const bool* s_losBlocking = nullptr;
static bool* s_fieldOfView = nullptr;
static ShadowCaster::Obstacles s_obs;

void ShadowCaster::init() {
    if (s_initialized) return;

    for (int i = 1; i <= MAX_DISTANCE; ++i) {
        for (int j = 1; j <= i; ++j) {
            double angle = std::asin(static_cast<double>(j) / (i + 0.5));
            double cosVal = std::cos(angle);
            int val = static_cast<int>(std::round(i * cosVal));
            s_rounding[i][j] = std::min(j, val);
        }
    }
    s_initialized = true;
}

void ShadowCaster::castShadow(int x, int y, bool* fieldOfView, int distance, const bool* losBlocking) {
    if (!s_initialized) {
        init();
    }

    if (!fieldOfView || !losBlocking) return;

    s_distance = std::clamp(distance, 1, MAX_DISTANCE);
    s_limits = s_rounding[s_distance];
    s_losBlocking = losBlocking;
    s_fieldOfView = fieldOfView;

    std::fill_n(fieldOfView, Level::LENGTH, false);
    if (x >= 0 && x < Level::WIDTH && y >= 0 && y < Level::HEIGHT) {
        fieldOfView[y * Level::WIDTH + x] = true;
    }

    scanSector(x, y, +1, +1,  0,  0);
    scanSector(x, y, -1, +1,  0,  0);
    scanSector(x, y, +1, -1,  0,  0);
    scanSector(x, y, -1, -1,  0,  0);
    scanSector(x, y,  0,  0, +1, +1);
    scanSector(x, y,  0,  0, -1, +1);
    scanSector(x, y,  0,  0, +1, -1);
    scanSector(x, y,  0,  0, -1, -1);
}

void ShadowCaster::scanSector(int cx, int cy, int m1, int m2, int m3, int m4) {
    s_obs.reset();

    for (int p = 1; p <= s_distance; ++p) {
        float dq2 = 0.5f / static_cast<float>(p);
        int pp = s_limits[p];

        for (int q = 0; q <= pp; ++q) {
            int x = cx + q * m1 + p * m3;
            int y = cy + p * m2 + q * m4;

            if (y >= 0 && y < Level::HEIGHT && x >= 0 && x < Level::WIDTH) {
                float a0 = static_cast<float>(q) / static_cast<float>(p);
                float a1 = a0 - dq2;
                float a2 = a0 + dq2;

                int pos = y * Level::WIDTH + x;

                if (!(s_obs.isBlocked(a0) && s_obs.isBlocked(a1) && s_obs.isBlocked(a2))) {
                    s_fieldOfView[pos] = true;
                }

                if (s_losBlocking[pos]) {
                    s_obs.add(a1, a2);
                }
            }
        }

        s_obs.nextRow();
    }
}

} // namespace pd

