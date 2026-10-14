#pragma once

#include <citro2d.h>
#include <string>
#include <functional>
#include "noosa/Types.h"
#include "pd/ui/TouchController.h"

namespace pd::ui {

class UIButton {
public:
    using ClickCallback = std::function<void()>;

    float x = 0.0f;
    float y = 0.0f;
    float width = 64.0f;
    float height = 24.0f;

    std::string label;
    std::string shortcut;
    u32 hardwareKey = 0; // e.g. KEY_A, KEY_B, etc.
    float z = 0.4f;
    bool enabled = true;
    bool active = false; // toggle/highlight state
    u32 customFill = 0;
    u32 customBorder = 0;

    ClickCallback onClick = nullptr;

    UIButton() = default;
    UIButton(float px, float py, float pw, float ph,
             std::string lbl, std::string sc = "", u32 hwKey = 0,
             ClickCallback cb = nullptr)
        : x(px), y(py), width(pw), height(ph),
          label(std::move(lbl)), shortcut(std::move(sc)),
          hardwareKey(hwKey), onClick(std::move(cb)) {}

    noosa::RectF bounds() const {
        return noosa::RectF(x, y, width, height);
    }

    bool update(const TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf) const;
};

} // namespace pd::ui

