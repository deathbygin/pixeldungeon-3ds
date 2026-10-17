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

struct QuickSlot {
    mutable pd::Item* item = nullptr;
    std::string itemClass;
    int placeholderImage = -1;
    std::string placeholderName;
    bool isStackable = false;
    std::unique_ptr<pd::Item> virtualItem = nullptr;

    QuickSlot() = default;
    ~QuickSlot() = default;
    QuickSlot(QuickSlot&&) noexcept = default;
    QuickSlot& operator=(QuickSlot&&) noexcept = default;
    QuickSlot(const QuickSlot&) = delete;
    QuickSlot& operator=(const QuickSlot&) = delete;

    void set(pd::Item* it);
    void clear();
    bool isEmpty() const;
    bool isGhost(const pd::Hero* hero = nullptr) const;
    pd::Item* resolve(const pd::Hero* hero = nullptr) const;
};

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
    Toolbar(Toolbar&&) noexcept = default;
    Toolbar& operator=(Toolbar&&) noexcept = default;
    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    Action update(const TouchController& touch, u32 kDown, pd::Hero* hero);
    void draw(const pd::Hero* hero, C2D_TextBuf textBuf, bool isTargeting = false, const pd::Item* targetItem = nullptr, bool isExamining = false) const;

    void setQuickItem1(pd::Item* it) { m_slot1.set(it); }
    void setQuickItem2(pd::Item* it) { m_slot2.set(it); }
    pd::Item* getQuickItem1(const pd::Hero* hero = nullptr) const { return m_slot1.resolve(hero); }
    pd::Item* getQuickItem2(const pd::Hero* hero = nullptr) const { return m_slot2.resolve(hero); }

    const QuickSlot& getSlot1() const { return m_slot1; }
    const QuickSlot& getSlot2() const { return m_slot2; }
    QuickSlot& getSlot1() { return m_slot1; }
    QuickSlot& getSlot2() { return m_slot2; }
    void clearQuickslots() { m_slot1.clear(); m_slot2.clear(); }

private:
    std::shared_ptr<noosa::Texture> m_texToolbar;
    std::shared_ptr<noosa::Texture> m_texItems;
    std::shared_ptr<noosa::Texture> m_texIcons;

    QuickSlot m_slot1;
    QuickSlot m_slot2;

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
