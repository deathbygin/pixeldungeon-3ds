#include "pd/Effects.h"
#include "pd/Assets.h"
#include "noosa/Texture.h"

namespace pd {

std::unique_ptr<noosa::Image> Effects::get(Type type) {
    auto tex = noosa::Texture::get(Assets::EFFECTS);
    auto img = std::make_unique<noosa::Image>(tex);

    switch (type) {
    case Type::RIPPLE:
        img->setFrame(0, 0, 16, 16);
        break;
    case Type::LIGHTNING:
        img->setFrame(16, 0, 16, 8);
        break;
    case Type::WOUND:
        img->setFrame(16, 8, 16, 8);
        break;
    case Type::RAY:
        img->setFrame(16, 16, 16, 8);
        break;
    }

    img->targetScreen = noosa::ScreenTarget::Top;
    return img;
}

} // namespace pd

