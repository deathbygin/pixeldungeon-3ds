#pragma once

#include "Types.h"
#include <algorithm>

namespace noosa {

class Visual;

class Camera {
public:
    static Camera* main;

    static constexpr float TOP_SCREEN_WIDTH  = 400.0f;
    static constexpr float TOP_SCREEN_HEIGHT = 240.0f;

    Camera();
    ~Camera();

    // Viewport configuration
    void setViewport(float w, float h);
    float viewportWidth() const { return m_viewportW; }
    float viewportHeight() const { return m_viewportH; }

    // Zoom factor (e.g. 1.0f = 1:1, 2.0f = 2x pixel magnification)
    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }

    // Camera center in world space
    void setCenter(float x, float y);
    PointF getCenter() const { return m_scroll; }

    // Follow a visual target (e.g. Hero)
    void follow(Visual* target, float speed = 1.0f);
    void unfollow();

    // Screen shake effect
    void shake(float duration, float magnitude);

    // Update camera follow & screen shake per frame
    void update(float dt);

    // Coordinate conversion
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void screenToWorld(float sx, float sy, float& wx, float& wy) const;

    // World boundary limits
    void setBounds(float minX, float minY, float maxX, float maxY);
    void clearBounds();

    // Check if an AABB in world space is visible within the camera's viewport
    bool isVisible(float wx, float wy, float w, float h) const;
    void getVisibleWorldBounds(float& left, float& top, float& right, float& bottom) const;

private:
    float m_viewportW = TOP_SCREEN_WIDTH;
    float m_viewportH = TOP_SCREEN_HEIGHT;
    float m_zoom = 1.0f;

    PointF m_scroll{ 0.0f, 0.0f };
    Visual* m_target = nullptr;
    float m_followSpeed = 1.0f;

    // Shaking
    float m_shakeDuration = 0.0f;
    float m_shakeMagnitude = 0.0f;
    PointF m_shakeOffset{ 0.0f, 0.0f };

    // Bounds
    bool m_hasBounds = false;
    float m_minX = 0.0f;
    float m_minY = 0.0f;
    float m_maxX = 0.0f;
    float m_maxY = 0.0f;
};

} // namespace noosa

