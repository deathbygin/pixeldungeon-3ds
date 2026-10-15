#include "pd/Mob.h"
#include "pd/Hero.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"
#include "pd/Random.h"
#include "pd/PathFinder.h"
#include "pd/ShadowCaster.h"
#include "pd/Ballistica.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Badges.h"
#include "pd/Item.h"
#include "pd/CharSprite.h"
#include "noosa/Audio.h"
#include <cmath>
#include <algorithm>

namespace pd {

bool Mob::s_mobFov[1024] = { false };

Mob::Mob() {
    viewDistance = 6;
    state = State::SLEEPING;
}

CharSprite* Mob::getSprite() {
    if (!sprite) {
        sprite = CharSprite::createForMob(this);
        sprite->place(pos);
    }
    return sprite.get();
}

void Mob::damage(int dmg, Actor* src) {
    if (state == State::SLEEPING) {
        state = State::WANDERING;
    }
    alerted = true;
    Char::damage(dmg, src);
}

void Mob::die(Actor* src) {
    if (Dungeon::hero) {
        Dungeon::hero->earnExp(expAward);
        FX::floatingText(pos, "+" + std::to_string(expAward) + "EXP", 0xd4af37FF);
    }
    Badges::validateMonstersSlain();
    Char::die(src);
}

bool Mob::canAttack(Char* enemyChar) {
    if (!enemyChar || !enemyChar->isAlive()) return false;
    return PathFinder::distanceChebyshev(pos, enemyChar->pos) <= 1;
}

bool Mob::doAttack(Char* enemyChar) {
    attack(enemyChar);
    spend(TICK / speed());
    return true;
}

bool Mob::getCloser(int targetCell) {
    if (rooted || !Dungeon::level) return false;

    int step = Dungeon::findPath(this, pos, targetCell,
                                 Dungeon::level->passable,
                                 s_mobFov);
    if (step != -1) {
        move(step);
        Dungeon::level->mobPress(this);
        return true;
    }
    return false;
}

bool Mob::getFurther(int targetCell) {
    if (rooted || !Dungeon::level) return false;

    int step = Dungeon::flee(this, pos, targetCell,
                             Dungeon::level->passable,
                             s_mobFov);
    if (step != -1) {
        move(step);
        Dungeon::level->mobPress(this);
        return true;
    }
    return false;
}

void Mob::nowhereToRun() {
}

void Mob::notice() {
    alerted = true;
    FX::floatingText(pos, "!", 0xe43b44FF);
}

void Mob::beckon(int cell) {
    notice();
    if (state != State::HUNTING) {
        state = State::WANDERING;
    }
    target = cell;
}

void Mob::aiStep(Hero* hero, Level* currentLevel) {
    if (!hero || !hero->isAlive() || !currentLevel) {
        spend(TICK);
        return;
    }

    if (paralysed) {
        enemySeen = false;
        spend(TICK);
        return;
    }

    // 1. Calculate mob's independent FOV from its own position
    int mx = pos % Level::WIDTH;
    int my = pos / Level::WIDTH;
    ShadowCaster::castShadow(mx, my, s_mobFov, viewDistance, currentLevel->losBlocking);

    enemy = hero;
    bool enemyInFOV = (enemy && enemy->isAlive() &&
                       s_mobFov[enemy->pos] &&
                       enemy->invisible <= 0);

    int dist = PathFinder::distanceChebyshev(pos, enemy->pos);

    switch (state) {
    case State::SLEEPING: {
        // Authentic Java formula: Random.Int(distance(enemy) + enemy.stealth() + (enemy.flying ? 2 : 0)) == 0
        int stealthMod = enemy->stealth() + (enemy->flying ? 2 : 0);
        int rollRange = dist + stealthMod;
        if (rollRange <= 0) rollRange = 1;

        if (enemyInFOV && Random::Int(rollRange) == 0) {
            enemySeen = true;
            notice();
            state = State::HUNTING;
            target = enemy->pos;
            spend(TIME_TO_WAKE_UP);
        } else {
            enemySeen = false;
            spend(TICK);
        }
        break;
    }

    case State::WANDERING: {
        // Authentic Java formula: enemyInFOV && (alerted || Random.Int(distance(enemy) / 2 + enemy.stealth()) == 0)
        int rollRange = (dist / 2) + enemy->stealth();
        if (rollRange <= 0) rollRange = 1;

        if (enemyInFOV && (alerted || Random::Int(rollRange) == 0)) {
            enemySeen = true;
            notice();
            state = State::HUNTING;
            target = enemy->pos;
            spend(TICK / speed());
        } else {
            enemySeen = false;
            if (target != -1 && getCloser(target)) {
                spend(TICK / speed());
            } else {
                target = currentLevel->randomDestination();
                spend(TICK);
            }
        }
        break;
    }

    case State::HUNTING: {
        enemySeen = enemyInFOV;
        if (enemyInFOV && canAttack(enemy)) {
            doAttack(enemy);
        } else {
            if (enemyInFOV) {
                target = enemy->pos;
            }

            if (target != -1 && getCloser(target)) {
                spend(TICK / speed());
            } else {
                spend(TICK);
                state = State::WANDERING;
                target = currentLevel->randomDestination();
            }
        }
        break;
    }

    case State::FLEEING: {
        enemySeen = enemyInFOV;
        if (enemyInFOV) {
            target = enemy->pos;
        }

        if (target != -1 && getFurther(target)) {
            spend(TICK / speed());
        } else {
            spend(TICK);
            nowhereToRun();
        }
        break;
    }
    }

    alerted = false;
}

bool Mob::act() {
    if (!isAlive()) return true;
    aiStep(Dungeon::hero.get(), Dungeon::level.get());
    return true;
}

int Mob::defenseProc(Char* enemyChar, int damage) {
    if (!enemySeen && enemyChar == Dungeon::hero.get()) {
        Hero* hero = Dungeon::hero.get();
        if (hero && hero->subClass == HeroSubClass::Assassin) {
            damage += Random::IntRange(1, damage);
            FX::slash(pos);
            FX::blood(pos, 10);
            GLog::p("Surprise attack! Critical strike!");
        }
    }
    return damage;
}

// -----------------------------------------------------------------------------
// Bestiary Implementations
// -----------------------------------------------------------------------------

Rat::Rat() {
    name = "Marsupial Rat";
    HP = 8;
    HT = 8;
    expAward = 1;
}

int Rat::damageRoll() {
    return Random::IntRange(1, 4);
}

int Rat::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 8;
}

