#include "pd/FX.h"
#include "pd/Dungeon.h"
#include "pd/Random.h"
#include "pd/MissileSprite.h"
#include "pd/Buff.h"
#include <cmath>

namespace pd {

PixelParticle FX::s_particles[MAX_PARTICLES];
Ripple FX::s_ripples[MAX_RIPPLES];
CheckedCell FX::s_checked[MAX_CHECKED];
Wound FX::s_wounds[MAX_WOUNDS];
FloatingText FX::s_texts[MAX_TEXTS];
std::vector<std::unique_ptr<MissileSprite>> FX::s_missiles;
FX::MagicMissileState FX::s_magicMissile;
FX::LightningState FX::s_lightning;
FX::DeathRayState FX::s_deathRay;

Blob FX::s_toxicGas(Blob::BlobType::TOXIC_GAS);
Blob FX::s_paralyticGas(Blob::BlobType::PARALYTIC_GAS);
Blob FX::s_fire(Blob::BlobType::FIRE);
Blob FX::s_confusionGas(Blob::BlobType::CONFUSION_GAS);
Blob FX::s_corrosiveGas(Blob::BlobType::CORROSIVE_GAS);
Blob FX::s_regrowth(Blob::BlobType::REGROWTH);
Blob FX::s_foliage(Blob::BlobType::FOLIAGE);
Blob FX::s_waterOfHealth(Blob::BlobType::WATER_OF_HEALTH);
Blob FX::s_waterOfAwareness(Blob::BlobType::WATER_OF_AWARENESS);
Blob FX::s_waterOfTransmutation(Blob::BlobType::WATER_OF_TRANSMUTATION);

float FX::s_blobEmitTimer = 0.0f;
static float s_flashDuration = 0.0f;
static float s_flashElapsed = 0.0f;
static uint32_t s_flashColor = 0xFFFFFFFF;

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
    s_magicMissile = {};
    s_lightning = {};
    s_deathRay = {};

    s_toxicGas.clear();
    s_paralyticGas.clear();
    s_fire.clear();
    s_confusionGas.clear();
    s_corrosiveGas.clear();
    s_regrowth.clear();
    s_foliage.clear();
    s_waterOfHealth.clear();
    s_waterOfAwareness.clear();
    s_waterOfTransmutation.clear();
    s_blobEmitTimer = 0.0f;
    s_flashDuration = 0.0f;
    s_flashElapsed = 0.0f;
}

void FX::flash(uint32_t color, float duration) {
    s_flashColor = color;
    s_flashDuration = duration;
    s_flashElapsed = 0.0f;
}

bool FX::isFlashing() {
    return s_flashElapsed < s_flashDuration && s_flashDuration > 0.0f;
}

uint32_t FX::getFlashColor() {
    if (!isFlashing()) return 0;
    float alphaRatio = 1.0f - (s_flashElapsed / s_flashDuration);
    if (alphaRatio < 0.0f) alphaRatio = 0.0f;
    if (alphaRatio > 1.0f) alphaRatio = 1.0f;

    uint8_t r = (s_flashColor >> 24) & 0xFF;
    uint8_t g = (s_flashColor >> 16) & 0xFF;
    uint8_t b = (s_flashColor >> 8)  & 0xFF;
    uint8_t a = (s_flashColor)       & 0xFF;
    uint8_t scaledA = static_cast<uint8_t>(a * alphaRatio);

    return C2D_Color32(r, g, b, scaledA);
}

