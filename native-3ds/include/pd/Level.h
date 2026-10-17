#pragma once

#include "Terrain.h"
#include "Room.h"
#include "PathFinder.h"
#include "Heap.h"
#include "pd/Plant.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace pd {

class Char;
class Item;

class Level {
public:
    static constexpr int WIDTH = PathFinder::WIDTH;   // 32
    static constexpr int HEIGHT = PathFinder::HEIGHT; // 32
    static constexpr int LENGTH = PathFinder::LENGTH; // 1024

    static constexpr int NEIGHBOURS4[4] = { -WIDTH, 1, WIDTH, -1 };
    static constexpr int NEIGHBOURS8[8] = { 1, -1, WIDTH, -WIDTH, 1 + WIDTH, 1 - WIDTH, -1 + WIDTH, -1 - WIDTH };
    static constexpr int NEIGHBOURS9[9] = { 0, 1, -1, WIDTH, -WIDTH, 1 + WIDTH, 1 - WIDTH, -1 + WIDTH, -1 - WIDTH };

    int map[LENGTH] = { 0 };
    bool visited[LENGTH] = { false };
    bool mapped[LENGTH] = { false };
    bool fieldOfView[LENGTH] = { false };
    bool passable[LENGTH] = { false };
    bool losBlocking[LENGTH] = { false };
    bool pit[LENGTH] = { false };
    bool secret[LENGTH] = { false };
    bool avoid[LENGTH] = { false };
    bool water[LENGTH] = { false };

    enum class Feeling {
        NONE,
        CHASM,
        WATER,
        GRASS
    };

    Feeling feeling = Feeling::NONE;
    int secretDoors = 0;
    int viewDistance = 8;

    static bool pitRoomNeeded;
    static bool weakFloorCreated;

    int entrance = 0;
    int exit = 0;
    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<std::unique_ptr<Heap>> heaps;
    std::vector<std::unique_ptr<Item>> itemsToSpawn;

    enum class WellType {
        Awareness,
        Health,
        Transmutation
    };
    std::unordered_map<int, WellType> wells;
    void affectWell(int cell, Char* ch);
    void bounceItemFrom(int cell);

    void addItemToSpawn(std::unique_ptr<Item> item) {
        if (item) itemsToSpawn.push_back(std::move(item));
    }
    std::unique_ptr<Item> itemToSpawnAsPrize();
    int pitLandingCell = -1;
    bool hasPitRoom() const;
    int randomRespawnCell() const;
    virtual int pitCell() const;
    Room* room(int pos) const;

    Level();
    virtual ~Level() = default;

    static std::unique_ptr<Level> create(int depth);

    virtual int tunnelTile() const {
        return feeling == Feeling::CHASM ? Terrain::EMPTY_SP : Terrain::EMPTY;
    }

    int getWaterTile(int pos) const;

    // Procedural level builder
    virtual bool build();
    virtual void decorate() {}
    virtual std::vector<bool> createWater();
    virtual std::vector<bool> createGrass();
    virtual void paintWater();
    virtual void paintGrass();

    // Field of view calculation using raycasting
    void updateFieldOfView(int centerCell, int distance = 8);

    // Refresh passability & line of sight caches
    void buildFlags();
    void cleanWalls();

    // Tile access
    int get(int x, int y) const { return map[y * WIDTH + x]; }
    int get(int cell) const { return (cell >= 0 && cell < LENGTH) ? map[cell] : Terrain::WALL; }
    void set(int cell, int tile);
    void set(int x, int y, int tile) { set(y * WIDTH + x, tile); }

    // Cell helpers
    static int cell(int x, int y) { return y * WIDTH + x; }
    static int cellX(int c) { return c % WIDTH; }
    static int cellY(int c) { return c / WIDTH; }
    int randomDestination() const;

    bool isPassable(int c) const {
        return (c >= 0 && c < LENGTH) && passable[c];
    }

    bool isLosBlocking(int c) const {
        return (c < 0 || c >= LENGTH) || losBlocking[c];
    }

    bool isFlamable(int c) const {
        return (c >= 0 && c < LENGTH) && Terrain::isFlamable(map[c]);
    }

    void destroy(int cell);
    void trample(int cell, Char* ch);
    void press(int cell, Char* ch);
    void mobPress(class Mob* mob);

    // Heap management
    Heap* getHeap(int cell) const;
    Heap* drop(std::unique_ptr<Item> item, int cell);
    void removeHeap(Heap* heap);

    // Plant management
    std::vector<std::unique_ptr<Plant>> plants;
    Plant* getPlant(int cell) const;
    Plant* plant(PlantSeed::PlantType type, int cell);
    void uproot(int cell);
    void activatePlant(int cell, Char* ch);

    static const char* tileName(int tile);
    static const char* tileDesc(int tile);
};

} // namespace pd
