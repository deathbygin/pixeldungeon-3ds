#include "pd/CharSprite.h"
#include "pd/Char.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"
#include "pd/Assets.h"
#include "noosa/Texture.h"
#include <algorithm>
#include <cmath>

namespace pd {

CharSprite::CharSprite() {
    m_movieClip.targetScreen = noosa::ScreenTarget::Top;
    m_movieClip.z = 0.60f;
}

void CharSprite::init(const std::string& texturePath, int frameWidth, int frameHeight) {
    m_width = static_cast<float>(frameWidth);
    m_height = static_cast<float>(frameHeight);

    auto tex = noosa::Texture::get(texturePath);
    if (tex && tex->isLoaded()) {
        m_film = std::make_shared<noosa::TextureFilm>(tex, frameWidth, frameHeight);
        m_movieClip.setFilm(m_film);
    }

    m_movieClip.setCallback([this](noosa::MovieClip*) {
        if (m_state == State::Attack || m_state == State::Operate || m_state == State::Zap) {
            idle();
        } else if (m_state == State::Die) {
            if (!(m_char && m_char->asHero())) {
                m_isDead = true;
            }
        }
    });
}

void CharSprite::link(Char* ch) {
    m_char = ch;
    if (m_char) {
        place(m_char->pos);
    }
}

void CharSprite::place(int cell) {
    int cx = cell % Level::WIDTH;
    int cy = cell / Level::WIDTH;

    m_worldX = (cx + 0.5f) * 16.0f - m_width * 0.5f;
    m_worldY = (cy + 1.0f) * 16.0f - m_height;

    m_movieClip.x = m_worldX;
    m_movieClip.y = m_worldY;

    m_isMoving = false;
    m_moveTimer = 0.0f;
}

void CharSprite::move(int from, int to) {
    turnTo(from, to);

    int fx = from % Level::WIDTH;
    int fy = from / Level::WIDTH;
    int tx = to % Level::WIDTH;
    int ty = to / Level::WIDTH;

    m_startX = (fx + 0.5f) * 16.0f - m_width * 0.5f;
    m_startY = (fy + 1.0f) * 16.0f - m_height;
    m_targetX = (tx + 0.5f) * 16.0f - m_width * 0.5f;
    m_targetY = (ty + 1.0f) * 16.0f - m_height;

    m_worldX = m_startX;
    m_worldY = m_startY;
    m_movieClip.x = m_worldX;
    m_movieClip.y = m_worldY;

    m_isMoving = true;
    m_moveTimer = MOVE_INTERVAL;

    m_state = State::Run;
    if (!m_run.frames.empty()) {
        m_movieClip.play(m_run);
    }
}

void CharSprite::attack(int targetCell) {
    if (m_char) {
        turnTo(m_char->pos, targetCell);
    }

    m_state = State::Attack;
    if (!m_attack.frames.empty()) {
        m_movieClip.play(m_attack, true);
    }
}

void CharSprite::die() {
    m_state = State::Die;
    if (m_char && m_char->asHero()) {
        m_isDying = false;
        m_movieClip.alpha = 1.0f;
    } else {
        m_isDying = true;
        m_fadeTimer = FADE_TIME;
    }
    if (!m_die.frames.empty()) {
        m_movieClip.play(m_die, true);
    }
}

void CharSprite::idle() {
    m_state = State::Idle;
    if (!m_idle.frames.empty()) {
        m_movieClip.play(m_idle);
    }
}

void CharSprite::turnTo(int from, int to) {
    int fx = from % Level::WIDTH;
    int tx = to % Level::WIDTH;
    if (tx < fx) {
        m_movieClip.flipHorizontal = true;
    } else if (tx > fx) {
        m_movieClip.flipHorizontal = false;
    }
}

void CharSprite::update(float dt) {
    if (m_isMoving) {
        m_moveTimer -= dt;
        float progress = 1.0f - std::max(0.0f, m_moveTimer / MOVE_INTERVAL);
        progress = std::clamp(progress, 0.0f, 1.0f);

        m_worldX = m_startX + (m_targetX - m_startX) * progress;
        m_worldY = m_startY + (m_targetY - m_startY) * progress;

        if (m_moveTimer <= 0.0f) {
            m_isMoving = false;
            m_worldX = m_targetX;
            m_worldY = m_targetY;
            if (m_state == State::Run) {
                idle();
            }
        }
    } else if (m_char && !m_isDying) {
        // Keep grounded on cell
        int cx = m_char->pos % Level::WIDTH;
        int cy = m_char->pos / Level::WIDTH;
        m_worldX = (cx + 0.5f) * 16.0f - m_width * 0.5f;
        m_worldY = (cy + 1.0f) * 16.0f - m_height;
        if (m_char->flying) {
            m_worldY -= 3.0f; // 3-pixel hover offset for levitating characters
        }
    }

    m_movieClip.x = m_worldX;
    m_movieClip.y = m_worldY;

    if (m_isDying) {
        m_fadeTimer -= dt;
        m_movieClip.alpha = std::clamp(m_fadeTimer / FADE_TIME, 0.0f, 1.0f);
        if (m_fadeTimer <= 0.0f) {
            m_isDead = true;
        }
    }

    m_movieClip.update(dt);
}

void CharSprite::draw(const noosa::Camera* camera) {
    if (m_char && !m_isDying) {
        if (m_char->getBuff<Burning>()) {
            m_movieClip.tint = noosa::Color(255, 140, 60, 255);
        } else if (m_char->getBuff<Frost>()) {
            m_movieClip.tint = noosa::Color(100, 190, 255, 255);
        } else if (m_char->paralysed) {
            m_movieClip.tint = noosa::Color(255, 255, 120, 255);
        } else {
            m_movieClip.tint = noosa::Color(255, 255, 255, 255);
        }

        if (m_char->invisible > 0) {
            m_movieClip.alpha = 0.40f;
        } else {
            m_movieClip.alpha = 1.0f;
        }
    }

    m_movieClip.draw(camera);
}

// =============================================================================
// HeroSprite Implementation
// =============================================================================

void HeroSprite::initHero(int heroClass, int tier) {
    m_heroClass = heroClass;
    m_tier = tier;

    std::string sheet = Assets::WARRIOR;
    if (heroClass == static_cast<int>(Hero::HeroClass::Mage)) {
        sheet = Assets::MAGE;
    } else if (heroClass == static_cast<int>(Hero::HeroClass::Rogue)) {
        sheet = Assets::ROGUE;
    } else if (heroClass == static_cast<int>(Hero::HeroClass::Huntress)) {
        sheet = Assets::HUNTRESS;
    }

    init(sheet, 12, 15);
    auto tex = noosa::Texture::get(sheet);
    if (tex && tex->isLoaded()) {
        m_cols = tex->width() / 12;
    }

    updateArmor(tier);
}

void HeroSprite::updateArmor(int tier) {
    m_tier = tier;
    int base = tier * m_cols;

    m_idle = noosa::Animation(1.0f, { base + 0, base + 0, base + 0, base + 1, base + 0, base + 0, base + 1, base + 1 }, true);
    m_run = noosa::Animation(1.0f / 20.0f, { base + 2, base + 3, base + 4, base + 5, base + 6, base + 7 }, true);
    m_attack = noosa::Animation(1.0f / 15.0f, { base + 13, base + 14, base + 15, base + 0 }, false);
    m_die = noosa::Animation(1.0f / 20.0f, { base + 8, base + 9, base + 10, base + 11, base + 12, base + 11 }, false);
    m_operate = noosa::Animation(1.0f / 8.0f, { base + 16, base + 17, base + 16, base + 17 }, false);
    m_fly = noosa::Animation(1.0f, { base + 18 }, true);
    m_read = noosa::Animation(1.0f / 20.0f, { base + 19, base + 20, base + 20, base + 20, base + 20, base + 20, base + 20, base + 20, base + 20, base + 19 }, false);
    m_zap = m_attack;

    idle();
}

void HeroSprite::operate(int cell) {
    if (m_char) {
        turnTo(m_char->pos, cell);
    }
    m_state = State::Operate;
    m_movieClip.play(m_operate, true);
}

void HeroSprite::read() {
    m_state = State::Operate;
    m_movieClip.play(m_read, true);
}

void HeroSprite::update(float dt) {
    if (m_char) {
        Hero* hero = static_cast<Hero*>(m_char);
        if (hero->tier() != m_tier) {
            updateArmor(hero->tier());
        }
        if (hero->flying && !m_isMoving && m_state == State::Idle) {
            m_movieClip.play(m_fly);
        }
    }
    CharSprite::update(dt);
}

// =============================================================================
// Factory Helpers
// =============================================================================

std::unique_ptr<CharSprite> CharSprite::createForHero(Hero* hero) {
    auto sprite = std::make_unique<HeroSprite>();
    if (hero) {
        sprite->initHero(static_cast<int>(hero->heroClass), hero->tier());
        sprite->link(hero);
    }
    return sprite;
}

std::unique_ptr<CharSprite> CharSprite::createForMob(Mob* mob) {
    auto sprite = std::make_unique<CharSprite>();
    if (!mob) return sprite;

    switch (mob->mobType()) {
        case Mob::MobType::Rat:
            sprite->init(Assets::RAT, 16, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 6, 7, 8, 9, 10 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 2, 3, 4, 5, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 10.0f, { 11, 12, 13, 14 }, false);
            break;

        case Mob::MobType::Gnoll:
            sprite->init(Assets::GNOLL, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1, 0, 0, 1, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 4, 5, 6, 7 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 2, 3, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 8, 9, 10 }, false);
            break;

        case Mob::MobType::Crab:
            sprite->init(Assets::CRAB, 16, 16);
            sprite->m_idle = noosa::Animation(1.0f / 5.0f, { 0, 1, 0, 2 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 3, 4, 5, 6 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 7, 8, 9 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 10, 11, 12, 13 }, false);
            break;

        case Mob::MobType::Skeleton:
            sprite->init(Assets::SKELETON, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 12.0f, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 4, 5, 6, 7, 8, 9 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 14, 15, 16 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 10, 11, 12, 13 }, false);
            break;

