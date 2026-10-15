#include "pd/ui/BottomScreenUI.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Hero.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"
#include "pd/Heap.h"
#include "pd/Actor.h"
#include "pd/Terrain.h"
#include "pd/SaveManager.h"
#include "pd/GLog.h"
#include "pd/Ballistica.h"
#include "pd/PathFinder.h"
#include "pd/Mob.h"
#include "pd/FX.h"
#include "pd/Assets.h"
#include "noosa/Audio.h"
#include <algorithm>

namespace pd::ui {

BottomScreenUI::BottomScreenUI() {
    m_logConsole.y = 158.0f;
    m_logConsole.height = 48.0f;
    initLogger();
}

BottomScreenUI::BottomScreenUI(BottomScreenUI&& other) noexcept {
    *this = std::move(other);
}

BottomScreenUI& BottomScreenUI::operator=(BottomScreenUI&& other) noexcept {
    if (this != &other) {
        m_touch = std::move(other.m_touch);
        m_statusPane = std::move(other.m_statusPane);
        m_toolbar = std::move(other.m_toolbar);
        m_wndBag = std::move(other.m_wndBag);
        m_logConsole = std::move(other.m_logConsole);
        m_wndHero = std::move(other.m_wndHero);
        m_isTargeting = other.m_isTargeting;
        m_targetCell = other.m_targetCell;
        m_targetItem = other.m_targetItem;
        m_targetHoldTimer = other.m_targetHoldTimer;
        m_targetLastDirX = other.m_targetLastDirX;
        m_targetLastDirY = other.m_targetLastDirY;
        m_showMinimap = other.m_showMinimap;
        initLogger();
    }
    return *this;
}

void BottomScreenUI::initLogger() {
    GLog::setLogger([this](const std::string& msg, u32 color) {
        m_logConsole.addMessage(msg, color);
    });
}

void BottomScreenUI::reset() {
    m_logConsole.clear();
    cancelTargeting();
    if (m_wndBag.isOpen) m_wndBag.close();
    if (m_wndHero.isOpen()) m_wndHero.hide();
    m_wndGame.hide();
    m_wndSettings.hide();
    m_pendingGameAction = pd::windows::GameAction::NONE;
    m_toolbar = Toolbar();
    m_showMinimap = true;
    initLogger();
}

static bool canTargetCell(const Hero* hero, const Level* level, const Item* item, int cell) {
    if (!hero || !level || cell < 0 || cell >= Level::LENGTH) return false;

    // Check item max throw / cast distance (Chebyshev)
    int maxDist = item ? item->maxDistance(hero) : 6;
    int dist = PathFinder::distanceChebyshev(hero->pos, cell);
    if (dist > maxDist) return false;

    // Must be explored or currently visible
    if (!Dungeon::visible[cell] && !level->visited[cell] && !level->mapped[cell]) return false;

    // Deep solid wall check: if impassable, must be visible and adjacent to a walkable cell
    if (!level->isPassable(cell) && !level->avoid[cell]) {
        if (!Dungeon::visible[cell]) return false;
        bool hasWalkableNeighbor = false;
        for (int i = 0; i < 8; ++i) {
            int n = cell + PathFinder::NEIGHBOURS8[i];
            if (n >= 0 && n < Level::LENGTH && (level->isPassable(n) || level->avoid[n])) {
                hasWalkableNeighbor = true;
                break;
            }
        }
        if (!hasWalkableNeighbor) return false;
    }

    // Line of sight / raycast check: cannot target beyond blocking walls or closed doors
    int coll = Ballistica::cast(hero->pos, cell, false, false);
    if (coll != cell) {
        // If the cell itself is an impassable wall that stopped the ray,
        // coll will be the tile immediately preceding it (distance 1)
        if (!level->isPassable(cell) && !level->avoid[cell]) {
            if (PathFinder::distanceChebyshev(coll, cell) > 1) {
                return false;
            }
        } else {
            // Target cell is passable/door, but ray stopped earlier: blocked!
            return false;
        }
    }

    return true;
}

void BottomScreenUI::startTargeting(Item* item) {
    if (!item) return;
    m_isTargeting = true;
    m_targetItem = item;
    m_targetHoldTimer = 0;
    m_targetLastDirX = 0;
    m_targetLastDirY = 0;

    // Default target: snap to nearest visible enemy within maxDistance, or hero pos
    m_targetCell = Dungeon::hero ? Dungeon::hero->pos : 0;
    if (Dungeon::hero) {
        int maxDist = item->maxDistance(Dungeon::hero.get());
        float minDist = 999.0f;
        for (const auto& m : Dungeon::mobs) {
            if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                int dist = PathFinder::distanceChebyshev(Dungeon::hero->pos, m->pos);
                if (dist <= maxDist && static_cast<float>(dist) < minDist) {
                    minDist = static_cast<float>(dist);
                    m_targetCell = m->pos;
                }
            }
        }
    }
}

