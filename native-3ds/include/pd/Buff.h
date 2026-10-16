#pragma once

#include "Actor.h"
#include <string>

namespace pd {

class Char;

enum class BuffType {
    Hunger,
    Regeneration,
    Burning,
    Poison,
    Paralysis,
    Bleeding,
    Barkskin,
    Roots,
    Frost,
    Cripple,
    Invisibility,
    Levitation,
    Shadows,
    Ooze,
    Combo,
    Fury,
    SnipersMark,
    Light,
    Vertigo,
    Slow,
    Speed,
    Sleep,
    Amok,
    Terror,
    Charm,
    Blindness,
    MindVision,
    Awareness,
    Weakness,
    SoulMark
};

class Combo;

class Buff : public Actor {
public:
    Char* target = nullptr;

    Buff() = default;
    ~Buff() override = default;

    virtual BuffType type() const = 0;
    virtual std::string name() const = 0;
    virtual int icon() const { return -1; }
    virtual std::string desc() const { return ""; }
    virtual float duration() const { return 0.0f; }

    Buff* asBuff() override { return this; }
    virtual Combo* asCombo() { return nullptr; }

    virtual void attachTo(Char* ch);
    virtual void detach();

    bool act() override = 0;
};

// Buff that lasts for a specific time and automatically detaches upon act()
class FlavourBuff : public Buff {
public:
    bool act() override;
};

// =============================================================================
// Hunger & Starvation
// =============================================================================
class Hunger : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Hunger;
    static constexpr float STEP = 10.0f;
    static constexpr float HUNGRY = 260.0f;
    static constexpr float STARVING = 360.0f;

    float level = 0.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return isStarving() ? "Starving" : (isHungry() ? "Hungry" : "Hunger"); }
    int icon() const override { return isStarving() ? 6 : (isHungry() ? 5 : -1); }
    std::string desc() const override;

    bool act() override;
    void satisfy(float energy);

    bool isHungry() const { return level >= HUNGRY; }
    bool isStarving() const { return level >= STARVING; }
};

// =============================================================================
// Natural Regeneration
// =============================================================================
class Regeneration : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Regeneration;
    static constexpr float REGENERATION_DELAY = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Regeneration"; }
    int icon() const override { return 19; }
    std::string desc() const override;

    bool act() override;
};

// =============================================================================
// Elemental & Damage Debuffs
// =============================================================================
class Burning : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Burning;
    int durationTicks = 4;

    Burning(int dur = 4) : durationTicks(dur) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Burning"; }
    int icon() const override { return 2; }
    std::string desc() const override;
    float duration() const override { return static_cast<float>(durationTicks); }

    void attachTo(Char* ch) override;
    bool act() override;
};

class Poison : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Poison;
    int durationTicks = 5;
    int damagePerTick = 2;

    Poison(int dur = 5, int dmg = 2) : durationTicks(dur), damagePerTick(dmg) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Poison"; }
    int icon() const override { return 3; }
    std::string desc() const override;
    float duration() const override { return static_cast<float>(durationTicks); }

    void attachTo(Char* ch) override;
    bool act() override;
};

class Bleeding : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Bleeding;
    int level = 5;

    Bleeding(int lvl = 5) : level(lvl) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Bleeding"; }
    int icon() const override { return 26; }
    std::string desc() const override;
    float duration() const override { return static_cast<float>(level); }

    void attachTo(Char* ch) override;
    bool act() override;
};

class Ooze : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Ooze;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Caustic Ooze"; }
    int icon() const override { return 8; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    bool act() override;
};

// =============================================================================
// Movement & Incapacitation Buffs
// =============================================================================
class Paralysis : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Paralysis;
    int durationTicks = 3;

    Paralysis(int dur = 3) : durationTicks(dur) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Paralysis"; }
    int icon() const override { return 4; }
    std::string desc() const override;
    float duration() const override { return static_cast<float>(durationTicks); }

    void attachTo(Char* ch) override;
    void detach() override;
    bool act() override;
};

