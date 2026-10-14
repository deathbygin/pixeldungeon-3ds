#include "pd/ui/BottomScreenUI.h"
#include "pd/Hero.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"
#include "pd/Heap.h"
#include "pd/Actor.h"
#include "pd/Terrain.h"
#include "pd/SaveManager.h"
#include "pd/GLog.h"
#include "noosa/Audio.h"

namespace pd::ui {

BottomScreenUI::BottomScreenUI()
    : m_btnWait(8.0f, 76.0f, 72.0f, 26.0f, "WAIT", "[A]", KEY_A),
      m_btnSearch(84.0f, 76.0f, 72.0f, 26.0f, "SEARCH", "[X]", KEY_X),
      m_btnInventory(160.0f, 76.0f, 74.0f, 26.0f, "ITEMS", "[Y]", KEY_Y),
      m_btnStairs(238.0f, 76.0f, 74.0f, 26.0f, "STAIRS", "[B]", KEY_B),
      m_btnQuick1(8.0f, 106.0f, 148.0f, 24.0f, "QUICK 1: POTION", "[L]", KEY_L),
      m_btnQuick2(164.0f, 106.0f, 148.0f, 24.0f, "QUICK 2: SCROLL", "[R]", KEY_R) {

    m_logConsole.y = 136.0f;
    m_logConsole.height = 76.0f;

    GLog::setLogger([this](const std::string& msg, u32 color) {
        m_logConsole.addMessage(msg, color);
    });
}

void BottomScreenUI::toggleInventory() {
    if (m_inventoryView.isOpen) {
        m_inventoryView.close();
    } else {
        m_inventoryView.open();
    }
}

void BottomScreenUI::log(std::string msg, u32 color) {
    m_logConsole.addMessage(std::move(msg), color);
}

bool BottomScreenUI::update(pd::Hero* hero, pd::Level* level, u32 kDown, u32 kHeld, u32 kUp) {
    m_touch.update(kDown, kHeld, kUp);

    if (!hero || !hero->isAlive()) return false;

    // 1. If Inventory is active on bottom screen, route inputs exclusively to it
    if (m_inventoryView.isOpen) {
        std::string invMsg;
        if (m_inventoryView.update(hero, m_touch, kDown, invMsg)) {
            if (!invMsg.empty()) {
                log(invMsg, C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));
            }
            Actor::process();
            return true;
        }
        return true;
    }

    // Toggle inventory via Hardware Button Y (when inventory is closed)
    if (kDown & KEY_Y) {
        toggleInventory();
        return false;
    }

    // 2. Normal HUD button handling
    Heap* standingHeap = level ? level->getHeap(hero->pos) : nullptr;
    if (standingHeap && !standingHeap->isEmpty()) {
        m_btnWait.label = "PICK UP";
        m_btnWait.shortcut = "[A]";
        m_btnWait.active = true;
    } else {
        m_btnWait.label = "WAIT";
        m_btnWait.shortcut = "[A]";
        m_btnWait.active = false;
    }

    // Wait / Rest / Pick Up
    if (m_btnWait.update(m_touch, kDown) || (kDown & KEY_A)) {
        if (standingHeap && !standingHeap->isEmpty()) {
            if (hero->pickUp(level)) {
                Actor::process();
                return true;
            }
            return false;
        } else {
            hero->rest();
            log("Hero rests a turn...", C2D_Color32(0xa5, 0xcd, 0xe4, 0xFF));
            Actor::process();
            return true;
        }
    }

    // Search surroundings
    if (m_btnSearch.update(m_touch, kDown) || (kDown & KEY_X)) {
        hero->search(level, true);
        Actor::process();
        return true;
    }

    // Open inventory via touch button
    if (m_btnInventory.update(m_touch, kDown)) {
        m_inventoryView.open();
        return false;
    }

    // Descend / Ascend stairs
    if (m_btnStairs.update(m_touch, kDown)) {
        if (level && level->get(hero->pos) == Terrain::EXIT) {
            Dungeon::nextLevel();
            SaveManager::saveGame();
            GLog::p("Descended to Depth " + std::to_string(Dungeon::depth) + "! Autosaved.");
        } else if (level && level->get(hero->pos) == Terrain::ENTRANCE && Dungeon::depth > 1) {
            Dungeon::prevLevel();
            SaveManager::saveGame();
            GLog::i("Ascended to Depth " + std::to_string(Dungeon::depth) + "! Autosaved.");
        } else {
            GLog::w("No staircase here.");
        }
        return false;
    }

