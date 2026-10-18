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
#include "pd/GLog.h"
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
    if (paralysed && !buff(BuffType::Paralysis) && !buff(BuffType::Frost)) {
        const_cast<Char*>(this)->paralysed = false;
    }
    if (rooted && !buff(BuffType::Roots)) {
        const_cast<Char*>(this)->rooted = false;
    }
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
    } else if (Dungeon::hero) {
        Dungeon::hero->clearAutoPickDisabled();
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

    // Stepping into water immediately extinguishes Burning and washes away Ooze
    if (Dungeon::level && step >= 0 && step < Level::LENGTH && Dungeon::level->water[step] && !flying) {
        FX::ripple(step);
        FX::waterSplash(step, 4);

        if (buff(BuffType::Burning)) {
            removeBuff(BuffType::Burning);
            FX::waterSplash(step, 6);
        }
        if (buff(BuffType::Ooze)) {
            removeBuff(BuffType::Ooze);
            if (this == Dungeon::hero.get()) {
                GLog::p("The water washes away the caustic ooze!");
            }
        }
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
        if (heroAttacker && heroAttacker->belongings.equippedWeapon && heroAttacker->belongings.equippedWeapon->name == "war hammer") {
            enemyDr /= 2; // War hammer crushing armor penetration
        }
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

        spend(attackDelay());
        return true;
    } else {
        // Sound effect for miss/dodge
        Sample::playAt(Assets::SND_MISS, enemy->pos);

        FX::floatingText(enemy->pos, "MISS", 0x8b9bb4FF);
        spend(attackDelay());
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

void Char::unroot() {
    if (!buff(BuffType::Roots)) {
        rooted = false;
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

    if (isImmune(b->type())) {
        delete b;
        return;
    }

    Buff* existing = buff(b->type());
    if (existing) {
        if (existing == b) return;

        switch (b->type()) {
        case BuffType::Burning: {
            Burning* eb = static_cast<Burning*>(existing);
            Burning* nb = static_cast<Burning*>(b);
            eb->durationTicks = std::max(eb->durationTicks, nb->durationTicks);
            break;
        }
        case BuffType::Poison: {
            Poison* eb = static_cast<Poison*>(existing);
            Poison* nb = static_cast<Poison*>(b);
            eb->durationTicks = std::max(eb->durationTicks, nb->durationTicks);
            eb->damagePerTick = std::max(eb->damagePerTick, nb->damagePerTick);
            break;
        }
        case BuffType::Bleeding: {
            Bleeding* eb = static_cast<Bleeding*>(existing);
            Bleeding* nb = static_cast<Bleeding*>(b);
            eb->level = std::max(eb->level, nb->level);
            break;
        }
        case BuffType::Paralysis: {
            Paralysis* eb = static_cast<Paralysis*>(existing);
            Paralysis* nb = static_cast<Paralysis*>(b);
            eb->durationTicks = std::max(eb->durationTicks, nb->durationTicks);
            break;
        }
        case BuffType::Ooze: {
            Ooze* eb = static_cast<Ooze*>(existing);
            Ooze* nb = static_cast<Ooze*>(b);
            eb->damage = std::min(10, eb->damage + nb->damage);
            break;
        }
        case BuffType::Weakness: {
            existing->postpone(Weakness::DURATION);
            break;
        }
        case BuffType::Frost: {
            existing->postpone(Frost::DURATION);
            break;
        }
        case BuffType::Cripple: {
            existing->postpone(Cripple::DURATION);
            break;
        }
        case BuffType::Slow: {
            existing->postpone(Slow::DURATION);
            break;
        }
        case BuffType::Speed: {
            existing->postpone(Speed::DURATION);
            break;
        }
        case BuffType::Roots: {
            existing->postpone(5.0f);
            break;
        }
        case BuffType::Vertigo: {
            existing->postpone(Vertigo::DURATION);
            break;
        }
        case BuffType::Amok: {
            existing->postpone(Amok::DURATION);
            break;
        }
        case BuffType::Rage: {
            existing->postpone(Rage::DURATION);
            break;
        }
        case BuffType::Terror: {
            existing->postpone(Terror::DURATION);
            break;
        }
        case BuffType::Charm: {
            existing->postpone(Charm::DURATION);
            break;
        }
        case BuffType::Blindness: {
            existing->postpone(Blindness::DURATION);
            break;
        }
        case BuffType::Barkskin: {
            Barkskin* eb = static_cast<Barkskin*>(existing);
            Barkskin* nb = static_cast<Barkskin*>(b);
            eb->setLevel(nb->level);
            break;
        }
        case BuffType::EarthrootArmor: {
            EarthrootArmor* eb = static_cast<EarthrootArmor*>(existing);
            EarthrootArmor* nb = static_cast<EarthrootArmor*>(b);
            eb->setLevel(nb->level);
            break;
        }
        case BuffType::Light: {
            Light* el = static_cast<Light*>(existing);
            el->prolong(b->duration() > 0.0f ? b->duration() : Light::DURATION);
            break;
        }
        default: {
            if (b->duration() > 0.0f) {
                existing->postpone(b->duration());
            }
            break;
        }
        }

        delete b;
        return;
    }

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
