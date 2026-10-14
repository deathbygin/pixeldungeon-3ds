#include "noosa/Image.h"
#include "noosa/Camera.h"
#include <cmath>

namespace noosa {

Image::Image() = default;

Image::Image(std::shared_ptr<Texture> texture) {
    setTexture(std::move(texture));
}

Image::Image(std::shared_ptr<TextureFilm> film, int frameIndex) {
    setFilm(std::move(film), frameIndex);
}

void Image::setTexture(std::shared_ptr<Texture> texture) {
    m_texture = std::move(texture);
    m_film.reset();
    m_useCustomSubTex = false;

    if (m_texture && m_texture->isLoaded()) {
        width = static_cast<float>(m_texture->width());
        height = static_cast<float>(m_texture->height());
        setFrame(0, 0, m_texture->width(), m_texture->height());
    } else {
        m_image = { nullptr, nullptr };
    }
}

void Image::setFilm(std::shared_ptr<TextureFilm> film, int frameIndex) {
    m_film = std::move(film);
    m_useCustomSubTex = false;

    if (m_film) {
        m_texture = m_film->getTexture();
        setFrame(frameIndex);
    } else {
        m_texture.reset();
        m_image = { nullptr, nullptr };
    }
}

void Image::setFrame(int frameIndex) {
    if (m_film) {
        m_image = m_film->get(frameIndex);
        m_useCustomSubTex = false;
        if (m_image.subtex) {
            width = static_cast<float>(m_image.subtex->width);
            height = static_cast<float>(m_image.subtex->height);
        }
    }
}

void Image::setFrame(const Rect& rect) {
    setFrame(rect.x, rect.y, rect.width, rect.height);
}

void Image::setFrame(int fx, int fy, int fw, int fh) {
    if (!m_texture || !m_texture->isLoaded() || fw <= 0 || fh <= 0) {
        return;
    }

    float potW = static_cast<float>(m_texture->potWidth());
    float potH = static_cast<float>(m_texture->potHeight());

    m_customSubTex.width = static_cast<u16>(fw);
    m_customSubTex.height = static_cast<u16>(fh);
    m_customSubTex.left = static_cast<float>(fx) / potW;
    m_customSubTex.right = static_cast<float>(fx + fw) / potW;
    m_customSubTex.top = 1.0f - (static_cast<float>(fy) / potH);
    m_customSubTex.bottom = 1.0f - (static_cast<float>(fy + fh) / potH);

    m_image.tex = m_texture->getTex();
    m_image.subtex = &m_customSubTex;
    m_useCustomSubTex = true;

    width = static_cast<float>(fw);
    height = static_cast<float>(fh);
}

void Image::setFrame(float fx, float fy, float fw, float fh) {
    if (!m_texture || !m_texture->isLoaded() || fw <= 0.0f || fh <= 0.0f) {
        return;
    }

    float potW = static_cast<float>(m_texture->potWidth());
    float potH = static_cast<float>(m_texture->potHeight());

    m_customSubTex.width = static_cast<u16>(fw);
    m_customSubTex.height = static_cast<u16>(fh);
    m_customSubTex.left = fx / potW;
    m_customSubTex.right = (fx + fw) / potW;
    m_customSubTex.top = 1.0f - (fy / potH);
    m_customSubTex.bottom = 1.0f - ((fy + fh) / potH);

    m_image.tex = m_texture->getTex();
    m_image.subtex = &m_customSubTex;
    m_useCustomSubTex = true;

    width = fw;
    height = fh;
}

void Image::draw(const Camera* camera) {
    if (!visible || !alive || alpha <= 0.0f || !m_image.tex || !m_image.subtex) {
        return;
    }

    float drawX = x;
    float drawY = y;
    float drawScaleX = scale.x;
    float drawScaleY = scale.y;

    if (targetScreen == ScreenTarget::Top && camera != nullptr) {
        // Culling in world space
        if (!camera->isVisible(x, y, width * std::abs(scale.x), height * std::abs(scale.y))) {
            return;
        }

        camera->worldToScreen(x, y, drawX, drawY);
        float zoom = camera->getZoom();
        drawScaleX *= zoom;
        drawScaleY *= zoom;
    }

    if (flipHorizontal) {
        drawScaleX = -drawScaleX;
    }
    if (flipVertical) {
        drawScaleY = -drawScaleY;
    }

    C2D_DrawParams params;
    params.pos.x = drawX;
    params.pos.y = drawY;
    params.pos.w = width * drawScaleX;
    params.pos.h = height * drawScaleY;
    params.center.x = origin.x * std::abs(drawScaleX);
    params.center.y = origin.y * std::abs(drawScaleY);
    params.depth = z;
    params.angle = angle;

    uint8_t effectiveAlpha = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * tint.a);
    if (effectiveAlpha != 255 || tint.r != 255 || tint.g != 255 || tint.b != 255) {
        C2D_ImageTint c2dTint;
        u32 color = C2D_Color32(tint.r, tint.g, tint.b, effectiveAlpha);
        C2D_PlainImageTint(&c2dTint, color, 1.0f);
        C2D_DrawImage(m_image, &params, &c2dTint);
    } else {
        C2D_DrawImage(m_image, &params, nullptr);
    }
}

} // namespace noosa

