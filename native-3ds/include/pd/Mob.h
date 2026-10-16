#pragma once

#include "Char.h"
#include <memory>

namespace pd {

class Level;
class Hero;
class Item;
class Weapon;

class Mob : public Char {
public:
    enum class State {
        SLEEPING,
        WANDERING,
        HUNTING,
        FLEEING,
        PASSIVE
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
        Statue,
        Wraith,
        Goo,
        Tengu,
        DM300,
        King,
        Yog,
        MirrorImage
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
    Mob* asMob() override { return this; }

    virtual MobType mobType() const { return MobType::Rat; }
    virtual std::string description() const;
    std::string status() const;
    std::string statePreview() const;
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
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

class Gnoll : public Mob {
public:
    Gnoll();
    MobType mobType() const override { return MobType::Gnoll; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

class Crab : public Mob {
public:
    Crab();
    MobType mobType() const override { return MobType::Crab; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

class Skeleton : public Mob {
public:
    Skeleton();
    MobType mobType() const override { return MobType::Skeleton; }
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    bool canAttack(Char* enemy) override;
    bool doAttack(Char* enemy) override;
    void die(Actor* src) override;
};

class Scorpio : public Mob {
public:
    Scorpio();
    MobType mobType() const override { return MobType::Scorpio; }
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    void adjustStats(int dungeonDepth);

    static void spawnAround(int pos);
    static Wraith* spawnAt(int pos);
};

class Statue : public Mob {
public:
    std::unique_ptr<Weapon> weapon;

    Statue();
    MobType mobType() const override { return MobType::Statue; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    void damage(int dmg, Actor* src) override;
    int attackProc(Char* enemy, int damage) override;
    void die(Actor* src) override;
};

// --- Bosses ---

class Goo : public Mob {
public:
    bool pumpedUp = false;

    Goo();
    MobType mobType() const override { return MobType::Goo; }
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
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
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    void damage(int dmg, Actor* src) override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void die(Actor* src) override;
};

class MirrorImage : public Mob {
public:
    MirrorImage();
    MobType mobType() const override { return MobType::MirrorImage; }
    std::string description() const override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void damage(int dmg, Actor* src) override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
};

} // namespace pd
