#pragma once

#include "Visual.h"
#include "Texture.h"
#include "TextureFilm.h"
#include <citro2d.h>
#include <memory>

namespace noosa {

class Image : public Visual {
public:
    Image();
    explicit Image(std::shared_ptr<Texture> texture);
    Image(std::shared_ptr<TextureFilm> film, int frameIndex = 0);
    ~Image() override = default;

    void setTexture(std::shared_ptr<Texture> texture);
    void setFilm(std::shared_ptr<TextureFilm> film, int frameIndex = 0);

    void setFrame(int frameIndex);
    void setFrame(const Rect& rect);
    void setFrame(int fx, int fy, int fw, int fh);
    void setFrame(float fx, float fy, float fw, float fh);

    bool flipHorizontal = false;
    bool flipVertical = false;

    void draw(const Camera* camera = nullptr) override;

    const C2D_Image& getC2DImage() const { return m_image; }
    std::shared_ptr<Texture> getTexture() const { return m_texture; }
    std::shared_ptr<TextureFilm> getFilm() const { return m_film; }

protected:
    std::shared_ptr<Texture> m_texture;
    std::shared_ptr<TextureFilm> m_film;
    C2D_Image m_image{ nullptr, nullptr };
    Tex3DS_SubTexture m_customSubTex{};
    bool m_useCustomSubTex = false;
};

} // namespace noosa

