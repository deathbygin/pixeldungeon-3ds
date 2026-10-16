#include "pd/Item.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/CharSprite.h"
#include "pd/Random.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/Dungeon.h"
#include "pd/Level.h"
#include "pd/Heap.h"
#include "pd/FX.h"
#include "pd/Buff.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Ballistica.h"
#include "noosa/Audio.h"
#include <sstream>
#include <algorithm>

namespace pd {

std::string Item::displayName() const {
    std::string s = name;
    if (levelKnown) {
        if (cursed && cursedKnown) s = "cursed " + s;
        if (level > 0) s += " +" + std::to_string(level);
        else if (level < 0) s += " " + std::to_string(level);
    } else if (cursed && cursedKnown) {
        s = "cursed " + s;
    }
    if (quantity > 1) s += " x" + std::to_string(quantity);
    return s;
}

Item* Item::random() {
    return this;
}

std::unique_ptr<Item> Item::clone(int qty) const {
    auto it = std::make_unique<Item>();
    it->name = name;
    it->image = image;
    it->quantity = qty;
    return it;
}

std::string Item::info() const {
    return "A common dungeon object.";
}

std::vector<std::string> Item::actions(Hero* hero) {
    (void)hero;
    return { "THROW", "DROP" };
}

static bool doDropItem(Hero* hero, Item* item) {
    if (!hero || !item) return false;
    std::string itemName = item->displayName();
    std::unique_ptr<Item> dropped = hero->belongings.drop(item);
    if (dropped && Dungeon::level) {
        Dungeon::level->drop(std::move(dropped), hero->pos);
        GLog::i("You dropped " + itemName);
    }
    hero->spendAndNext(0.5f * Actor::TICK);
    return true;
}

void Item::cast(Hero* hero, int targetCell) {
    if (!hero) return;

    int cell = Ballistica::cast(hero->pos, targetCell, false, true);

    // Hero turns and operates/zaps towards target
    if (hero->sprite) {
        hero->sprite->turnTo(hero->pos, cell);
    }
    HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
    if (hspr) {
        hspr->operate(cell);
    }

    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_MISS).get(), 0.6f, 0.0f);

    float delay = Actor::TICK * speedFactor(hero);

    // Separate 1 unit from stack if stackable
    std::unique_ptr<Item> thrown = nullptr;
    if (quantity > 1) {
        quantity--;
        thrown = clone(1);
    } else {
        thrown = hero->belongings.drop(this);
    }

    if (!thrown) return;

    Item* thrownPtr = thrown.release();

    // Spawn animated parabolic projectile
    FX::missile(hero->pos, cell, thrownPtr->image, [hero, thrownPtr, cell, delay]() {
        thrownPtr->onThrow(cell);
        hero->spendAndNext(delay);
        Actor::process();
        if (Dungeon::level) {
            FX::evolveBlobs(Dungeon::level.get());
        }
        Dungeon::mobs.erase(
            std::remove_if(Dungeon::mobs.begin(), Dungeon::mobs.end(),
                           [](const std::unique_ptr<Mob>& m) { return !m || !m->isAlive(); }),
            Dungeon::mobs.end()
        );
    });
}

void Item::onThrow(int cell) {
    if (Dungeon::level) {
        Char* ch = Actor::findChar(cell);
        if (ch && ch != Dungeon::hero.get()) {
            int dmg = Random::IntRange(1, 2);
            ch->damage(dmg, Dungeon::hero.get());
            FX::floatingText(cell, std::to_string(dmg), 0xe43b44FF);
            GLog::i("The " + displayName() + " strikes " + ch->name + " for " + std::to_string(dmg) + " damage!");
        }

        std::unique_ptr<Item> self(this);
        Dungeon::level->drop(std::move(self), cell);
    }
}

static bool doThrowItem(Hero* hero, Item* item) {
    if (!hero || !item) return false;

    int targetCell = hero->pos;
    Char* nearestEnemy = nullptr;
    float minDist = 999.0f;
    for (const auto& m : Dungeon::mobs) {
        if (m && m->isAlive() && Dungeon::visible[m->pos]) {
            float dist = PathFinder::distanceChebyshev(hero->pos, m->pos);
            if (dist < minDist) {
                minDist = dist;
                nearestEnemy = m.get();
            }
        }
    }

    if (nearestEnemy) {
        targetCell = nearestEnemy->pos;
    } else {
        for (int i = 0; i < 8; ++i) {
            int n = hero->pos + PathFinder::NEIGHBOURS8[i];
            if (Dungeon::level && Dungeon::level->isPassable(n)) {
                targetCell = n;
                break;
            }
        }
    }

    item->cast(hero, targetCell);
    return true;
}

bool Item::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

// -----------------------------------------------------------------------------
// Weapon
// -----------------------------------------------------------------------------
Weapon::Weapon(std::string n, int minD, int maxD, int strReq)
    : minDamage(minD), maxDamage(maxD), STR(strReq) {
    name = std::move(n);
    if (name == "Shortsword") image = ItemSpriteSheet::SHORT_SWORD;
    else if (name == "Longsword") image = ItemSpriteSheet::LONG_SWORD;
    else if (name == "Dagger") image = ItemSpriteSheet::DAGGER;
    else if (name == "Mace") image = ItemSpriteSheet::MACE;
    else if (name == "Battle Axe") image = ItemSpriteSheet::BATTLE_AXE;
    else if (name == "War Hammer") image = ItemSpriteSheet::WAR_HAMMER;
    else if (name == "Sword") image = ItemSpriteSheet::SWORD;
    else if (name == "Knuckles" || name == "Knuckleduster") image = ItemSpriteSheet::KNUCKLEDUSTER;
    else if (name == "Quarterstaff") image = ItemSpriteSheet::QUARTERSTAFF;
    else if (name == "Spear") image = ItemSpriteSheet::SPEAR;
    else if (name == "Glaive") image = ItemSpriteSheet::GLAIVE;
    else {
        if (STR <= 10) image = ItemSpriteSheet::DAGGER;
        else if (STR <= 12) image = ItemSpriteSheet::SHORT_SWORD;
        else if (STR <= 14) image = ItemSpriteSheet::SWORD;
        else if (STR <= 16) image = ItemSpriteSheet::LONG_SWORD;
        else image = ItemSpriteSheet::WAR_HAMMER;
    }
}

int Weapon::typicalSTR() const {
    if (name == "Dagger" || name == "Knuckles" || name == "Knuckleduster") return 10;
    if (name == "Shortsword" || name == "Spear" || name == "Quarterstaff") return 12;
    if (name == "Sword" || name == "Mace") return 14;
    if (name == "Longsword" || name == "Battle Axe") return 16;
    if (name == "War Hammer" || name == "Glaive") return 18;
    return STR + (levelKnown ? level : 0);
}

Item* Weapon::identify() {
    identified = true;
    levelKnown = true;
    cursedKnown = true;
    hitsToKnow = 0;
    return this;
}

Item* Weapon::random() {
    identified = false;
    levelKnown = false;
    cursedKnown = false;
    hitsToKnow = HITS_TO_KNOW;

    if (Random::Float() < 0.40f) {
        int n = 1;
        if (Random::Int(3) == 0) {
            n++;
            if (Random::Int(3) == 0) {
                n++;
            }
        }
        if (Random::Int(2) == 0) {
            level = n;
            cursed = false;
        } else {
            level = -n;
            cursed = true;
        }
    } else {
        level = 0;
        cursed = false;
    }
    return this;
}

std::string Weapon::displayName() const {
    std::string s = name;
    if (levelKnown) {
        if (cursed && cursedKnown) s = "cursed " + s;
        if (level > 0) s += " +" + std::to_string(level);
        else if (level < 0) s += " " + std::to_string(level);
    } else if (cursed && cursedKnown) {
        s = "cursed " + s;
    }
    return s;
}

std::vector<std::string> Weapon::actions(Hero* hero) {
    if (hero && hero->belongings.equippedWeapon == this) {
        return { "UNEQUIP", "THROW", "DROP" };
    }
    return { "EQUIP", "THROW", "DROP" };
}

