#pragma once

#include "Level.h"

namespace pd {

class ShadowCaster {
public:
    static constexpr int MAX_DISTANCE = 8;

    static void init();
    static void castShadow(int x, int y, bool* fieldOfView, int distance, const bool* losBlocking);

    class Obstacles {
    public:
        static constexpr int SIZE = (MAX_DISTANCE + 1) * (MAX_DISTANCE + 1) / 2;
        float a1[SIZE] = { 0.0f };
        float a2[SIZE] = { 0.0f };
        int length = 0;
        int limit = 0;

        void reset() {
            length = 0;
            limit = 0;
        }

        void add(float o1, float o2) {
            if (length > limit && o1 <= a2[length - 1]) {
                a2[length - 1] = o2;
            } else if (length < SIZE) {
                a1[length] = o1;
                a2[length++] = o2;
            }
        }

        bool isBlocked(float a) const {
            for (int i = 0; i < limit; ++i) {
                if (a >= a1[i] && a <= a2[i]) {
                    return true;
                }
            }
            return false;
        }

        void nextRow() {
            limit = length;
        }
    };

private:
    static void scanSector(int cx, int cy, int m1, int m2, int m3, int m4);
};

} // namespace pd
