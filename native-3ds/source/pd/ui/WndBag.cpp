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
    m_subWindowOpen = false;
    m_subItem = nullptr;
    m_selectedSlot = 0;
}

void WndBag::close() {
    isOpen = false;
    m_subWindowOpen = false;
    m_subItem = nullptr;
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
    m_subWindowOpen = true;
    m_subItem = it;
    m_selectedAction = 0;
    m_subActions.clear();

    bool isEquipped = (it == hero->belongings.equippedWeapon || it == hero->belongings.equippedArmor ||
                       it == hero->belongings.equippedRing1 || it == hero->belongings.equippedRing2);

    if (isEquipped) {
        m_subActions.push_back("UNEQUIP");
    } else if (it->isEquippable()) {
        m_subActions.push_back("EQUIP");
    }

    if (it->category() == ItemCategory::Potion) {
        m_subActions.push_back("DRINK");
    } else if (it->category() == ItemCategory::Scroll) {
        m_subActions.push_back("READ");
    }

    if (it->name == "Ration of Food" || it->name == "Pasty" || it->name == "Mystery Meat") {
        m_subActions.push_back("EAT");
    }

    if (it->asWand()) {
        m_subActions.push_back("ZAP");
    }

    if (it->asShortSword()) {
        m_subActions.push_back("REFORGE");
    }

    m_subActions.push_back("THROW");
    m_subActions.push_back("DROP");
    m_subActions.push_back("QUICKSLOT");
    m_subActions.push_back("CLOSE");
}

void WndBag::closeSubWindow() {
    m_subWindowOpen = false;
    m_subItem = nullptr;
    m_subActions.clear();
}