void BottomScreenUI::cancelTargeting() {
    m_isTargeting = false;
    m_targetItem = nullptr;
    m_targetHoldTimer = 0;
    m_targetLastDirX = 0;
    m_targetLastDirY = 0;
}

void BottomScreenUI::toggleInventory() {
    if (m_wndHero.isOpen()) {
        m_wndHero.hide();
    }
    if (m_wndBag.isOpen) {
        m_wndBag.close();
    } else {
        m_wndBag.open();
    }
}

void BottomScreenUI::toggleWndHero() {
    if (m_wndBag.isOpen) {
        m_wndBag.close();
    }
    m_wndHero.toggle();
}

void BottomScreenUI::openPauseMenu() {
    if (m_wndBag.isOpen) m_wndBag.close();
    if (m_wndHero.isOpen()) m_wndHero.hide();
    if (m_isTargeting) cancelTargeting();
    m_wndSettings.hide();
    m_wndGame.show();
}

void BottomScreenUI::closePauseMenu() {
    m_wndGame.hide();
    m_wndSettings.hide();
}

void BottomScreenUI::log(std::string msg, u32 color) {
    m_logConsole.addMessage(std::move(msg), color);
}

void BottomScreenUI::updateQuickslots(pd::Hero* hero) {
    if (!hero) return;

    // Configure Quickslot 1 dynamically if not manually set
    if (!m_toolbar.getQuickItem1()) {
        if (hero->heroClass == Hero::HeroClass::Mage) {
            for (auto& it : hero->belongings.backpack) {
                if (it && it->asWand()) {
                    m_toolbar.setQuickItem1(it.get());
                    break;
                }
            }
            if (!m_toolbar.getQuickItem1() && hero->belongings.equippedWeapon && hero->belongings.equippedWeapon->asWand()) {
                m_toolbar.setQuickItem1(hero->belongings.equippedWeapon);
            }
        } else if (hero->heroClass == Hero::HeroClass::Huntress) {
            for (auto& it : hero->belongings.backpack) {
                if (it && it->asBoomerang()) {
                    m_toolbar.setQuickItem1(it.get());
                    break;
                }
            }
            if (!m_toolbar.getQuickItem1() && hero->belongings.equippedWeapon && hero->belongings.equippedWeapon->asBoomerang()) {
                m_toolbar.setQuickItem1(hero->belongings.equippedWeapon);
            }
        } else {
            for (auto& it : hero->belongings.backpack) {
                if (it && it->asDart()) {
                    m_toolbar.setQuickItem1(it.get());
                    break;
                }
            }
        }
    }
}

