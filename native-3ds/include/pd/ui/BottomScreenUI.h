#pragma once

#include <citro2d.h>
#include <string>
#include <memory>
#include "pd/ui/TouchController.h"
#include "pd/ui/UIButton.h"
#include "pd/ui/StatusBar.h"
#include "pd/ui/LogConsole.h"
#include "pd/ui/InventoryView.h"

namespace pd {
class Hero;
class Level;
}

namespace pd::ui {

class BottomScreenUI {
public:
    BottomScreenUI();
    ~BottomScreenUI() = default;

    // Updates touch and button input, executes hero actions if triggered
    // Returns true if a turn-consuming action occurred
    bool update(pd::Hero* hero, pd::Level* level, u32 kDown, u32 kHeld, u32 kUp);

    // Renders complete bottom screen UI
    void draw(const pd::Hero* hero, int depth, C2D_TextBuf textBuf);

    // Log message helper
    void log(std::string msg, u32 color = C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));

    bool isInventoryOpen() const { return m_inventoryView.isOpen; }
    void toggleInventory();

    const TouchController& getTouchController() const { return m_touch; }

private:
    TouchController m_touch;
    StatusBar m_statusBar;
    LogConsole m_logConsole;
    InventoryView m_inventoryView;

    // Action button strip
    UIButton m_btnWait;
    UIButton m_btnSearch;
    UIButton m_btnInventory;
    UIButton m_btnStairs;

    // Quickslot buttons
    UIButton m_btnQuick1;
    UIButton m_btnQuick2;
};

} // namespace pd::ui

