#pragma once

#include "noosa/Texture.h"
#include "noosa/TextureFilm.h"
#include "noosa/Image.h"
#include "noosa/Camera.h"
#include "pd/Level.h"
#include <memory>
#include <string>
#include <vector>

namespace pd {

class DungeonTilemap {
public:
    static constexpr int SIZE = 16; // 16x16 tile size in pixels

    DungeonTilemap();
    ~DungeonTilemap() = default;

    // Load tileset for current dungeon depth (e.g. tiles0.png for Sewers)
    bool loadForDepth(int depth);

    // Get visual frame index for a specific terrain tile (handles secret doors/traps)
    static int getVisualTileIndex(int terrainType);

    // Update animated textures (such as water flowing UVs)
    void update(float dt);

    // True for tiles standing on the raised midground plane (walls, doors, tall grass)
    static bool isRaised(int rawTile);

    // Average colour of a cell's tile, packed as 0x00BBGGRR
    uint32_t cellColor(const Level* level, int cell) const;

    // Draw the two terrain planes through the camera frustum, deepest first
    void drawFloor(const Level* level, const noosa::Camera* camera, uint32_t& drawnCount, uint32_t& culledCount);
    void drawRaised(const Level* level, const noosa::Camera* camera);

    std::shared_ptr<noosa::TextureFilm> getFilm() const { return m_film; }
    std::shared_ptr<noosa::Texture> getTexture() const { return m_texture; }

private:
    struct TileRange {
        int minX, maxX, minY, maxY;
    };

    static TileRange visibleRange(const noosa::Camera* camera);
    void drawTile(int x, int y, int rawTile, const noosa::Camera* camera);

    int m_currentDepth = 0;
    float m_waterOffsetY = 0.0f;
    float m_shimmerTime = 0.0f;
    std::shared_ptr<noosa::Texture> m_texture;
    std::shared_ptr<noosa::TextureFilm> m_film;
    std::shared_ptr<noosa::Texture> m_waterTexture;
    std::shared_ptr<noosa::TextureFilm> m_waterFilm;

    noosa::Image m_tileImage;
    noosa::Image m_waterImage;

    std::vector<uint32_t> m_tileColors;
    uint32_t m_waterColor = 0;
};

} // namespace pd

