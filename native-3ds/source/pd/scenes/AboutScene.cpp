#include "pd/scenes/AboutScene.h"
#include "pd/Assets.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Sample.h"
#include <cmath>
#include <algorithm>

namespace pd::scenes {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int x, int y, int w, int h) {
    sub.width = static_cast<u16>(w);
    sub.height = static_cast<u16>(h);
    sub.left = static_cast<float>(x) / potW;
    sub.right = static_cast<float>(x + w) / potW;
    sub.top = 1.0f - static_cast<float>(y) / potH;
    sub.bottom = 1.0f - static_cast<float>(y + h) / potH;
}

AboutScene::AboutScene() {
    initTextures();
    buildContent();
    refresh();
}

void AboutScene::initTextures() {
    m_texBanners = noosa::Texture::get(pd::Assets::BANNERS);
    m_texIcons = noosa::Texture::get(pd::Assets::ICONS);

    if (m_texBanners && m_texBanners->isLoaded()) {
        float potW = static_cast<float>(m_texBanners->potWidth());
        float potH = static_cast<float>(m_texBanners->potHeight());
        setupSubTex(m_subBannerLogo, potW, potH, 0, 0, 128, 70);
    }

    if (m_texIcons && m_texIcons->isLoaded()) {
        float potW = static_cast<float>(m_texIcons->potWidth());
        float potH = static_cast<float>(m_texIcons->potHeight());
        // Watabou avatar in icons.png: x=30, y=16, w=15, h=10
        setupSubTex(m_subWata, potW, potH, 30, 16, 15, 10);
    }

    m_texturesInit = true;
}

void AboutScene::buildContent() {
    m_lines.clear();

    float interiorW = m_winW - 24.0f; // 276.0f

    // 1. Title & Version
    m_lines.push_back({ "Pixel Dungeon", pd::ui::BitmapFont::FontType::Font2x, pd::ui::COLOR_TEXT_ACCENT, true, false, 18.0f });
    m_lines.push_back({ "v1.0.0-alpha (based on v1.9.1)", pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_MUTED, true, false, 12.0f });

    // 2. Divider
    m_lines.push_back({ "", pd::ui::BitmapFont::FontType::Font1x, 0, false, true, 8.0f });

    // 3. Original Attributions
    m_lines.push_back({ "Code & graphics: Watabou", pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_PRIMARY, false, false, 10.0f });
    m_lines.push_back({ "Music: Cube_Code", pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_PRIMARY, false, false, 14.0f });

    // 4. Brogue Inspiration
    std::string brogue = "This game is inspired by Brian Walker's Brogue. Try it on Windows, Mac OS or Linux - it's awesome! ;)";
    auto brogueLines = pd::ui::BitmapFont::splitMultiline(pd::ui::BitmapFont::FontType::Font1x, brogue, interiorW);
    for (size_t i = 0; i < brogueLines.size(); ++i) {
        float h = (i + 1 == brogueLines.size()) ? 14.0f : 10.0f;
        m_lines.push_back({ brogueLines[i], pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_PRIMARY, false, false, h });
    }

    // 5. Official Website
    m_lines.push_back({ "Please visit official website for additional info:", pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_MUTED, false, false, 10.0f });
    m_lines.push_back({ "pixeldungeon.watabou.ru", pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_ACCENT, false, false, 14.0f });

    // 6. Divider
    m_lines.push_back({ "", pd::ui::BitmapFont::FontType::Font1x, 0, false, true, 8.0f });

    // 7. Nintendo 3DS Port Credit
    m_lines.push_back({ "Ported to 3DS by deathbygin", pd::ui::BitmapFont::FontType::Font1x, pd::ui::COLOR_TEXT_ACCENT, true, false, 12.0f });

    m_totalContentH = 0.0f;
    for (const auto& item : m_lines) {
        m_totalContentH += item.height;
    }
}

void AboutScene::refresh() {
    float clientH = m_winH - 20.0f;
    m_maxScroll = std::max(0.0f, m_totalContentH - clientH);
    m_scrollY = 0.0f;
}

