#pragma once

#include "noosa/Texture.h"
#include <citro2d.h>
#include <memory>
#include <string>

namespace noosa {

class NinePatch {
public:
    NinePatch() = default;
    NinePatch(std::shared_ptr<Texture> texture, int srcX, int srcY, int srcW, int srcH, int margin);
    NinePatch(std::shared_ptr<Texture> texture, int srcX, int srcY, int srcW, int srcH,
              int left, int top, int right, int bottom);
    NinePatch(const std::string& assetPath, int srcX, int srcY, int srcW, int srcH, int margin);
    NinePatch(const std::string& assetPath, int srcX, int srcY, int srcW, int srcH,
              int left, int top, int right, int bottom);

    void init(std::shared_ptr<Texture> texture, int srcX, int srcY, int srcW, int srcH,
              int left, int top, int right, int bottom);

    // Draws the 9-patch stretched to fit [x, y, w, h] at depth z
    void draw(float x, float y, float w, float h, float z = 0.5f,
              u32 tintColor = 0xFFFFFFFF, float brightness = 1.0f) const;

    int marginLeft() const { return m_left; }
    int marginTop() const { return m_top; }
    int marginRight() const { return m_right; }
    int marginBottom() const { return m_bottom; }

    int minWidth() const { return m_left + m_right; }
    int minHeight() const { return m_top + m_bottom; }

    bool isInitialized() const { return m_initialized; }

private:
    std::shared_ptr<Texture> m_texture;
    int m_srcX = 0, m_srcY = 0, m_srcW = 0, m_srcH = 0;
    int m_left = 0, m_top = 0, m_right = 0, m_bottom = 0;

    // 3x3 grid of subtextures and images
    // [row][col]: row 0 = top, 1 = mid, 2 = bot; col 0 = left, 1 = mid, 2 = right
    Tex3DS_SubTexture m_subTex[3][3]{};
    C2D_Image m_images[3][3]{};
    int m_sliceW[3]{};
    int m_sliceH[3]{};
    bool m_valid[3][3]{};
    bool m_initialized = false;
};

} // namespace noosa

