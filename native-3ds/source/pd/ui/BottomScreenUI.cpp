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
#include "pd/Sample.h"
#include "pd/Sign.h"
#include "noosa/Audio.h"
#include <algorithm>

namespace pd::ui {

BottomScreenUI* BottomScreenUI::s_instance = nullptr;

BottomScreenUI::BottomScreenUI() {
    s_instance = this;
    m_logConsole.y = 158.0f;
    m_logConsole.height = 48.0f;
    initLogger();
}

BottomScreenUI::~BottomScreenUI() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void BottomScreenUI::selectItem(WndBag::Mode mode, const std::string& title, std::function<void(Item*)> onSelect) {
    m_wndBag.openForSelection(mode, title, std::move(onSelect));
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
        m_wndOptions = std::move(other.m_wndOptions);
        m_wndMessage = std::move(other.m_wndMessage);
        m_pendingGameAction = other.m_pendingGameAction;
        m_pendingTransition = other.m_pendingTransition;
        m_isInspecting = other.m_isInspecting;
        m_isExamining = other.m_isExamining;
        m_examineCell = other.m_examineCell;
        m_examineHoldTimer = other.m_examineHoldTimer;
        m_examineLastDirX = other.m_examineLastDirX;
        m_examineLastDirY = other.m_examineLastDirY;
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
    pd::Sign::setMessageCallback([this](const std::string& msg) {
        showMessage(msg);
    });
}

void BottomScreenUI::reset() {
    m_logConsole.clear();
    cancelTargeting();
    cancelExamine();
    m_lastTarget = nullptr;
    if (m_wndBag.isOpen) m_wndBag.close();
    if (m_wndHero.isOpen()) m_wndHero.hide();
    m_wndGame.hide();
    m_wndSettings.hide();
    m_wndOptions.hide();
    m_wndMessage.hide();
    m_pendingGameAction = pd::windows::GameAction::NONE;
    m_pendingTransition = LevelTransition::NONE;
    m_isInspecting = false;
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

void BottomScreenUI::setLastTarget(Char* target) {
    if (s_instance && target && target != Dungeon::hero.get()) {
        s_instance->m_lastTarget = target;
    }
}

void BottomScreenUI::startTargeting(Item* item) {
    if (!item) return;
    m_isTargeting = true;
    m_targetItem = item;
    m_targetHoldTimer = 0;
    m_targetLastDirX = 0;
    m_targetLastDirY = 0;

    // Default target: snap to last targeted enemy, or nearest visible enemy with clear LOS, or hero pos
    m_targetCell = Dungeon::hero ? Dungeon::hero->pos : 0;
    if (Dungeon::hero) {
        int maxDist = item->maxDistance(Dungeon::hero.get());

        bool targetValid = false;
        if (m_lastTarget && m_lastTarget->isAlive() && Dungeon::visible[m_lastTarget->pos]) {
            int dist = PathFinder::distanceChebyshev(Dungeon::hero->pos, m_lastTarget->pos);
            if (dist <= maxDist) {
                int castCell = Ballistica::cast(Dungeon::hero->pos, m_lastTarget->pos, false, true);
                if (castCell == m_lastTarget->pos) {
                    m_targetCell = m_lastTarget->pos;
                    targetValid = true;
                }
            }
        }

        if (!targetValid) {
            m_lastTarget = nullptr;
            float minDist = 999.0f;
            for (const auto& m : Dungeon::mobs) {
                if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                    int dist = PathFinder::distanceChebyshev(Dungeon::hero->pos, m->pos);
                    if (dist <= maxDist && static_cast<float>(dist) < minDist) {
                        int castCell = Ballistica::cast(Dungeon::hero->pos, m->pos, false, true);
                        if (castCell == m->pos) {
                            minDist = static_cast<float>(dist);
                            m_targetCell = m->pos;
                            m_lastTarget = m.get();
                        }
                    }
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

void BottomScreenUI::startExamine(int startCell) {
    m_isExamining = true;
    m_examineHoldTimer = 0;
    m_examineLastDirX = 0;
    m_examineLastDirY = 0;

    if (startCell >= 0 && startCell < Level::LENGTH) {
        m_examineCell = startCell;
    } else if (Dungeon::hero) {
        m_examineCell = Dungeon::hero->pos;
    } else {
        m_examineCell = 0;
    }

    log("Examine Mode: [D-Pad/CPad] Move  [A] Inspect  [B] Cancel", C2D_Color32(0xFF, 0xEE, 0x88, 0xFF));
}

void BottomScreenUI::cancelExamine() {
    if (m_isExamining) {
        m_isExamining = false;
        m_examineHoldTimer = 0;
        m_examineLastDirX = 0;
        m_examineLastDirY = 0;
        log("Examine mode cancelled.", C2D_Color32(0xA5, 0xCD, 0xE4, 0xFF));
    }
}

std::string BottomScreenUI::getHoverPreviewText(int cell, const pd::Level* level) const {
    if (!level || cell < 0 || cell >= Level::LENGTH) return "";

    if (!Dungeon::visible[cell] && !level->visited[cell] && !level->mapped[cell]) {
        return "Unexplored Area";
    }

    // 1. Hero
    if (Dungeon::hero && Dungeon::hero->pos == cell) {
        return Dungeon::hero->name + " (Hero, HP: " + std::to_string(Dungeon::hero->HP) + "/" + std::to_string(Dungeon::hero->HT) + ")";
    }

    // 2. Mob
    if (Dungeon::visible[cell]) {
        for (const auto& m : Dungeon::mobs) {
            if (m && m->isAlive() && m->pos == cell) {
                return m->name + " (" + m->statePreview() + ", HP: " + std::to_string(m->HP) + "/" + std::to_string(m->HT) + ")";
            }
        }
    }

    // 3. Item Heap
    Heap* heap = level->getHeap(cell);
    if (heap && (level->visited[cell] || Dungeon::visible[cell])) {
        if (heap->type == Heap::Type::CHEST) return "Chest (unlocked)";
        if (heap->type == Heap::Type::LOCKED_CHEST) return "Locked Chest";
        if (heap->type == Heap::Type::CRYSTAL_CHEST) return "Crystal Chest";
        if (heap->type == Heap::Type::TOMB) return "Tomb";
        if (heap->type == Heap::Type::SKELETON) return "Skeletal Remains";
        if (!heap->isEmpty() && heap->peek()) {
            return heap->peek()->displayName();
        }
    }

    // 3b. Active Plant
    Plant* plant = level->getPlant(cell);
    if (plant && (level->visited[cell] || Dungeon::visible[cell])) {
        return "Plant: " + plant->plantName;
    }

    // 4. Terrain & Traps
    int terr = level->map[cell];
    switch (terr) {
        case Terrain::SIGN:           return "Signpost";
        case Terrain::ENTRANCE:       return "Stairs Up";
        case Terrain::EXIT:           return "Stairs Down";
        case Terrain::DOOR:           return "Closed Door";
        case Terrain::OPEN_DOOR:      return "Open Door";
        case Terrain::LOCKED_DOOR:    return "Locked Door";
        case Terrain::WATER:          return "Water";
        case Terrain::HIGH_GRASS:     return "High Grass";
        case Terrain::GRASS:          return "Grass";
        case Terrain::STATUE:         return "Statue";
        case Terrain::ALCHEMY:        return "Alchemy Pot";
        case Terrain::CHASM:          return "Chasm";
        case Terrain::PEDESTAL:       return "Pedestal";
        case Terrain::BARRICADE:      return "Wood Barricade";
        case Terrain::BOOKSHELF:      return "Bookshelf";
        case Terrain::EMBERS:         return "Glowing Embers";
        case Terrain::TOXIC_TRAP:     return "Toxic Gas Trap";
        case Terrain::FIRE_TRAP:      return "Fire Trap";
        case Terrain::PARALYTIC_TRAP: return "Paralytic Gas Trap";
        case Terrain::POISON_TRAP:    return "Poison Dart Trap";
        case Terrain::ALARM_TRAP:     return "Alarm Trap";
        case Terrain::LIGHTNING_TRAP: return "Lightning Trap";
        case Terrain::GRIPPING_TRAP:  return "Gripping Trap";
        case Terrain::SECRET_DOOR:             return "Wall";
        case Terrain::SECRET_TOXIC_TRAP:
        case Terrain::SECRET_FIRE_TRAP:
        case Terrain::SECRET_PARALYTIC_TRAP:
        case Terrain::SECRET_POISON_TRAP:
        case Terrain::SECRET_ALARM_TRAP:
        case Terrain::SECRET_LIGHTNING_TRAP:
        case Terrain::SECRET_GRIPPING_TRAP:
        case Terrain::SECRET_SUMMONING_TRAP:   return "Floor";
        case Terrain::WALL:                    return "Wall";
        case Terrain::WALL_DECO:      return "Wall";
        case Terrain::EMPTY:          return "Floor";
        case Terrain::EMPTY_DECO:     return "Floor";
        case Terrain::EMPTY_SP:       return "Floor";
        default:                      return Level::tileName(terr);
    }
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

    if (!hero) return false;

    updateQuickslots(hero);

    // Lock input during flying missile trajectory
    if (FX::isMissileActive()) {
        return false;
    }

    // Modal Message Window (WndMessage)
    if (m_wndMessage.isOpen()) {
        m_wndMessage.update(m_touch, kDown);
        return false;
    }

    // Modal Options Window (WndOptions)
    if (m_wndOptions.isOpen()) {
        m_wndOptions.update(m_touch, kDown);
        return false;
    }

    // Modal Developer / Debug Menu (WndDebugMenu)
    if (m_wndDebugMenu.isOpen()) {
        pd::windows::DebugMenuAction act = m_wndDebugMenu.update(m_touch, kDown);
        if (act == pd::windows::DebugMenuAction::OPEN_ITEM_SPAWNER) {
            m_wndItemSpawner.show();
        }
        return false;
    }

    // Modal Item Spawner (WndItemSpawner)
    if (m_wndItemSpawner.isOpen()) {
        m_wndItemSpawner.update(m_touch, kDown);
        if (!m_wndItemSpawner.isOpen()) {
            m_wndDebugMenu.show(); // Return to Debug Menu
        }
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
            m_wndSettings.show(true);
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
        bool actionTaken = m_wndBag.update(hero, m_touch, kDown, kHeld, bagMsg);

        if (!hero->isAlive()) {
            // Dead hero can view inventory and inspect items, but not take gameplay actions
            return false;
        }

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
        if (!hero->isAlive() || !m_targetItem) {
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

    // 3.5. Top-Screen Examine Mode
    if (m_isExamining) {
        // Cancel via B
        if (kDown & KEY_B) {
            cancelExamine();
            return false;
        }

        // Toggle / Cancel via SELECT
        if (kDown & KEY_SELECT) {
            cancelExamine();
            return false;
        }

        // Detailed Inspect via A
        if (kDown & KEY_A) {
            inspectCell(m_examineCell, level);
            return false;
        }

        // D-Pad and Circle Pad tile-by-tile navigation with debouncing and autorepeat
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
            m_examineHoldTimer = 0;
            m_examineLastDirX = 0;
            m_examineLastDirY = 0;
        } else {
            bool newDir = (curDirX != m_examineLastDirX || curDirY != m_examineLastDirY);
            bool dpadTap = (kDown & (KEY_DLEFT | KEY_DRIGHT | KEY_DUP | KEY_DDOWN)) != 0;
            if (newDir || dpadTap) {
                shouldStep = true;
                m_examineHoldTimer = 1;
                m_examineLastDirX = curDirX;
                m_examineLastDirY = curDirY;
            } else {
                m_examineHoldTimer++;
                // Initial delay: 18 frames (~300ms), repeat every 5 frames (~12 tiles/s)
                if (m_examineHoldTimer >= 18 && ((m_examineHoldTimer - 18) % 5 == 0)) {
                    shouldStep = true;
                }
            }
        }

        if (shouldStep) {
            int ex = m_examineCell % Level::WIDTH + curDirX;
            int ey = m_examineCell / Level::WIDTH + curDirY;
            if (ex >= 0 && ex < Level::WIDTH && ey >= 0 && ey < Level::HEIGHT) {
                int nextCell = ey * Level::WIDTH + ex;
                // Bounds clamping: strictly to visible or previously visited/mapped tiles
                if (Dungeon::visible[nextCell] || (level && (level->visited[nextCell] || level->mapped[nextCell]))) {
                    m_examineCell = nextCell;
                }
            }
        }

        // Stylus minimap tap jumping to tile
        if (level && hero) {
            float vpX = 10.0f;
            float vpY = 42.0f;
            float vpW = 142.0f;
            float vpH = 106.0f;
            noosa::RectF minimapRect(vpX, vpY, vpW, vpH);
            if (m_touch.justReleased() && minimapRect.contains(static_cast<float>(m_touch.x()), static_cast<float>(m_touch.y()))) {
                float cellSize = 3.0f;
                int cols = static_cast<int>(vpW / cellSize);
                int rows = static_cast<int>(vpH / cellSize);
                int startX = (hero->pos % Level::WIDTH) - (cols / 2);
                int startY = (hero->pos / Level::WIDTH) - (rows / 2);

                int clickedX = startX + static_cast<int>((m_touch.x() - vpX) / cellSize);
                int clickedY = startY + static_cast<int>((m_touch.y() - vpY) / cellSize);
                if (clickedX >= 0 && clickedX < Level::WIDTH && clickedY >= 0 && clickedY < Level::HEIGHT) {
                    int cell = clickedY * Level::WIDTH + clickedX;
                    if (Dungeon::visible[cell] || level->visited[cell] || level->mapped[cell]) {
                        m_examineCell = cell;
                    }
                }
            }
        }

        // Tap on the Bottom Screen Examine HUD bar to inspect
        noosa::RectF barRect(4.0f, 158.0f, 312.0f, 48.0f);
        if (m_touch.justReleased() && barRect.contains(static_cast<float>(m_touch.x()), static_cast<float>(m_touch.y()))) {
            inspectCell(m_examineCell, level);
            return false;
        }

        // Toolbar check (e.g. tapping '?' button again cancels examine)
        Toolbar::Action tbAction = m_toolbar.update(m_touch, kDown, hero);
        if (tbAction == Toolbar::Action::INFO) {
            cancelExamine();
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
    } else if (statusAction == 3) {
        if (level && hero) {
            if (level->map[hero->pos] == Terrain::EXIT) {
                promptStairs(true);
            } else if (level->map[hero->pos] == Terrain::ENTRANCE) {
                promptStairs(false);
            } else {
                log("Depth " + std::to_string(Dungeon::depth) + ". Find the stairs down to progress.", C2D_Color32(0xff, 0xcc, 0x66, 0xFF));
            }
        }
        return false;
    } else if (statusAction == 4) {
        openDebugMenu();
        return false;
    }

    // Check if hero buff icon was clicked in StatusPane tray
    if (hero) {
        const pd::Buff* b = m_statusPane.clickedBuff(hero, m_touch);
        if (b) {
            std::string buffInfo = b->name() + "\n\n" + b->desc();
            showMessage(buffInfo);
            return false;
        }
    }

    // Hardware shortcuts to enter Examine Mode:
    // 1. SELECT button
    // 2. Double-tap X button
    static u64 lastTapXTick = 0;
    if (kDown & KEY_X) {
        u64 now = svcGetSystemTick();
        u64 dtTicks = now - lastTapXTick;
        lastTapXTick = now;
        if (dtTicks < 85000000ULL) { // within ~300ms
            if (m_wndBag.isOpen) m_wndBag.close();
            startExamine(hero ? hero->pos : 0);
            return false;
        }
    }

    if (kDown & KEY_SELECT) {
        if (!hero->isAlive()) {
            toggleWndHero();
            return false;
        }
        startExamine(hero ? hero->pos : 0);
        return false;
    }

    // 5. Minimap Touch Inspection
    if (!m_wndBag.isOpen && !m_wndHero.isOpen() && level && hero && hero->isAlive()) {
        float vpX = 10.0f;
        float vpY = 42.0f;
        float vpW = 142.0f;
        float vpH = 106.0f;
        noosa::RectF minimapRect(vpX, vpY, vpW, vpH);
        if (m_touch.justReleased() && minimapRect.contains(static_cast<float>(m_touch.x()), static_cast<float>(m_touch.y()))) {
            float cellSize = 3.0f;
            int cols = static_cast<int>(vpW / cellSize);
            int rows = static_cast<int>(vpH / cellSize);
            int startX = (hero->pos % Level::WIDTH) - (cols / 2);
            int startY = (hero->pos / Level::WIDTH) - (rows / 2);

            int clickedX = startX + static_cast<int>((m_touch.x() - vpX) / cellSize);
            int clickedY = startY + static_cast<int>((m_touch.y() - vpY) / cellSize);
            if (clickedX >= 0 && clickedX < Level::WIDTH && clickedY >= 0 && clickedY < Level::HEIGHT) {
                int cell = clickedY * Level::WIDTH + clickedX;
                if (level->visited[cell] || level->mapped[cell]) {
                    inspectCell(cell, level);
                    m_isInspecting = false;
                    return false;
                }
            }
        }
    }

    // 6. Persistent Toolbar Handling (Wait, Search, Info, Bag, Quickslots)
    Toolbar::Action tbAction = m_toolbar.update(m_touch, kDown, hero);
    switch (tbAction) {
        case Toolbar::Action::WAIT: {
            if (!hero->isAlive()) return false;
            if (level && hero) {
                if (level->map[hero->pos] == Terrain::EXIT) {
                    promptStairs(true);
                    return false;
                } else if (level->map[hero->pos] == Terrain::ENTRANCE) {
                    promptStairs(false);
                    return false;
                }
            }
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
            if (!hero->isAlive()) return false;
            hero->search(level, true);
            Actor::process();
            return true;
        }

        case Toolbar::Action::INFO: {
            if (m_isExamining) {
                cancelExamine();
            } else {
                startExamine(hero ? hero->pos : 0);
            }
            break;
        }

        case Toolbar::Action::BAG: {
            toggleInventory();
            return false;
        }

        case Toolbar::Action::QUICK1: {
            if (!hero->isAlive()) return false;
            QuickSlot& slot = m_toolbar.getSlot1();

            // 1. If slot is empty: open inventory in QUICKSLOT mode
            if (slot.isEmpty()) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                selectItem(WndBag::Mode::QUICKSLOT, "Select item for Quickslot 1", [this](Item* it) {
                    if (it) {
                        m_toolbar.setQuickItem1(it);
                        log("Quickslot 1 assigned: " + it->displayName(), COLOR_TEXT_POSITIVE);
                    }
                });
                return false;
            }

            // 2. Resolve active item
            pd::Item* qIt = slot.resolve(hero);

            // 3. If slot is ghost / depleted (quantity <= 0 or not found):
            if (slot.isGhost(hero) || !qIt || qIt->quantity <= 0) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                std::string name = !slot.placeholderName.empty() ? slot.placeholderName : "Item";
                log(name + " is depleted!", COLOR_TEXT_WARNING);
                return false;
            }

            // 4. Quick-fire double tap: if targeting is already active for this item
            if (m_isTargeting && m_targetItem == qIt) {
                int target = m_targetCell;
                Char* targetChar = Actor::findChar(target);
                if (targetChar && targetChar != hero) {
                    setLastTarget(targetChar);
                }
                cancelTargeting();
                qIt->cast(hero, target);
                return false;
            }

            // 5. Activate default action
            std::string act = !qIt->defaultAction.empty() ? qIt->defaultAction :
                              (qIt->asWand() ? "ZAP" :
                              (qIt->asMissile() ? "THROW" :
                              (qIt->category() == ItemCategory::Potion ? "DRINK" :
                              (qIt->category() == ItemCategory::Scroll ? "READ" : "USE"))));

            if (act == "THROW" || act == "ZAP") {
                startTargeting(qIt);
                log("Aiming " + qIt->displayName() + "...", COLOR_TEXT_ACCENT);
                return false;
            } else {
                log(act + " " + qIt->displayName(), COLOR_TEXT_POSITIVE);
                bool used = qIt->execute(hero, act);
                if (used) {
                    if (qIt->quantity <= 0) hero->belongings.drop(qIt);
                    Actor::process();
                    return true;
                }
                return false;
            }
        }

        case Toolbar::Action::QUICK2: {
            if (!hero->isAlive()) return false;
            QuickSlot& slot = m_toolbar.getSlot2();

            // 1. If slot is empty: open inventory in QUICKSLOT mode
            if (slot.isEmpty()) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                selectItem(WndBag::Mode::QUICKSLOT, "Select item for Quickslot 2", [this](Item* it) {
                    if (it) {
                        m_toolbar.setQuickItem2(it);
                        log("Quickslot 2 assigned: " + it->displayName(), COLOR_TEXT_POSITIVE);
                    }
                });
                return false;
            }

            // 2. Resolve active item
            pd::Item* qIt = slot.resolve(hero);

            // 3. If slot is ghost / depleted (quantity <= 0 or not found):
            if (slot.isGhost(hero) || !qIt || qIt->quantity <= 0) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                std::string name = !slot.placeholderName.empty() ? slot.placeholderName : "Item";
                log(name + " is depleted!", COLOR_TEXT_WARNING);
                return false;
            }

            // 4. Quick-fire double tap: if targeting is already active for this item
            if (m_isTargeting && m_targetItem == qIt) {
                int target = m_targetCell;
                Char* targetChar = Actor::findChar(target);
                if (targetChar && targetChar != hero) {
                    setLastTarget(targetChar);
                }
                cancelTargeting();
                qIt->cast(hero, target);
                return false;
            }

            // 5. Activate default action
            std::string act = !qIt->defaultAction.empty() ? qIt->defaultAction :
                              (qIt->asWand() ? "ZAP" :
                              (qIt->asMissile() ? "THROW" :
                              (qIt->category() == ItemCategory::Potion ? "DRINK" :
                              (qIt->category() == ItemCategory::Scroll ? "READ" : "USE"))));

            if (act == "THROW" || act == "ZAP") {
                startTargeting(qIt);
                log("Aiming " + qIt->displayName() + "...", COLOR_TEXT_ACCENT);
                return false;
            } else {
                log(act + " " + qIt->displayName(), COLOR_TEXT_POSITIVE);
                bool used = qIt->execute(hero, act);
                if (used) {
                    if (qIt->quantity <= 0) hero->belongings.drop(qIt);
                    Actor::process();
                    return true;
                }
                return false;
            }
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
    Chrome::get(Chrome::Type::TOAST).draw(frameX, frameY, frameW, frameH, zMap);

    // 2. Inner viewport
    float vpX = frameX + 5.0f;
    float vpY = frameY + 5.0f;
    float vpW = frameW - 10.0f;
    float vpH = frameH - 10.0f;

    C2D_DrawRectSolid(vpX, vpY, zMap + 0.01f, vpW, vpH, C2D_Color32(0x14, 0x14, 0x14, 0xFF));

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
            u32 col = C2D_Color32(0x36, 0x36, 0x36, 0xFF); // explored floor/corridor (neutral dark gray)

            if (terr == Terrain::WALL || terr == Terrain::WALL_DECO) col = C2D_Color32(0x56, 0x56, 0x56, 0xFF); // neutral medium gray
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

    // 6. Examine cursor on minimap (pulsing gold/white)
    if (m_isExamining) {
        int ex = m_examineCell % Level::WIDTH;
        int ey = m_examineCell / Level::WIDTH;
        if (ex >= startX && ex < endX && ey >= startY && ey < endY) {
            float epx = vpX + (ex - startX) * cellSize;
            float epy = vpY + (ey - startY) * cellSize;
            if (epx >= vpX && epx <= vpX + vpW - cellSize && epy >= vpY && epy <= vpY + vpH - cellSize) {
                static float minimapPulse = 0.0f;
                minimapPulse += 0.08f;
                float p = 0.5f + 0.5f * std::sin(minimapPulse);
                u8 r = static_cast<u8>(255 * p + 255 * (1.0f - p));
                u8 g = static_cast<u8>(215 * p + 255 * (1.0f - p));
                u8 b = static_cast<u8>(0 * p + 255 * (1.0f - p));
                C2D_DrawRectSolid(epx - 1.0f, epy - 1.0f, zMap + 0.07f, cellSize + 2.0f, cellSize + 2.0f, C2D_Color32(r, g, b, 0xFF));
            }
        }
    }

    // 7. Depth indicator tag on upper-left border
    std::string depthTag = std::to_string(Dungeon::depth) + "F";
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, depthTag, frameX + 8.0f, frameY + 2.0f, zMap + 0.10f, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
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

    // Check if an entity or enemy is targeted, examined, or visible
    Char* targetChar = nullptr;
    if (m_isTargeting) {
        targetChar = Actor::findChar(m_targetCell);
    } else if (m_isExamining) {
        if (Dungeon::visible[m_examineCell]) {
            targetChar = Actor::findChar(m_examineCell);
        }
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

    if (m_isExamining && (!targetChar || targetChar == hero)) {
        int cell = m_examineCell;
        const Level* level = Dungeon::level.get();
        if (cell != hero->pos) {
            // --- MODE C: EXAMINE TILE / ITEM / FEATURE ---
            std::string previewTitle = getHoverPreviewText(cell, level);
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, previewTitle, cardX + 8.0f, cardY + 8.0f, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

            int dist = PathFinder::distanceChebyshev(hero->pos, cell);
            float curX = cardX + 8.0f;
            float curY = cardY + 22.0f;
            curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Distance: ", curX, curY, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, std::to_string(dist) + " tiles", curX, curY, zText, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

            std::string visStr = Dungeon::visible[cell] ? "Visible (in LOS)" : "Remembered";
            u32 visCol = Dungeon::visible[cell] ? C2D_Color32(0x56, 0xBB, 0x77, 0xFF) : C2D_Color32(0x88, 0x88, 0x88, 0xFF);
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, visStr, cardX + 8.0f, cardY + 36.0f, zText, visCol, C2D_Color32(0, 0, 0, 0xFF));

            Heap* heap = level ? level->getHeap(cell) : nullptr;
            if (heap && (level->visited[cell] || Dungeon::visible[cell])) {
                BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Contains Item", cardX + 8.0f, cardY + 52.0f, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
            } else if (level) {
                std::string attr = level->isPassable(cell) ? "Passable" : "Impassable";
                if (level->avoid[cell]) attr += " (Hazard)";
                BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, attr, cardX + 8.0f, cardY + 52.0f, zText, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
            }

            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "[A] View Details", cardX + 8.0f, cardY + 74.0f, zText, C2D_Color32(0x56, 0xBB, 0x77, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "[B] Exit Examine", cardX + 8.0f, cardY + 88.0f, zText, C2D_Color32(0xA5, 0xCD, 0xE4, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
            return;
        }
    }

    if (targetChar && targetChar != hero) {
        // --- MODE A: TARGET MOB IN VIEW ---
        // Header: Mob Name in Font1x with shadow
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, targetChar->name, cardX + 8.0f, cardY + 8.0f, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

        // Distance
        int dist = PathFinder::distanceChebyshev(hero->pos, targetChar->pos);
        float curX = cardX + 8.0f;
        float curY = cardY + 22.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Dist: ", curX, curY, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, std::to_string(dist) + " tiles", curX, curY, zText, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

        // Mob Health Bar
        float barX = cardX + 8.0f;
        float barY = cardY + 36.0f;
        float barW = 138.0f;
        float barH = 10.0f;
        float hpRatio = targetChar->HT > 0 ? std::clamp(static_cast<float>(targetChar->HP) / static_cast<float>(targetChar->HT), 0.0f, 1.0f) : 0.0f;

        C2D_DrawRectSolid(barX - 1.0f, barY - 1.0f, zText - 0.02f, barW + 2.0f, barH + 2.0f, C2D_Color32(0x10, 0x10, 0x10, 0xFF));
        C2D_DrawRectSolid(barX, barY, zText - 0.01f, barW, barH, C2D_Color32(0x3D, 0x11, 0x11, 0xFF));
        if (hpRatio > 0.0f) {
            u32 mobHpCol = hpRatio > 0.5f ? C2D_Color32(0x22, 0xCC, 0x44, 0xFF) : (hpRatio > 0.25f ? C2D_Color32(0xEE, 0xCC, 0x22, 0xFF) : C2D_Color32(0xEE, 0x33, 0x33, 0xFF));
            C2D_DrawRectSolid(barX, barY, zText, std::floor(barW * hpRatio), barH, mobHpCol);
        }

        char mobHpBuf[32];
        std::snprintf(mobHpBuf, sizeof(mobHpBuf), "%d / %d", targetChar->HP, targetChar->HT);
        float thW = BitmapFont::width(BitmapFont::FontType::Font1x, mobHpBuf);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, mobHpBuf, std::floor(barX + (barW - thW) * 0.5f), barY + 1.0f, zText + 0.01f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), C2D_Color32(0, 0, 0, 0xFF));

        // Mob State / Awareness & Active Buffs
        std::string previewState = "Hunting";
        u32 statusCol = C2D_Color32(0xEE, 0x44, 0x44, 0xFF);
        if (Mob* mobTarget = targetChar->asMob()) {
            previewState = mobTarget->statePreview();
            if (mobTarget->state == Mob::State::SLEEPING) statusCol = C2D_Color32(0x88, 0xAA, 0xEE, 0xFF);
            else if (mobTarget->state == Mob::State::WANDERING) statusCol = C2D_Color32(0xEE, 0xCC, 0x22, 0xFF);
            else if (mobTarget->state == Mob::State::FLEEING) statusCol = C2D_Color32(0xCC, 0x88, 0xEE, 0xFF);
        }
        if (targetChar->buff(BuffType::Paralysis)) {
            previewState = "Paralyzed";
            statusCol = C2D_Color32(0xEE, 0xCC, 0x22, 0xFF);
        } else if (targetChar->buff(BuffType::Frost)) {
            previewState = "Frozen";
            statusCol = C2D_Color32(0x64, 0xBE, 0xFF, 0xFF);
        } else if (targetChar->buff(BuffType::Roots)) {
            previewState = "Rooted";
            statusCol = C2D_Color32(0x88, 0xBB, 0x44, 0xFF);
        }
        curX = cardX + 8.0f;
        curY = cardY + 50.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Status: ", curX, curY, zText, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF), C2D_Color32(0, 0, 0, 0xFF));
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, previewState, curX, curY, zText, statusCol, C2D_Color32(0, 0, 0, 0xFF));

        // List active buffs affecting target mob
        std::string buffList;
        bool first = true;
        for (const pd::Buff* b : targetChar->buffs) {
            if (!b) continue;
            if (!first) buffList += ", ";
            buffList += b->name();
            first = false;
        }
        if (first) buffList = "None";
        if (buffList.length() > 22) buffList = buffList.substr(0, 19) + "...";
        curX = cardX + 8.0f;
        curY = cardY + 64.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Buffs: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, buffList, curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

        if (m_isExamining) {
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "[A] View Lore & Stats", cardX + 8.0f, cardY + 76.0f, zText, COLOR_TEXT_POSITIVE, COLOR_TEXT_SHADOW);
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "[B] Exit Examine", cardX + 8.0f, cardY + 88.0f, zText, C2D_Color32(0xA5, 0xCD, 0xE4, 0xFF), COLOR_TEXT_SHADOW);
        } else {
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "[A] View Lore & Stats", cardX + 8.0f, cardY + 76.0f, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Tap to target/shoot", cardX + 8.0f, cardY + 88.0f, zText, COLOR_TEXT_MUTED, COLOR_TEXT_SHADOW);
        }
    } else {
        // --- MODE B: HERO QUICK STATS (EXPLORATION) ---
        std::string titleStr = hero->name + " (" + hero->className() + ")";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, titleStr, cardX + 8.0f, cardY + 8.0f, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);

        // Stats summary: STR: <val>   LVL: <val>
        float curX = cardX + 8.0f;
        float curY = cardY + 22.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "STR: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, std::to_string(hero->STR) + "   ", curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "LVL: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, std::to_string(hero->level), curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

        // Weapon
        curX = cardX + 8.0f;
        curY = cardY + 36.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "WEAP: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        std::string weapVal = hero->belongings.equippedWeapon ? hero->belongings.equippedWeapon->displayName() : "None (Fists)";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, weapVal, curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

        // Armor
        curX = cardX + 8.0f;
        curY = cardY + 50.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "ARM: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        std::string armVal = hero->belongings.equippedArmor ? hero->belongings.equippedArmor->displayName() : "Cloth (+0)";
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, armVal, curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

        // Gold
        curX = cardX + 8.0f;
        curY = cardY + 64.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "GOLD: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, std::to_string(hero->gold), curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

        // XP Progress
        curX = cardX + 8.0f;
        curY = cardY + 80.0f;
        curX = BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "EXP: ", curX, curY, zText, COLOR_TEXT_ACCENT, COLOR_TEXT_SHADOW);
        std::string expVal = std::to_string(hero->exp) + "/" + std::to_string(hero->maxExp);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, expVal, curX, curY, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

        float barX = cardX + 8.0f;
        float barY = cardY + 94.0f;
        float barW = 138.0f;
        float barH = 8.0f;
        float expRatio = hero->maxExp > 0 ? std::clamp(static_cast<float>(hero->exp) / static_cast<float>(hero->maxExp), 0.0f, 1.0f) : 0.0f;

        C2D_DrawRectSolid(barX - 1.0f, barY - 1.0f, zText - 0.02f, barW + 2.0f, barH + 2.0f, C2D_Color32(0x10, 0x10, 0x10, 0xFF));
        C2D_DrawRectSolid(barX, barY, zText - 0.01f, barW, barH, C2D_Color32(0x18, 0x18, 0x18, 0xFF));
        if (expRatio > 0.0f) {
            C2D_DrawRectSolid(barX, barY, zText, std::floor(barW * expRatio), barH, C2D_Color32(0x8A, 0x48, 0xC4, 0xFF));
        }
    }
}

void BottomScreenUI::draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf) {
    // 1. Very Dark Gray Base (320x240)
    C2D_DrawRectSolid(0.0f, 0.0f, 0.2f, 320.0f, 240.0f, C2D_Color32(0x14, 0x14, 0x14, 0xFF));

    // 2. Persistent Top Status Header (320x33, y=0..33)
    m_statusPane.draw(hero, depth, textBuf);

    // 3. Central Multi-Pane Area (when inventory & hero windows are closed)
    if (!m_wndBag.isOpen && !m_wndHero.isOpen()) {
        drawMinimap(Dungeon::level.get(), hero);
        drawRightGlanceCard(hero, textBuf);
        if (m_isExamining) {
            // Examine Mode Banner / HUD Bar
            float barX = 4.0f;
            float barY = 157.0f;
            float barW = 312.0f;
            float barH = 48.0f;
            float zBar = 0.35f;
            float zText = zBar + 0.05f;

            Chrome::get(Chrome::Type::TOAST).draw(barX, barY, barW, barH, zBar);

            // Pulsing gold title
            static float bannerPulse = 0.0f;
            bannerPulse += 0.05f;
            float p = 0.5f + 0.5f * std::sin(bannerPulse);
            u8 r = static_cast<u8>(255 * p + 255 * (1.0f - p));
            u8 g = static_cast<u8>(215 * p + 255 * (1.0f - p));
            u8 b = static_cast<u8>(0 * p + 255 * (1.0f - p));
            u32 titleCol = C2D_Color32(r, g, b, 0xFF);

            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "EXAMINE MODE", barX + 8.0f, barY + 6.0f, zText, titleCol, COLOR_TEXT_SHADOW);

            std::string preview = getHoverPreviewText(m_examineCell, Dungeon::level.get());
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, preview, barX + 8.0f, barY + 20.0f, zText, COLOR_TEXT_PRIMARY, COLOR_TEXT_SHADOW);

            std::string hint = "[A] Inspect Details    [B] Cancel Examine";
            BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, hint, barX + 8.0f, barY + 34.0f, zText, COLOR_TEXT_POSITIVE, COLOR_TEXT_SHADOW);
        } else {
            m_logConsole.draw(textBuf);
        }
    }

    // 4. Persistent Bottom Action Toolbar (320x25, y=210..235)
    m_toolbar.draw(hero, textBuf, m_isTargeting, m_targetItem, m_isExamining);

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

    // 10. Modal Options Window (WndOptions)
    if (m_wndOptions.isOpen()) {
        m_wndOptions.draw(textBuf, 0.85f);
    }

    // 11. Modal Message Window (WndMessage)
    if (m_wndMessage.isOpen()) {
        m_wndMessage.draw(textBuf, 0.86f);
    }

    // 12. Modal Developer / Debug Menu (WndDebugMenu)
    if (m_wndDebugMenu.isOpen()) {
        m_wndDebugMenu.draw(textBuf, 0.86f);
    }

    // 13. Modal Item Spawner (WndItemSpawner)
    if (m_wndItemSpawner.isOpen()) {
        m_wndItemSpawner.draw(textBuf, 0.88f);
    }
}

