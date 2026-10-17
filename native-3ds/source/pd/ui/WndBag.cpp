#include "pd/ui/WndBag.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Hero.h"
#include "pd/Item.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/Dungeon.h"
#include "pd/Assets.h"
#include "pd/GLog.h"
#include "pd/FX.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include <cstdio>
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

WndBag::WndBag() {
    m_texItems = noosa::Texture::get(Assets::ITEMS);
    m_texIcons = noosa::Texture::get(Assets::ICONS);
}

void WndBag::open() {
    isOpen = true;
    m_wndItem.hide();
    m_selectedSlot = 0;
    m_selectionMode = Mode::ALL;
    m_selectionTitle.clear();
    m_selectionListener = nullptr;
}

void WndBag::openForSelection(Mode mode, const std::string& title, std::function<void(pd::Item*)> listener) {
    isOpen = true;
    m_wndItem.hide();
    m_selectedSlot = 0;
    m_selectionMode = mode;
    m_selectionTitle = title;
    m_selectionListener = std::move(listener);
}

void WndBag::close() {
    isOpen = false;
    m_wndItem.hide();
    m_selectionMode = Mode::ALL;
    m_selectionTitle.clear();
    m_selectionListener = nullptr;
}

bool WndBag::isItemEligible(const pd::Item* it) const {
    if (!it) return false;
    switch (m_selectionMode) {
        case Mode::QUICKSLOT:
            return !it->defaultAction.empty();
        case Mode::UNIDENTIFIED:
            return !it->isIdentified();
        case Mode::UPGRADEABLE:
            return it->isUpgradable();
        case Mode::ENCHANTABLE:
            return ((it->category() == ItemCategory::Weapon && !const_cast<Item*>(it)->asWand()) || it->category() == ItemCategory::Armor);
        case Mode::ALL:
        default:
            return true;
    }
}

pd::Item* WndBag::getSelectedItem(pd::Hero* hero) const {
    if (!hero) return nullptr;
    if (m_selectedSlot == 0) return hero->belongings.equippedWeapon;
    if (m_selectedSlot == 1) return hero->belongings.equippedArmor;
    if (m_selectedSlot == 2) return hero->belongings.equippedRing1;
    if (m_selectedSlot == 3) return hero->belongings.equippedRing2;

    int packIdx = m_selectedSlot - 4;
    auto& pack = hero->belongings.backpack;
    if (packIdx >= 0 && packIdx < static_cast<int>(pack.size())) {
        return pack[packIdx].get();
    }
    return nullptr;
}

void WndBag::openSubWindow(pd::Item* it, pd::Hero* hero) {
    if (!it) return;
    m_wndItem.show(it, hero, true);
}

void WndBag::closeSubWindow() {
    m_wndItem.hide();
}

