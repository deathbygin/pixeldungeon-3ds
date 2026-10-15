#pragma once

#include <citro2d.h>
#include <memory>
#include <functional>
#include "pd/ui/TouchController.h"
#include "pd/ui/WndOptions.h"
#include "noosa/Texture.h"

namespace pd::windows {

class WndSettings {
public:
    WndSettings();
    ~WndSettings() = default;

    void show(bool inGame = false);
    void hide() { m_open = false; m_confirmWipe.hide(); }
    bool isOpen() const { return m_open; }

    void setOnDataWiped(std::function<void()> cb) { m_onDataWiped = std::move(cb); }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    bool m_inGame = false;
    int m_selected = 0; // In-game: 0..2 zoom, 3 music, 4 sound, 5 brightness, 6 return. Title: 0 music, 1 sound, 2 brightness, 3 clear data, 4 return.

    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_w = 190.0f;
    float m_h = 168.0f;

    std::shared_ptr<noosa::Texture> m_texIcons;
    mutable Tex3DS_SubTexture m_subChecked{};
    mutable Tex3DS_SubTexture m_subUnchecked{};
    bool m_subIconsInit = false;

    pd::ui::WndOptions m_confirmWipe;
    std::function<void()> m_onDataWiped = nullptr;

    void initIcons();
};

} // namespace pd::windows
