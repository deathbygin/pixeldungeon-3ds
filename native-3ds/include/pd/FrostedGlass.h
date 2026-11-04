#pragma once

#include "noosa/Texture.h"
#include "noosa/Camera.h"
#include "pd/DungeonTilemap.h"
#include <citro2d.h>
#include <cstdint>
#include <memory>

namespace pd {

// Backdrop for glass panels on the top screen. Reading the framebuffer back would stall the
// frame twice over in stereo, so the blur comes from a tiny map holding one texel per cell:
// bilinear filtering stretches it across the panel as a soft image of the dungeon behind.
class FrostedGlass {
public:
    FrostedGlass();

    // Rebuilds the map from the current level. Call once per frame, before the first eye.
    void refresh(const Level* level, const DungeonTilemap& tilemap);

    // Draws the backdrop for a panel on the UI plane. Each eye sees through the glass to the
    // floor at its own angle, so the sample window slides against the panel by their parallax.
    void draw(const noosa::Camera* camera, float x, float y, float w, float h, float z, uint8_t alpha) const;

private:
    std::unique_ptr<uint32_t[]> m_pixels;
    std::shared_ptr<noosa::Texture> m_texture;
};

} // namespace pd
