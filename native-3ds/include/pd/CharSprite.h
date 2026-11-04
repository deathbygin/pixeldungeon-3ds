#pragma once

#include "noosa/MovieClip.h"
#include "noosa/Camera.h"
#include "pd/Buff.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pd {

class Char;
class Hero;
class Mob;
class HeroSprite;

class CharSprite {
public:
    enum class State {
        Idle,
        Run,
        Attack,
        Die,
        Operate,
        Zap
    };

    CharSprite();
    virtual ~CharSprite() = default;

    virtual void init(const std::string& texturePath, int frameWidth, int frameHeight);

    void link(Char* ch);

    // Coordinate conversion matching Pixel Dungeon worldToCamera:
    // x = (cell % WIDTH + 0.5f) * 16.0f - width * 0.5f
    // y = (cell / WIDTH + 1.0f) * 16.0f - height
    void place(int cell);
    void move(int from, int to);

    // Leaps along a parabola, calling back on landing. Height and duration match Java's
    // CharSprite.jump (distance * 4 pixels over distance * 0.1 seconds).
    void jump(int from, int to, std::function<void()> callback);
    void attack(int targetCell);
    void die();
    void idle();
    void turnTo(int from, int to);

    virtual void update(float dt);
    virtual void draw(const noosa::Camera* camera);

    // Shadow cast on the floor while airborne, drawn with the ground layer
    void drawJumpShadow(const noosa::Camera* camera) const;

    bool isMoving() const { return m_isMoving; }
    bool isJumping() const { return m_isJumping; }
    bool isDying() const { return m_isDying; }
    bool isDead() const { return m_isDead; }

    float getX() const { return m_worldX; }
    float getY() const { return m_worldY; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }

    void setFlipHorizontal(bool flip) { m_movieClip.flipHorizontal = flip; }
    bool getFlipHorizontal() const { return m_movieClip.flipHorizontal; }

    virtual HeroSprite* asHeroSprite() { return nullptr; }

    noosa::MovieClip& movieClip() { return m_movieClip; }

    static std::unique_ptr<CharSprite> createForHero(Hero* hero);
    static std::unique_ptr<CharSprite> createForMob(Mob* mob);

protected:
    Char* m_char = nullptr;
    noosa::MovieClip m_movieClip;
    std::shared_ptr<noosa::TextureFilm> m_film;

    noosa::Animation m_idle;
    noosa::Animation m_run;
    noosa::Animation m_attack;
    noosa::Animation m_die;
    noosa::Animation m_operate;
    noosa::Animation m_zap;

    State m_state = State::Idle;

    float m_width = 16.0f;
    float m_height = 16.0f;
    float m_worldX = 0.0f;
    float m_worldY = 0.0f;

    // Movement tweening (MOVE_INTERVAL = 0.1s in Pixel Dungeon)
    bool m_isMoving = false;
    float m_startX = 0.0f;
    float m_startY = 0.0f;
    float m_targetX = 0.0f;
    float m_targetY = 0.0f;
    float m_moveTimer = 0.0f;
    static constexpr float MOVE_INTERVAL = 0.10f;

    // Parabolic leap, reusing the movement start/target
    bool m_isJumping = false;
    float m_jumpElapsed = 0.0f;
    float m_jumpDuration = 0.0f;
    float m_jumpHeight = 0.0f;
    float m_jumpAltitude = 0.0f; // 0 on the ground, 1 at the apex
    std::function<void()> m_jumpCallback;

    // Death animation & fade
    bool m_isDying = false;
    bool m_isDead = false;
    float m_fadeTimer = 1.0f;
    static constexpr float FADE_TIME = 1.0f;

    void updatePosition();
};

class HeroSprite : public CharSprite {
public:
    HeroSprite() = default;
    ~HeroSprite() override = default;

    HeroSprite* asHeroSprite() override { return this; }

    void initHero(int heroClass, int tier);
    void updateArmor(int tier);

    void operate(int cell);
    void read();

    void update(float dt) override;

private:
    int m_heroClass = 0;
    int m_tier = 0;
    int m_cols = 21;
    noosa::Animation m_read;
    noosa::Animation m_fly;
};

} // namespace pd

