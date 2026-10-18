#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <memory>
#include "pd/HeroSubClass.h"
#include "pd/ui/TouchController.h"

namespace pd {
class Hero;
class TomeOfMastery;
}

namespace pd::windows {

class WndChooseSubclass {
public:
    WndChooseSubclass();
    ~WndChooseSubclass() = default;

    void show(TomeOfMastery* tome, Hero* hero);
    void hide() { m_open = false; }
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    TomeOfMastery* m_tome = nullptr;
    Hero* m_hero = nullptr;

    HeroSubClass m_sc1 = HeroSubClass::None;
    HeroSubClass m_sc2 = HeroSubClass::None;
    bool m_isRespec = false;

    // Focused element index: 0 = Branch 1, 1 = Branch 2, 2 = Cancel
    int m_focusIndex = 0;

    // Confirmation modal state
    bool m_confirming = false;
    HeroSubClass m_pendingChoice = HeroSubClass::None;
    int m_confirmFocus = 0; // 0 = Yes, 1 = No

    float m_x = 12.0f;
    float m_y = 10.0f;
    float m_w = 296.0f;
    float m_h = 220.0f;

    struct CardLayout {
        float x;
        float y;
        float w;
        float h;
        float btnX;
        float btnY;
        float btnW;
        float btnH;
        std::vector<std::string> descLines;
    };

    CardLayout m_card1;
    CardLayout m_card2;
    float m_cancelBtnX = 0.0f;
    float m_cancelBtnY = 0.0f;
    float m_cancelBtnW = 0.0f;
    float m_cancelBtnH = 0.0f;

    void setupLayout();
    void confirmSelection(HeroSubClass way);
    void applySelection(HeroSubClass way);
    std::vector<std::string> wrapText(const std::string& text, float maxW);
};

} // namespace pd::windows