bool BottomScreenUI::update(pd::Hero* hero, pd::Level* level, u32 kDown, u32 kHeld, u32 kUp) {
    m_touch.update(kDown, kHeld, kUp);

    if (!hero || !hero->isAlive()) return false;

    updateQuickslots(hero);

    // Lock input during flying missile trajectory
    if (FX::isMissileActive()) {
        return false;
    }

    // 1. If WndSettings modal is open, route input exclusively to it
    if (m_wndSettings.isOpen()) {
        m_wndSettings.update(m_touch, kDown);
        if (!m_wndSettings.isOpen()) {
            m_wndGame.show(); // Return to in-game pause menu
        }
        return false;
    }

    // 2. If WndGame modal is open, route input exclusively to it
    if (m_wndGame.isOpen()) {
        pd::windows::GameAction act = m_wndGame.update(m_touch, kDown);
        if (act == pd::windows::GameAction::SETTINGS) {
            m_wndGame.hide();
            m_wndSettings.show();
        } else if (act == pd::windows::GameAction::MAIN_MENU || act == pd::windows::GameAction::EXIT_GAME) {
            m_pendingGameAction = act;
            m_wndGame.hide();
        }
        return false;
    }

    // 3. If WndHero modal is open, route input exclusively to it
    if (m_wndHero.isOpen()) {
        m_wndHero.update(m_touch, kDown, hero);
        return false;
    }

    // 4. If WndBag (Inventory) is open, route inputs exclusively to it
    if (m_wndBag.isOpen) {
        std::string bagMsg;
        bool actionTaken = m_wndBag.update(hero, m_touch, kDown, bagMsg);

        if (bagMsg == "START_TARGETING") {
            pd::Item* targetIt = m_wndBag.getSelectedItem(hero);
            if (targetIt) {
                startTargeting(targetIt);
                log("Aiming " + targetIt->displayName() + "...", C2D_Color32(0xff, 0xff, 0x66, 0xFF));
            }
            return false;
        } else if (bagMsg == "SET_QUICKSLOT") {
            pd::Item* it = m_wndBag.getSelectedItem(hero);
            if (it) {
                m_toolbar.setQuickItem1(it);
                log("Quickslot assigned: " + it->displayName(), C2D_Color32(0x56, 0xbb, 0x77, 0xFF));
            }
            return false;
        }

        if (actionTaken) {
            if (!bagMsg.empty()) {
                log(bagMsg, C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));
            }
            Actor::process();
            return true;
        }
        return false;
    }

    // 3. Ranged Targeting Mode (Aiming wand/missile)
    if (m_isTargeting) {
        if (!m_targetItem) {
            cancelTargeting();
            return false;
        }

        // Hardware shortcut R: snap to nearest visible enemy within maxDistance
        if (kDown & KEY_R) {
            float minDist = 9999.0f;
            int bestCell = -1;
            int maxDist = m_targetItem->maxDistance(hero);
            for (const auto& m : Dungeon::mobs) {
                if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                    int dist = PathFinder::distanceChebyshev(hero->pos, m->pos);
                    if (dist <= maxDist && static_cast<float>(dist) < minDist) {
                        minDist = static_cast<float>(dist);
                        bestCell = m->pos;
                    }
                }
            }
            if (bestCell != -1) {
                m_targetCell = bestCell;
            }
        }

        // D-Pad and Circle Pad tile-by-tile reticle navigation with debouncing and autorepeat
        circlePosition cpad;
        hidCircleRead(&cpad);
        int curDirX = 0;
        int curDirY = 0;

        if ((kHeld & KEY_DLEFT) || cpad.dx < -55) curDirX = -1;
        else if ((kHeld & KEY_DRIGHT) || cpad.dx > 55) curDirX = 1;

        if ((kHeld & KEY_DUP) || cpad.dy > 55) curDirY = -1;
        else if ((kHeld & KEY_DDOWN) || cpad.dy < -55) curDirY = 1;

        bool shouldStep = false;
        if (curDirX == 0 && curDirY == 0) {
            m_targetHoldTimer = 0;
            m_targetLastDirX = 0;
            m_targetLastDirY = 0;
        } else {
            bool newDir = (curDirX != m_targetLastDirX || curDirY != m_targetLastDirY);
            bool dpadTap = (kDown & (KEY_DLEFT | KEY_DRIGHT | KEY_DUP | KEY_DDOWN)) != 0;
            if (newDir || dpadTap) {
                shouldStep = true;
                m_targetHoldTimer = 1;
                m_targetLastDirX = curDirX;
                m_targetLastDirY = curDirY;
            } else {
                m_targetHoldTimer++;
                // Initial delay: 18 frames (~300ms), repeat every 5 frames (~12 tiles/s)
                if (m_targetHoldTimer >= 18 && ((m_targetHoldTimer - 18) % 5 == 0)) {
                    shouldStep = true;
                }
            }
        }

        if (shouldStep) {
            int tx = m_targetCell % Level::WIDTH + curDirX;
            int ty = m_targetCell / Level::WIDTH + curDirY;
            if (tx >= 0 && tx < Level::WIDTH && ty >= 0 && ty < Level::HEIGHT) {
                int nextCell = ty * Level::WIDTH + tx;
                if (canTargetCell(hero, level, m_targetItem, nextCell)) {
                    m_targetCell = nextCell;
                }
            }
        }

        // Confirm Throw: A button
        if (kDown & KEY_A) {
            Item* it = m_targetItem;
            int target = m_targetCell;
            cancelTargeting();
            it->cast(hero, target);
            return false; // Turn is advanced upon missile impact callback
        }

        // Cancel Throw: B button
        if (kDown & KEY_B) {
            cancelTargeting();
            log("Throw cancelled.", C2D_Color32(0xa5, 0xcd, 0xe4, 0xFF));
            return false;
        }

        return false;
    }

    // 4. Status Pane Touch & Menu Handling
    int statusAction = m_statusPane.update(m_touch, kDown);
    if (statusAction == 1) {
        toggleWndHero();
        return false;
    } else if (statusAction == 2 || (kDown & KEY_START)) {
        openPauseMenu();
        return false;
    }

    if (kDown & KEY_SELECT) {
        toggleWndHero();
        return false;
    }

    // 5. Persistent Toolbar Handling (Wait, Search, Info, Bag, Quickslots)
    Toolbar::Action tbAction = m_toolbar.update(m_touch, kDown, hero);
    switch (tbAction) {
        case Toolbar::Action::WAIT: {
            Heap* standingHeap = level ? level->getHeap(hero->pos) : nullptr;
            if (standingHeap && !standingHeap->isEmpty()) {
                if (hero->pickUp(level)) {
                    Actor::process();
                    return true;
                }
            } else {
                hero->rest();
                log("Hero rests a turn...", C2D_Color32(0xa5, 0xcd, 0xe4, 0xFF));
                Actor::process();
                return true;
            }
            break;
        }

        case Toolbar::Action::SEARCH: {
            hero->search(level, true);
            Actor::process();
            return true;
        }

        case Toolbar::Action::INFO: {
            log("Tap a visible tile or creature to inspect.", C2D_Color32(0xff, 0xcc, 0x66, 0xFF));
            break;
        }

        case Toolbar::Action::BAG: {
            toggleInventory();
            return false;
        }

        case Toolbar::Action::QUICK1: {
            pd::Item* qIt = m_toolbar.getQuickItem1();
            if (qIt) {
                if (Wand* w = qIt->asWand()) {
                    if (w->curCharges > 0) {
                        startTargeting(w);
                    } else {
                        log("Wand is out of charges!", C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
                    }
                    return false;
                } else if (qIt->asMissile()) {
                    startTargeting(qIt);
                    return false;
                } else {
                    std::string act = (qIt->category() == ItemCategory::Potion) ? "DRINK" :
                                      (qIt->category() == ItemCategory::Scroll) ? "READ" : "USE";
                    log(act + " " + qIt->displayName(), C2D_Color32(0x38, 0x9e, 0x38, 0xFF));
                    qIt->execute(hero, act);
                    if (qIt->quantity <= 0) hero->belongings.drop(qIt);
                    Actor::process();
                    return true;
                }
            } else {
                log("Quickslot 1 is empty!", C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
            }
            break;
        }

        case Toolbar::Action::QUICK2: {
            pd::Item* qIt = m_toolbar.getQuickItem2();
            if (qIt) {
                if (Wand* w = qIt->asWand()) {
                    if (w->curCharges > 0) startTargeting(w);
                    return false;
                } else if (qIt->asMissile()) {
                    startTargeting(qIt);
                    return false;
                }
            } else {
                log("Quickslot 2 is empty!", C2D_Color32(0xd6, 0x30, 0x30, 0xFF));
            }
            break;
        }

        case Toolbar::Action::NONE:
        default:
            break;
    }

    return false;
}

void BottomScreenUI::drawMinimap(const pd::Level* level, const pd::Hero* hero) const {
    if (!level || !hero) return;

    float frameX = 4.0f;
    float frameY = 36.0f;
    float frameW = 154.0f;
    float frameH = 118.0f;
    float zMap = 0.30f;

    // 1. Dedicated stone window frame built from chrome.png 9-slice
    Chrome::get(Chrome::Type::WINDOW).draw(frameX, frameY, frameW, frameH, zMap);

    // 2. Inner viewport (142x106)
    float vpX = frameX + 6.0f;
    float vpY = frameY + 6.0f;
    float vpW = 142.0f;
    float vpH = 106.0f;

    C2D_DrawRectSolid(vpX, vpY, zMap + 0.01f, vpW, vpH, C2D_Color32(0x10, 0x0E, 0x17, 0xFF));

    // 3. Render 3x3 pixel cells centered around hero
    float cellSize = 3.0f;
    int heroTileX = hero->pos % Level::WIDTH;
    int heroTileY = hero->pos / Level::WIDTH;

    int cols = static_cast<int>(vpW / cellSize); // ~47
    int rows = static_cast<int>(vpH / cellSize); // ~35

    int startX = heroTileX - (cols / 2);
    int endX = startX + cols;
    int startY = heroTileY - (rows / 2);
    int endY = startY + rows;

    for (int y = startY; y < endY; ++y) {
        if (y < 0 || y >= Level::HEIGHT) continue;
        for (int x = startX; x < endX; ++x) {
            if (x < 0 || x >= Level::WIDTH) continue;

            int cell = y * Level::WIDTH + x;
            if (!level->visited[cell] && !level->mapped[cell]) continue;

            float px = vpX + (x - startX) * cellSize;
            float py = vpY + (y - startY) * cellSize;

            int terr = level->map[cell];
            u32 col = C2D_Color32(0x38, 0x34, 0x44, 0xFF); // explored floor/corridor

            if (terr == Terrain::WALL || terr == Terrain::WALL_DECO) col = C2D_Color32(0x58, 0x52, 0x64, 0xFF);
            else if (terr == Terrain::DOOR || terr == Terrain::OPEN_DOOR) col = C2D_Color32(0x99, 0x77, 0x44, 0xFF);
            else if (terr == Terrain::WATER) col = C2D_Color32(0x33, 0x55, 0x99, 0xFF);
            else if (terr == Terrain::EXIT || terr == Terrain::ENTRANCE) col = C2D_Color32(0xAA, 0x55, 0xAA, 0xFF);
            else if (terr == Terrain::HIGH_GRASS || terr == Terrain::GRASS) col = C2D_Color32(0x44, 0x77, 0x33, 0xFF);

            C2D_DrawRectSolid(px, py, zMap + 0.02f, cellSize, cellSize, col);
        }
    }

    // 4. Hero position as pulsing bright gold 3x3 dot
    float hpx = vpX + (heroTileX - startX) * cellSize;
    float hpy = vpY + (heroTileY - startY) * cellSize;
    if (hpx >= vpX && hpx <= vpX + vpW - cellSize && hpy >= vpY && hpy <= vpY + vpH - cellSize) {
        C2D_DrawRectSolid(hpx, hpy, zMap + 0.05f, cellSize, cellSize, C2D_Color32(0xFF, 0xDD, 0x22, 0xFF));
    }

    // 5. Visible enemies in bright red
    for (const auto& m : Dungeon::mobs) {
        if (m && m->isAlive() && Dungeon::visible[m->pos]) {
            int mx = m->pos % Level::WIDTH;
            int my = m->pos / Level::WIDTH;
            if (mx >= startX && mx < endX && my >= startY && my < endY) {
                float epx = vpX + (mx - startX) * cellSize;
                float epy = vpY + (my - startY) * cellSize;
                if (epx >= vpX && epx <= vpX + vpW - cellSize && epy >= vpY && epy <= vpY + vpH - cellSize) {
                    C2D_DrawRectSolid(epx, epy, zMap + 0.04f, cellSize, cellSize, C2D_Color32(0xFF, 0x22, 0x22, 0xFF));
                }
            }
        }
    }

    // 6. Depth indicator tag on upper-left border
    std::string depthTag = std::to_string(Dungeon::depth) + "F";
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, depthTag, frameX + 8.0f, frameY + 2.0f, zMap + 0.10f, 0xCACFC2FF, 0x000000FF);
}