bool Weapon::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "EQUIP") {
        hero->belongings.equip(this);
        cursedKnown = true;
        if (cursed) {
            FX::burst(hero->pos, 0x550055FFu, 8);
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CURSED).get());
            GLog::n("The " + displayName() + " is cursed! You cannot take it off!");
        }
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "UNEQUIP") {
        if (cursed && hero->belongings.equippedWeapon == this) {
            GLog::w("You can't remove cursed " + displayName() + "!");
            return false;
        }
        hero->belongings.unequip(this);
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "DROP") {
        if (hero->belongings.equippedWeapon == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
            hero->belongings.unequip(this);
        }
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        if (hero->belongings.equippedWeapon == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
            hero->belongings.unequip(this);
        }
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Weapon::info() const {
    if (levelKnown) {
        return "Damage: " + std::to_string(minDamage + level) + "-" + std::to_string(maxDamage + level) +
               " | STR required: " + std::to_string(std::max(1, STR - level));
    } else {
        return "Typical " + name + " deals " + std::to_string(minDamage) + "-" + std::to_string(maxDamage) +
               " damage and requires " + std::to_string(STR) + " points of strength.";
    }
}

float Weapon::acuracyFactor(Hero* hero) const {
    if (!hero) return 1.0f;
    int encumbrance = STR - hero->STR;
    return (encumbrance > 0) ? static_cast<float>(1.0 / std::pow(1.5, encumbrance)) : 1.0f;
}

float Weapon::speedFactor(Hero* hero) const {
    if (!hero) return 1.0f;
    int encumbrance = STR - hero->STR;
    return (encumbrance > 0) ? static_cast<float>(std::pow(1.2, encumbrance)) : 1.0f;
}

int Weapon::damageRoll() const {
    return Random::IntRange(minDamage, maxDamage) + level;
}

int Weapon::damageRoll(Hero* hero) const {
    int dmg = damageRoll();
    if (hero && hero->heroClass != Hero::HeroClass::Huntress) {
        int exStr = hero->STR - STR;
        if (exStr > 0) {
            dmg += Random::IntRange(0, exStr);
        }
    }
    return dmg;
}

void Weapon::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    (void)defender;
    (void)damage;
    if (!levelKnown) {
        if (--hitsToKnow <= 0) {
            identify();
            GLog::i("You are now familiar enough with your " + name + " to identify it. It is " + displayName() + ".");
        }
    }
}

// -----------------------------------------------------------------------------
// ShortSword
// -----------------------------------------------------------------------------
ShortSword::ShortSword() : Weapon("Shortsword", 3, 8, 11) {
    image = ItemSpriteSheet::SHORT_SWORD;
}

std::vector<std::string> ShortSword::actions(Hero* hero) {
    auto acts = Weapon::actions(hero);
    if (level > 0) {
        acts.push_back("REFORGE");
    }
    return acts;
}

bool ShortSword::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "REFORGE") {
        Weapon* target = nullptr;
        if (hero->belongings.equippedWeapon && hero->belongings.equippedWeapon != this) {
            target = hero->belongings.equippedWeapon;
        } else {
            for (auto& it : hero->belongings.backpack) {
                if (it && it.get() != this) {
                    Weapon* w = it->asWeapon();
                    if (w && !w->asBoomerang()) {
                        target = w;
                        break;
                    }
                }
            }
        }
        if (target) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_EVOKE).get());
            target->upgrade();
            target->identify();
            GLog::w("You reforged the shortsword to upgrade your " + target->name + "!");
            FX::burst(hero->pos, 0xffd700FF, 12);
            FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
            hero->spendAndNext(2.0f * Actor::TICK);
            quantity = 0;
            hero->belongings.drop(this);
            return true;
        } else {
            GLog::w("You need another melee weapon to reforge this onto!");
            return false;
        }
    }
    return Weapon::execute(hero, action);
}

std::string ShortSword::info() const {
    return "It is indeed quite short, just a few inches longer than a dagger. When upgraded, it can be reforged to upgrade another melee weapon.";
}

std::unique_ptr<Item> ShortSword::clone(int qty) const {
    auto s = std::make_unique<ShortSword>();
    s->level = level;
    s->quantity = qty;
    s->identified = identified;
    s->levelKnown = levelKnown;
    return s;
}

// -----------------------------------------------------------------------------
// Wand
// -----------------------------------------------------------------------------
Wand::Wand(std::string n, int initialC)
    : Weapon(std::move(n), 2, 7, 10), maxCharges(initialC), curCharges(initialC) {
    image = ItemSpriteSheet::WAND_MAGIC_MISSILE;
}

std::vector<std::string> Wand::actions(Hero* hero) {
    std::vector<std::string> acts;
    if (curCharges > 0) {
        acts.push_back("ZAP");
    }
    if (hero && hero->heroClass == Hero::HeroClass::Mage) {
        if (hero->belongings.equippedWeapon == this) {
            acts.push_back("UNEQUIP");
        } else {
            acts.push_back("EQUIP");
        }
    }
    acts.push_back("THROW");
    acts.push_back("DROP");
    return acts;
}

bool Wand::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "ZAP") {
        if (curCharges <= 0) {
            GLog::w("Your wand fizzles; it must be out of charges for now.");
            hero->spendAndNext(Actor::TICK);
            return true;
        }
        int targetCell = hero->pos;
        Char* nearestEnemy = nullptr;
        float minDist = 999.0f;
        for (const auto& m : Dungeon::mobs) {
            if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                float dist = static_cast<float>(PathFinder::distanceChebyshev(hero->pos, m->pos));
                if (dist < minDist) {
                    minDist = dist;
                    nearestEnemy = m.get();
                }
            }
        }
        if (nearestEnemy) {
            targetCell = nearestEnemy->pos;
        } else {
            for (int i = 0; i < 8; ++i) {
                int n = hero->pos + PathFinder::NEIGHBOURS8[i];
                if (Dungeon::level && Dungeon::level->isPassable(n)) {
                    targetCell = n;
                    break;
                }
            }
        }

        cast(hero, targetCell);
        return true;
    }
    return Weapon::execute(hero, action);
}

std::string Wand::info() const {
    std::string s = "A magical wand. When zapped, it unleashes potent arcane energy.";
    if (Dungeon::hero && Dungeon::hero->heroClass == Hero::HeroClass::Mage) {
        s += "\n\nYou can also equip this wand as a melee weapon.";
    }
    return s;
}

std::string Wand::displayName() const {
    std::string s = Weapon::displayName();
    s += " (" + std::to_string(curCharges) + "/" + std::to_string(maxCharges) + ")";
    return s;
}

void Wand::updateCharge(float dt, Hero* hero) {
    if (curCharges >= maxCharges) {
        chargeTimer = 0.0f;
        return;
    }
    chargeTimer += dt;
    float timeToCharge = 40.0f;
    if (hero && hero->heroClass == Hero::HeroClass::Mage) {
        timeToCharge = 40.0f / std::sqrt(1.0f + static_cast<float>(level));
    }
    if (chargeTimer >= timeToCharge) {
        chargeTimer -= timeToCharge;
        curCharges++;
    }
}

void Wand::restoreCharge(int count) {
    curCharges = std::min(maxCharges, curCharges + count);
}

void Wand::upgrade() {
    Weapon::upgrade();
    maxCharges = std::min(9, maxCharges + 1);
    curCharges = std::min(maxCharges, curCharges + 1);
}

void Wand::degrade() {
    Weapon::degrade();
    maxCharges = std::max(1, maxCharges - 1);
    curCharges = std::min(maxCharges, curCharges);
}

void Wand::onZap(int cell) {
    (void)cell;
}

void Wand::cast(Hero* hero, int targetCell) {
    if (!hero) return;

    if (curCharges <= 0) {
        GLog::w("Your wand fizzles; it must be out of charges for now.");
        hero->spendAndNext(Actor::TICK);
        return;
    }

    int cell = Ballistica::cast(hero->pos, targetCell, true, true);
    if (hero->getSprite()) {
        hero->getSprite()->turnTo(hero->pos, cell);
        HeroSprite* hspr = hero->getSprite()->asHeroSprite();
        if (hspr) hspr->operate(cell);
    }

    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_ZAP).get());
    curCharges--;
    onZap(cell);
    hero->spendAndNext(Actor::TICK);
    Actor::process();
    if (Dungeon::level) {
        FX::evolveBlobs(Dungeon::level.get());
    }
    Dungeon::mobs.erase(
        std::remove_if(Dungeon::mobs.begin(), Dungeon::mobs.end(),
                       [](const std::unique_ptr<Mob>& m) { return !m || !m->isAlive(); }),
        Dungeon::mobs.end()
    );
}

// -----------------------------------------------------------------------------
// WandOfMagicMissile
// -----------------------------------------------------------------------------
WandOfMagicMissile::WandOfMagicMissile() : Wand("Wand of Magic Missile", 3) {
    image = ItemSpriteSheet::WAND_MAGIC_MISSILE;
    identified = true;
    levelKnown = true;
}

std::vector<std::string> WandOfMagicMissile::actions(Hero* hero) {
    auto acts = Wand::actions(hero);
    if (level > 0) {
        acts.push_back("DISENCHANT");
    }
    return acts;
}

bool WandOfMagicMissile::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "DISENCHANT") {
        Wand* targetWand = nullptr;
        for (auto& it : hero->belongings.backpack) {
            if (it && it.get() != this) {
                Wand* w = it->asWand();
                if (w) {
                    targetWand = w;
                    break;
                }
            }
        }
        if (targetWand) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_EVOKE).get());
            targetWand->upgrade();
            targetWand->identify();
            GLog::w("You disenchanted the Wand of Magic Missile to upgrade your " + targetWand->name + "!");
            FX::burst(hero->pos, 0x99ccffFF, 12);
            FX::floatingText(hero->pos, "+1 LVL", 0x99ccffFF);
            hero->spendAndNext(2.0f * Actor::TICK);
            quantity = 0;
            hero->belongings.drop(this);
            return true;
        } else {
            GLog::w("You need another wand to disenchant this into!");
            return false;
        }
    }
    return Wand::execute(hero, action);
}