void BottomScreenUI::openDebugMenu() {
    if (m_wndBag.isOpen) m_wndBag.close();
    if (m_wndHero.isOpen()) m_wndHero.hide();
    if (m_wndGame.isOpen()) m_wndGame.hide();
    if (m_wndSettings.isOpen()) m_wndSettings.hide();
    if (m_wndItemSpawner.isOpen()) m_wndItemSpawner.hide();
    m_wndDebugMenu.show();
}

void BottomScreenUI::closeDebugMenu() {
    m_wndDebugMenu.hide();
    m_wndItemSpawner.hide();
}

void BottomScreenUI::openItemSpawner() {
    m_wndDebugMenu.hide();
    m_wndItemSpawner.show();
}

void BottomScreenUI::closeItemSpawner() {
    m_wndItemSpawner.hide();
}

void BottomScreenUI::showMessage(std::string text, std::function<void()> onDismiss) {
    if (m_wndBag.isOpen) m_wndBag.close();
    if (m_wndHero.isOpen()) m_wndHero.hide();
    m_wndMessage.show(std::move(text), std::move(onDismiss));
}

void BottomScreenUI::showOptions(std::string title, std::string message,
                                 std::vector<std::string> options,
                                 std::function<void(int)> onSelect) {
    if (m_wndBag.isOpen) m_wndBag.close();
    if (m_wndHero.isOpen()) m_wndHero.hide();
    m_wndOptions.show(std::move(title), std::move(message), std::move(options), std::move(onSelect));
}

