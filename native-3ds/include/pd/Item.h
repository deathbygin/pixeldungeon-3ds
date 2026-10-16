#pragma once

#include <string>
#include <vector>
#include <memory>
#include "pd/HeroSubClass.h"

namespace pd {

class Char;
class Hero;
class Weapon;
class Armor;
class Wand;
class MissileWeapon;
class Boomerang;
class Dart;
class ShortSword;

enum class ItemCategory {
    Generic,
    Weapon,
    Armor,
    Potion,
    Scroll
};

class Item {
public:
    std::string name = "Item";
    int image = 0;
    int quantity = 1;
    int level = 0;
    bool identified = false;
    bool cursed = false;
    bool cursedKnown = false;
    bool levelKnown = false;

    Item() = default;
    virtual ~Item() = default;

    virtual ItemCategory category() const { return ItemCategory::Generic; }
    virtual std::vector<std::string> actions(Hero* hero);
    virtual bool execute(Hero* hero, const std::string& action);

    virtual void cast(Hero* hero, int targetCell);
    virtual void onThrow(int cell);
    virtual int maxDistance(const Hero* hero = nullptr) const { (void)hero; return 6; }
    virtual float speedFactor(Hero* hero) const { (void)hero; return 1.0f; }
    virtual std::unique_ptr<Item> clone(int qty = 1) const;

    virtual bool isEquippable() const { return false; }
    virtual bool isStackable() const { return false; }
    virtual bool isUpgradable() const { return false; }
    virtual void upgrade() { level++; cursed = false; cursedKnown = true; }
    virtual void degrade() { level--; }
    virtual Item* random();

    virtual bool isIdentified() const { return identified; }
    virtual Item* identify() {
        identified = true;
        levelKnown = true;
        cursedKnown = true;
        return this;
    }

    virtual std::string info() const;
    virtual std::string displayName() const;

    virtual Weapon* asWeapon() { return nullptr; }
    virtual Armor* asArmor() { return nullptr; }
    virtual Wand* asWand() { return nullptr; }
    virtual MissileWeapon* asMissile() { return nullptr; }
    virtual Boomerang* asBoomerang() { return nullptr; }
    virtual Dart* asDart() { return nullptr; }
    virtual ShortSword* asShortSword() { return nullptr; }
};

class Weapon : public Item {
public:
    static constexpr int HITS_TO_KNOW = 20;

    int minDamage = 1;
    int maxDamage = 6;
    int STR = 10;
    int hitsToKnow = HITS_TO_KNOW;

    Weapon(std::string n, int minD, int maxD, int strReq);
    Weapon* asWeapon() override { return this; }
    ItemCategory category() const override { return ItemCategory::Weapon; }
    bool isEquippable() const override { return true; }
    bool isUpgradable() const override { return true; }
    bool isIdentified() const override { return identified && levelKnown; }
    Item* identify() override;
    Item* random() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    virtual int typicalSTR() const;