void WandOfMagicMissile::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch) {
        int dmg = Random::IntRange(1, 6 + level * 2);
        ch->damage(dmg, Dungeon::hero.get());
        FX::burst(cell, 0x99ccffFF, 8 + level);
        FX::floatingText(cell, std::to_string(dmg), 0x99ccffFF);
        GLog::i("The magic missile strikes " + ch->name + " for " + std::to_string(dmg) + " damage!");
    } else {
        FX::burst(cell, 0x99ccffFF, 6);
    }
}

std::string WandOfMagicMissile::info() const {
    return "This wand launches missiles of pure magical energy, dealing moderate damage to a target creature.";
}

std::unique_ptr<Item> WandOfMagicMissile::clone(int qty) const {
    auto w = std::make_unique<WandOfMagicMissile>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// Armor
// -----------------------------------------------------------------------------
Armor::Armor(std::string n, int t, int strReq)
    : tier(t), STR(strReq) {
    name = std::move(n);
    switch (tier) {
        case 1: image = ItemSpriteSheet::ARMOR_CLOTH; break;
        case 2: image = ItemSpriteSheet::ARMOR_LEATHER; break;
        case 3: image = ItemSpriteSheet::ARMOR_MAIL; break;
        case 4: image = ItemSpriteSheet::ARMOR_SCALE; break;
        case 5: image = ItemSpriteSheet::ARMOR_PLATE; break;
        default: image = (tier <= 1) ? ItemSpriteSheet::ARMOR_CLOTH : ItemSpriteSheet::ARMOR_PLATE; break;
    }
}

int Armor::typicalSTR() const {
    if (tier >= 1 && tier <= 5) return 7 + tier * 2;
    return STR + (levelKnown ? level : 0);
}

Item* Armor::identify() {
    identified = true;
    levelKnown = true;
    cursedKnown = true;
    hitsToKnow = 0;
    return this;
}

Item* Armor::random() {
    identified = false;
    levelKnown = false;
    cursedKnown = false;
    hitsToKnow = HITS_TO_KNOW;

    if (Random::Float() < 0.40f) {
        int n = 1;
        if (Random::Int(3) == 0) {
            n++;
            if (Random::Int(3) == 0) {
                n++;
            }
        }
        if (Random::Int(2) == 0) {
            level = n;
            cursed = false;
        } else {
            level = -n;
            cursed = true;
        }
    } else {
        level = 0;
        cursed = false;
    }
    return this;
}

std::string Armor::displayName() const {
    std::string s = name;
    if (levelKnown) {
        if (cursed && cursedKnown) s = "cursed " + s;
        if (level > 0) s += " +" + std::to_string(level);
        else if (level < 0) s += " " + std::to_string(level);
    } else if (cursed && cursedKnown) {
        s = "cursed " + s;
    }
    return s;
}

std::vector<std::string> Armor::actions(Hero* hero) {
    if (hero && hero->belongings.equippedArmor == this) {
        return { "UNEQUIP", "THROW", "DROP" };
    }
    return { "EQUIP", "THROW", "DROP" };
}

bool Armor::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "EQUIP") {
        hero->belongings.equip(this);
        cursedKnown = true;
        if (cursed) {
            FX::burst(hero->pos, 0x550055FFu, 8);
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CURSED).get());
            GLog::n("The " + displayName() + " is cursed! You cannot take it off!");
        }
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "UNEQUIP") {
        if (cursed && hero->belongings.equippedArmor == this) {
            GLog::w("You can't remove cursed " + displayName() + "!");
            return false;
        }
        hero->belongings.unequip(this);
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "DROP") {
        if (hero->belongings.equippedArmor == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
            hero->belongings.unequip(this);
        }
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        if (hero->belongings.equippedArmor == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
            hero->belongings.unequip(this);
        }
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Armor::info() const {
    if (levelKnown) {
        return "Defense: Tier " + std::to_string(tier) + " (Max DR " + std::to_string(tier * 2 + level) +
               ") | STR required: " + std::to_string(std::max(1, STR - level));
    } else {
        return "Typical " + name + " provides damage absorption up to " + std::to_string(tier * 2) +
               " points per attack and requires " + std::to_string(STR) + " points of strength.";
    }
}

int Armor::drRoll() const {
    return Random::IntRange(0, tier * 2 + level);
}

int Armor::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    (void)defender;
    if (!levelKnown) {
        if (--hitsToKnow <= 0) {
            identify();
            GLog::w("You are now familiar enough with your " + name + " to identify it. It is " + displayName() + ".");
        }
    }
    return damage;
}

// -----------------------------------------------------------------------------
// Potion
// -----------------------------------------------------------------------------
static const std::string s_potionColors[Potion::POTION_COUNT] = {
    "turquoise", "crimson", "azure", "jade", "golden", "magenta",
    "charcoal", "ivory", "amber", "bistre", "indigo", "silver"
};

static const int s_potionImages[Potion::POTION_COUNT] = {
    ItemSpriteSheet::POTION_TURQUOISE,
    ItemSpriteSheet::POTION_CRIMSON,
    ItemSpriteSheet::POTION_AZURE,
    ItemSpriteSheet::POTION_JADE,
    ItemSpriteSheet::POTION_GOLDEN,
    ItemSpriteSheet::POTION_MAGENTA,
    ItemSpriteSheet::POTION_CHARCOAL,
    ItemSpriteSheet::POTION_IVORY,
    ItemSpriteSheet::POTION_AMBER,
    ItemSpriteSheet::POTION_BISTRE,
    ItemSpriteSheet::POTION_INDIGO,
    ItemSpriteSheet::POTION_SILVER
};

static const std::string s_potionTrueNames[Potion::POTION_COUNT] = {
    "Potion of Healing",
    "Potion of Experience",
    "Potion of Toxic Gas",
    "Potion of Liquid Flame",
    "Potion of Strength",
    "Potion of Paralytic Gas",
    "Potion of Levitation",
    "Potion of Mind Vision",
    "Potion of Purity",
    "Potion of Invisibility",
    "Potion of Might",
    "Potion of Frost"
};

static const std::string s_potionTrueDescs[Potion::POTION_COUNT] = {
    "An elixir that will instantly return you to full health and cure harmful ailments.",
    "A glowing draught that permanently increases your experience level.",
    "Uncorking or shattering this pressurized glass causes a deadly cloud of toxic green gas to explode outward.",
    "A volatile liquid that bursts into roaring flames upon contact with air.",
    "A potent draught that permanently increases your strength by 1 point.",
    "A dense vapor that instantly paralyzes anyone caught within its reach.",
    "A buoyant elixir that allows you to float harmlessly above traps and chasms.",
    "A mystical brew that allows your mind to pierce darkness and sense all creatures on this depth.",
    "A clarifying liquid that purges gases, poisons, and debuffs from the body and immediate area.",
    "A shimmering potion that renders you completely invisible to all enemies for a duration.",
    "An extraordinary brew that grants +1 strength and permanently raises maximum health by 5 points.",
    "A freezing draught that releases a blast of subzero frost, freezing enemies and liquids solid."
};

static std::vector<int> s_potionColorMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static std::vector<bool> s_potionKnown(Potion::POTION_COUNT, false);

void Potion::initColors() {
    s_potionColorMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int i = static_cast<int>(POTION_COUNT) - 1; i > 0; --i) {
        int j = Random::Int(i + 1);
        std::swap(s_potionColorMap[i], s_potionColorMap[j]);
    }
    std::fill(s_potionKnown.begin(), s_potionKnown.end(), false);
}

bool Potion::isKnown(PotionType tp) {
    size_t idx = static_cast<size_t>(tp);
    return idx < POTION_COUNT ? s_potionKnown[idx] : false;
}

void Potion::setKnown(PotionType tp) {
    size_t idx = static_cast<size_t>(tp);
    if (idx >= POTION_COUNT) return;
    s_potionKnown[idx] = true;

    // 11 out of 12 deduction rule from original Pixel Dungeon
    int knownCount = 0;
    int unknownIdx = -1;
    for (size_t i = 0; i < POTION_COUNT; ++i) {
        if (s_potionKnown[i]) {
            knownCount++;
        } else {
            unknownIdx = static_cast<int>(i);
        }
    }
    if (knownCount == static_cast<int>(POTION_COUNT) - 1 && unknownIdx >= 0) {
        s_potionKnown[unknownIdx] = true;
    }

    // Refresh perception in hero pack and ground heaps
    if (Dungeon::hero) {
        for (auto& it : Dungeon::hero->belongings.backpack) {
            if (it && it->category() == ItemCategory::Potion) {
                static_cast<Potion*>(it.get())->updatePerception();
            }
        }
    }
    if (Dungeon::level) {
        for (auto& heap : Dungeon::level->heaps) {
            if (heap) {
                for (auto& it : heap->items) {
                    if (it && it->category() == ItemCategory::Potion) {
                        static_cast<Potion*>(it.get())->updatePerception();
                    }
                }
            }
        }
    }
}

const std::string& Potion::colorName(PotionType tp) {
    size_t idx = static_cast<size_t>(tp);
    int cIdx = (idx < POTION_COUNT) ? s_potionColorMap[idx] : 0;
    return s_potionColors[cIdx];
}

int Potion::imageForType(PotionType tp) {
    size_t idx = static_cast<size_t>(tp);
    int cIdx = (idx < POTION_COUNT) ? s_potionColorMap[idx] : 0;
    return s_potionImages[cIdx];
}