void BottomScreenUI::promptStairs(bool descend) {
    if (m_wndOptions.isOpen() || m_wndMessage.isOpen()) return;

    if (descend) {
        showOptions("Stairs Down", "Do you want to descend to the next depth?",
            { "Yes, descend", "Cancel" },
            [this](int idx) {
                if (idx == 0) {
                    m_pendingTransition = LevelTransition::DESCEND;
                }
            });
    } else {
        if (Dungeon::depth == 1) {
            bool hasAmulet = false;
            if (Dungeon::hero && Dungeon::hero->belongings.getItem("Amulet of Yendor")) {
                hasAmulet = true;
            }
            if (!hasAmulet) {
                showMessage("One does not simply leave Pixel Dungeon.");
            } else {
                showOptions("Leave Dungeon", "Are you ready to return to the surface with the Amulet of Yendor?",
                    { "Yes, return to surface!", "Cancel" },
                    [this](int idx) {
                        if (idx == 0) {
                            m_pendingTransition = LevelTransition::WIN;
                        }
                    });
            }
        } else {
            showOptions("Stairs Up", "Do you want to ascend to the previous depth?",
                { "Yes, ascend", "Cancel" },
                [this](int idx) {
                    if (idx == 0) {
                        m_pendingTransition = LevelTransition::ASCEND;
                    }
                });
        }
    }
}