int Rat::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 3;
}

Gnoll::Gnoll() {
    name = "Gnoll Scout";
    HP = 12;
    HT = 12;
    expAward = 2;
}

int Gnoll::damageRoll() {
    return Random::IntRange(2, 6);
}

int Gnoll::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 11;
}

int Gnoll::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 4;
}

Crab::Crab() {
    name = "Sewer Crab";
    HP = 15;
    HT = 15;
    baseSpeed = 2.0f; // Fast mover!
    expAward = 3;
}

int Crab::damageRoll() {
    return Random::IntRange(3, 7);
}

int Crab::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 13;
}

int Crab::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 8;
}

Skeleton::Skeleton() {
    name = "Skeleton";
    HP = 20;
    HT = 20;
    expAward = 4;
}

int Skeleton::damageRoll() {
    return Random::IntRange(4, 9);
}

int Skeleton::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 14;
}

int Skeleton::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 6;
}

void Skeleton::die(Actor* src) {
    for (int n = 0; n < 8; ++n) {
        int neighbor = pos + PathFinder::NEIGHBOURS8[n];
        Char* ch = Actor::findChar(neighbor);
        if (ch && ch->isAlive()) {
            int boneDmg = Random::IntRange(2, 6);
            ch->damage(boneDmg, this);
            FX::blood(neighbor, 6);
            FX::floatingText(neighbor, std::to_string(boneDmg), 0xd63030FF);
        }
    }

    float wx = (pos % Level::WIDTH) * 16.0f + 8.0f;
    float wy = (pos / Level::WIDTH) * 16.0f + 8.0f;
    FX::burst(wx, wy, 0xddddddFF, 12, 30.0f, 70.0f, +80.0f);

    Mob::die(src);
}

// Swarm of flies
Swarm::Swarm() {
    name = "Swarm of Flies";
    HP = 80;
    HT = 80;
    expAward = 4;
    flying = true;
}

int Swarm::damageRoll() {
    return Random::NormalIntRange(1, 4);
}

int Swarm::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 12;
}

int Swarm::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 5;
}