const std::string& Potion::trueName(PotionType tp) {
    size_t idx = static_cast<size_t>(tp);
    return (idx < POTION_COUNT) ? s_potionTrueNames[idx] : s_potionTrueNames[0];
}

const std::string& Potion::trueDesc(PotionType tp) {
    size_t idx = static_cast<size_t>(tp);
    return (idx < POTION_COUNT) ? s_potionTrueDescs[idx] : s_potionTrueDescs[0];
}

const std::vector<int>& Potion::getColorMapping() {
    return s_potionColorMap;
}

const std::vector<bool>& Potion::getKnownTable() {
    return s_potionKnown;
}

void Potion::restoreStatus(const std::vector<int>& mapping, const std::vector<bool>& known) {
    if (mapping.size() == POTION_COUNT) {
        s_potionColorMap = mapping;
    }
    if (known.size() == POTION_COUNT) {
        s_potionKnown = known;
    }
}

Potion::Potion(PotionType tp) : type(tp) {
    updatePerception();
}

void Potion::updatePerception() {
    image = imageForType(type);
    identified = isKnown(type);
    levelKnown = identified;
    cursedKnown = true;
    name = identified ? trueName(type) : (colorName(type) + " potion");
}

Item* Potion::identify() {
    setKnown(type);
    updatePerception();
    return this;
}

std::string Potion::displayName() const {
    std::string s = isKnown(type) ? trueName(type) : (colorName(type) + " potion");
    if (quantity > 1) s += " x" + std::to_string(quantity);
    return s;
}

std::unique_ptr<Item> Potion::clone(int qty) const {
    auto p = std::make_unique<Potion>(type);
    p->quantity = qty;
    return p;
}

std::vector<std::string> Potion::actions(Hero* hero) {
    (void)hero;
    return { "DRINK", "THROW", "DROP" };
}

bool Potion::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;

    if (action == "DRINK") {
        HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
        if (hspr) hspr->operate(hero->pos);

        setKnown(type);
        updatePerception();

        switch (type) {
            case PotionType::HEALING: {
                hero->HP = hero->HT;
                hero->removeBuff(BuffType::Poison);
                hero->removeBuff(BuffType::Cripple);
                hero->removeBuff(BuffType::Weakness);
                hero->removeBuff(BuffType::Bleeding);
                FX::floatingText(hero->pos, "HEALED", 0x38b764FF);
                FX::burst(hero->pos, 0x38b764FF, 12);
                GLog::p("Your wounds heal completely.");
                break;
            }
            case PotionType::STRENGTH:
                hero->STR += 1;
                FX::floatingText(hero->pos, "+1 STR", 0xe4a672FF);
                FX::burst(hero->pos, 0xe4a672FF, 8);
                GLog::p("Newfound strength surges through your body.");
                break;
            case PotionType::INVISIBILITY:
                hero->affect<Invisibility>();
                FX::floatingText(hero->pos, "INVISIBLE", 0xa5cde4FF);
                FX::burst(hero->pos, 0xa5cde4FF, 10);
                GLog::i("You see your hands turn invisible!");
                break;
            case PotionType::EXPERIENCE:
                hero->earnExp(hero->maxExp - hero->exp);
                FX::burst(hero->pos, 0xffd700FF, 10);
                GLog::p("You feel more experienced!");
                break;
            case PotionType::TOXIC_GAS:
                shatter(hero->pos);
                break;
            case PotionType::LIQUID_FLAME:
                shatter(hero->pos);
                break;
            case PotionType::PARALYTIC_GAS:
                shatter(hero->pos);
                break;
            case PotionType::FROST:
                shatter(hero->pos);
                break;
            case PotionType::LEVITATION:
                hero->affect<Levitation>();
                FX::burst(hero->pos, 0xdda0ddFF, 8);
                GLog::p("You float into the air!");
                break;
            case PotionType::MIND_VISION:
                hero->affect<MindVision>();
                Dungeon::observe();
                FX::burst(hero->pos, 0x9955ffFF, 12);
                GLog::p("You can sense the minds of creatures on this floor!");
                break;
            case PotionType::PURITY:
                hero->affect<GasesImmunity>();
                FX::burst(hero->pos, 0xffffffFF, 12);
                GLog::w("You've stopped sensing any smells!");
                break;
            case PotionType::MIGHT:
                hero->STR += 1;
                hero->HT += 5;
                hero->HP = std::min(hero->HT, hero->HP + 5);
                FX::burst(hero->pos, 0xffd700FF, 12);
                GLog::p("Newfound strength surges through your body.");
                break;
        }

        quantity--;
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    }

    return false;
}

std::string Potion::info() const {
    if (isKnown(type)) {
        return trueDesc(type);
    } else {
        return "This flask contains a swirling " + colorName(type) + " liquid. Who knows what it will do when drunk or thrown?";
    }
}

void Potion::onThrow(int cell) {
    shatter(cell);
    delete this;
}

void Potion::shatter(int cell) {
    if (cell < 0 || cell >= Level::LENGTH) return;

    if (Dungeon::visible[cell]) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_SHATTER).get());
        FX::splash(cell, 0x88ccffFF, 8);
    }

    Char* targetChar = Actor::findChar(cell);

    switch (type) {
        case PotionType::TOXIC_GAS: {
            if (Dungeon::visible[cell]) setKnown(type);
            FX::burst(cell, 0x00ff00FF, 16);
            FX::seedBlob(cell, Blob::BlobType::TOXIC_GAS, 1000);
            if (Dungeon::visible[cell]) {
                GLog::i("The flask shatters and a cloud of toxic gas billows out!");
            }
            break;
        }
        case PotionType::LIQUID_FLAME: {
            if (Dungeon::visible[cell]) setKnown(type);
            FX::burst(cell, 0xff4500FF, 16);
            FX::seedBlob(cell, Blob::BlobType::FIRE, 4);
            if (targetChar) {
                targetChar->affect<Burning>();
            }
            if (Dungeon::visible[cell]) {
                GLog::i("The flask shatters and liquid flame bursts into a roaring inferno!");
            }
            break;
        }
        case PotionType::PARALYTIC_GAS: {
            if (Dungeon::visible[cell]) setKnown(type);
            FX::burst(cell, 0xffff00FF, 16);
            FX::seedBlob(cell, Blob::BlobType::PARALYTIC_GAS, 1000);
            if (Dungeon::visible[cell]) {
                GLog::i("The flask shatters into a cloud of paralyzing vapor!");
            }
            break;
        }
        case PotionType::FROST: {
            if (Dungeon::visible[cell]) setKnown(type);
            Blob* fire = FX::getBlob(Blob::BlobType::FIRE);
            int cx = cell % Level::WIDTH;
            int cy = cell / Level::WIDTH;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    if (std::abs(dx) + std::abs(dy) > 3) continue;
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < Level::WIDTH && ny >= 0 && ny < Level::HEIGHT) {
                        int c = ny * Level::WIDTH + nx;
                        if (fire && fire->cur[c] > 0) {
                            fire->volume -= fire->cur[c];
                            fire->cur[c] = 0;
                        }
                        Char* ch = Actor::findChar(c);
                        if (ch) {
                            ch->affect<Frost>();
                        }
                        if (Dungeon::visible[c]) {
                            FX::burst(c, 0x00ffffff, 4);
                        }
                    }
                }
            }
            if (Dungeon::visible[cell]) {
                GLog::i("The flask shatters and freezing frost encases the area!");
            }
            break;
        }
        case PotionType::PURITY: {
            Blob* toxic = FX::getBlob(Blob::BlobType::TOXIC_GAS);
            Blob* para = FX::getBlob(Blob::BlobType::PARALYTIC_GAS);
            int cx = cell % Level::WIDTH;
            int cy = cell / Level::WIDTH;
            bool procd = false;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    if (std::abs(dx) + std::abs(dy) > 3) continue;
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < Level::WIDTH && ny >= 0 && ny < Level::HEIGHT) {
                        int c = ny * Level::WIDTH + nx;
                        if (toxic && toxic->cur[c] > 0) {
                            toxic->volume -= toxic->cur[c];
                            toxic->cur[c] = 0;
                            procd = true;
                        }
                        if (para && para->cur[c] > 0) {
                            para->volume -= para->cur[c];
                            para->cur[c] = 0;
                            procd = true;
                        }
                        if (Dungeon::visible[c]) {
                            FX::burst(c, 0xffffffFF, 2);
                        }
                    }
                }
            }
            if (Dungeon::visible[cell] || procd) {
                setKnown(type);
            }
            int hx = Dungeon::hero->pos % Level::WIDTH;
            int hy = Dungeon::hero->pos / Level::WIDTH;
            if (std::abs(hx - cx) <= 2 && std::abs(hy - cy) <= 2) {
                GLog::p("You feel uncommon freshness in the air.");
                setKnown(type);
            }
            break;
        }
        case PotionType::HEALING: {
            if (targetChar) {
                targetChar->HP = targetChar->HT;
                targetChar->removeBuff(BuffType::Poison);
                targetChar->removeBuff(BuffType::Cripple);
                targetChar->removeBuff(BuffType::Weakness);
                targetChar->removeBuff(BuffType::Bleeding);
                FX::burst(cell, 0x38b764FF, 12);
                FX::floatingText(cell, "HEALED", 0x38b764FF);
                if (Dungeon::visible[cell]) setKnown(type);
            } else if (Dungeon::visible[cell]) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
            }
            break;
        }
        default: {
            if (Dungeon::visible[cell]) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
            }
            break;
        }
    }

    if (Dungeon::level) {
        Dungeon::level->press(cell, nullptr);
    }
}

