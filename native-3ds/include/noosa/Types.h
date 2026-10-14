#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <cstdint>
#include <algorithm>

namespace noosa {

enum class ScreenTarget : uint8_t {
    Top = 0,
    Bottom = 1
};

struct Point {
    int x = 0;
    int y = 0;

    constexpr Point() = default;
    constexpr Point(int px, int py) : x(px), y(py) {}
};

struct PointF {
    float x = 0.0f;
    float y = 0.0f;

    constexpr PointF() = default;
    constexpr PointF(float px, float py) : x(px), y(py) {}

    constexpr PointF operator+(const PointF& o) const { return PointF(x + o.x, y + o.y); }
    constexpr PointF operator-(const PointF& o) const { return PointF(x - o.x, y - o.y); }
    constexpr PointF operator*(float s) const { return PointF(x * s, y * s); }
    PointF& operator+=(const PointF& o) { x += o.x; y += o.y; return *this; }
    PointF& operator-=(const PointF& o) { x -= o.x; y -= o.y; return *this; }
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr Rect() = default;
    constexpr Rect(int rx, int ry, int rw, int rh) : x(rx), y(ry), width(rw), height(rh) {}

    constexpr int right() const { return x + width; }
    constexpr int bottom() const { return y + height; }
    constexpr bool contains(int px, int py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }
};

struct RectF {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    constexpr RectF() = default;
    constexpr RectF(float rx, float ry, float rw, float rh) : x(rx), y(ry), width(rw), height(rh) {}

    constexpr float right() const { return x + width; }
    constexpr float bottom() const { return y + height; }
    constexpr bool contains(float px, float py) const {
        return px >= x && px <= right() && py >= y && py <= bottom();
    }
};

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t pr, uint8_t pg, uint8_t pb, uint8_t pa = 255)
        : r(pr), g(pg), b(pb), a(pa) {}

    constexpr uint32_t toU32() const {
        return C2D_Color32(r, g, b, a);
    }

    static constexpr Color fromHex(uint32_t hex, uint8_t alpha = 255) {
        return Color(
            static_cast<uint8_t>((hex >> 16) & 0xFF),
            static_cast<uint8_t>((hex >> 8) & 0xFF),
            static_cast<uint8_t>(hex & 0xFF),
            alpha
        );
    }
};

} // namespace noosa