int Swarm::defenseProc(Char* enemyChar, int damage) {
    damage = Mob::defenseProc(enemyChar, damage);
    if (HP >= damage + 2 && Dungeon::level) {
        std::vector<int> candidates;
        int n4[4] = { pos + 1, pos - 1, pos + Level::WIDTH, pos - Level::WIDTH };
        for (int n : n4) {
            if (n >= 0 && n < Level::LENGTH && Dungeon::level->passable[n] && Actor::findChar(n) == nullptr) {
                candidates.push_back(n);
            }
        }
        if (!candidates.empty()) {
            int spawnCell = candidates[Random::Int(static_cast<int>(candidates.size()))];
            auto clone = std::make_unique<Swarm>();
            clone->generation = generation + 1;
            clone->HP = (HP - damage) / 2;
            clone->HT = HT;
            clone->move(spawnCell);
            clone->state = State::HUNTING;
            clone->target = target;
            Actor::occupyCell(clone.get());
            Actor::add(clone.get(), 1.0f);
            HP -= clone->HP;
            FX::burst(spawnCell, 0x222222FFu, 4);
            Dungeon::mobs.push_back(std::move(clone));
        }
    }
    return damage;
}

// Crazy Thief
Thief::Thief() {
    name = "Crazy Thief";
    HP = 20;
    HT = 20;
    expAward = 5;
    baseSpeed = 1.0f;
}

int Thief::damageRoll() {
    return Random::NormalIntRange(1, 7);
}

int Thief::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 12;
}

int Thief::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 12;
}

int Thief::dr() {
    return 3;
}

int Thief::attackProc(Char* enemyChar, int damage) {
    if (!stolenItem && enemyChar == Dungeon::hero.get()) {
        Hero* h = Dungeon::hero.get();
        // Steal a random item from hero backpack
        auto& bp = h->belongings.backpack;
        if (!bp.empty()) {
            int idx = Random::Int(static_cast<int>(bp.size()));
            stolenItem = std::move(bp[idx]);
            bp.erase(bp.begin() + idx);
            GLog::w("Thief stole your " + stolenItem->name + "!");
            state = State::FLEEING;
            notice();
        }
    }
    return damage;
}

void Thief::nowhereToRun() {
    state = State::HUNTING;
}

void Thief::die(Actor* src) {
    if (stolenItem && Dungeon::level) {
        Dungeon::level->drop(std::move(stolenItem), pos);
    }
    Mob::die(src);
}

// Gnoll Shaman
Shaman::Shaman() {
    name = "Gnoll Shaman";
    HP = 18;
    HT = 18;
    expAward = 6;
}

int Shaman::damageRoll() {
    return Random::NormalIntRange(2, 6);
}

int Shaman::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 11;
}

int Shaman::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 8;
}

int Shaman::dr() {
    return 4;
}

bool Shaman::canAttack(Char* enemyChar) {
    if (!enemyChar || !enemyChar->isAlive()) return false;
    return Ballistica::cast(pos, enemyChar->pos, false, true) == enemyChar->pos;
}

bool Shaman::doAttack(Char* enemyChar) {
    if (PathFinder::distanceChebyshev(pos, enemyChar->pos) <= 1) {
        attack(enemyChar);
        spend(TICK / speed());
    } else {
        // Lightning zap
        spend(2.0f * TICK);
        int zapDmg = Random::IntRange(2, 12);
        if (Dungeon::level && Dungeon::level->get(enemyChar->pos) == Terrain::WATER && !enemyChar->flying) {
            zapDmg = static_cast<int>(zapDmg * 1.5f);
        }
        enemyChar->damage(zapDmg, this);
        FX::floatingText(enemyChar->pos, std::to_string(zapDmg), 0x55ffffff);
        FX::burst(enemyChar->pos, 0x88ccffffu, 8);
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_LIGHTNING).get());
        GLog::w("Gnoll Shaman zaps you with lightning!");
    }
    return true;
}

// Vampire Bat
Bat::Bat() {
    name = "Vampire Bat";
    HP = 30;
    HT = 30;
    baseSpeed = 2.0f;
    expAward = 7;
    flying = true;
}

int Bat::damageRoll() {
    return Random::NormalIntRange(6, 12);
}

int Bat::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 16;
}

int Bat::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 15;
}

int Bat::dr() {
    return 4;
}

int Bat::attackProc(Char* enemyChar, int damage) {
    (void)enemyChar;
    int reg = std::min(damage, HT - HP);
    if (reg > 0) {
        HP += reg;
        FX::floatingText(pos, "+" + std::to_string(reg), 0x38b764FF);
    }
    return damage;
}

