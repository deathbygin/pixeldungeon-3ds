#pragma once

#include "noosa/Texture.h"
#include "pd/ui/TouchController.h"
#include <citro2d.h>
#include <memory>
#include <string>

namespace pd {
class Hero;
}

namespace pd::ui {

class StatusPane {
public:
    static constexpr float HEIGHT = 33.0f;
    static constexpr float WIDTH  = 320.0f;

    StatusPane();
    ~StatusPane() = default;

    // Updates touch interactions. Returns:
    // 1: Avatar clicked (toggle hero info / WndHero)
    // 2: Menu button clicked (toggle pause / WndGame / options)
    // 0: No action
    int update(const TouchController& touch, u32 kDown);

    void draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) const;

private:
    std::shared_ptr<noosa::Texture> m_texStatus;
    std::shared_ptr<noosa::Texture> m_texHp;
    std::shared_ptr<noosa::Texture> m_texExp;
    std::shared_ptr<noosa::Texture> m_texBuffs;
    std::shared_ptr<noosa::Texture> m_texIcons;
    std::shared_ptr<noosa::Texture> m_texItems;

    mutable Tex3DS_SubTexture m_subShield{};
    mutable Tex3DS_SubTexture m_subTopBar{};
    mutable Tex3DS_SubTexture m_subBtnMenu{};
    mutable Tex3DS_SubTexture m_subDepthIcon{};
    mutable Tex3DS_SubTexture m_subGoldIcon{};
    mutable Tex3DS_SubTexture m_subSkullIcon{};
    mutable Tex3DS_SubTexture m_subBuffIcon{};
    mutable Tex3DS_SubTexture m_subHpBar{};
    mutable Tex3DS_SubTexture m_subExpBar{};

    bool m_menuPressed = false;
    bool m_avatarPressed = false;
};

} // namespace pd::ui
