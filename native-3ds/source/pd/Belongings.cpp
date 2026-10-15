#include "pd/Belongings.h"
#include <algorithm>

namespace pd {

bool Belongings::collect(std::unique_ptr<Item> item) {
    if (!item) return false;

    // Check stackable items
    if (item->isStackable()) {
        for (auto& slot : backpack) {
            if (slot && slot->category() == item->category()) {
                if (item->category() == ItemCategory::Potion) {
                    if (static_cast<Potion*>(slot.get())->type == static_cast<Potion*>(item.get())->type) {
                        slot->quantity += item->quantity;
                        return true;
                    }
                } else if (item->category() == ItemCategory::Scroll) {
                    if (static_cast<Scroll*>(slot.get())->type == static_cast<Scroll*>(item.get())->type) {
                        slot->quantity += item->quantity;
                        return true;
                    }
                } else if (slot->name == item->name) {
                    slot->quantity += item->quantity;
                    return true;
                }
            }
        }
    }

    if (isFull()) return false;

    backpack.push_back(std::move(item));
    return true;
}

bool Belongings::equip(Item* item) {
    if (!item) return false;

    if (item->category() == ItemCategory::Weapon) {
        equippedWeapon = static_cast<Weapon*>(item);
        return true;
    } else if (item->category() == ItemCategory::Armor) {
        equippedArmor = static_cast<Armor*>(item);
        return true;
    }
    return false;
}

bool Belongings::unequip(Item* item) {
    if (!item) return false;

    if (equippedWeapon == item) {
        equippedWeapon = nullptr;
        return true;
    } else if (equippedArmor == item) {
        equippedArmor = nullptr;
        return true;
    } else if (equippedRing1 == item) {
        equippedRing1 = nullptr;
        return true;
    } else if (equippedRing2 == item) {
        equippedRing2 = nullptr;
        return true;
    }
    return false;
}

std::unique_ptr<Item> Belongings::drop(Item* item) {
    if (!item) return nullptr;

    unequip(item);

    for (auto it = backpack.begin(); it != backpack.end(); ++it) {
        if (it->get() == item) {
            std::unique_ptr<Item> dropped = std::move(*it);
            backpack.erase(it);
            return dropped;
        }
    }
    return nullptr;
}

Item* Belongings::getKey(const std::string& keyName, int depth) {
    (void)depth;
    for (auto& it : backpack) {
        if (it && it->name == keyName) {
            return it.get();
        }
    }
    return nullptr;
}

bool Belongings::consumeKey(const std::string& keyName, int depth) {
    (void)depth;
    for (auto it = backpack.begin(); it != backpack.end(); ++it) {
        if (*it && (*it)->name == keyName) {
            (*it)->quantity--;
            if ((*it)->quantity <= 0) {
                backpack.erase(it);
            }
            return true;
        }
    }
    return false;
}

void Belongings::observe() {
    if (equippedWeapon) {
        equippedWeapon->identify();
    }
    if (equippedArmor) {
        equippedArmor->identify();
    }
    for (auto& item : backpack) {
        if (item) {
            item->cursedKnown = true;
        }
    }
}

} // namespace pd
