#include "pd/ui/Chrome.h"
#include "pd/Assets.h"

namespace pd::ui {

const noosa::NinePatch& Chrome::get(Type type) {
    static noosa::NinePatch s_window(Assets::CHROME, 0, 0, 22, 22, 7);
    static noosa::NinePatch s_toast(Assets::CHROME, 22, 0, 18, 18, 5);
    static noosa::NinePatch s_toastTr(Assets::CHROME, 40, 0, 18, 18, 5);
    static noosa::NinePatch s_button(Assets::CHROME, 58, 0, 6, 6, 2);
    static noosa::NinePatch s_tag(Assets::CHROME, 22, 18, 16, 14, 3);
    static noosa::NinePatch s_scroll(Assets::CHROME, 32, 32, 32, 32, 5, 11, 5, 11);
    static noosa::NinePatch s_tabSet(Assets::CHROME, 64, 0, 22, 22, 7, 7, 7, 7);
    static noosa::NinePatch s_tabSelected(Assets::CHROME, 64, 22, 10, 14, 4, 7, 4, 6);
    static noosa::NinePatch s_tabUnselected(Assets::CHROME, 74, 22, 10, 14, 4, 7, 4, 6);

    switch (type) {
        case Type::WINDOW:         return s_window;
        case Type::TOAST:          return s_toast;
        case Type::TOAST_TR:       return s_toastTr;
        case Type::BUTTON:         return s_button;
        case Type::TAG:            return s_tag;
        case Type::SCROLL:         return s_scroll;
        case Type::TAB_SET:        return s_tabSet;
        case Type::TAB_SELECTED:   return s_tabSelected;
        case Type::TAB_UNSELECTED: return s_tabUnselected;
        default:                   return s_window;
    }
}

} // namespace pd::ui