void FX::update(float dt) {
    if (s_flashElapsed < s_flashDuration) {
        s_flashElapsed += dt;
    }

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

    if (s_magicMissile.active) {
        s_magicMissile.elapsed += dt;
        float t = s_magicMissile.elapsed / s_magicMissile.duration;
        if (t >= 1.0f) {
            s_magicMissile.active = false;
            if (s_magicMissile.callback) {
                auto cb = std::move(s_magicMissile.callback);
                s_magicMissile.callback = nullptr;
                cb();
            }
        } else {
            s_magicMissile.curX = s_magicMissile.startX + (s_magicMissile.targetX - s_magicMissile.startX) * t;
            s_magicMissile.curY = s_magicMissile.startY + (s_magicMissile.targetY - s_magicMissile.startY) * t;
            for (auto& p : s_particles) {
                if (!p.active) {
                    float vx = Random::Float(-10.0f, 10.0f);
                    float vy = Random::Float(-10.0f, 10.0f);
                    p.reset(s_magicMissile.curX, s_magicMissile.curY, s_magicMissile.color,
                            Random::Float(2.5f, 4.0f), 0.22f, vx, vy, 0.0f, 0.0f, true);
                    break;
                }
            }
        }
    }

    if (s_lightning.active) {
        s_lightning.time -= dt;
        if (s_lightning.time <= 0.0f) {
            s_lightning.active = false;
            if (s_lightning.callback) {
                auto cb = std::move(s_lightning.callback);
                s_lightning.callback = nullptr;
                cb();
            }
        }
    }

    if (s_deathRay.active) {
        s_deathRay.time -= dt;
        if (s_deathRay.time <= 0.0f) {
            s_deathRay.active = false;
            if (s_deathRay.callback) {
                auto cb = std::move(s_deathRay.callback);
                s_deathRay.callback = nullptr;
                cb();
            }
        }
    }

    // Periodic ambient particle emission for active blobs in player FOV (matching BlobEmitter)
    s_blobEmitTimer += dt;
    if (s_blobEmitTimer >= 0.03f) {
        s_blobEmitTimer = 0.0f;

        auto spawnParticle = [](float wx, float wy, uint32_t col, float sz, float life,
                                float vx, float vy, float ax, float ay, bool shrink) {
            for (auto& p : s_particles) {
                if (!p.active) {
                    p.reset(wx, wy, col, sz, life, vx, vy, ax, ay, shrink);
                    return true;
                }
            }
            return false;
        };

        // 1. FIRE: 1:1 with Java Fire.use(BlobEmitter) and FlameParticle
        // In original Pixel Dungeon, every visible burning tile emits particles each tick.
        // If cur[i] >= 3, emit 2 particles for an extra-dense core body matching original graphics.
        if (s_fire.volume > 0) {
            static const uint32_t flameCols[] = {
                0xFFFFFFFF, // Brilliant incandescent white core
                0xFFFF66FF, // Electric hot yellow
                0xFFEE22FF, // Golden amber
                0xFF9911FF, // Vibrant fiery orange
                0xEE4411FF, // Deep flame red-orange
                0xFF7711FF  // Pure flame orange
            };
            for (int i = 0; i < Level::LENGTH; ++i) {
                if (s_fire.cur[i] > 0 && Dungeon::visible[i]) {
                    int count = (s_fire.cur[i] >= 3) ? 2 : 1;
                    for (int c = 0; c < count; ++c) {
                        float wx = (i % Level::WIDTH) * 16.0f + Random::Float(1.0f, 15.0f);
                        float wy = (i / Level::WIDTH) * 16.0f + Random::Float(1.0f, 15.0f);
                        uint32_t col = flameCols[Random::Int(6)];
                        float sz = Random::Float(3.0f, 4.5f);
                        float life = Random::Float(0.45f, 0.65f);
                        float vx = Random::Float(-5.0f, 5.0f);
                        float vy = Random::Float(-35.0f, -65.0f);
                        float ay = -80.0f; // Rapid buoyant upward acceleration matching FlameParticle
                        if (!spawnParticle(wx, wy, col, sz, life, vx, vy, 0.0f, ay, true)) {
                            break;
                        }
                    }
                }
            }
        }

        // 2. GAS BLOBS: 1:1 with Java ToxicGas/ParalyticGas/ConfusionGas pour(Speck)
        // Gases use steady density-scaled emission of billowing specks that drift and fade.
        auto emitGasBlob = [&](const Blob& blob) {
            if (blob.volume <= 0) return;
            for (int i = 0; i < Level::LENGTH; ++i) {
                if (blob.cur[i] > 0 && Dungeon::visible[i]) {
                    bool isWellWater = (blob.type == Blob::BlobType::WATER_OF_HEALTH ||
                                        blob.type == Blob::BlobType::WATER_OF_AWARENESS ||
                                        blob.type == Blob::BlobType::WATER_OF_TRANSMUTATION);
                    float prob = isWellWater ? 0.16f : ((blob.cur[i] >= 100) ? 0.35f : std::clamp(static_cast<float>(blob.cur[i]) / 100.0f, 0.08f, 0.35f));
                    if (Random::Float() < prob) {
                        float wx = isWellWater ? ((i % Level::WIDTH) * 16.0f + Random::Float(4.0f, 12.0f))
                                               : ((i % Level::WIDTH) * 16.0f + Random::Float(1.0f, 15.0f));
                        float wy = isWellWater ? ((i / Level::WIDTH) * 16.0f + Random::Float(7.0f, 11.0f))
                                               : ((i / Level::WIDTH) * 16.0f + Random::Float(1.0f, 15.0f));

                        uint32_t col = 0xFFFFFFFF;
                        float sz = 4.0f;
                        float life = 1.0f;
                        float vx = 0.0f, vy = 0.0f, ax = 0.0f, ay = 0.0f;
                        bool shrink = false;

                        switch (blob.type) {
                        case Blob::BlobType::TOXIC_GAS: {
                            static const uint32_t toxicCols[] = { 0x50FF60FF, 0x38B764FF, 0x66FF88FF, 0x88FF66FF };
                            col = toxicCols[Random::Int(4)];
                            sz = Random::Float(3.5f, 5.5f);
                            life = Random::Float(0.9f, 1.6f);
                            vx = Random::Float(-5.0f, 5.0f);
                            vy = Random::Float(-6.0f, 2.0f);
                            ay = -3.0f;
                            break;
                        }
                        case Blob::BlobType::PARALYTIC_GAS: {
                            static const uint32_t paraCols[] = { 0xFFFF66FF, 0xE4D83BFF, 0xFFEE88FF, 0xFFF899FF };
                            col = paraCols[Random::Int(4)];
                            sz = Random::Float(3.5f, 5.5f);
                            life = Random::Float(0.9f, 1.6f);
                            vx = Random::Float(-4.0f, 4.0f);
                            vy = Random::Float(-5.0f, 2.0f);
                            ay = -2.0f;
                            break;
                        }
                        case Blob::BlobType::CONFUSION_GAS: {
                            static const uint32_t confCols[] = { 0xBF55ECFF, 0x33CCFFFF, 0xFF66CCFF, 0xEE82EEFF, 0xFFAAFFFF };
                            col = confCols[Random::Int(5)];
                            sz = Random::Float(3.5f, 5.5f);
                            life = Random::Float(0.8f, 1.4f);
                            vx = Random::Float(-6.0f, 6.0f);
                            vy = Random::Float(-6.0f, 3.0f);
                            ay = 0.0f;
                            break;
                        }
                        case Blob::BlobType::CORROSIVE_GAS: {
                            static const uint32_t corrCols[] = { 0x22AA44FF, 0x88BB22FF, 0x448822FF, 0x55AA33FF };
                            col = corrCols[Random::Int(4)];
                            sz = Random::Float(3.0f, 5.0f);
                            life = Random::Float(0.7f, 1.2f);
                            vx = Random::Float(-5.0f, 5.0f);
                            vy = Random::Float(-8.0f, -2.0f);
                            ay = -6.0f;
                            break;
                        }
                        case Blob::BlobType::REGROWTH: {
                            static const uint32_t leafCols[] = { 0x44AA33FF, 0x66BB44FF, 0x227711FF };
                            col = leafCols[Random::Int(3)];
                            sz = Random::Float(2.5f, 3.5f);
                            life = Random::Float(0.7f, 1.1f);
                            vx = Random::Float(-6.0f, 6.0f);
                            vy = Random::Float(-10.0f, -3.0f);
                            ay = 20.0f;
                            shrink = true;
                            break;
                        }
                        case Blob::BlobType::WATER_OF_HEALTH: {
                            // Sparkles matching Speck.HEALING (emerald / spring green rising bubbles)
                            static const uint32_t healthCols[] = { 0x20CC66FF, 0x50FF88FF, 0xA0FFD0FF, 0xFFFFFFFF };
                            col = healthCols[Random::Int(4)];
                            sz = Random::Float(2.0f, 3.5f);
                            life = Random::Float(0.6f, 1.0f);
                            vx = Random::Float(-3.0f, 3.0f);
                            vy = Random::Float(-16.0f, -8.0f);
                            ay = -4.0f;
                            shrink = true;
                            break;
                        }
                        case Blob::BlobType::WATER_OF_AWARENESS: {
                            // Sparkles matching Speck.QUESTION (celestial indigo / cyan rising light)
                            static const uint32_t awareCols[] = { 0x3388EEFF, 0x66AAFFFF, 0xAA66FFFF, 0xDDBBFFFF };
                            col = awareCols[Random::Int(4)];
                            sz = Random::Float(2.0f, 3.5f);
                            life = Random::Float(0.7f, 1.1f);
                            vx = Random::Float(-4.0f, 4.0f);
                            vy = Random::Float(-14.0f, -6.0f);
                            ay = -2.0f;
                            shrink = true;
                            break;
                        }
                        case Blob::BlobType::WATER_OF_TRANSMUTATION: {
                            // Sparkles matching Speck.CHANGE (iridescent golden / purple spiral motes)
                            static const uint32_t transCols[] = { 0xEEBB22FF, 0xFFDD44FF, 0xAA33FFFF, 0xFF66CCFF };
                            col = transCols[Random::Int(4)];
                            sz = Random::Float(2.0f, 3.5f);
                            life = Random::Float(0.8f, 1.2f);
                            vx = Random::Float(-5.0f, 5.0f);
                            vy = Random::Float(-12.0f, -5.0f);
                            ay = -2.0f;
                            shrink = true;
                            break;
                        }
                        default:
                            break;
                        }

                        spawnParticle(wx, wy, col, sz, life, vx, vy, ax, ay, shrink);
                    }
                }
            }
        };

        emitGasBlob(s_toxicGas);
        emitGasBlob(s_paralyticGas);
        emitGasBlob(s_confusionGas);
        emitGasBlob(s_corrosiveGas);
        emitGasBlob(s_regrowth);
        emitGasBlob(s_waterOfHealth);
        emitGasBlob(s_waterOfAwareness);
        emitGasBlob(s_waterOfTransmutation);

        // 3. Status Effect Particles on Characters (Burning flame particles & Frost snow motes)
        auto emitCharBuffFX = [&](Char* ch) {
            if (!ch || !ch->isAlive()) return;
            if (!Dungeon::visible[ch->pos] && (!Dungeon::hero || ch != Dungeon::hero.get())) return;

            float cx = (ch->pos % Level::WIDTH) * 16.0f + 8.0f;
            float cy = (ch->pos / Level::WIDTH) * 16.0f + 8.0f;

            if (ch->buff(BuffType::Burning)) {
                static const uint32_t flameCols[] = { 0xFFFFFFFF, 0xFFFF66FF, 0xFF9911FF, 0xEE4411FF };
                float wx = cx + Random::Float(-6.0f, 6.0f);
                float wy = cy + Random::Float(-4.0f, 6.0f);
                spawnParticle(wx, wy, flameCols[Random::Int(4)], Random::Float(3.0f, 4.5f),
                              Random::Float(0.35f, 0.55f), Random::Float(-4.0f, 4.0f),
                              Random::Float(-35.0f, -60.0f), 0.0f, -80.0f, true);
            }

            if (ch->buff(BuffType::Frost)) {
                if (Random::Float() < 0.35f) {
                    float wx = cx + Random::Float(-6.0f, 6.0f);
                    float wy = cy + Random::Float(-8.0f, 0.0f);
                    uint32_t frostCol = (Random::Int(2) == 0) ? 0xD0F0FFFF : 0xFFFFFFFF;
                    spawnParticle(wx, wy, frostCol, Random::Float(2.0f, 3.5f),
                                  Random::Float(0.8f, 1.3f), Random::Float(-2.0f, 2.0f),
                                  Random::Float(6.0f, 14.0f), 0.0f, 5.0f, false);
                }
            }

            if (ch->buff(BuffType::Light)) {
                // Torch flame & ember particles matching Java Speck.LIGHT / FlameParticle / TorchHalo
                if (Random::Float() < 0.65f) {
                    static const uint32_t torchCols[] = {
                        0xFFFFFFFF, // Brilliant incandescent white/gold spark
                        0xFFFFDD66, // Warm torch amber
                        0xFFFF9922, // Rich flame orange
                        0xFFEE4411  // Fiery red-orange ember
                    };
                    float wx = cx + Random::Float(-3.0f, 3.0f);
                    float wy = cy + Random::Float(-10.0f, -2.0f); // Emanating from torch held high
                    spawnParticle(wx, wy, torchCols[Random::Int(4)], Random::Float(2.0f, 3.5f),
                                  Random::Float(0.35f, 0.55f), Random::Float(-4.0f, 4.0f),
                                  Random::Float(-22.0f, -42.0f), 0.0f, -35.0f, true);
                }
            }
        };

        emitCharBuffFX(Dungeon::hero.get());
        for (const auto& mob : Dungeon::mobs) {
            emitCharBuffFX(mob.get());
        }
    }
}

