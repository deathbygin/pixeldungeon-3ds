#include "pd/FX.h"
#include "pd/Dungeon.h"
#include "pd/Random.h"
#include "pd/MissileSprite.h"
#include <cmath>

namespace pd {

PixelParticle FX::s_particles[MAX_PARTICLES];
Ripple FX::s_ripples[MAX_RIPPLES];
CheckedCell FX::s_checked[MAX_CHECKED];
Wound FX::s_wounds[MAX_WOUNDS];
FloatingText FX::s_texts[MAX_TEXTS];
std::vector<std::unique_ptr<MissileSprite>> FX::s_missiles;

Blob FX::s_toxicGas(Blob::BlobType::TOXIC_GAS);
Blob FX::s_paralyticGas(Blob::BlobType::PARALYTIC_GAS);
Blob FX::s_fire(Blob::BlobType::FIRE);
Blob FX::s_confusionGas(Blob::BlobType::CONFUSION_GAS);
Blob FX::s_corrosiveGas(Blob::BlobType::CORROSIVE_GAS);
Blob FX::s_regrowth(Blob::BlobType::REGROWTH);
Blob FX::s_foliage(Blob::BlobType::FOLIAGE);

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
    s_missiles.clear();

    s_toxicGas.clear();
    s_paralyticGas.clear();
    s_fire.clear();
    s_confusionGas.clear();
    s_corrosiveGas.clear();
    s_regrowth.clear();
    s_foliage.clear();
    s_blobEmitTimer = 0.0f;
}

void FX::update(float dt) {
    for (auto& p : s_particles) if (p.active) p.update(dt);
    for (auto& r : s_ripples)   if (r.active) r.update(dt);
    for (auto& c : s_checked)   if (c.active) c.update(dt);
    for (auto& w : s_wounds)    if (w.active) w.update(dt);
    for (auto& t : s_texts)     if (t.active) t.update(dt);

    for (auto& m : s_missiles) {
        if (m && m->active) {
            m->update(dt);
        }
    }
    s_missiles.erase(
        std::remove_if(s_missiles.begin(), s_missiles.end(),
                       [](const std::unique_ptr<MissileSprite>& m) { return !m || !m->active; }),
        s_missiles.end()
    );

    // Periodic ambient particle emission for active blobs in player FOV (matching BlobEmitter)
    s_blobEmitTimer += dt;
    if (s_blobEmitTimer >= 0.05f) {
        s_blobEmitTimer = 0.0f;

        auto emitBlobParticles = [](const Blob& blob) {
            if (blob.volume <= 0) return;
            for (int i = 0; i < Level::LENGTH; ++i) {
                if (blob.cur[i] > 0 && Dungeon::visible[i]) {
                    // Density-scaled emission probability:
                    // Dense blobs emit frequently, dissipating blobs emit occasionally
                    float prob = std::clamp(static_cast<float>(blob.cur[i]) / 25.0f, 0.08f, 0.45f);
                    if (Random::Float() < prob) {
                        float wx = (i % Level::WIDTH) * 16.0f + Random::Float(1.0f, 15.0f);
                        float wy = (i / Level::WIDTH) * 16.0f + Random::Float(1.0f, 15.0f);

                        uint32_t col = blob.getParticleColor();
                        float sz = 3.5f;
                        float life = 0.6f;
                        float vx = Random::Float(-6.0f, 6.0f);
                        float vy = Random::Float(-10.0f, 0.0f);
                        float ax = 0.0f;
                        float ay = 0.0f;

                        switch (blob.type) {
                        case Blob::BlobType::FIRE: {
                            static const uint32_t flameCols[] = { 0xee7722FF, 0xff9922FF, 0xffcc22FF, 0xff5511FF };
                            col = flameCols[Random::Int(4)];
                            sz = Random::Float(3.0f, 5.0f);
                            life = Random::Float(0.40f, 0.65f);
                            vx = Random::Float(-5.0f, 5.0f);
                            vy = Random::Float(-30.0f, -55.0f);
                            ay = -80.0f; // Rapid upward acceleration
                            break;
                        }
                        case Blob::BlobType::TOXIC_GAS: {
                            static const uint32_t toxicCols[] = { 0x50ff60FF, 0x38b764FF, 0x66ff88FF };
                            col = toxicCols[Random::Int(3)];
                            sz = Random::Float(3.5f, 5.5f);
                            life = Random::Float(0.7f, 1.3f);
                            vx = Random::Float(-6.0f, 6.0f);
                            vy = Random::Float(-6.0f, 2.0f);
                            ay = -3.0f;
                            break;
                        }
                        case Blob::BlobType::PARALYTIC_GAS: {
                            static const uint32_t paraCols[] = { 0xffff66FF, 0xe4d83bFF, 0xffee88FF };
                            col = paraCols[Random::Int(3)];
                            sz = Random::Float(3.5f, 5.5f);
                            life = Random::Float(0.7f, 1.3f);
                            vx = Random::Float(-5.0f, 5.0f);
                            vy = Random::Float(-5.0f, 2.0f);
                            ay = -2.0f;
                            break;
                        }
                        case Blob::BlobType::CONFUSION_GAS: {
                            static const uint32_t confCols[] = { 0xbf55ecFF, 0x33ccffFF, 0xff66ccFF, 0xee82eeFF };
                            col = confCols[Random::Int(4)];
                            sz = Random::Float(3.0f, 5.0f);
                            life = Random::Float(0.6f, 1.1f);
                            vx = Random::Float(-8.0f, 8.0f);
                            vy = Random::Float(-8.0f, 3.0f);
                            break;
                        }
                        case Blob::BlobType::CORROSIVE_GAS: {
                            static const uint32_t corrCols[] = { 0x22aa44FF, 0x88bb22FF, 0x448822FF, 0x55aa33FF };
                            col = corrCols[Random::Int(4)];
                            sz = Random::Float(3.0f, 5.0f);
                            life = Random::Float(0.6f, 1.0f);
                            vx = Random::Float(-6.0f, 6.0f);
                            vy = Random::Float(-8.0f, -2.0f);
                            ay = -8.0f;
                            break;
                        }
                        case Blob::BlobType::REGROWTH: {
                            static const uint32_t leafCols[] = { 0x44aa33FF, 0x66bb44FF, 0x227711FF };
                            col = leafCols[Random::Int(3)];
                            sz = Random::Float(2.5f, 3.5f);
                            life = Random::Float(0.7f, 1.1f);
                            vx = Random::Float(-8.0f, 8.0f);
                            vy = Random::Float(-12.0f, -4.0f);
                            ay = 25.0f; // Flutter downwards
                            break;
                        }
                        default:
                            break;
                        }

                        for (auto& p : s_particles) {
                            if (!p.active) {
                                p.reset(wx, wy, col, sz, life, vx, vy, ax, ay, true);
                                break;
                            }
                        }
                    }
                }
            }
        };

        emitBlobParticles(s_fire);
        emitBlobParticles(s_toxicGas);
        emitBlobParticles(s_paralyticGas);
        emitBlobParticles(s_confusionGas);
        emitBlobParticles(s_corrosiveGas);
        emitBlobParticles(s_regrowth);
    }
}