// Gnoll Brute
Brute::Brute() {
    name = "Gnoll Brute";
    HP = 40;
    HT = 40;
    expAward = 8;
}

int Brute::damageRoll() {
    return enraged ? Random::NormalIntRange(10, 40) : Random::NormalIntRange(8, 18);
}

int Brute::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 20;
}

int Brute::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 15;
}

int Brute::dr() {
    return 8;
}

void Brute::damage(int dmg, Actor* src) {
    Mob::damage(dmg, src);
    if (isAlive() && !enraged && HP < HT / 4) {
        enraged = true;
        spend(TICK);
        FX::floatingText(pos, "ENRAGED!", 0xd63030FF);
        GLog::w("Gnoll Brute becomes enraged!");
    }
}

// Dwarf Monk
Monk::Monk() {
    name = "Dwarf Monk";
    HP = 70;
    HT = 70;
    expAward = 11;
}

int Monk::damageRoll() {
    return Random::NormalIntRange(12, 16);
}

int Monk::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 30;
}

int Monk::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 30;
}

int Monk::dr() {
    return 2;
}

int Monk::attackProc(Char* enemyChar, int damage) {
    if (enemyChar == Dungeon::hero.get() && Random::Int(6) == 0) {
        Hero* hero = Dungeon::hero.get();
        if (hero->belongings.equippedWeapon && Dungeon::level) {
            std::string wName = hero->belongings.equippedWeapon->name;
            // Disarm: drop equipped weapon to ground
            for (auto it = hero->belongings.backpack.begin(); it != hero->belongings.backpack.end(); ++it) {
                if (it->get() == hero->belongings.equippedWeapon) {
                    Dungeon::level->drop(std::move(*it), hero->pos);
                    hero->belongings.backpack.erase(it);
                    hero->belongings.equippedWeapon = nullptr;
                    break;
                }
            }
            GLog::w("Dwarf Monk has knocked the " + wName + " from your hands!");
        }
    }
    return damage;
}

// Dwarf Warlock
Warlock::Warlock() {
    name = "Dwarf Warlock";
    HP = 70;
    HT = 70;
    expAward = 11;
}

int Warlock::damageRoll() {
    return Random::NormalIntRange(12, 20);
}

int Warlock::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 25;
}

int Warlock::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 18;
}

int Warlock::dr() {
    return 8;
}

bool Warlock::canAttack(Char* enemyChar) {
    if (!enemyChar || !enemyChar->isAlive()) return false;
    return Ballistica::cast(pos, enemyChar->pos, false, true) == enemyChar->pos;
}

bool Warlock::doAttack(Char* enemyChar) {
    if (PathFinder::distanceChebyshev(pos, enemyChar->pos) <= 1) {
        attack(enemyChar);
        spend(TICK / speed());
    } else {
        // Shadow bolt
        spend(TICK);
        int boltDmg = Random::IntRange(12, 18);
        enemyChar->damage(boltDmg, this);
        FX::floatingText(enemyChar->pos, std::to_string(boltDmg), 0xa020f0FF);
        FX::burst(enemyChar->pos, 0x550088FFu, 6);
        GLog::w("Dwarf Warlock casts shadow bolt at you!");
    }
    return true;
}

// Golem
Golem::Golem() {
    name = "Golem";
    HP = 85;
    HT = 85;
    expAward = 12;
    baseSpeed = 0.67f; // Slow but mighty
}

int Golem::damageRoll() {
    return Random::NormalIntRange(20, 40);
}

int Golem::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 28;
}

int Golem::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 18;
}

int Golem::dr() {
    return 12;
}

// Succubus
Succubus::Succubus() {
    name = "Succubus";
    HP = 80;
    HT = 80;
    expAward = 12;
}

int Succubus::damageRoll() {
    return Random::NormalIntRange(15, 25);
}

int Succubus::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 40;
}

int Succubus::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 25;
}

int Succubus::attackProc(Char* enemyChar, int damage) {
    if (enemyChar == Dungeon::hero.get() && Random::Int(3) == 0) {
        GLog::w("Succubus winks at you seductively! You feel charmed.");
        FX::floatingText(pos, "<3", 0xff69b4FF);
    }
    return damage;
}

