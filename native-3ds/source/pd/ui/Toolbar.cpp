#include "pd/ui/Toolbar.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Hero.h"
#include "pd/Item.h"
#include "pd/Assets.h"
#include "noosa/Audio.h"
#include <algorithm>

namespace pd::ui {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int fx, int fy, int fw, int fh) {
    sub.width = static_cast<u16>(fw);
    sub.height = static_cast<u16>(fh);
    sub.left = static_cast<float>(fx) / static_cast<float>(potW);
    sub.right = static_cast<float>(fx + fw) / static_cast<float>(potW);
    sub.top = 1.0f - (static_cast<float>(fy) / static_cast<float>(potH));
    sub.bottom = 1.0f - (static_cast<float>(fy + fh) / static_cast<float>(potH));
}

Toolbar::Toolbar() {
    m_texToolbar = noosa::Texture::get(Assets::TOOLBAR);
    m_texItems = noosa::Texture::get(Assets::ITEMS);
    m_texIcons = noosa::Texture::get(Assets::ICONS);
}

Toolbar::Action Toolbar::update(const TouchController& touch, u32 kDown, pd::Hero* hero) {
    (void)hero;

    // Hardware button shortcuts
    if (kDown & KEY_R) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::WAIT;
    }
    if (kDown & KEY_L) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::SEARCH;
    }
    if (kDown & KEY_X) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::BAG;
    }
    if (kDown & KEY_Y) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::QUICK1;
    }

    // Touch hitboxes matching the continuous docked clusters with generous vertical touch zone
    // Left cluster: Wait (20px), Search (20px), Info (21px) -> x: 4..65
    noosa::RectF rWait(4.0f, 208.0f, 20.0f, 29.0f);
    noosa::RectF rSearch(24.0f, 208.0f, 20.0f, 29.0f);
    noosa::RectF rInfo(44.0f, 208.0f, 21.0f, 29.0f);

    // Right cluster: Bag (23px), Quickslot 1 (22px), Quickslot 2 (22px) -> x: 249..316
    noosa::RectF rBag(249.0f, 208.0f, 23.0f, 29.0f);
    noosa::RectF rQ1(272.0f, 208.0f, 22.0f, 29.0f);
    noosa::RectF rQ2(294.0f, 208.0f, 22.0f, 29.0f);

    // Track press state for visual feedback
    m_pressedTool = -1;
    if (touch.isDown()) {
        float tx = static_cast<float>(touch.x());
        float ty = static_cast<float>(touch.y());
        if (rWait.contains(tx, ty)) m_pressedTool = 0;
        else if (rSearch.contains(tx, ty)) m_pressedTool = 1;
        else if (rInfo.contains(tx, ty)) m_pressedTool = 2;
        else if (rBag.contains(tx, ty)) m_pressedTool = 3;
        else if (rQ1.contains(tx, ty)) m_pressedTool = 4;
        else if (rQ2.contains(tx, ty)) m_pressedTool = 5;
    }

    if (touch.clicked(rWait)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::WAIT;
    }
    if (touch.clicked(rSearch)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::SEARCH;
    }
    if (touch.clicked(rInfo)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::INFO;
    }
    if (touch.clicked(rBag)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::BAG;
    }
    if (touch.clicked(rQ1)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::QUICK1;
    }
    if (touch.clicked(rQ2)) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        return Action::QUICK2;
    }

    return Action::NONE;
}

