#include "pd/Random.h"
#include <algorithm>

namespace pd {

uint64_t Random::s_state = 0x853c49e6748fea9bULL;

void Random::seed(uint64_t s) {
    s_state = (s == 0) ? 0x853c49e6748fea9bULL : s;
}

uint64_t Random::getSeed() {
    return s_state;
}

uint64_t Random::nextU64() {
    uint64_t z = (s_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

int Random::Int(int max) {
    if (max <= 0) return 0;
    return static_cast<int>(nextU64() % static_cast<uint64_t>(max));
}

int Random::IntRange(int min, int max) {
    if (min >= max) return min;
    return min + Int(max - min + 1);
}

float Random::Float() {
    // 24 bits of precision in [0.0, 1.0)
    return static_cast<float>(nextU64() & 0xFFFFFF) / 16777216.0f;
}

float Random::Float(float max) {
    return Float() * max;
}

float Random::Float(float min, float max) {
    return min + Float() * (max - min);
}

int Random::NormalIntRange(int min, int max) {
    if (min >= max) return min;
    float r1 = Float();
    float r2 = Float();
    float avg = (r1 + r2) * 0.5f;
    return min + static_cast<int>(avg * (max - min + 1));
}

int Random::Chances(const float* chances, int count) {
    if (!chances || count <= 0) return 0;
    float sum = 0.0f;
    for (int i = 0; i < count; ++i) sum += chances[i];
    if (sum <= 0.0f) return 0;

    float r = Float(sum);
    float acc = 0.0f;
    for (int i = 0; i < count; ++i) {
        acc += chances[i];
        if (r < acc) return i;
    }
    return count - 1;
}

} // namespace pd