// -----------------------------------------------------------------------------
// Scroll
// -----------------------------------------------------------------------------
static const std::string s_scrollRunes[Scroll::SCROLL_COUNT] = {
    "KAUNAN", "SOWILO", "LAGUZ", "YNGVI", "GYFU", "RAIDO",
    "ISAZ", "MANNAZ", "NAUDIZ", "BERKANAN", "ODAL", "TIWAZ"
};

static const int s_scrollImages[Scroll::SCROLL_COUNT] = {
    ItemSpriteSheet::SCROLL_KAUNAN,
    ItemSpriteSheet::SCROLL_SOWILO,
    ItemSpriteSheet::SCROLL_LAGUZ,
    ItemSpriteSheet::SCROLL_YNGVI,
    ItemSpriteSheet::SCROLL_GYFU,
    ItemSpriteSheet::SCROLL_RAIDO,
    ItemSpriteSheet::SCROLL_ISAZ,
    ItemSpriteSheet::SCROLL_MANNAZ,
    ItemSpriteSheet::SCROLL_NAUDIZ,
    ItemSpriteSheet::SCROLL_BERKANAN,
    ItemSpriteSheet::SCROLL_ODAL,
    ItemSpriteSheet::SCROLL_TIWAZ
};

static const std::string s_scrollTrueNames[Scroll::SCROLL_COUNT] = {
    "Scroll of Identify",
    "Scroll of Magic Mapping",
    "Scroll of Recharging",
    "Scroll of Remove Curse",
    "Scroll of Teleportation",
    "Scroll of Challenge",
    "Scroll of Terror",
    "Scroll of Lullaby",
    "Scroll of Psionic Blast",
    "Scroll of Mirror Image",
    "Scroll of Upgrade",
    "Scroll of Enchantment"
};

static const std::string s_scrollTrueDescs[Scroll::SCROLL_COUNT] = {
    "Permanently reveals all secrets and enchantments of a chosen item.",
    "Etches a complete mental layout of this floor directly into your mind.",
    "Infuses all wands and magical items with surging power and renewed charges.",
    "A cleansing incantation that purges evil curses binding equipped items.",
    "Warps reality to instantly teleport you to a safe, unexplored room.",
    "Unleashes a deafening challenge that provokes all creatures on this floor to pursue you.",
    "Strikes primal dread into all creatures in sight, causing them to flee.",
    "Inscribes a lullaby that puts all visible monsters into a deep magical slumber.",
    "Unleashes devastating psychic shockwaves that harm and disorient all visible enemies.",
    "Conjures illusory mirror clones that confuse foes and fight at your side.",
    "Permanently enhances an item's statistics and cleanses harmful curses.",
    "Infuses an item with rare, mystical enchantments and arcane power."
};

static std::vector<int> s_scrollRuneMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static std::vector<bool> s_scrollKnown(Scroll::SCROLL_COUNT, false);

void Scroll::initLabels() {
    s_scrollRuneMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int i = static_cast<int>(SCROLL_COUNT) - 1; i > 0; --i) {
        int j = Random::Int(i + 1);
        std::swap(s_scrollRuneMap[i], s_scrollRuneMap[j]);
    }
    std::fill(s_scrollKnown.begin(), s_scrollKnown.end(), false);
}

bool Scroll::isKnown(ScrollType tp) {
    size_t idx = static_cast<size_t>(tp);
    return idx < SCROLL_COUNT ? s_scrollKnown[idx] : false;
}

void Scroll::setKnown(ScrollType tp) {
    size_t idx = static_cast<size_t>(tp);
    if (idx >= SCROLL_COUNT) return;
    s_scrollKnown[idx] = true;

    // 11 out of 12 deduction rule
    int knownCount = 0;
    int unknownIdx = -1;
    for (size_t i = 0; i < SCROLL_COUNT; ++i) {
        if (s_scrollKnown[i]) {
            knownCount++;
        } else {
            unknownIdx = static_cast<int>(i);
        }
    }
    if (knownCount == static_cast<int>(SCROLL_COUNT) - 1 && unknownIdx >= 0) {
        s_scrollKnown[unknownIdx] = true;
    }

    // Refresh perception in hero pack and ground heaps
    if (Dungeon::hero) {
        for (auto& it : Dungeon::hero->belongings.backpack) {
            if (it && it->category() == ItemCategory::Scroll) {
                static_cast<Scroll*>(it.get())->updatePerception();
            }
        }
    }
    if (Dungeon::level) {
        for (auto& heap : Dungeon::level->heaps) {
            if (heap) {
                for (auto& it : heap->items) {
                    if (it && it->category() == ItemCategory::Scroll) {
                        static_cast<Scroll*>(it.get())->updatePerception();
                    }
                }
            }
        }
    }
}

const std::string& Scroll::runeName(ScrollType tp) {
    size_t idx = static_cast<size_t>(tp);
    int rIdx = (idx < SCROLL_COUNT) ? s_scrollRuneMap[idx] : 0;
    return s_scrollRunes[rIdx];
}

int Scroll::imageForType(ScrollType tp) {
    size_t idx = static_cast<size_t>(tp);
    int rIdx = (idx < SCROLL_COUNT) ? s_scrollRuneMap[idx] : 0;
    return s_scrollImages[rIdx];
}

const std::string& Scroll::trueName(ScrollType tp) {
    size_t idx = static_cast<size_t>(tp);
    return (idx < SCROLL_COUNT) ? s_scrollTrueNames[idx] : s_scrollTrueNames[0];
}

const std::string& Scroll::trueDesc(ScrollType tp) {
    size_t idx = static_cast<size_t>(tp);
    return (idx < SCROLL_COUNT) ? s_scrollTrueDescs[idx] : s_scrollTrueDescs[0];
}

const std::vector<int>& Scroll::getRuneMapping() {
    return s_scrollRuneMap;
}

const std::vector<bool>& Scroll::getKnownTable() {
    return s_scrollKnown;
}

void Scroll::restoreStatus(const std::vector<int>& mapping, const std::vector<bool>& known) {
    if (mapping.size() == SCROLL_COUNT) {
        s_scrollRuneMap = mapping;
    }
    if (known.size() == SCROLL_COUNT) {
        s_scrollKnown = known;
    }
}

Scroll::Scroll(ScrollType tp) : type(tp) {
    updatePerception();
}

void Scroll::updatePerception() {
    image = imageForType(type);
    identified = isKnown(type);
    levelKnown = identified;
    cursedKnown = true;
    name = identified ? trueName(type) : ("scroll \"" + runeName(type) + "\"");
}

Item* Scroll::identify() {
    setKnown(type);
    updatePerception();
    return this;
}

std::string Scroll::displayName() const {
    std::string s = isKnown(type) ? trueName(type) : ("scroll \"" + runeName(type) + "\"");
    if (quantity > 1) s += " x" + std::to_string(quantity);
    return s;
}

std::unique_ptr<Item> Scroll::clone(int qty) const {
    auto s = std::make_unique<Scroll>(type);
    s->quantity = qty;
    return s;
}

std::vector<std::string> Scroll::actions(Hero* hero) {
    (void)hero;
    return { "READ", "THROW", "DROP" };
}

