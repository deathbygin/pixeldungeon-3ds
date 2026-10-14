#include "pd/FX.h"
#include "pd/Dungeon.h"
#include "pd/Random.h"
#include <cmath>

namespace pd {

PixelParticle FX::s_particles[MAX_PARTICLES];
Ripple FX::s_ripples[MAX_RIPPLES];
CheckedCell FX::s_checked[MAX_CHECKED];
Wound FX::s_wounds[MAX_WOUNDS];
FloatingText FX::s_texts[MAX_TEXTS];

Blob FX::s_toxicGas(Blob::BlobType::TOXIC_GAS);
Blob FX::s_paralyticGas(Blob::BlobType::PARALYTIC_GAS);
Blob FX::s_fire(Blob::BlobType::FIRE);

float FX::s_blobEmitTimer = 0.0f;

void FX::init() {
    clear();
}

void FX::clear() {
    for (auto& p : s_particles) p.active = false;
    for (auto& r : s_ripples)   r.active = false;
    for (auto& c : s_checked)   c.active = false;
    for (auto& w : s_wounds)    w.active = false;
    for (auto& t : s_texts)     t.active = false;

    s_toxicGas.clear();
    s_paralyticGas.clear();
    s_fire.clear();
    s_blobEmitTimer = 0.0f;
}

void FX::update(float dt) {
    for (auto& p : s_particles) if (p.active) p.update(dt);
    for (auto& r : s_ripples)   if (r.active) r.update(dt);
    for (auto& c : s_checked)   if (c.active) c.update(dt);
    for (auto& w : s_wounds)    if (w.active) w.update(dt);
    for (auto& t : s_texts)     if (t.active) t.update(dt);

    // Periodic ambient particle emission for active gas & fire in FOV (matching BlobEmitter)
    s_blobEmitTimer += dt;
    if (s_blobEmitTimer >= 0.08f) {
        s_blobEmitTimer = 0.0f;

        auto emitBlobParticles = [](const Blob& blob) {
            if (blob.volume <= 0) return;
            for (int i = 0; i < Level::LENGTH; ++i) {
                if (blob.cur[i] > 0 && Dungeon::visible[i]) {
                    if (Random::Float() < 0.25f) {
                        float wx = (i % Level::WIDTH) * 16.0f + Random::Float(2.0f, 14.0f);
                        float wy = (i / Level::WIDTH) * 16.0f + Random::Float(2.0f, 14.0f);
                        float vy = (blob.type == Blob::BlobType::FIRE) ? Random::Float(-30.0f, -60.0f) : Random::Float(-10.0f, -20.0f);
                        float vx = Random::Float(-10.0f, 10.0f);
                        float ay = (blob.type == Blob::BlobType::FIRE) ? -80.0f : 0.0f;

                        for (auto& p : s_particles) {
                            if (!p.active) {
                                p.reset(wx, wy, blob.getParticleColor(),
                                        (blob.type == Blob::BlobType::FIRE) ? 4.0f : 3.0f,
                                        Random::Float(0.4f, 0.7f),
                                        vx, vy, 0.0f, ay, true);
                                break;
                            }
                        }
                    }
                }
            }
        };

        emitBlobParticles(s_toxicGas);
        emitBlobParticles(s_paralyticGas);
        emitBlobParticles(s_fire);
    }
}

void FX::draw(const noosa::Camera* camera, C2D_TextBuf textBuf) {
    // 1. Water ripples & search highlights (drawn under characters)
    for (auto& r : s_ripples) if (r.active) r.draw(camera);
    for (auto& c : s_checked) if (c.active) c.draw(camera);

    // 2. Melee wound slashes
    for (auto& w : s_wounds)  if (w.active) w.draw(camera);

    // 3. Pixel particles
    for (const auto& p : s_particles) if (p.active) p.draw(camera);

    // 4. Floating damage / text indicators (drawn on top)
    for (const auto& t : s_texts) if (t.active) t.draw(camera, textBuf);
}

void FX::splash(int cell, uint32_t color, int count) {
    if (cell < 0 || cell >= Level::LENGTH) return;
    if (!Dungeon::visible[cell] && (!Dungeon::hero || cell != Dungeon::hero->pos)) return;
    float cx = (cell % Level::WIDTH) * 16.0f + 8.0f;
    float cy = (cell / Level::WIDTH) * 16.0f + 8.0f;
    burst(cx, cy, color, count, 40.0f, 80.0f, +100.0f);
}

void FX::blood(int cell, int count) {
    splash(cell, 0xd63030FF, count);
}

void FX::waterSplash(int cell, int count) {
    splash(cell, 0x55aaffFF, count);
}

void FX::slash(int cell) {
    if (cell < 0 || cell >= Level::LENGTH) return;
    if (!Dungeon::visible[cell] && (!Dungeon::hero || cell != Dungeon::hero->pos)) return;
    for (auto& w : s_wounds) {
        if (!w.active) {
            w.reset(cell);
            return;
        }
    }
}

void FX::ripple(int cell) {
    if (cell < 0 || cell >= Level::LENGTH) return;
    if (!Dungeon::visible[cell] && (!Dungeon::hero || cell != Dungeon::hero->pos)) return;
    for (auto& r : s_ripples) {
        if (!r.active) {
            r.reset(cell);
            return;
        }
    }
}

void FX::searchCell(int cell) {
    if (cell < 0 || cell >= Level::LENGTH) return;
    for (auto& c : s_checked) {
        if (!c.active) {
            c.reset(cell);
            return;
        }
    }
}

void FX::floatingText(int cell, const std::string& str, uint32_t color) {
    if (cell < 0 || cell >= Level::LENGTH || str.empty()) return;
    if (!Dungeon::visible[cell] && (!Dungeon::hero || cell != Dungeon::hero->pos)) return;
    float cx = (cell % Level::WIDTH) * 16.0f + 8.0f;
    float cy = (cell / Level::WIDTH) * 16.0f;

    for (auto& t : s_texts) {
        if (!t.active) {
            t.reset(cx, cy, str, color);
            return;
        }
    }
}

void FX::burst(float wx, float wy, uint32_t color, int count,
               float speedMin, float speedMax, float gravity) {
    int spawned = 0;
    for (auto& p : s_particles) {
        if (!p.active) {
            float angle = Random::Float(-3.14159265f, 0.0f); // Upward semicircle
            float spd = Random::Float(speedMin, speedMax);
            float vx = std::cos(angle) * spd;
            float vy = std::sin(angle) * spd;

            p.reset(wx, wy, color, Random::Float(3.0f, 5.0f),
                    Random::Float(0.4f, 0.7f),
                    vx, vy, 0.0f, gravity, true);

            if (++spawned >= count) break;
        }
    }
}

void FX::seedBlob(int cell, Blob::BlobType type, int amount) {
    Blob* b = getBlob(type);
    if (b) {
        b->seed(cell, amount);
    }
}

void FX::evolveBlobs(const Level* level) {
    s_toxicGas.evolve(level);
    s_paralyticGas.evolve(level);
    s_fire.evolve(level);
}

Blob* FX::getBlob(Blob::BlobType type) {
    switch (type) {
    case Blob::BlobType::TOXIC_GAS:     return &s_toxicGas;
    case Blob::BlobType::PARALYTIC_GAS: return &s_paralyticGas;
    case Blob::BlobType::FIRE:          return &s_fire;
    default:                            return nullptr;
    }
}

} // namespace pd

