#pragma once

#include "pd/Level.h"
#include <cstdint>

namespace pd {

class Blob {
public:
    enum class BlobType {
        TOXIC_GAS,
        PARALYTIC_GAS,
        FIRE
    };

    BlobType type = BlobType::TOXIC_GAS;
    int volume = 0;
    int cur[Level::LENGTH] = { 0 };
    int off[Level::LENGTH] = { 0 };

    explicit Blob(BlobType t);

    void clear();
    void seed(int cell, int amount = 100);
    void evolve(const Level* level);

    uint32_t getParticleColor() const;
};

} // namespace pd

