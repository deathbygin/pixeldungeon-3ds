#include "pd/MissileSprite.h"
#include "pd/Level.h"
#include "pd/ItemSpriteSheet.h"
#include "noosa/Camera.h"
#include <cmath>
#include <algorithm>

namespace pd {

std::shared_ptr<noosa::TextureFilm> MissileSprite::s_itemsFilm = nullptr;

void MissileSprite::initFilm(std::shared_ptr<noosa::TextureFilm> film) {
    s_itemsFilm = std::move(film);
}

MissileSprite::MissileSprite() {
    active = false;
    visible = false;
    z = 0.70f; // Render above characters (0.60f) and fog of war (0.65f)
    origin = noosa::PointF(8.0f, 8.0f); // Center origin for 16x16 sprite rotation
    targetScreen = noosa::ScreenTarget::Top;
}

void MissileSprite::reset(int fromCell, int toCell, int itemImage, std::function<void()> callback) {
    if (!s_itemsFilm) {
        auto tex = noosa::Texture::get(Assets::ITEMS);
        if (tex && tex->isLoaded()) {
            s_itemsFilm = std::make_shared<noosa::TextureFilm>(tex, 16, 16);
        }
    }

    if (s_itemsFilm) {
        setFilm(s_itemsFilm, itemImage);
    }

    // World tile coordinates (center of source and destination tiles)
    startX = (fromCell % Level::WIDTH) * 16.0f + 8.0f;
    startY = (fromCell / Level::WIDTH) * 16.0f + 8.0f;
    targetX = (toCell % Level::WIDTH) * 16.0f + 8.0f;
    targetY = (toCell / Level::WIDTH) * 16.0f + 8.0f;

    float dx = targetX - startX;
    float dy = targetY - startY;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Minimum flight duration of 0.16s (~10 frames at 60 FPS) ensures throw animation is clearly visible
    duration = std::max(0.16f, dist / SPEED);
    elapsed = 0.0f;
    onComplete = std::move(callback);

    // Matching Java Pixel Dungeon MissileSprite:
    // Darts, Javelins face flight vector (orientable) with +45 deg offset aligning sprite tip to trajectory
    if (itemImage == ItemSpriteSheet::DART ||
        itemImage == ItemSpriteSheet::INCENDIARY_DART ||
        itemImage == ItemSpriteSheet::CURARE_DART ||
        itemImage == ItemSpriteSheet::JAVELIN) {
        angularSpeed = 0.0f;
        angle = std::atan2(dy, dx) + 0.785398f; // +45 degrees in radians
    } else if (itemImage == ItemSpriteSheet::SHURIKEN || itemImage == ItemSpriteSheet::BOOMERANG) {
        angularSpeed = 25.1327f; // ~1440 deg/s in radians
        angle = 0.0f;
    } else {
        angularSpeed = 12.5664f; // ~720 deg/s in radians
        angle = 0.0f;
    }

    // Dynamic vertical arc height scaled by throw distance
    heightArc = std::clamp(dist * 0.35f, 8.0f, 32.0f);

    x = startX;
    y = startY;
    active = true;
    visible = true;
    alive = true;
}

void MissileSprite::update(float dt) {
    if (!active) return;

    elapsed += dt;
    float t = elapsed / duration;

    if (t >= 1.0f) {
        t = 1.0f;
        x = targetX;
        y = targetY;
        active = false;
        visible = false;

        if (onComplete) {
            auto cb = std::move(onComplete);
            onComplete = nullptr;
            cb();
        }
        return;
    }

    // Linear horizontal/vertical interpolation along the ground line
    float curX = startX + (targetX - startX) * t;
    float curY = startY + (targetY - startY) * t;

    // Parabolic vertical arc offset: -height * 4 * t * (1 - t)
    float arcOffset = -heightArc * 4.0f * t * (1.0f - t);

    x = curX;
    y = curY + arcOffset;

    if (angularSpeed != 0.0f) {
        angle += angularSpeed * dt;
    }
}

void MissileSprite::draw(const noosa::Camera* camera) {
    if (!active || !visible) return;

    float zoom = camera ? camera->getZoom() : 1.0f;
    float t = duration > 0.0f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 0.0f;

    // 1. Ground shadow (directly underneath the airborne projectile on the floor)
    float curGroundX = startX + (targetX - startX) * t;
    float curGroundY = startY + (targetY - startY) * t;
    float gsx, gsy;
    if (camera) {
        camera->worldToScreen(curGroundX, curGroundY, gsx, gsy);
    } else {
        gsx = curGroundX;
        gsy = curGroundY;
    }
    float altRatio = std::max(0.0f, 4.0f * t * (1.0f - t));
    float shadowRadius = std::max(1.5f, (3.5f - 1.0f * altRatio)) * zoom;
    C2D_DrawCircleSolid(gsx, gsy + 4.0f * zoom, 0.28f, shadowRadius, C2D_Color32(0x00, 0x00, 0x00, 0x88));

    // 2. Airborne flying projectile
    if (m_image.tex && m_image.subtex) {
        noosa::Image::draw(camera);
    } else {
        // Fallback procedural diamond indicator
        float px, py;
        if (camera) {
            camera->worldToScreen(x, y, px, py);
        } else {
            px = x;
            py = y;
        }
        float size = 8.0f * zoom;
        C2D_DrawRectSolid(px - size * 0.5f, py - size * 0.5f, z, size, size, C2D_Color32(0xff, 0xee, 0x66, 0xFF));
    }
}

} // namespace pd
