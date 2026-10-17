#pragma once

#include "Item.h"
#include "Hero.h"
#include <string>

namespace pd {

class Blacksmith {
public:
    // Verifies whether two items can be combined/reforged by the Blacksmith.
    // Returns nullptr if valid, or an error string matching canonical Pixel Dungeon rules.
    static const char* verify(const Item* item1, const Item* item2);

    // Reforges two valid items: upgrades the higher-level one (or item1 if tied),
    // consumes/destroys the second item, and spends 2 turns.
    static bool upgrade(Hero* hero, Item* item1, Item* item2);
};

} // namespace pd