void BottomScreenUI::drawRightGlanceCard(const pd::Hero* hero, C2D_TextBuf textBuf) const {
    (void)textBuf;
    if (!hero) return;

    float cardX = 162.0f;
    float cardY = 36.0f;
    float cardW = 154.0f;
    float cardH = 118.0f;
    float zCard = 0.30f;
    float zText = zCard + 0.05f;

    // Outer frame with authentic Chrome::TOAST
    Chrome::get(Chrome::Type::TOAST).draw(cardX, cardY, cardW, cardH, zCard);

    // Check if an enemy is targeted or visible
    Char* targetChar = nullptr;
    if (m_isTargeting) {
        targetChar = Actor::findChar(m_targetCell);
    } else {
        float minDist = 999.0f;
        for (const auto& m : Dungeon::mobs) {
            if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                float dist = static_cast<float>(PathFinder::distanceChebyshev(hero->pos, m->pos));
                if (dist < minDist) {
                    minDist = dist;
                    targetChar = m.get();
                }
            }
        }
    }

    if (targetChar && targetChar != hero) {
        // --- MODE A: TARGET MOB IN VIEW ---
        // Header: Mob Name in Font1x with shadow
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, targetChar->name, cardX + 8.0f, cardY + 8.0f, zText, 0xFFFFFFFF, 0x000000FF);

        // Distance
        int dist = PathFinder::distanceChebyshev(hero->pos, targetChar->pos);
        std::string distStr = "Dist: " + std::to_string(dist) + " tiles";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, distStr, cardX + 8.0f, cardY + 22.0f, zText, 0xCACFC2FF, 0x000000FF);

        // Mob Health Bar
        float barX = cardX + 8.0f;
        float barY = cardY + 36.0f;
        float barW = 138.0f;
        float barH = 10.0f;
        float hpRatio = targetChar->HT > 0 ? std::clamp(static_cast<float>(targetChar->HP) / static_cast<float>(targetChar->HT), 0.0f, 1.0f) : 0.0f;

        C2D_DrawRectSolid(barX - 1.0f, barY - 1.0f, zText - 0.02f, barW + 2.0f, barH + 2.0f, C2D_Color32(0x14, 0x10, 0x18, 0xFF));
        C2D_DrawRectSolid(barX, barY, zText - 0.01f, barW, barH, C2D_Color32(0x3D, 0x11, 0x11, 0xFF));
        if (hpRatio > 0.0f) {
            u32 mobHpCol = hpRatio > 0.5f ? C2D_Color32(0x22, 0xCC, 0x44, 0xFF) : (hpRatio > 0.25f ? C2D_Color32(0xEE, 0xCC, 0x22, 0xFF) : C2D_Color32(0xEE, 0x33, 0x33, 0xFF));
            C2D_DrawRectSolid(barX, barY, zText, std::floor(barW * hpRatio), barH, mobHpCol);
        }

        char mobHpBuf[32];
        std::snprintf(mobHpBuf, sizeof(mobHpBuf), "%d / %d", targetChar->HP, targetChar->HT);
        float thW = BitmapFont::width(BitmapFont::FontType::Font1x, mobHpBuf);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, mobHpBuf, std::floor(barX + (barW - thW) * 0.5f), barY + 1.0f, zText + 0.01f, 0xFFFFFFFF, 0x000000FF);

        // Mob State / Awareness
        std::string statusStr = "Status: Hunting";
        u32 statusCol = C2D_Color32(0xEE, 0x44, 0x44, 0xFF);
        if (targetChar->buff(BuffType::Paralysis)) {
            statusStr = "Status: Paralyzed";
            statusCol = C2D_Color32(0xEE, 0xCC, 0x22, 0xFF);
        } else if (targetChar->buff(BuffType::Roots)) {
            statusStr = "Status: Rooted";
            statusCol = C2D_Color32(0x88, 0xBB, 0x44, 0xFF);
        }
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, statusStr, cardX + 8.0f, cardY + 52.0f, zText, statusCol, 0x000000FF);

        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Melee Hostile", cardX + 8.0f, cardY + 68.0f, zText, 0xCACFC2FF, 0x000000FF);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Tap to target/shoot", cardX + 8.0f, cardY + 84.0f, zText, 0x888899FF, 0x000000FF);
    } else {
        // --- MODE B: HERO QUICK STATS (EXPLORATION) ---
        std::string titleStr = hero->name + " (" + hero->className() + ")";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, titleStr, cardX + 8.0f, cardY + 8.0f, zText, 0xFFEBA4FF, 0x000000FF);

        // Stats summary
        std::string statStr = "STR: " + std::to_string(hero->STR) + "   LVL: " + std::to_string(hero->level);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, statStr, cardX + 8.0f, cardY + 22.0f, zText, 0xCACFC2FF, 0x000000FF);

        // Weapon
        std::string weapStr = "WEAP: ";
        if (hero->belongings.equippedWeapon) weapStr += hero->belongings.equippedWeapon->displayName();
        else weapStr += "None (Fists)";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, weapStr, cardX + 8.0f, cardY + 36.0f, zText, 0xE4A672FF, 0x000000FF);

        // Armor
        std::string armStr = "ARM: ";
        if (hero->belongings.equippedArmor) armStr += hero->belongings.equippedArmor->displayName();
        else armStr += "Cloth (+0)";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, armStr, cardX + 8.0f, cardY + 50.0f, zText, 0xCACFC2FF, 0x000000FF);

        // Gold
        std::string invStr = "GOLD: " + std::to_string(hero->gold);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, invStr, cardX + 8.0f, cardY + 64.0f, zText, 0xFFCC33FF, 0x000000FF);

        // XP Progress
        std::string expLabel = "EXP: " + std::to_string(hero->exp) + "/" + std::to_string(hero->maxExp);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, expLabel, cardX + 8.0f, cardY + 80.0f, zText, 0xCACFC2FF, 0x000000FF);

        float barX = cardX + 8.0f;
        float barY = cardY + 94.0f;
        float barW = 138.0f;
        float barH = 8.0f;
        float expRatio = hero->maxExp > 0 ? std::clamp(static_cast<float>(hero->exp) / static_cast<float>(hero->maxExp), 0.0f, 1.0f) : 0.0f;

        C2D_DrawRectSolid(barX - 1.0f, barY - 1.0f, zText - 0.02f, barW + 2.0f, barH + 2.0f, C2D_Color32(0x14, 0x10, 0x18, 0xFF));
        C2D_DrawRectSolid(barX, barY, zText - 0.01f, barW, barH, C2D_Color32(0x22, 0x14, 0x30, 0xFF));
        if (expRatio > 0.0f) {
            C2D_DrawRectSolid(barX, barY, zText, std::floor(barW * expRatio), barH, C2D_Color32(0x90, 0x40, 0xD0, 0xFF));
        }
    }
}