    static noosa::SoundSample s_sndDrink;
    static noosa::SoundSample s_sndRead;
    static bool s_quickSndInit = false;
    if (!s_quickSndInit) {
        s_sndDrink.synthesizeTone(440.0f, 0.08f, 0.5f);
        s_sndRead.synthesizeTone(587.3f, 0.12f, 0.5f);
        s_quickSndInit = true;
    }

    // Quickslot 1 (Potion of Healing)
    if (m_btnQuick1.update(m_touch, kDown)) {
        auto& pack = hero->belongings.backpack;
        for (auto& item : pack) {
            if (item && item->category() == ItemCategory::Potion) {
                s_sndDrink.play(0.6f, 0.0f);
                log("Drank " + item->displayName(), C2D_Color32(0x38, 0x9e, 0x38, 0xFF));
                item->execute(hero, "DRINK");
                if (item->quantity <= 0) hero->belongings.drop(item.get());
                Actor::process();
                return true;
            }
        }
        log("No potions available!", C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
        return false;
    }

    // Quickslot 2 (Scroll / Alternate)
    if (m_btnQuick2.update(m_touch, kDown)) {
        auto& pack = hero->belongings.backpack;
        for (size_t idx = 0; idx < pack.size(); ++idx) {
            auto& item = pack[idx];
            if (item && item->category() == ItemCategory::Scroll) {
                Scroll* scr = static_cast<Scroll*>(item.get());
                if (scr->type == Scroll::ScrollType::IDENTIFY) {
                    m_inventoryView.openIdentify(static_cast<int>(idx));
                    log("Select an item to identify", C2D_Color32(0x66, 0xff, 0xff, 0xFF));
                    return false;
                } else if (scr->type == Scroll::ScrollType::UPGRADE) {
                    m_inventoryView.openUpgrade(static_cast<int>(idx));
                    log("Select an item to upgrade", C2D_Color32(0xff, 0xff, 0x66, 0xFF));
                    return false;
                }
                s_sndRead.play(0.6f, 0.0f);
                log("Read " + item->displayName(), C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));
                item->execute(hero, "READ");
                if (item->quantity <= 0) hero->belongings.drop(item.get());
                Actor::process();
                return true;
            }
        }
        log("No scrolls available!", C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
        return false;
    }

    return false;
}

void BottomScreenUI::draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) {
    // Background Slate
    C2D_DrawRectSolid(0.0f, 0.0f, 0.2f, 320.0f, 240.0f, C2D_Color32(0x22, 0x20, 0x34, 0xFF));

    // Top Header
    C2D_DrawRectSolid(0.0f, 0.0f, 0.5f, 320.0f, 20.0f, C2D_Color32(0x63, 0x48, 0x3d, 0xFF));
    C2D_Text textObj;
    std::string headerStr = "TACTICAL HUD  |  RomFS " + std::string(SaveManager::isRomfsMounted() ? "[OK]" : "[--]");
    C2D_TextParse(&textObj, textBuf, headerStr.c_str());
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_WithColor, 8.0f, 2.0f, 0.55f, 0.44f, 0.44f, C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));

    // 1. Status Bar (HP, STR, Depth, EXP, Buffs)
    m_statusBar.draw(hero, depth, textBuf);

    // 2. Action Strip Buttons
    m_btnWait.draw(textBuf);
    m_btnSearch.draw(textBuf);
    m_btnInventory.draw(textBuf);
    m_btnStairs.draw(textBuf);

    // 3. Quickslot Bar
    m_btnQuick1.draw(textBuf);
    m_btnQuick2.draw(textBuf);

    // 4. Combat / Narrative Log
    m_logConsole.draw(textBuf);

    // 5. Stylus touch indicator reticle
    if (m_touch.isDown()) {
        float tx = static_cast<float>(m_touch.x());
        float ty = static_cast<float>(m_touch.y());
        C2D_DrawCircleSolid(tx, ty, 0.85f, 5.0f, C2D_Color32(0xe4, 0xa6, 0x72, 0xAA));
        C2D_DrawCircleSolid(tx, ty, 0.90f, 2.5f, C2D_Color32(0x5a, 0xa5, 0xcd, 0xFF));
    }

    // 6. Modal Inventory Overlay (if open)
    if (m_inventoryView.isOpen) {
        m_inventoryView.draw(hero, textBuf);
    }
}

} // namespace pd::ui