void FX::drawGround(const noosa::Camera* camera) {
    // Ground FX: Water ripples & search checked cell highlights (drawn between tiles/heaps and characters)
    for (auto& r : s_ripples) if (r.active) r.draw(camera);
    for (auto& c : s_checked) if (c.active) c.draw(camera);
}

void FX::draw(const noosa::Camera* camera, C2D_TextBuf textBuf) {
    // 2. Melee wound slashes
    for (auto& w : s_wounds)  if (w.active) w.draw(camera);

    // 3. Pixel particles
    for (const auto& p : s_particles) if (p.active) p.draw(camera);

    // 4. Flying missile sprites
    for (const auto& m : s_missiles) if (m && m->active) m->draw(camera);

    // 5. Floating damage / text indicators (drawn on top)
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
        if (c.active && c.cell == cell) {
            c.reset(cell);
            return;
        }
    }
    for (auto& c : s_checked) {
        if (!c.active) {
            c.reset(cell);
            return;
        }
    }
    CheckedCell* oldest = &s_checked[0];
    for (auto& c : s_checked) {
        if (c.time < oldest->time) {
            oldest = &c;
        }
    }
    oldest->reset(cell);
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
    s_confusionGas.evolve(level);
    s_corrosiveGas.evolve(level);
    s_regrowth.evolve(level);
    s_foliage.evolve(level);
}

Blob* FX::getBlob(Blob::BlobType type) {
    switch (type) {
    case Blob::BlobType::TOXIC_GAS:     return &s_toxicGas;
    case Blob::BlobType::PARALYTIC_GAS: return &s_paralyticGas;
    case Blob::BlobType::FIRE:          return &s_fire;
    case Blob::BlobType::CONFUSION_GAS: return &s_confusionGas;
    case Blob::BlobType::CORROSIVE_GAS: return &s_corrosiveGas;
    case Blob::BlobType::REGROWTH:      return &s_regrowth;
    case Blob::BlobType::FOLIAGE:       return &s_foliage;
    default:                            return nullptr;
    }
}

void FX::missile(int fromCell, int toCell, int itemImage, std::function<void()> callback) {
    auto m = std::make_unique<MissileSprite>();
    m->reset(fromCell, toCell, itemImage, std::move(callback));
    s_missiles.push_back(std::move(m));
}

bool FX::isMissileActive() {
    return !s_missiles.empty();
}

} // namespace pd

