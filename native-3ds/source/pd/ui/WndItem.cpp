#include "pd/ui/WndItem.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Item.h"
#include "pd/Hero.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include "noosa/Texture.h"
#include <algorithm>
#include <cmath>

namespace pd::ui {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int fx, int fy, int fw, int fh) {
    sub.width = static_cast<u16>(fw);
    sub.height = static_cast<u16>(fh);
    sub.left = static_cast<float>(fx) / static_cast<float>(potW);
    sub.right = static_cast<float>(fx + fw) / static_cast<float>(potW);
    sub.top = 1.0f - (static_cast<float>(fy) / static_cast<float>(potH));
    sub.bottom = 1.0f - (static_cast<float>(fy + fh) / static_cast<float>(potH));
}

WndItem::WndItem() {
    m_texItems = noosa::Texture::get(Assets::ITEMS);
}

void WndItem::show(pd::Item* item, pd::Hero* hero, bool allowActions) {
    if (!item) return;
    m_open = true;
    m_item = item;
    m_selectedAction = 0;
    m_scrollY = 0.0f;
    m_isDragging = false;
    m_actions.clear();

    if (allowActions && hero && hero->isAlive()) {
        m_actions = item->actions(hero);
        if (!m_actions.empty()) {
            m_actions.push_back("QUICKSLOT");
        }
        m_actions.push_back("CLOSE");
    } else {
        m_actions.push_back("CLOSE");
    }

    calculateLayout(hero);
}

void WndItem::hide() {
    m_open = false;
    m_item = nullptr;
    m_actions.clear();
    m_descLines.clear();
    m_isDragging = false;
}

void WndItem::calculateLayout(pd::Hero* hero) {
    (void)hero;
    if (!m_item) return;

    // 1. Usable window bounds calculation
    m_winW = 260.0f;
    m_winX = std::round((320.0f - m_winW) * 0.5f);

    // Frame padding and margins: Chrome::WINDOW has 7px margins + 7px inner padding
    const float paddingHor = 14.0f;
    m_usableTextW = m_winW - (paddingHor * 2.0f);
    m_bodyX = m_winX + paddingHor;

    // 2. Subheader information (Tier, Strength requirement, Damage, Armor absorbtion, Charges, Dew Vial Volume)
    m_subHeader.clear();
    if (DewVial* vial = m_item->asDewVial()) {
        m_subHeader = "Volume: " + std::to_string(vial->volume) + "/10";
    } else if (Wand* wand = m_item->asWand()) {
        if (wand->levelKnown) {
            m_subHeader = "Charges: " + (wand->curChargeKnown ? std::to_string(wand->curCharges) : "?") + "/" + std::to_string(wand->maxCharges);
        }
    } else if (Weapon* w = m_item->asWeapon()) {
        bool identified = w->levelKnown;
        int reqStr = identified ? w->STR : w->typicalSTR();
        std::string sub;
        if (w->tier > 0) {
            sub += "Tier " + std::to_string(w->tier) + "  ";
        }
        sub += "STR: " + (identified ? std::to_string(reqStr) : ("?" + std::to_string(reqStr)));
        if (identified) {
            sub += "  Dmg: " + std::to_string(w->min()) + "-" + std::to_string(w->max());
        }
        m_subHeader = sub;
    } else if (Armor* a = m_item->asArmor()) {
        bool identified = a->levelKnown;
        int reqStr = identified ? a->STR : a->typicalSTR();
        m_subHeader = "Tier " + std::to_string(a->tier) + "  STR: " + (identified ? std::to_string(reqStr) : ("?" + std::to_string(reqStr)));
        if (identified) {
            int maxDr = a->tier * 2 + a->level;
            m_subHeader += "  DR: 0-" + std::to_string(maxDr);
        }
    } else if (m_item->quantity > 1) {
        m_subHeader = "Quantity: " + std::to_string(m_item->quantity);
    }

    if (!m_subHeader.empty()) {
        m_headerH = 34.0f;
    } else {
        m_headerH = 24.0f;
    }

    // 3. Multiline description text word wrapping
    m_descLines = BitmapFont::splitMultiline(BitmapFont::FontType::Font1x, m_item->info(), m_usableTextW);
    float lineH = BitmapFont::lineHeight(BitmapFont::FontType::Font1x);
    m_totalTextH = m_descLines.size() * lineH;

    // 4. Footer buttons layout
    int numActions = static_cast<int>(m_actions.size());
    m_btnCols = 2;
    m_btnH = 18.0f;
    float btnGap = 4.0f;
    int numRows = (numActions + m_btnCols - 1) / m_btnCols;
    m_footerH = numRows * m_btnH + (numRows > 1 ? (numRows - 1) * btnGap : 0.0f);
    m_btnW = (m_usableTextW - btnGap) / static_cast<float>(m_btnCols);

    // 5. Dynamic Window Sizing & Vertical Constraints
    const float topInset = 12.0f;
    const float gapHeaderBody = 6.0f;
    const float gapBodyFooter = (numActions > 0) ? 8.0f : 0.0f;
    const float bottomInset = 12.0f;

    float totalRequiredH = topInset + m_headerH + gapHeaderBody + m_totalTextH + gapBodyFooter + m_footerH + bottomInset;
    const float maxScreenH = 224.0f; // Leaves 8px top/bottom margins on 240px screen
    const float minScreenH = 90.0f;

    if (totalRequiredH <= maxScreenH) {
        m_winH = std::max(minScreenH, totalRequiredH);
        m_bodyH = m_totalTextH;
        m_isScrollable = false;
        m_scrollY = 0.0f;
        m_maxScroll = 0.0f;
    } else {
        m_winH = maxScreenH;
        float nonBodyH = topInset + m_headerH + gapHeaderBody + gapBodyFooter + m_footerH + bottomInset;
        m_bodyH = std::max(24.0f, m_winH - nonBodyH);
        m_isScrollable = true;
        m_maxScroll = m_totalTextH - m_bodyH;
        m_scrollY = 0.0f;
    }

    m_winY = std::round((240.0f - m_winH) * 0.5f);
    m_headerY = m_winY + topInset;
    m_bodyY = m_headerY + m_headerH + gapHeaderBody;
    m_footerY = m_bodyY + m_bodyH + gapBodyFooter;
}