void FX::drawGround(const noosa::Camera* camera) {
    // Ground FX: Water ripples & search checked cell highlights (drawn between tiles/heaps and characters)
    for (auto& r : s_ripples) if (r.active) r.draw(camera);
    for (auto& c : s_checked) if (c.active) c.draw(camera);

    // Warm radial torch halo matching Java TorchHalo (0xFFDDCC, 0.15f alpha, radius 24)
    if (camera && Dungeon::hero && Dungeon::hero->buff(BuffType::Light)) {
        float cx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
        float cy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
        float sx = 0.0f, sy = 0.0f;
        camera->worldToScreen(cx, cy, sx, sy);
        float zoom = camera->getZoom();
        float pulse = 1.0f + 0.05f * std::sin(Actor::now * 6.0f);
        float radius = 24.0f * zoom * pulse;
        C2D_DrawCircleSolid(sx, sy, 0.21f, radius, C2D_Color32(0xFF, 0xDD, 0xAA, 0x24));
    }
}

void FX::draw(const noosa::Camera* camera, C2D_TextBuf textBuf) {
    // 2. Melee wound slashes
    for (auto& w : s_wounds)  if (w.active) w.draw(camera);

    // 3. Pixel particles
    for (const auto& p : s_particles) if (p.active) p.draw(camera);

    // 4. Flying missile sprites
    for (const auto& m : s_missiles) if (m && m->active) m->draw(camera);

    // 4b. Death ray beam
    if (s_deathRay.active) {
        float x1 = (s_deathRay.fromCell % Level::WIDTH) * 16.0f + 8.0f;
        float y1 = (s_deathRay.fromCell / Level::WIDTH) * 16.0f + 8.0f;
        float x2 = (s_deathRay.toCell % Level::WIDTH) * 16.0f + 8.0f;
        float y2 = (s_deathRay.toCell / Level::WIDTH) * 16.0f + 8.0f;
        float sx1, sy1, sx2, sy2;
        camera->worldToScreen(x1, y1, sx1, sy1);
        camera->worldToScreen(x2, y2, sx2, sy2);
        float p = std::clamp(s_deathRay.time / 0.5f, 0.0f, 1.0f);
        u8 alpha = static_cast<u8>(p * 255.0f);
        u32 beamCol = C2D_Color32(0xcc, 0x44, 0xff, alpha);
        u32 coreCol = C2D_Color32(0xff, 0xff, 0xff, alpha);
        C2D_DrawLine(sx1, sy1, beamCol, sx2, sy2, beamCol, 6.0f * p * camera->getZoom(), 0.77f);
        C2D_DrawLine(sx1, sy1, coreCol, sx2, sy2, coreCol, 2.5f * p * camera->getZoom(), 0.78f);
    }

    // 4c. Lightning electric arcs
    if (s_lightning.active) {
        for (size_t i = 0; i + 1 < s_lightning.points.size(); ++i) {
            int c1 = s_lightning.points[i];
            int c2 = s_lightning.points[i + 1];
            float x1 = (c1 % Level::WIDTH) * 16.0f + 8.0f;
            float y1 = (c1 / Level::WIDTH) * 16.0f + 8.0f;
            float x2 = (c2 % Level::WIDTH) * 16.0f + 8.0f;
            float y2 = (c2 / Level::WIDTH) * 16.0f + 8.0f;
            float mx = (x1 + x2) * 0.5f + Random::Float(-4.0f, 4.0f);
            float my = (y1 + y2) * 0.5f + Random::Float(-4.0f, 4.0f);
            float sx1, sy1, smx, smy, sx2, sy2;
            camera->worldToScreen(x1, y1, sx1, sy1);
            camera->worldToScreen(mx, my, smx, smy);
            camera->worldToScreen(x2, y2, sx2, sy2);
            u32 boltCol = C2D_Color32(0xff, 0xff, 0xaa, 0xFF);
            u32 glowCol = C2D_Color32(0x55, 0xbb, 0xff, 0xAA);
            C2D_DrawLine(sx1, sy1, glowCol, smx, smy, glowCol, 4.0f * camera->getZoom(), 0.77f);
            C2D_DrawLine(smx, smy, glowCol, sx2, sy2, glowCol, 4.0f * camera->getZoom(), 0.77f);
            C2D_DrawLine(sx1, sy1, boltCol, smx, smy, boltCol, 2.0f * camera->getZoom(), 0.78f);
            C2D_DrawLine(smx, smy, boltCol, sx2, sy2, boltCol, 2.0f * camera->getZoom(), 0.78f);
        }
    }

    // 4d. Magic missile glowing projectile
    if (s_magicMissile.active) {
        float msx, msy;
        camera->worldToScreen(s_magicMissile.curX, s_magicMissile.curY, msx, msy);
        C2D_DrawCircleSolid(msx, msy, 0.77f, 3.5f * camera->getZoom(), s_magicMissile.color);
    }

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
    splash(cell, Dungeon::waterColor(), count);
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
    s_waterOfHealth.evolve(level);
    s_waterOfAwareness.evolve(level);
    s_waterOfTransmutation.evolve(level);
}

