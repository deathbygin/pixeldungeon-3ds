#include "pd/ui/InventoryView.h"
#include "pd/Hero.h"
#include "pd/Item.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/Dungeon.h"
#include "pd/Assets.h"
#include "pd/GLog.h"
#include "pd/FX.h"
#include "noosa/Texture.h"
#include "noosa/TextureFilm.h"
#include "noosa/Image.h"
#include <cstdio>
#include <algorithm>

namespace pd::ui {

InventoryView::InventoryView()
    : m_btnClose(164.0f, 184.0f, 144.0f, 22.0f, "CLOSE", "[B]", KEY_B) {
    m_btnClose.z = 0.82f;
    m_btnClose.customFill = C2D_Color32(0x44, 0x49, 0x46, 0xFF);
    m_btnClose.customBorder = C2D_Color32(0x60, 0x66, 0x63, 0xFF);
}

void InventoryView::open() {
    isOpen = true;
    mode = Mode::NORMAL;
    m_selectedIndex = 0;
    m_targetScrollIdx = -1;
}

void InventoryView::openUpgrade(int scrollIdx) {
    isOpen = true;
    mode = Mode::SELECT_UPGRADE;
    m_selectedIndex = 0;
    m_targetScrollIdx = scrollIdx;
}

void InventoryView::openIdentify(int scrollIdx) {
    isOpen = true;
    mode = Mode::SELECT_IDENTIFY;
    m_selectedIndex = 0;
    m_targetScrollIdx = scrollIdx;
}

void InventoryView::close() {
    isOpen = false;
    mode = Mode::NORMAL;
    m_targetScrollIdx = -1;
}

bool InventoryView::update(pd::Hero* hero, const TouchController& touch, u32 kDown, std::string& outMessage) {
    if (!isOpen || !hero) return false;

    // Close / cancel on B button
    if (kDown & KEY_B) {
        close();
        return false;
    }

    auto& pack = hero->belongings.backpack;
    int packSize = static_cast<int>(pack.size());

    // -------------------------------------------------------------------------
    // SELECT_UPGRADE / SELECT_IDENTIFY MODE: Prompt to pick an item
    // -------------------------------------------------------------------------
    if (mode == Mode::SELECT_UPGRADE || mode == Mode::SELECT_IDENTIFY) {
        // Grid navigation via D-Pad
        if (kDown & KEY_DLEFT)  m_selectedIndex = std::max(0, m_selectedIndex - 1);
        if (kDown & KEY_DRIGHT) m_selectedIndex = std::min(std::max(0, packSize - 1), m_selectedIndex + 1);
        if (kDown & KEY_DUP)    m_selectedIndex = std::max(0, m_selectedIndex - GRID_COLS);
        if (kDown & KEY_DDOWN)  m_selectedIndex = std::min(std::max(0, packSize - 1), m_selectedIndex + GRID_COLS);

        // Check if an item slot was tapped or selected via [A]
        int pickedSlot = -1;
        for (size_t i = 0; i < Belongings::BACKPACK_SIZE; ++i) {
            int col = static_cast<int>(i % GRID_COLS);
            int row = static_cast<int>(i / GRID_COLS);
            float sx = GRID_START_X + col * (SLOT_SIZE + 3.0f);
            float sy = GRID_START_Y + row * (SLOT_SIZE + 3.0f);

            if (touch.clicked(noosa::RectF(sx, sy, SLOT_SIZE, SLOT_SIZE))) {
                if (static_cast<int>(i) < packSize) {
                    pickedSlot = static_cast<int>(i);
                }
            }
        }
        if (kDown & KEY_A) {
            pickedSlot = m_selectedIndex;
        }

        if (pickedSlot >= 0 && pickedSlot < packSize && pack[pickedSlot]) {
            Item* targetItem = pack[pickedSlot].get();
            if (mode == Mode::SELECT_UPGRADE) {
                if (targetItem->isUpgradable()) {
                    if (targetItem->cursed) {
                        targetItem->cursed = false;
                        targetItem->cursedKnown = true;
                    }
                    if (targetItem->isBroken()) {
                        targetItem->fix();
                    } else {
                        if (auto* w = targetItem->asWeapon()) {
                            w->upgrade(false);
                        } else if (auto* a = targetItem->asArmor()) {
                            a->upgrade(false);
                        } else {
                            targetItem->upgrade();
                        }
                    }
                    FX::burst(hero->pos, 0xffd700FF, 12);
                    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                    outMessage = "your " + targetItem->displayName() + " certainly looks better now";

                    // Consume the upgrade scroll and identify it
                    if (m_targetScrollIdx >= 0 && m_targetScrollIdx < packSize && pack[m_targetScrollIdx]) {
                        Scroll::setKnown(Scroll::ScrollType::UPGRADE);
                        pack[m_targetScrollIdx]->quantity--;
                        if (pack[m_targetScrollIdx]->quantity <= 0) {
                            hero->belongings.drop(pack[m_targetScrollIdx].get());
                        }
                    }

                    hero->spendAndNext(Actor::TICK);
                    close();
                    return true;
                } else {
                    outMessage = "This item cannot be upgraded!";
                }
            } else if (mode == Mode::SELECT_IDENTIFY) {
                if (!targetItem->isIdentified()) {
                    targetItem->identify();
                    FX::burst(hero->pos, 0x00ffffFFu, 12);
                    FX::floatingText(hero->pos, "IDENTIFIED", 0x00ffffFFu);
                    outMessage = "It is " + targetItem->displayName() + "!";
                    GLog::i("It is " + targetItem->displayName());

                    // Consume the identify scroll and identify it
                    if (m_targetScrollIdx >= 0 && m_targetScrollIdx < packSize && pack[m_targetScrollIdx]) {
                        Scroll::setKnown(Scroll::ScrollType::IDENTIFY);
                        pack[m_targetScrollIdx]->quantity--;
                        if (pack[m_targetScrollIdx]->quantity <= 0) {
                            hero->belongings.drop(pack[m_targetScrollIdx].get());
                        }
                    }

                    hero->spendAndNext(Actor::TICK);
                    close();
                    return true;
                } else {
                    outMessage = "This item is already identified!";
                }
            }
        }

        m_btnClose.x = 164.0f;
        m_btnClose.y = 184.0f;
        m_btnClose.width = 144.0f;
        m_btnClose.customFill = C2D_Color32(0x44, 0x49, 0x46, 0xFF);
        m_btnClose.customBorder = C2D_Color32(0x60, 0x66, 0x63, 0xFF);
        if (m_btnClose.update(touch, kDown)) {
            close();
            return false;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // NORMAL MODE: Browse inventory and execute context-specific actions
    // -------------------------------------------------------------------------
    for (size_t i = 0; i < Belongings::BACKPACK_SIZE; ++i) {
        int col = static_cast<int>(i % GRID_COLS);
        int row = static_cast<int>(i / GRID_COLS);
        float sx = GRID_START_X + col * (SLOT_SIZE + 3.0f);
        float sy = GRID_START_Y + row * (SLOT_SIZE + 3.0f);

        if (touch.clicked(noosa::RectF(sx, sy, SLOT_SIZE, SLOT_SIZE))) {
            if (static_cast<int>(i) < packSize) {
                m_selectedIndex = static_cast<int>(i);
            }
        }
    }

    // D-Pad navigation within grid
    if (kDown & KEY_DLEFT)  m_selectedIndex = std::max(0, m_selectedIndex - 1);
    if (kDown & KEY_DRIGHT) m_selectedIndex = std::min(std::max(0, packSize - 1), m_selectedIndex + 1);
    if (kDown & KEY_DUP)    m_selectedIndex = std::max(0, m_selectedIndex - GRID_COLS);
    if (kDown & KEY_DDOWN)  m_selectedIndex = std::min(std::max(0, packSize - 1), m_selectedIndex + GRID_COLS);

    // Selected item actions update
    if (m_selectedIndex >= 0 && m_selectedIndex < packSize && pack[m_selectedIndex]) {
        Item* it = pack[m_selectedIndex].get();
        auto actions = it->actions(hero);

        // Rebuild action buttons dynamically matching item->actions
        m_actionButtons.clear();
        for (size_t i = 0; i < actions.size(); ++i) {
            const std::string& act = actions[i];
            std::string shortcut;
            u32 hwKey = 0;

            if (i == 0) {
                shortcut = "[A]";
                hwKey = KEY_A;
            } else if (act == "THROW") {
                shortcut = "[Y]";
                hwKey = KEY_Y;
            } else if (act == "DROP") {
                shortcut = "[X]";
                hwKey = KEY_X;
            }

            float btnY = 104.0f + static_cast<float>(i) * 26.0f;
            UIButton btn(164.0f, btnY, 144.0f, 22.0f, act, shortcut, hwKey);
            btn.z = 0.82f;
            if (i == 0) {
                btn.active = true; // Highlight primary default action
                btn.customFill = C2D_Color32(0x56, 0x5e, 0x5a, 0xFF);
                btn.customBorder = C2D_Color32(0xff, 0xff, 0x88, 0xFF);
            } else {
                btn.customFill = C2D_Color32(0x44, 0x49, 0x46, 0xFF);
                btn.customBorder = C2D_Color32(0x60, 0x66, 0x63, 0xFF);
            }
            m_actionButtons.push_back(btn);
        }

        m_btnClose.x = 164.0f;
        m_btnClose.width = 144.0f;
        m_btnClose.y = 104.0f + static_cast<float>(actions.size()) * 26.0f + 2.0f;
        m_btnClose.z = 0.82f;
        m_btnClose.customFill = C2D_Color32(0x44, 0x49, 0x46, 0xFF);
        m_btnClose.customBorder = C2D_Color32(0x60, 0x66, 0x63, 0xFF);

        // Check button activations
        for (size_t i = 0; i < m_actionButtons.size(); ++i) {
            if (m_actionButtons[i].update(touch, kDown)) {
                const std::string& chosenAction = actions[i];

                // Curse lock check for equipped items
                bool isEquipped = (it == hero->belongings.equippedWeapon ||
                                   it == hero->belongings.equippedArmor ||
                                   it == hero->belongings.equippedRing1 ||
                                   it == hero->belongings.equippedRing2);
                if (isEquipped && it->cursed && (chosenAction == "UNEQUIP" || chosenAction == "DROP" || chosenAction == "THROW")) {
                    outMessage = "You can't remove a cursed item!";
                    GLog::w("You can't remove cursed " + it->displayName() + "!");
                    return false;
                }

                if (chosenAction == "EQUIP") {
                    if (it->category() == ItemCategory::Weapon && hero->belongings.equippedWeapon && hero->belongings.equippedWeapon->isCursed()) {
                        outMessage = "You can't remove your cursed weapon!";
                        GLog::w("You can't remove cursed " + hero->belongings.equippedWeapon->displayName() + "!");
                        return false;
                    }
                    if (it->category() == ItemCategory::Armor && hero->belongings.equippedArmor && hero->belongings.equippedArmor->isCursed()) {
                        outMessage = "You can't remove your cursed armor!";
                        GLog::w("You can't remove cursed " + hero->belongings.equippedArmor->displayName() + "!");
                        return false;
                    }
                    if (it->category() == ItemCategory::Ring && hero->belongings.equippedRing1 && hero->belongings.equippedRing1->isCursed() && hero->belongings.equippedRing2 && hero->belongings.equippedRing2->isCursed()) {
                        outMessage = "You can't remove your cursed rings!";
                        GLog::w("You can't remove your cursed rings!");
                        return false;
                    }
                }

                // Prompt specifically for Scroll of Upgrade or Scroll of Identify
                if (it->category() == ItemCategory::Scroll && chosenAction == "READ") {
                    auto* scr = static_cast<Scroll*>(it);
                    if (scr->type == Scroll::ScrollType::UPGRADE) {
                        mode = Mode::SELECT_UPGRADE;
                        m_targetScrollIdx = m_selectedIndex;
                        outMessage = "Select an item to upgrade";
                        return false;
                    } else if (scr->type == Scroll::ScrollType::IDENTIFY) {
                        mode = Mode::SELECT_IDENTIFY;
                        m_targetScrollIdx = m_selectedIndex;
                        outMessage = "Select an item to identify";
                        return false;
                    }
                }

                if (chosenAction == "THROW" || chosenAction == "ZAP" || chosenAction == "HEROIC LEAP" || chosenAction == "SMOKE BOMB") {
                    outMessage = "START_TARGETING";
                    close();
                    return false;
                }

                outMessage = chosenAction + " " + it->displayName();
                bool used = it->execute(hero, chosenAction);
                if (used) {
                    if (it->quantity <= 0) {
                        hero->belongings.drop(it);
                    }
                    close();
                    return true;
                } else {
                    close();
                    return false;
                }
            }
        }
    } else {
        m_actionButtons.clear();
        m_btnClose.x = 164.0f;
        m_btnClose.width = 144.0f;
        m_btnClose.y = 130.0f;
        m_btnClose.z = 0.82f;
        m_btnClose.customFill = C2D_Color32(0x44, 0x49, 0x46, 0xFF);
        m_btnClose.customBorder = C2D_Color32(0x60, 0x66, 0x63, 0xFF);
    }

    if (m_btnClose.update(touch, kDown)) {
        close();
        return false;
    }

    return false;
}

void InventoryView::draw(const pd::Hero* hero, C2D_TextBuf textBuf) const {
    if (!isOpen || !hero) return;

    // Dim background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, 0.7f, 320.0f, 240.0f, C2D_Color32(0x10, 0x14, 0x12, 0xD8));

    // Outer window frame (classic stone gray)
    float winX = 4.0f;
    float winY = 4.0f;
    float winW = 312.0f;
    float winH = 232.0f;

    // Window background fill
    C2D_DrawRectSolid(winX, winY, 0.72f, winW, winH, C2D_Color32(0x44, 0x49, 0x46, 0xFF));

    // Outer border 1px (medium light stone gray)
    C2D_DrawRectSolid(winX, winY, 0.74f, winW, 1.0f, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(winX, winY + winH - 1.0f, 0.74f, winW, 1.0f, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(winX, winY, 0.74f, 1.0f, winH, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));
    C2D_DrawRectSolid(winX + winW - 1.0f, winY, 0.74f, 1.0f, winH, C2D_Color32(0x6e, 0x75, 0x71, 0xFF));

    // Inner border inset 1px (darker stone shadow)
    C2D_DrawRectSolid(winX + 1.0f, winY + 1.0f, 0.74f, winW - 2.0f, 1.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(winX + 1.0f, winY + winH - 2.0f, 0.74f, winW - 2.0f, 1.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(winX + 1.0f, winY + 1.0f, 0.74f, 1.0f, winH - 2.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));
    C2D_DrawRectSolid(winX + winW - 2.0f, winY + 1.0f, 0.74f, 1.0f, winH - 2.0f, C2D_Color32(0x33, 0x38, 0x35, 0xFF));

    // Header bar background
    C2D_DrawRectSolid(winX + 2.0f, winY + 2.0f, 0.75f, winW - 4.0f, 21.0f, C2D_Color32(0x38, 0x3d, 0x3a, 0xFF));
    // Divider line below header
    C2D_DrawRectSolid(winX + 2.0f, winY + 23.0f, 0.76f, winW - 4.0f, 1.0f, C2D_Color32(0x2d, 0x31, 0x2f, 0xFF));

    static std::shared_ptr<noosa::Texture> s_itemsTex;
    static std::shared_ptr<noosa::TextureFilm> s_itemsFilm;
    static bool s_invInit = false;
    if (!s_invInit) {
        s_itemsTex = noosa::Texture::get(Assets::ITEMS);
        if (s_itemsTex && s_itemsTex->isLoaded()) {
            s_itemsFilm = std::make_shared<noosa::TextureFilm>(s_itemsTex, 16, 16);
        }
        s_invInit = true;
    }

    C2D_Text textObj;
    if (mode == Mode::SELECT_UPGRADE) {
        C2D_TextParse(&textObj, textBuf, "Select an item to upgrade");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, winX + 8.0f, winY + 5.0f, 0.8f, 0.44f, 0.44f, C2D_Color32(0xff, 0xff, 0x66, 0xFF));
    } else if (mode == Mode::SELECT_IDENTIFY) {
        C2D_TextParse(&textObj, textBuf, "Select an item to identify");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, winX + 8.0f, winY + 5.0f, 0.8f, 0.44f, 0.44f, C2D_Color32(0x66, 0xff, 0xff, 0xFF));
    } else {
        C2D_TextParse(&textObj, textBuf, "Backpack");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, winX + 8.0f, winY + 5.0f, 0.8f, 0.46f, 0.46f, C2D_Color32(0xff, 0xf0, 0x33, 0xFF));
    }

    // Gold on top-right of header (e.g. "38 [Gold Icon]")
    int currentGold = hero->gold > 0 ? hero->gold : Dungeon::gold;
    std::string goldStr = std::to_string(currentGold);
    C2D_TextParse(&textObj, textBuf, goldStr.c_str());
    C2D_TextOptimize(&textObj);
    float goldTextW = textObj.width * 0.44f;
    float goldIconX = winX + winW - 24.0f;
    float goldTextX = goldIconX - goldTextW - 4.0f;
    C2D_DrawText(&textObj, C2D_WithColor, goldTextX, winY + 5.0f, 0.8f, 0.44f, 0.44f, C2D_Color32(0xff, 0xf0, 0x33, 0xFF));

    // Draw gold coin sprite next to gold count
    if (s_itemsFilm) {
        noosa::Image goldIcon(s_itemsFilm, ItemSpriteSheet::GOLD);
        goldIcon.targetScreen = noosa::ScreenTarget::Bottom;
        goldIcon.x = goldIconX;
        goldIcon.y = winY + 4.0f;
        goldIcon.z = 0.82f;
        goldIcon.draw(nullptr);
    }

    const auto& pack = hero->belongings.backpack;
    int packSize = static_cast<int>(pack.size());

    // 4x5 Backpack Grid
    for (size_t i = 0; i < Belongings::BACKPACK_SIZE + 1; ++i) {
        int col = static_cast<int>(i % GRID_COLS);
        int row = static_cast<int>(i / GRID_COLS);
        float sx = GRID_START_X + col * (SLOT_SIZE + 3.0f);
        float sy = GRID_START_Y + row * (SLOT_SIZE + 3.0f);

        bool isSelected = (static_cast<int>(i) == m_selectedIndex);
        bool hasItem = (static_cast<int>(i) < packSize && pack[i]);
        Item* item = hasItem ? pack[i].get() : nullptr;

        bool isEquipped = hasItem && (item == hero->belongings.equippedWeapon || item == hero->belongings.equippedArmor);
        bool isUnidentified = hasItem && !item->isIdentified();
        bool isUpgradableTarget = (mode == Mode::SELECT_UPGRADE) && hasItem && item->isUpgradable();
        bool isUnidentifiedTarget = (mode == Mode::SELECT_IDENTIFY) && hasItem && !item->isIdentified();

        // Slot Background Color:
        // Unidentified items have a rich purple shaded background!
        u32 slotBg;
        if (isUnidentified) {
            slotBg = isSelected ? C2D_Color32(0x73, 0x4f, 0x8e, 0xFF) : C2D_Color32(0x5e, 0x3e, 0x74, 0xFF);
        } else if (isEquipped) {
            slotBg = isSelected ? C2D_Color32(0x6e, 0x73, 0x67, 0xFF) : C2D_Color32(0x56, 0x5b, 0x54, 0xFF);
        } else {
            slotBg = isSelected ? C2D_Color32(0x56, 0x5c, 0x59, 0xFF) : C2D_Color32(0x42, 0x47, 0x44, 0xFF);
        }

        // Slot Border:
        u32 borderCol;
        if (isUpgradableTarget) {
            borderCol = C2D_Color32(0xff, 0xd7, 0x00, 0xFF);
        } else if (isUnidentifiedTarget) {
            borderCol = C2D_Color32(0x00, 0xff, 0xff, 0xFF);
        } else if (isSelected) {
            borderCol = C2D_Color32(0xff, 0xff, 0x77, 0xFF);
        } else if (isEquipped) {
            borderCol = C2D_Color32(0x8c, 0x96, 0x88, 0xFF);
        } else {
            borderCol = C2D_Color32(0x2f, 0x33, 0x31, 0xFF);
        }

        C2D_DrawRectSolid(sx, sy, 0.75f, SLOT_SIZE, SLOT_SIZE, slotBg);
        C2D_DrawRectSolid(sx, sy, 0.78f, SLOT_SIZE, 1.0f, borderCol);
        C2D_DrawRectSolid(sx, sy + SLOT_SIZE - 1.0f, 0.78f, SLOT_SIZE, 1.0f, borderCol);
        C2D_DrawRectSolid(sx, sy, 0.78f, 1.0f, SLOT_SIZE, borderCol);
        C2D_DrawRectSolid(sx + SLOT_SIZE - 1.0f, sy, 0.78f, 1.0f, SLOT_SIZE, borderCol);

        if (hasItem && item) {
            // Center item sprite
            if (s_itemsFilm) {
                noosa::Image icon(s_itemsFilm, item->image);
                icon.targetScreen = noosa::ScreenTarget::Bottom;
                icon.x = sx + (SLOT_SIZE - 16.0f) * 0.5f;
                icon.y = sy + (SLOT_SIZE - 16.0f) * 0.5f;
                icon.z = 0.80f;
                icon.draw(nullptr);
            } else {
                std::string label = item->name.substr(0, 3);
                C2D_TextParse(&textObj, textBuf, label.c_str());
                C2D_TextOptimize(&textObj);
                C2D_DrawText(&textObj, C2D_WithColor, sx + 4.0f, sy + 8.0f, 0.85f, 0.36f, 0.36f,
                             isEquipped ? C2D_Color32(0xe4, 0xa6, 0x72, 0xFF) : C2D_Color32(0xff, 0xff, 0xff, 0xFF));
            }

            // Top-Left: Quantity of item
            if (item->quantity > 1) {
                std::string q = "x" + std::to_string(item->quantity);
                C2D_TextParse(&textObj, textBuf, q.c_str());
                C2D_TextOptimize(&textObj);
                // Shadow
                C2D_DrawText(&textObj, C2D_WithColor, sx + 3.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                C2D_DrawText(&textObj, C2D_WithColor, sx + 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));
            }

            // Top-Right: Strength required for weapons and armors
            if (item->category() == ItemCategory::Weapon && !item->asWand()) {
                auto* w = static_cast<Weapon*>(item);
                if (w->levelKnown || w->isIdentified()) {
                    int req = std::max(1, w->STR - w->level);
                    std::string s = ":" + std::to_string(req);
                    C2D_TextParse(&textObj, textBuf, s.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.32f;
                    u32 col = (req > hero->STR) ? C2D_Color32(0xff, 0x44, 0x44, 0xFF) : C2D_Color32(0xff, 0xff, 0xff, 0xFF);
                    // Shadow
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, col);
                } else {
                    std::string s = "?" + std::to_string(w->typicalSTR());
                    C2D_TextParse(&textObj, textBuf, s.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.32f;
                    // Shadow
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, C2D_Color32(0xff, 0x88, 0x00, 0xFF));
                }
            } else if (item->category() == ItemCategory::Armor) {
                auto* a = static_cast<Armor*>(item);
                if (a->levelKnown || a->isIdentified()) {
                    int req = std::max(1, a->STR - a->level);
                    std::string s = ":" + std::to_string(req);
                    C2D_TextParse(&textObj, textBuf, s.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.32f;
                    u32 col = (req > hero->STR) ? C2D_Color32(0xff, 0x44, 0x44, 0xFF) : C2D_Color32(0xff, 0xff, 0xff, 0xFF);
                    // Shadow
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, col);
                } else {
                    std::string s = "?" + std::to_string(a->typicalSTR());
                    C2D_TextParse(&textObj, textBuf, s.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.32f;
                    // Shadow
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, C2D_Color32(0xff, 0x88, 0x00, 0xFF));
                }
            }

            // Top-Right: Identified Potion or Scroll Badge/Icon
            if (item->category() == ItemCategory::Potion && item->isIdentified()) {
                auto* pot = static_cast<Potion*>(item);
                if (pot->type == Potion::PotionType::HEALING) {
                    // Bold red cross
                    float cx = sx + SLOT_SIZE - 8.0f;
                    float cy = sy + 3.0f;
                    C2D_DrawRectSolid(cx - 3.0f, cy + 1.0f, 0.84f, 9.0f, 5.0f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawRectSolid(cx - 1.0f, cy - 1.0f, 0.84f, 5.0f, 9.0f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawRectSolid(cx - 2.0f, cy + 2.0f, 0.85f, 7.0f, 3.0f, C2D_Color32(0xff, 0x22, 0x22, 0xFF));
                    C2D_DrawRectSolid(cx, cy, 0.85f, 3.0f, 7.0f, C2D_Color32(0xff, 0x22, 0x22, 0xFF));
                } else {
                    const char* badgeStr = "";
                    u32 badgeCol = 0xffffffff;
                    switch (pot->type) {
                        case Potion::PotionType::EXPERIENCE:    badgeStr = "XP"; badgeCol = C2D_Color32(0xff, 0xd7, 0x00, 0xFF); break;
                        case Potion::PotionType::TOXIC_GAS:      badgeStr = "G";  badgeCol = C2D_Color32(0x44, 0xff, 0x44, 0xFF); break;
                        case Potion::PotionType::LIQUID_FLAME:   badgeStr = "F";  badgeCol = C2D_Color32(0xff, 0x55, 0x00, 0xFF); break;
                        case Potion::PotionType::STRENGTH:       badgeStr = "S";  badgeCol = C2D_Color32(0xff, 0x33, 0x33, 0xFF); break;
                        case Potion::PotionType::PARALYTIC_GAS:  badgeStr = "P";  badgeCol = C2D_Color32(0xff, 0xff, 0x00, 0xFF); break;
                        case Potion::PotionType::LEVITATION:     badgeStr = "^";  badgeCol = C2D_Color32(0x77, 0xcc, 0xff, 0xFF); break;
                        case Potion::PotionType::MIND_VISION:    badgeStr = "O";  badgeCol = C2D_Color32(0x44, 0x88, 0xff, 0xFF); break;
                        case Potion::PotionType::PURITY:         badgeStr = "o";  badgeCol = C2D_Color32(0xee, 0xee, 0xff, 0xFF); break;
                        case Potion::PotionType::INVISIBILITY:   badgeStr = "I";  badgeCol = C2D_Color32(0xcc, 0x88, 0xff, 0xFF); break;
                        case Potion::PotionType::MIGHT:          badgeStr = "M";  badgeCol = C2D_Color32(0xff, 0x22, 0x44, 0xFF); break;
                        case Potion::PotionType::FROST:          badgeStr = "*";  badgeCol = C2D_Color32(0x66, 0xff, 0xff, 0xFF); break;
                        default: break;
                    }
                    if (badgeStr[0] != '\0') {
                        C2D_TextParse(&textObj, textBuf, badgeStr);
                        C2D_TextOptimize(&textObj);
                        float tw = textObj.width * 0.32f;
                        C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                        C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, badgeCol);
                    }
                }
            } else if (item->category() == ItemCategory::Scroll && item->isIdentified()) {
                auto* scr = static_cast<Scroll*>(item);
                if (scr->type == Scroll::ScrollType::UPGRADE) {
                    // Bright green upward arrow matching source PD
                    float ax = sx + SLOT_SIZE - 8.0f;
                    float ay = sy + 2.0f;
                    C2D_DrawTriangle(ax + 1.5f, ay - 1.0f, C2D_Color32(0x00, 0x00, 0x00, 0xCC),
                                     ax - 3.0f, ay + 5.5f, C2D_Color32(0x00, 0x00, 0x00, 0xCC),
                                     ax + 6.0f, ay + 5.5f, C2D_Color32(0x00, 0x00, 0x00, 0xCC), 0.84f);
                    C2D_DrawRectSolid(ax - 1.0f, ay + 3.0f, 0.84f, 5.0f, 5.0f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawTriangle(ax + 1.5f, ay, C2D_Color32(0x44, 0xff, 0x44, 0xFF),
                                     ax - 2.0f, ay + 4.5f, C2D_Color32(0x44, 0xff, 0x44, 0xFF),
                                     ax + 5.0f, ay + 4.5f, C2D_Color32(0x44, 0xff, 0x44, 0xFF), 0.85f);
                    C2D_DrawRectSolid(ax, ay + 4.0f, 0.85f, 3.0f, 3.0f, C2D_Color32(0x44, 0xff, 0x44, 0xFF));
                } else if (scr->type == Scroll::ScrollType::IDENTIFY) {
                    // Cyan question mark
                    C2D_TextParse(&textObj, textBuf, "?");
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.38f;
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.38f, 0.38f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.38f, 0.38f, C2D_Color32(0x33, 0xcc, 0xff, 0xFF));
                } else if (scr->type == Scroll::ScrollType::REMOVE_CURSE) {
                    // White 4-point sparkle
                    float spx = sx + SLOT_SIZE - 7.0f;
                    float spy = sy + 4.0f;
                    C2D_DrawRectSolid(spx - 3.0f, spy, 0.84f, 7.0f, 3.0f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawRectSolid(spx - 1.0f, spy - 2.0f, 0.84f, 3.0f, 7.0f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawRectSolid(spx - 2.0f, spy + 1.0f, 0.85f, 5.0f, 1.0f, 0xffffffff);
                    C2D_DrawRectSolid(spx, spy - 1.0f, 0.85f, 1.0f, 5.0f, 0xffffffff);
                    C2D_DrawRectSolid(spx - 1.0f, spy, 0.85f, 3.0f, 3.0f, 0xffffffff);
                } else {
                    const char* badgeStr = "";
                    u32 badgeCol = 0xffffffff;
                    switch (scr->type) {
                        case Scroll::ScrollType::MAGIC_MAPPING: badgeStr = "M"; badgeCol = C2D_Color32(0xff, 0xcc, 0x00, 0xFF); break;
                        case Scroll::ScrollType::TELEPORTATION: badgeStr = "@"; badgeCol = C2D_Color32(0xbb, 0x66, 0xff, 0xFF); break;
                        case Scroll::ScrollType::RECHARGING:    badgeStr = "Z"; badgeCol = C2D_Color32(0xff, 0xff, 0x22, 0xFF); break;
                        case Scroll::ScrollType::CHALLENGE:     badgeStr = "!"; badgeCol = C2D_Color32(0xff, 0x33, 0x33, 0xFF); break;
                        case Scroll::ScrollType::TERROR:        badgeStr = "!"; badgeCol = C2D_Color32(0xff, 0x66, 0x00, 0xFF); break;
                        case Scroll::ScrollType::LULLABY:       badgeStr = "z"; badgeCol = C2D_Color32(0x66, 0xcc, 0xff, 0xFF); break;
                        case Scroll::ScrollType::PSIONIC_BLAST: badgeStr = "*"; badgeCol = C2D_Color32(0xff, 0x33, 0xaa, 0xFF); break;
                        case Scroll::ScrollType::MIRROR_IMAGE:  badgeStr = "="; badgeCol = C2D_Color32(0x33, 0xdd, 0xcc, 0xFF); break;
                        case Scroll::ScrollType::ENCHANTMENT:   badgeStr = "*"; badgeCol = C2D_Color32(0xdd, 0x55, 0xff, 0xFF); break;
                        default: break;
                    }
                    if (badgeStr[0] != '\0') {
                        C2D_TextParse(&textObj, textBuf, badgeStr);
                        C2D_TextOptimize(&textObj);
                        float tw = textObj.width * 0.32f;
                        C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + 2.0f, 0.84f, 0.32f, 0.32f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                        C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + 1.0f, 0.85f, 0.32f, 0.32f, badgeCol);
                    }
                }
            }

            // Bottom-Right: Enhancement (+N, -N, or cursed)
            if (item->isUpgradable() && (item->levelKnown || item->isIdentified())) {
                if (item->level > 0) {
                    std::string lvl = "+" + std::to_string(item->level);
                    C2D_TextParse(&textObj, textBuf, lvl.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.34f;
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + SLOT_SIZE - 9.0f, 0.84f, 0.34f, 0.34f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + SLOT_SIZE - 10.0f, 0.85f, 0.34f, 0.34f, C2D_Color32(0x44, 0xff, 0x44, 0xFF));
                } else if (item->level < 0) {
                    std::string lvl = std::to_string(item->level);
                    C2D_TextParse(&textObj, textBuf, lvl.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.34f;
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + SLOT_SIZE - 9.0f, 0.84f, 0.34f, 0.34f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + SLOT_SIZE - 10.0f, 0.85f, 0.34f, 0.34f, C2D_Color32(0xff, 0x44, 0x44, 0xFF));
                } else if (item->cursed && item->cursedKnown) {
                    std::string lvl = "-";
                    C2D_TextParse(&textObj, textBuf, lvl.c_str());
                    C2D_TextOptimize(&textObj);
                    float tw = textObj.width * 0.34f;
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 1.0f, sy + SLOT_SIZE - 9.0f, 0.84f, 0.34f, 0.34f, C2D_Color32(0x00, 0x00, 0x00, 0xCC));
                    C2D_DrawText(&textObj, C2D_WithColor, sx + SLOT_SIZE - tw - 2.0f, sy + SLOT_SIZE - 10.0f, 0.85f, 0.34f, 0.34f, C2D_Color32(0xff, 0x44, 0x44, 0xFF));
                }
            }

            // Bottom-Left: Wand charge pips (green for active, dark socket for depleted)
            if (Wand* wand = item->asWand()) {
                int maxCh = std::max(1, wand->maxCharges);
                int curCh = std::clamp(wand->curCharges, 0, maxCh);
                for (int i = 0; i < maxCh; ++i) {
                    int col = i % 4;
                    int row = i / 4;
                    float px = sx + 2.0f + col * 3.0f;
                    float py = sy + SLOT_SIZE - 5.0f - row * 3.0f;
                    u32 sqCol = (i < curCh) ? C2D_Color32(0x00, 0xee, 0x00, 0xFF) : C2D_Color32(0x28, 0x2b, 0x26, 0xFF);
                    C2D_DrawRectSolid(px, py, 0.85f, 2.0f, 2.0f, sqCol);
                }
            }
        }
    }

    // Right Details Box (classic stone gray theme)
    float detX = 160.0f;
    float detY = 30.0f;
    float detW = 152.0f;
    float detH = 202.0f;

    // Fill with neutral stone gray
    C2D_DrawRectSolid(detX, detY, 0.74f, detW, detH, C2D_Color32(0x38, 0x3d, 0x3a, 0xFF));

    // Outer border
    C2D_DrawRectSolid(detX, detY, 0.76f, detW, 1.0f, C2D_Color32(0x56, 0x5c, 0x58, 0xFF));
    C2D_DrawRectSolid(detX, detY + detH - 1.0f, 0.76f, detW, 1.0f, C2D_Color32(0x56, 0x5c, 0x58, 0xFF));
    C2D_DrawRectSolid(detX, detY, 0.76f, 1.0f, detH, C2D_Color32(0x56, 0x5c, 0x58, 0xFF));
    C2D_DrawRectSolid(detX + detW - 1.0f, detY, 0.76f, 1.0f, detH, C2D_Color32(0x56, 0x5c, 0x58, 0xFF));

    if (mode == Mode::SELECT_UPGRADE) {
        C2D_TextParse(&textObj, textBuf, "UPGRADE TARGET");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, detX + 8.0f, detY + 8.0f, 0.85f, 0.40f, 0.40f, C2D_Color32(0xff, 0xd7, 0x00, 0xFF));

        std::string promptStr = "Choose weapon or armor\nto enhance with Scroll.\n\n[B] Cancel";
        C2D_TextParse(&textObj, textBuf, promptStr.c_str());
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, detX + 8.0f, detY + 36.0f, 0.85f, 0.34f, 0.34f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));

        m_btnClose.draw(textBuf);
    } else if (mode == Mode::SELECT_IDENTIFY) {
        C2D_TextParse(&textObj, textBuf, "IDENTIFY TARGET");
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, detX + 8.0f, detY + 8.0f, 0.85f, 0.40f, 0.40f, C2D_Color32(0x66, 0xff, 0xff, 0xFF));

