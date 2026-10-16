#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
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
class Ring;
class DewVial;
class Potion;
class Scroll;

enum class ItemCategory {
    Generic,
    Weapon,
    Armor,
    Potion,
    Scroll,
    Ring,
    Wand
};

class Item {
public:
    using ItemCategory = pd::ItemCategory;
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
    virtual class PlantSeed* asPlantSeed() { return nullptr; }
    virtual std::vector<std::string> actions(Hero* hero);
    virtual bool execute(Hero* hero, const std::string& action);

    virtual void cast(Hero* hero, int targetCell);
    virtual void onThrow(int cell);
    virtual void onCollect(Hero* hero) { (void)hero; }
    virtual int maxDistance(const Hero* hero = nullptr) const { (void)hero; return 6; }
    virtual float speedFactor(Hero* hero) const { (void)hero; return 1.0f; }
    virtual std::unique_ptr<Item> clone(int qty = 1) const;

    virtual bool isEquippable() const { return false; }
    virtual bool isStackable() const { return false; }
    virtual bool isUpgradable() const { return false; }
    virtual void upgrade() { level++; cursed = false; cursedKnown = true; }
    virtual void degrade() { level--; }
    virtual bool isBroken() const { return false; }
    virtual void fix() {}
    virtual Item* random();

    virtual bool isIdentified() const { return identified; }
    virtual bool isCursed() const { return cursed; }
    virtual Item* identify() {
        identified = true;
        levelKnown = true;
        cursedKnown = true;
        return this;
    }

    virtual std::string info() const;
    virtual std::string displayName() const;
    virtual int price() const { return 0; }

    virtual Weapon* asWeapon() { return nullptr; }
    virtual Armor* asArmor() { return nullptr; }
    virtual Wand* asWand() { return nullptr; }
    virtual MissileWeapon* asMissile() { return nullptr; }
    virtual Boomerang* asBoomerang() { return nullptr; }
    virtual Dart* asDart() { return nullptr; }
    virtual ShortSword* asShortSword() { return nullptr; }
    virtual Ring* asRing() { return nullptr; }
    virtual DewVial* asDewVial() { return nullptr; }
    virtual Potion* asPotion() { return nullptr; }
    virtual Scroll* asScroll() { return nullptr; }
    virtual class Key* asKey() { return nullptr; }
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
    void upgrade() override { upgrade(false); }
    virtual void upgrade(bool enchant);
    bool isIdentified() const override { return identified && levelKnown; }
    Item* identify() override;
    Item* random() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    virtual int typicalSTR() const;

    enum class Enchantment {
        NONE,
        BLAZING,
        CHILLING,
        VENOMOUS,
        PARALYTIC,
        LEECH,
        SHOCKING,
        UNSTABLE
    };
    Enchantment enchantment = Enchantment::NONE;
    bool isEnchanted() const { return enchantment != Enchantment::NONE; }
    void enchant(Enchantment ench = Enchantment::NONE);

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
    enum class WandType {
        Teleportation = 0,
        Slowness,
        Firebolt,
        Poison,
        Regrowth,
        Blink,
        Lightning,
        Amok,
        Reach,
        Flock,
        Disintegration,
        Avalanche,
        Paralysis,
        MagicMissile,
        COUNT = 12
    };

    static constexpr int USAGES_TO_KNOW = 40;
    static constexpr int INITIAL_CHARGES = 2;

    int maxCharges = INITIAL_CHARGES;
    int curCharges = INITIAL_CHARGES;
    float chargeTimer = 0.0f;
    bool curChargeKnown = false;
    int usagesToKnow = USAGES_TO_KNOW;
    bool hitChars = true;
    std::string wood;
    int woodImage = 0;
    WandType wandType = WandType::Teleportation;

    static void initWoods();
    static bool isKnown(WandType tp);
    static void setKnown(WandType tp);
    static bool allKnown();
    static const std::vector<int>& getWoodMapping();
    static const std::vector<bool>& getKnownTable();
    static void restoreStatus(const std::vector<int>& mapping, const std::vector<bool>& known);
    static std::unique_ptr<Wand> randomWand();
    static std::unique_ptr<Wand> createByType(WandType tp);
    static std::unique_ptr<Wand> createByName(const std::string& wandName);

    explicit Wand(std::string n = "Wand", int initialC = 2, WandType tp = WandType::Teleportation);
    Wand* asWand() override { return this; }
    ItemCategory category() const override { return ItemCategory::Weapon; }
    bool isEquippable() const override;
    bool isUpgradable() const override { return true; }
    bool isIdentified() const override { return Weapon::isIdentified() && isKnown() && curChargeKnown; }
    bool isKnown() const;
    void setKnown();
    bool hitsChars() const { return hitChars; }

    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::string status() const;

    virtual std::string desc() const;
    virtual int initialCharges() const { return INITIAL_CHARGES; }
    int power() const;
    int minDamage() const;
    int maxDamage() const;
    int damageRoll(Hero* hero = nullptr) const override;
    int typicalSTR() const override { return 10; }

    void updateCharge(float dt, Hero* hero);
    void restoreCharge(int count = 1);
    void upgrade() override;
    void degrade() override;
    Item* identify() override;
    void wandUsed(Hero* hero);

    virtual void fx(int cell, std::function<void()> callback);
    virtual void onZap(int cell);
    void cast(Hero* hero, int targetCell) override;
    int maxDistance(const Hero* hero = nullptr) const override { (void)hero; return 32; }
    Item* random() override;
    int price() const;
    void updatePerception();
};