bool Scroll::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;

    if (action == "READ") {
        HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
        if (hspr) hspr->read();

        setKnown(type);
        updatePerception();

        switch (type) {
            case ScrollType::IDENTIFY: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                FX::burst(hero->pos, 0x00ffffFFu, 12);
                bool found = false;
                for (auto& item : hero->belongings.backpack) {
                    if (item && !item->isIdentified()) {
                        item->identify();
                        FX::floatingText(hero->pos, "IDENTIFIED", 0x00ffffFFu);
                        GLog::i("It is " + item->displayName() + "!");
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    if (hero->belongings.equippedWeapon && !hero->belongings.equippedWeapon->isIdentified()) {
                        hero->belongings.equippedWeapon->identify();
                        GLog::i("It is " + hero->belongings.equippedWeapon->displayName() + "!");
                    } else if (hero->belongings.equippedArmor && !hero->belongings.equippedArmor->isIdentified()) {
                        hero->belongings.equippedArmor->identify();
                        GLog::i("It is " + hero->belongings.equippedArmor->displayName() + "!");
                    } else {
                        GLog::i("You have nothing to identify.");
                    }
                }
                break;
            }
            case ScrollType::UPGRADE:
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                if (hero->belongings.equippedWeapon) {
                    hero->belongings.equippedWeapon->upgrade();
                    FX::burst(hero->pos, 0xffd700FF, 10);
                    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                    GLog::p("Your weapon glows and grows sharper!");
                } else if (hero->belongings.equippedArmor) {
                    hero->belongings.equippedArmor->upgrade();
                    FX::burst(hero->pos, 0xffd700FF, 10);
                    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                    GLog::p("Your armor glows and reinforces!");
                }
                break;
            case ScrollType::TELEPORTATION: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                if (Dungeon::level) {
                    for (int tries = 0; tries < 100; ++tries) {
                        int cell = Random::Int(Level::LENGTH);
                        if (Dungeon::level->isPassable(cell) && !Actor::findChar(cell)) {
                            FX::burst(hero->pos, 0x9955ffFF, 8);
                            hero->place(cell);
                            FX::burst(hero->pos, 0x9955ffFF, 8);
                            Dungeon::observe();
                            GLog::p("You feel a sudden shift in space!");
                            break;
                        }
                    }
                }
                break;
            }
            case ScrollType::MAGIC_MAPPING: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                if (Dungeon::level) {
                    for (int i = 0; i < Level::LENGTH; ++i) {
                        if (Dungeon::level->map[i] != Terrain::WALL) {
                            Dungeon::level->mapped[i] = true;
                            Dungeon::level->visited[i] = true;
                        }
                    }
                    Dungeon::observe();
                    FX::burst(hero->pos, 0x44aaffFF, 12);
                    GLog::p("You now sense the layout of this level!");
                }
                break;
            }
            case ScrollType::REMOVE_CURSE: {
                bool procced = false;
                if (hero->belongings.equippedWeapon && hero->belongings.equippedWeapon->cursed) {
                    hero->belongings.equippedWeapon->cursed = false;
                    procced = true;
                }
                if (hero->belongings.equippedArmor && hero->belongings.equippedArmor->cursed) {
                    hero->belongings.equippedArmor->cursed = false;
                    procced = true;
                }
                if (hero->belongings.equippedRing1 && hero->belongings.equippedRing1->cursed) {
                    hero->belongings.equippedRing1->cursed = false;
                    procced = true;
                }
                if (hero->belongings.equippedRing2 && hero->belongings.equippedRing2->cursed) {
                    hero->belongings.equippedRing2->cursed = false;
                    procced = true;
                }
                for (auto& item : hero->belongings.backpack) {
                    if (item && item->cursed) {
                        item->cursed = false;
                        procced = true;
                    }
                }
                hero->removeBuff(BuffType::Weakness);
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                FX::burst(hero->pos, 0xdda0ddFFu, 16);
                if (procced) {
                    GLog::p("Your pack glows with a cleansing light, and a malevolent energy disperses.");
                } else {
                    GLog::i("Your pack glows with a cleansing light, but nothing happens.");
                }
                break;
            }
            case ScrollType::RECHARGING: {
                int count = 0;
                if (hero->belongings.equippedWeapon) {
                    if (auto* w = hero->belongings.equippedWeapon->asWand()) {
                        if (w->curCharges < w->maxCharges) {
                            w->curCharges = w->maxCharges;
                            count++;
                        }
                    }
                }
                for (auto& item : hero->belongings.backpack) {
                    if (item) {
                        if (auto* w = item->asWand()) {
                            if (w->curCharges < w->maxCharges) {
                                w->curCharges = w->maxCharges;
                                count++;
                            }
                        }
                    }
                }
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                FX::burst(hero->pos, 0x55ffaaFF, 15);
                if (count > 0) {
                    GLog::i("A surge of energy courses through your pack, recharging your wand" + std::string(count > 1 ? "s" : "") + "!");
                } else {
                    GLog::i("A surge of energy courses through your pack, but nothing happens.");
                }
                break;
            }
            case ScrollType::CHALLENGE: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CHALLENGE).get());
                FX::burst(hero->pos, 0xff0000FF, 16);
                GLog::w("The scroll emits a challenging roar that echoes throughout the dungeon!");
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive()) {
                        m->removeBuff(BuffType::Sleep);
                        m->beckon(hero->pos);
                        if (Dungeon::visible[m->pos]) {
                            m->affect<Rage>();
                        }
                    }
                }
                break;
            }
            case ScrollType::TERROR: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                FX::burst(hero->pos, 0xff0000FF, 16);
                int count = 0;
                std::string affectedName = "";
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                        m->affect<Terror>();
                        count++;
                        affectedName = m->name;
                    }
                }
                if (count == 0) {
                    GLog::i("The scroll emits a brilliant flash of red light.");
                } else if (count == 1) {
                    GLog::i("The scroll emits a brilliant flash of red light and the " + affectedName + " flees!");
                } else {
                    GLog::i("The scroll emits a brilliant flash of red light and the monsters flee!");
                }
                break;
            }
            case ScrollType::LULLABY: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_LULLABY).get());
                FX::burst(hero->pos, 0x6699ffFF, 12);
                int count = 0;
                std::string affectedName = "";
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                        m->affect<Sleep>();
                        count++;
                        affectedName = m->name;
                    }
                }
                if (count == 0) {
                    GLog::i("The scroll utters a soothing melody.");
                } else if (count == 1) {
                    GLog::i("The scroll utters a soothing melody and the " + affectedName + " falls asleep!");
                } else {
                    GLog::i("The scroll utters a soothing melody and the monsters fall asleep!");
                }
                break;
            }
            case ScrollType::PSIONIC_BLAST: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_BLAST).get());
                FX::burst(hero->pos, 0xffffffFF, 20);
                GLog::w("A burst of psychic energy explodes around you!");
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                        m->affect<Blindness>();
                        m->damage(Random::IntRange(1, std::max(1, m->HT * 2 / 3)), hero);
                    }
                }
                hero->affect<Blindness>();
                Dungeon::observe();
                break;
            }
            case ScrollType::MIRROR_IMAGE: {
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                FX::burst(hero->pos, 0x00ffffFF, 12);
                GLog::p("Illusory mirror images appear to aid you!");

                std::vector<int> respawnPoints;
                static const int NEIGHBOURS8[] = {
                    -1 - Level::WIDTH, 0 - Level::WIDTH, 1 - Level::WIDTH,
                    -1,                                  1,
                    -1 + Level::WIDTH, 0 + Level::WIDTH, 1 + Level::WIDTH
                };
                for (int d : NEIGHBOURS8) {
                    int p = hero->pos + d;
                    if (p >= 0 && p < Level::LENGTH && Dungeon::level && Dungeon::level->isPassable(p) && !Actor::findChar(p)) {
                        respawnPoints.push_back(p);
                    }
                }

                int nImages = 3;
                while (nImages > 0 && !respawnPoints.empty()) {
                    int idx = Random::Int(static_cast<int>(respawnPoints.size()));
                    int p = respawnPoints[idx];
                    respawnPoints.erase(respawnPoints.begin() + idx);

                    auto img = std::make_unique<MirrorImage>();
                    img->HP = img->HT = 1;
                    img->move(p);
                    img->state = Mob::State::HUNTING;
                    Actor::occupyCell(img.get());
                    Actor::add(img.get(), 1.0f);
                    FX::burst(p, 0x00ffffff, 8);
                    Dungeon::mobs.push_back(std::move(img));
                    nImages--;
                }
                break;
            }
            case ScrollType::ENCHANTMENT:
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                if (hero->belongings.equippedWeapon) {
                    hero->belongings.equippedWeapon->upgrade();
                    FX::burst(hero->pos, 0xff00ffFF, 16);
                    GLog::p("Your weapon glows with arcane enchantment!");
                }
                break;
        }

        quantity--;
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    }

    return false;
}

std::string Scroll::info() const {
    if (isKnown(type)) {
        return trueDesc(type);
    } else {
        return "This parchment is covered with indecipherable writing, and bears a title of rune " + runeName(type) + ". Who knows what it will do when read aloud?";
    }
}

