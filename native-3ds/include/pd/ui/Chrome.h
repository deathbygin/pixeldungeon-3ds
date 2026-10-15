#pragma once

#include "noosa/NinePatch.h"

namespace pd::ui {

class Chrome {
public:
    enum class Type {
        WINDOW,
        TOAST,
        TOAST_TR,
        BUTTON,
        TAG,
        SCROLL,
        TAB_SET,
        TAB_SELECTED,
        TAB_UNSELECTED
    };

    static const noosa::NinePatch& get(Type type);
};

} // namespace pd::ui