bool WndBag::update(pd::Hero* hero, const TouchController& touch, u32 kDown, u32 kHeld, std::string& outMessage) {
    if (!isOpen || !hero) return false;

    // -------------------------------------------------------------------------
    // Sub-window (WndItem options popup) is active
    // -------------------------------------------------------------------------
    if (m_wndItem.isOpen()) {
        std::string action;
        pd::Item* subIt = m_wndItem.getItem();
        std::string subName = subIt ? subIt->displayName() : "";
        bool turnAdvanced = m_wndItem.update(hero, touch, kDown, kHeld, action);

        if (!m_wndItem.isOpen()) {
            if (action == "QUICKSLOT") {
                outMessage = "SET_QUICKSLOT";
                close();
                return false;
            }
            if (action == "THROW" || action == "ZAP") {
                outMessage = "START_TARGETING";
                close();
                return false;
            }
            if (isSelectionMode()) {
                return false;
            }
            if (turnAdvanced) {
                outMessage = action + " " + subName;
                if (subIt && subIt->quantity <= 0) {
                    hero->belongings.drop(subIt);
                }
                close();
                return true;
            }
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Tap-Outside-to-Dismiss (Backdrop Touch on 320x240 screen)
    // -------------------------------------------------------------------------
    float tabY = WIN_Y - 9.0f;
    noosa::RectF rWin(WIN_X, tabY, WIN_W, WIN_H + (WIN_Y - tabY));
    if (touch.justPressed()) {
        m_longPressTriggered = false;
        float tx = static_cast<float>(touch.x());
        float ty = static_cast<float>(touch.y());
        if (!rWin.contains(tx, ty)) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            if (isSelectionMode()) {
                auto listener = std::move(m_selectionListener);
                close();
                if (listener) listener(nullptr);
            } else {
                close();
            }
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // Selection Mode (e.g. Scroll of Identify, Upgrade, Enchantment)
    // -------------------------------------------------------------------------
    if (isSelectionMode()) {
        if (kDown & (KEY_B | KEY_X)) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            auto listener = std::move(m_selectionListener);
            close();
            if (listener) listener(nullptr);
            return false;
        }

        // D-Pad Grid navigation
        if (kDown & KEY_DLEFT)  m_selectedSlot = (m_selectedSlot % COLS > 0) ? m_selectedSlot - 1 : m_selectedSlot;
        if (kDown & KEY_DRIGHT) m_selectedSlot = (m_selectedSlot % COLS < COLS - 1) ? m_selectedSlot + 1 : m_selectedSlot;
        if (kDown & KEY_DUP)    m_selectedSlot = std::max(0, m_selectedSlot - COLS);
        if (kDown & KEY_DDOWN)  m_selectedSlot = std::min(COLS * ROWS - 1, m_selectedSlot + COLS);

        // Slot click / tap detection
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                int slotIdx = r * COLS + c;
                float sx = GRID_START_X + c * (SLOT_SIZE + SLOT_MARGIN);
                float sy = GRID_START_Y + r * (SLOT_SIZE + SLOT_MARGIN);
                noosa::RectF rSlot(sx, sy, SLOT_SIZE, SLOT_SIZE);

                if (touch.clicked(rSlot)) {
                    m_selectedSlot = slotIdx;
                    pd::Item* it = getSelectedItem(hero);
                    if (it && isItemEligible(it)) {
                        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                        auto listener = std::move(m_selectionListener);
                        close();
                        if (listener) listener(it);
                        return false;
                    }
                    return false;
                }
            }
        }

        if (kDown & KEY_A) {
            pd::Item* it = getSelectedItem(hero);
            if (it && isItemEligible(it)) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                auto listener = std::move(m_selectionListener);
                close();
                if (listener) listener(it);
                return false;
            }
            return false;
        }

        return false;
    }

    // -------------------------------------------------------------------------
    // Main Backpack Grid Navigation
    // -------------------------------------------------------------------------
    if (kDown & (KEY_B | KEY_X)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        close();
        return false;
    }

    // D-Pad Grid navigation
    if (kDown & KEY_DLEFT)  m_selectedSlot = (m_selectedSlot % COLS > 0) ? m_selectedSlot - 1 : m_selectedSlot;
    if (kDown & KEY_DRIGHT) m_selectedSlot = (m_selectedSlot % COLS < COLS - 1) ? m_selectedSlot + 1 : m_selectedSlot;
    if (kDown & KEY_DUP)    m_selectedSlot = std::max(0, m_selectedSlot - COLS);
    if (kDown & KEY_DDOWN)  m_selectedSlot = std::min(COLS * ROWS - 1, m_selectedSlot + COLS);

    // Long-press detection to quickly assign item to Quickslot (authentic Pixel Dungeon behavior)
    if (!isSelectionMode() && touch.isDown() && !m_longPressTriggered && touch.heldTime() >= 0.5f) {
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                int slotIdx = r * COLS + c;
                float sx = GRID_START_X + c * (SLOT_SIZE + SLOT_MARGIN);
                float sy = GRID_START_Y + r * (SLOT_SIZE + SLOT_MARGIN);
                noosa::RectF rSlot(sx, sy, SLOT_SIZE, SLOT_SIZE);

                if (touch.hitTest(rSlot)) {
                    m_selectedSlot = slotIdx;
                    pd::Item* it = getSelectedItem(hero);
                    if (it && !it->defaultAction.empty()) {
                        m_longPressTriggered = true;
                        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                        outMessage = "SET_QUICKSLOT";
                        close();
                        return false;
                    }
                    break;
                }
            }
        }
    }

    // Slot click / tap detection
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int slotIdx = r * COLS + c;
            float sx = GRID_START_X + c * (SLOT_SIZE + SLOT_MARGIN);
            float sy = GRID_START_Y + r * (SLOT_SIZE + SLOT_MARGIN);
            noosa::RectF rSlot(sx, sy, SLOT_SIZE, SLOT_SIZE);

            if (touch.clicked(rSlot)) {
                m_selectedSlot = slotIdx;
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                pd::Item* it = getSelectedItem(hero);
                if (it) {
                    openSubWindow(it, hero);
                }
                return false;
            }
        }
    }

    // [A] button opens selected item options
    if (kDown & KEY_A) {
        pd::Item* it = getSelectedItem(hero);
        if (it) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            openSubWindow(it, hero);
        }
        return false;
    }


    return false;
}

