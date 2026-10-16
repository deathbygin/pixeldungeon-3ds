#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <functional>
#include "pd/ui/TouchController.h"

namespace pd::ui {

/**
 * Authentic Pixel Dungeon stone-bordered message modal window (WndMessage).
 * Displays word-wrapped narrative, tips, and modal notices centered on the Bottom Screen.
 * Dismissable via touch or hardware buttons (A, B, X, Y, START, SELECT).
 */
class WndMessage {
public:
    using DismissCallback = std::function<void()>;

    WndMessage() = default;
    explicit WndMessage(std::string text, DismissCallback onDismiss = nullptr);

    void show(std::string text, DismissCallback onDismiss = nullptr);
    void hide();

    bool isOpen() const { return m_open; }

    bool update(const TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf, float baseDepth = 0.82f);

private:
    bool m_open = false;
    std::string m_text;
    std::vector<std::string> m_lines;
    DismissCallback m_callback = nullptr;

    float m_x = 35.0f;
    float m_y = 60.0f;
    float m_w = 250.0f;
    float m_h = 120.0f;
};

} // namespace pd::ui

