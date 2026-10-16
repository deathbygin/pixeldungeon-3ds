#pragma once

#include "noosa/Texture.h"
#include "noosa/TextureFilm.h"
#include "noosa/Image.h"
#include "noosa/Camera.h"
#include "pd/Level.h"
#include <memory>
#include <string>

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

    // Draw visible level tiles through camera frustum
    void draw(const Level* level, const noosa::Camera* camera, uint32_t& drawnCount, uint32_t& culledCount);

    std::shared_ptr<noosa::TextureFilm> getFilm() const { return m_film; }
    std::shared_ptr<noosa::Texture> getTexture() const { return m_texture; }

private:
    int m_currentDepth = 0;
    float m_waterOffsetY = 0.0f;
    float m_shimmerTime = 0.0f;
    std::shared_ptr<noosa::Texture> m_texture;
    std::shared_ptr<noosa::TextureFilm> m_film;
    std::shared_ptr<noosa::Texture> m_waterTexture;
    std::shared_ptr<noosa::TextureFilm> m_waterFilm;

    noosa::Image m_tileImage;
    noosa::Image m_waterImage;
};

} // namespace pd

