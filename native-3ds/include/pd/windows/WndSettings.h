#pragma once

#include <citro2d.h>
#include <memory>
#include "pd/ui/TouchController.h"
#include "noosa/Texture.h"

namespace pd::windows {

class WndSettings {
public:
    WndSettings();
    ~WndSettings() = default;

    void show();
    void hide() { m_open = false; }
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    int m_selected = 0; // 0..2 for zoom buttons, 3 for music, 4 for sound, 5 for brightness, 6 for return

    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_w = 190.0f;
    float m_h = 176.0f;

    std::shared_ptr<noosa::Texture> m_texIcons;
    mutable Tex3DS_SubTexture m_subChecked{};
    mutable Tex3DS_SubTexture m_subUnchecked{};
    bool m_subIconsInit = false;

    void initIcons();
};

} // namespace pd::windows

