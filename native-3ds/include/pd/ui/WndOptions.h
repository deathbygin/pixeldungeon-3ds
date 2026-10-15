#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <functional>
#include "pd/ui/TouchController.h"

namespace pd::ui {

/**
 * Clean 3DS modal dialog matching Pixel Dungeon's WndOptions.
 * Uses chrome.png 9-patch window styling, displays a title, message, and choice buttons.
 */
class WndOptions {
public:
    using OptionCallback = std::function<void(int index)>;

    WndOptions() = default;
    WndOptions(std::string title, std::string message,
               std::vector<std::string> options,
               OptionCallback onSelect = nullptr);

    void show(std::string title, std::string message,
              std::vector<std::string> options,
              OptionCallback onSelect = nullptr);
    void hide();

    bool isOpen() const { return m_open; }

    bool update(const TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    std::string m_title;
    std::string m_message;
    std::vector<std::string> m_messageLines;
    std::vector<std::string> m_options;
    OptionCallback m_callback = nullptr;

    int m_selected = 0; // Focus index for hardware D-pad/Circle pad

    // Modal layout
    float m_x = 20.0f;
    float m_y = 35.0f;
    float m_w = 280.0f;
    float m_h = 170.0f;

    struct ButtonLayout {
        float x;
        float y;
        float w;
        float h;
    };
    std::vector<ButtonLayout> m_buttonLayouts;
};

} // namespace pd::ui

