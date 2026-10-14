#pragma once

#include "Types.h"
#include "Texture.h"
#include <citro2d.h>
#include <tex3ds.h>
#include <vector>
#include <memory>

namespace noosa {

class TextureFilm {
public:
    TextureFilm();
    explicit TextureFilm(std::shared_ptr<Texture> texture);
    TextureFilm(std::shared_ptr<Texture> texture, int frameWidth, int frameHeight);
    ~TextureFilm() = default;

    // Slices texture into a uniform grid of frames of size frameWidth x frameHeight
    void sliceUniform(int frameWidth, int frameHeight, int margin = 0, int spacing = 0);

    // Adds a custom frame rectangle in pixel coordinates
    int addFrame(int x, int y, int width, int height);
    int addFrame(const Rect& rect);

    // Get C2D_Image for a given frame index
    C2D_Image get(int index) const;

    // Number of frames
    size_t count() const { return m_subTextures.size(); }
    size_t size() const { return m_subTextures.size(); }

    int frameWidth(int index = 0) const;
    int frameHeight(int index = 0) const;

    std::shared_ptr<Texture> getTexture() const { return m_texture; }
    void setTexture(std::shared_ptr<Texture> texture) { m_texture = texture; }

private:
    std::shared_ptr<Texture> m_texture;
    std::vector<Tex3DS_SubTexture> m_subTextures;
};

} // namespace noosa