bool WndItem::update(pd::Hero* hero, const TouchController& touch, u32 kDown, u32 kHeld, std::string& outAction) {
    if (!m_open || !m_item) return false;

    // 1. Hardware Back / Cancel via B
    if (kDown & KEY_B) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        outAction = "CLOSE";
        return false;
    }

    // 2. Hardware scrolling via Circle Pad
    if (m_isScrollable) {
        circlePosition cpad;
        hidCircleRead(&cpad);
        if (std::abs(cpad.dy) > 35) {
            m_scrollY -= (static_cast<float>(cpad.dy) / 150.0f) * 3.5f;
            m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScroll);
        }

        // L/R shoulder button scrolling
        if (kHeld & KEY_L) {
            m_scrollY = std::max(0.0f, m_scrollY - 3.0f);
        }
        if (kHeld & KEY_R) {
            m_scrollY = std::min(m_maxScroll, m_scrollY + 3.0f);
        }
    }

    // 3. D-Pad button selection and edge scrolling
    if (!m_actions.empty()) {
        int numActions = static_cast<int>(m_actions.size());
        if (kDown & KEY_DLEFT) {
            if (m_selectedAction % m_btnCols > 0) m_selectedAction--;
        }
        if (kDown & KEY_DRIGHT) {
            if (m_selectedAction % m_btnCols < m_btnCols - 1 && m_selectedAction + 1 < numActions) {
                m_selectedAction++;
            }
        }
        if (kDown & KEY_DUP) {
            if (m_selectedAction >= m_btnCols) {
                m_selectedAction -= m_btnCols;
            } else if (m_isScrollable) {
                m_scrollY = std::max(0.0f, m_scrollY - 18.0f);
            }
        }
        if (kDown & KEY_DDOWN) {
            if (m_selectedAction + m_btnCols < numActions) {
                m_selectedAction += m_btnCols;
            } else if (m_isScrollable) {
                m_scrollY = std::min(m_maxScroll, m_scrollY + 18.0f);
            }
        }
    }

    // 4. Touch drag scrolling within the body text area
    noosa::RectF rBody(m_bodyX - 4.0f, m_bodyY, m_usableTextW + 8.0f, m_bodyH);
    if (m_isScrollable) {
        if (touch.justPressed() && rBody.contains(static_cast<float>(touch.x()), static_cast<float>(touch.y()))) {
            m_isDragging = true;
            m_dragStartY = static_cast<float>(touch.y());
            m_dragStartScrollY = m_scrollY;
        }
        if (m_isDragging) {
            if (touch.isDown()) {
                float delta = static_cast<float>(touch.y()) - m_dragStartY;
                m_scrollY = std::clamp(m_dragStartScrollY - delta, 0.0f, m_maxScroll);
            } else {
                m_isDragging = false;
            }
        }
    }

    // 5. Action Buttons Interaction (Touch tap or A button)
    for (size_t i = 0; i < m_actions.size(); ++i) {
        int col = static_cast<int>(i) % m_btnCols;
        int row = static_cast<int>(i) / m_btnCols;
        float bx = m_bodyX + col * (m_btnW + 4.0f);
        float by = m_footerY + row * (m_btnH + 4.0f);
        noosa::RectF rBtn(bx, by, m_btnW, m_btnH);

        if (touch.clicked(rBtn) || ((kDown & KEY_A) && static_cast<int>(i) == m_selectedAction)) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            std::string action = m_actions[i];
            outAction = action;

            if (action == "CLOSE") {
                hide();
                return false;
            }

            if (action == "QUICKSLOT" || action == "THROW" || action == "ZAP" || action == "HEROIC LEAP" || action == "SMOKE BOMB") {
                hide();
                return false;
            }

            pd::Item* targetItem = m_item;
            hide();
            bool used = (targetItem && hero) ? targetItem->execute(hero, action) : false;
            return used;
        }
    }

    // 6. Dismiss on tap outside the stone window
    noosa::RectF rWin(m_winX, m_winY, m_winW, m_winH);
    if (touch.justReleased() && !rWin.contains(static_cast<float>(touch.x()), static_cast<float>(touch.y())) && !m_isDragging) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        outAction = "CLOSE";
        return false;
    }

    return false;
}

