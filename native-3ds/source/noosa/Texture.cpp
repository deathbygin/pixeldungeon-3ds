#include "noosa/Texture.h"
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <csetjmp>
#include <png.h>

namespace noosa {

std::unordered_map<std::string, std::shared_ptr<Texture>> Texture::s_cache;

int Texture::nextPOT(int v) {
    if (v <= 0) return 8;
    int p = 1;
    while (p < v) p <<= 1;
    // 3DS PICA200 textures must be at least 8x8
    return std::max(p, 8);
}

uint32_t Texture::getMortonTileOffset(uint32_t x, uint32_t y, uint32_t potWidth, uint32_t bytesPerPixel) {
    uint32_t tileX = x >> 3;
    uint32_t tileY = y >> 3;
    uint32_t inX = x & 7;
    uint32_t inY = y & 7;

    uint32_t tilesPerRow = potWidth >> 3;
    uint32_t tileIndex = tileY * tilesPerRow + tileX;

    // 8x8 Morton curve bit interleaving
    uint32_t morton = (inX & 1) | ((inY & 1) << 1) |
                      ((inX & 2) << 1) | ((inY & 2) << 2) |
                      ((inX & 4) << 2) | ((inY & 4) << 3);

    return (tileIndex * 64 + morton) * bytesPerPixel;
}

Texture::Texture() = default;

Texture::~Texture() {
    freeTexture();
}

Texture::Texture(Texture&& other) noexcept {
    m_tex = other.m_tex;
    m_spriteSheet = other.m_spriteSheet;
    m_ownsTex = other.m_ownsTex;
    m_width = other.m_width;
    m_height = other.m_height;
    m_potWidth = other.m_potWidth;
    m_potHeight = other.m_potHeight;

    other.m_ownsTex = false;
    other.m_spriteSheet = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        freeTexture();
        m_tex = other.m_tex;
        m_spriteSheet = other.m_spriteSheet;
        m_ownsTex = other.m_ownsTex;
        m_width = other.m_width;
        m_height = other.m_height;
        m_potWidth = other.m_potWidth;
        m_potHeight = other.m_potHeight;

        other.m_ownsTex = false;
        other.m_spriteSheet = nullptr;
    }
    return *this;
}

void Texture::freeTexture() {
    if (m_ownsTex) {
        C3D_TexDelete(&m_tex);
        m_ownsTex = false;
    }
    if (m_spriteSheet) {
        C2D_SpriteSheetFree(m_spriteSheet);
        m_spriteSheet = nullptr;
    }
    m_width = 0;
    m_height = 0;
    m_potWidth = 0;
    m_potHeight = 0;
}

bool Texture::loadFromRGBA8(const uint8_t* rgbaPixels, int width, int height) {
    if (!rgbaPixels || width <= 0 || height <= 0) {
        return false;
    }

    freeTexture();

    m_width = width;
    m_height = height;
    m_potWidth = nextPOT(width);
    m_potHeight = nextPOT(height);

    if (!C3D_TexInit(&m_tex, static_cast<u16>(m_potWidth), static_cast<u16>(m_potHeight), GPU_RGBA8)) {
        return false;
    }
    m_ownsTex = true;

    // Set pixel art friendly filtering: NEAREST
    C3D_TexSetFilter(&m_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&m_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    // Clear destination linear buffer to zero
    uint8_t* dst = static_cast<uint8_t*>(m_tex.data);
    std::memset(dst, 0, m_potWidth * m_potHeight * 4);

    // Convert row-major linear RGBA8 to 3DS 8x8 Morton tiled format
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcRow = rgbaPixels + (y * width * 4);
        for (int x = 0; x < width; ++x) {
            uint32_t dstOffset = getMortonTileOffset(static_cast<uint32_t>(x),
                                                     static_cast<uint32_t>(y),
                                                     static_cast<uint32_t>(m_potWidth),
                                                     4);
            const uint8_t* srcPixel = srcRow + (x * 4);
            dst[dstOffset + 0] = srcPixel[3]; // PICA200 RGBA8 order: byte 0 = A, 1 = B, 2 = G, 3 = R (or standard RGBA)
            dst[dstOffset + 1] = srcPixel[2];
            dst[dstOffset + 2] = srcPixel[1];
            dst[dstOffset + 3] = srcPixel[0];
        }
    }

    // Flush CPU data cache to VRAM/main memory for PICA200
    C3D_TexFlush(&m_tex);
    return true;
}