// -----------------------------------------------------------------------------
// Food
// -----------------------------------------------------------------------------
Food::Food() {
    name = "Ration of Food";
    image = ItemSpriteSheet::RATION;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::vector<std::string> Food::actions(Hero* hero) {
    (void)hero;
    return { "EAT", "THROW", "DROP" };
}

bool Food::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "EAT") {
        HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
        if (hspr) hspr->operate(hero->pos);

        Hunger* hunger = hero->getBuff<Hunger>();
        if (hunger) {
            hunger->satisfy(Hunger::HUNGRY);
        }
        GLog::i("That food tasted delicious!");
        if (hero->heroClass == Hero::HeroClass::Warrior) {
            if (hero->HP < hero->HT) {
                hero->HP = std::min(hero->HT, hero->HP + 5);
                FX::floatingText(hero->pos, "+5", 0x38b764FF);
            }
        } else if (hero->heroClass == Hero::HeroClass::Mage) {
            for (auto& item : hero->belongings.backpack) {
                if (item) {
                    Wand* w = item->asWand();
                    if (w) {
                        w->restoreCharge(1);
                    }
                }
            }
            if (hero->belongings.equippedWeapon) {
                Wand* w = hero->belongings.equippedWeapon->asWand();
                if (w) {
                    w->restoreCharge(1);
                }
            }
            GLog::i("Your wands surge with re-energized magic!");
            FX::floatingText(hero->pos, "RECHARGED", 0x99ccffFF);
        }
        quantity--;
        hero->spendAndNext(3.0f * Actor::TICK);
        return true;
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Food::info() const {
    return "Nutritious traveling ration that satisfies hunger and restores vigor.";
}

std::unique_ptr<Item> Food::clone(int qty) const {
    auto f = std::make_unique<Food>();
    f->quantity = qty;
    return f;
}

// -----------------------------------------------------------------------------
// Dewdrop
// -----------------------------------------------------------------------------
Dewdrop::Dewdrop() {
    name = "Dewdrop";
    image = ItemSpriteSheet::DEWDROP;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::vector<std::string> Dewdrop::actions(Hero* hero) {
    (void)hero;
    return {};
}

bool Dewdrop::execute(Hero* hero, const std::string& action) {
    (void)hero;
    (void)action;
    return false;
}

std::string Dewdrop::info() const {
    return "A crystal clear dewdrop.";
}

std::unique_ptr<Item> Dewdrop::clone(int qty) const {
    auto d = std::make_unique<Dewdrop>();
    d->quantity = qty;
    return d;
}

// -----------------------------------------------------------------------------
// Gold
// -----------------------------------------------------------------------------
Gold::Gold(int val) {
    name = "Gold";
    image = ItemSpriteSheet::GOLD;
    quantity = val;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::vector<std::string> Gold::actions(Hero* hero) {
    (void)hero;
    return {};
}

bool Gold::execute(Hero* hero, const std::string& action) {
    (void)hero;
    (void)action;
    return false;
}

std::string Gold::info() const {
    return "A pile of " + std::to_string(quantity) + " gold coins. Collect gold coins to spend them later in a shop.";
}

std::unique_ptr<Item> Gold::clone(int qty) const {
    return std::make_unique<Gold>(qty);
}

// -----------------------------------------------------------------------------
// Torch
// -----------------------------------------------------------------------------
Torch::Torch() {
    name = "Torch";
    image = ItemSpriteSheet::TORCH;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::vector<std::string> Torch::actions(Hero* hero) {
    (void)hero;
    return { "LIGHT", "DROP", "THROW" };
}

bool Torch::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "LIGHT" || action == "USE") {
        hero->spendAndNext(Actor::TICK);
        hero->affect<Light>();
        GLog::p("You light a torch. The warm glow pushes back the darkness!");
        quantity--;
        if (quantity <= 0) {
            hero->belongings.backpack.erase(
                std::remove_if(hero->belongings.backpack.begin(), hero->belongings.backpack.end(),
                               [this](const std::unique_ptr<Item>& it) { return it.get() == this; }),
                hero->belongings.backpack.end());
        }
        return true;
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Torch::info() const {
    return "It's an indispensable item in dark dungeons and The Demon Halls, which are notorious for their poor ambient lighting.";
}

std::unique_ptr<Item> Torch::clone(int qty) const {
    auto t = std::make_unique<Torch>();
    t->quantity = qty;
    return t;
}

// -----------------------------------------------------------------------------
// Keys
// -----------------------------------------------------------------------------
Key::Key() : Key(Dungeon::depth) {}

Key::Key(int d) : depth(d) {
    name = "Key";
    image = ItemSpriteSheet::IRON_KEY;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::vector<std::string> Key::actions(Hero* hero) {
    (void)hero;
    return { "DROP", "THROW" };
}

bool Key::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "DROP") return doDropItem(hero, this);
    if (action == "THROW") return doThrowItem(hero, this);
    return false;
}

GoldenKey::GoldenKey() : GoldenKey(Dungeon::depth) {}

GoldenKey::GoldenKey(int d) : Key(d) {
    name = "Golden Key";
    image = ItemSpriteSheet::GOLDEN_KEY;
}

std::string GoldenKey::info() const {
    return "The notches on this golden key are tiny and intricate. Maybe it can open some chest lock?";
}

std::unique_ptr<Item> GoldenKey::clone(int qty) const {
    auto k = std::make_unique<GoldenKey>(depth);
    k->quantity = qty;
    return k;
}

IronKey::IronKey() : IronKey(Dungeon::depth) {}

IronKey::IronKey(int d) : Key(d) {
    name = "Iron Key";
    image = ItemSpriteSheet::IRON_KEY;
}

std::string IronKey::info() const {
    return "A simple iron key that can open a locked door.";
}

std::unique_ptr<Item> IronKey::clone(int qty) const {
    auto k = std::make_unique<IronKey>(depth);
    k->quantity = qty;
    return k;
}

// =============================================================================
// MissileWeapon Implementations
// =============================================================================
MissileWeapon::MissileWeapon(std::string n, int minD, int maxD, int strReq)
    : Weapon(std::move(n), minD, maxD, strReq) {
}

std::vector<std::string> MissileWeapon::actions(Hero* hero) {
    (void)hero;
    return { "THROW", "DROP" };
}

float MissileWeapon::acuracyFactor(Hero* hero) const {
    if (!hero) return 1.0f;
    int enc = STR - hero->STR;
    if (hero->heroClass == Hero::HeroClass::Warrior) {
        enc += 3;
    } else if (hero->heroClass == Hero::HeroClass::Huntress) {
        enc -= 2;
    }
    return enc > 0 ? static_cast<float>(1.0 / std::pow(1.5, enc)) : 1.0f;
}

float MissileWeapon::speedFactor(Hero* hero) const {
    int enc = STR - (hero ? hero->STR : 10);
    if (hero && hero->heroClass == Hero::HeroClass::Huntress) {
        enc -= 2;
    }
    return enc > 0 ? (1.0f * std::pow(1.2f, static_cast<float>(enc))) : 1.0f;
}

int MissileWeapon::maxDistance(const Hero* hero) const {
    bool isSniper = (hero && hero->subClass == HeroSubClass::Sniper);
    return isSniper ? 10 : 8;
}

void MissileWeapon::onThrow(int cell) {
    Char* enemy = Actor::findChar(cell);
    if (!enemy || enemy == Dungeon::hero.get()) {
        miss(cell);
    } else {
        if (!Dungeon::hero->shoot(enemy, this)) {
            miss(cell);
        } else {
            // Check breakage chance on hit (1 in 4 breaks, surviving missile drops to floor)
            if (Random::Int(4) == 0) {
                FX::burst(cell, 0x888888FF, 6);
                delete this;
            } else {
                miss(cell);
            }
        }
    }
}

void MissileWeapon::miss(int cell) {
    if (Dungeon::level) {
        std::unique_ptr<Item> self(this);
        Dungeon::level->drop(std::move(self), cell);
    }
}

// Dart
Dart::Dart(int number) : MissileWeapon("dart", 1, 4, 10) {
    image = ItemSpriteSheet::DART;
    quantity = number;
}

Item* Dart::random() {
    quantity = Random::IntRange(5, 15);
    return this;
}

std::string Dart::info() const {
    return "These simple metal spikes are weighted to fly true and sting their prey with a flick of the wrist.";
}

std::unique_ptr<Item> Dart::clone(int qty) const {
    return std::make_unique<Dart>(qty);
}

// Incendiary Dart
IncendiaryDart::IncendiaryDart(int number) : MissileWeapon("incendiary dart", 1, 2, 12) {
    image = ItemSpriteSheet::INCENDIARY_DART;
    quantity = number;
}

Item* IncendiaryDart::random() {
    quantity = Random::IntRange(3, 6);
    return this;
}

void IncendiaryDart::onThrow(int cell) {
    Char* enemy = Actor::findChar(cell);
    if (!enemy || enemy == Dungeon::hero.get()) {
        FX::seedBlob(cell, Blob::BlobType::FIRE, 4);
        miss(cell);
    } else {
        MissileWeapon::onThrow(cell);
    }
}

void IncendiaryDart::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    (void)damage;
    if (defender) {
        defender->affect<Burning>(4);
    }
}

std::string IncendiaryDart::info() const {
    return "The spike on each of these darts pins it to its target while volatile compounds burst into brilliant flames.";
}

std::unique_ptr<Item> IncendiaryDart::clone(int qty) const {
    return std::make_unique<IncendiaryDart>(qty);
}

// Curare Dart
CurareDart::CurareDart(int number) : MissileWeapon("curare dart", 1, 3, 14) {
    image = ItemSpriteSheet::CURARE_DART;
    quantity = number;
}

Item* CurareDart::random() {
    quantity = Random::IntRange(2, 5);
    return this;
}

void CurareDart::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    (void)damage;
    if (defender) {
        defender->affect<Paralysis>(3);
    }
}

std::string CurareDart::info() const {
    return "These little evil darts can paralyze the target leaving it helpless and motionless for some time.";
}

std::unique_ptr<Item> CurareDart::clone(int qty) const {
    return std::make_unique<CurareDart>(qty);
}

// Javelin
Javelin::Javelin(int number) : MissileWeapon("javelin", 2, 15, 15) {
    image = ItemSpriteSheet::JAVELIN;
    quantity = number;
}

Item* Javelin::random() {
    quantity = Random::IntRange(5, 15);
    return this;
}

void Javelin::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    (void)damage;
    if (defender) {
        defender->affect<Cripple>();
    }
}

int Javelin::maxDistance(const Hero* hero) const {
    bool isSniper = (hero && hero->subClass == HeroSubClass::Sniper);
    return isSniper ? 7 : 6;
}

std::string Javelin::info() const {
    return "This length of metal is weighted to keep the spike at its tip foremost as it sails through the air.";
}

