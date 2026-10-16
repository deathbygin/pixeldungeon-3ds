#pragma once
#include "pd/Actor.h"
#include "pd/Level.h"
#include <cstdint>

namespace pd {

class Blob : public Actor {
public:
    enum class BlobType {
        TOXIC_GAS,
        PARALYTIC_GAS,
        FIRE,
        CONFUSION_GAS,
        CORROSIVE_GAS,
        REGROWTH,
        FOLIAGE,
        WATER_OF_HEALTH,
        WATER_OF_AWARENESS,
        WATER_OF_TRANSMUTATION
    };

    BlobType type = BlobType::TOXIC_GAS;
    int volume = 0;
    int cur[Level::LENGTH] = { 0 };
    int off[Level::LENGTH] = { 0 };

    explicit Blob(BlobType t);
    ~Blob() override = default;

    Blob* asBlob() override { return this; }
    bool act() override;

    void clear();
    void clear(int cell);
    void seed(int cell, int amount = 100);
    void evolve(const Level* level);

    uint32_t getParticleColor() const;
    const char* tileDesc() const;
};

class Freezing {
public:
    static bool affect(int cell, Blob* fire = nullptr);
};

} // namespace pd

