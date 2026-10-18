#pragma once

#include "Item.h"
#include <vector>
#include <memory>

namespace pd {

class Hero;

class Belongings {
public:
    static constexpr size_t BACKPACK_SIZE = 15;

    std::vector<std::unique_ptr<Item>> backpack;
    std::unique_ptr<Keyring> keyring;
    std::unique_ptr<Weapon> weaponSlot;
    std::unique_ptr<Armor> armorSlot;
    std::unique_ptr<Item> ring1Slot;
    std::unique_ptr<Item> ring2Slot;

    Weapon* equippedWeapon = nullptr;
    Armor* equippedArmor = nullptr;
    Item* equippedRing1 = nullptr;
    Item* equippedRing2 = nullptr;

    Belongings();
    ~Belongings() = default;

    bool canCollect(const Item* item) const;
    bool collect(std::unique_ptr<Item>& item);
    bool collect(std::unique_ptr<Item>&& item) { return collect(item); }
    bool equip(Item* item);
    bool equip(std::unique_ptr<Item> item);
    bool unequip(Item* item);
    std::unique_ptr<Item> drop(Item* item);
    Item* getKey(const std::string& keyName, int depth);
    bool consumeKey(const std::string& keyName, int depth);
    Item* getItem(const std::string& itemName);
    void observe();
    void uncurseEquipped();
    void identify();
    bool isFull() const { return backpack.size() >= BACKPACK_SIZE; }
};

} // namespace pd

