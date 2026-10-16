#pragma once

#include <citro2d.h>
#include <string>
#include <memory>
#include "pd/ui/TouchController.h"
#include "pd/ui/StatusPane.h"
#include "pd/ui/Toolbar.h"
#include "pd/ui/WndBag.h"
#include "pd/ui/LogConsole.h"
#include "pd/windows/WndHero.h"
#include "pd/windows/WndGame.h"
#include "pd/windows/WndSettings.h"
#include "pd/windows/WndDebugMenu.h"
#include "pd/windows/WndItemSpawner.h"
#include "pd/ui/WndOptions.h"
#include "pd/ui/WndMessage.h"

namespace pd {
class Hero;
class Level;
class Item;
}

#include "pd/LevelTransition.h"

namespace pd::ui {

using pd::LevelTransition;

class BottomScreenUI {
public:
    BottomScreenUI();
    ~BottomScreenUI();

    static BottomScreenUI* instance() { return s_instance; }
    void selectItem(WndBag::Mode mode, const std::string& title, std::function<void(Item*)> onSelect);

    BottomScreenUI(BottomScreenUI&& other) noexcept;
    BottomScreenUI& operator=(BottomScreenUI&& other) noexcept;
    BottomScreenUI(const BottomScreenUI&) = delete;
    BottomScreenUI& operator=(const BottomScreenUI&) = delete;

    void reset();
    void initLogger();

    // Updates touch and button input, executes hero actions if triggered
    // Returns true if a turn-consuming action occurred
    bool update(pd::Hero* hero, pd::Level* level, u32 kDown, u32 kHeld, u32 kUp);

    // Renders complete bottom screen UI
    void draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf);

    // Log message helper
    void log(std::string msg, u32 color = C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));

    bool isInventoryOpen() const { return m_wndBag.isOpen; }
    void toggleInventory();

    bool isWndHeroOpen() const { return m_wndHero.isOpen(); }
    void toggleWndHero();

    bool isPauseMenuOpen() const { return m_wndGame.isOpen() || m_wndSettings.isOpen(); }
    void openPauseMenu();
    void closePauseMenu();

    bool isDebugMenuOpen() const { return m_wndDebugMenu.isOpen() || m_wndItemSpawner.isOpen(); }
    void openDebugMenu();
    void closeDebugMenu();
    void openItemSpawner();
    void closeItemSpawner();

    bool isModalOpen() const {
        return m_wndOptions.isOpen() || m_wndMessage.isOpen() ||
               m_wndBag.isOpen || m_wndHero.isOpen() ||
               m_wndGame.isOpen() || m_wndSettings.isOpen() ||
               m_wndDebugMenu.isOpen() || m_wndItemSpawner.isOpen();
    }

    pd::windows::GameAction getPendingGameAction() const { return m_pendingGameAction; }
    void clearPendingGameAction() { m_pendingGameAction = pd::windows::GameAction::NONE; }

    LevelTransition getPendingTransition() const { return m_pendingTransition; }
    void clearPendingTransition() { m_pendingTransition = LevelTransition::NONE; }

    void promptStairs(bool descend);
    void showMessage(std::string text, std::function<void()> onDismiss = nullptr);
    void showOptions(std::string title, std::string message,
                     std::vector<std::string> options,
                     std::function<void(int)> onSelect);
    void inspectCell(int cell, pd::Level* level);

    // Top-Screen Examine mode
    bool isExamineMode() const { return m_isExamining; }
    void startExamine(int startCell = -1);
    void cancelExamine();
    int getExamineCell() const { return m_examineCell; }
    std::string getHoverPreviewText(int cell, const pd::Level* level) const;

    // Ranged targeting mode
    bool isTargeting() const { return m_isTargeting; }
    void startTargeting(Item* item);
    void cancelTargeting();
    int getTargetCell() const { return m_targetCell; }
    Item* getTargetItem() const { return m_targetItem; }

    const TouchController& getTouchController() const { return m_touch; }

private:
    TouchController m_touch;
    StatusPane m_statusPane;
    Toolbar m_toolbar;
    WndBag m_wndBag;
    LogConsole m_logConsole;
    pd::windows::WndHero m_wndHero;
    pd::windows::WndGame m_wndGame;
    pd::windows::WndSettings m_wndSettings;
    pd::windows::WndDebugMenu m_wndDebugMenu;
    pd::windows::WndItemSpawner m_wndItemSpawner;
    WndOptions m_wndOptions;
    WndMessage m_wndMessage;
    pd::windows::GameAction m_pendingGameAction = pd::windows::GameAction::NONE;
    LevelTransition m_pendingTransition = LevelTransition::NONE;
    bool m_isInspecting = false;

    // Top-Screen Examine State
    bool m_isExamining = false;
    int m_examineCell = 0;
    int m_examineHoldTimer = 0;
    int m_examineLastDirX = 0;
    int m_examineLastDirY = 0;

    // Ranged Targeting State
    bool m_isTargeting = false;
    int m_targetCell = 0;
    Item* m_targetItem = nullptr;
    int m_targetHoldTimer = 0;
    int m_targetLastDirX = 0;
    int m_targetLastDirY = 0;

    // Minimap / Central View Mode
    bool m_showMinimap = true;

    void drawMinimap(const pd::Level* level, const pd::Hero* hero) const;
    void drawRightGlanceCard(const pd::Hero* hero, C2D_TextBuf textBuf) const;
    void updateQuickslots(pd::Hero* hero);

    static BottomScreenUI* s_instance;
};

} // namespace pd::ui
