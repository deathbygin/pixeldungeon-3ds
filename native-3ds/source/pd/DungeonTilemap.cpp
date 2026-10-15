#include "pd/DungeonTilemap.h"
#include "pd/Assets.h"
#include "pd/Terrain.h"
#include <algorithm>

namespace pd {

DungeonTilemap::DungeonTilemap() {
    m_tileImage.targetScreen = noosa::ScreenTarget::Top;
    m_waterImage.targetScreen = noosa::ScreenTarget::Top;
}

bool DungeonTilemap::loadForDepth(int depth) {
    if (m_currentDepth == depth && m_texture && m_texture->isLoaded()) {
        return true;
    }

    m_currentDepth = depth;
    const std::string& tilesPath = Assets::getTilesetForDepth(depth);
    m_texture = noosa::Texture::get(tilesPath);

    if (m_texture && m_texture->isLoaded()) {
        m_film = std::make_shared<noosa::TextureFilm>(m_texture, SIZE, SIZE);
        m_tileImage.setFilm(m_film);
    }

    const std::string& waterPath = Assets::getWaterForDepth(depth);
    m_waterTexture = noosa::Texture::get(waterPath);
    if (m_waterTexture && m_waterTexture->isLoaded()) {
        m_waterFilm = std::make_shared<noosa::TextureFilm>(m_waterTexture, SIZE, SIZE);
        m_waterImage.setFilm(m_waterFilm);
        m_waterTexture->setWrap(GPU_REPEAT, GPU_REPEAT);
        m_waterImage.setTexture(m_waterTexture);
    }

    return (m_texture && m_texture->isLoaded());
}

void DungeonTilemap::update(float dt) {
    // Flowing water animation matching Java Pixel Dungeon (5 pixels per second vertical shift)
    m_waterOffsetY -= 5.0f * dt;
    while (m_waterOffsetY < 0.0f) m_waterOffsetY += 32.0f;
    while (m_waterOffsetY >= 32.0f) m_waterOffsetY -= 32.0f;
}

int DungeonTilemap::getVisualTileIndex(int terrainType) {
    if (terrainType == Terrain::SECRET_DOOR) {
        return Terrain::WALL; // Mask secret door as solid wall
    }
    if (Terrain::isSecret(terrainType)) {
        return Terrain::EMPTY; // Mask secret traps as regular floor
    }
    return terrainType;
}

void DungeonTilemap::draw(const Level* level, const noosa::Camera* camera, uint32_t& drawnCount, uint32_t& culledCount) {
    if (!level || !camera || !m_film) return;

    float camL, camT, camR, camB;
    camera->getVisibleWorldBounds(camL, camT, camR, camB);
    int minTileX = std::max(0, static_cast<int>(camL / static_cast<float>(SIZE)));
    int maxTileX = std::min(Level::WIDTH - 1, static_cast<int>(camR / static_cast<float>(SIZE)) + 1);
    int minTileY = std::max(0, static_cast<int>(camT / static_cast<float>(SIZE)));
    int maxTileY = std::min(Level::HEIGHT - 1, static_cast<int>(camB / static_cast<float>(SIZE)) + 1);

    drawnCount = 0;
    noosa::Color litTint(255, 255, 255, 255);

    for (int y = minTileY; y <= maxTileY; ++y) {
        for (int x = minTileX; x <= maxTileX; ++x) {
            int cellIdx = y * Level::WIDTH + x;
            drawnCount++;

            int rawTile = level->get(cellIdx);
            bool isWaterTile = (rawTile == Terrain::WATER || (rawTile >= Terrain::WATER_TILES && rawTile < Terrain::WATER_TILES + 16));

            // 1. Flowing animated water layer (water0.png - water4.png) rendered beneath water and organic shoreline cells
            if (isWaterTile && m_waterTexture && m_waterTexture->isLoaded()) {
                m_waterImage.x = x * static_cast<float>(SIZE);
                m_waterImage.y = y * static_cast<float>(SIZE);
                m_waterImage.z = 0.18f;
                // waterX.png is 32x32: 2x2 grid of 16x16 tiles
                int waterFrame = ((y % 2) * 2 + (x % 2)) % m_waterFilm->size();
                m_waterImage.setFrame(waterFrame);

                float fx = static_cast<float>((x * SIZE) % 32);
                float fy = std::fmod(static_cast<float>(y * SIZE) + m_waterOffsetY, 32.0f);
                if (fy < 0.0f) fy += 32.0f;

                m_waterImage.setFrame(fx, fy, static_cast<float>(SIZE), static_cast<float>(SIZE));
                m_waterImage.tint = litTint;
                m_waterImage.draw(camera);
            }

            // 2. Tilemap overlay:
            // If it's pure open water (rawTile == Terrain::WATER), the animated water texture is shown directly.
            // If it's a shore/edge tile (Terrain::WATER_TILES to WATER_TILES+15), the semi-transparent stitched tile
            // from tilesX.png is rendered over the water texture with full alpha blending!
            if (rawTile != Terrain::WATER) {
                m_tileImage.x = x * static_cast<float>(SIZE);
                m_tileImage.y = y * static_cast<float>(SIZE);
                m_tileImage.z = 0.2f;

                int visualFrame = getVisualTileIndex(rawTile);
                m_tileImage.setFrame(visualFrame);
                m_tileImage.tint = litTint;
                m_tileImage.draw(camera);
            }
        }
    }

    culledCount = Level::LENGTH - drawnCount;
}

} // namespace pd