void AboutScene::update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch) {
    if (!m_texturesInit) {
        initTextures();
    }

    m_archs.update(dt);
    m_animTimer += dt;

    // Hardware button navigation
    if (kDown & (KEY_B | KEY_START)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_pendingAction = AboutAction::BACK_TO_TITLE;
        return;
    }

    // D-Pad / Circle Pad vertical scrolling
    float scrollSpeed = 120.0f * dt;
    if (kHeld & (KEY_DUP | KEY_CPAD_UP)) {
        m_scrollY = std::max(0.0f, m_scrollY - scrollSpeed);
    }
    if (kHeld & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        m_scrollY = std::min(m_maxScroll, m_scrollY + scrollSpeed);
    }

    // Stylus Touch handling
    noosa::RectF rBack(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH);
    if (touch.clicked(rBack)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_pendingAction = AboutAction::BACK_TO_TITLE;
        return;
    }

    noosa::RectF rWindow(m_winX, m_winY, m_winW, m_winH);
    float tx = static_cast<float>(touch.x());
    float ty = static_cast<float>(touch.y());

    if (touch.justPressed() && rWindow.contains(tx, ty)) {
        m_isDragging = true;
        m_dragStartY = ty;
        m_dragStartScrollY = m_scrollY;
    }

    if (m_isDragging) {
        if (touch.isDown()) {
            float dy = m_dragStartY - ty;
            m_scrollY = std::clamp(m_dragStartScrollY + dy, 0.0f, m_maxScroll);
        } else {
            m_isDragging = false;
        }
    }
}

