#pragma once

#include "Char.h"
#include <memory>

namespace pd {

class Level;
class Hero;
class Item;

class Mob : public Char {
public:
    enum class State {
        SLEEPING,
        WANDERING,
        HUNTING,
        FLEEING
    };

    enum class MobType {
        Rat,
        Gnoll,
        Crab,
        Skeleton,
        Swarm,
        Thief,
        Shaman,
        Bat,
        Brute,
        Monk,
        Warlock,
        Golem,
        Succubus,
        Eye,
        Scorpio,
        Piranha,
        Wraith,
        Goo,
        Tengu,
        DM300,
        King,
        Yog
    };

    State state = State::SLEEPING;
    int expAward = 1;
    int target = -1;
    Char* enemy = nullptr;
    bool enemySeen = false;
    bool alerted = false;

    static constexpr float TIME_TO_WAKE_UP = 1.0f;

    Mob();
    ~Mob() override = default;

    CharSprite* getSprite() override;

    virtual MobType mobType() const { return MobType::Rat; }
    bool act() override;
    void damage(int dmg, Actor* src) override;
    int defenseProc(Char* enemy, int damage) override;

    virtual void aiStep(Hero* hero, Level* currentLevel);
    void die(Actor* src) override;

    virtual bool canAttack(Char* enemy);
    virtual bool doAttack(Char* enemy);
    virtual bool getCloser(int target);
    virtual bool getFurther(int target);
    virtual void nowhereToRun();

    void notice();
    void beckon(int cell);

    static bool s_mobFov[1024];
};

class Rat : public Mob {
public:
    Rat();
    MobType mobType() const override { return MobType::Rat; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

class Gnoll : public Mob {
public:
    Gnoll();
    MobType mobType() const override { return MobType::Gnoll; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

class Crab : public Mob {
public:
    Crab();
    MobType mobType() const override { return MobType::Crab; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

class Skeleton : public Mob {
public:
    Skeleton();
    MobType mobType() const override { return MobType::Skeleton; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    void die(Actor* src) override;
};

class Swarm : public Mob {
public:
    int generation = 0;
    Swarm();
    MobType mobType() const override { return MobType::Swarm; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int defenseProc(Char* enemy, int damage) override;
};

class Thief : public Mob {
public:
    std::unique_ptr<Item> stolenItem;
    Thief();
    MobType mobType() const override { return MobType::Thief; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
    void nowhereToRun() override;
    void die(Actor* src) override;
};

class Shaman : public Mob {
public:
    Shaman();
    MobType mobType() const override { return MobType::Shaman; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool canAttack(Char* enemy) override;
    bool doAttack(Char* enemy) override;
};

class Bat : public Mob {
public:
    Bat();
    MobType mobType() const override { return MobType::Bat; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
};

class Brute : public Mob {
public:
    bool enraged = false;
    Brute();
    MobType mobType() const override { return MobType::Brute; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    void damage(int dmg, Actor* src) override;
};

class Monk : public Mob {
public:
    Monk();
    MobType mobType() const override { return MobType::Monk; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
};

class Warlock : public Mob {
public:
    Warlock();
    MobType mobType() const override { return MobType::Warlock; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool canAttack(Char* enemy) override;
    bool doAttack(Char* enemy) override;
};

class Golem : public Mob {
public:
    Golem();
    MobType mobType() const override { return MobType::Golem; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
};

class Succubus : public Mob {
public:
    int blinkDelay = 0;
    Succubus();
    MobType mobType() const override { return MobType::Succubus; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int attackProc(Char* enemy, int damage) override;
    bool getCloser(int target) override;
};

class Eye : public Mob {
public:
    Eye();
    MobType mobType() const override { return MobType::Eye; }
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool canAttack(Char* enemy) override;
    bool doAttack(Char* enemy) override;
};

class Scorpio : public Mob {
public:
    Scorpio();
    MobType mobType() const override { return MobType::Scorpio; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool canAttack(Char* enemy) override;
    int attackProc(Char* enemy, int damage) override;
    bool getCloser(int target) override;
};

class Piranha : public Mob {
public:
    Piranha();
    MobType mobType() const override { return MobType::Piranha; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool act() override;
    bool getCloser(int target) override;
    void die(Actor* src) override;
};

class Wraith : public Mob {
public:
    int level = 1;
    Wraith();
    MobType mobType() const override { return MobType::Wraith; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    void adjustStats(int dungeonDepth);
};

// --- Bosses ---

class Goo : public Mob {
public:
    bool pumpedUp = false;

    Goo();
    MobType mobType() const override { return MobType::Goo; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
    bool act() override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void die(Actor* src) override;
};

class Tengu : public Mob {
public:
    int timeToJump = 5;

    Tengu();
    MobType mobType() const override { return MobType::Tengu; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void jump(Level* currentLevel);
    void die(Actor* src) override;
};

class DM300 : public Mob {
public:
    DM300();
    MobType mobType() const override { return MobType::DM300; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool act() override;
    void move(int step) override;
    void die(Actor* src) override;
};

class King : public Mob {
public:
    int summonCountdown = 3;
    King();
    MobType mobType() const override { return MobType::King; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void die(Actor* src) override;
};

class Yog : public Mob {
public:
    int fistsAlive = 2;
    Yog();
    MobType mobType() const override { return MobType::Yog; }
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    void damage(int dmg, Actor* src) override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void die(Actor* src) override;
};

} // namespace pd
