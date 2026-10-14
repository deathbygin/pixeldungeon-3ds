#pragma once

#include "noosa/Image.h"
#include "pd/Assets.h"
#include <memory>

namespace pd {

class Effects {
public:
    enum class Type {
        RIPPLE,
        LIGHTNING,
        WOUND,
        RAY
    };

    // Creates an Image sliced to the appropriate region of effects.png
    static std::unique_ptr<noosa::Image> get(Type type);
};

} // namespace pd

