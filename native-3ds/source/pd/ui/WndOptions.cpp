#include "pd/ui/WndOptions.h"
#include <algorithm>

namespace pd::ui {

WndOptions::WndOptions(std::string title, std::string message,
                       std::vector<std::string> options,
                       OptionCallback onSelect) {
    show(std::move(title), std::move(message), std::move(options), std::move(onSelect));
}

void WndOptions::show(std::string title, std::string message,
                      std::vector<std::string> options,
                      OptionCallback onSelect) {
    m_title = std::move(title);
    m_message = std::move(message);
    m_options = std::move(options);
    m_callback = std::move(onSelect);
    m_selected = 0;
    m_open = true;

    // Recalculate layout
    m_w = 280.0f;
    m_x = (320.0f - m_w) * 0.5f;

    float btnH = 24.0f;
    float btnSpacing = 6.0f;
    float totalBtnH = m_options.size() * (btnH + btnSpacing);
    m_h = 75.0f + totalBtnH;
    m_y = (240.0f - m_h) * 0.5f;

    m_buttonLayouts.clear();
    float curY = m_y + m_h - totalBtnH - 8.0f;
    for (size_t i = 0; i < m_options.size(); ++i) {
        m_buttonLayouts.push_back({ m_x + 16.0f, curY, m_w - 32.0f, btnH });
        curY += btnH + btnSpacing;
    }
}

void WndOptions::hide() {
    m_open = false;
}

bool WndOptions::update(const TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Hardware D-pad up/down navigation
    if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
        if (m_selected > 0) m_selected--;
        return true;
    }
    if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        if (m_selected + 1 < static_cast<int>(m_options.size())) m_selected++;
        return true;
    }

    // Hardware confirmation / cancel
    if (kDown & KEY_A) {
        int choice = m_selected;
        hide();
        if (m_callback) m_callback(choice);
        return true;
    }
    if (kDown & KEY_B) {
        // Option index 1 (usually Cancel / No) or last option
        int cancelIdx = (m_options.size() > 1) ? 1 : 0;
        hide();
        if (m_callback) m_callback(cancelIdx);
        return true;
    }

    // Stylus Touch handling
    for (size_t i = 0; i < m_buttonLayouts.size(); ++i) {
        const auto& b = m_buttonLayouts[i];
        noosa::RectF r(b.x, b.y, b.w, b.h);
        if (touch.hitTest(r)) {
            m_selected = static_cast<int>(i);
        }
        if (touch.clicked(r)) {
            hide();
            if (m_callback) m_callback(static_cast<int>(i));
            return true;
        }
    }

    return true; // Absorb all input while modal is active
}

void WndOptions::draw(C2D_TextBuf textBuf) {
    if (!m_open) return;

    // 1. Darken background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, 0.80f, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xAA));

    // 2. Dialog box background (matching Pixel Dungeon's WndOptions style)
    u32 boxBg = C2D_Color32(0x22, 0x1d, 0x2a, 0xFA);
    u32 borderCol = C2D_Color32(0x5a, 0x47, 0x41, 0xFF);

    C2D_DrawRectSolid(m_x, m_y, 0.82f, m_w, m_h, boxBg);

    // Frame borders
    C2D_DrawRectSolid(m_x, m_y, 0.83f, m_w, 2.0f, borderCol);
    C2D_DrawRectSolid(m_x, m_y + m_h - 2.0f, 0.83f, m_w, 2.0f, borderCol);
    C2D_DrawRectSolid(m_x, m_y, 0.83f, 2.0f, m_h, borderCol);
    C2D_DrawRectSolid(m_x + m_w - 2.0f, m_y, 0.83f, 2.0f, m_h, borderCol);

    // 3. Title text (Golden yellow title color)
    C2D_Text titleObj;
    C2D_TextParse(&titleObj, textBuf, m_title.c_str());
    C2D_TextOptimize(&titleObj);
    float tW = 0.0f, tH = 0.0f;
    C2D_TextGetDimensions(&titleObj, 0.42f, 0.42f, &tW, &tH);
    float titleX = m_x + (m_w - tW) * 0.5f;
    float titleY = m_y + 12.0f;
    C2D_DrawText(&titleObj, C2D_WithColor, titleX, titleY, 0.85f, 0.42f, 0.42f, C2D_Color32(0xff, 0xcc, 0x44, 0xFF));

    // 4. Message text
    if (!m_message.empty()) {
        C2D_Text msgObj;
        C2D_TextParse(&msgObj, textBuf, m_message.c_str());
        C2D_TextOptimize(&msgObj);
        float mW = 0.0f, mH = 0.0f;
        C2D_TextGetDimensions(&msgObj, 0.36f, 0.36f, &mW, &mH);
        float msgX = m_x + (m_w - mW) * 0.5f;
        float msgY = titleY + tH + 6.0f;
        C2D_DrawText(&msgObj, C2D_WithColor, msgX, msgY, 0.85f, 0.36f, 0.36f, C2D_Color32(0xdd, 0xdd, 0xdd, 0xFF));
    }

    // 5. Option buttons (Red buttons matching original RedButton)
    for (size_t i = 0; i < m_buttonLayouts.size(); ++i) {
        const auto& b = m_buttonLayouts[i];
        bool isSel = (m_selected == static_cast<int>(i));

        u32 btnBg = isSel ? C2D_Color32(0xa6, 0x30, 0x30, 0xFF) : C2D_Color32(0x6e, 0x22, 0x22, 0xFF);
        u32 btnBorder = isSel ? C2D_Color32(0xff, 0xaa, 0xaa, 0xFF) : C2D_Color32(0x3e, 0x12, 0x12, 0xFF);

        C2D_DrawRectSolid(b.x, b.y, 0.85f, b.w, b.h, btnBg);
        C2D_DrawRectSolid(b.x, b.y, 0.86f, b.w, 1.0f, btnBorder);
        C2D_DrawRectSolid(b.x, b.y + b.h - 1.0f, 0.86f, b.w, 1.0f, btnBorder);
        C2D_DrawRectSolid(b.x, b.y, 0.86f, 1.0f, b.h, btnBorder);
        C2D_DrawRectSolid(b.x + b.w - 1.0f, b.y, 0.86f, 1.0f, b.h, btnBorder);

        C2D_Text optObj;
        C2D_TextParse(&optObj, textBuf, m_options[i].c_str());
        C2D_TextOptimize(&optObj);
        float oW = 0.0f, oH = 0.0f;
        C2D_TextGetDimensions(&optObj, 0.38f, 0.38f, &oW, &oH);
        float optX = b.x + (b.w - oW) * 0.5f;
        float optY = b.y + (b.h - oH) * 0.5f;

        u32 optCol = isSel ? C2D_Color32(0xff, 0xff, 0x88, 0xFF) : C2D_Color32(0xca, 0xcf, 0xc2, 0xFF);
        C2D_DrawText(&optObj, C2D_WithColor, optX, optY, 0.88f, 0.38f, 0.38f, optCol);
    }
}

} // namespace pd::ui

