#pragma once

#include <algorithm>

namespace pd {

struct Point {
    int x = 0;
    int y = 0;

    Point() = default;
    Point(int px, int py) : x(px), y(py) {}

    Point& set(int px, int py) { x = px; y = py; return *this; }
    Point& offset(int dx, int dy) { x += dx; y += dy; return *this; }
    Point& offset(const Point& o) { x += o.x; y += o.y; return *this; }

    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

struct Rect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    Rect() = default;
    Rect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}

    int width() const { return right - left; }
    int height() const { return bottom - top; }
    int square() const { return width() * height(); }

    Rect& set(int l, int t, int r, int b) {
        left = l; top = t; right = r; bottom = b;
        return *this;
    }
    Rect& set(const Rect& o) {
        left = o.left; top = o.top; right = o.right; bottom = o.bottom;
        return *this;
    }

    Rect intersect(const Rect& o) const {
        return Rect(
            std::max(left, o.left),
            std::max(top, o.top),
            std::min(right, o.right),
            std::min(bottom, o.bottom)
        );
    }

    bool inside(int x, int y) const {
        return x > left && x < right && y > top && y < bottom;
    }

    bool contains(int x, int y) const {
        return x >= left && x <= right && y >= top && y <= bottom;
    }
};

} // namespace pd

