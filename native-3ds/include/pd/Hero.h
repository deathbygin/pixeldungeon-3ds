#pragma once

#include "Char.h"
#include "Belongings.h"
#include "HeroSubClass.h"

namespace pd {

class Level;
class Heap;

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
    bool pickUp(Level* currentLevel = nullptr, bool spendTurn = true);
    void openContainer(Heap* heap, int targetCell);
    void rest(bool tillHealthy = true);
    bool search(Level* currentLevel, bool intentional = true);

    bool shoot(Char* enemy, MissileWeapon* wep);

    void earnExp(int amount);

    // Hunger state queries
    bool isStarving() const;
    bool isHungry() const;
    std::string hungerStatus() const;
    int tier() const { return belongings.equippedArmor ? belongings.equippedArmor->tier : 0; }

    // Combat skill calculations
    int attackSkill(Char* target) override;
    int defenseSkill(Char* attacker) override;
    int dr() override;
    int damageRoll() override;
    int stealth() const override;
    int attackProc(Char* enemy, int damage) override;
    int defenseProc(Char* enemy, int damage) override;
};

} // namespace pd
