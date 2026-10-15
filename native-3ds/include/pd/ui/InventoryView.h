#pragma once

#include <citro2d.h>
#include <vector>
#include <string>
#include "noosa/Types.h"
#include "pd/ui/TouchController.h"
#include "pd/ui/UIButton.h"

namespace pd {
class Hero;
class Item;
}

namespace pd::ui {

class InventoryView {
public:
    enum class Mode {
        NORMAL,
        SELECT_UPGRADE,
        SELECT_IDENTIFY
    };

    bool isOpen = false;
    Mode mode = Mode::NORMAL;

    InventoryView();
    ~InventoryView() = default;

    void open();
    void openUpgrade(int scrollIdx);
    void openIdentify(int scrollIdx);
    void close();

    // Returns true if an item action was taken
    bool update(pd::Hero* hero, const TouchController& touch, u32 kDown, std::string& outMessage);
    void draw(const pd::Hero* hero, C2D_TextBuf textBuf) const;

    int getSelectedIndex() const { return m_selectedIndex; }

private:
    int m_selectedIndex = 0;
    int m_targetScrollIdx = -1;

    mutable std::vector<UIButton> m_actionButtons;
    mutable UIButton m_btnClose;

    static constexpr int GRID_COLS = 4;
    static constexpr int GRID_ROWS = 5;
    static constexpr float SLOT_SIZE = 34.0f;
    static constexpr float GRID_START_X = 10.0f;
    static constexpr float GRID_START_Y = 30.0f;
};

} // namespace pd::ui
