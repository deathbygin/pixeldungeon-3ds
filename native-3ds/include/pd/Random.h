#pragma once

#include <cstdint>
#include <vector>

namespace pd {

class Random {
public:
    static void seed(uint64_t s);
    static uint64_t getSeed();

    // Returns an integer in [0, max)
    static int Int(int max);

    // Returns an integer in [min, max]
    static int IntRange(int min, int max);

    // Returns a float in [0.0, 1.0)
    static float Float();

    // Returns a float in [0.0, max)
    static float Float(float max);

    // Returns a float in [min, max)
    static float Float(float min, float max);

    // Returns normally-distributed integer between min and max (triangular distribution)
    static int NormalIntRange(int min, int max);

    // Pick an index from a list of relative probabilities
    static int Chances(const float* chances, int count);

    template<typename T>
    static const T& element(const std::vector<T>& vec) {
        return vec[Int(static_cast<int>(vec.size()))];
    }

private:
    static uint64_t s_state;
    static uint64_t nextU64();
};

} // namespace pd