bool Succubus::getCloser(int targetCell) {
    if (s_mobFov[targetCell] && PathFinder::distanceChebyshev(pos, targetCell) > 2 && blinkDelay <= 0) {
        int cell = Ballistica::cast(pos, targetCell, true, true);
        if (Actor::findChar(cell) != nullptr && Ballistica::distance > 1) {
            cell = Ballistica::trace[Ballistica::distance - 2];
        }
        if (cell >= 0 && cell < Level::LENGTH && Dungeon::level && Dungeon::level->passable[cell] && !Actor::findChar(cell)) {
            move(cell);
            blinkDelay = 5;
            FX::burst(cell, 0xff69b4FFu, 8);
            GLog::i("Succubus teleports next to you!");
            return true;
        }
    }
    blinkDelay--;
    return Mob::getCloser(targetCell);
}

// Evil Eye
Eye::Eye() {
    name = "Evil Eye";
    HP = 100;
    HT = 100;
    expAward = 13;
    flying = true;
}

int Eye::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 30;
}

int Eye::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 20;
}

int Eye::dr() {
    return 10;
}

bool Eye::canAttack(Char* enemyChar) {
    if (!enemyChar || !enemyChar->isAlive()) return false;
    Ballistica::cast(pos, enemyChar->pos, true, false);
    for (int i = 1; i < Ballistica::distance; ++i) {
        if (Ballistica::trace[i] == enemyChar->pos) {
            return true;
        }
    }
    return false;
}

bool Eye::doAttack(Char* enemyChar) {
    spend(1.6f * TICK);
    int gazeDmg = Random::IntRange(14, 26);
    enemyChar->damage(gazeDmg, this);
    FX::floatingText(enemyChar->pos, std::to_string(gazeDmg), 0x9900ffFF);
    FX::burst(enemyChar->pos, 0x9900ffFFu, 10);
    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_ZAP).get());
    GLog::w("Evil Eye fires deathgaze piercing beam!");
    return true;
}

// Scorpio
Scorpio::Scorpio() {
    name = "Scorpio";
    HP = 95;
    HT = 95;
    expAward = 14;
}

int Scorpio::damageRoll() {
    return Random::NormalIntRange(20, 32);
}

int Scorpio::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 36;
}

int Scorpio::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 24;
}

int Scorpio::dr() {
    return 16;
}

bool Scorpio::canAttack(Char* enemyChar) {
    if (!enemyChar || !enemyChar->isAlive()) return false;
    return (PathFinder::distanceChebyshev(pos, enemyChar->pos) > 1) &&
           (Ballistica::cast(pos, enemyChar->pos, false, true) == enemyChar->pos);
}

int Scorpio::attackProc(Char* enemyChar, int damage) {
    if (enemyChar && Random::Int(2) == 0) {
        enemyChar->rooted = true;
        FX::floatingText(enemyChar->pos, "CRIPPLED", 0x884400FF);
        GLog::w("Scorpio's barb cripples you!");
    }
    return damage;
}

bool Scorpio::getCloser(int targetCell) {
    if (state == State::HUNTING && enemySeen) {
        return getFurther(targetCell); // Scorpio runs away while shooting
    }
    return Mob::getCloser(targetCell);
}

// Giant Piranha
Piranha::Piranha() {
    name = "Giant Piranha";
    HP = 20;
    HT = 20;
    expAward = 0;
    baseSpeed = 2.0f;
}

int Piranha::damageRoll() {
    return Random::NormalIntRange(4, 10);
}

int Piranha::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 24;
}

int Piranha::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 16;
}

int Piranha::dr() {
    return 2;
}

bool Piranha::act() {
    if (Dungeon::level && !Dungeon::level->water[pos]) {
        die(nullptr);
        return true;
    }
    return Mob::act();
}

bool Piranha::getCloser(int targetCell) {
    if (rooted || !Dungeon::level) return false;
    // Water-only navigation
    static bool waterPass[Level::LENGTH];
    for (int i = 0; i < Level::LENGTH; ++i) {
        waterPass[i] = Dungeon::level->water[i];
    }
    int step = Dungeon::findPath(this, pos, targetCell, waterPass, s_mobFov);
    if (step != -1) {
        move(step);
        return true;
    }
    return false;
}

void Piranha::die(Actor* src) {
    Mob::die(src);
}

// Wraith
Wraith::Wraith() {
    name = "Wraith";
    HP = 1;
    HT = 1;
    expAward = 0;
    flying = true;
    adjustStats(1);
}