void Toolbar::draw(const pd::Hero* hero, C2D_TextBuf textBuf, bool isTargeting, const pd::Item* targetItem, bool isExamining) const {
    (void)hero;
    (void)textBuf;
    if (!m_texToolbar || !m_texToolbar->isLoaded()) return;

    int potW = m_texToolbar->potWidth();
    int potH = m_texToolbar->potHeight();
    C3D_Tex* tex = m_texToolbar->getTex();
    float zBar = 0.50f;
    float zContent = 0.52f;
    float zText = 0.56f;

    // Helper to draw an authentic tool from toolbar.png with pressed state feedback
    auto drawTool = [&](Tex3DS_SubTexture& sub, int sx, int sy, int sw, int sh,
                        float bx, float by, int toolIndex) {
        setupSubTex(sub, potW, potH, sx, sy, sw, sh);
        C2D_Image img = { tex, &sub };
        C2D_DrawParams p;
        p.pos.x = bx;
        p.pos.y = by;
        p.pos.w = static_cast<float>(sw);
        p.pos.h = static_cast<float>(sh);
        p.center.x = p.center.y = 0.0f;
        p.depth = zBar;
        p.angle = 0.0f;

        if (m_pressedTool == toolIndex) {
            p.pos.y += 1.0f; // Tactile 1px downward depression
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xEE, 0xCC, 0xFF), 0.35f);
            C2D_DrawImage(img, &p, &tint);
        } else {
            C2D_DrawImage(img, &p, nullptr);
        }
    };

    // -------------------------------------------------------------
    // LEFT CLUSTER: Continuous Action Strip (Wait, Search, Info)
    // Slices from toolbar.png (y=7, h=25):
    // Wait: (0, 7, 20, 25) at x = 4.0f
    // Search: (20, 7, 20, 25) at x = 24.0f
    // Info: (40, 7, 21, 25) at x = 44.0f
    // Together: 61x25 seamless continuous grey stone bar
    // -------------------------------------------------------------
    drawTool(m_subWait,   0, 7, 20, 25,  4.0f, Y_POS, 0);
    drawTool(m_subSearch, 20, 7, 20, 25, 24.0f, Y_POS, 1);

    if (isExamining) {
        setupSubTex(m_subInfo, potW, potH, 40, 7, 21, 25);
        C2D_Image img = { tex, &m_subInfo };
        C2D_DrawParams p;
        p.pos.x = 44.0f;
        p.pos.y = Y_POS + 1.0f;
        p.pos.w = 21.0f;
        p.pos.h = 25.0f;
        p.center.x = p.center.y = 0.0f;
        p.depth = zBar;
        p.angle = 0.0f;
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xEE, 0x88, 0xFF), 0.40f);
        C2D_DrawImage(img, &p, &tint);
    } else {
        drawTool(m_subInfo,   40, 7, 21, 25, 44.0f, Y_POS, 2);
    }

    // -------------------------------------------------------------
    // RIGHT CLUSTER: Continuous Quickslot Strip (Bag, Q1, Q2)
    // Slices from toolbar.png (y=7, h=25):
    // Bag: (60, 7, 23, 25) at x = 249.0f
    // Quickslot 1: (83, 7, 22, 25) at x = 272.0f (recessed well)
    // Quickslot 2: (83, 7, 22, 25) at x = 294.0f (recessed well)
    // Together: 67x25 seamless continuous grey stone bar
    // -------------------------------------------------------------
    drawTool(m_subBag,        60, 7, 23, 25, 249.0f, Y_POS, 3);
    drawTool(m_subQuickSlot1, 83, 7, 22, 25, 272.0f, Y_POS, 4);
    drawTool(m_subQuickSlot2, 83, 7, 22, 25, 294.0f, Y_POS, 5);

    // Draw Assigned Items inside Quickslots
    if (m_texItems && m_texItems->isLoaded()) {
        int iPotW = m_texItems->potWidth();
        int iPotH = m_texItems->potHeight();
        C3D_Tex* iTex = m_texItems->getTex();

        auto drawSlotItem = [&](pd::Item* item, float slotX, Tex3DS_SubTexture& sub, int toolIdx) {
            if (!item) return;
            int imgIdx = item->image;
            setupSubTex(sub, iPotW, iPotH, (imgIdx % 8) * 16, (imgIdx / 8) * 16, 16, 16);
            C2D_Image img = { iTex, &sub };
            C2D_DrawParams p;
            p.pos.x = slotX + 3.0f;
            p.pos.y = Y_POS + 5.0f;
            if (m_pressedTool == toolIdx) {
                p.pos.y += 1.0f;
            }
            p.pos.w = 16.0f;
            p.pos.h = 16.0f;
            p.center.x = p.center.y = 0.0f;
            p.depth = zContent;
            p.angle = 0.0f;
            C2D_DrawImage(img, &p, nullptr);

            // Stack quantity / wand charge badge in top-left
            float textY = (m_pressedTool == toolIdx) ? (Y_POS + 1.0f) : Y_POS;
            if (Wand* w = item->asWand()) {
                std::string chStr = std::to_string(w->curCharges);
                u32 col = w->curCharges > 0 ? C2D_Color32(0x99, 0xcc, 0xff, 0xFF) : C2D_Color32(0xee, 0x44, 0x44, 0xFF);
                BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, chStr, slotX + 2.0f, textY + 1.0f, zText, col, 0x000000FF);
            } else if (item->quantity > 1) {
                std::string qStr = std::to_string(item->quantity);
                BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, qStr, slotX + 2.0f, textY + 1.0f, zText, 0xFFFFFFFF, 0x000000FF);
            }
        };

        drawSlotItem(m_quickItem1, 272.0f, m_subItem1, 4);
        drawSlotItem(m_quickItem2, 294.0f, m_subItem2, 5);
    }

    // Crosshair target overlay when targeting mode is active
    if (isTargeting && m_texIcons && m_texIcons->isLoaded()) {
        float crossX = 272.0f; // default to quickslot 1
        int crossTool = 4;
        if (targetItem && targetItem == m_quickItem2) {
            crossX = 294.0f;
            crossTool = 5;
        }

        int oPotW = m_texIcons->potWidth();
        int oPotH = m_texIcons->potHeight();
        setupSubTex(m_subTargetCross, oPotW, oPotH, 0, 13, 16, 16);
        C2D_Image imgTarget = { m_texIcons->getTex(), &m_subTargetCross };
        C2D_DrawParams pT;
        pT.pos.x = crossX + 3.0f;
        pT.pos.y = Y_POS + 5.0f + (m_pressedTool == crossTool ? 1.0f : 0.0f);
        pT.pos.w = 16.0f;
        pT.pos.h = 16.0f;
        pT.center.x = pT.center.y = 0.0f;
        pT.depth = zText;
        pT.angle = 0.0f;
        C2D_DrawImage(imgTarget, &pT, nullptr);
    }
}

} // namespace pd::ui

