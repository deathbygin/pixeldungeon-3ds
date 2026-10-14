#pragma once

#include <string>
#include <vector>
#include <memory>

namespace pd {

class Char;
class Hero;

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
};

class Weapon : public Item {
public:
    static constexpr int HITS_TO_KNOW = 20;

    int minDamage = 1;
    int maxDamage = 6;
    int STR = 10;
    int hitsToKnow = HITS_TO_KNOW;

    Weapon(std::string n, int minD, int maxD, int strReq);
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

    int damageRoll() const;
    void proc(Char* attacker, Char* defender, int damage);
};

class Armor : public Item {
public:
    static constexpr int HITS_TO_KNOW = 10;

    int tier = 1;
    int STR = 11;
    int hitsToKnow = HITS_TO_KNOW;

    Armor(std::string n, int t, int strReq);
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
};

class Dewdrop : public Item {
public:
    Dewdrop();
    ItemCategory category() const override { return ItemCategory::Generic; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
};

class Gold : public Item {
public:
    explicit Gold(int value = 1);
    ItemCategory category() const override { return ItemCategory::Generic; }
    bool isStackable() const override { return true; }
    std::vector<std::string> actions(Hero* hero) override;
    bool execute(Hero* hero, const std::string& action) override;
    std::string info() const override;
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
};

class IronKey : public Key {
public:
    IronKey();
    explicit IronKey(int d);
    std::string info() const override;
};

} // namespace pd