void WndItem::draw(C2D_TextBuf textBuf, float baseDepth) const {
    (void)textBuf;
    if (!m_open || !m_item) return;

    float zDim = baseDepth - 0.02f;
    float zWin = baseDepth;
    float zContent = baseDepth + 0.03f;
    float zText = baseDepth + 0.05f;

    // 1. Dim background backdrop
    C2D_DrawRectSolid(0.0f, 0.0f, zDim, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0x88));

    // 2. 9-slice stone window container
    Chrome::get(Chrome::Type::WINDOW).draw(m_winX, m_winY, m_winW, m_winH, zWin);

    // Cursed background highlight indicator
    if (m_item->visiblyCursed()) {
        C2D_DrawRectSolid(m_winX + 4.0f, m_winY + 4.0f, zWin + 0.01f, m_winW - 8.0f, m_winH - 8.0f, C2D_Color32(0x55, 0x00, 0x00, 0x33));
        C2D_DrawRectSolid(m_bodyX - 2.0f, m_headerY - 2.0f, zWin + 0.02f, 20.0f, 20.0f, C2D_Color32(0xCC, 0x00, 0x00, 0x44));
    }

    // 3. Item Sprite Icon (16x16)
    if (m_texItems && m_texItems->isLoaded()) {
        int potW = m_texItems->potWidth();
        int potH = m_texItems->potHeight();
        int imgIdx = m_item->image;
        setupSubTex(m_subItemIcon, potW, potH, (imgIdx % 8) * 16, (imgIdx / 8) * 16, 16, 16);
        C2D_Image img = { m_texItems->getTex(), &m_subItemIcon };
        C2D_DrawParams pI;
        pI.pos.x = m_bodyX;
        pI.pos.y = m_headerY;
        pI.pos.w = 16.0f;
        pI.pos.h = 16.0f;
        pI.center.x = pI.center.y = 0.0f;
        pI.depth = zContent;
        pI.angle = 0.0f;
        C2D_DrawImage(img, &pI, nullptr);
    }

    // 4. Header: Item Display Name and Subheader
    std::string dName = m_item->displayName();
    u32 nameCol = COLOR_TEXT_ACCENT;

    float nameX = m_bodyX + 22.0f;
    float titleMaxW = m_usableTextW - 24.0f;
    float titleW = BitmapFont::width(BitmapFont::FontType::Font2x, dName);

    if (titleW <= titleMaxW) {
        BitmapFont::draw(BitmapFont::FontType::Font2x, dName, nameX, m_headerY - 1.0f, zText, nameCol);
    } else {
        BitmapFont::draw(BitmapFont::FontType::Font1x, dName, nameX, m_headerY + 2.0f, zText, nameCol);
    }

    if (!m_subHeader.empty()) {
        if (DewVial* vial = m_item->asDewVial()) {
            std::string label = "Volume: ";
            float lw = BitmapFont::width(BitmapFont::FontType::Font1x, label);
            BitmapFont::draw(BitmapFont::FontType::Font1x, label, nameX, m_headerY + 16.0f, zText, COLOR_TEXT_ACCENT);
            std::string volStr = std::to_string(vial->volume) + "/10";
            BitmapFont::draw(BitmapFont::FontType::Font1x, volStr, nameX + lw, m_headerY + 16.0f, zText, COLOR_TEXT_PRIMARY);
        } else {
            BitmapFont::draw(BitmapFont::FontType::Font1x, m_subHeader, nameX, m_headerY + 16.0f, zText, COLOR_TEXT_MUTED);
        }
    }

    // 5. Divider line between header and description body
    C2D_DrawRectSolid(m_bodyX, m_bodyY - 4.0f, zContent, m_usableTextW, 1.0f, C2D_Color32(0x3a, 0x3b, 0x36, 0xAA));

    // 6. Body: Multiline word-wrapped description text with vertical glyph clipping
    float clipTop = m_bodyY;
    float clipBottom = m_bodyY + m_bodyH;
    float lineH = BitmapFont::lineHeight(BitmapFont::FontType::Font1x);
    float startY = m_bodyY - m_scrollY;

    for (size_t i = 0; i < m_descLines.size(); ++i) {
        float lineY = startY + i * lineH;
        if (!m_descLines[i].empty()) {
            BitmapFont::drawClipped(BitmapFont::FontType::Font1x, m_descLines[i], m_bodyX, lineY, clipTop, clipBottom, zText, COLOR_TEXT_PRIMARY);
        }
    }

    // 7. Scroll indicator thumb when content is scrollable
    if (m_isScrollable && m_maxScroll > 0.0f) {
        float sbX = m_winX + m_winW - 12.0f;
        float sbY = m_bodyY;
        float sbH = m_bodyH;

        // Scroll track
        C2D_DrawRectSolid(sbX, sbY, zContent, 3.0f, sbH, C2D_Color32(0x22, 0x24, 0x20, 0x88));

        // Scroll thumb
        float thumbH = std::max(12.0f, sbH * (sbH / m_totalTextH));
        float thumbY = sbY + (sbH - thumbH) * (m_scrollY / m_maxScroll);
        C2D_DrawRectSolid(sbX, thumbY, zText, 3.0f, thumbH, C2D_Color32(0xaa, 0x88, 0x44, 0xEE));
    }

    // 8. Footer: Contextual action buttons
    for (size_t i = 0; i < m_actions.size(); ++i) {
        int col = static_cast<int>(i) % m_btnCols;
        int row = static_cast<int>(i) / m_btnCols;
        float bx = m_bodyX + col * (m_btnW + 4.0f);
        float by = m_footerY + row * (m_btnH + 4.0f);

        bool isSelected = (static_cast<int>(i) == m_selectedAction);
        Chrome::get(Chrome::Type::BUTTON).draw(bx, by, m_btnW, m_btnH, zContent,
                                               0xFFFFFFFF, isSelected ? 1.4f : 1.0f);

        bool isDefault = (!m_item->defaultAction.empty() && m_actions[i] == m_item->defaultAction) ||
                         (m_item->defaultAction.empty() && i == 0);
        u32 btnCol = isDefault ? COLOR_TEXT_ACCENT : COLOR_TEXT_PRIMARY;
        float tw = BitmapFont::width(BitmapFont::FontType::Font1x, m_actions[i]);
        float tx = bx + (m_btnW - tw) * 0.5f;
        BitmapFont::draw(BitmapFont::FontType::Font1x, m_actions[i], tx, by + 5.0f, zText, btnCol);
    }
}

} // namespace pd::ui