int Wraith::damageRoll() {
    return Random::NormalIntRange(1, 3 + level);
}

int Wraith::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 10 + level;
}

int Wraith::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return (10 + level) * 5; // Very high evasion!
}

void Wraith::adjustStats(int dungeonDepth) {
    level = dungeonDepth;
}

// =============================================================================
// Boss Implementations
// =============================================================================

Goo::Goo() {
    name = "Goo";
    HP = 80;
    HT = 80;
    expAward = 10;
    baseSpeed = 1.0f;
    state = State::WANDERING;
}

int Goo::damageRoll() {
    return pumpedUp ? Random::NormalIntRange(5, 30) : Random::NormalIntRange(2, 12);
}

int Goo::attackSkill(Char* targetChar) {
    (void)targetChar;
    return pumpedUp ? 30 : 15;
}

int Goo::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 12;
}

int Goo::dr() {
    return 2;
}

int Goo::attackProc(Char* enemyChar, int damage) {
    if (Random::Int(3) == 0 && enemyChar) {
        enemyChar->affect<Ooze>();
        FX::burst(enemyChar->pos, 0x000000FFu, 5);
        if (enemyChar == Dungeon::hero.get()) {
            GLog::w("Goo covers you with caustic ooze!");
        }
    }
    return damage;
}

bool Goo::act() {
    if (Dungeon::level && Dungeon::level->get(pos) == Terrain::WATER && HP < HT) {
        HP = std::min(HT, HP + 1);
        FX::floatingText(pos, "+1", 0x38b764FF);
    }
    return Mob::act();
}

void Goo::aiStep(Hero* hero, Level* currentLevel) {
    if (!hero || !hero->isAlive() || !currentLevel) {
        spend(TICK);
        return;
    }

    enemy = hero;
    bool inRange = canAttack(hero);

    if (pumpedUp) {
        if (inRange) {
            attack(hero);
            pumpedUp = false;
        } else {
            getCloser(hero->pos);
        }
        spend(TICK / speed());
        return;
    }

    if (inRange) {
        if (Random::Int(3) == 0) {
            pumpedUp = true;
            spend(2.0f * TICK);
            FX::floatingText(pos, "!!!", 0xd63030FF);
            GLog::n("Goo is pumping itself up!");
            return;
        }
        attack(hero);
        spend(TICK / speed());
        return;
    }

    Mob::aiStep(hero, currentLevel);
}

void Goo::die(Actor* src) {
    GLog::h("The black ooze disperses! Goo has been defeated!");
    Badges::validateBossSlain(1);
    Mob::die(src);
}

Tengu::Tengu() {
    name = "Tengu";
    HP = 120;
    HT = 120;
    expAward = 20;
    baseSpeed = 1.0f;
    timeToJump = 5;
    state = State::WANDERING;
}

int Tengu::damageRoll() {
    return Random::NormalIntRange(8, 15);
}

int Tengu::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 20;
}

int Tengu::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 20;
}

int Tengu::dr() {
    return 5;
}

void Tengu::jump(Level* currentLevel) {
    timeToJump = 5;
    if (!currentLevel) return;

    for (int i = 0; i < 3; ++i) {
        int trapPos = Random::Int(Level::LENGTH);
        if (currentLevel->isPassable(trapPos)) {
            currentLevel->set(trapPos, Terrain::POISON_TRAP);
        }
    }

    int newPos = -1;
    for (int attempts = 0; attempts < 100; ++attempts) {
        int candidate = Random::Int(Level::LENGTH);
        if (currentLevel->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
            if (!enemy || PathFinder::distanceChebyshev(candidate, enemy->pos) >= 3) {
                newPos = candidate;
                break;
            }
        }
    }

    if (newPos != -1) {
        move(newPos);
        FX::burst(newPos, 0xddddddFFu, 8);
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_PUFF).get());
        GLog::i("Tengu vanishes in a puff of smoke!");
    }
    spend(TICK / speed());
}

void Tengu::aiStep(Hero* hero, Level* currentLevel) {
    if (!hero || !hero->isAlive() || !currentLevel) {
        spend(TICK);
        return;
    }

    enemy = hero;
    int dist = PathFinder::distanceChebyshev(pos, hero->pos);

    if (dist <= 1) {
        timeToJump--;
        if (timeToJump <= 0) {
            jump(currentLevel);
            return;
        }
    }

    if (Ballistica::cast(pos, hero->pos, false, true) == hero->pos) {
        attack(hero);
        spend(TICK / speed());
        return;
    }

    Mob::aiStep(hero, currentLevel);
}