void WndBag::draw(const pd::Hero* hero, C2D_TextBuf textBuf) const {
    if (!isOpen || !hero) return;

    float zDim = 0.65f;
    float zWin = 0.70f;
    float zSlot = 0.72f;
    float zItem = 0.74f;
    float zText = 0.78f;

    // 1. Dark semi-transparent backdrop overlay
    C2D_DrawRectSolid(0.0f, 0.0f, zDim, 320.0f, 240.0f, C2D_Color32(0x0a, 0x0c, 0x10, 0xBB));

    // 2. Stone Window Frame using authentic Chrome::WINDOW
    Chrome::get(Chrome::Type::WINDOW).draw(WIN_X, WIN_Y, WIN_W, WIN_H, zWin);

    // 3. Dedicated raised 9-slice container tab for "Backpack" Title & Icon
    std::string titleStr = (isSelectionMode() && !m_selectionTitle.empty()) ? m_selectionTitle : "Backpack";
    float titleW = BitmapFont::width(BitmapFont::FontType::Font1x, titleStr);

    bool showIcon = !isSelectionMode() || (m_selectionTitle.length() <= 14);
    float iconW = showIcon ? 10.0f : 0.0f;
    float iconSpacing = showIcon ? 6.0f : 0.0f;
    float contentW = iconW + iconSpacing + titleW;

    float tabH = 22.0f;
    float tabW = std::clamp(contentW + 24.0f, 90.0f, WIN_W - 16.0f);
    float tabX = WIN_X + std::floor((WIN_W - tabW) * 0.5f);
    float tabY = WIN_Y - 9.0f;
    float zTab = zWin + 0.02f;
    float zContent = zTab + 0.03f;

    // Draw the raised stone container tab
    Chrome::get(Chrome::Type::TOAST).draw(tabX, tabY, tabW, tabH, zTab);

    // Center icon & title inside the tab
    float contentX = tabX + std::floor((tabW - contentW) * 0.5f);

    if (showIcon && m_texIcons && m_texIcons->isLoaded()) {
        float iconX = contentX;
        float iconY = tabY + std::floor((tabH - 10.0f) * 0.5f);

        Tex3DS_SubTexture subTab;
        setupSubTex(subTab, m_texIcons->potWidth(), m_texIcons->potHeight(), 58, 0, 10, 10);
        C2D_Image imgTab = { m_texIcons->getTex(), &subTab };
        C2D_DrawParams pT;
        pT.pos.x = iconX;
        pT.pos.y = iconY;
        pT.pos.w = 10.0f;
        pT.pos.h = 10.0f;
        pT.center.x = 0.0f;
        pT.center.y = 0.0f;
        pT.depth = zContent;
        pT.angle = 0.0f;
        C2D_DrawImage(imgTab, &pT, nullptr);
    }

    float textX = contentX + iconW + iconSpacing;
    float textY = tabY + std::floor((tabH - 8.0f) * 0.5f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, titleStr, textX, textY, zContent, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);

    // 5. 4x5 Recessed Item Grid (Slot size 28x28, margin 2px)
    int potW = m_texItems ? m_texItems->potWidth() : 128;
    int potH = m_texItems ? m_texItems->potHeight() : 256;
    C3D_Tex* iTex = m_texItems ? m_texItems->getTex() : nullptr;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int slotIdx = r * COLS + c;
            float sx = GRID_START_X + c * (SLOT_SIZE + SLOT_MARGIN);
            float sy = GRID_START_Y + r * (SLOT_SIZE + SLOT_MARGIN);

            // Determine item and equipped state
            pd::Item* it = nullptr;
            bool isEquippedSlot = (slotIdx < 4);

            if (slotIdx == 0) it = hero->belongings.equippedWeapon;
            else if (slotIdx == 1) it = hero->belongings.equippedArmor;
            else if (slotIdx == 2) it = hero->belongings.equippedRing1;
            else if (slotIdx == 3) it = hero->belongings.equippedRing2;
            else if (slotIdx == 19) {
                // Gold slot
            } else {
                int pIdx = slotIdx - 4;
                if (pIdx < static_cast<int>(hero->belongings.backpack.size())) {
                    it = hero->belongings.backpack[pIdx].get();
                }
            }

            // Recessed slot background tile
            u32 slotColor = isEquippedSlot ? C2D_Color32(0x63, 0x66, 0x5b, 0xFF) : C2D_Color32(0x4a, 0x4d, 0x44, 0xFF);
            if (it && it->cursed && it->cursedKnown) {
                slotColor = C2D_Color32(0x73, 0x3d, 0x3d, 0xFF); // Reddish cursed tint
            } else if (it && !it->isIdentified()) {
                // Neutral shaded background for unidentified items
                slotColor = isEquippedSlot ? C2D_Color32(0x56, 0x50, 0x48, 0xFF) : C2D_Color32(0x40, 0x3c, 0x38, 0xFF);
            }
            C2D_DrawRectSolid(sx, sy, zSlot, SLOT_SIZE, SLOT_SIZE, slotColor);

            // 1px Border around slot
            C2D_DrawRectSolid(sx, sy, zSlot + 0.01f, SLOT_SIZE, 1.0f, C2D_Color32(0x2a, 0x2b, 0x27, 0xFF));
            C2D_DrawRectSolid(sx, sy + SLOT_SIZE - 1.0f, zSlot + 0.01f, SLOT_SIZE, 1.0f, C2D_Color32(0x2a, 0x2b, 0x27, 0xFF));
            C2D_DrawRectSolid(sx, sy, zSlot + 0.01f, 1.0f, SLOT_SIZE, C2D_Color32(0x2a, 0x2b, 0x27, 0xFF));
            C2D_DrawRectSolid(sx + SLOT_SIZE - 1.0f, sy, zSlot + 0.01f, 1.0f, SLOT_SIZE, C2D_Color32(0x2a, 0x2b, 0x27, 0xFF));

            // Selected Reticle Box
            if (slotIdx == m_selectedSlot) {
                C2D_DrawRectSolid(sx - 1.0f, sy - 1.0f, zSlot + 0.02f, SLOT_SIZE + 2.0f, 1.0f, C2D_Color32(0xff, 0xff, 0x44, 0xFF));
                C2D_DrawRectSolid(sx - 1.0f, sy + SLOT_SIZE, zSlot + 0.02f, SLOT_SIZE + 2.0f, 1.0f, C2D_Color32(0xff, 0xff, 0x44, 0xFF));
                C2D_DrawRectSolid(sx - 1.0f, sy - 1.0f, zSlot + 0.02f, 1.0f, SLOT_SIZE + 2.0f, C2D_Color32(0xff, 0xff, 0x44, 0xFF));
                C2D_DrawRectSolid(sx + SLOT_SIZE, sy - 1.0f, zSlot + 0.02f, 1.0f, SLOT_SIZE + 2.0f, C2D_Color32(0xff, 0xff, 0x44, 0xFF));
            }

            // Draw Item / Placeholder / Gold
            if (slotIdx == 19 && iTex) {
                // Gold Slot
                setupSubTex(m_subItemIcon, potW, potH, (ItemSpriteSheet::GOLD % 8) * 16, (ItemSpriteSheet::GOLD / 8) * 16, 16, 16);
                C2D_Image imgG = { iTex, &m_subItemIcon };
                C2D_DrawParams pG;
                pG.pos.x = sx + 6.0f; pG.pos.y = sy + 6.0f; pG.pos.w = 16.0f; pG.pos.h = 16.0f;
                pG.center.x = pG.center.y = 0.0f; pG.depth = zItem; pG.angle = 0.0f;
                C2D_DrawImage(imgG, &pG, nullptr);

                char gBuf[16];
                std::snprintf(gBuf, sizeof(gBuf), "%d", hero->gold);
                BitmapFont::draw(BitmapFont::FontType::Font1x, gBuf, sx + 2.0f, sy + 18.0f, zText, COLOR_TEXT_PRIMARY);
            } else if (it && iTex) {
                // Render Item Icon (16x16)
                int imgIdx = it->image;
                setupSubTex(m_subItemIcon, potW, potH, (imgIdx % 8) * 16, (imgIdx / 8) * 16, 16, 16);
                C2D_Image imgIt = { iTex, &m_subItemIcon };
                C2D_DrawParams pI;
                pI.pos.x = sx + 6.0f; pI.pos.y = sy + 6.0f; pI.pos.w = 16.0f; pI.pos.h = 16.0f;
                pI.center.x = pI.center.y = 0.0f; pI.depth = zItem; pI.angle = 0.0f;
                C2D_DrawImage(imgIt, &pI, nullptr);

                // Top-Left Badge: Stack count (e.g., 8)
                if (it->quantity > 1) {
                    std::string qStr = std::to_string(it->quantity);
                    BitmapFont::draw(BitmapFont::FontType::Font1x, qStr, sx + 2.0f, sy + 1.0f, zText, COLOR_TEXT_PRIMARY);
                }

                // Top-Right Badge: Strength requirement (:10 or ?10)
                if ((it->category() == ItemCategory::Weapon && !it->asWand()) || it->category() == ItemCategory::Armor) {
                    bool identified = it->levelKnown;
                    int reqStr = 10;
                    if (Weapon* w = it->asWeapon()) {
                        reqStr = identified ? w->STR : w->typicalSTR();
                    } else if (Armor* a = it->asArmor()) {
                        reqStr = identified ? a->STR : a->typicalSTR();
                    }

                    std::string strBadge = identified ? (":" + std::to_string(reqStr)) : ("?" + std::to_string(reqStr));
                    u32 strCol;
                    if (!identified) {
                        strCol = COLOR_TEXT_ACCENT; // Warning orange for typical/unidentified strength
                    } else if (reqStr > hero->STR) {
                        strCol = COLOR_TEXT_WARNING; // Red if hero strength is insufficient
                    } else {
                        strCol = COLOR_TEXT_PRIMARY; // Clean white
                    }
                    float bw = BitmapFont::width(BitmapFont::FontType::Font1x, strBadge);
                    float bx = sx + SLOT_SIZE - bw - 2.0f;
                    BitmapFont::draw(BitmapFont::FontType::Font1x, strBadge, bx, sy + 1.0f, zText, strCol);
                }

                // Bottom-Right Badge: Upgrade Level (+1) or Cursed marker (C)
                if (it->levelKnown && it->level != 0) {
                    std::string lvlBadge = (it->level > 0 ? "+" : "") + std::to_string(it->level);
                    float lw = BitmapFont::width(BitmapFont::FontType::Font1x, lvlBadge);
                    float lx = sx + SLOT_SIZE - lw - 2.0f;
                    u32 lCol = (it->level > 0) ? COLOR_TEXT_POSITIVE : COLOR_TEXT_WARNING;
                    BitmapFont::draw(BitmapFont::FontType::Font1x, lvlBadge, lx, sy + 18.0f, zText, lCol);
                } else if (it->cursed && it->cursedKnown) {
                    float cw = BitmapFont::width(BitmapFont::FontType::Font1x, "C");
                    float cx = sx + SLOT_SIZE - cw - 2.0f;
                    BitmapFont::draw(BitmapFont::FontType::Font1x, "C", cx, sy + 18.0f, zText, COLOR_TEXT_WARNING);
                }

                // Bottom-Left Ticks: Durability
                if (it->isUpgradable() && it->levelKnown) {
                    C2D_DrawRectSolid(sx + 2.0f, sy + 23.0f, zText, 2.0f, 2.0f, C2D_Color32(0x00, 0xee, 0x00, 0xFF));
                    C2D_DrawRectSolid(sx + 5.0f, sy + 23.0f, zText, 2.0f, 2.0f, C2D_Color32(0x00, 0xee, 0x00, 0xFF));
                    C2D_DrawRectSolid(sx + 8.0f, sy + 23.0f, zText, 2.0f, 2.0f, C2D_Color32(0x00, 0xee, 0x00, 0xFF));
                }
            } else if (isEquippedSlot && iTex) {
                // Empty placeholder icon (translucent weapon/armor/ring silhouette)
                int phIdx = (slotIdx == 0) ? ItemSpriteSheet::WEAPON :
                            (slotIdx == 1) ? ItemSpriteSheet::ARMOR : ItemSpriteSheet::RING;
                setupSubTex(m_subItemIcon, potW, potH, (phIdx % 8) * 16, (phIdx / 8) * 16, 16, 16);
                C2D_Image imgPH = { iTex, &m_subItemIcon };
                C2D_DrawParams pPH;
                pPH.pos.x = sx + 6.0f; pPH.pos.y = sy + 6.0f; pPH.pos.w = 16.0f; pPH.pos.h = 16.0f;
                pPH.center.x = pPH.center.y = 0.0f; pPH.depth = zItem; pPH.angle = 0.0f;
                C2D_ImageTint tint;
                C2D_PlainImageTint(&tint, C2D_Color32(0xff, 0xff, 0xff, 0x44), 1.0f);
                C2D_DrawImage(imgPH, &pPH, &tint);
            }

            // In selection mode, dim ineligible or empty slots
            if (isSelectionMode()) {
                if (slotIdx == 19 || !it || !isItemEligible(it)) {
                    C2D_DrawRectSolid(sx, sy, zItem + 0.03f, SLOT_SIZE, SLOT_SIZE, C2D_Color32(0x00, 0x00, 0x00, 0xAA));
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 6. Sub-window (WndItem Action Menu) Overlay
    // -------------------------------------------------------------------------
    if (m_wndItem.isOpen()) {
        m_wndItem.draw(textBuf, 0.82f);
    }
}

} // namespace pd::ui

