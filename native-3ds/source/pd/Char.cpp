#include "pd/Char.h"
#include "pd/CharSprite.h"
#include "pd/Buff.h"
#include "pd/Random.h"
#include "pd/FX.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"
#include "pd/Terrain.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/Debug.h"
#include <algorithm>

namespace pd {

Char::Char() {
    Actor::add(this);
}

Char::~Char() {
    Actor::remove(this);
    if (pos >= 0 && pos < PathFinder::LENGTH && Actor::chars[pos] == this) {
        Actor::chars[pos] = nullptr;
    }
    while (!buffs.empty()) {
        Buff* b = buffs.back();
        buffs.pop_back();
        Actor::remove(b);
        delete b;
    }
}

bool Char::act() {
    return false;
}

float Char::speed() const {
    if (paralysed || rooted) return 0.0f;
    float s = baseSpeed;
    if (buff(BuffType::Cripple) != nullptr) {
        s *= 0.5f;
    }
    return s;
}

void Char::interrupt() {
    // Overridden by Hero to abort resting / continuous actions
}

CharSprite* Char::getSprite() {
    return sprite.get();
}

void Char::place(int cell) {
    if (pos >= 0 && pos < PathFinder::LENGTH && Actor::chars[pos] == this) {
        Actor::chars[pos] = nullptr;
    }
    pos = cell;
    if (pos >= 0 && pos < PathFinder::LENGTH) {
        Actor::chars[pos] = this;
    }
    CharSprite* spr = getSprite();
    if (spr) {
        spr->place(cell);
    }
    if (this != Dungeon::hero.get()) {
        spriteVisible = (pos >= 0 && pos < Level::LENGTH) ? Dungeon::visible[pos] : false;
    }
}

void Char::move(int step) {
    int prevPos = pos;
    if (pos >= 0 && pos < PathFinder::LENGTH && Actor::chars[pos] == this) {
        Actor::chars[pos] = nullptr;
    }
    pos = step;
    if (pos >= 0 && pos < PathFinder::LENGTH) {
        Actor::chars[pos] = this;
    }

    CharSprite* spr = getSprite();
    if (spr) {
        spr->move(prevPos, step);
    }

    if (this != Dungeon::hero.get()) {
        spriteVisible = (pos >= 0 && pos < Level::LENGTH) ? Dungeon::visible[pos] : false;
    }

    if (Dungeon::level && step >= 0 && step < Level::LENGTH) {
        if (Dungeon::level->get(step) == Terrain::HIGH_GRASS) {
            Dungeon::level->trample(step, this);
        }
    }

    // Detach stationary plant buffs upon moving away
    EarthrootArmor* earthArmor = getBuff<EarthrootArmor>();
    if (earthArmor) {
        earthArmor->detach();
    }
    HerbalHealing* herbalHealing = getBuff<HerbalHealing>();
    if (herbalHealing) {
        herbalHealing->detach();
    }

    // Bleeding damage upon movement
    Bleeding* b = getBuff<Bleeding>();
    if (b && isAlive()) {
        damage(Random::Int(b->level), b);
    }
}

int Char::attackSkill(Char* target) {
    (void)target;
    return 10;
}

int Char::defenseSkill(Char* attacker) {
    (void)attacker;
    return 5;
}

int Char::dr() {
    return 0;
}

int Char::damageRoll() {
    return Random::IntRange(1, 4);
}

int Char::attackProc(Char* enemy, int damage) {
    (void)enemy;
    return damage;
}

int Char::defenseProc(Char* enemy, int damage) {
    (void)enemy;
    return damage;
}

bool Char::attack(Char* enemy) {
    if (!enemy || !enemy->isAlive()) return false;

    CharSprite* spr = getSprite();
    if (spr) {
        spr->attack(enemy->pos);
    }
    CharSprite* espr = enemy->getSprite();
    if (espr) {
        espr->turnTo(enemy->pos, pos);
    }

    // Check for surprise attack BEFORE breaking invisibility/shadows
    Buff* inv = buff(BuffType::Invisibility);
    Buff* shd = buff(BuffType::Shadows);
    bool surprise = (inv != nullptr) || (shd != nullptr) || (invisible > 0);

    Mob* enemyMob = enemy->asMob();
    if (enemyMob && (!enemyMob->enemySeen || enemyMob->paralysed || enemyMob->state == Mob::State::SLEEPING)) {
        surprise = true;
    }

    if (inv) inv->detach();
    if (shd) shd->detach();

    float acuRoll = Random::Float(static_cast<float>(attackSkill(enemy)));
    float defRoll = surprise ? 0.0f : Random::Float(static_cast<float>(enemy->defenseSkill(this)));

    if (acuRoll >= defRoll) {
        int dmg = damageRoll();
        Hero* heroAttacker = (this == Dungeon::hero.get()) ? Dungeon::hero.get() : nullptr;
        int enemyDr = (heroAttacker && heroAttacker->rangedWeapon && heroAttacker->subClass == HeroSubClass::Sniper)
            ? 0
            : Random::IntRange(0, enemy->dr());
        int effective = std::max(0, dmg - enemyDr);

        effective = attackProc(enemy, effective);
        effective = enemy->defenseProc(this, effective);

        if (heroAttacker && Debug::oneHitKill) {
            effective = std::max(effective, enemy->HP);
        }

        enemy->damage(effective, this);

        // Sound effect for hit matching 1:1 Pixel Dungeon with authentic pitch variance
        Sample::playAt(Assets::SND_HIT, enemy->pos, 1.0f, Random::Float(0.8f, 1.25f));

        // Visual effects for combat
        FX::slash(enemy->pos);
        FX::blood(enemy->pos, 8);
        FX::floatingText(enemy->pos, std::to_string(effective), 0xe43b44FF);

        spend(TICK / speed());
        return true;
    } else {
        // Sound effect for miss/dodge
        Sample::playAt(Assets::SND_MISS, enemy->pos);

        FX::floatingText(enemy->pos, "MISS", 0x8b9bb4FF);
        spend(TICK / speed());
        return false;
    }
}

void Char::spend(float time) {
    float timeScale = 1.0f;
    if (buff(BuffType::Slow) != nullptr) {
        timeScale *= 0.5f;
    }
    if (buff(BuffType::Speed) != nullptr) {
        timeScale *= 2.0f;
    }
    Actor::spend(time / timeScale);
}

void Char::unfreeze() {
    if (!buff(BuffType::Paralysis) && !buff(BuffType::Frost)) {
        paralysed = false;
    }
}

bool Char::isCharmedBy(Char* ch) const {
    if (!ch) return false;
    Charm* c = getBuff<Charm>();
    return c && c->charmer == ch;
}

void Char::damage(int dmg, Actor* src) {
    (void)src;
    interrupt();

    // Damage reactions on active buffs:
    // 1. Frost melts immediately upon taking any damage
    Frost* frostBuff = getBuff<Frost>();
    if (frostBuff && dmg > 0) {
        frostBuff->detach();
    }

    // 2. Sleep awakens immediately upon taking any damage
    Sleep* sleepBuff = getBuff<Sleep>();
    if (sleepBuff && dmg > 0) {
        sleepBuff->detach();
    }

    // 3. Paralysis has a chance to break upon taking damage
    Paralysis* pBuff = getBuff<Paralysis>();
    if (pBuff && dmg > 0 && HT > 0) {
        if (Random::Int(dmg) >= Random::Int(HP)) {
            pBuff->detach();
        }
    }

    // 4. Earthroot Armor damage absorption
    EarthrootArmor* armor = getBuff<EarthrootArmor>();
    if (armor && dmg > 0) {
        dmg = armor->absorb(dmg);
    }

    HP = std::max(0, HP - dmg);
    if (HP <= 0) {
        die(src);
    }
}

void Char::destroy() {
    HP = 0;
    if (pos >= 0 && pos < PathFinder::LENGTH && Actor::chars[pos] == this) {
        Actor::chars[pos] = nullptr;
    }
    Actor::remove(this);
}

void Char::die(Actor* src) {
    (void)src;
    destroy();
    CharSprite* spr = getSprite();
    if (spr) {
        spr->die();
    }
}

void Char::addBuff(Buff* b) {
    if (!b) return;
    if (std::find(buffs.begin(), buffs.end(), b) == buffs.end()) {
        buffs.push_back(b);
    }
    b->attachTo(this);
}

void Char::removeBuff(Buff* b) {
    if (!b) return;
    auto it = std::find(buffs.begin(), buffs.end(), b);
    if (it != buffs.end()) {
        buffs.erase(it);
    }
}

void Char::removeBuff(BuffType type) {
    while (Buff* b = buff(type)) {
        b->detach();
    }
}

Buff* Char::buff(BuffType type) const {
    for (Buff* b : buffs) {
        if (b && b->type() == type) {
            return b;
        }
    }
    return nullptr;
}

} // namespace pd