bool WndBag::update(pd::Hero* hero, const TouchController& touch, u32 kDown, std::string& outMessage) {
    if (!isOpen || !hero) return false;

    // -------------------------------------------------------------------------
    // Sub-window (WndItem options popup) is active
    // -------------------------------------------------------------------------
    if (m_subWindowOpen && m_subItem) {
        if (kDown & KEY_B) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            closeSubWindow();
            return false;
        }

        // Sub-window action button layout
        for (size_t i = 0; i < m_subActions.size(); ++i) {
            float bx = SUB_WIN_X + 8.0f + (i % 2) * (SUB_BTN_W + 4.0f);
            float by = SUB_BTN_Y + (i / 2) * (SUB_BTN_H + 4.0f);
            noosa::RectF rBtn(bx, by, SUB_BTN_W, SUB_BTN_H);

            if (touch.clicked(rBtn) || ((kDown & KEY_A) && static_cast<int>(i) == m_selectedAction)) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                const std::string action = m_subActions[i];

                if (action == "CLOSE") {
                    closeSubWindow();
                    return false;
                }

                if (action == "QUICKSLOT") {
                    outMessage = "SET_QUICKSLOT";
                    closeSubWindow();
                    close();
                    return false;
                }

                if (action == "THROW") {
                    outMessage = "START_TARGETING";
                    closeSubWindow();
                    close();
                    return false;
                }

                outMessage = action + " " + m_subItem->displayName();
                m_subItem->execute(hero, action);
                if (m_subItem->quantity <= 0) {
                    hero->belongings.drop(m_subItem);
                }
                closeSubWindow();
                close();
                return true; // Advancing turn
            }
        }

        // D-pad navigation between sub-window actions
        if (kDown & KEY_DLEFT)  m_selectedAction = std::max(0, m_selectedAction - 1);
        if (kDown & KEY_DRIGHT) m_selectedAction = std::min(static_cast<int>(m_subActions.size()) - 1, m_selectedAction + 1);
        if (kDown & KEY_DUP)    m_selectedAction = std::max(0, m_selectedAction - 2);
        if (kDown & KEY_DDOWN)  m_selectedAction = std::min(static_cast<int>(m_subActions.size()) - 1, m_selectedAction + 2);

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

    // 3. Tab along top edge (Backpack selected tab)
    Chrome::get(Chrome::Type::TAB_SELECTED).draw(WIN_X + 10.0f, WIN_Y - 11.0f, 24.0f, 14.0f, zWin + 0.01f);
    if (m_texIcons && m_texIcons->isLoaded()) {
        Tex3DS_SubTexture subTab;
        setupSubTex(subTab, m_texIcons->potWidth(), m_texIcons->potHeight(), 58, 0, 10, 10);
        C2D_Image imgTab = { m_texIcons->getTex(), &subTab };
        C2D_DrawParams pT;
        pT.pos.x = WIN_X + 17.0f; pT.pos.y = WIN_Y - 9.0f; pT.pos.w = 10.0f; pT.pos.h = 10.0f;
        pT.center.x = pT.center.y = 0.0f; pT.depth = zWin + 0.02f; pT.angle = 0.0f;
        C2D_DrawImage(imgTab, &pT, nullptr);
    }

    // 4. Yellow Title Banner "Backpack" using authentic Font2x
    BitmapFont::draw(BitmapFont::FontType::Font2x, "Backpack", WIN_X + 90.0f, WIN_Y + 2.0f, zText, 0xFFFF44FF);

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
                // Purple shaded background for unidentified items
                slotColor = isEquippedSlot ? C2D_Color32(0x70, 0x54, 0x7e, 0xFF) : C2D_Color32(0x5a, 0x3d, 0x6e, 0xFF);
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
                BitmapFont::draw(BitmapFont::FontType::Font1x, gBuf, sx + 2.0f, sy + 18.0f, zText, 0xE4A672FF);
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
                    BitmapFont::draw(BitmapFont::FontType::Font1x, qStr, sx + 2.0f, sy + 1.0f, zText, 0xFFFFFFFF);
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
                        strCol = 0xFF8800FF; // Authentic warning orange for typical/unidentified strength
                    } else if (reqStr > hero->STR) {
                        strCol = 0xFF4444FF; // Red if hero strength is insufficient
                    } else {
                        strCol = 0xCACFC2FF; // Normal light gray
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
                    u32 lCol = (it->level > 0) ? 0x44FF44FF : 0xFF4444FF;
                    BitmapFont::draw(BitmapFont::FontType::Font1x, lvlBadge, lx, sy + 18.0f, zText, lCol);
                } else if (it->cursed && it->cursedKnown) {
                    float cw = BitmapFont::width(BitmapFont::FontType::Font1x, "C");
                    float cx = sx + SLOT_SIZE - cw - 2.0f;
                    BitmapFont::draw(BitmapFont::FontType::Font1x, "C", cx, sy + 18.0f, zText, 0xFF4444FF);
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
        }
    }

    // -------------------------------------------------------------------------
    // 6. Sub-window (WndItem Action Menu) Overlay
    // -------------------------------------------------------------------------
    if (m_subWindowOpen && m_subItem) {
        float subZ = 0.82f;
        float subTextZ = 0.86f;

        // Dim background behind subwindow
        C2D_DrawRectSolid(0.0f, 0.0f, subZ - 0.02f, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0x77));

        // Stone frame for item details
        Chrome::get(Chrome::Type::WINDOW).draw(SUB_WIN_X, SUB_WIN_Y, SUB_WIN_W, SUB_WIN_H, subZ);

        // Item Icon (16x16)
        if (iTex) {
            int imgIdx = m_subItem->image;
            setupSubTex(m_subItemIcon, potW, potH, (imgIdx % 8) * 16, (imgIdx / 8) * 16, 16, 16);
            C2D_Image imgSub = { iTex, &m_subItemIcon };
            C2D_DrawParams pS;
            pS.pos.x = SUB_WIN_X + 12.0f; pS.pos.y = SUB_WIN_Y + 10.0f; pS.pos.w = 16.0f; pS.pos.h = 16.0f;
            pS.center.x = pS.center.y = 0.0f; pS.depth = subTextZ; pS.angle = 0.0f;
            C2D_DrawImage(imgSub, &pS, nullptr);
        }

        // Item Name
        std::string dName = m_subItem->displayName();
        C2D_Text txtName;
        C2D_TextParse(&txtName, textBuf, dName.c_str());
        C2D_TextOptimize(&txtName);
        u32 nameCol = (m_subItem->levelKnown && m_subItem->level > 0) ? 0x44FF44FF :
                      (m_subItem->cursed && m_subItem->cursedKnown ? 0xFF4444FF : 0xFFEBA4FF);
        BitmapFont::draw(BitmapFont::FontType::Font2x, dName, SUB_WIN_X + 34.0f, SUB_WIN_Y + 11.0f, subTextZ, nameCol);

        // Item Info description using multiline BitmapFont::Font1x
        std::string infoStr = m_subItem->info();
        if (infoStr.length() > 95) infoStr = infoStr.substr(0, 92) + "...";
        BitmapFont::drawMultiline(BitmapFont::FontType::Font1x, infoStr, SUB_WIN_X + 12.0f, SUB_WIN_Y + 34.0f, SUB_WIN_W - 24.0f, subTextZ, 0xCACFC2FF);

        // Action Buttons using Chrome::BUTTON (RedButton)
        for (size_t i = 0; i < m_subActions.size(); ++i) {
            float bx = SUB_WIN_X + 8.0f + (i % 2) * (SUB_BTN_W + 4.0f);
            float by = SUB_BTN_Y + (i / 2) * (SUB_BTN_H + 4.0f);

            bool isSelected = (static_cast<int>(i) == m_selectedAction);
            Chrome::get(Chrome::Type::BUTTON).draw(bx, by, SUB_BTN_W, SUB_BTN_H, subZ + 0.02f,
                                                   0xFFFFFFFF, isSelected ? 1.4f : 1.0f);

            u32 btnTxtCol = (i == 0) ? 0xFFFF44FF : 0xFFFFFFFF;
            float tw = BitmapFont::width(BitmapFont::FontType::Font1x, m_subActions[i]);
            float tx = bx + (SUB_BTN_W - tw) * 0.5f;
            BitmapFont::draw(BitmapFont::FontType::Font1x, m_subActions[i], tx, by + 6.0f, subTextZ, btnTxtCol);
        }
    }
}

} // namespace pd::ui

