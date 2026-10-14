#include "noosa/Camera.h"
#include "noosa/Visual.h"
#include <cmath>
#include <cstdlib>

namespace noosa {

Camera* Camera::main = nullptr;

Camera::Camera() {
    if (!main) main = this;
    setViewport(TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT);
}

Camera::~Camera() {
    if (main == this) main = nullptr;
}

void Camera::setViewport(float w, float h) {
    m_viewportW = (w > 0.0f) ? w : TOP_SCREEN_WIDTH;
    m_viewportH = (h > 0.0f) ? h : TOP_SCREEN_HEIGHT;
}

void Camera::setZoom(float zoom) {
    m_zoom = (zoom > 0.05f) ? zoom : 1.0f;
}

void Camera::setCenter(float x, float y) {
    m_scroll.x = x;
    m_scroll.y = y;

    if (m_hasBounds) {
        float halfW = (m_viewportW * 0.5f) / m_zoom;
        float halfH = (m_viewportH * 0.5f) / m_zoom;
        if (m_scroll.x - halfW < m_minX) m_scroll.x = m_minX + halfW;
        if (m_scroll.x + halfW > m_maxX) m_scroll.x = m_maxX - halfW;
        if (m_scroll.y - halfH < m_minY) m_scroll.y = m_minY + halfH;
        if (m_scroll.y + halfH > m_maxY) m_scroll.y = m_maxY - halfH;
    }
}

void Camera::follow(Visual* target, float speed) {
    m_target = target;
    m_followSpeed = std::max(0.01f, std::min(1.0f, speed));
}

void Camera::unfollow() {
    m_target = nullptr;
}

void Camera::shake(float duration, float magnitude) {
    m_shakeDuration = duration;
    m_shakeMagnitude = magnitude;
}

void Camera::update(float dt) {
    // 1. Target tracking
    if (m_target) {
        float targetCenterX = m_target->x + (m_target->width * 0.5f);
        float targetCenterY = m_target->y + (m_target->height * 0.5f);

        if (m_followSpeed >= 1.0f) {
            setCenter(targetCenterX, targetCenterY);
        } else {
            float lerpX = m_scroll.x + (targetCenterX - m_scroll.x) * m_followSpeed;
            float lerpY = m_scroll.y + (targetCenterY - m_scroll.y) * m_followSpeed;
            setCenter(lerpX, lerpY);
        }
    }

    // 2. Shake update
    if (m_shakeDuration > 0.0f) {
        m_shakeDuration -= dt;
        if (m_shakeDuration <= 0.0f) {
            m_shakeOffset = { 0.0f, 0.0f };
        } else {
            float rx = ((std::rand() % 200) - 100) / 100.0f;
            float ry = ((std::rand() % 200) - 100) / 100.0f;
            m_shakeOffset.x = rx * m_shakeMagnitude;
            m_shakeOffset.y = ry * m_shakeMagnitude;
        }
    } else {
        m_shakeOffset = { 0.0f, 0.0f };
    }
}

void Camera::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    float effectiveCenterX = m_scroll.x + m_shakeOffset.x;
    float effectiveCenterY = m_scroll.y + m_shakeOffset.y;

    sx = ((wx - effectiveCenterX) * m_zoom) + (m_viewportW * 0.5f);
    sy = ((wy - effectiveCenterY) * m_zoom) + (m_viewportH * 0.5f);
}

void Camera::screenToWorld(float sx, float sy, float& wx, float& wy) const {
    float effectiveCenterX = m_scroll.x + m_shakeOffset.x;
    float effectiveCenterY = m_scroll.y + m_shakeOffset.y;

    wx = ((sx - (m_viewportW * 0.5f)) / m_zoom) + effectiveCenterX;
    wy = ((sy - (m_viewportH * 0.5f)) / m_zoom) + effectiveCenterY;
}

void Camera::setBounds(float minX, float minY, float maxX, float maxY) {
    m_hasBounds = true;
    m_minX = minX;
    m_minY = minY;
    m_maxX = maxX;
    m_maxY = maxY;
}

void Camera::clearBounds() {
    m_hasBounds = false;
}

bool Camera::isVisible(float wx, float wy, float w, float h) const {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    float sw = w * m_zoom;
    float sh = h * m_zoom;

    return (sx + sw >= 0.0f && sx <= m_viewportW &&
            sy + sh >= 0.0f && sy <= m_viewportH);
}

void Camera::getVisibleWorldBounds(float& left, float& top, float& right, float& bottom) const {
    screenToWorld(0.0f, 0.0f, left, top);
    screenToWorld(m_viewportW, m_viewportH, right, bottom);
}

} // namespace noosa

