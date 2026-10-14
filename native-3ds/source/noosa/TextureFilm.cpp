#include "noosa/TextureFilm.h"

namespace noosa {

TextureFilm::TextureFilm() = default;

TextureFilm::TextureFilm(std::shared_ptr<Texture> texture)
    : m_texture(std::move(texture)) {
    if (m_texture && m_texture->isLoaded()) {
        addFrame(0, 0, m_texture->width(), m_texture->height());
    }
}

TextureFilm::TextureFilm(std::shared_ptr<Texture> texture, int frameWidth, int frameHeight)
    : m_texture(std::move(texture)) {
    if (m_texture && m_texture->isLoaded()) {
        sliceUniform(frameWidth, frameHeight);
    }
}

void TextureFilm::sliceUniform(int frameWidth, int frameHeight, int margin, int spacing) {
    if (!m_texture || !m_texture->isLoaded() || frameWidth <= 0 || frameHeight <= 0) {
        return;
    }

    m_subTextures.clear();

    int texW = m_texture->width();
    int texH = m_texture->height();

    for (int y = margin; y + frameHeight <= texH; y += frameHeight + spacing) {
        for (int x = margin; x + frameWidth <= texW; x += frameWidth + spacing) {
            addFrame(x, y, frameWidth, frameHeight);
        }
    }
}

int TextureFilm::addFrame(int x, int y, int width, int height) {
    if (!m_texture || !m_texture->isLoaded() || width <= 0 || height <= 0) {
        return -1;
    }

    float potW = static_cast<float>(m_texture->potWidth());
    float potH = static_cast<float>(m_texture->potHeight());

    Tex3DS_SubTexture sub;
    sub.width = static_cast<u16>(width);
    sub.height = static_cast<u16>(height);
    sub.left = static_cast<float>(x) / potW;
    sub.right = static_cast<float>(x + width) / potW;
    sub.top = 1.0f - (static_cast<float>(y) / potH);
    sub.bottom = 1.0f - (static_cast<float>(y + height) / potH);

    int index = static_cast<int>(m_subTextures.size());
    m_subTextures.push_back(sub);
    return index;
}

int TextureFilm::addFrame(const Rect& rect) {
    return addFrame(rect.x, rect.y, rect.width, rect.height);
}

C2D_Image TextureFilm::get(int index) const {
    C2D_Image image = { nullptr, nullptr };
    if (!m_texture) return image;

    if (m_texture->getSpriteSheet()) {
        return C2D_SpriteSheetGetImage(m_texture->getSpriteSheet(), static_cast<size_t>(index));
    }

    if (index >= 0 && static_cast<size_t>(index) < m_subTextures.size()) {
        image.tex = m_texture->getTex();
        image.subtex = &m_subTextures[index];
    }
    return image;
}

int TextureFilm::frameWidth(int index) const {
    if (index >= 0 && static_cast<size_t>(index) < m_subTextures.size()) {
        return m_subTextures[index].width;
    }
    return m_texture ? m_texture->width() : 0;
}

int TextureFilm::frameHeight(int index) const {
    if (index >= 0 && static_cast<size_t>(index) < m_subTextures.size()) {
        return m_subTextures[index].height;
    }
    return m_texture ? m_texture->height() : 0;
}

} // namespace noosa

