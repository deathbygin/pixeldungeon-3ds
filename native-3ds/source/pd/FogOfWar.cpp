#include "pd/FogOfWar.h"
#include "pd/Level.h"
#include "pd/Preferences.h"
#include <algorithm>

namespace pd {

FogOfWar::FogOfWar(int mapWidth, int mapHeight)
    : m_mapWidth(mapWidth),
      m_mapHeight(mapHeight),
      m_pWidth(mapWidth + 1),
      m_pHeight(mapHeight + 1)
{
    m_width2 = 1;
    while (m_width2 < m_pWidth) {
        m_width2 <<= 1;
    }

    m_height2 = 1;
    while (m_height2 < m_pHeight) {
        m_height2 <<= 1;
    }

    m_pixels = std::make_unique<uint32_t[]>(m_width2 * m_height2);
    std::fill_n(m_pixels.get(), m_width2 * m_height2, COLOR_INVISIBLE);

    m_texture = std::make_shared<noosa::Texture>();
    m_texture->loadFromRGBA8(reinterpret_cast<const uint8_t*>(m_pixels.get()), m_width2, m_height2);
    m_texture->setFilter(GPU_LINEAR, GPU_LINEAR);
    m_texture->setWrap(GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    m_subTex.width = static_cast<u16>(m_width2);
    m_subTex.height = static_cast<u16>(m_height2);
    m_subTex.left = 0.0f;
    m_subTex.right = 1.0f;
    m_subTex.top = 1.0f;
    m_subTex.bottom = 0.0f;

    m_image.tex = m_texture->getTex();
    m_image.subtex = &m_subTex;

    // 16 pixels per tile, offset by half a tile to align corner vertices
    x = -8.0f;
    y = -8.0f;
    width = static_cast<float>(m_width2);
    height = static_cast<float>(m_height2);
    scale.x = 16.0f;
    scale.y = 16.0f;
    targetScreen = noosa::ScreenTarget::Top;
    z = 0.65f;
}

void FogOfWar::updateVisibility(const bool* visible, const bool* visited, const bool* mapped) {
    if (!visible || !visited) return;

    std::fill_n(m_pixels.get(), m_width2 * m_height2, COLOR_INVISIBLE);

    for (int i = 1; i < m_pHeight - 1; ++i) {
        int pos = (m_pWidth - 1) * i;
        for (int j = 1; j < m_pWidth - 1; ++j) {
            pos++;
            uint32_t c = COLOR_INVISIBLE;

            int p0 = pos;
            int p1 = pos - (m_pWidth - 1);
            int p2 = pos - 1;
            int p3 = pos - (m_pWidth - 1) - 1;

            if (visible[p0] && visible[p1] && visible[p2] && visible[p3]) {
                c = COLOR_VISIBLE;
            } else if (visited[p0] && visited[p1] && visited[p2] && visited[p3]) {
                c = pd::Preferences::brightness() ? 0x88111111 : COLOR_VISITED;
            } else if (mapped && mapped[p0] && mapped[p1] && mapped[p2] && mapped[p3]) {
                c = COLOR_MAPPED;
            }

            m_pixels[i * m_width2 + j] = c;
        }
    }

    m_texture->updateSubRGBA8(reinterpret_cast<const uint8_t*>(m_pixels.get()), m_width2, m_height2);
}

void FogOfWar::draw(const noosa::Camera* camera) {
    if (!visible || !alive || !m_image.tex || !m_image.subtex) {
        return;
    }

    float drawX = x;
    float drawY = y;
    float drawScaleX = scale.x;
    float drawScaleY = scale.y;

    if (camera != nullptr) {
        if (!camera->isVisible(x, y, width * std::abs(scale.x), height * std::abs(scale.y))) {
            return;
        }

        camera->worldToScreen(x, y, drawX, drawY);
        float zoom = camera->getZoom();
        drawScaleX *= zoom;
        drawScaleY *= zoom;
    }

    C2D_DrawParams params;
    params.pos.x = drawX;
    params.pos.y = drawY;
    params.pos.w = width * drawScaleX;
    params.pos.h = height * drawScaleY;
    params.center.x = 0.0f;
    params.center.y = 0.0f;
    params.depth = z;
    params.angle = 0.0f;

    C2D_DrawImage(m_image, &params, nullptr);
}

} // namespace pd