void AboutScene::drawTop(C2D_TextBuf textBuf) {
    (void)textBuf;
    m_archs.drawTop(0.05f);

    float zBanner = 0.20f;
    float zContent = 0.30f;
    float zText = 0.40f;

    // 1. Pixel Dungeon Banner Logo (128x70) scaled 1.25x (160x87.5)
    if (m_texBanners && m_texBanners->isLoaded()) {
        C2D_Image img;
        img.tex = m_texBanners->getTex();
        img.subtex = &m_subBannerLogo;

        float bScale = 1.25f;
        float bW = 128.0f * bScale;
        float bH = 70.0f * bScale;
        C2D_DrawParams params;
        params.pos.x = (400.0f - bW) * 0.5f;
        params.pos.y = 12.0f;
        params.pos.w = bW;
        params.pos.h = bH;
        params.center.x = 0.0f;
        params.center.y = 0.0f;
        params.depth = zBanner;
        params.angle = 0.0f;
        C2D_DrawImage(img, &params, nullptr);
    }

    // 2. Showcase Window Card for Creator & Port
    float cardW = 340.0f;
    float cardH = 92.0f;
    float cardX = (400.0f - cardW) * 0.5f;
    float cardY = 112.0f;
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(cardX, cardY, cardW, cardH, zContent);

    // Left Slot: Recessed Avatar Frame with Watabou Sprite
    float slotX = cardX + 16.0f;
    float slotY = cardY + 16.0f;
    float slotW = 60.0f;
    float slotH = 60.0f;

    C2D_DrawRectSolid(slotX, slotY, zContent + 0.01f, slotW, slotH, C2D_Color32(0x28, 0x2B, 0x26, 0xFF));
    C2D_DrawRectSolid(slotX, slotY, zContent + 0.02f, slotW, 1.0f, C2D_Color32(0x38, 0x3C, 0x34, 0xFF));
    C2D_DrawRectSolid(slotX, slotY, zContent + 0.02f, 1.0f, slotH, C2D_Color32(0x38, 0x3C, 0x34, 0xFF));

    // Watabou Avatar: 15x10 scaled 3x (45x30)
    if (m_texIcons && m_texIcons->isLoaded()) {
        C2D_Image img;
        img.tex = m_texIcons->getTex();
        img.subtex = &m_subWata;

        float wScale = 3.0f;
        float wW = 15.0f * wScale;
        float wH = 10.0f * wScale;
        float wX = slotX + (slotW - wW) * 0.5f;
        float wY = slotY + (slotH - wH) * 0.5f;

        C2D_DrawParams params;
        params.pos.x = wX;
        params.pos.y = wY;
        params.pos.w = wW;
        params.pos.h = wH;
        params.center.x = 0.0f;
        params.center.y = 0.0f;
        params.depth = zContent + 0.03f;
        params.angle = 0.0f;
        C2D_DrawImage(img, &params, nullptr);

        // Subtle pulsing accent border around slot
        float pulse = 0.6f + 0.4f * std::sin(m_animTimer * 2.5f);
        u8 borderAlpha = static_cast<u8>(pulse * 0xCC);
        C2D_DrawRectSolid(slotX, slotY, zContent + 0.04f, slotW, 1.0f, C2D_Color32(0xFF, 0xAA, 0x22, borderAlpha));
        C2D_DrawRectSolid(slotX, slotY + slotH - 1.0f, zContent + 0.04f, slotW, 1.0f, C2D_Color32(0xFF, 0xAA, 0x22, borderAlpha));
        C2D_DrawRectSolid(slotX, slotY, zContent + 0.04f, 1.0f, slotH, C2D_Color32(0xFF, 0xAA, 0x22, borderAlpha));
        C2D_DrawRectSolid(slotX + slotW - 1.0f, slotY, zContent + 0.04f, 1.0f, slotH, C2D_Color32(0xFF, 0xAA, 0x22, borderAlpha));
    }

    // Right Column: Creator Info & Port Attribution
    float textX = cardX + 88.0f;
    float textY = cardY + 14.0f;

    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font2x, "Pixel Dungeon", textX, textY, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Original Game by Watabou (Oleg Dolya)", textX, textY + 20.0f, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Music by Cube_Code", textX, textY + 33.0f, zText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Ported to 3DS by deathbygin", textX, textY + 48.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    // Footer prompt
    float footW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Press [B] to return");
    float footX = std::round((400.0f - footW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Press [B] to return", footX, 222.0f, zText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
}

void AboutScene::drawBottom(C2D_TextBuf textBuf) {
    (void)textBuf;

    float zBanner = 0.20f;
    float zContent = 0.30f;
    float zText = 0.40f;

    // Header Toast: "About"
    float toastW = 120.0f;
    float toastH = 22.0f;
    float toastX = (320.0f - toastW) * 0.5f;
    float toastY = 4.0f;

    pd::ui::Chrome::get(pd::ui::Chrome::Type::TOAST).draw(toastX, toastY, toastW, toastH, zBanner);
    float tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font2x, "About");
    float tX = std::round((320.0f - tW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font2x, "About", tX, toastY + 2.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    // Stone Frame Window
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_winX, m_winY, m_winW, m_winH, zContent);

    // Usable client bounds
    float clipTop = m_winY + 8.0f;
    float clipBottom = m_winY + m_winH - 8.0f;
    float clipLeft = m_winX + 12.0f;
    float clipRight = m_winX + m_winW - 12.0f;
    float maxWidth = clipRight - clipLeft;

    float curY = clipTop + 4.0f - m_scrollY;

    for (const auto& item : m_lines) {
        if (curY + item.height > clipTop && curY < clipBottom) {
            if (item.isDivider) {
                float divY = std::round(curY + item.height * 0.5f);
                if (divY >= clipTop && divY <= clipBottom) {
                    C2D_DrawRectSolid(clipLeft + 16.0f, divY, zText, maxWidth - 32.0f, 1.0f, C2D_Color32(0x40, 0x44, 0x3C, 0xFF));
                }
            } else if (!item.text.empty()) {
                float x = clipLeft;
                if (item.centered) {
                    float lineW = pd::ui::BitmapFont::width(item.font, item.text);
                    x = std::round(clipLeft + (maxWidth - lineW) * 0.5f);
                }

                // Draw clipped with subtle drop shadow
                pd::ui::BitmapFont::drawClipped(item.font, item.text, x + 1.0f, curY + 1.0f, clipTop, clipBottom, zText - 0.01f, pd::ui::COLOR_TEXT_SHADOW);
                pd::ui::BitmapFont::drawClipped(item.font, item.text, x, curY, clipTop, clipBottom, zText, item.color);
            }
        }
        curY += item.height;
    }

    // Scrollbar Indicator
    if (m_maxScroll > 0.0f) {
        float scrollTrackY = m_winY + 12.0f;
        float scrollTrackH = m_winH - 24.0f;
        float scrollThumbH = std::max(18.0f, scrollTrackH * (scrollTrackH / (scrollTrackH + m_maxScroll)));
        float thumbY = scrollTrackY + (m_scrollY / m_maxScroll) * (scrollTrackH - scrollThumbH);
        float thumbX = m_winX + m_winW - 9.0f;

        C2D_DrawRectSolid(thumbX, scrollTrackY, zContent + 0.01f, 3.0f, scrollTrackH, C2D_Color32(0x22, 0x24, 0x20, 0xFF));
        C2D_DrawRectSolid(thumbX, thumbY, zContent + 0.02f, 3.0f, scrollThumbH, C2D_Color32(0x7A, 0x81, 0x73, 0xFF));
    }

    // Bottom Action Button: "Back (B)"
    float zBtn = 0.45f;
    float zBtnText = 0.48f;
    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);

    btnPatch.draw(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH, zBtn, 0xFFFFFFFF, 1.0f);
    float backW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Back (B)");
    float backX = std::round(m_btnBackX + (m_btnBackW - backW) * 0.5f);
    float backY = std::round(m_btnBackY + (m_btnBackH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Back (B)", backX, backY, zBtnText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
}

} // namespace pd::scenes

