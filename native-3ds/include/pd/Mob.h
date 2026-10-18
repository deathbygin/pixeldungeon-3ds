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
        BurningFist,
        RottingFist,
        MirrorImage,
        Sheep,
        Mimic,
        Shopkeeper,
        Bee,
        Spinner
    };

    State state = State::SLEEPING;
    bool hostile = true;
    int expAward = 1;
    int target = -1;
    Char* enemy = nullptr;
    bool enemySeen = false;
    bool alerted = false;

    static constexpr float TIME_TO_WAKE_UP = 1.0f;

    Mob();
    ~Mob() override = default;

    virtual void aggro(Char* ch) { enemy = ch; }

    CharSprite* getSprite() override;
    Mob* asMob() override { return this; }

    virtual MobType mobType() const { return MobType::Rat; }
    virtual bool isBoss() const {
        MobType t = mobType();
        return t == MobType::Goo || t == MobType::Tengu || t == MobType::DM300 ||
               t == MobType::King || t == MobType::Yog ||
               t == MobType::BurningFist || t == MobType::RottingFist;
    }
    virtual bool isHostile() const { return (buff(BuffType::Amok) != nullptr) || hostile; }
    virtual bool isAlly() const { return !isHostile(); }
    virtual bool isPet() const { return mobType() == MobType::Bee; }
    virtual void interact();
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

    virtual void notice();
    void beckon(int cell);
    void yell(const std::string& str);

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
    void die(Actor* src) override;
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

class Bandit : public Thief {
public:
    Bandit();
    std::string description() const override;
    int attackProc(Char* enemy, int damage) override;
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
    void die(Actor* src) override;
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
    void move(int step) override;
    void notice() override;
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

class RottingFist : public Mob {
public:
    RottingFist();
    MobType mobType() const override { return MobType::RottingFist; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
    bool act() override;
    void die(Actor* src) override;
};

class BurningFist : public Mob {
public:
    BurningFist();
    MobType mobType() const override { return MobType::BurningFist; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int dr() override;
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
    void interact() override;
};

class Sheep : public Mob {
public:
    float lifespan = 5.0f;
    bool initialized = false;

    explicit Sheep(float life = 5.0f);
    MobType mobType() const override { return MobType::Sheep; }
    std::string description() const override;
    bool act() override;
    void damage(int dmg, Actor* src) override { (void)dmg; (void)src; }
    int defenseSkill(Char* attacker) override;
    void aiStep(Hero* hero, Level* currentLevel) override { (void)hero; (void)currentLevel; }
    void interact() override;
};

class Mimic : public Mob {
public:
    int level = 1;
    std::vector<std::unique_ptr<Item>> items;

    Mimic();
    MobType mobType() const override { return MobType::Mimic; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
    void die(Actor* src) override;
    void adjustStats(int dungeonDepth);

    static std::unique_ptr<Item> createChestDrop();
    static Mimic* spawnAt(int pos, std::vector<std::unique_ptr<Item>> chestItems = {});
};

class Shopkeeper : public Mob {
public:
    Shopkeeper();
    MobType mobType() const override { return MobType::Shopkeeper; }
    std::string description() const override;
    bool act() override;
    void damage(int dmg, Actor* src) override;
    void die(Actor* src) override;
    void aiStep(Hero* hero, Level* currentLevel) override { (void)hero; (void)currentLevel; }
    void interact() override;
    void flee();
};

class Bee : public Mob {
public:
    int level = 1;

    Bee();
    MobType mobType() const override { return MobType::Bee; }
    std::string description() const override;
    void spawn(int depth);
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int attackProc(Char* enemy, int damage) override;
    bool isImmune(BuffType type) const override { return type == BuffType::Poison; }
    bool act() override;
    void aiStep(Hero* hero, Level* currentLevel) override;
    void die(Actor* src) override;
    void interact() override;
};

class Spinner : public Mob {
public:
    Spinner();
    MobType mobType() const override { return MobType::Spinner; }
    std::string description() const override;
    int damageRoll() override;
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int attackProc(Char* enemy, int damage) override;
    bool act() override;
    void die(Actor* src) override;
};

} // namespace pd
