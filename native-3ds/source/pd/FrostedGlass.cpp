#include "pd/FrostedGlass.h"
#include "pd/Dungeon.h"
#include "pd/FogOfWar.h"
#include "noosa/Stereo.h"
#include <algorithm>

namespace pd {

FrostedGlass::FrostedGlass() {
    m_pixels = std::make_unique<uint32_t[]>(Level::LENGTH);
    std::fill_n(m_pixels.get(), Level::LENGTH, 0xFF000000);

    m_texture = std::make_shared<noosa::Texture>();
    m_texture->loadFromRGBA8(reinterpret_cast<const uint8_t*>(m_pixels.get()), Level::WIDTH, Level::HEIGHT);
    m_texture->setFilter(GPU_LINEAR, GPU_LINEAR);
    m_texture->setWrap(GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
}

void FrostedGlass::refresh(const Level* level, const DungeonTilemap& tilemap) {
    if (!level) return;

    uint32_t shade = FogOfWar::rememberedShade();

    for (int i = 0; i < Level::LENGTH; ++i) {
        uint32_t c = 0;
        if (Dungeon::visible[i]) {
            c = tilemap.cellColor(level, i);
        } else if (level->visited[i] || level->mapped[i]) {
            uint32_t lit = tilemap.cellColor(level, i);
            c = (((lit & 0xFF) * shade) >> 8) |
                ((((lit >> 8) & 0xFF) * shade >> 8) << 8) |
                ((((lit >> 16) & 0xFF) * shade >> 8) << 16);
        }
        m_pixels[i] = 0xFF000000 | c;
    }

    m_texture->updateSubRGBA8(reinterpret_cast<const uint8_t*>(m_pixels.get()), Level::WIDTH, Level::HEIGHT);
}

void FrostedGlass::draw(const noosa::Camera* camera, float x, float y, float w, float h, float z, uint8_t alpha) const {
    if (!camera || !m_texture->getTex()) return;

    using noosa::Stereo;
    float parallax = Stereo::snappedShift(Stereo::PLANE_UI) - Stereo::snappedShift(Stereo::PLANE_FLOOR);

    float worldL, worldT, worldR, worldB;
    camera->screenToWorld(x + parallax, y, worldL, worldT);
    camera->screenToWorld(x + w + parallax, y + h, worldR, worldB);

    float mapW = Level::WIDTH * static_cast<float>(DungeonTilemap::SIZE);
    float mapH = Level::HEIGHT * static_cast<float>(DungeonTilemap::SIZE);

    Tex3DS_SubTexture sub;
    sub.width = static_cast<u16>(w);
    sub.height = static_cast<u16>(h);
    sub.left = worldL / mapW;
    sub.right = worldR / mapW;
    sub.top = 1.0f - worldT / mapH;
    sub.bottom = 1.0f - worldB / mapH;

    C2D_Image img = { const_cast<C3D_Tex*>(m_texture->getTex()), &sub };

    C2D_DrawParams params;
    params.pos.x = x;
    params.pos.y = y;
    params.pos.w = w;
    params.pos.h = h;
    params.center.x = 0.0f;
    params.center.y = 0.0f;
    params.depth = z;
    params.angle = 0.0f;

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xFF, alpha), 1.0f);
    C2D_DrawImage(img, &params, &tint);
}

} // namespace pd
