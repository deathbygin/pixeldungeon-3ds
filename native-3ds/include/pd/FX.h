#pragma once

#include "pd/PixelParticle.h"
#include "pd/Ripple.h"
#include "pd/CheckedCell.h"
#include "pd/Wound.h"
#include "pd/FloatingText.h"
#include "pd/Blob.h"
#include "noosa/Camera.h"
#include <citro2d.h>
#include <string>
#include <vector>
#include <functional>

namespace pd {

class Level;

class FX {
public:
    static constexpr size_t MAX_PARTICLES = 512;
    static constexpr size_t MAX_RIPPLES   = 16;
    static constexpr size_t MAX_CHECKED   = 64;
    static constexpr size_t MAX_WOUNDS    = 16;
    static constexpr size_t MAX_TEXTS     = 16;

    static void init();
    static void clear();
    static void update(float dt);
    static void draw(const noosa::Camera* camera, C2D_TextBuf textBuf);
    static void drawGround(const noosa::Camera* camera);

    // Spawners
    static void splash(int cell, uint32_t color = 0x73eff7FF, int count = 8);
    static void blood(int cell, int count = 8);
    static void waterSplash(int cell, int count = 8);
    static void slash(int cell);
    static void ripple(int cell);
    static void searchCell(int cell);
    static void floatingText(int cell, const std::string& str, uint32_t color);
    static void burst(float wx, float wy, uint32_t color, int count,
                      float speedMin = 30.0f, float speedMax = 70.0f, float gravity = 80.0f);
    static void burst(int cell, uint32_t color, int count,
                      float speedMin = 30.0f, float speedMax = 70.0f, float gravity = 80.0f) {
        float wx = (cell % 32) * 16.0f + 8.0f;
        float wy = (cell / 32) * 16.0f + 8.0f;
        burst(wx, wy, color, count, speedMin, speedMax, gravity);
    }

    // Blobs (gas & fire)
    static void seedBlob(int cell, Blob::BlobType type, int amount = 100);
    static void evolveBlobs(const Level* level);
    static Blob* getBlob(Blob::BlobType type);

    // Missile Projectile
    static void missile(int fromCell, int toCell, int itemImage, std::function<void()> callback = nullptr);
    static bool isMissileActive();

private:
    static PixelParticle s_particles[MAX_PARTICLES];
    static Ripple s_ripples[MAX_RIPPLES];
    static CheckedCell s_checked[MAX_CHECKED];
    static Wound s_wounds[MAX_WOUNDS];
    static FloatingText s_texts[MAX_TEXTS];
    static std::vector<std::unique_ptr<class MissileSprite>> s_missiles;

    static Blob s_toxicGas;
    static Blob s_paralyticGas;
    static Blob s_fire;
    static Blob s_confusionGas;
    static Blob s_corrosiveGas;
    static Blob s_regrowth;
    static Blob s_foliage;
    static Blob s_waterOfHealth;
    static Blob s_waterOfAwareness;
    static Blob s_waterOfTransmutation;

    static float s_blobEmitTimer;
};

} // namespace pd

