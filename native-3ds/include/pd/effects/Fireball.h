#pragma once

#include <citro2d.h>
#include <memory>
#include <vector>
#include "noosa/Texture.h"

namespace pd::effects {

/**
 * Animated Fireball / Torch particle emitter matching com.watabou.pixeldungeon.effects.Fireball.
 * Features rotating back/front halo lights, rising shrinking flame particles, and sparks.
 */
class Fireball {
public:
    Fireball();
    ~Fireball() = default;

    void setPos(float x, float y);
    void update(float dt);
    void draw(float z = 0.5f) const;

private:
    struct FlameParticle {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = -40.0f;
        float accY = -20.0f;
        float timeLeft = 1.0f;
        float lifespan = 1.0f;
        int frameIndex = 0; // 0: FLAME1 (uv 64..96), 1: FLAME2 (uv 96..128)
        bool alive = false;
    };

    struct SparkParticle {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float accY = 80.0f;
        float timeLeft = 1.0f;
        float lifespan = 1.0f;
        float size = 2.0f;
        u32 color = 0xFF66FF;
        bool alive = false;
    };

    float m_x = 0.0f;
    float m_y = 0.0f;

    float m_bAngle = 0.0f; // Angular speed -90 deg/s
    float m_fAngle = 0.0f; // Angular speed +360 deg/s

    float m_emitTimer = 0.0f;
    static constexpr float EMIT_INTERVAL = 0.10f; // seconds

    std::shared_ptr<noosa::Texture> m_tex;
    std::vector<FlameParticle> m_flames;
    std::vector<SparkParticle> m_sparks;

    Tex3DS_SubTexture m_subBlight;
    Tex3DS_SubTexture m_subFlight;
    Tex3DS_SubTexture m_subFlame1;
    Tex3DS_SubTexture m_subFlame2;
    bool m_subTexInit = false;

    void initSubTextures();
    void emitFlame();
    void emitSpark();
};

} // namespace pd::effects