class WandOfMagicMissile : public Wand {
public:
    WandOfMagicMissile();
    int initialCharges() const override { return 3; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfLightning : public Wand {
public:
    WandOfLightning();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
private:
    void hit(Char* ch, int damage, std::vector<Char*>& affected, std::vector<int>& points);
};

class WandOfFirebolt : public Wand {
public:
    WandOfFirebolt();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfDisintegration : public Wand {
public:
    WandOfDisintegration();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfPoison : public Wand {
public:
    WandOfPoison();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfSlowness : public Wand {
public:
    WandOfSlowness();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

using WandOfSlow = WandOfSlowness;

class WandOfParalysis : public Wand {
public:
    WandOfParalysis();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfFlock : public Wand {
public:
    WandOfFlock();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfAmok : public Wand {
public:
    WandOfAmok();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfTeleportation : public Wand {
public:
    WandOfTeleportation();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfBlink : public Wand {
public:
    WandOfBlink();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfRegrowth : public Wand {
public:
    WandOfRegrowth();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfReach : public Wand {
public:
    WandOfReach();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
    void onZap(int cell) override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class WandOfAvalanche : public Wand {
public:
    WandOfAvalanche();
    std::string desc() const override;
    void fx(int cell, std::function<void()> callback) override;
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
    void upgrade() override { upgrade(false); }
    virtual void upgrade(bool inscribe);
    bool isIdentified() const override { return identified && levelKnown; }
    Item* identify() override;
    Item* random() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    virtual int typicalSTR() const;

    enum class Glyph {
        NONE,
        BOUNCE,
        AFFECTION,
        ANTI_ENTROPY,
        AUTO_REPAIR,
        DISPLACEMENT,
        ENTANGLEMENT,
        EVIL_AURORA,
        FLOW,
        POTENTIAL,
        METABOLISM,
        MULTI_VISION,
        STENCH,
        VISCOSITY
    };
    Glyph glyph = Glyph::NONE;
    bool isInscribed() const { return glyph != Glyph::NONE; }
    void inscribe(Glyph g = Glyph::NONE);

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
    Potion* asPotion() override { return this; }
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
    virtual void apply(Hero* hero);
    int price() const override;
    int maxDistance(const Hero* hero = nullptr) const override { (void)hero; return 6; }

    void updatePerception();
    static void heal(Hero* hero);

    // Static Status Handler methods
    static void initColors();
    static bool isKnown(PotionType tp);
    static void setKnown(PotionType tp);
    static bool allKnown();
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
        ENCHANTMENT,
        WIPE_OUT
    };
    static constexpr size_t RUNIC_SCROLL_COUNT = 12;
    static constexpr size_t SCROLL_COUNT = 13;

    ScrollType type = ScrollType::UPGRADE;

    explicit Scroll(ScrollType tp);
    Scroll* asScroll() override { return this; }
    ItemCategory category() const override { return ItemCategory::Scroll; }
    bool isStackable() const override { return true; }
    bool isIdentified() const override { return isKnown(type); }
    Item* identify() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
    int price() const override;
    Item* random() override;
    static std::unique_ptr<Scroll> createRandom();

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
    Key* asKey() override { return this; }
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

class SkeletonKey : public Key {
public:
    SkeletonKey();
    explicit SkeletonKey(int d);
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
    PlantSeed* asPlantSeed() override { return this; }
    bool isStackable() const override { return true; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    void onThrow(int cell) override;
    void onCollect(Hero* hero) override;
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
// ArmorKit & Amulet of Yendor
// =============================================================================
class ArmorKit : public Item {
public:
    ArmorKit();
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

class Amulet : public Item {
public:
    Amulet();
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// Ring
// =============================================================================
class Ring : public Item {
public:
    enum class Gem {
        DIAMOND,
        OPAL,
        GARNET,
        RUBY,
        AMETHYST,
        TOPAZ,
        ONYX,
        TOURMALINE,
        EMERALD,
        SAPPHIRE,
        QUARTZ,
        AGATE
    };
    Gem gem = Gem::DIAMOND;
    std::string effect = "Mending";

    explicit Ring(Gem g = Gem::DIAMOND, std::string eff = "Mending");
    Ring* asRing() override { return this; }
    ItemCategory category() const override { return ItemCategory::Ring; }
    bool isEquippable() const override { return true; }
    bool isUpgradable() const override { return true; }
    bool isIdentified() const override { return identified && levelKnown; }
    Item* identify() override;
    Item* random() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// DewVial
// =============================================================================
class DewVial : public Item {
public:
    static constexpr int MAX_VOLUME = 10;
    int volume = 0;

    DewVial();
    DewVial* asDewVial() override { return this; }
    bool isStackable() const override { return false; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    bool isFull() const { return volume >= MAX_VOLUME; }
    void fill() { volume = MAX_VOLUME; }
    void collectDew(int count = 1);
    static void autoDrink(Hero* hero);

    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::string displayName() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// Bomb
// =============================================================================
class Bomb : public Item {
public:
    Bomb();
    bool isStackable() const override { return true; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    Item* random() override;
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    void onThrow(int cell) override;
    void explode(int cell);
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// Honeypot
// =============================================================================
class Honeypot : public Item {
public:
    Honeypot();
    bool isStackable() const override { return true; }
    bool isUpgradable() const override { return false; }
    bool isIdentified() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    void onThrow(int cell) override;
    void shatter(int cell);
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

// =============================================================================
// MysteryMeat
// =============================================================================
class MysteryMeat : public Food {
public:
    MysteryMeat();
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
    std::unique_ptr<Item> clone(int qty = 1) const override;
};

} // namespace pd
