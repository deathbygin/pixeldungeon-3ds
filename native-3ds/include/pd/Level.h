#pragma once

#include "Terrain.h"
#include "Room.h"
#include "PathFinder.h"
#include "Heap.h"
#include <vector>
#include <memory>

namespace pd {

class Char;
class Item;

class Level {
public:
    static constexpr int WIDTH = PathFinder::WIDTH;   // 32
    static constexpr int HEIGHT = PathFinder::HEIGHT; // 32
    static constexpr int LENGTH = PathFinder::LENGTH; // 1024

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

    int entrance = 0;
    int exit = 0;
    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<std::unique_ptr<Heap>> heaps;

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
};

} // namespace pd
