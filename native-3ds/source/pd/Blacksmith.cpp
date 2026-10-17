#include "pd/Blacksmith.h"
#include "pd/Hero.h"
#include "pd/Item.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Badges.h"
#include "pd/Actor.h"
#include <cstdio>

namespace pd {

const char* Blacksmith::verify(const Item* item1, const Item* item2) {
    if (!item1 || !item2) {
        return "Select 2 items!";
    }

    if (item1 == item2) {
        return "Select 2 different items, not the same item twice!";
    }

    if (item1->name != item2->name) {
        return "Select 2 items of the same type!";
    }

    if (!item1->isIdentified() || !item2->isIdentified()) {
        return "I need to know what I'm working with, identify them first!";
    }

    if (item1->cursed || item2->cursed) {
        return "I don't work with cursed items!";
    }

    if (item1->level < 0 || item2->level < 0) {
        return "It's a junk, the quality is too poor!";
    }

    if (!item1->isUpgradable() || !item2->isUpgradable()) {
        return "I can't reforge these items!";
    }

    return nullptr;
}

bool Blacksmith::upgrade(Hero* hero, Item* item1, Item* item2) {
    if (!hero || !item1 || !item2) return false;

    const char* err = verify(item1, item2);
    if (err) {
        GLog::w(err);
        return false;
    }

    Item* first = (item2->level > item1->level) ? item2 : item1;
    Item* second = (item2->level > item1->level) ? item1 : item2;

    Sample::play(Assets::SND_EVOKE);

    // Safe upgrade: preserve enchantment/glyph if present
    if (auto* w = first->asWeapon()) {
        w->upgrade(w->isEnchanted());
    } else if (auto* a = first->asArmor()) {
        a->upgrade(a->isInscribed());
    } else {
        first->upgrade();
    }

    char buf[128];
    std::snprintf(buf, sizeof(buf), "your %s certainly looks better now", first->displayName().c_str());
    GLog::p(buf);

    // Unequip and remove second item
    if (second->isEquipped(hero)) {
        hero->belongings.unequip(second);
    }
    second->quantity = 0;
    hero->belongings.drop(second);

    FX::burst(hero->pos, 0xffd700FF, 12);
    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
    hero->spendAndNext(2.0f * Actor::TICK);

    return true;
}

} // namespace pd

