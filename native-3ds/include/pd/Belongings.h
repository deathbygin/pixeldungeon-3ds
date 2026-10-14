#pragma once

#include "Item.h"
#include <vector>
#include <memory>

namespace pd {

class Hero;

class Belongings {
public:
    static constexpr size_t BACKPACK_SIZE = 19;

    std::vector<std::unique_ptr<Item>> backpack;
    Weapon* equippedWeapon = nullptr;
    Armor* equippedArmor = nullptr;

    Belongings() = default;
    ~Belongings() = default;

    bool collect(std::unique_ptr<Item> item);
    bool equip(Item* item);
    bool unequip(Item* item);
    std::unique_ptr<Item> drop(Item* item);
    Item* getKey(const std::string& keyName, int depth);
    bool consumeKey(const std::string& keyName, int depth);
    void observe();
    bool isFull() const { return backpack.size() >= BACKPACK_SIZE; }
};

} // namespace pd

