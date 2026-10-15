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
    SnipersMark
};

class Combo;

class Buff : public Actor {
public:
    Char* target = nullptr;

    Buff() = default;
    ~Buff() override = default;

    virtual BuffType type() const = 0;
    virtual std::string name() const = 0;

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

    bool act() override;
};

// =============================================================================
// Elemental & Damage Debuffs
// =============================================================================
class Burning : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Burning;
    int duration = 4;

    Burning(int dur = 4) : duration(dur) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Burning"; }

    bool act() override;
};

class Poison : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Poison;
    int duration = 5;
    int damagePerTick = 2;

    Poison(int dur = 5, int dmg = 2) : duration(dur), damagePerTick(dmg) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Poison"; }

    bool act() override;
};

class Bleeding : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Bleeding;
    int level = 5;

    Bleeding(int lvl = 5) : level(lvl) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Bleeding"; }

    bool act() override;
};

class Ooze : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Ooze;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Caustic Ooze"; }

    bool act() override;
};

// =============================================================================
// Movement & Incapacitation Buffs
// =============================================================================
class Paralysis : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Paralysis;
    int duration = 3;

    Paralysis(int dur = 3) : duration(dur) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Paralysis"; }

    void attachTo(Char* ch) override;
    void detach() override;
    bool act() override;
};

class Roots : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Roots;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Rooted"; }

    void attachTo(Char* ch) override;
    void detach() override;
};

class Cripple : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Cripple;
    static constexpr float DURATION = 10.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Crippled"; }
};

class Frost : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Frost;
    static constexpr float DURATION = 5.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Frozen"; }

    void attachTo(Char* ch) override;
    void detach() override;
};

// =============================================================================
// Defensive & Utility Buffs
// =============================================================================
class Barkskin : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Barkskin;
    int level = 0;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Barkskin"; }

    bool act() override;
    void setLevel(int val) { if (level < val) level = val; }
    int getLevel() const { return level; }
};

class Invisibility : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Invisibility;
    static constexpr float DURATION = 15.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Invisible"; }

    void attachTo(Char* ch) override;
    void detach() override;
};

class Levitation : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Levitation;
    static constexpr float DURATION = 20.0f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Levitating"; }

    void attachTo(Char* ch) override;
    void detach() override;
};

class Shadows : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Shadows;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Shadows"; }

    bool act() override;
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

    int hit(Char* enemy, int damage);
    bool act() override;
};

class Fury : public Buff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::Fury;
    static constexpr float LEVEL = 0.40f;

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Fury"; }

    bool act() override;
};

class SnipersMark : public FlavourBuff {
public:
    static constexpr BuffType STATIC_TYPE = BuffType::SnipersMark;
    int targetId = -1;
    float duration = 2.0f;

    SnipersMark(int tId = -1, float dur = 2.0f) : targetId(tId), duration(dur) {}

    BuffType type() const override { return STATIC_TYPE; }
    std::string name() const override { return "Sniper's Mark"; }
};

} // namespace pd