void BottomScreenUI::inspectCell(int cell, pd::Level* level) {
    if (!level || cell < 0 || cell >= Level::LENGTH) return;

    if (level->map[cell] == Terrain::SIGN) {
        pd::Sign::read(cell);
        return;
    }

    // 1. Check for mob at cell
    for (const auto& m : Dungeon::mobs) {
        if (m && m->isAlive() && m->pos == cell && Dungeon::visible[cell]) {
            std::string fullMobInfo = m->name + "\n";
            fullMobInfo += "Health: " + m->status() + " (" + std::to_string(m->HP) + "/" + std::to_string(m->HT) + ")\n";
            fullMobInfo += "Status: " + m->statePreview() + "\n\n";
            fullMobInfo += m->description();
            showMessage(fullMobInfo);
            return;
        }
    }

    // 2. Check for hero at cell
    if (Dungeon::hero && Dungeon::hero->pos == cell) {
        std::string heroInfo = Dungeon::hero->name + " (" + Dungeon::hero->className() + ")\n";
        heroInfo += "Level " + std::to_string(Dungeon::hero->level) + " (" + std::to_string(Dungeon::hero->exp) + "/" + std::to_string(Dungeon::hero->maxExp) + " EXP)\n";
        heroInfo += "Health: " + std::to_string(Dungeon::hero->HP) + "/" + std::to_string(Dungeon::hero->HT) + "   Strength: " + std::to_string(Dungeon::hero->STR) + "\n\n";
        heroInfo += "A brave adventurer seeking the legendary Amulet of Yendor in the deepest recesses of the dungeon.";
        showMessage(heroInfo);
        return;
    }

    // 3. Check for item heap
    Heap* heap = level->getHeap(cell);
    if (heap && (level->visited[cell] || Dungeon::visible[cell])) {
        if (heap->type == Heap::Type::CHEST) {
            showMessage("Chest\n\nThis wooden chest is unlocked and may hold valuable treasure.");
            return;
        } else if (heap->type == Heap::Type::LOCKED_CHEST) {
            showMessage("Locked Chest\n\nA solid golden lock secures this chest. You need a golden key to open it.");
            return;
        } else if (heap->type == Heap::Type::CRYSTAL_CHEST) {
            showMessage("Crystal Chest\n\nA translucent crystal chest. You can see the prize inside, but you need a golden key.");
            return;
        } else if (heap->type == Heap::Type::TOMB) {
            showMessage("Tomb\n\nAn ancient sarcophagus. Disturbing it may anger the spirits within.");
            return;
        } else if (heap->type == Heap::Type::SKELETON) {
            showMessage("Skeletal Remains\n\nThe bones of an unfortunate adventurer who perished here long ago.");
            return;
        } else if (!heap->isEmpty() && heap->peek()) {
            Item* it = heap->peek();
            std::string itInfo = it->info();
            if (itInfo.empty()) {
                showMessage(it->displayName());
            } else {
                showMessage(it->displayName() + "\n\n" + itInfo);
            }
            return;
        }
    }

    // 3b. Check for active plant on the cell
    Plant* plant = level->getPlant(cell);
    if (plant && (level->visited[cell] || Dungeon::visible[cell])) {
        showMessage(plant->plantName + "\n\n" + plant->desc());
        return;
    }

    // 4. Traps & Specialized Dungeon Terrain
    int tile = level->map[cell];
    switch (tile) {
        case Terrain::TOXIC_TRAP:
            showMessage("Toxic Gas Trap\n\nA hidden trigger connected to canisters of concentrated toxic gas. Stepping on it fills the surrounding area with a deadly green cloud.");
            return;
        case Terrain::FIRE_TRAP:
            showMessage("Fire Trap\n\nA pressure plate rigged with volatile liquid. Stepping on it ignites the tile into violent flames, spreading to any adjacent flammables.");
            return;
        case Terrain::PARALYTIC_TRAP:
            showMessage("Paralytic Gas Trap\n\nA canister trap that releases thick yellow vapors upon triggering. Inhaling the gas completely paralyzes creatures for several turns.");
            return;
        case Terrain::POISON_TRAP:
            showMessage("Poison Dart Trap\n\nConcealed spring-loaded mechanisms that fire venom-tipped darts at whoever steps on the plate.");
            return;
        case Terrain::ALARM_TRAP:
            showMessage("Alarm Trap\n\nA resonant metal chime that rings with an ear-splitting noise when tripped, immediately awakening and alerting all monsters on this level.");
            return;
        case Terrain::LIGHTNING_TRAP:
            showMessage("Lightning Trap\n\nA conductive rune plate that stores static electrical potential. Triggering it discharges a high-voltage arc that chains between nearby targets.");
            return;
        case Terrain::GRIPPING_TRAP:
            showMessage("Gripping Trap\n\nA spring-loaded steel bear trap. Stepping on it painfully clamps shut on the victim's legs, inflicting heavy bleeding and crippling immobilization.");
            return;
        case Terrain::SUMMONING_TRAP:
            showMessage("Summoning Trap\n\nA mystical glyph etched into the stone floor. When activated, it emits a planar beacon, instantly summoning hostile monsters to surround the intruder.");
            return;
        case Terrain::ENTRANCE:
            showMessage("Stairs Up\n\nA flight of stone steps leading upward to the previous floor.");
            return;
        case Terrain::EXIT:
            showMessage("Stairs Down\n\nA stone archway with steps descending into the dark unknown of the next depth.");
            return;
        case Terrain::WATER:
            showMessage("Water\n\nDamp, cool water pools across the floor. Moving through water extinguishes burning and washes away caustic ooze.");
            return;
        case Terrain::DOOR:
            showMessage("Closed Door\n\nA heavy wooden door. Walking into it opens the door, or you can surprise-attack enemies through doorways.");
            return;
        case Terrain::OPEN_DOOR:
            showMessage("Open Door\n\nA wooden door that has been pushed open.");
            return;
        case Terrain::LOCKED_DOOR:
            showMessage("Locked Door\n\nThis sturdy iron-banded door is locked tight. An iron key from this floor is required to unlock it.");
            return;
        case Terrain::HIGH_GRASS:
            showMessage("High Grass\n\nThick, tall vegetation that blocks line of sight. Trampling high grass may yield dew drops or seeds.");
            return;
        case Terrain::STATUE:
            showMessage("Statue\n\nA menacing carved stone guardian. Defeating an animated statue will reward you with the weapon it wields.");
            return;
        case Terrain::ALCHEMY:
            showMessage("Alchemy Pot\n\nA large iron cauldron bubbling with alchemical broth. Throwing three seeds into the pot brews a random potion.");
            return;
        case Terrain::CHASM:
            showMessage("Chasm\n\nA bottomless abyss. Leaping into the chasm will cause you to fall into the next floor, sustaining severe injury.");
            return;
        case Terrain::BARRICADE:
            showMessage("Wood Barricade\n\nA sturdy wooden barricade blocking the passage. It can be burned down with fire (Liquid Flame potion or Wand of Firebolt).");
            return;
        case Terrain::BOOKSHELF:
            showMessage("Bookshelf\n\nA wooden shelf holding old tomes and scrolls. Highly flammable.");
            return;
        default:
            break;
    }

    std::string tName = Level::tileName(tile);
    std::string tDesc = Level::tileDesc(tile);

    // Check for active blobs on the cell (Fire, Toxic Gas, Paralytic Gas, etc.)
    static const Blob::BlobType blobTypes[] = {
        Blob::BlobType::FIRE,
        Blob::BlobType::TOXIC_GAS,
        Blob::BlobType::PARALYTIC_GAS,
        Blob::BlobType::CONFUSION_GAS,
        Blob::BlobType::CORROSIVE_GAS,
        Blob::BlobType::REGROWTH,
        Blob::BlobType::FOLIAGE
    };
    for (auto bt : blobTypes) {
        Blob* b = FX::getBlob(bt);
        if (b && b->cur[cell] > 0 && Dungeon::visible[cell]) {
            const char* bd = b->tileDesc();
            if (bd) {
                if (!tDesc.empty()) tDesc += "\n\n";
                tDesc += bd;
            }
        }
    }

    if (tDesc.empty()) {
        log(tName, C2D_Color32(0xff, 0xcc, 0x66, 0xFF));
    } else {
        showMessage(tName + "\n\n" + tDesc);
    }
}

} // namespace pd::ui
