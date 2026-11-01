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
    int stairs = 0;

    SewerBossLevel();
    ~SewerBossLevel() override = default;

    bool build() override;
    std::vector<bool> createWater() override;
    std::vector<bool> createGrass() override;
    void decorate() override;

    void seal();
    void unseal();
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
    // The hero arrives in a sealed 3x3 chamber in the middle of the hall
    static constexpr int ROOM_LEFT   = WIDTH / 2 - 1;
    static constexpr int ROOM_RIGHT  = WIDTH / 2 + 1;
    static constexpr int ROOM_TOP    = HEIGHT / 2 - 1;
    static constexpr int ROOM_BOTTOM = HEIGHT / 2 + 1;

    HallsBossLevel();
    ~HallsBossLevel() override = default;

    bool build() override;
    void decorate() override;
    void onPress(int cell, Char* ch) override;

private:
    bool arenaOpen() const;
};

class LastLevel : public Level {
public:
    LastLevel();
    ~LastLevel() override = default;

    bool build() override;
    void decorate() override;
};

} // namespace pd
