#pragma once

#include <citro2d.h>
#include <memory>
#include <vector>
#include <string>
#include "pd/ui/Archs.h"
#include "pd/ui/TouchController.h"
#include "pd/ui/BitmapFont.h"
#include "noosa/Texture.h"

namespace pd::scenes {

enum class AboutAction {
    NONE,
    BACK_TO_TITLE
};

class AboutScene {
public:
    AboutScene();
    ~AboutScene() = default;

    void update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch);
    void drawTop(C2D_TextBuf textBuf);
    void drawBottom(C2D_TextBuf textBuf);

    AboutAction getPendingAction() const { return m_pendingAction; }
    void clearAction() { m_pendingAction = AboutAction::NONE; }
    void refresh();

private:
    AboutAction m_pendingAction = AboutAction::NONE;

    pd::ui::Archs m_archs;

    // Textures & Subtextures for Top Screen
    std::shared_ptr<noosa::Texture> m_texBanners;
    std::shared_ptr<noosa::Texture> m_texIcons;
    Tex3DS_SubTexture m_subBannerLogo;
    Tex3DS_SubTexture m_subWata;
    bool m_texturesInit = false;

    float m_animTimer = 0.0f;

    // Bottom Screen scrolling
    float m_scrollY = 0.0f;
    float m_maxScroll = 0.0f;
    bool m_isDragging = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    // Window layout metrics (Bottom Screen: 320x240)
    float m_winX = 10.0f;
    float m_winY = 24.0f;
    float m_winW = 300.0f;
    float m_winH = 176.0f;

    // Back button
    float m_btnBackX = 90.0f;
    float m_btnBackY = 208.0f;
    float m_btnBackW = 140.0f;
    float m_btnBackH = 24.0f;

    struct LineItem {
        std::string text;
        pd::ui::BitmapFont::FontType font;
        u32 color;
        bool centered;
        bool isDivider = false;
        float height;
    };
    std::vector<LineItem> m_lines;
    float m_totalContentH = 0.0f;

    void initTextures();
    void buildContent();
};

} // namespace pd::scenes

