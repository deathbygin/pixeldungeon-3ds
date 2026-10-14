#pragma once

#include "Types.h"
#include <citro3d.h>
#include <citro2d.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace noosa {

class Texture {
public:
    Texture();
    ~Texture();

    // Non-copyable, movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Load from raw linear RGBA8 pixel data (converts to PICA200 8x8 tiled format)
    bool loadFromRGBA8(const uint8_t* rgbaPixels, int width, int height);

    // Load from a .png image file (RomFS or SDMC, decoded via libpng)
    bool loadPNG(const char* filepath);

    // Load from a .t3x spritesheet file (packaged asset)
    bool loadFromT3X(const char* filepath);

    // Create a solid color texture (useful for UI panels, debug, placeholders)
    bool createSolidColor(int width, int height, uint32_t color);

    // Update existing texture data in-place from linear RGBA8 buffer
    bool updateSubRGBA8(const uint8_t* rgbaPixels, int width, int height);

    // Texture filtering and wrap controls
    void setFilter(GPU_TEXTURE_FILTER_PARAM mag, GPU_TEXTURE_FILTER_PARAM min);
    void setWrap(GPU_TEXTURE_WRAP_PARAM wrapS, GPU_TEXTURE_WRAP_PARAM wrapT);

    // Free resources
    void freeTexture();

    C3D_Tex* getTex() { return m_ownsTex ? &m_tex : nullptr; }
    const C3D_Tex* getTex() const { return m_ownsTex ? &m_tex : nullptr; }

    C2D_SpriteSheet getSpriteSheet() const { return m_spriteSheet; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int potWidth() const { return m_potWidth; }
    int potHeight() const { return m_potHeight; }
    bool isLoaded() const { return m_ownsTex || m_spriteSheet != nullptr; }

    // Global texture cache (maps resource path/name to Texture singleton)
    static std::shared_ptr<Texture> get(const std::string& key);
    static void clearCache();

    // Helper to calculate next power-of-two
    static int nextPOT(int v);

    // Helper to calculate PICA200 8x8 Morton tile memory offset
    static uint32_t getMortonTileOffset(uint32_t x, uint32_t y, uint32_t potWidth, uint32_t bytesPerPixel);

private:
    C3D_Tex m_tex{};
    C2D_SpriteSheet m_spriteSheet = nullptr;
    bool m_ownsTex = false;

    int m_width = 0;
    int m_height = 0;
    int m_potWidth = 0;
    int m_potHeight = 0;

    static std::unordered_map<std::string, std::shared_ptr<Texture>> s_cache;
};

} // namespace noosa

