#pragma once

#include "noosa/Visual.h"
#include "noosa/Texture.h"
#include "noosa/Camera.h"
#include <citro2d.h>
#include <memory>
#include <cstdint>

namespace pd {

class FogOfWar : public noosa::Visual {
public:
    static constexpr uint32_t COLOR_VISIBLE   = 0x00000000;
    static constexpr uint32_t COLOR_VISITED   = 0xCC111111;
    static constexpr uint32_t COLOR_MAPPED    = 0xCC112244;
    static constexpr uint32_t COLOR_INVISIBLE = 0xFF000000;

    FogOfWar(int mapWidth, int mapHeight);
    ~FogOfWar() override = default;

    void updateVisibility(const bool* visible, const bool* visited, const bool* mapped = nullptr);

    void draw(const noosa::Camera* camera = nullptr) override;

private:
    int m_mapWidth;
    int m_mapHeight;
    int m_pWidth;
    int m_pHeight;
    int m_width2;
    int m_height2;

    std::unique_ptr<uint32_t[]> m_pixels;
    std::shared_ptr<noosa::Texture> m_texture;
    C2D_Image m_image{ nullptr, nullptr };
    Tex3DS_SubTexture m_subTex{};
};

} // namespace pd