void Tengu::die(Actor* src) {
    GLog::h("Tengu yells: 'Free at last...'");
    Badges::validateBossSlain(2);
    Mob::die(src);
}

// DM-300 Boss
DM300::DM300() {
    name = "DM-300";
    HP = 200;
    HT = 200;
    expAward = 30;
    state = State::WANDERING;
}

int DM300::damageRoll() {
    return Random::NormalIntRange(18, 24);
}

int DM300::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 28;
}

int DM300::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 18;
}

int DM300::dr() {
    return 10;
}

bool DM300::act() {
    // Toxic gas emission
    FX::seedBlob(pos, Blob::BlobType::TOXIC_GAS, 30);
    return Mob::act();
}

void DM300::move(int step) {
    Mob::move(step);
    if (Dungeon::level && Dungeon::level->get(step) == Terrain::INACTIVE_TRAP && HP < HT) {
        HP = std::min(HT, HP + Random::IntRange(10, 25));
        FX::floatingText(step, "+HP", 0x38b764FF);
        GLog::n("DM-300 repairs itself!");
    }
}

void DM300::die(Actor* src) {
    GLog::h("The machine collapses into a heap of scrap metal!");
    Badges::validateBossSlain(3);
    Mob::die(src);
}

// King of Dwarves Boss
King::King() {
    name = "King of Dwarves";
    HP = 300;
    HT = 300;
    expAward = 40;
    state = State::WANDERING;
}

int King::damageRoll() {
    return Random::NormalIntRange(20, 38);
}

int King::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 32;
}

int King::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 25;
}

int King::dr() {
    return 14;
}

void King::aiStep(Hero* hero, Level* currentLevel) {
    if (!hero || !hero->isAlive() || !currentLevel) {
        spend(TICK);
        return;
    }

    summonCountdown--;
    if (summonCountdown <= 0) {
        summonCountdown = 4;
        // Summon undead skeleton minion
        int spawnCell = -1;
        for (int n : PathFinder::NEIGHBOURS8) {
            int c = pos + n;
            if (c >= 0 && c < Level::LENGTH && currentLevel->passable[c] && !Actor::findChar(c)) {
                spawnCell = c;
                break;
            }
        }
        if (spawnCell != -1) {
            auto minion = std::make_unique<Skeleton>();
            minion->move(spawnCell);
            minion->state = State::HUNTING;
            minion->target = hero->pos;
            Actor::occupyCell(minion.get());
            Actor::add(minion.get(), 1.0f);
            FX::burst(spawnCell, 0xddddddFFu, 8);
            GLog::w("The King summons an undead thrall!");
            Dungeon::mobs.push_back(std::move(minion));
        }
    }

    Mob::aiStep(hero, currentLevel);
}

void King::die(Actor* src) {
    GLog::h("The King of Dwarves falls! The throne is empty.");
    Badges::validateBossSlain(4);
    Mob::die(src);
}

// Yog-Dzewa Old God Boss
Yog::Yog() {
    name = "Yog-Dzewa";
    HP = 300;
    HT = 300;
    expAward = 50;
    state = State::WANDERING;
}

int Yog::damageRoll() {
    return Random::NormalIntRange(15, 30);
}

int Yog::attackSkill(Char* targetChar) {
    (void)targetChar;
    return 25;
}

int Yog::defenseSkill(Char* attackerChar) {
    (void)attackerChar;
    return 15;
}

int Yog::dr() {
    return 10;
}

void Yog::damage(int dmg, Actor* src) {
    if (fistsAlive > 0) {
        dmg >>= fistsAlive; // Damage heavily shielded while fists are alive
        GLog::n("Yog's chaotic aura deflects the blow!");
    }
    Mob::damage(dmg, src);
}

void Yog::aiStep(Hero* hero, Level* currentLevel) {
    if (!hero || !hero->isAlive() || !currentLevel) {
        spend(TICK);
        return;
    }
    // Stationary Old God, casts psychic pulses
    spend(TICK);
    if (canAttack(hero)) {
        attack(hero);
    }
}

void Yog::die(Actor* src) {
    GLog::h("Yog-Dzewa dissolves into cosmic dust! You have banished the Old God!");
    Badges::validateBossSlain(5);
    Mob::die(src);
}

} // namespace pd