std::unique_ptr<Item> Javelin::clone(int qty) const {
    return std::make_unique<Javelin>(qty);
}

// Shuriken
Shuriken::Shuriken(int number) : MissileWeapon("shuriken", 2, 6, 13) {
    image = ItemSpriteSheet::SHURIKEN;
    quantity = number;
}

Item* Shuriken::random() {
    quantity = Random::IntRange(5, 15);
    return this;
}

float Shuriken::speedFactor(Hero* hero) const {
    return 0.5f * MissileWeapon::speedFactor(hero);
}

std::string Shuriken::info() const {
    return "Star-shaped pieces of metal with razor-sharp blades. They can be thrown at very high rate.";
}

std::unique_ptr<Item> Shuriken::clone(int qty) const {
    return std::make_unique<Shuriken>(qty);
}

// Tamahawk
Tamahawk::Tamahawk(int number) : MissileWeapon("tomahawk", 4, 20, 17) {
    image = ItemSpriteSheet::TOMAHAWK;
    quantity = number;
}

Item* Tamahawk::random() {
    quantity = Random::IntRange(5, 12);
    return this;
}

void Tamahawk::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    if (defender) {
        defender->affect<Bleeding>(damage);
    }
}

int Tamahawk::maxDistance(const Hero* hero) const {
    bool isSniper = (hero && hero->subClass == HeroSubClass::Sniper);
    return isSniper ? 7 : 6;
}

std::string Tamahawk::info() const {
    return "This throwing axe is not that heavy, but it still requires significant strength to be used effectively.";
}

std::unique_ptr<Item> Tamahawk::clone(int qty) const {
    return std::make_unique<Tamahawk>(qty);
}

// Boomerang
Boomerang::Boomerang() : MissileWeapon("boomerang", 1, 4, 10) {
    image = ItemSpriteSheet::BOOMERANG;
    quantity = 1;
}

void Boomerang::onThrow(int cell) {
    Char* enemy = Actor::findChar(cell);
    if (!enemy || enemy == Dungeon::hero.get()) {
        circleBack(cell, Dungeon::hero.get());
    } else {
        if (!Dungeon::hero->shoot(enemy, this)) {
            circleBack(cell, Dungeon::hero.get());
        }
    }
}

void Boomerang::miss(int cell) {
    circleBack(cell, Dungeon::hero.get());
}

void Boomerang::proc(Char* attacker, Char* defender, int damage) {
    (void)damage;
    if (attacker && defender) {
        Hero* h = attacker == Dungeon::hero.get() ? Dungeon::hero.get() : nullptr;
        if (h && h->rangedWeapon == this) {
            circleBack(defender->pos, h);
        }
    }
}

int Boomerang::maxDistance(const Hero* hero) const {
    bool isSniper = (hero && hero->subClass == HeroSubClass::Sniper);
    return isSniper ? 11 : 9;
}

void Boomerang::circleBack(int fromCell, Hero* owner) {
    if (!owner) return;
    int heroCell = owner->pos;

    // Launch return missile sprite arc backwards to hero
    FX::missile(fromCell, heroCell, image, [this, owner, heroCell]() {
        std::unique_ptr<Item> self(this);
        if (!owner->belongings.collect(std::move(self))) {
            if (Dungeon::level) {
                Dungeon::level->drop(std::unique_ptr<Item>(this), heroCell);
            }
        }
    });
}

std::string Boomerang::info() const {
    return "Thrown to the enemy this flat curved wooden missile will return to the hands of its thrower.";
}

std::unique_ptr<Item> Boomerang::clone(int qty) const {
    auto b = std::make_unique<Boomerang>();
    b->quantity = qty;
    return b;
}

// =============================================================================
// PlantSeed Implementation
// =============================================================================
PlantSeed::PlantSeed(PlantType pt) : plantType(pt) {
    switch (pt) {
        case PlantType::FIREBLOOM:  name = "Seed of Firebloom";  image = ItemSpriteSheet::SEED_FIREBLOOM;  break;
        case PlantType::ICECAP:     name = "Seed of Icecap";     image = ItemSpriteSheet::SEED_ICECAP;     break;
        case PlantType::SORROWMOSS: name = "Seed of Sorrowmoss"; image = ItemSpriteSheet::SEED_SORROWMOSS; break;
        case PlantType::DREAMWEED:  name = "Seed of Dreamweed";  image = ItemSpriteSheet::SEED_DREAMWEED;  break;
        case PlantType::SUNGRASS:   name = "Seed of Sungrass";   image = ItemSpriteSheet::SEED_SUNGRASS;   break;
        case PlantType::EARTHROOT:  name = "Seed of Earthroot";  image = ItemSpriteSheet::SEED_EARTHROOT;  break;
        case PlantType::FADELEAF:   name = "Seed of Fadeleaf";   image = ItemSpriteSheet::SEED_FADELEAF;   break;
        case PlantType::ROTBERRY:   name = "Seed of Rotberry";   image = ItemSpriteSheet::SEED_ROTBERRY;   break;
    }
}

std::vector<std::string> PlantSeed::actions(Hero* hero) {
    (void)hero;
    return { "THROW", "PLANT", "DROP" };
}

bool PlantSeed::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "PLANT") {
        onThrow(hero->pos);
        hero->spendAndNext(Actor::TICK);
        return true;
    }
    return Item::execute(hero, action);
}

void PlantSeed::onThrow(int cell) {
    if (!Dungeon::level) return;

    // Planting logic:
    // If thrown into open chasm, drops into chasm
    if (Dungeon::level->map[cell] == Terrain::CHASM || Dungeon::level->pit[cell]) {
        std::unique_ptr<Item> self(this);
        Dungeon::dropToChasm(std::move(self));
        return;
    }

    // High grass / plantable ground
    if (Dungeon::visible[cell]) {
        FX::splash(cell, 0x44aa44FF, 6);
        GLog::i("The seed lands on the ground.");
    }

    std::unique_ptr<Item> self(this);
    Dungeon::level->drop(std::move(self), cell);
}

std::string PlantSeed::info() const {
    return "Throw this seed to the place where you want to plant it.";
}

std::string PlantSeed::displayName() const {
    std::string s = name;
    if (quantity > 1) s += " x" + std::to_string(quantity);
    return s;
}

std::unique_ptr<Item> PlantSeed::clone(int qty) const {
    auto p = std::make_unique<PlantSeed>(plantType);
    p->quantity = qty;
    return p;
}

// -----------------------------------------------------------------------------
// TomeOfMastery
// -----------------------------------------------------------------------------
TomeOfMastery::TomeOfMastery() {
    name = "Tome of Mastery";
    image = ItemSpriteSheet::MASTERY;
    identified = true;
    levelKnown = true;
}

std::string TomeOfMastery::displayName() const {
    if (Dungeon::hero && Dungeon::hero->subClass != HeroSubClass::None) {
        return "Tome of Remastery";
    }
    return "Tome of Mastery";
}

std::vector<std::string> TomeOfMastery::actions(Hero* hero) {
    (void)hero;
    return { "READ", "DROP" };
}

bool TomeOfMastery::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "READ") {
        if (hero->buff(BuffType::Paralysis)) {
            GLog::w("You can't read while paralyzed!");
            return false;
        }
        HeroSubClass sc1 = HeroSubClass::None;
        HeroSubClass sc2 = HeroSubClass::None;
        switch (hero->heroClass) {
            case Hero::HeroClass::Warrior:
                sc1 = HeroSubClass::Gladiator;
                sc2 = HeroSubClass::Berserker;
                break;
            case Hero::HeroClass::Mage:
                sc1 = HeroSubClass::Battlemage;
                sc2 = HeroSubClass::Warlock;
                break;
            case Hero::HeroClass::Rogue:
                sc1 = HeroSubClass::Assassin;
                sc2 = HeroSubClass::Freerunner;
                break;
            case Hero::HeroClass::Huntress:
                sc1 = HeroSubClass::Sniper;
                sc2 = HeroSubClass::Warden;
                break;
        }

        choose(hero, (hero->subClass == sc1) ? sc2 : sc1);
        return true;
    }
    return Item::execute(hero, action);
}

void TomeOfMastery::choose(Hero* hero, HeroSubClass way) {
    if (!hero) return;
    hero->subClass = way;
    hero->spendAndNext(10.0f * Actor::TICK);
    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_MASTERY).get());
    FX::burst(hero->pos, 0xffd700FF, 16);
    FX::floatingText(hero->pos, subClassTitle(way), 0xffd700FF);
    GLog::w("You have chosen the way of the " + std::string(subClassTitle(way)) + "!");

    if (way == HeroSubClass::Berserker && hero->HP <= hero->HT * 0.40f) {
        hero->affect<Fury>();
    }

    quantity = 0;
    hero->belongings.drop(this);
}

std::string TomeOfMastery::info() const {
    return "This worn leather book is not that thick, but you feel somehow that you can gather a lot from it. Remember though that reading this tome may require some time.";
}

std::unique_ptr<Item> TomeOfMastery::clone(int qty) const {
    auto t = std::make_unique<TomeOfMastery>();
    t->quantity = qty;
    return t;
}

} // namespace pd