class Roots : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Roots;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Rooted"; }
    int icon() const override { return 11; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

class Cripple : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Cripple;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Crippled"; }
    int icon() const override { return 23; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Frost : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Frost;
    static constexpr float DURATION = 5.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Frozen"; }
    int icon() const override { return 15; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

class Vertigo : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Vertigo;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Vertigo"; }
    int icon() const override { return 29; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Slow : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Slow;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Slow"; }
    int icon() const override { return 7; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Speed : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Speed;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Speed"; }
    int icon() const override { return -1; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Sleep : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Sleep;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Asleep"; }
    int icon() const override { return -1; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
    bool act() override;
};

// =============================================================================
// Mental & Behavioral Debuffs
// =============================================================================
class Amok : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Amok;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Amok"; }
    int icon() const override { return 9; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Terror : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Terror;
    static constexpr float DURATION = 10.0f;
    Actor* source = nullptr;

    Terror(Actor* src = nullptr) : source(src) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Fleeing"; }
    int icon() const override { return 10; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Charm : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Charm;
    static constexpr float DURATION = 10.0f;
    Char* charmer = nullptr;

    Charm(Char* src = nullptr) : charmer(src) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Charmed"; }
    int icon() const override { return 21; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class Blindness : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Blindness;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Blind"; }
    int icon() const override { return 16; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

// =============================================================================
// Vision & Stealth Buffs
// =============================================================================
class Invisibility : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Invisibility;
    static constexpr float DURATION = 15.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Invisible"; }
    int icon() const override { return 12; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

class Levitation : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Levitation;
    static constexpr float DURATION = 20.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Levitating"; }
    int icon() const override { return 1; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

class Light : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Light;
    static constexpr float DURATION = 250.0f;
    static constexpr int DISTANCE = 6;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Illuminated"; }
    int icon() const override { return 22; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

class Shadows : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Shadows;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Shadows"; }
    int icon() const override { return 13; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
    bool act() override;
};

class MindVision : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::MindVision;
    static constexpr float DURATION = 20.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Mind Vision"; }
    int icon() const override { return 0; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
    void detach() override;
};

class Awareness : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Awareness;
    static constexpr float DURATION = 2.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Awareness"; }
    int icon() const override { return -1; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

// =============================================================================
// Defensive & Stat Buffs
// =============================================================================
class Barkskin : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Barkskin;
    int level = 0;

    Barkskin(int lvl = 0) : level(lvl) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Barkskin"; }
    int icon() const override { return 24; }
    std::string desc() const override;
    float duration() const override { return static_cast<float>(level); }

    void attachTo(Char* ch) override;
    bool act() override;
    void setLevel(int val) { if (level < val) level = val; }
    int getLevel() const { return level; }
};

class Weakness : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Weakness;
    static constexpr float DURATION = 15.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Weakened"; }
    int icon() const override { return 14; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

// =============================================================================
// Subclass Specialization Buffs
// =============================================================================
class Combo : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Combo;
    int count = 0;

    Combo* asCombo() override { return this; }
    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Combo"; }
    int icon() const override { return 17; }
    std::string desc() const override;

    int hit(Char* enemy, int damage);
    bool act() override;
};

class Fury : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Fury;
    static constexpr float LEVEL = 0.40f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Fury"; }
    int icon() const override { return 18; }
    std::string desc() const override;

    bool act() override;
};

class SnipersMark : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::SnipersMark;
    int targetId = -1;
    float dur = 2.0f;

    SnipersMark(int tId = -1, float d = 2.0f) : targetId(tId), dur(d) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Sniper's Mark"; }
    int icon() const override { return 27; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

class SoulMark : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::SoulMark;
    static constexpr float DURATION = 15.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Soul Mark"; }
    int icon() const override { return 27; }
    std::string desc() const override;

    void attachTo(Char* ch) override;
};

} // namespace pd