void BottomScreenUI::draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) {
    // 1. Dark Slate Base (320x240)
    C2D_DrawRectSolid(0.0f, 0.0f, 0.2f, 320.0f, 240.0f, C2D_Color32(0x18, 0x14, 0x25, 0xFF));

    // 2. Persistent Top Status Header (320x33, y=0..33)
    m_statusPane.draw(hero, depth, textBuf);

    // 3. Central Multi-Pane Area (when inventory & hero windows are closed)
    if (!m_wndBag.isOpen && !m_wndHero.isOpen()) {
        drawMinimap(Dungeon::level.get(), hero);
        drawRightGlanceCard(hero, textBuf);
        m_logConsole.draw(textBuf);
    }

    // 4. Persistent Bottom Action Toolbar (320x25, y=210..235)
    m_toolbar.draw(hero, textBuf, m_isTargeting, m_targetItem);

    // 5. Stylus Touch Indicator
    if (m_touch.isDown()) {
        float tx = static_cast<float>(m_touch.x());
        float ty = static_cast<float>(m_touch.y());
        C2D_DrawCircleSolid(tx, ty, 0.90f, 4.0f, C2D_Color32(0xe4, 0xa6, 0x72, 0x88));
        C2D_DrawCircleSolid(tx, ty, 0.92f, 2.0f, C2D_Color32(0x5a, 0xa5, 0xcd, 0xEE));
    }

    // 6. Modal Inventory Overlay (WndBag)
    if (m_wndBag.isOpen) {
        m_wndBag.draw(hero, textBuf);
    }

    // 7. Modal Hero Info Overlay (WndHero)
    if (m_wndHero.isOpen()) {
        m_wndHero.draw(hero, textBuf);
    }

    // 8. Modal Pause Menu (WndGame)
    if (m_wndGame.isOpen()) {
        m_wndGame.draw(textBuf);
    }

    // 9. Modal Settings Window (WndSettings)
    if (m_wndSettings.isOpen()) {
        m_wndSettings.draw(textBuf);
    }
}

} // namespace pd::ui