        std::string promptStr = "Choose an unidentified\nitem from backpack to\nreveal its secrets.\n\n[B] Cancel";
        C2D_TextParse(&textObj, textBuf, promptStr.c_str());
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, detX + 8.0f, detY + 36.0f, 0.85f, 0.34f, 0.34f, C2D_Color32(0xff, 0xff, 0xff, 0xFF));

        m_btnClose.draw(textBuf);
    } else {
        if (m_selectedIndex >= 0 && m_selectedIndex < packSize && pack[m_selectedIndex]) {
            Item* it = pack[m_selectedIndex].get();

            // Draw item icon in details pane
            if (s_itemsFilm) {
                noosa::Image icon(s_itemsFilm, it->image);
                icon.targetScreen = noosa::ScreenTarget::Bottom;
                icon.x = detX + 8.0f;
                icon.y = detY + 8.0f;
                icon.z = 0.85f;
                icon.draw(nullptr);
            }

            // Draw item display name
            C2D_TextParse(&textObj, textBuf, it->displayName().c_str());
            C2D_TextOptimize(&textObj);
            C2D_DrawText(&textObj, C2D_WithColor, detX + 28.0f, detY + 8.0f, 0.85f, 0.38f, 0.38f, C2D_Color32(0xff, 0xd7, 0x00, 0xFF));

            // Draw item info / stats text
            std::string itInfo = it->info();
            C2D_TextParse(&textObj, textBuf, itInfo.c_str());
            C2D_TextOptimize(&textObj);
            C2D_DrawText(&textObj, C2D_WithColor, detX + 8.0f, detY + 32.0f, 0.85f, 0.30f, 0.30f, C2D_Color32(0xdd, 0xdd, 0xdd, 0xFF));

            // Divider line
            C2D_DrawRectSolid(detX + 4.0f, detY + 68.0f, 0.8f, detW - 8.0f, 1.0f, C2D_Color32(0x4e, 0x54, 0x50, 0xFF));

            // Draw dynamic action buttons
            for (const auto& btn : m_actionButtons) {
                btn.draw(textBuf);
            }
        } else {
            C2D_TextParse(&textObj, textBuf, "Empty Slot");
            C2D_TextOptimize(&textObj);
            C2D_DrawText(&textObj, C2D_WithColor, detX + 10.0f, detY + 10.0f, 0.85f, 0.38f, 0.38f, C2D_Color32(0x8a, 0x8a, 0x8a, 0xFF));
        }

        m_btnClose.draw(textBuf);
    }
}

} // namespace pd::ui
