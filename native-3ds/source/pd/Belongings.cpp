#include "pd/Belongings.h"
#include "pd/Dungeon.h"
#include "pd/Hero.h"
#include <algorithm>

namespace pd {

bool Belongings::canCollect(const Item* item) const {
    if (!item) return false;

    // Check stackable items
    if (item->isStackable()) {
        for (const auto& slot : backpack) {
            if (slot && slot->category() == item->category()) {
                if (item->category() == ItemCategory::Potion) {
                    if (static_cast<const Potion*>(slot.get())->type == static_cast<const Potion*>(item)->type) {
                        return true;
                    }
                } else if (item->category() == ItemCategory::Scroll) {
                    if (static_cast<const Scroll*>(slot.get())->type == static_cast<const Scroll*>(item)->type) {
                        return true;
                    }
                } else if (slot->name == item->name) {
                    return true;
                }
            }
        }
    }

    return !isFull();
}

bool Belongings::collect(std::unique_ptr<Item>& item) {
    if (!item) return false;

    Hero* hero = Dungeon::hero ? Dungeon::hero.get() : nullptr;

    // Check stackable items
    if (item->isStackable()) {
        for (auto& slot : backpack) {
            if (slot && slot->category() == item->category()) {
                if (item->category() == ItemCategory::Potion) {
                    if (static_cast<Potion*>(slot.get())->type == static_cast<Potion*>(item.get())->type) {
                        slot->quantity += item->quantity;
                        item->onCollect(hero);
                        item.reset();
                        return true;
                    }
                } else if (item->category() == ItemCategory::Scroll) {
                    if (static_cast<Scroll*>(slot.get())->type == static_cast<Scroll*>(item.get())->type) {
                        slot->quantity += item->quantity;
                        item->onCollect(hero);
                        item.reset();
                        return true;
                    }
                } else if (slot->name == item->name) {
                    slot->quantity += item->quantity;
                    item->onCollect(hero);
                    item.reset();
                    return true;
                }
            }
        }
    }

    if (isFull()) return false;

    item->onCollect(hero);
    backpack.push_back(std::move(item));
    return true;
}

bool Belongings::equip(std::unique_ptr<Item> item) {
    if (!item || !item->isEquippable() || item->asWand()) return false;

    if (item->category() == ItemCategory::Weapon) {
        if (weaponSlot && weaponSlot->isCursed()) {
            return false;
        }
        std::unique_ptr<Weapon> newWeapon(static_cast<Weapon*>(item.release()));
        if (weaponSlot) {
            backpack.push_back(std::move(weaponSlot));
        }
        weaponSlot = std::move(newWeapon);
        equippedWeapon = weaponSlot.get();
        return true;
    } else if (item->category() == ItemCategory::Armor) {
        if (armorSlot && armorSlot->isCursed()) {
            return false;
        }
        std::unique_ptr<Armor> newArmor(static_cast<Armor*>(item.release()));
        if (armorSlot) {
            backpack.push_back(std::move(armorSlot));
        }
        armorSlot = std::move(newArmor);
        equippedArmor = armorSlot.get();
        return true;
    } else if (item->category() == ItemCategory::Ring) {
        if (!ring1Slot) {
            ring1Slot = std::move(item);
            equippedRing1 = ring1Slot.get();
            return true;
        } else if (!ring2Slot) {
            ring2Slot = std::move(item);
            equippedRing2 = ring2Slot.get();
            return true;
        } else {
            // Both ring slots occupied: replace an uncursed ring if available
            if (!ring1Slot->isCursed()) {
                backpack.push_back(std::move(ring1Slot));
                ring1Slot = std::move(item);
                equippedRing1 = ring1Slot.get();
                return true;
            } else if (!ring2Slot->isCursed()) {
                backpack.push_back(std::move(ring2Slot));
                ring2Slot = std::move(item);
                equippedRing2 = ring2Slot.get();
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

bool Belongings::equip(Item* item) {
    if (!item || !item->isEquippable() || item->asWand()) return false;
    if (item == equippedWeapon || item == equippedArmor || item == equippedRing1 || item == equippedRing2) {
        return true;
    }

    if (item->category() == ItemCategory::Weapon) {
        if (weaponSlot && weaponSlot->isCursed()) {
            return false;
        }
    } else if (item->category() == ItemCategory::Armor) {
        if (armorSlot && armorSlot->isCursed()) {
            return false;
        }
    } else if (item->category() == ItemCategory::Ring) {
        if (ring1Slot && ring1Slot->isCursed() && ring2Slot && ring2Slot->isCursed()) {
            return false;
        }
    }

    auto it = std::find_if(backpack.begin(), backpack.end(), [item](const auto& p) { return p.get() == item; });
    if (it != backpack.end()) {
        std::unique_ptr<Item> itemPtr = std::move(*it);
        backpack.erase(it);
        if (equip(std::move(itemPtr))) {
            return true;
        } else {
            backpack.push_back(std::move(itemPtr));
            return false;
        }
    }
    return false;
}

bool Belongings::unequip(Item* item) {
    if (!item) return false;
    if (item->isCursed()) return false;
    if (isFull()) return false;

    if (weaponSlot && weaponSlot.get() == item) {
        backpack.push_back(std::move(weaponSlot));
        equippedWeapon = nullptr;
        return true;
    } else if (armorSlot && armorSlot.get() == item) {
        backpack.push_back(std::move(armorSlot));
        equippedArmor = nullptr;
        return true;
    } else if (ring1Slot && ring1Slot.get() == item) {
        backpack.push_back(std::move(ring1Slot));
        equippedRing1 = nullptr;
        return true;
    } else if (ring2Slot && ring2Slot.get() == item) {
        backpack.push_back(std::move(ring2Slot));
        equippedRing2 = nullptr;
        return true;
    }
    return false;
}

std::unique_ptr<Item> Belongings::drop(Item* item) {
    if (!item) return nullptr;
    if (item->isCursed() && (item == equippedWeapon || item == equippedArmor || item == equippedRing1 || item == equippedRing2)) {
        return nullptr;
    }

    if (weaponSlot && weaponSlot.get() == item) {
        std::unique_ptr<Item> dropped = std::move(weaponSlot);
        equippedWeapon = nullptr;
        return dropped;
    } else if (armorSlot && armorSlot.get() == item) {
        std::unique_ptr<Item> dropped = std::move(armorSlot);
        equippedArmor = nullptr;
        return dropped;
    } else if (ring1Slot && ring1Slot.get() == item) {
        std::unique_ptr<Item> dropped = std::move(ring1Slot);
        equippedRing1 = nullptr;
        return dropped;
    } else if (ring2Slot && ring2Slot.get() == item) {
        std::unique_ptr<Item> dropped = std::move(ring2Slot);
        equippedRing2 = nullptr;
        return dropped;
    }

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
    for (auto& it : backpack) {
        if (it && it->name == keyName) {
            Key* k = it->asKey();
            if (!k || depth <= 0 || k->depth == depth) {
                return it.get();
            }
        }
    }
    return nullptr;
}

bool Belongings::consumeKey(const std::string& keyName, int depth) {
    for (auto it = backpack.begin(); it != backpack.end(); ++it) {
        if (*it && (*it)->name == keyName) {
            Key* k = (*it)->asKey();
            if (!k || depth <= 0 || k->depth == depth) {
                (*it)->quantity--;
                if ((*it)->quantity <= 0) {
                    backpack.erase(it);
                }
                return true;
            }
        }
    }
    return false;
}

Item* Belongings::getItem(const std::string& itemName) {
    if (equippedWeapon && equippedWeapon->name == itemName) return equippedWeapon;
    if (equippedArmor && equippedArmor->name == itemName) return equippedArmor;
    if (equippedRing1 && equippedRing1->name == itemName) return equippedRing1;
    if (equippedRing2 && equippedRing2->name == itemName) return equippedRing2;
    for (auto& item : backpack) {
        if (item && item->name == itemName) {
            return item.get();
        }
    }
    return nullptr;
}

void Belongings::observe() {
    if (equippedWeapon) {
        equippedWeapon->identify();
    }
    if (equippedArmor) {
        equippedArmor->identify();
    }
    if (equippedRing1) {
        equippedRing1->identify();
    }
    if (equippedRing2) {
        equippedRing2->identify();
    }
    for (auto& item : backpack) {
        if (item) {
            item->cursedKnown = true;
        }
    }
}

void Belongings::uncurseEquipped() {
    if (equippedWeapon && equippedWeapon->isCursed()) {
        equippedWeapon->cursed = false;
        equippedWeapon->cursedKnown = true;
    }
    if (equippedArmor && equippedArmor->isCursed()) {
        equippedArmor->cursed = false;
        equippedArmor->cursedKnown = true;
    }
    if (equippedRing1 && equippedRing1->isCursed()) {
        equippedRing1->cursed = false;
        equippedRing1->cursedKnown = true;
    }
    if (equippedRing2 && equippedRing2->isCursed()) {
        equippedRing2->cursed = false;
        equippedRing2->cursedKnown = true;
    }
}

void Belongings::identify() {
    if (equippedWeapon) equippedWeapon->identify();
    if (equippedArmor) equippedArmor->identify();
    if (equippedRing1) equippedRing1->identify();
    if (equippedRing2) equippedRing2->identify();
    for (auto& it : backpack) {
        if (it) it->identify();
    }
}

} // namespace pd
