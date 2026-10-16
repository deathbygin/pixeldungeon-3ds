#pragma once

#include "pd/RegularLevel.h"

namespace pd {

// =============================================================================
// Sewers (Depths 1-4) & Sewer Boss (Depth 5: Goo Arena)
// =============================================================================

class SewerLevel : public RegularLevel {
public:
    SewerLevel();
    ~SewerLevel() override = default;

    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void decorate() override;
};

class SewerBossLevel : public RegularLevel {
public:
    SewerBossLevel();
    ~SewerBossLevel() override = default;

    bool build() override;
    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void decorate() override;
};

// =============================================================================
// Prison (Depths 6-9) & Prison Boss (Depth 10: Tengu Arena)
// =============================================================================

class PrisonLevel : public RegularLevel {
public:
    PrisonLevel();
    ~PrisonLevel() override = default;

    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void assignRoomType() override;
    void decorate() override;
};

class PrisonBossLevel : public RegularLevel {
public:
    PrisonBossLevel();
    ~PrisonBossLevel() override = default;

    bool build() override;
};

// =============================================================================
// Caves (Depths 11-14) & Caves Boss (Depth 15: DM-300 Arena)
// =============================================================================

class CavesLevel : public RegularLevel {
public:
    CavesLevel();
    ~CavesLevel() override = default;

    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void decorate() override;
};

class CavesBossLevel : public Level {
public:
    CavesBossLevel();
    ~CavesBossLevel() override = default;

    bool build() override;
    void decorate() override;
};

// =============================================================================
// City (Depths 16-19, 21) & City Boss (Depth 20: King of Dwarves Arena)
// =============================================================================

class CityLevel : public RegularLevel {
public:
    CityLevel();
    ~CityLevel() override = default;

    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void assignRoomType() override;
    void decorate() override;
};

class CityBossLevel : public Level {
public:
    CityBossLevel();
    ~CityBossLevel() override = default;

    bool build() override;
    void decorate() override;
};

// =============================================================================
// Halls (Depths 22-24), Halls Boss (Depth 25: Yog-Dzewa) & Last (Depth 26: Amulet)
// =============================================================================

class HallsLevel : public RegularLevel {
public:
    HallsLevel();
    ~HallsLevel() override = default;

    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void decorate() override;
};

class HallsBossLevel : public Level {
public:
    HallsBossLevel();
    ~HallsBossLevel() override = default;

    bool build() override;
    void decorate() override;
};

class LastLevel : public Level {
public:
    LastLevel();
    ~LastLevel() override = default;

    bool build() override;
    void decorate() override;
};

} // namespace pd
