#include "pd/BadgeBanner.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/ui/BitmapFont.h"
#include <algorithm>
#include <cmath>

namespace pd {

BadgeBanner::State BadgeBanner::s_state = BadgeBanner::State::INACTIVE;
Badges::Badge BadgeBanner::s_currentBadge = Badges::Badge::COUNT;
float BadgeBanner::s_time = 0.0f;
float BadgeBanner::s_sparkleTime = 0.0f;
std::shared_ptr<noosa::Texture> BadgeBanner::s_texBadges = nullptr;
std::shared_ptr<noosa::Texture> BadgeBanner::s_texLocked = nullptr;

void BadgeBanner::initTextures() {
    if (!s_texBadges) {
        s_texBadges = noosa::Texture::get(Assets::BADGES);
    }
    if (!s_texLocked) {
        s_texLocked = noosa::Texture::get(Assets::LOCKED);
    }
}

bool BadgeBanner::isActive() {
    return s_state != State::INACTIVE;
}

void BadgeBanner::show(Badges::Badge badge) {
    if (badge >= Badges::Badge::COUNT) return;
    initTextures();
    s_currentBadge = badge;
    s_state = State::FADE_IN;
    s_time = 0.35f;
    s_sparkleTime = 0.0f;
    Sample::play(Assets::SND_BADGE);
}

void BadgeBanner::update(float dt) {
    if (s_state == State::INACTIVE) return;

    s_time -= dt;
    s_sparkleTime += dt;

    if (s_time <= 0.0f) {
        switch (s_state) {
            case State::FADE_IN:
                s_state = State::STATIC;
                s_time = 1.75f;
                break;
            case State::STATIC:
                s_state = State::FADE_OUT;
                s_time = 0.8f;
                break;
            case State::FADE_OUT:
                s_state = State::INACTIVE;
                s_currentBadge = Badges::Badge::COUNT;
                break;
            default:
                break;
        }
    }
}

void BadgeBanner::drawBadge(int imageIndex, float x, float y, float scale, float alpha, bool locked, float depth) {
    initTextures();

    if (locked) {
        if (s_texLocked && s_texLocked->isLoaded() && s_texLocked->getTex()) {
            Tex3DS_SubTexture sub;
            int potW = s_texLocked->potWidth();
            int potH = s_texLocked->potHeight();
            sub.width = 16;
            sub.height = 16;
            sub.left = 0.0f;
            sub.right = 16.0f / potW;
            sub.top = 1.0f;
            sub.bottom = 1.0f - (16.0f / potH);

            C2D_Image img = { s_texLocked->getTex(), &sub };
            C2D_DrawParams params;
            params.pos.x = x;
            params.pos.y = y;
            params.pos.w = 16.0f * scale;
            params.pos.h = 16.0f * scale;
            params.center.x = 0.0f;
            params.center.y = 0.0f;
            params.depth = depth;
            params.angle = 0.0f;

            C2D_ImageTint tint;
            u32 c = C2D_Color32(0xFF, 0xFF, 0xFF, static_cast<u8>(255 * alpha));
            C2D_PlainImageTint(&tint, c, 1.0f);
            C2D_DrawImage(img, &params, &tint);
            return;
        }
    }

    if (!s_texBadges || !s_texBadges->isLoaded() || !s_texBadges->getTex()) return;
    if (imageIndex < 0 || imageIndex >= 64) return;

    int col = imageIndex % 8;
    int row = imageIndex / 8;
    int potW = s_texBadges->potWidth();
    int potH = s_texBadges->potHeight();

    Tex3DS_SubTexture sub;
    sub.width = 16;
    sub.height = 16;
    sub.left = (col * 16.0f) / potW;
    sub.right = ((col + 1) * 16.0f) / potW;
    sub.top = 1.0f - (row * 16.0f) / potH;
    sub.bottom = 1.0f - ((row + 1) * 16.0f) / potH;

    C2D_Image img = { s_texBadges->getTex(), &sub };
    C2D_DrawParams params;
    params.pos.x = x;
    params.pos.y = y;
    params.pos.w = 16.0f * scale;
    params.pos.h = 16.0f * scale;
    params.center.x = 0.0f;
    params.center.y = 0.0f;
    params.depth = depth;
    params.angle = 0.0f;

    C2D_ImageTint tint;
    u32 color = locked ? C2D_Color32(0x40, 0x40, 0x40, static_cast<u8>(180 * alpha))
                       : C2D_Color32(0xFF, 0xFF, 0xFF, static_cast<u8>(255 * alpha));
    C2D_PlainImageTint(&tint, color, 1.0f);
    C2D_DrawImage(img, &params, &tint);
}

void BadgeBanner::highlight(int imageIndex, float x, float y, float scale, float depth) {
    (void)imageIndex;
    // Render a small bright sparkle/glint effect across the badge
    float sparkX = x + 10.0f * scale;
    float sparkY = y + 3.0f * scale;
    float sparkSize = 4.0f * scale;
    C2D_DrawRectSolid(sparkX, sparkY, depth, sparkSize, sparkSize, C2D_Color32(0xFF, 0xFF, 0xFF, 0xEE));
}

void BadgeBanner::drawTop(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (s_state == State::INACTIVE || s_currentBadge >= Badges::Badge::COUNT) return;

    float alpha = 1.0f;
    float scaleAnim = 1.0f;

    if (s_state == State::FADE_IN) {
        float p = std::clamp(s_time / 0.35f, 0.0f, 1.0f);
        alpha = 1.0f - p;
        scaleAnim = 1.0f + p * 0.4f;
    } else if (s_state == State::FADE_OUT) {
        alpha = std::clamp(s_time / 0.8f, 0.0f, 1.0f);
    }

    const auto& info = Badges::getInfo(s_currentBadge);

    // Banner layout on Top Screen (400x240)
    float bW = 340.0f;
    float bH = 56.0f;
    float bX = (400.0f - bW) * 0.5f;
    float bY = 80.0f;
    float zBg = 0.94f;
    float zContent = 0.95f;
    float zText = 0.96f;

    u8 aBg = static_cast<u8>(230.0f * alpha);
    u8 aBorder = static_cast<u8>(255.0f * alpha);
    u8 aText = static_cast<u8>(255.0f * alpha);

    // Background Plaque
    C2D_DrawRectSolid(bX, bY, zBg, bW, bH, C2D_Color32(0x14, 0x16, 0x12, aBg));
    // Gold borders (top & bottom)
    C2D_DrawRectSolid(bX, bY, zBg + 0.005f, bW, 2.0f, C2D_Color32(0xFF, 0xAA, 0x22, aBorder));
    C2D_DrawRectSolid(bX, bY + bH - 2.0f, zBg + 0.005f, bW, 2.0f, C2D_Color32(0xFF, 0xAA, 0x22, aBorder));
    // Subtle side accents
    C2D_DrawRectSolid(bX, bY, zBg + 0.005f, 2.0f, bH, C2D_Color32(0x7A, 0x55, 0x11, aBorder));
    C2D_DrawRectSolid(bX + bW - 2.0f, bY, zBg + 0.005f, 2.0f, bH, C2D_Color32(0x7A, 0x55, 0x11, aBorder));

    // Animated Badge Icon
    float iconSize = 32.0f * scaleAnim;
    float iconX = bX + 16.0f + (32.0f - iconSize) * 0.5f;
    float iconY = bY + (bH - iconSize) * 0.5f;
    drawBadge(info.image >= 0 ? info.image : 0, iconX, iconY, 2.0f * scaleAnim, alpha, false);

    // Sparkling highlight in STATIC state
    if (s_state == State::STATIC) {
        float sparkPhase = std::fmod(s_sparkleTime * 3.0f, 1.0f);
        if (sparkPhase < 0.4f) {
            float spX = iconX + 6.0f;
            float spY = iconY + 6.0f;
            C2D_DrawCircleSolid(spX, spY, zContent + 0.01f, 3.0f, C2D_Color32(0xFF, 0xFF, 0xFF, static_cast<u8>(220 * alpha)));
        }
    }

    // Texts on Right
    float textX = bX + 60.0f;
    float titleY = bY + 11.0f;
    float descY = bY + 29.0f;

    std::string headerStr = info.meta ? "SUPER BADGE UNLOCKED!" : "BADGE UNLOCKED!";
    u32 headerCol = C2D_Color32(0xFF, 0xAA, 0x22, aText);
    u32 shadowCol = C2D_Color32(0x10, 0x10, 0x10, aText);
    u32 descCol = C2D_Color32(0xEE, 0xEE, 0xEE, aText);

    ui::BitmapFont::drawWithShadow(ui::BitmapFont::FontType::Font1x, headerStr, textX, titleY, zText, headerCol, shadowCol);
    ui::BitmapFont::drawWithShadow(ui::BitmapFont::FontType::Font1x, info.description, textX, descY, zText, descCol, shadowCol);
}

} // namespace pd
