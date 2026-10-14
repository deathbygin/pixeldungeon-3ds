#include "pd/ui/UIButton.h"

namespace pd::ui {

bool UIButton::update(const TouchController& touch, u32 kDown) {
    if (!enabled) return false;

    bool triggered = false;

    if (hardwareKey != 0 && (kDown & hardwareKey)) {
        triggered = true;
    } else if (touch.clicked(bounds())) {
        triggered = true;
    }

    if (triggered) {
        if (onClick) {
            onClick();
        }
        return true;
    }
    return false;
}

void UIButton::draw(C2D_TextBuf textBuf) const {
    if (!enabled) return;

    u32 fillColor = customFill != 0 ? customFill : (active ? C2D_Color32(0x76, 0x42, 0x8a, 0xFF) : C2D_Color32(0x32, 0x2b, 0x44, 0xFF));
    u32 borderColor = customBorder != 0 ? customBorder : (active ? C2D_Color32(0xe4, 0xa6, 0x72, 0xFF) : C2D_Color32(0x63, 0x48, 0x3d, 0xFF));

    // Button fill
    C2D_DrawRectSolid(x, y, z, width, height, fillColor);

    // 1px Border
    C2D_DrawRectSolid(x, y, z + 0.02f, width, 1.0f, borderColor);
    C2D_DrawRectSolid(x, y + height - 1.0f, z + 0.02f, width, 1.0f, borderColor);
    C2D_DrawRectSolid(x, y, z + 0.02f, 1.0f, height, borderColor);
    C2D_DrawRectSolid(x + width - 1.0f, y, z + 0.02f, 1.0f, height, borderColor);

    // Text label
    C2D_Text textObj;
    std::string fullText = label;
    if (!shortcut.empty()) {
        fullText += " " + shortcut;
    }

    C2D_TextParse(&textObj, textBuf, fullText.c_str());
    C2D_TextOptimize(&textObj);

    float textScale = 0.38f;
    float textX = x + 6.0f;
    float textY = y + (height - 12.0f) * 0.5f;

    u32 textColor = active ? C2D_Color32(0xe4, 0xa6, 0x72, 0xFF) : C2D_Color32(0xff, 0xff, 0xff, 0xFF);
    C2D_DrawText(&textObj, C2D_WithColor, textX, textY, z + 0.05f, textScale, textScale, textColor);
}

} // namespace pd::ui

