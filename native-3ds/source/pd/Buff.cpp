#include "pd/Buff.h"
#include "pd/Char.h"
#include "pd/Hero.h"
#include "pd/Dungeon.h"
#include "pd/Terrain.h"
#include "pd/Random.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include <algorithm>

namespace pd {

void Buff::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, TICK);
}

void Buff::detach() {
    if (target) {
        target->removeBuff(this);
        target = nullptr;
    }
    Actor::remove(this);
    delete this;
}

bool FlavourBuff::act() {
    detach();
    return true;
}

// -----------------------------------------------------------------------------
// Hunger
// -----------------------------------------------------------------------------
bool Hunger::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    if (isStarving()) {
        if (Random::Float() < 0.3f && (target->HP > 1 || !target->paralysed)) {
            GLog::n("You are starving!");
            target->damage(1, this);
            target->interrupt();
        }
    } else {
        float newLevel = level + STEP;
        if (newLevel >= STARVING) {
            GLog::n("You are starving!");
            target->interrupt();
        } else if (newLevel >= HUNGRY && level < HUNGRY) {
            GLog::w("You are hungry.");
        }
        level = newLevel;
    }

    float step = STEP;
    if (target == Dungeon::hero.get()) {
        Hero* hero = static_cast<Hero*>(target);
        if (hero->heroClass == Hero::HeroClass::Rogue) {
            step *= 1.2f;
        }
    }
    if (target->buff(BuffType::Shadows) != nullptr) {
        step *= 1.5f;
    }
    spend(step * TICK);
    return true;
}

void Hunger::satisfy(float energy) {
    level = std::max(0.0f, level - energy);
    if (level > STARVING) {
        level = STARVING;
    }
}

// -----------------------------------------------------------------------------
// Regeneration
// -----------------------------------------------------------------------------
bool Regeneration::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    if (target->HP < target->HT) {
        Hunger* h = target->getBuff<Hunger>();
        if (!h || !h->isStarving()) {
            target->HP++;
            FX::floatingText(target->pos, "+1", 0x38b764FF);
        }
    }

    spend(REGENERATION_DELAY * TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Burning
// -----------------------------------------------------------------------------
bool Burning::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    int dmg = Random::IntRange(1, 5);
    target->damage(dmg, this);
    FX::burst((target->pos % Level::WIDTH) * 16.0f + 8.0f,
              (target->pos / Level::WIDTH) * 16.0f + 8.0f,
              0xee7722FF, 4, 15.0f, 35.0f, -40.0f);
    FX::floatingText(target->pos, std::to_string(dmg), 0xee7722FF);

    // Stepping into water immediately extinguishes flame
    if (Dungeon::level && Dungeon::level->water[target->pos]) {
        FX::waterSplash(target->pos, 6);
        detach();
        return true;
    }

    duration--;
    if (duration <= 0) {
        detach();
    } else {
        spend(TICK);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Poison
// -----------------------------------------------------------------------------
bool Poison::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    int dmg = (duration / 3) + 1;
    target->damage(dmg, this);
    FX::floatingText(target->pos, std::to_string(dmg), 0x38b764FF);

    duration--;
    if (duration <= 0) {
        detach();
    } else {
        spend(TICK);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Bleeding
// -----------------------------------------------------------------------------
bool Bleeding::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    level = Random::IntRange(level / 2, level);
    if (level > 0) {
        target->damage(level, this);
        FX::blood(target->pos, std::min(10 * level / std::max(1, target->HT), 10));
        FX::floatingText(target->pos, std::to_string(level), 0xd63030FF);
        spend(TICK);
    } else {
        detach();
    }
    return true;
}

// -----------------------------------------------------------------------------
// Ooze
// -----------------------------------------------------------------------------
bool Ooze::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    target->damage(1, this);
    if (target == Dungeon::hero.get()) {
        GLog::n("Caustic ooze burns your skin! (-1 HP)");
    }
    FX::burst(target->pos, 0x000000FFu, 4);

    if (Dungeon::level && Dungeon::level->water[target->pos]) {
        if (target == Dungeon::hero.get()) {
            GLog::p("The water washes away the caustic ooze!");
        }
        detach();
        return true;
    }

    spend(TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Paralysis
// -----------------------------------------------------------------------------
void Paralysis::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        target->paralysed = true;
    }
}

void Paralysis::detach() {
    if (target) {
        target->paralysed = false;
    }
    Buff::detach();
}

bool Paralysis::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    duration--;
    if (duration <= 0) {
        detach();
    } else {
        spend(TICK);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Roots
// -----------------------------------------------------------------------------
void Roots::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        target->rooted = true;
    }
}

void Roots::detach() {
    if (target) {
        target->rooted = false;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Frost
// -----------------------------------------------------------------------------
void Frost::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        target->paralysed = true;
        Buff* b = target->buff(BuffType::Burning);
        if (b) b->detach();
    }
}

void Frost::detach() {
    if (target) {
        target->paralysed = false;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Barkskin
// -----------------------------------------------------------------------------
bool Barkskin::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    spend(TICK);
    level--;
    if (level <= 0) {
        detach();
    }
    return true;
}

// -----------------------------------------------------------------------------
// Invisibility
// -----------------------------------------------------------------------------
void Invisibility::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        target->invisible++;
    }
}

void Invisibility::detach() {
    if (target) {
        target->invisible--;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Levitation
// -----------------------------------------------------------------------------
void Levitation::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        target->flying = true;
        Buff* r = target->buff(BuffType::Roots);
        if (r) r->detach();
    }
}

void Levitation::detach() {
    if (target) {
        target->flying = false;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Shadows
// -----------------------------------------------------------------------------
bool Shadows::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }
    spend(TICK);
    return true;
}

} // namespace pd