        case Mob::MobType::Swarm:
            sprite->init(Assets::SWARM, 16, 16);
            sprite->m_idle = noosa::Animation(1.0f / 15.0f, { 0, 1, 2, 3, 4, 5 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 0, 1, 2, 3, 4, 5 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 20.0f, { 6, 7, 8, 9 }, false);
            sprite->m_die = noosa::Animation(1.0f / 15.0f, { 10, 11, 12, 13, 14 }, false);
            break;

        case Mob::MobType::Thief:
            sprite->init(Assets::THIEF, 12, 13);
            sprite->m_idle = noosa::Animation(1.0f, { 0, 0, 0, 1, 0, 0, 0, 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 0, 0, 2, 3, 3, 4 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 10, 11, 12, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 10.0f, { 5, 6, 7, 8, 9 }, false);
            break;

        case Mob::MobType::Shaman:
            sprite->init(Assets::SHAMAN, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1, 0, 0, 1, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 4, 5, 6, 7 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 2, 3, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 8, 9, 10 }, false);
            break;

        case Mob::MobType::Bat:
            sprite->init(Assets::BAT, 15, 15);
            sprite->m_idle = noosa::Animation(1.0f / 8.0f, { 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 0, 1 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 2, 3, 0, 1 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 4, 5, 6 }, false);
            break;

        case Mob::MobType::Brute:
            sprite->init(Assets::BRUTE, 12, 16);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1, 0, 0, 1, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 4, 5, 6, 7 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 2, 3, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 8, 9, 10 }, false);
            break;

        case Mob::MobType::Monk:
            sprite->init(Assets::MONK, 15, 14);
            sprite->m_idle = noosa::Animation(1.0f / 6.0f, { 1, 0, 1, 2 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 11, 12, 13, 14, 15, 16 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 3, 4, 3, 4 }, false);
            sprite->m_die = noosa::Animation(1.0f / 15.0f, { 1, 7, 8, 8, 9, 10 }, false);
            break;

        case Mob::MobType::Warlock:
            sprite->init(Assets::WARLOCK, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1, 0, 0, 1, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 0, 2, 3, 4 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 0, 5, 6 }, false);
            sprite->m_die = noosa::Animation(1.0f / 15.0f, { 0, 7, 8, 8, 9, 10 }, false);
            break;

        case Mob::MobType::Golem:
            sprite->init(Assets::GOLEM, 16, 16);
            sprite->m_idle = noosa::Animation(1.0f / 4.0f, { 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 2, 3, 4, 5 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 10.0f, { 6, 7, 8 }, false);
            sprite->m_die = noosa::Animation(1.0f / 15.0f, { 9, 10, 11, 12, 13 }, false);
            break;

        case Mob::MobType::Succubus:
            sprite->init(Assets::SUCCUBUS, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 8.0f, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 3, 4, 5, 6, 7, 8 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 9, 10, 11 }, false);
            sprite->m_die = noosa::Animation(1.0f / 10.0f, { 12 }, false);
            break;

        case Mob::MobType::Eye:
            sprite->init(Assets::EYE, 16, 18);
            sprite->m_idle = noosa::Animation(1.0f / 8.0f, { 0, 1, 2 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 5, 6 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 8.0f, { 4, 3 }, false);
            sprite->m_die = noosa::Animation(1.0f / 8.0f, { 7, 8, 9 }, false);
            break;

        case Mob::MobType::Scorpio:
            sprite->init(Assets::SCORPIO, 18, 17);
            sprite->m_idle = noosa::Animation(1.0f / 12.0f, { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 2, 1, 2 }, true);
            sprite->m_run = noosa::Animation(1.0f / 8.0f, { 5, 5, 6, 6 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 0, 3, 4 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 0, 7, 8, 9, 10 }, false);
            break;

        case Mob::MobType::Piranha:
            sprite->init(Assets::PIRANHA, 12, 16);
            sprite->m_idle = noosa::Animation(1.0f / 8.0f, { 0, 1, 2, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 20.0f, { 0, 1, 2, 1 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 20.0f, { 3, 4, 5, 6, 7, 8, 9, 10, 11 }, false);
            sprite->m_die = noosa::Animation(1.0f / 4.0f, { 12, 13, 14 }, false);
            break;

        case Mob::MobType::Statue:
            sprite->init(Assets::STATUE, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 0, 0, 1, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 2, 3, 4, 5, 6, 7 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 8, 9, 10 }, false);
            sprite->m_die = noosa::Animation(1.0f / 5.0f, { 11, 12, 13, 14, 15, 15 }, false);
            break;

        case Mob::MobType::Wraith:
            sprite->init(Assets::WRAITH, 14, 15);
            sprite->m_idle = noosa::Animation(1.0f / 5.0f, { 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 0, 1 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 10.0f, { 0, 2, 3 }, false);
            sprite->m_die = noosa::Animation(1.0f / 8.0f, { 0, 4, 5, 6, 7 }, false);
            break;

        case Mob::MobType::Goo:
            sprite->init(Assets::GOO, 20, 14);
            sprite->m_idle = noosa::Animation(1.0f / 10.0f, { 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 0, 1 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 10.0f, { 5, 0, 6 }, false);
            sprite->m_die = noosa::Animation(1.0f / 10.0f, { 2, 3, 4 }, false);
            break;

        case Mob::MobType::Tengu:
            sprite->init(Assets::TENGU, 14, 16);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 2, 3, 4, 5, 0 }, false);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 6, 7, 7, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 8.0f, { 8, 9, 10, 10, 10, 10, 10, 10 }, false);
            break;

        case Mob::MobType::DM300:
            sprite->init(Assets::DM300, 22, 20);
            sprite->m_idle = noosa::Animation(1.0f / 10.0f, { 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 2, 3 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 4, 5, 6 }, false);
            sprite->m_die = noosa::Animation(1.0f / 20.0f, { 0, 7, 0, 7, 0, 7, 0, 7, 0, 7, 0, 7, 8 }, false);
            break;

        case Mob::MobType::King:
            sprite->init(Assets::KING, 16, 16);
            sprite->m_idle = noosa::Animation(1.0f / 12.0f, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2 }, true);
            sprite->m_run = noosa::Animation(1.0f / 15.0f, { 3, 4, 5, 6, 7, 8 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 9, 10, 11 }, false);
            sprite->m_die = noosa::Animation(1.0f / 8.0f, { 12, 13, 14, 15 }, false);
            break;

        case Mob::MobType::Yog:
            sprite->init(Assets::YOG, 20, 19);
            sprite->m_idle = noosa::Animation(1.0f / 10.0f, { 0, 1, 2, 2, 1, 0, 3, 4, 4, 3, 0, 5, 6, 6, 5 }, true);
            sprite->m_run = noosa::Animation(1.0f / 12.0f, { 0 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 12.0f, { 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 10.0f, { 0, 7, 8, 9 }, false);
            break;

        case Mob::MobType::MirrorImage: {
            std::string texPath = Assets::WARRIOR;
            if (Dungeon::hero) {
                switch (Dungeon::hero->heroClass) {
                    case Hero::HeroClass::Warrior:  texPath = Assets::WARRIOR; break;
                    case Hero::HeroClass::Mage:     texPath = Assets::MAGE; break;
                    case Hero::HeroClass::Rogue:    texPath = Assets::ROGUE; break;
                    case Hero::HeroClass::Huntress: texPath = Assets::HUNTRESS; break;
                }
            }
            sprite->init(texPath, 12, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 2, 3, 4, 5, 6, 7 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 8, 9, 10, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 12.0f, { 11, 12, 13, 14 }, false);
            break;
        }

        case Mob::MobType::Sheep:
            sprite->init(Assets::SHEEP, 16, 15);
            sprite->m_idle = noosa::Animation(1.0f / 8.0f, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 0 }, true);
            sprite->m_run = sprite->m_idle;
            sprite->m_attack = sprite->m_idle;
            sprite->m_die = noosa::Animation(1.0f / 20.0f, { 0 }, false);
            break;

        case Mob::MobType::Mimic:
            sprite->init(Assets::MIMIC, 16, 16);
            sprite->m_idle = noosa::Animation(1.0f / 5.0f, { 0, 0, 0, 1, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 0, 1, 2, 3, 3, 2, 1 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 10.0f, { 0, 4, 5, 6 }, false);
            sprite->m_die = noosa::Animation(1.0f / 5.0f, { 7, 8, 9 }, false);
            break;

        default:
            sprite->init(Assets::RAT, 16, 15);
            sprite->m_idle = noosa::Animation(1.0f / 2.0f, { 0, 0, 0, 1 }, true);
            sprite->m_run = noosa::Animation(1.0f / 10.0f, { 6, 7, 8, 9, 10 }, true);
            sprite->m_attack = noosa::Animation(1.0f / 15.0f, { 2, 3, 4, 5, 0 }, false);
            sprite->m_die = noosa::Animation(1.0f / 10.0f, { 11, 12, 13, 14 }, false);
            break;
    }

    sprite->idle();
    sprite->link(mob);
    return sprite;
}

} // namespace pd