Blob* FX::getBlob(Blob::BlobType type) {
    switch (type) {
    case Blob::BlobType::TOXIC_GAS:              return &s_toxicGas;
    case Blob::BlobType::PARALYTIC_GAS:          return &s_paralyticGas;
    case Blob::BlobType::FIRE:                   return &s_fire;
    case Blob::BlobType::CONFUSION_GAS:          return &s_confusionGas;
    case Blob::BlobType::CORROSIVE_GAS:          return &s_corrosiveGas;
    case Blob::BlobType::REGROWTH:               return &s_regrowth;
    case Blob::BlobType::FOLIAGE:                return &s_foliage;
    case Blob::BlobType::WATER_OF_HEALTH:        return &s_waterOfHealth;
    case Blob::BlobType::WATER_OF_AWARENESS:     return &s_waterOfAwareness;
    case Blob::BlobType::WATER_OF_TRANSMUTATION: return &s_waterOfTransmutation;
    default:                                     return nullptr;
    }
}

void FX::missile(int fromCell, int toCell, int itemImage, std::function<void()> callback) {
    auto m = std::make_unique<MissileSprite>();
    m->reset(fromCell, toCell, itemImage, std::move(callback));
    s_missiles.push_back(std::move(m));
}

void FX::magicMissile(int fromCell, int toCell, uint32_t color, std::function<void()> callback) {
    s_magicMissile.active = true;
    s_magicMissile.startX = (fromCell % Level::WIDTH) * 16.0f + 8.0f;
    s_magicMissile.startY = (fromCell / Level::WIDTH) * 16.0f + 8.0f;
    s_magicMissile.targetX = (toCell % Level::WIDTH) * 16.0f + 8.0f;
    s_magicMissile.targetY = (toCell / Level::WIDTH) * 16.0f + 8.0f;
    s_magicMissile.curX = s_magicMissile.startX;
    s_magicMissile.curY = s_magicMissile.startY;
    float dx = s_magicMissile.targetX - s_magicMissile.startX;
    float dy = s_magicMissile.targetY - s_magicMissile.startY;
    float dist = std::sqrt(dx * dx + dy * dy);
    s_magicMissile.duration = std::clamp(dist / 220.0f, 0.12f, 0.35f);
    s_magicMissile.elapsed = 0.0f;
    s_magicMissile.color = color;
    s_magicMissile.callback = std::move(callback);
}

void FX::lightning(const std::vector<int>& points, std::function<void()> callback) {
    s_lightning.active = true;
    s_lightning.points = points;
    s_lightning.time = 0.3f;
    s_lightning.callback = std::move(callback);
}

void FX::deathRay(int fromCell, int toCell, std::function<void()> callback) {
    s_deathRay.active = true;
    s_deathRay.fromCell = fromCell;
    s_deathRay.toCell = toCell;
    s_deathRay.time = 0.5f;
    s_deathRay.callback = nullptr;
    if (callback) callback(); // DeathRay applies damage and affects terrain immediately matching Java
}

bool FX::isMissileActive() {
    return !s_missiles.empty() || s_magicMissile.active || s_lightning.active;
}

} // namespace pd

