#pragma once

#include "pd/Item.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <cstdint>

namespace pd {

class Char;
class Level;

enum class SeedTag : uint32_t {
    None       = 0,
    Hazard     = 1 << 0,
    Elemental  = 1 << 1,
    Beneficial = 1 << 2,
    Disabling  = 1 << 3,
    Spatial    = 1 << 4,
    Toxic      = 1 << 5
};

inline SeedTag operator|(SeedTag a, SeedTag b) {
    return static_cast<SeedTag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(SeedTag a, SeedTag b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

struct SeedEffect {
    PlantSeed::PlantType type;
    std::string plantName;
    std::string seedName;
    int image = 0;              // 0-7 frame in plants.png
    uint32_t particleColor = 0xFFFFFFFF;
    int aoeRadius = 0;          // 0 = single target, 1 = 3x3 area
    float duration = 0.0f;
    SeedTag tags = SeedTag::None;
    std::string description;
    std::string alchemyPotion;

    // Callback invoked when plant is stepped on or seed directly hits a character
    std::function<void(int cell, Char* target, bool directHit)> activate;
};

class SeedRegistry {
public:
    static void init();
    static const SeedEffect& get(PlantSeed::PlantType type);
    static bool isPlantable(const Level* level, int cell);
};

class Plant {
public:
    PlantSeed::PlantType type;
    int pos = -1;
    int image = 0;
    std::string plantName;

    Plant(PlantSeed::PlantType t, int p);
    void activate(Char* ch);
    void wither();
    std::string desc() const;
};

// Unified dispatcher for seed impact (on throw landing, planting, or drop)
void OnSeedImpact(PlantSeed::PlantType type, int cell);

} // namespace pd

