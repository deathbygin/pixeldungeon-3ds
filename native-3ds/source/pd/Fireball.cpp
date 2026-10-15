#include "pd/effects/Fireball.h"
#include "pd/Assets.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace pd::effects {

static float randFloat(float minVal, float maxVal) {
    float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return minVal + r * (maxVal - minVal);
}

Fireball::Fireball() {
    m_tex = noosa::Texture::get(pd::Assets::FIREBALL);
    m_flames.resize(32);
    m_sparks.resize(32);
    initSubTextures();
}

void Fireball::setPos(float x, float y) {
    m_x = x;
    m_y = y;
}

void Fireball::initSubTextures() {
    if (!m_tex || !m_tex->isLoaded() || m_subTexInit) return;

    float potW = static_cast<float>(m_tex->potWidth());
    float potH = static_cast<float>(m_tex->potHeight());

    auto setupSub = [&](Tex3DS_SubTexture& sub, float x0, float x1) {
        sub.width = 32;
        sub.height = 32;
        sub.left = x0 / potW;
        sub.right = x1 / potW;
        sub.top = 1.0f;
        sub.bottom = 1.0f - (32.0f / potH);
    };

    setupSub(m_subBlight, 0.0f, 32.0f);
    setupSub(m_subFlight, 32.0f, 64.0f);
    setupSub(m_subFlame1, 64.0f, 96.0f);
    setupSub(m_subFlame2, 96.0f, 128.0f);

    m_subTexInit = true;
}

void Fireball::emitFlame() {
    initSubTextures();
    for (auto& f : m_flames) {
        if (!f.alive) {
            f.alive = true;
            // Emitter position is jittered around center
            f.x = m_x + randFloat(-4.0f, 4.0f);
            f.y = m_y + randFloat(-4.0f, 4.0f);
            f.vx = 0.0f;
            f.vy = -40.0f;
            f.accY = -20.0f;
            f.lifespan = 1.0f;
            f.timeLeft = f.lifespan;
            f.frameIndex = std::rand() % 2;
            break;
        }
    }
}

void Fireball::emitSpark() {
    for (auto& s : m_sparks) {
        if (!s.alive) {
            s.alive = true;
            s.x = m_x;
            s.y = m_y;
            s.vx = randFloat(-40.0f, 40.0f);
            s.vy = randFloat(-60.0f, 20.0f);
            s.accY = 80.0f;
            s.lifespan = randFloat(0.5f, 1.0f);
            s.timeLeft = s.lifespan;
            s.size = 2.0f;

            // Random color between 0xFF66FF and 0x66FF66
            float t = randFloat(0.0f, 1.0f);
            uint8_t r = static_cast<uint8_t>(0xFF * (1.0f - t) + 0x66 * t);
            uint8_t g = static_cast<uint8_t>(0x66 * (1.0f - t) + 0xFF * t);
            uint8_t b = static_cast<uint8_t>(0xFF * (1.0f - t) + 0x66 * t);
            s.color = C2D_Color32(r, g, b, 0xFF);
            break;
        }
    }
}

void Fireball::update(float dt) {
    initSubTextures();

    // Rotations: BLIGHT = -90 deg/s, FLIGHT = +360 deg/s
    // Citro2D uses radians for angle
    const float deg2rad = 3.1415926535f / 180.0f;
    m_bAngle -= 90.0f * deg2rad * dt;
    m_fAngle += 360.0f * deg2rad * dt;

    // Emit flame particles every 0.1s
    m_emitTimer += dt;
    while (m_emitTimer >= EMIT_INTERVAL) {
        m_emitTimer -= EMIT_INTERVAL;
        emitFlame();
    }

    // Emit sparks randomly
    if (randFloat(0.0f, 1.0f) < dt * 2.0f) {
        emitSpark();
    }

    // Update flames
    for (auto& f : m_flames) {
        if (!f.alive) continue;
        f.timeLeft -= dt;
        if (f.timeLeft <= 0.0f) {
            f.alive = false;
            continue;
        }
        f.vy += f.accY * dt;
        f.x += f.vx * dt;
        f.y += f.vy * dt;
    }

    // Update sparks
    for (auto& s : m_sparks) {
        if (!s.alive) continue;
        s.timeLeft -= dt;
        if (s.timeLeft <= 0.0f) {
            s.alive = false;
            continue;
        }
        s.vy += s.accY * dt;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
    }
}

void Fireball::draw(float z) const {
    if (!m_tex || !m_tex->isLoaded() || !m_tex->getTex() || !m_subTexInit) return;

    C2D_Image img;
    img.tex = m_tex->getTex();

    // 1. Draw BLIGHT (Back halo)
    img.subtex = &m_subBlight;
    C2D_DrawParams bParams;
    bParams.pos.x = m_x;
    bParams.pos.y = m_y;
    bParams.pos.w = 32.0f;
    bParams.pos.h = 32.0f;
    bParams.center.x = 16.0f;
    bParams.center.y = 16.0f;
    bParams.depth = z;
    bParams.angle = m_bAngle;
    C2D_DrawImage(img, &bParams, nullptr);

    // 2. Draw Flame Particles
    for (const auto& f : m_flames) {
        if (!f.alive) continue;

        float p = f.timeLeft / f.lifespan;
        float alpha = (p > 0.8f ? (1.0f - p) * 5.0f : p * 1.25f);
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        float scale = p;

        img.subtex = (f.frameIndex == 0) ? &m_subFlame1 : &m_subFlame2;

        C2D_DrawParams fParams;
        fParams.pos.x = f.x;
        fParams.pos.y = f.y;
        fParams.pos.w = 32.0f * scale;
        fParams.pos.h = 32.0f * scale;
        fParams.center.x = 16.0f * scale;
        fParams.center.y = 16.0f * scale;
        fParams.depth = z + 0.01f;
        fParams.angle = 0.0f;

        C2D_ImageTint tint;
        u8 a = static_cast<u8>(alpha * 255.0f);
        C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xFF, a), 1.0f);
        C2D_DrawImage(img, &fParams, &tint);
    }

    // 3. Draw FLIGHT (Front halo rays)
    img.subtex = &m_subFlight;
    C2D_DrawParams fLightParams;
    fLightParams.pos.x = m_x;
    fLightParams.pos.y = m_y;
    fLightParams.pos.w = 32.0f;
    fLightParams.pos.h = 32.0f;
    fLightParams.center.x = 16.0f;
    fLightParams.center.y = 16.0f;
    fLightParams.depth = z + 0.02f;
    fLightParams.angle = m_fAngle;
    C2D_DrawImage(img, &fLightParams, nullptr);

    // 4. Draw Sparks
    for (const auto& s : m_sparks) {
        if (!s.alive) continue;
        float p = s.timeLeft / s.lifespan;
        float curSize = s.size * p;
        C2D_DrawRectSolid(s.x, s.y, z + 0.03f, curSize, curSize, s.color);
    }
}

} // namespace pd::effects

