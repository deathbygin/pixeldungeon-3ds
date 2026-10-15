#include "noosa/NinePatch.h"
#include <algorithm>
#include <cmath>

namespace noosa {

NinePatch::NinePatch(std::shared_ptr<Texture> texture, int srcX, int srcY, int srcW, int srcH, int margin) {
    init(std::move(texture), srcX, srcY, srcW, srcH, margin, margin, margin, margin);
}

NinePatch::NinePatch(std::shared_ptr<Texture> texture, int srcX, int srcY, int srcW, int srcH,
                     int left, int top, int right, int bottom) {
    init(std::move(texture), srcX, srcY, srcW, srcH, left, top, right, bottom);
}

NinePatch::NinePatch(const std::string& assetPath, int srcX, int srcY, int srcW, int srcH, int margin) {
    init(Texture::get(assetPath), srcX, srcY, srcW, srcH, margin, margin, margin, margin);
}

NinePatch::NinePatch(const std::string& assetPath, int srcX, int srcY, int srcW, int srcH,
                     int left, int top, int right, int bottom) {
    init(Texture::get(assetPath), srcX, srcY, srcW, srcH, left, top, right, bottom);
}

void NinePatch::init(std::shared_ptr<Texture> texture, int srcX, int srcY, int srcW, int srcH,
                     int left, int top, int right, int bottom) {
    m_texture = std::move(texture);
    m_srcX = srcX;
    m_srcY = srcY;
    m_srcW = srcW;
    m_srcH = srcH;
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;

    if (!m_texture || !m_texture->isLoaded() || srcW <= 0 || srcH <= 0) {
        m_initialized = false;
        return;
    }

    float potW = static_cast<float>(m_texture->potWidth());
    float potH = static_cast<float>(m_texture->potHeight());
    C3D_Tex* c3dTex = m_texture->getTex();

    // Column widths: [0] = left, [1] = center, [2] = right
    m_sliceW[0] = left;
    m_sliceW[1] = std::max(0, srcW - left - right);
    m_sliceW[2] = right;

    // Row heights: [0] = top, [1] = middle, [2] = bottom
    m_sliceH[0] = top;
    m_sliceH[1] = std::max(0, srcH - top - bottom);
    m_sliceH[2] = bottom;

    // Source X offsets for each column
    int curX = srcX;
    int srcColX[3];
    for (int c = 0; c < 3; ++c) {
        srcColX[c] = curX;
        curX += m_sliceW[c];
    }

    // Source Y offsets for each row
    int curY = srcY;
    int srcRowY[3];
    for (int r = 0; r < 3; ++r) {
        srcRowY[r] = curY;
        curY += m_sliceH[r];
    }

    // Precalculate subtextures and images for all 9 cells
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int sw = m_sliceW[c];
            int sh = m_sliceH[r];

            if (sw > 0 && sh > 0) {
                m_valid[r][c] = true;
                Tex3DS_SubTexture& sub = m_subTex[r][c];
                sub.width = static_cast<u16>(sw);
                sub.height = static_cast<u16>(sh);
                sub.left = static_cast<float>(srcColX[c]) / potW;
                sub.right = static_cast<float>(srcColX[c] + sw) / potW;
                sub.top = 1.0f - (static_cast<float>(srcRowY[r]) / potH);
                sub.bottom = 1.0f - (static_cast<float>(srcRowY[r] + sh) / potH);

                m_images[r][c].tex = c3dTex;
                m_images[r][c].subtex = &m_subTex[r][c];
            } else {
                m_valid[r][c] = false;
                m_images[r][c] = { nullptr, nullptr };
            }
        }
    }

    m_initialized = true;
}

void NinePatch::draw(float x, float y, float w, float h, float z,
                     u32 tintColor, float brightness) const {
    if (!m_initialized) return;

    // Calculate destination slice dimensions
    float dstW[3];
    dstW[0] = static_cast<float>(m_left);
    dstW[2] = static_cast<float>(m_right);
    dstW[1] = std::max(0.0f, w - dstW[0] - dstW[2]);

    float dstH[3];
    dstH[0] = static_cast<float>(m_top);
    dstH[2] = static_cast<float>(m_bottom);
    dstH[1] = std::max(0.0f, h - dstH[0] - dstH[2]);

    // Destination positions
    float curDstX = x;
    float posX[3];
    for (int c = 0; c < 3; ++c) {
        posX[c] = curDstX;
        curDstX += dstW[c];
    }

    float curDstY = y;
    float posY[3];
    for (int r = 0; r < 3; ++r) {
        posY[r] = curDstY;
        curDstY += dstH[r];
    }

    // Prepare tint if requested
    bool useTint = (tintColor != 0xFFFFFFFF || brightness != 1.0f);
    C2D_ImageTint c2dTint;
    if (useTint) {
        if (brightness != 1.0f) {
            u8 r = static_cast<u8>(std::clamp(((tintColor >> 0) & 0xff) * brightness, 0.0f, 255.0f));
            u8 g = static_cast<u8>(std::clamp(((tintColor >> 8) & 0xff) * brightness, 0.0f, 255.0f));
            u8 b = static_cast<u8>(std::clamp(((tintColor >> 16) & 0xff) * brightness, 0.0f, 255.0f));
            u8 a = static_cast<u8>((tintColor >> 24) & 0xff);
            C2D_PlainImageTint(&c2dTint, C2D_Color32(r, g, b, a), 1.0f);
        } else {
            C2D_PlainImageTint(&c2dTint, tintColor, 1.0f);
        }
    }

    // Draw each slice
    for (int r = 0; r < 3; ++r) {
        if (dstH[r] <= 0.0f) continue;
        for (int c = 0; c < 3; ++c) {
            if (dstW[c] <= 0.0f || !m_valid[r][c]) continue;

            C2D_DrawParams params;
            params.pos.x = posX[c];
            params.pos.y = posY[r];
            params.pos.w = dstW[c];
            params.pos.h = dstH[r];
            params.center.x = 0.0f;
            params.center.y = 0.0f;
            params.depth = z;
            params.angle = 0.0f;

            C2D_DrawImage(m_images[r][c], &params, useTint ? &c2dTint : nullptr);
        }
    }
}

} // namespace noosa

