#pragma once

#include "noosa/Texture.h"
#include "pd/ui/TouchController.h"
#include <citro2d.h>
#include <memory>
#include <string>

namespace pd {
class Hero;
class Item;
}

namespace pd::ui {

class Toolbar {
public:
    static constexpr float HEIGHT = 25.0f;
    static constexpr float Y_POS  = 210.0f;
    static constexpr float WIDTH  = 320.0f;

    enum class Action {
        NONE,
        WAIT,
        SEARCH,
        INFO,
        BAG,
        QUICK1,
        QUICK2
    };

    Toolbar();
    ~Toolbar() = default;

    Action update(const TouchController& touch, u32 kDown, pd::Hero* hero);
    void draw(const pd::Hero* hero, C2D_TextBuf textBuf, bool isTargeting = false, const pd::Item* targetItem = nullptr, bool isExamining = false) const;

    void setQuickItem1(pd::Item* it) { m_quickItem1 = it; }
    void setQuickItem2(pd::Item* it) { m_quickItem2 = it; }
    pd::Item* getQuickItem1() const { return m_quickItem1; }
    pd::Item* getQuickItem2() const { return m_quickItem2; }

private:
    std::shared_ptr<noosa::Texture> m_texToolbar;
    std::shared_ptr<noosa::Texture> m_texItems;
    std::shared_ptr<noosa::Texture> m_texIcons;

    pd::Item* m_quickItem1 = nullptr;
    pd::Item* m_quickItem2 = nullptr;

    mutable Tex3DS_SubTexture m_subWait{};
    mutable Tex3DS_SubTexture m_subSearch{};
    mutable Tex3DS_SubTexture m_subInfo{};
    mutable Tex3DS_SubTexture m_subBag{};
    mutable Tex3DS_SubTexture m_subQuickSlot1{};
    mutable Tex3DS_SubTexture m_subQuickSlot2{};
    mutable Tex3DS_SubTexture m_subItem1{};
    mutable Tex3DS_SubTexture m_subItem2{};
    mutable Tex3DS_SubTexture m_subTargetCross{};

    int m_pressedTool = -1; // 0: wait, 1: search, 2: info, 3: bag, 4: q2, 5: q1
};

} // namespace pd::ui
