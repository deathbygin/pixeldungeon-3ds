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
    m_activeTab = Tab::BACKPACK;
    m_selectedSlot = 0;
    m_selectionMode = Mode::ALL;
    m_selectionTitle.clear();
    m_selectionListener = nullptr;
}

void WndBag::openForSelection(Mode mode, const std::string& title, std::function<void(pd::Item*)> listener) {
    isOpen = true;
    m_wndItem.hide();
    m_activeTab = Tab::BACKPACK;
    m_selectedSlot = 0;
    m_selectionMode = mode;
    m_selectionTitle = title;
    m_selectionListener = std::move(listener);
}

void WndBag::close() {
    isOpen = false;
    m_wndItem.hide();
    m_activeTab = Tab::BACKPACK;
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
            return it->isUpgradable() && (it->category() == ItemCategory::Armor || (it->category() == ItemCategory::Weapon && !const_cast<Item*>(it)->asWand() && (!const_cast<Item*>(it)->asMissile() || const_cast<Item*>(it)->asBoomerang())));
        case Mode::WEAPON:
            return (const_cast<Item*>(it)->asWeapon() != nullptr && !const_cast<Item*>(it)->asWand() && (!const_cast<Item*>(it)->asMissile() || const_cast<Item*>(it)->asBoomerang()));
        case Mode::WAND:
            return (const_cast<Item*>(it)->asWand() != nullptr && it->isUpgradable());
        case Mode::FOR_SALE:
            return it->price() > 0 && (!Dungeon::hero || !it->isEquipped(Dungeon::hero.get()) || !it->isCursed());
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

    if (m_activeTab == Tab::KEYRING) {
        int keyIdx = m_selectedSlot - 4;
        if (hero->belongings.keyring && keyIdx >= 0 && keyIdx < static_cast<int>(hero->belongings.keyring->keys.size())) {
            return hero->belongings.keyring->keys[keyIdx].get();
        }
        return nullptr;
    }

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
    // L/R Shoulder Button Tab Switching
    // -------------------------------------------------------------------------
    if (kDown & (KEY_L | KEY_R)) {
        m_activeTab = (m_activeTab == Tab::BACKPACK) ? Tab::KEYRING : Tab::BACKPACK;
        m_selectedSlot = (m_activeTab == Tab::BACKPACK) ? 0 : 4;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        return false;
    }

    // -------------------------------------------------------------------------
    // Tab Header Touch Hit Detection & Tap-Outside-to-Dismiss
    // -------------------------------------------------------------------------
    float tabY = WIN_Y - 12.0f;
    noosa::RectF rWin(WIN_X, tabY, WIN_W, WIN_H + (WIN_Y - tabY));
    noosa::RectF rTab0(WIN_X + 12.0f, tabY, 76.0f, 26.0f);
    noosa::RectF rTab1(WIN_X + 92.0f, tabY, 76.0f, 26.0f);

    if (touch.justPressed()) {
        m_longPressTriggered = false;
        float tx = static_cast<float>(touch.x());
        float ty = static_cast<float>(touch.y());

        if (rTab0.contains(tx, ty)) {
            if (m_activeTab != Tab::BACKPACK) {
                m_activeTab = Tab::BACKPACK;
                m_selectedSlot = 0;
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            }
            return false;
        } else if (rTab1.contains(tx, ty)) {
            if (m_activeTab != Tab::KEYRING) {
                m_activeTab = Tab::KEYRING;
                m_selectedSlot = 4;
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            }
            return false;
        }

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

    // 3. Dual raised container tabs: Backpack and Key Ring
    float tabH = 22.0f;
    float tab0X = WIN_X + 12.0f;
    float tab0W = 76.0f;
    float tab1X = WIN_X + 92.0f;
    float tab1W = 76.0f;

    bool backpackActive = (m_activeTab == Tab::BACKPACK);
    bool keyringActive = (m_activeTab == Tab::KEYRING);

    // --- Tab 0: Backpack ---
    float tab0Y = backpackActive ? (WIN_Y - 9.0f) : (WIN_Y - 5.0f);
    float zTab0 = backpackActive ? (zWin + 0.02f) : (zWin + 0.01f);
    float zContent0 = zTab0 + 0.03f;
    Chrome::get(Chrome::Type::TOAST).draw(tab0X, tab0Y, tab0W, tabH, zTab0);

    float bContentW = 10.0f + 5.0f + BitmapFont::width(BitmapFont::FontType::Font1x, "Backpack");
    float bContentX = tab0X + std::floor((tab0W - bContentW) * 0.5f);

    if (m_texIcons && m_texIcons->isLoaded()) {
        float iconX = bContentX;
        float iconY = tab0Y + std::floor((tabH - 10.0f) * 0.5f);

        Tex3DS_SubTexture subTab;
        setupSubTex(subTab, m_texIcons->potWidth(), m_texIcons->potHeight(), 58, 0, 10, 10);
        C2D_Image imgTab = { m_texIcons->getTex(), &subTab };
        C2D_DrawParams pT;
        pT.pos.x = iconX; pT.pos.y = iconY; pT.pos.w = 10.0f; pT.pos.h = 10.0f;
        pT.center.x = 0.0f; pT.center.y = 0.0f; pT.depth = zContent0; pT.angle = 0.0f;
        if (!backpackActive) {
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0x99, 0x99, 0x99, 0xFF), 1.0f);
            C2D_DrawImage(imgTab, &pT, &tint);
        } else {
            C2D_DrawImage(imgTab, &pT, nullptr);
        }
    }
    float bTextX = bContentX + 10.0f + 5.0f;
    float bTextY = tab0Y + std::floor((tabH - 8.0f) * 0.5f);
    u32 bTextCol = backpackActive ? COLOR_TEXT_ACCENT : COLOR_TEXT_MUTED;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Backpack", bTextX, bTextY, zContent0, bTextCol, COLOR_TEXT_SHADOW);

    // --- Tab 1: Key Ring ---
    float tab1Y = keyringActive ? (WIN_Y - 9.0f) : (WIN_Y - 5.0f);
    float zTab1 = keyringActive ? (zWin + 0.02f) : (zWin + 0.01f);
    float zContent1 = zTab1 + 0.03f;
    Chrome::get(Chrome::Type::TOAST).draw(tab1X, tab1Y, tab1W, tabH, zTab1);

    float kContentW = 10.0f + 5.0f + BitmapFont::width(BitmapFont::FontType::Font1x, "Keys");
    float kContentX = tab1X + std::floor((tab1W - kContentW) * 0.5f);

    if (m_texIcons && m_texIcons->isLoaded()) {
        float iconX = kContentX;
        float iconY = tab1Y + std::floor((tabH - 10.0f) * 0.5f);

        Tex3DS_SubTexture subTab;
        setupSubTex(subTab, m_texIcons->potWidth(), m_texIcons->potHeight(), 64, 29, 10, 10);
        C2D_Image imgTab = { m_texIcons->getTex(), &subTab };
        C2D_DrawParams pT;
        pT.pos.x = iconX; pT.pos.y = iconY; pT.pos.w = 10.0f; pT.pos.h = 10.0f;
        pT.center.x = 0.0f; pT.center.y = 0.0f; pT.depth = zContent1; pT.angle = 0.0f;
        if (!keyringActive) {
            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, C2D_Color32(0x99, 0x99, 0x99, 0xFF), 1.0f);
            C2D_DrawImage(imgTab, &pT, &tint);
        } else {
            C2D_DrawImage(imgTab, &pT, nullptr);
        }
    }
    float kTextX = kContentX + 10.0f + 5.0f;
    float kTextY = tab1Y + std::floor((tabH - 8.0f) * 0.5f);
    u32 kTextCol = keyringActive ? COLOR_TEXT_ACCENT : COLOR_TEXT_MUTED;
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Keys", kTextX, kTextY, zContent1, kTextCol, COLOR_TEXT_SHADOW);

    // 4. Selection Title Banner (if in selection mode)
    if (isSelectionMode() && !m_selectionTitle.empty()) {
        float titleW = BitmapFont::width(BitmapFont::FontType::Font1x, m_selectionTitle);
        float titleX = WIN_X + WIN_W - titleW - 12.0f;
        float titleY = WIN_Y + 4.0f;
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, m_selectionTitle, titleX, titleY, zWin + 0.05f, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
    }

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
            bool isGoldSlot = (m_activeTab == Tab::BACKPACK && slotIdx == 19);

            if (slotIdx == 0) it = hero->belongings.equippedWeapon;
            else if (slotIdx == 1) it = hero->belongings.equippedArmor;
            else if (slotIdx == 2) it = hero->belongings.equippedRing1;
            else if (slotIdx == 3) it = hero->belongings.equippedRing2;
            else if (m_activeTab == Tab::BACKPACK) {
                if (slotIdx != 19) {
                    int pIdx = slotIdx - 4;
                    if (pIdx < static_cast<int>(hero->belongings.backpack.size())) {
                        it = hero->belongings.backpack[pIdx].get();
                    }
                }
            } else if (m_activeTab == Tab::KEYRING) {
                int kIdx = slotIdx - 4;
                if (hero->belongings.keyring && kIdx >= 0 && kIdx < static_cast<int>(hero->belongings.keyring->keys.size())) {
                    it = hero->belongings.keyring->keys[kIdx].get();
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
                bool isEligible = isSelectionMode() ? (it && isItemEligible(it)) : true;
                u32 reticleCol = (isSelectionMode() && !isEligible) ? C2D_Color32(0x55, 0x55, 0x55, 0xFF) : C2D_Color32(0xff, 0xff, 0x44, 0xFF);
                C2D_DrawRectSolid(sx - 1.0f, sy - 1.0f, zSlot + 0.02f, SLOT_SIZE + 2.0f, 1.0f, reticleCol);
                C2D_DrawRectSolid(sx - 1.0f, sy + SLOT_SIZE, zSlot + 0.02f, SLOT_SIZE + 2.0f, 1.0f, reticleCol);
                C2D_DrawRectSolid(sx - 1.0f, sy - 1.0f, zSlot + 0.02f, 1.0f, SLOT_SIZE + 2.0f, reticleCol);
                C2D_DrawRectSolid(sx + SLOT_SIZE, sy - 1.0f, zSlot + 0.02f, 1.0f, SLOT_SIZE + 2.0f, reticleCol);
            }

            // Draw Item / Placeholder / Gold
            if (isGoldSlot && iTex) {
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

                // Top-Left Badge: Key depth indicator OR Stack count
                if (Key* k = it->asKey()) {
                    std::string depthBadge = std::to_string(k->depth);
                    BitmapFont::draw(BitmapFont::FontType::Font1x, depthBadge, sx + 2.0f, sy + 1.0f, zText, COLOR_TEXT_PRIMARY);
                    if (k->quantity > 1) {
                        std::string qStr = std::to_string(k->quantity);
                        float qw = BitmapFont::width(BitmapFont::FontType::Font1x, qStr);
                        float qx = sx + SLOT_SIZE - qw - 2.0f;
                        BitmapFont::draw(BitmapFont::FontType::Font1x, qStr, qx, sy + 18.0f, zText, COLOR_TEXT_PRIMARY);
                    }
                } else if (it->quantity > 1) {
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
                    } else if (reqStr > hero->effectiveSTR()) {
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

                // Bottom-Left: Wand charge pips (green for active, dark socket for depleted)
                if (Wand* wand = it->asWand()) {
                    int maxCh = std::max(1, wand->maxCharges);
                    int curCh = std::clamp(wand->curCharges, 0, maxCh);
                    for (int i = 0; i < maxCh; ++i) {
                        int col = i % 4;
                        int row = i / 4;
                        float px = sx + 2.0f + col * 3.0f;
                        float py = sy + 23.0f - row * 3.0f;
                        u32 sqCol = (i < curCh) ? C2D_Color32(0x00, 0xee, 0x00, 0xFF) : C2D_Color32(0x28, 0x2b, 0x26, 0xFF);
                        C2D_DrawRectSolid(px, py, zText, 2.0f, 2.0f, sqCol);
                    }
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
                if (isGoldSlot || !it || !isItemEligible(it)) {
                    C2D_DrawRectSolid(sx, sy, zText + 0.01f, SLOT_SIZE, SLOT_SIZE, C2D_Color32(0x00, 0x00, 0x00, 0xB8));
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