    virtual float acuracyFactor(Hero* hero) const;
    float speedFactor(Hero* hero) const override;
    virtual int damageRoll() const;
    virtual int damageRoll(Hero* hero) const;
    void proc(Char* attacker, Char* defender, int damage);
};

class ShortSword : public Weapon {
public:
    ShortSword();
    ShortSword* asShortSword() override { return this; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Wand : public Weapon {
public:
    static constexpr int INITIAL_CHARGES = 2;
    int maxCharges = INITIAL_CHARGES;
    int curCharges = INITIAL_CHARGES;
    float chargeTimer = 0.0f;

    explicit Wand(std::string n = "Wand", int initialC = 2);
    Wand* asWand() override { return this; }
    ItemCategory category() const override { return ItemCategory::Weapon; }
    bool isEquippable() const override { return true; }
    bool isUpgradable() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;

    void updateCharge(float dt, Hero* hero);
    void restoreCharge(int count = 1);
    void upgrade() override;
    void degrade() override;

    virtual void onZap(int cell);
    void cast(Hero* hero, int targetCell) override;
    int maxDistance(const Hero* hero = nullptr) const override { (void)hero; return 8; }
};

class WandOfMagicMissile : public Wand {
public:
    WandOfMagicMissile();
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Armor : public Item {
public:
    static constexpr int HITS_TO_KNOW = 10;

    int tier = 1;
    int STR = 11;
    int hitsToKnow = HITS_TO_KNOW;

    Armor(std::string n, int t, int strReq);
    Armor* asArmor() override { return this; }
    ItemCategory category() const override { return ItemCategory::Armor; }
    bool isEquippable() const override { return true; }
    bool isUpgradable() const override { return true; }
    bool isIdentified() const override { return identified && levelKnown; }
    Item* identify() override;
    Item* random() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    virtual int typicalSTR() const;

    int drRoll() const;
    int proc(Char* attacker, Char* defender, int damage);
};

class Potion : public Item {
public:
    enum class PotionType {
        HEALING,
        EXPERIENCE,
        TOXIC_GAS,
        LIQUID_FLAME,
        STRENGTH,
        PARALYTIC_GAS,
        LEVITATION,
        MIND_VISION,
        PURITY,
        INVISIBILITY,
        MIGHT,
        FROST
    };
    static constexpr size_t POTION_COUNT = 12;

    PotionType type = PotionType::HEALING;

    explicit Potion(PotionType tp);
    ItemCategory category() const override { return ItemCategory::Potion; }
    bool isStackable() const override { return true; }
    bool isIdentified() const override { return isKnown(type); }
    Item* identify() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;

    void onThrow(int cell) override;
    virtual void shatter(int cell);
    int maxDistance(const Hero* hero = nullptr) const override { (void)hero; return 6; }

    void updatePerception();

    // Static Status Handler methods
    static void initColors();
    static bool isKnown(PotionType tp);
    static void setKnown(PotionType tp);
    static const std::string& colorName(PotionType tp);
    static int imageForType(PotionType tp);
    static const std::string& trueName(PotionType tp);
    static const std::string& trueDesc(PotionType tp);
    static const std::vector<int>& getColorMapping();
    static const std::vector<bool>& getKnownTable();
    static void restoreStatus(const std::vector<int>& mapping, const std::vector<bool>& known);
};

class Scroll : public Item {
public:
    enum class ScrollType {
        IDENTIFY,
        MAGIC_MAPPING,
        RECHARGING,
        REMOVE_CURSE,
        TELEPORTATION,
        CHALLENGE,
        TERROR,
        LULLABY,
        PSIONIC_BLAST,
        MIRROR_IMAGE,
        UPGRADE,
        ENCHANTMENT
    };
    static constexpr size_t SCROLL_COUNT = 12;

    ScrollType type = ScrollType::UPGRADE;

    explicit Scroll(ScrollType tp);
    ItemCategory category() const override { return ItemCategory::Scroll; }
    bool isStackable() const override { return true; }
    bool isIdentified() const override { return isKnown(type); }
    Item* identify() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;

    void updatePerception();

    // Static Status Handler methods
    static void initLabels();
    static bool isKnown(ScrollType tp);
    static void setKnown(ScrollType tp);
    static const std::string& runeName(ScrollType tp);
    static int imageForType(ScrollType tp);
    static const std::string& trueName(ScrollType tp);
    static const std::string& trueDesc(ScrollType tp);
    static const std::vector<int>& getRuneMapping();
    static const std::vector<bool>& getKnownTable();
    static void restoreStatus(const std::vector<int>& mapping, const std::vector<bool>& known);
};

class Food : public Item {
public:
    Food();
    ItemCategory category() const override { return ItemCategory::Generic; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Dewdrop : public Item {
public:
    Dewdrop();
    ItemCategory category() const override { return ItemCategory::Generic; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Gold : public Item {
public:
    explicit Gold(int value = 1);
    ItemCategory category() const override { return ItemCategory::Generic; }
    bool isStackable() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Torch : public Item {
public:
    Torch();
    ItemCategory category() const override { return ItemCategory::Generic; }
    bool isStackable() const override { return true; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Key : public Item {
public:
    int depth = 1;

    Key();
    explicit Key(int d);
    ItemCategory category() const override { return ItemCategory::Generic; }
    bool isStackable() const override { return false; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
};

class GoldenKey : public Key {
public:
    GoldenKey();
    explicit GoldenKey(int d);
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class IronKey : public Key {
public:
    IronKey();
    explicit IronKey(int d);
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// Missile Weapons
// =============================================================================
class MissileWeapon : public Weapon {
public:
    MissileWeapon(std::string n, int minD, int maxD, int strReq);
    MissileWeapon* asMissile() override { return this; }

    bool isEquippable() const override { return false; }
    bool isStackable() const override { return true; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }

    std::vector<std::string> actions(Hero* hero) override;
    void onThrow(int cell) override;
    virtual void miss(int cell);
    float acuracyFactor(Hero* hero) const override;
    float speedFactor(Hero* hero) const override;
    int maxDistance(const Hero* hero = nullptr) const override;
};

class Dart : public MissileWeapon {
public:
    Dart(int number = 1);
    Dart* asDart() override { return this; }
    Item* random() override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class IncendiaryDart : public MissileWeapon {
public:
    IncendiaryDart(int number = 1);
    Item* random() override;
    void onThrow(int cell) override;
    void proc(Char* attacker, Char* defender, int damage);
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class CurareDart : public MissileWeapon {
public:
    CurareDart(int number = 1);
    Item* random() override;
    void proc(Char* attacker, Char* defender, int damage);
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Javelin : public MissileWeapon {
public:
    Javelin(int number = 1);
    Item* random() override;
    void proc(Char* attacker, Char* defender, int damage);
    int maxDistance(const Hero* hero = nullptr) const override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Shuriken : public MissileWeapon {
public:
    Shuriken(int number = 1);
    Item* random() override;
    float speedFactor(Hero* hero) const override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Tamahawk : public MissileWeapon {
public:
    Tamahawk(int number = 1);
    Item* random() override;
    void proc(Char* attacker, Char* defender, int damage);
    int maxDistance(const Hero* hero = nullptr) const override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Boomerang : public MissileWeapon {
public:
    Boomerang();
    Boomerang* asBoomerang() override { return this; }
    bool isStackable() const override { return false; }
    bool isUpgradable() const override { return true; }
    bool isEquippable() const override { return true; }

    int minDamage = 1;
    int maxDamage = 4;

    void onThrow(int cell) override;
    void miss(int cell) override;
    void proc(Char* attacker, Char* defender, int damage);
    void circleBack(int fromCell, Hero* owner);
    int maxDistance(const Hero* hero = nullptr) const override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// Plants & Seeds
// =============================================================================
class PlantSeed : public Item {
public:
    enum class PlantType {
        FIREBLOOM,
        ICECAP,
        SORROWMOSS,
        DREAMWEED,
        SUNGRASS,
        EARTHROOT,
        FADELEAF,
        ROTBERRY
    };

    PlantType plantType = PlantType::FIREBLOOM;

    explicit PlantSeed(PlantType pt);
    bool isStackable() const override { return true; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    void onThrow(int cell) override;
    int maxDistance(const Hero* hero = nullptr) const override { (void)hero; return 5; }
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// Tome of Mastery
// =============================================================================
class TomeOfMastery : public Item {
public:
    TomeOfMastery();
    bool isStackable() const override { return false; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;

    void choose(Hero* hero, HeroSubClass way);
};

// =============================================================================
// Amulet of Yendor
// =============================================================================
class Amulet : public Item {
public:
    Amulet() {
        name = "Amulet of Yendor";
        image = 15;
        identified = true;
    }
    bool isIdentified() const override { return true; }
    std::string info() const override {
        return "The Amulet of Yendor is the most powerful known artifact.";
    }
    std::unique_ptr<Item> clone(int qty = 1) const override {
        (void)qty;
        return std::make_unique<Amulet>();
    }
};

} // namespace pd
