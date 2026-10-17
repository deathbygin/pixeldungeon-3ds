#pragma once

#include <citro2d.h>
#include <string>
#include <memory>
#include "pd/Badges.h"
#include "noosa/Texture.h"

namespace pd {

class BadgeBanner {
public:
    enum class State {
        INACTIVE,
        FADE_IN,
        STATIC,
        FADE_OUT
    };

    static void show(Badges::Badge badge);
    static void update(float dt);
    static void drawTop(C2D_TextBuf textBuf);
    static bool isActive();

    static void drawBadge(int imageIndex, float x, float y, float scale = 1.0f, float alpha = 1.0f, bool locked = false, float depth = 0.65f);
    static void highlight(int imageIndex, float x, float y, float scale = 1.0f, float depth = 0.66f);

private:
    static State s_state;
    static Badges::Badge s_currentBadge;
    static float s_time;
    static float s_sparkleTime;
    static std::shared_ptr<noosa::Texture> s_texBadges;
    static std::shared_ptr<noosa::Texture> s_texLocked;

    static void initTextures();
};

} // namespace pd
