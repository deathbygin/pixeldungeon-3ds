#pragma once

#include "Char.h"
#include "Belongings.h"
#include "HeroSubClass.h"

namespace pd {

class Level;
class Heap;
class Mob;

class MissileWeapon;

class Hero : public Char {
public:
    enum class HeroClass {
        Warrior,
        Mage,
        Rogue,
        Huntress
    };

    HeroClass heroClass = HeroClass::Warrior;
    HeroSubClass subClass = HeroSubClass::None;
    int STR = 10;
    bool weakened = false;
    int effectiveSTR() const { return weakened ? std::max(1, STR - 2) : STR; }
    int exp = 0;
    int level = 1;
    int maxExp = 5;
    int gold = 0;
    float awareness = 0.1f;

    Belongings belongings;
    MissileWeapon* rangedWeapon = nullptr;
    bool waitingForInput = true;
    bool restoreHealth = false;
    bool halted = false;

    Hero();
    explicit Hero(HeroClass cl);
    ~Hero() override = default;
    Hero* asHero() override { return this; }

    std::string className() const;
    void updateAwareness();

    void initClass(HeroClass cl);

    CharSprite* getSprite() override;
    void updateArmorTier();

    void live();
    bool act() override;
    void interrupt() override;
    void die(Actor* src) override;
    void damage(int dmg, Actor* src) override;
    float speed() const override;

    // Turn actions
    bool stepTo(int nextCell, Level* currentLevel);
    void swapWith(Mob* ally, Level* currentLevel);
    bool pickUp(Level* currentLevel = nullptr, bool spendTurn = true);
    bool lastStepPickedUp() const { return m_lastStepPickedUp; }
    bool lastStepAttacked() const { return m_lastStepAttacked; }
    bool lastStepSwapped() const { return m_lastStepSwapped; }
    void openContainer(Heap* heap, int targetCell);
    void rest(bool tillHealthy = true);
    bool search(Level* currentLevel, bool intentional = true);

    bool m_lastStepPickedUp = false;
    bool m_lastStepAttacked = false;
    bool m_lastStepSwapped = false;
    int m_autoPickDisabledCell = -1;

    void disableAutoPick(int cell) { m_autoPickDisabledCell = cell; }
    void enableAutoPick(int cell) { if (m_autoPickDisabledCell == cell) m_autoPickDisabledCell = -1; }
    bool isAutoPickDisabled(int cell) const { return m_autoPickDisabledCell == cell; }
    void clearAutoPickDisabled() { m_autoPickDisabledCell = -1; }

    bool shoot(Char* enemy, MissileWeapon* wep);

    void earnExp(int amount);

    // Hunger state queries
    bool isStarving() const;
    bool isHungry() const;
    std::string hungerStatus() const;
    int tier() const { return belongings.equippedArmor ? belongings.equippedArmor->tier : 0; }

    // Combat skill calculations
    int attackSkill(Char* target) override;
    float attackDelay() const;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int damageRoll() override;
    int stealth() const override;
    int attackProc(Char* enemy, int damage) override;
    int defenseProc(Char* enemy, int damage) override;
};

} // namespace pd