bool Texture::createSolidColor(int width, int height, uint32_t color) {
    if (width <= 0 || height <= 0) return false;

    freeTexture();

    m_width = width;
    m_height = height;
    m_potWidth = nextPOT(width);
    m_potHeight = nextPOT(height);

    if (!C3D_TexInit(&m_tex, static_cast<u16>(m_potWidth), static_cast<u16>(m_potHeight), GPU_RGBA8)) {
        return false;
    }
    m_ownsTex = true;

    C3D_TexSetFilter(&m_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&m_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    uint8_t a = static_cast<uint8_t>((color >> 24) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    uint8_t r = static_cast<uint8_t>(color & 0xFF);

    uint8_t* dst = static_cast<uint8_t*>(m_tex.data);
    std::memset(dst, 0, m_potWidth * m_potHeight * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t dstOffset = getMortonTileOffset(static_cast<uint32_t>(x),
                                                     static_cast<uint32_t>(y),
                                                     static_cast<uint32_t>(m_potWidth),
                                                     4);
            dst[dstOffset + 0] = a;
            dst[dstOffset + 1] = b;
            dst[dstOffset + 2] = g;
            dst[dstOffset + 3] = r;
        }
    }

    C3D_TexFlush(&m_tex);
    return true;
}

bool Texture::updateSubRGBA8(const uint8_t* rgbaPixels, int width, int height) {
    if (!m_ownsTex || !m_tex.data || !rgbaPixels || width <= 0 || height <= 0) {
        return false;
    }

    uint8_t* dst = static_cast<uint8_t*>(m_tex.data);
    int updateW = std::min(width, m_potWidth);
    int updateH = std::min(height, m_potHeight);

    for (int y = 0; y < updateH; ++y) {
        const uint8_t* srcRow = rgbaPixels + (y * width * 4);
        for (int x = 0; x < updateW; ++x) {
            uint32_t dstOffset = getMortonTileOffset(static_cast<uint32_t>(x),
                                                     static_cast<uint32_t>(y),
                                                     static_cast<uint32_t>(m_potWidth),
                                                     4);
            const uint8_t* srcPixel = srcRow + (x * 4);
            dst[dstOffset + 0] = srcPixel[3]; // A
            dst[dstOffset + 1] = srcPixel[2]; // B
            dst[dstOffset + 2] = srcPixel[1]; // G
            dst[dstOffset + 3] = srcPixel[0]; // R
        }
    }

    C3D_TexFlush(&m_tex);
    return true;
}

void Texture::setFilter(GPU_TEXTURE_FILTER_PARAM mag, GPU_TEXTURE_FILTER_PARAM min) {
    if (m_ownsTex) {
        C3D_TexSetFilter(&m_tex, mag, min);
    }
}

void Texture::setWrap(GPU_TEXTURE_WRAP_PARAM wrapS, GPU_TEXTURE_WRAP_PARAM wrapT) {
    if (m_ownsTex) {
        C3D_TexSetWrap(&m_tex, wrapS, wrapT);
    }
}

bool Texture::loadFromT3X(const char* filepath) {
    if (!filepath) return false;

    freeTexture();

    m_spriteSheet = C2D_SpriteSheetLoad(filepath);
    if (!m_spriteSheet) {
        return false;
    }

    size_t count = C2D_SpriteSheetCount(m_spriteSheet);
    if (count > 0) {
        C2D_Image firstImg = C2D_SpriteSheetGetImage(m_spriteSheet, 0);
        if (firstImg.subtex) {
            m_width = firstImg.subtex->width;
            m_height = firstImg.subtex->height;
        }
        if (firstImg.tex) {
            m_potWidth = firstImg.tex->width;
            m_potHeight = firstImg.tex->height;
        }
    }

    return true;
}

bool Texture::loadPNG(const char* filepath) {
    if (!filepath) return false;

    FILE* fp = std::fopen(filepath, "rb");
    if (!fp) return false;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        std::fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    png_uint_32 imgWidth = 0;
    png_uint_32 imgHeight = 0;
    int bitDepth = 0;
    int colorType = 0;
    png_get_IHDR(png, info, &imgWidth, &imgHeight, &bitDepth, &colorType, nullptr, nullptr, nullptr);

    // Transform formats to standard 32-bit RGBA8
    if (bitDepth == 16) {
        png_set_strip_16(png);
    }
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    int w = static_cast<int>(imgWidth);
    int h = static_cast<int>(imgHeight);
    std::vector<uint8_t> pixels(w * h * 4);
    std::vector<png_bytep> rows(h);
    for (int y = 0; y < h; ++y) {
        rows[y] = pixels.data() + (y * w * 4);
    }

    png_read_image(png, rows.data());
    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(fp);

    return loadFromRGBA8(pixels.data(), w, h);
}

std::shared_ptr<Texture> Texture::get(const std::string& key) {
    auto it = s_cache.find(key);
    if (it != s_cache.end()) {
        return it->second;
    }
    auto tex = std::make_shared<Texture>();
    if (key.length() >= 4 && key.rfind(".png") == key.length() - 4) {
        tex->loadPNG(key.c_str());
    } else if (key.length() >= 4 && key.rfind(".t3x") == key.length() - 4) {
        tex->loadFromT3X(key.c_str());
    }
    s_cache[key] = tex;
    return tex;
}

void Texture::clearCache() {
    s_cache.clear();
}

} // namespace noosa

