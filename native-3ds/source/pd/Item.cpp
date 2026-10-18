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
#include "pd/Badges.h"
#include "pd/Ballistica.h"
#include "pd/Plant.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"
#include "pd/ui/BottomScreenUI.h"
#include "pd/Statistics.h"
#include "pd/SaveManager.h"
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

void Item::notifyIdentified(Hero* hero) {
    if (!hero) hero = Dungeon::hero.get();
    Sample::play(Assets::SND_ITEM);
    if (hero) {
        FX::floatingText(hero->pos, displayName() + " is identified", 0xffd700FF);
    }
    GLog::i("You are now familiar enough with your " + displayName() + " to identify it.");
}

int Item::considerState(int basePrice) const {
    int p = basePrice;
    if (cursed && cursedKnown) {
        p /= 2;
    }
    if (levelKnown) {
        if (level > 0) {
            p *= (level + 1);
            if (isBroken()) {
                p /= 2;
            }
        } else if (level < 0) {
            p /= (1 - level);
        }
    }
    return p < 1 ? 1 : p;
}

bool Item::isEquipped(const Hero* hero) const {
    if (!hero) return false;
    return (this == hero->belongings.equippedWeapon ||
            this == hero->belongings.equippedArmor ||
            this == hero->belongings.equippedRing1 ||
            this == hero->belongings.equippedRing2);
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
        Sample::play(Assets::SND_ITEM);
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

    Sample::play(Assets::SND_MISS, 0.6f, 1.5f);

    float delay = Actor::TICK * speedFactor(hero);
    if (asMissile() && hero) {
        Char* enemy = Actor::findChar(cell);
        if (enemy) {
            SnipersMark* mark = hero->getBuff<SnipersMark>();
            if (mark) {
                if (mark->targetId == enemy->id) {
                    delay *= 0.5f;
                }
                mark->detach();
            }
        }
    }

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
            ui::BottomScreenUI::setLastTarget(ch);
            int dmg = Random::IntRange(1, 2);
            ch->damage(dmg, Dungeon::hero.get());
            FX::floatingText(cell, std::to_string(dmg), 0xe43b44FF);
            GLog::i("The " + displayName() + " strikes " + ch->name + " for " + std::to_string(dmg) + " damage!");
        }

        bool water = Dungeon::level->water[cell] || Dungeon::level->map[cell] == Terrain::WELL;
        if (water) {
            FX::ripple(cell);
            FX::waterSplash(cell, 3);
        }
        Sample::playAt(water ? Assets::SND_WATER : Assets::SND_STEP, cell, 0.8f, 1.2f);

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

bool Item::execute(Hero* hero) {
    return !defaultAction.empty() ? execute(hero, defaultAction) : false;
}

// -----------------------------------------------------------------------------
// Weapon
// -----------------------------------------------------------------------------
Weapon::Weapon(std::string n, int minD, int maxD, int strReq)
    : minDamage(minD), maxDamage(maxD), STR(strReq) {
    name = std::move(n);
    if (name == "Shortsword" || name == "short sword") image = ItemSpriteSheet::SHORT_SWORD;
    else if (name == "Longsword" || name == "longsword") image = ItemSpriteSheet::LONG_SWORD;
    else if (name == "Dagger" || name == "dagger") image = ItemSpriteSheet::DAGGER;
    else if (name == "Mace" || name == "mace") image = ItemSpriteSheet::MACE;
    else if (name == "Battle Axe" || name == "battle axe") image = ItemSpriteSheet::BATTLE_AXE;
    else if (name == "War Hammer" || name == "war hammer") image = ItemSpriteSheet::WAR_HAMMER;
    else if (name == "Sword" || name == "sword") image = ItemSpriteSheet::SWORD;
    else if (name == "Knuckles" || name == "Knuckleduster" || name == "knuckleduster") image = ItemSpriteSheet::KNUCKLEDUSTER;
    else if (name == "Quarterstaff" || name == "quarterstaff") image = ItemSpriteSheet::QUARTERSTAFF;
    else if (name == "Spear" || name == "spear") image = ItemSpriteSheet::SPEAR;
    else if (name == "Glaive" || name == "glaive") image = ItemSpriteSheet::GLAIVE;
    else {
        if (STR <= 10) image = ItemSpriteSheet::DAGGER;
        else if (STR <= 12) image = ItemSpriteSheet::SHORT_SWORD;
        else if (STR <= 14) image = ItemSpriteSheet::SWORD;
        else if (STR <= 16) image = ItemSpriteSheet::LONG_SWORD;
        else image = ItemSpriteSheet::WAR_HAMMER;
    }
}

int Weapon::min0() const {
    return tier > 0 ? tier : minDamage;
}

int Weapon::max0() const {
    if (tier > 0) {
        return static_cast<int>((tier * tier - tier + 10) / ACU * DLY);
    }
    return maxDamage;
}

int Weapon::min() const {
    return isBroken() ? min0() : min0() + level;
}

int Weapon::max() const {
    return isBroken() ? max0() : max0() + level * (tier > 0 ? tier : 1);
}

int Weapon::typicalSTR() const {
    if (tier > 0) {
        return 8 + tier * 2;
    }
    if (name == "Dagger" || name == "Knuckles" || name == "Knuckleduster" || name == "knuckleduster" || name == "dagger") return 10;
    if (name == "Shortsword" || name == "short sword" || name == "Spear" || name == "spear" || name == "Quarterstaff" || name == "quarterstaff") return 12;
    if (name == "Sword" || name == "sword" || name == "Mace" || name == "mace") return 14;
    if (name == "Longsword" || name == "longsword" || name == "Battle Axe" || name == "battle axe") return 16;
    if (name == "War Hammer" || name == "war hammer" || name == "Glaive" || name == "glaive") return 18;
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
    if (isEnchanted()) {
        switch (enchantment) {
            case Enchantment::BLAZING: s = "Blazing " + s; break;
            case Enchantment::CHILLING: s = "Chilling " + s; break;
            case Enchantment::VENOMOUS: s = "Venomous " + s; break;
            case Enchantment::PARALYTIC: s = "Paralytic " + s; break;
            case Enchantment::LEECH: s = "Vampiric " + s; break;
            case Enchantment::SHOCKING: s = "Shocking " + s; break;
            case Enchantment::UNSTABLE: s = "Unstable " + s; break;
            default: break;
        }
    }
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
        if (hero->belongings.equippedWeapon && hero->belongings.equippedWeapon != this && hero->belongings.equippedWeapon->isCursed()) {
            GLog::w("You can't remove cursed " + hero->belongings.equippedWeapon->displayName() + "!");
            return false;
        }
        if (!hero->belongings.equip(this)) {
            return false;
        }
        cursedKnown = true;
        if (cursed) {
            FX::burst(hero->pos, 0x550055FFu, 8);
            Sample::play(Assets::SND_CURSED);
            GLog::n("The " + displayName() + " is cursed! You cannot take it off!");
        }
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "UNEQUIP") {
        if (cursed && hero->belongings.equippedWeapon == this) {
            GLog::w("You can't remove cursed " + displayName() + "!");
            return false;
        }
        if (hero->belongings.isFull()) {
            GLog::w("Your backpack is too full to unequip that!");
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
        }
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        if (hero->belongings.equippedWeapon == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
        }
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Weapon::info() const {
    if (levelKnown) {
        return "Damage: " + std::to_string(min()) + "-" + std::to_string(max()) +
               " | STR required: " + std::to_string(std::max(1, STR));
    } else {
        return "Typical " + name + " deals " + std::to_string(min0()) + "-" + std::to_string(max0()) +
               " damage and requires " + std::to_string(typicalSTR()) + " points of strength.";
    }
}

float Weapon::acuracyFactor(Hero* hero) const {
    if (!hero) return ACU;
    int encumbrance = STR - hero->effectiveSTR();
    if (asMissile()) {
        if (hero->heroClass == Hero::HeroClass::Warrior) {
            encumbrance += 3;
        } else if (hero->heroClass == Hero::HeroClass::Huntress) {
            encumbrance -= 2;
        }
    }
    float factor = (encumbrance > 0) ? static_cast<float>(ACU / std::pow(1.5, encumbrance)) : ACU;
    if (imbue == Imbue::ACCURACY) factor *= 1.5f;
    return factor;
}

float Weapon::speedFactor(Hero* hero) const {
    if (!hero) return DLY;
    int encumbrance = STR - hero->effectiveSTR();
    if (asMissile() && hero->heroClass == Hero::HeroClass::Huntress) {
        encumbrance -= 2;
    }
    float factor = (encumbrance > 0) ? static_cast<float>(DLY * std::pow(1.2, encumbrance)) : DLY;
    if (imbue == Imbue::SPEED) factor *= 0.6f;
    return factor;
}

int Weapon::damageRoll() const {
    return Random::NormalIntRange(min(), max());
}

int Weapon::damageRoll(Hero* hero) const {
    int dmg = damageRoll();
    if (hero) {
        bool isRanged = (hero->rangedWeapon != nullptr);
        bool isHuntress = (hero->heroClass == Hero::HeroClass::Huntress);
        if (isRanged == isHuntress) {
            int exStr = hero->effectiveSTR() - STR;
            if (exStr > 0) {
                dmg += Random::IntRange(0, exStr);
            }
        }
    }
    return dmg;
}

std::unique_ptr<Weapon> Weapon::createByName(const std::string& rawName) {
    std::string n = rawName;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    if (n.find("dagger") != std::string::npos) return std::make_unique<Dagger>();
    if (n.find("knuckle") != std::string::npos) return std::make_unique<Knuckles>();
    if (n.find("short") != std::string::npos && n.find("sword") != std::string::npos) return std::make_unique<ShortSword>();
    if (n.find("quarterstaff") != std::string::npos) return std::make_unique<Quarterstaff>();
    if (n.find("spear") != std::string::npos) return std::make_unique<Spear>();
    if (n.find("longsword") != std::string::npos) return std::make_unique<Longsword>();
    if (n.find("sword") != std::string::npos) return std::make_unique<Sword>();
    if (n.find("mace") != std::string::npos) return std::make_unique<Mace>();
    if (n.find("battle") != std::string::npos || n.find("axe") != std::string::npos) return std::make_unique<BattleAxe>();
    if (n.find("hammer") != std::string::npos) return std::make_unique<WarHammer>();
    if (n.find("glaive") != std::string::npos) return std::make_unique<Glaive>();
    if (n.find("boomerang") != std::string::npos) return std::make_unique<Boomerang>();
    if (n.find("incendiary") != std::string::npos) return std::make_unique<IncendiaryDart>();
    if (n.find("curare") != std::string::npos) return std::make_unique<CurareDart>();
    if (n.find("dart") != std::string::npos) return std::make_unique<Dart>();
    if (n.find("javelin") != std::string::npos) return std::make_unique<Javelin>();
    if (n.find("shuriken") != std::string::npos) return std::make_unique<Shuriken>();
    if (n.find("tomahawk") != std::string::npos || n.find("tamahawk") != std::string::npos) return std::make_unique<Tamahawk>();
    return std::make_unique<Weapon>(rawName, 3, 8, 12);
}

void Weapon::upgrade(bool enchant) {
    if (!isUpgradable()) return;
    if (isEnchanted()) {
        if (!enchant && Random::Int(level) > 0) {
            GLog::w("wonderful energy of the scroll strips the enchantment from your weapon!");
            enchantment = Enchantment::NONE;
        }
    }
    if (!asWand() && !asBoomerang()) {
        if (STR > 1) STR--;
    }
    Item::upgrade();
}

void Weapon::degrade() {
    if (!isUpgradable()) return;
    if (!asWand() && !asBoomerang()) {
        STR++;
    }
    Item::degrade();
}

void Weapon::enchant(Enchantment ench) {
    if (ench == Enchantment::NONE) {
        int r = Random::Int(7);
        switch (r) {
            case 0: enchantment = Enchantment::BLAZING; break;
            case 1: enchantment = Enchantment::CHILLING; break;
            case 2: enchantment = Enchantment::VENOMOUS; break;
            case 3: enchantment = Enchantment::PARALYTIC; break;
            case 4: enchantment = Enchantment::LEECH; break;
            case 5: enchantment = Enchantment::SHOCKING; break;
            case 6: enchantment = Enchantment::UNSTABLE; break;
        }
    } else {
        enchantment = ench;
    }
}

void Weapon::proc(Char* attacker, Char* defender, int damage) {
    if (!levelKnown) {
        if (--hitsToKnow <= 0) {
            identify();
            Hero* h = attacker ? attacker->asHero() : nullptr;
            notifyIdentified(h);
        }
    }

    if (defender && defender->isAlive() && isEnchanted()) {
        Enchantment eff = enchantment;
        if (eff == Enchantment::UNSTABLE) {
            eff = static_cast<Enchantment>(1 + Random::Int(6));
        }

        switch (eff) {
            case Enchantment::BLAZING:
                defender->affect<Burning>();
                FX::burst(defender->pos, 0xff6600FFu, 6);
                Sample::playAt(Assets::SND_BURNING, defender->pos);
                break;
            case Enchantment::CHILLING:
                defender->affect<Frost>();
                FX::burst(defender->pos, 0x66ccffFFu, 6);
                Sample::playAt(Assets::SND_HIT, defender->pos);
                break;
            case Enchantment::VENOMOUS:
                defender->affect<Poison>();
                FX::burst(defender->pos, 0x00cc44FFu, 6);
                break;
            case Enchantment::PARALYTIC:
                if (Random::Int(4) == 0) {
                    defender->affect<Paralysis>();
                    FX::burst(defender->pos, 0xcccc00FFu, 6);
                }
                break;
            case Enchantment::LEECH:
                if (attacker && damage > 0) {
                    int heal = std::max(1, damage / 5);
                    attacker->HP = std::min(attacker->HT, attacker->HP + heal);
                    FX::floatingText(attacker->pos, "+" + std::to_string(heal), 0x38b764FF);
                }
                break;
            case Enchantment::SHOCKING:
                if (Dungeon::level) {
                    for (int n : Level::NEIGHBOURS8) {
                        int c = defender->pos + n;
                        Char* nearby = Actor::findChar(c);
                        if (nearby && nearby != attacker) {
                            nearby->damage(std::max(1, damage / 3), attacker);
                            FX::burst(c, 0xffff66FFu, 6);
                        }
                    }
                    Sample::playAt(Assets::SND_ZAP, defender->pos);
                }
                break;
            default:
                break;
        }
    }
}

// -----------------------------------------------------------------------------
// MeleeWeapon
// -----------------------------------------------------------------------------
MeleeWeapon::MeleeWeapon(std::string n, int t, float acu, float dly) {
    name = std::move(n);
    tier = t;
    ACU = acu;
    DLY = dly;
    STR = typicalSTR();
}

int MeleeWeapon::price() const {
    int basePrice = 20 * (1 << (tier - 1));
    if (isEnchanted()) {
        basePrice = static_cast<int>(basePrice * 1.5f);
    }
    return considerState(basePrice);
}

std::string MeleeWeapon::info() const {
    std::string s = desc();
    s += "\n\n";

    int lvl = visiblyUpgraded();
    std::string quality = (lvl != 0) ? ((lvl > 0) ? (isBroken() ? "broken" : "upgraded") : "degraded") : "";
    std::string indef = quality.empty() ? "a" : ((quality[0] == 'u' || quality[0] == 'a' || quality[0] == 'e' || quality[0] == 'i' || quality[0] == 'o') ? "an " + quality : "a " + quality);
    s += "This " + name + " is " + indef + " tier-" + std::to_string(tier) + " melee weapon. ";

    if (levelKnown) {
        int mn = min();
        int mx = max();
        s += "Its average damage is " + std::to_string(mn + (mx - mn) / 2) + " points per hit. ";
    } else {
        int mn = min0();
        int mx = max0();
        s += "Its typical average damage is " + std::to_string(mn + (mx - mn) / 2) + " points per hit and usually it requires " + std::to_string(typicalSTR()) + " points of strength. ";
        if (Dungeon::hero && typicalSTR() > Dungeon::hero->effectiveSTR()) {
            s += "Probably this weapon is too heavy for you. ";
        }
    }

    if (DLY != 1.0f) {
        s += "This is a rather " + std::string(DLY < 1.0f ? "fast" : "slow");
        if (ACU != 1.0f) {
            if ((ACU > 1.0f) == (DLY < 1.0f)) {
                s += " and ";
            } else {
                s += " but ";
            }
            s += (ACU > 1.0f ? "accurate" : "inaccurate");
        }
        s += " weapon. ";
    } else if (ACU != 1.0f) {
        s += "This is a rather " + std::string(ACU > 1.0f ? "accurate" : "inaccurate") + " weapon. ";
    }

    if (imbue == Imbue::SPEED) {
        s += "It was balanced to make it faster. ";
    } else if (imbue == Imbue::ACCURACY) {
        s += "It was balanced to make it more accurate. ";
    }

    if (isEnchanted()) {
        s += "It is enchanted. ";
    }

    if (levelKnown && Dungeon::hero) {
        if (STR > Dungeon::hero->effectiveSTR()) {
            s += "\n\nBecause of your inadequate strength the accuracy and speed of your attack with this " + name + " is decreased.";
        } else if (STR < Dungeon::hero->effectiveSTR()) {
            s += "\n\nBecause of your excess strength the damage of your attack with this " + name + " is increased.";
        }
    }

    if (Dungeon::hero && Dungeon::hero->belongings.equippedWeapon == this) {
        s += "\n\nYou hold the " + name + " at the ready" + (cursed ? ", and because it is cursed, you are powerless to let go." : ".");
    } else if (cursedKnown && cursed) {
        s += "\n\nYou can feel a malevolent magic lurking within " + name + ".";
    }

    return s;
}

// Dagger
Dagger::Dagger() : MeleeWeapon("dagger", 1, 1.2f, 1.0f) {
    image = ItemSpriteSheet::DAGGER;
}
std::string Dagger::desc() const {
    return "A simple iron dagger with a well worn wooden handle.";
}
std::unique_ptr<Item> Dagger::clone(int qty) const {
    auto d = std::make_unique<Dagger>();
    d->level = level;
    d->quantity = qty;
    d->identified = identified;
    d->levelKnown = levelKnown;
    d->cursed = cursed;
    d->cursedKnown = cursedKnown;
    d->enchantment = enchantment;
    d->imbue = imbue;
    return d;
}

// Knuckles
Knuckles::Knuckles() : MeleeWeapon("knuckleduster", 1, 1.0f, 0.5f) {
    image = ItemSpriteSheet::KNUCKLEDUSTER;
}
std::string Knuckles::desc() const {
    return "A piece of iron shaped to fit around the knuckles.";
}
std::unique_ptr<Item> Knuckles::clone(int qty) const {
    auto k = std::make_unique<Knuckles>();
    k->level = level;
    k->quantity = qty;
    k->identified = identified;
    k->levelKnown = levelKnown;
    k->cursed = cursed;
    k->cursedKnown = cursedKnown;
    k->enchantment = enchantment;
    k->imbue = imbue;
    return k;
}

// ShortSword
ShortSword::ShortSword() : MeleeWeapon("short sword", 1, 1.0f, 1.0f) {
    image = ItemSpriteSheet::SHORT_SWORD;
    STR = 11;
}

bool ShortSword::canReforge(const Hero* hero) const {
    if (!isWarriorStarter) return false;
    if (hero && hero->heroClass != Hero::HeroClass::Warrior) return false;
    if (level <= 0) return false;
    return true;
}

std::vector<std::string> ShortSword::actions(Hero* hero) {
    auto acts = Weapon::actions(hero);
    if (canReforge(hero)) {
        acts.push_back("REFORGE");
    }
    return acts;
}

bool ShortSword::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "REFORGE") {
        if (!isWarriorStarter || hero->heroClass != Hero::HeroClass::Warrior) {
            GLog::w("Only the Warrior's starting short sword can be reforged!");
            return false;
        }
        if (level <= 0) {
            GLog::w("The short sword must be upgraded before it can be reforged!");
            return false;
        }
        if (ui::BottomScreenUI::instance()) {
            ui::BottomScreenUI::instance()->selectItem(ui::WndBag::Mode::WEAPON, "Select a weapon to upgrade", [this, hero](Item* target) {
                if (!target) return;
                if (target->asBoomerang()) {
                    GLog::w("you can't upgrade a boomerang this way");
                    return;
                }
                if (target == this) {
                    GLog::w("You can't reforge a weapon onto itself!");
                    return;
                }
                Weapon* targetWeapon = target->asWeapon();
                if (targetWeapon && targetWeapon->isUpgradable() && !targetWeapon->asWand()) {
                    Sample::play(Assets::SND_EVOKE);
                    targetWeapon->upgrade(targetWeapon->isEnchanted());
                    targetWeapon->identify();
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "you reforged the short sword to upgrade your %s", targetWeapon->displayName().c_str());
                    GLog::w(buf);
                    FX::burst(hero->pos, 0xffd700FF, 12);
                    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                    hero->spendAndNext(2.0f * Actor::TICK);
                    this->quantity = 0;
                    hero->belongings.drop(this);
                    Actor::process();
                } else {
                    GLog::w("This item cannot be upgraded this way!");
                }
            });
            return false;
        }
        return false;
    }
    return Weapon::execute(hero, action);
}

std::string ShortSword::desc() const {
    return "It is indeed quite short, just a few inches longer, than a dagger.";
}

std::string ShortSword::info() const {
    std::string s = MeleeWeapon::info();
    if (isWarriorStarter && (!Dungeon::hero || Dungeon::hero->heroClass == Hero::HeroClass::Warrior)) {
        s += "\n\nWhen upgraded, it can be reforged to upgrade another melee weapon.";
    }
    return s;
}

std::unique_ptr<Item> ShortSword::clone(int qty) const {
    auto s = std::make_unique<ShortSword>();
    s->level = level;
    s->quantity = qty;
    s->identified = identified;
    s->levelKnown = levelKnown;
    s->cursed = cursed;
    s->cursedKnown = cursedKnown;
    s->enchantment = enchantment;
    s->imbue = imbue;
    s->isWarriorStarter = isWarriorStarter;
    return s;
}

// Quarterstaff
Quarterstaff::Quarterstaff() : MeleeWeapon("quarterstaff", 2, 1.0f, 1.0f) {
    image = ItemSpriteSheet::QUARTERSTAFF;
}
std::string Quarterstaff::desc() const {
    return "A staff of hardwood, its ends are shod with iron.";
}
std::unique_ptr<Item> Quarterstaff::clone(int qty) const {
    auto q = std::make_unique<Quarterstaff>();
    q->level = level;
    q->quantity = qty;
    q->identified = identified;
    q->levelKnown = levelKnown;
    q->cursed = cursed;
    q->cursedKnown = cursedKnown;
    q->enchantment = enchantment;
    q->imbue = imbue;
    return q;
}

// Spear
Spear::Spear() : MeleeWeapon("spear", 2, 1.0f, 1.5f) {
    image = ItemSpriteSheet::SPEAR;
}
std::string Spear::desc() const {
    return "A slender wooden rod tipped with sharpened iron.";
}
std::unique_ptr<Item> Spear::clone(int qty) const {
    auto s = std::make_unique<Spear>();
    s->level = level;
    s->quantity = qty;
    s->identified = identified;
    s->levelKnown = levelKnown;
    s->cursed = cursed;
    s->cursedKnown = cursedKnown;
    s->enchantment = enchantment;
    s->imbue = imbue;
    return s;
}

// Sword
Sword::Sword() : MeleeWeapon("sword", 3, 1.0f, 1.0f) {
    image = ItemSpriteSheet::SWORD;
}
std::string Sword::desc() const {
    return "The razor-sharp length of steel blade shines reassuringly.";
}
std::unique_ptr<Item> Sword::clone(int qty) const {
    auto s = std::make_unique<Sword>();
    s->level = level;
    s->quantity = qty;
    s->identified = identified;
    s->levelKnown = levelKnown;
    s->cursed = cursed;
    s->cursedKnown = cursedKnown;
    s->enchantment = enchantment;
    s->imbue = imbue;
    return s;
}

// Mace
Mace::Mace() : MeleeWeapon("mace", 3, 1.0f, 0.8f) {
    image = ItemSpriteSheet::MACE;
}
std::string Mace::desc() const {
    return "The iron head of this weapon inflicts substantial damage.";
}
std::unique_ptr<Item> Mace::clone(int qty) const {
    auto m = std::make_unique<Mace>();
    m->level = level;
    m->quantity = qty;
    m->identified = identified;
    m->levelKnown = levelKnown;
    m->cursed = cursed;
    m->cursedKnown = cursedKnown;
    m->enchantment = enchantment;
    m->imbue = imbue;
    return m;
}

// Longsword
Longsword::Longsword() : MeleeWeapon("longsword", 4, 1.0f, 1.0f) {
    image = ItemSpriteSheet::LONG_SWORD;
}
std::string Longsword::desc() const {
    return "This towering blade inflicts heavy damage by investing its heft into every cut.";
}
std::unique_ptr<Item> Longsword::clone(int qty) const {
    auto l = std::make_unique<Longsword>();
    l->level = level;
    l->quantity = qty;
    l->identified = identified;
    l->levelKnown = levelKnown;
    l->cursed = cursed;
    l->cursedKnown = cursedKnown;
    l->enchantment = enchantment;
    l->imbue = imbue;
    return l;
}

// BattleAxe
BattleAxe::BattleAxe() : MeleeWeapon("battle axe", 4, 1.2f, 1.0f) {
    image = ItemSpriteSheet::BATTLE_AXE;
}
std::string BattleAxe::desc() const {
    return "The enormous steel head of this battle axe puts considerable heft behind each stroke.";
}
std::unique_ptr<Item> BattleAxe::clone(int qty) const {
    auto b = std::make_unique<BattleAxe>();
    b->level = level;
    b->quantity = qty;
    b->identified = identified;
    b->levelKnown = levelKnown;
    b->cursed = cursed;
    b->cursedKnown = cursedKnown;
    b->enchantment = enchantment;
    b->imbue = imbue;
    return b;
}

// WarHammer
WarHammer::WarHammer() : MeleeWeapon("war hammer", 5, 1.2f, 1.0f) {
    image = ItemSpriteSheet::WAR_HAMMER;
}
std::string WarHammer::desc() const {
    return "Few creatures can withstand the crushing blow of this towering mass of lead and steel, but only the strongest of adventurers can use it effectively.";
}
std::unique_ptr<Item> WarHammer::clone(int qty) const {
    auto w = std::make_unique<WarHammer>();
    w->level = level;
    w->quantity = qty;
    w->identified = identified;
    w->levelKnown = levelKnown;
    w->cursed = cursed;
    w->cursedKnown = cursedKnown;
    w->enchantment = enchantment;
    w->imbue = imbue;
    return w;
}

// Glaive
Glaive::Glaive() : MeleeWeapon("glaive", 5, 1.0f, 1.5f) {
    image = ItemSpriteSheet::GLAIVE;
}
std::string Glaive::desc() const {
    return "A polearm consisting of a sword blade on the end of a pole.";
}
std::unique_ptr<Item> Glaive::clone(int qty) const {
    auto g = std::make_unique<Glaive>();
    g->level = level;
    g->quantity = qty;
    g->identified = identified;
    g->levelKnown = levelKnown;
    g->cursed = cursed;
    g->cursedKnown = cursedKnown;
    g->enchantment = enchantment;
    g->imbue = imbue;
    return g;
}

// -----------------------------------------------------------------------------
// Wand
// -----------------------------------------------------------------------------
static const std::string s_wandWoods[12] = {
    "holly", "yew", "ebony", "cherry", "teak", "rowan",
    "willow", "mahogany", "bamboo", "purpleheart", "oak", "birch"
};
static const int s_wandImages[12] = {
    ItemSpriteSheet::WAND_HOLLY,        // 48
    ItemSpriteSheet::WAND_YEW,          // 49
    ItemSpriteSheet::WAND_EBONY,        // 50
    ItemSpriteSheet::WAND_CHERRY,       // 51
    ItemSpriteSheet::WAND_TEAK,         // 52
    ItemSpriteSheet::WAND_ROWAN,        // 53
    ItemSpriteSheet::WAND_WILLOW,       // 54
    ItemSpriteSheet::WAND_MAHOGANY,     // 55
    ItemSpriteSheet::WAND_BAMBOO,       // 68
    ItemSpriteSheet::WAND_PURPLEHEART,  // 69
    ItemSpriteSheet::WAND_OAK,          // 70
    ItemSpriteSheet::WAND_BIRCH         // 71
};

static std::vector<int> s_wandWoodMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static std::vector<bool> s_wandKnown(14, false);

void Wand::initWoods() {
    s_wandWoodMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int i = 11; i > 0; --i) {
        int j = Random::Int(i + 1);
        std::swap(s_wandWoodMap[i], s_wandWoodMap[j]);
    }
    std::fill(s_wandKnown.begin(), s_wandKnown.end(), false);
    s_wandKnown[static_cast<size_t>(WandType::MagicMissile)] = true;
}

bool Wand::isKnown(WandType tp) {
    if (tp == WandType::MagicMissile) return true;
    size_t idx = static_cast<size_t>(tp);
    return idx < s_wandKnown.size() ? s_wandKnown[idx] : false;
}

void Wand::setKnown(WandType tp) {
    size_t idx = static_cast<size_t>(tp);
    if (idx >= s_wandKnown.size()) return;
    s_wandKnown[idx] = true;

    // 11 out of 12 deduction rule
    int knownCount = 0;
    int unknownIdx = -1;
    for (size_t i = 0; i < 12; ++i) {
        if (s_wandKnown[i]) {
            knownCount++;
        } else {
            unknownIdx = static_cast<int>(i);
        }
    }
    if (knownCount == 11 && unknownIdx >= 0) {
        s_wandKnown[unknownIdx] = true;
    }

    // Refresh perception in backpack and ground heaps
    if (Dungeon::hero) {
        for (auto& it : Dungeon::hero->belongings.backpack) {
            if (it && it->asWand()) {
                it->asWand()->updatePerception();
            }
        }
        if (Dungeon::hero->belongings.equippedWeapon && Dungeon::hero->belongings.equippedWeapon->asWand()) {
            Dungeon::hero->belongings.equippedWeapon->asWand()->updatePerception();
        }
    }
    if (Dungeon::level) {
        for (auto& heap : Dungeon::level->heaps) {
            if (heap) {
                for (auto& it : heap->items) {
                    if (it && it->asWand()) {
                        it->asWand()->updatePerception();
                    }
                }
            }
        }
    }
}

bool Wand::allKnown() {
    for (size_t i = 0; i < 12; ++i) {
        if (!s_wandKnown[i]) return false;
    }
    return true;
}

const std::vector<int>& Wand::getWoodMapping() {
    return s_wandWoodMap;
}

const std::vector<bool>& Wand::getKnownTable() {
    return s_wandKnown;
}

void Wand::restoreStatus(const std::vector<int>& mapping, const std::vector<bool>& known) {
    if (mapping.size() == 12) {
        s_wandWoodMap = mapping;
    }
    if (known.size() >= 12) {
        s_wandKnown = known;
    }
}

std::unique_ptr<Wand> Wand::createByType(WandType tp) {
    switch (tp) {
        case WandType::Teleportation:  return std::make_unique<WandOfTeleportation>();
        case WandType::Slowness:       return std::make_unique<WandOfSlowness>();
        case WandType::Firebolt:       return std::make_unique<WandOfFirebolt>();
        case WandType::Poison:         return std::make_unique<WandOfPoison>();
        case WandType::Regrowth:       return std::make_unique<WandOfRegrowth>();
        case WandType::Blink:          return std::make_unique<WandOfBlink>();
        case WandType::Lightning:      return std::make_unique<WandOfLightning>();
        case WandType::Amok:           return std::make_unique<WandOfAmok>();
        case WandType::Reach:          return std::make_unique<WandOfReach>();
        case WandType::Flock:          return std::make_unique<WandOfFlock>();
        case WandType::Disintegration: return std::make_unique<WandOfDisintegration>();
        case WandType::Avalanche:      return std::make_unique<WandOfAvalanche>();
        case WandType::Paralysis:      return std::make_unique<WandOfParalysis>();
        case WandType::MagicMissile:
        default:                       return std::make_unique<WandOfMagicMissile>();
    }
}

std::unique_ptr<Wand> Wand::createByName(const std::string& wandName) {
    if (wandName == "Wand of Magic Missile") return std::make_unique<WandOfMagicMissile>();
    if (wandName == "Wand of Lightning")     return std::make_unique<WandOfLightning>();
    if (wandName == "Wand of Firebolt")      return std::make_unique<WandOfFirebolt>();
    if (wandName == "Wand of Disintegration")return std::make_unique<WandOfDisintegration>();
    if (wandName == "Wand of Poison")        return std::make_unique<WandOfPoison>();
    if (wandName == "Wand of Slowness" || wandName == "Wand of Slow") return std::make_unique<WandOfSlowness>();
    if (wandName == "Wand of Paralysis")     return std::make_unique<WandOfParalysis>();
    if (wandName == "Wand of Flock")         return std::make_unique<WandOfFlock>();
    if (wandName == "Wand of Amok")          return std::make_unique<WandOfAmok>();
    if (wandName == "Wand of Teleportation") return std::make_unique<WandOfTeleportation>();
    if (wandName == "Wand of Blink")         return std::make_unique<WandOfBlink>();
    if (wandName == "Wand of Regrowth")      return std::make_unique<WandOfRegrowth>();
    if (wandName == "Wand of Reach")         return std::make_unique<WandOfReach>();
    if (wandName == "Wand of Avalanche")     return std::make_unique<WandOfAvalanche>();
    return nullptr;
}

std::unique_ptr<Wand> Wand::randomWand() {
    int r = Random::Int(12);
    auto w = createByType(static_cast<WandType>(r));
    w->random();
    return w;
}

Item* Wand::random() {
    if (Random::Float() < 0.5f) {
        upgrade();
        if (Random::Float() < 0.15f) {
            upgrade();
        }
    }
    return this;
}

Wand::Wand(std::string n, int initialC, WandType tp)
    : Weapon(std::move(n), 1, 6, 10), maxCharges(initialC), curCharges(initialC), wandType(tp) {
    defaultAction = "ZAP";
    updatePerception();
}

void Wand::updatePerception() {
    if (wandType == WandType::MagicMissile) {
        wood = "magic missile";
        woodImage = ItemSpriteSheet::WAND_MAGIC_MISSILE;
        image = woodImage;
        identified = true;
        levelKnown = true;
        curChargeKnown = true;
    } else {
        size_t idx = static_cast<size_t>(wandType);
        int wIdx = (idx < 12) ? s_wandWoodMap[idx] : 0;
        wood = s_wandWoods[wIdx];
        woodImage = s_wandImages[wIdx];
        image = woodImage;
        identified = isKnown(wandType) && levelKnown && curChargeKnown;
    }
}

bool Wand::isKnown() const {
    return isKnown(wandType);
}

void Wand::setKnown() {
    setKnown(wandType);
}

std::vector<std::string> Wand::actions(Hero* hero) {
    (void)hero;
    std::vector<std::string> acts;
    acts.push_back("ZAP");
    acts.push_back("THROW");
    acts.push_back("DROP");
    return acts;
}

bool Wand::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "ZAP") {
        int targetCell = hero->pos;
        float minDist = 999.0f;
        for (const auto& m : Dungeon::mobs) {
            if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                float dist = static_cast<float>(PathFinder::distanceChebyshev(hero->pos, m->pos));
                if (dist < minDist) {
                    minDist = dist;
                    targetCell = m->pos;
                }
            }
        }
        cast(hero, targetCell);
        return true;
    }
    if (action == "EQUIP" || action == "UNEQUIP") {
        return false;
    }
    return Weapon::execute(hero, action);
}

std::string Wand::status() const {
    if (levelKnown) {
        return (curChargeKnown ? std::to_string(curCharges) : "?") + "/" + std::to_string(maxCharges);
    }
    return "";
}

std::string Wand::displayName() const {
    std::string s = isKnown() ? name : (wood + " wand");
    if (levelKnown) {
        if (cursed && cursedKnown) s = "cursed " + s;
        if (level > 0) s += " +" + std::to_string(level);
        else if (level < 0) s += " " + std::to_string(level);
    } else if (cursed && cursedKnown) {
        s = "cursed " + s;
    }
    std::string st = status();
    if (!st.empty()) {
        s += " (" + st + ")";
    }
    return s;
}

std::string Wand::desc() const {
    return "A magical wand that channels arcane energy.";
}

std::string Wand::info() const {
    std::string s = isKnown() ? desc() : ("This thin " + wood + " wand is warm to the touch. Who knows what it will do when used?");
    if (Dungeon::hero && Dungeon::hero->heroClass == Hero::HeroClass::Mage) {
        s += "\n\n";
        if (levelKnown) {
            int mn = minDamage();
            int mx = maxDamage();
            int avg = mn + (mx - mn) / 2;
            s += "When this wand is used as a melee weapon, its average damage is " + std::to_string(avg) + " points per hit.";
        } else {
            s += "You can use this wand as a melee weapon.";
        }
    }
    return s;
}

int Wand::power() const {
    int p = level;
    if (Dungeon::hero) {
        if (Dungeon::hero->belongings.equippedRing1 && Dungeon::hero->belongings.equippedRing1->asRing()) {
            Ring* r = Dungeon::hero->belongings.equippedRing1->asRing();
            if (r->effect == "Power") p += r->level;
        }
        if (Dungeon::hero->belongings.equippedRing2 && Dungeon::hero->belongings.equippedRing2->asRing()) {
            Ring* r = Dungeon::hero->belongings.equippedRing2->asRing();
            if (r->effect == "Power") p += r->level;
        }
    }
    return std::max(0, p);
}

int Wand::minDamage() const {
    int tier = 1 + level / 3;
    return tier;
}

int Wand::maxDamage() const {
    int tier = 1 + level / 3;
    return (tier * tier - tier + 10) / 2 + level;
}

int Wand::damageRoll(Hero* hero) const {
    int mn = minDamage();
    int mx = maxDamage();
    int dmg = Random::IntRange(mn, mx);
    if (hero && hero->heroClass != Hero::HeroClass::Huntress) {
        int ex = hero->effectiveSTR() - 10;
        if (ex > 0) {
            dmg += Random::IntRange(0, ex);
        }
    }
    return dmg;
}

void Wand::updateCharge(float dt, Hero* hero) {
    if (curCharges >= maxCharges) {
        chargeTimer = 0.0f;
        return;
    }
    chargeTimer += dt;
    float timeToCharge = 40.0f;
    if (hero && hero->heroClass == Hero::HeroClass::Mage) {
        timeToCharge = 40.0f / std::sqrt(1.0f + static_cast<float>(std::max(0, level)));
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
    if (!isUpgradable()) return;
    Weapon::upgrade();
    maxCharges = std::min(initialCharges() + level, 9);
    curCharges = std::min(maxCharges, curCharges + 1);
}

void Wand::degrade() {
    Weapon::degrade();
    maxCharges = std::min(initialCharges() + level, 9);
    curCharges = std::min(maxCharges, curCharges);
}

Item* Wand::identify() {
    setKnown();
    curChargeKnown = true;
    Weapon::identify();
    return this;
}

void Wand::wandUsed(Hero* hero) {
    if (!hero) hero = Dungeon::hero.get();
    curCharges--;
    if (!isIdentified() && --usagesToKnow <= 0) {
        identify();
        notifyIdentified(hero);
    }
    if (hero) {
        hero->spendAndNext(Actor::TICK);
    }
}

int Wand::price() const {
    int p = 50;
    if (levelKnown) {
        if (level > 0) p *= (level + 1);
        else if (level < 0) p /= (1 - level);
    }
    return p;
}

void Wand::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x99ccffFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void Wand::onZap(int cell) {
    (void)cell;
}

void Wand::cast(Hero* hero, int targetCell) {
    if (!hero) return;

    if (targetCell == hero->pos) {
        GLog::i("You can't target yourself");
        return;
    }

    int cell = Ballistica::cast(hero->pos, targetCell, true, hitChars);
    Char* hitChar = Actor::findChar(cell);
    if (hitChar && hitChar != hero) {
        ui::BottomScreenUI::setLastTarget(hitChar);
    }
    if (hero->getSprite()) {
        hero->getSprite()->turnTo(hero->pos, cell);
        HeroSprite* hspr = hero->getSprite()->asHeroSprite();
        if (hspr) hspr->operate(cell);
    }

    if (curCharges > 0) {
        hero->removeBuff(BuffType::Invisibility);

        fx(cell, [this, hero, cell]() {
            onZap(cell);
            wandUsed(hero);
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
    } else {
        hero->spendAndNext(Actor::TICK);
        GLog::w("Your wand fizzles; it must be out of charges for now.");
        levelKnown = true;
    }
}

// -----------------------------------------------------------------------------
// WandOfMagicMissile
// -----------------------------------------------------------------------------
WandOfMagicMissile::WandOfMagicMissile()
    : Wand("Wand of Magic Missile", 3, WandType::MagicMissile) {
    image = ItemSpriteSheet::WAND_MAGIC_MISSILE;
    identified = true;
    levelKnown = true;
    curChargeKnown = true;
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
        if (ui::BottomScreenUI::instance()) {
            ui::BottomScreenUI::instance()->selectItem(ui::WndBag::Mode::WAND, "Select a wand to upgrade", [this, hero](Item* target) {
                if (!target) return;
                if (target == this) {
                    GLog::w("You can't disenchant a wand into itself!");
                    return;
                }
                Wand* targetWand = target->asWand();
                if (targetWand && targetWand->isUpgradable()) {
                    Sample::play(Assets::SND_EVOKE);
                    targetWand->upgrade();
                    targetWand->identify();
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "You disenchanted the Wand of Magic Missile to upgrade your %s!", targetWand->displayName().c_str());
                    GLog::w(buf);
                    FX::burst(hero->pos, 0x99ccffFF, 12);
                    FX::floatingText(hero->pos, "+1 LVL", 0x99ccffFF);
                    hero->spendAndNext(2.0f * Actor::TICK);
                    this->quantity = 0;
                    hero->belongings.drop(this);
                    Actor::process();
                } else {
                    GLog::w("This wand cannot be upgraded!");
                }
            });
            return false;
        }
        return false;
    }
    return Wand::execute(hero, action);
}

std::string WandOfMagicMissile::desc() const {
    return "This wand launches missiles of pure magical energy, dealing moderate damage to a target creature.";
}

void WandOfMagicMissile::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x99ccffFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfMagicMissile::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch) {
        int lvl = power();
        int dmg = Random::IntRange(1, 6 + lvl * 2);
        ch->damage(dmg, Dungeon::hero.get());
        FX::burst(cell, 0x99ccffFF, lvl / 2 + 2);
        FX::floatingText(cell, std::to_string(dmg), 0x99ccffFF);
        GLog::i("The magic missile strikes " + ch->name + " for " + std::to_string(dmg) + " damage!");
        if (ch == Dungeon::hero.get() && !ch->isAlive()) {
            GLog::n("You killed yourself with your own Wand of Magic Missile...");
        }
    } else {
        FX::burst(cell, 0x99ccffFF, 6);
    }
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
// WandOfLightning
// -----------------------------------------------------------------------------
WandOfLightning::WandOfLightning()
    : Wand("Wand of Lightning", 2, WandType::Lightning) {}

std::string WandOfLightning::desc() const {
    return "This wand conjures forth deadly arcs of electricity, which deal damage to several creatures standing close to each other.";
}

void WandOfLightning::hit(Char* ch, int damage, std::vector<Char*>& affected, std::vector<int>& points) {
    if (damage < 1 || !ch) return;

    if (ch == Dungeon::hero.get() && noosa::Camera::main) {
        noosa::Camera::main->shake(2.0f, 0.3f);
    }

    affected.push_back(ch);
    points.push_back(ch->pos);

    bool onWater = (Dungeon::level && Dungeon::level->water[ch->pos] && !ch->flying);
    int finalDmg = onWater ? (damage * 2) : damage;
    ch->damage(finalDmg, Dungeon::hero.get());

    FX::burst(ch->pos, 0xffff66FF, 3);

    std::vector<Char*> ns;
    for (int off : Level::NEIGHBOURS8) {
        int n = ch->pos + off;
        if (n >= 0 && n < Level::LENGTH) {
            Char* neighbor = Actor::findChar(n);
            if (neighbor && std::find(affected.begin(), affected.end(), neighbor) == affected.end()) {
                ns.push_back(neighbor);
            }
        }
    }

    if (!ns.empty()) {
        int r = Random::Int(static_cast<int>(ns.size()));
        hit(ns[r], Random::IntRange(damage / 2, damage), affected, points);
    }
}

void WandOfLightning::fx(int cell, std::function<void()> callback) {
    std::vector<int> points;
    points.push_back(Dungeon::hero ? Dungeon::hero->pos : 0);

    Char* ch = Actor::findChar(cell);
    if (ch) {
        std::vector<Char*> affected;
        int lvl = power();
        hit(ch, Random::IntRange(5 + lvl / 2, 10 + lvl), affected, points);
    } else {
        points.push_back(cell);
        FX::burst(cell, 0xffff66FF, 3);
    }

    FX::lightning(points, callback);
}

void WandOfLightning::onZap(int cell) {
    (void)cell;
    if (Dungeon::hero && !Dungeon::hero->isAlive()) {
        GLog::n("You killed yourself with your own Wand of Lightning...");
    }
}

std::unique_ptr<Item> WandOfLightning::clone(int qty) const {
    auto w = std::make_unique<WandOfLightning>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfFirebolt
// -----------------------------------------------------------------------------
WandOfFirebolt::WandOfFirebolt()
    : Wand("Wand of Firebolt", 2, WandType::Firebolt) {}

std::string WandOfFirebolt::desc() const {
    return "This wand unleashes bursts of magical fire. It will ignite flammable terrain, and will damage and burn a creature it hits.";
}

void WandOfFirebolt::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0xff6600FF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfFirebolt::onZap(int cell) {
    int lvl = power();

    for (int i = 1; i < Ballistica::distance - 1; ++i) {
        int c = Ballistica::trace[i];
        if (Dungeon::level && Dungeon::level->isFlamable(c)) {
            FX::seedBlob(c, Blob::BlobType::FIRE, 1);
        }
    }

    FX::seedBlob(cell, Blob::BlobType::FIRE, 1);

    Char* ch = Actor::findChar(cell);
    if (ch) {
        ch->damage(Random::IntRange(1, 8 + lvl * lvl), Dungeon::hero.get());
        ch->affect<Burning>();
        FX::burst(cell, 0xff6600FF, 5);

        if (ch == Dungeon::hero.get() && !ch->isAlive()) {
            GLog::n("You killed yourself with your own Wand of Firebolt...");
        }
    }
}

std::unique_ptr<Item> WandOfFirebolt::clone(int qty) const {
    auto w = std::make_unique<WandOfFirebolt>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfDisintegration
// -----------------------------------------------------------------------------
WandOfDisintegration::WandOfDisintegration()
    : Wand("Wand of Disintegration", 2, WandType::Disintegration) {
    hitChars = false;
}

std::string WandOfDisintegration::desc() const {
    return "This wand emits a beam of destructive energy, which pierces all creatures in its way. The more targets it hits, the more damage it inflicts to each of them.";
}

void WandOfDisintegration::fx(int cell, std::function<void()> callback) {
    (void)cell;
    int dist = level + 4;
    int endIdx = std::min(Ballistica::distance, dist) - 1;
    if (endIdx < 0) endIdx = 0;
    int target = Ballistica::trace[endIdx];
    FX::deathRay(Dungeon::hero ? Dungeon::hero->pos : 0, target, callback);
}

void WandOfDisintegration::onZap(int cell) {
    (void)cell;
    int lvl = power();
    int maxDist = level + 4;
    int rayDist = std::min(Ballistica::distance, maxDist);

    bool terrainAffected = false;
    std::vector<Char*> chars;

    for (int i = 1; i < rayDist; ++i) {
        int c = Ballistica::trace[i];
        Char* ch = Actor::findChar(c);
        if (ch) {
            chars.push_back(ch);
        }

        if (Dungeon::level) {
            int terr = Dungeon::level->map[c];
            if (terr == Terrain::DOOR || terr == Terrain::SIGN) {
                Dungeon::level->destroy(c);
                terrainAffected = true;
            } else if (terr == Terrain::HIGH_GRASS) {
                Dungeon::level->set(c, Terrain::GRASS);
                terrainAffected = true;
            }
        }

        FX::burst(c, 0xaa00aaFF, Random::IntRange(1, 2));
    }

    if (terrainAffected) {
        Dungeon::observe();
    }

    int effLvl = lvl + static_cast<int>(chars.size());
    int dmgMin = effLvl;
    int dmgMax = 8 + (effLvl * effLvl) / 3;

    for (Char* ch : chars) {
        if (ch && ch->isAlive()) {
            ch->damage(Random::NormalIntRange(dmgMin, dmgMax), Dungeon::hero.get());
            FX::burst(ch->pos, 0xaa00aaFF, 3);
        }
    }
}

std::unique_ptr<Item> WandOfDisintegration::clone(int qty) const {
    auto w = std::make_unique<WandOfDisintegration>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfPoison
// -----------------------------------------------------------------------------
WandOfPoison::WandOfPoison()
    : Wand("Wand of Poison", 2, WandType::Poison) {}

std::string WandOfPoison::desc() const {
    return "The vile blast of this twisted bit of wood will imbue its target with a deadly venom. A creature that is poisoned will suffer periodic damage until the effect ends. The duration of the effect increases with the level of the staff.";
}

void WandOfPoison::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x00cc44FF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfPoison::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch) {
        ch->affect<Poison>();
        FX::burst(cell, 0x00cc44FF, 6);
    } else {
        GLog::i("Nothing happened.");
    }
}

std::unique_ptr<Item> WandOfPoison::clone(int qty) const {
    auto w = std::make_unique<WandOfPoison>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfSlowness
// -----------------------------------------------------------------------------
WandOfSlowness::WandOfSlowness()
    : Wand("Wand of Slowness", 2, WandType::Slowness) {}

std::string WandOfSlowness::desc() const {
    return "This wand will cause a creature to move and attack at half its ordinary speed until the effect ends.";
}

void WandOfSlowness::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x8844ffFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfSlowness::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch) {
        ch->affect<Slow>();
        FX::burst(cell, 0x8844ffFF, 6);
    } else {
        GLog::i("Nothing happened.");
    }
}

std::unique_ptr<Item> WandOfSlowness::clone(int qty) const {
    auto w = std::make_unique<WandOfSlowness>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfParalysis
// -----------------------------------------------------------------------------
WandOfParalysis::WandOfParalysis()
    : Wand("Wand of Paralysis", 2, WandType::Paralysis) {}

std::string WandOfParalysis::desc() const {
    return "A burst of paralyzing magic that will freeze the target in place, rendering them completely helpless for several turns.";
}

void WandOfParalysis::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0xcccc00FF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfParalysis::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch) {
        ch->affect<Paralysis>();
        FX::burst(cell, 0xcccc00FF, 6);
    }
    FX::seedBlob(cell, Blob::BlobType::PARALYTIC_GAS, 100);
}

std::unique_ptr<Item> WandOfParalysis::clone(int qty) const {
    auto w = std::make_unique<WandOfParalysis>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfFlock
// -----------------------------------------------------------------------------
WandOfFlock::WandOfFlock()
    : Wand("Wand of Flock", 2, WandType::Flock) {}

std::string WandOfFlock::desc() const {
    return "This wand summons a magical flock of woolly sheep that block passage and absorb attacks before vanishing in a puff of wool.";
}

void WandOfFlock::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0xeeeeeeFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfFlock::onZap(int cell) {
    int lvl = power();
    int n = lvl + 2;

    if (Actor::findChar(cell) && Ballistica::distance > 2) {
        cell = Ballistica::trace[Ballistica::distance - 2];
    }

    bool passable[Level::LENGTH];
    for (int i = 0; i < Level::LENGTH; ++i) {
        passable[i] = Dungeon::level ? (Dungeon::level->passable[i] || Dungeon::level->avoid[i]) : true;
    }
    for (int pos = 0; pos < Level::LENGTH; ++pos) {
        if (Actor::findChar(pos)) {
            passable[pos] = false;
        }
    }

    PathFinder::buildDistanceMap(cell, passable, n);
    int dist = 0;
    if (Actor::findChar(cell)) {
        PathFinder::distance[cell] = 999999;
        dist = 1;
    }

    float lifespan = static_cast<float>(lvl + 3);

    for (int i = 0; i < n; ++i) {
        bool placed = false;
        while (dist < n && !placed) {
            for (int j = 0; j < Level::LENGTH; ++j) {
                if (PathFinder::distance[j] == dist) {
                    auto sheep = std::make_unique<Sheep>();
                    sheep->lifespan = lifespan;
                    sheep->pos = j;
                    if (Dungeon::level) Dungeon::level->mobPress(sheep.get());
                    Dungeon::mobs.push_back(std::move(sheep));
                    FX::burst(j, 0xeeeeeeFF, 4);
                    PathFinder::distance[j] = 999999;
                    placed = true;
                    break;
                }
            }
            if (!placed) dist++;
        }
    }
}

std::unique_ptr<Item> WandOfFlock::clone(int qty) const {
    auto w = std::make_unique<WandOfFlock>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfAmok
// -----------------------------------------------------------------------------
WandOfAmok::WandOfAmok()
    : Wand("Wand of Amok", 2, WandType::Amok) {}

std::string WandOfAmok::desc() const {
    return "The purple light from this wand will make the target run amok attacking random creatures in its vicinity.";
}

void WandOfAmok::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x9933ccFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfAmok::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch) {
        if (ch == Dungeon::hero.get()) {
            ch->affect<Vertigo>();
        } else {
            ch->affect<Amok>();
        }
        FX::burst(cell, 0x9933ccFF, 6);
    } else {
        GLog::i("Nothing happened.");
    }
}

std::unique_ptr<Item> WandOfAmok::clone(int qty) const {
    auto w = std::make_unique<WandOfAmok>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfTeleportation
// -----------------------------------------------------------------------------
WandOfTeleportation::WandOfTeleportation()
    : Wand("Wand of Teleportation", 2, WandType::Teleportation) {}

std::string WandOfTeleportation::desc() const {
    return "A blast from this wand will teleport a creature against its will to a random place on the current level.";
}

void WandOfTeleportation::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x33ccffFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfTeleportation::onZap(int cell) {
    Char* ch = Actor::findChar(cell);
    if (ch == Dungeon::hero.get()) {
        setKnown();
        if (Dungeon::level) {
            for (int tries = 0; tries < 100; ++tries) {
                int dest = Random::Int(Level::LENGTH);
                if (Dungeon::level->isPassable(dest) && !Actor::findChar(dest)) {
                    FX::burst(Dungeon::hero->pos, 0x9955ffFF, 8);
                    Dungeon::hero->place(dest);
                    FX::burst(Dungeon::hero->pos, 0x9955ffFF, 8);
                    Dungeon::observe();
                    GLog::p("You feel a sudden shift in space!");
                    Sample::play(Assets::SND_TELEPORT);
                    break;
                }
            }
        }
    } else if (ch) {
        int pos = -1;
        if (Dungeon::level) {
            for (int tries = 0; tries < 100; ++tries) {
                int dest = Random::Int(Level::LENGTH);
                if (Dungeon::level->isPassable(dest) && !Actor::findChar(dest)) {
                    pos = dest;
                    break;
                }
            }
        }
        if (pos != -1) {
            ch->pos = pos;
            if (ch->getSprite()) {
                ch->getSprite()->place(pos);
            }
            GLog::i("Teleported " + ch->name + " to somewhere!");
            Sample::play(Assets::SND_TELEPORT);
            FX::burst(pos, 0x33ccffFF, 8);
        } else {
            GLog::w("Nothing happened.");
        }
    } else {
        GLog::i("Nothing happened.");
    }
}

std::unique_ptr<Item> WandOfTeleportation::clone(int qty) const {
    auto w = std::make_unique<WandOfTeleportation>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfBlink
// -----------------------------------------------------------------------------
WandOfBlink::WandOfBlink()
    : Wand("Wand of Blink", 2, WandType::Blink) {}

std::string WandOfBlink::desc() const {
    return "This wand will allow you to teleport in the chosen direction. Creatures and inanimate obstructions will block the teleportation.";
}

void WandOfBlink::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0xffffffFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfBlink::onZap(int cell) {
    int lvl = power();
    if (Ballistica::distance > lvl + 4) {
        cell = Ballistica::trace[lvl + 3];
    } else if (Actor::findChar(cell) && Ballistica::distance > 1) {
        cell = Ballistica::trace[Ballistica::distance - 2];
    }
    if (Dungeon::hero) {
        Dungeon::hero->move(cell);
        if (Dungeon::hero->getSprite()) Dungeon::hero->getSprite()->place(cell);
        Sample::play(Assets::SND_TELEPORT);
        FX::burst(cell, 0xffffffFF, 8);
        Dungeon::observe();
    }
}

std::unique_ptr<Item> WandOfBlink::clone(int qty) const {
    auto w = std::make_unique<WandOfBlink>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfRegrowth
// -----------------------------------------------------------------------------
WandOfRegrowth::WandOfRegrowth()
    : Wand("Wand of Regrowth", 2, WandType::Regrowth) {}

std::string WandOfRegrowth::desc() const {
    return "\"When life ceases new life always begins to grow... The eternal cycle always remains!\"";
}

void WandOfRegrowth::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x44aa22FF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfRegrowth::onZap(int cell) {
    for (int i = 1; i < Ballistica::distance - 1; ++i) {
        int p = Ballistica::trace[i];
        if (Dungeon::level) {
            int c = Dungeon::level->map[p];
            if (c == Terrain::EMPTY || c == Terrain::EMBERS || c == Terrain::EMPTY_DECO) {
                Dungeon::level->set(p, Terrain::GRASS);
            }
        }
    }
    if (Dungeon::level) {
        int c = Dungeon::level->map[cell];
        if (c == Terrain::EMPTY || c == Terrain::EMBERS || c == Terrain::EMPTY_DECO ||
            c == Terrain::GRASS || c == Terrain::HIGH_GRASS) {
            FX::seedBlob(cell, Blob::BlobType::REGROWTH, (power() + 2) * 20);
        } else {
            GLog::i("Nothing happened.");
        }
    }
}

std::unique_ptr<Item> WandOfRegrowth::clone(int qty) const {
    auto w = std::make_unique<WandOfRegrowth>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfReach
// -----------------------------------------------------------------------------
WandOfReach::WandOfReach()
    : Wand("Wand of Reach", 2, WandType::Reach) {
    hitChars = false;
}

std::string WandOfReach::desc() const {
    return "This utility wand can be used to grab objects from a distance and to switch places with enemies. Waves of magic force radiated from it will affect all cells on their way triggering traps, trampling high vegetation, opening closed doors and closing open ones.";
}

void WandOfReach::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0xccccccFF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfReach::onZap(int cell) {
    (void)cell;
    int reach = std::min(Ballistica::distance, power() + 4);
    bool mapUpdated = false;

    for (int i = 1; i < reach; ++i) {
        int c = Ballistica::trace[i];

        Char* ch = Actor::findChar(c);
        if (ch && ch != Dungeon::hero.get()) {
            int tmp = Dungeon::hero->pos;
            Dungeon::hero->pos = ch->pos;
            ch->pos = tmp;
            if (Dungeon::hero->getSprite()) Dungeon::hero->getSprite()->place(Dungeon::hero->pos);
            if (ch->getSprite()) ch->getSprite()->place(ch->pos);
            Sample::play(Assets::SND_TELEPORT);
            FX::burst(Dungeon::hero->pos, 0xccccccFF, 5);
            FX::burst(ch->pos, 0xccccccFF, 5);
            break;
        }

        Heap* heap = Dungeon::level ? Dungeon::level->getHeap(c) : nullptr;
        if (heap) {
            if (heap->type == Heap::Type::HEAP) {
                auto it = heap->pickUp();
                if (it) {
                    std::string itName = it->displayName();
                    if (it->name == "Dewdrop") {
                        DewVial* vial = nullptr;
                        for (auto& bIt : Dungeon::hero->belongings.backpack) {
                            if (bIt && bIt->asDewVial()) { vial = bIt->asDewVial(); break; }
                        }
                        if (vial && !vial->isFull()) {
                            vial->collectDew(it->quantity);
                            Sample::play(Assets::SND_DEWDROP);
                        } else {
                            int value = 1 + (Dungeon::depth - 1) / 5;
                            if (Dungeon::hero->heroClass == Hero::HeroClass::Huntress) value++;
                            int effect = std::min(Dungeon::hero->HT - Dungeon::hero->HP, value * it->quantity);
                            if (effect > 0) {
                                Dungeon::hero->HP += effect;
                                FX::floatingText(Dungeon::hero->pos, "+" + std::to_string(effect), 0x38b764FF);
                                FX::burst(Dungeon::hero->pos, 0x38b764FF, 4);
                            }
                            Sample::play(Assets::SND_DEWDROP);
                        }
                    } else if (it->name == "Gold") {
                        int g = it->quantity;
                        Dungeon::hero->gold += g;
                        Dungeon::gold += g;
                        Statistics::goldCollected += g;
                        FX::floatingText(Dungeon::hero->pos, "+" + std::to_string(g), 0xffd700FF);
                        GLog::p("You have magically transported " + std::to_string(g) + " gold into your backpack");
                        Sample::play(Assets::SND_GOLD);
                    } else if (Dungeon::hero->belongings.collect(it)) {
                        GLog::p("You have magically transported " + itName + " into your backpack");
                        Sample::play(Assets::SND_ITEM);
                    } else {
                        GLog::w("Your backpack is too full to transport " + itName);
                        if (Dungeon::level) Dungeon::level->drop(std::move(it), Dungeon::hero->pos);
                    }
                    if (heap->isEmpty() && Dungeon::level) {
                        Dungeon::level->removeHeap(heap);
                    }
                }
            } else {
                heap->open(Dungeon::hero.get());
            }
            break;
        }

        if (Dungeon::level) {
            int terr = Dungeon::level->map[c];
            Dungeon::level->press(c, nullptr);
            if (terr == Terrain::OPEN_DOOR) {
                Dungeon::level->set(c, Terrain::DOOR);
                mapUpdated = true;
            } else if (Dungeon::level->water[c]) {
                FX::ripple(c);
            }
        }
    }

    if (mapUpdated) {
        Dungeon::observe();
    }
}

std::unique_ptr<Item> WandOfReach::clone(int qty) const {
    auto w = std::make_unique<WandOfReach>();
    w->level = level;
    w->curCharges = curCharges;
    w->maxCharges = maxCharges;
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// WandOfAvalanche
// -----------------------------------------------------------------------------
WandOfAvalanche::WandOfAvalanche()
    : Wand("Wand of Avalanche", 2, WandType::Avalanche) {
    hitChars = false;
}

std::string WandOfAvalanche::desc() const {
    return "When a discharge of this wand hits a wall (or any other solid obstacle) it causes an avalanche of stones, damaging and stunning all creatures in the affected area.";
}

void WandOfAvalanche::fx(int cell, std::function<void()> callback) {
    FX::magicMissile(Dungeon::hero ? Dungeon::hero->pos : 0, cell, 0x886633FF, callback);
    Sample::play(Assets::SND_ZAP);
}

void WandOfAvalanche::onZap(int cell) {
    Sample::play(Assets::SND_ROCKS);
    int lvl = power();
    int size = 1 + lvl / 3;

    bool nonSolid[Level::LENGTH];
    for (int i = 0; i < Level::LENGTH; ++i) {
        nonSolid[i] = Dungeon::level ? !Terrain::isSolid(Dungeon::level->map[i]) : true;
    }

    PathFinder::buildDistanceMap(cell, nonSolid, size);

    int shake = 0;
    for (int i = 0; i < Level::LENGTH; ++i) {
        int d = PathFinder::distance[i];
        if (d <= size) {
            Char* ch = Actor::findChar(i);
            if (ch) {
                ch->damage(Random::IntRange(2, 6 + (size - d) * 2), Dungeon::hero.get());
                if (ch->isAlive() && Random::Int(2 + d) == 0) {
                    ch->affect<Paralysis>();
                }
            }

            if (Dungeon::level) {
                Dungeon::level->press(i, ch);
            }

            if (Dungeon::visible[i]) {
                FX::burst(i, 0x886633FF, 3 + (size - d));
                if (Dungeon::level && Dungeon::level->water[i]) {
                    FX::ripple(i);
                }
                if (shake < size - d) {
                    shake = size - d;
                }
            }
        }
    }

    if (noosa::Camera::main) {
        noosa::Camera::main->shake(3.0f, 0.07f * (3 + shake));
    }

    if (Dungeon::hero && !Dungeon::hero->isAlive()) {
        GLog::n("You killed yourself with your own Wand of Avalanche...");
    }
}

std::unique_ptr<Item> WandOfAvalanche::clone(int qty) const {
    auto w = std::make_unique<WandOfAvalanche>();
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

int Armor::price() const {
    int basePrice = 10 * (1 << (tier - 1));
    if (isInscribed()) {
        basePrice = static_cast<int>(basePrice * 1.5f);
    }
    return considerState(basePrice);
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

void Armor::upgrade(bool inscribe) {
    if (!isUpgradable()) return;
    if (isInscribed()) {
        if (!inscribe && Random::Int(level) > 0) {
            GLog::w("wonderful energy of the scroll strips the glyph from your armor!");
            glyph = Glyph::NONE;
        }
    }
    if (STR > 1) STR--;
    Item::upgrade();
}

void Armor::degrade() {
    if (!isUpgradable()) return;
    STR++;
    Item::degrade();
}

void Armor::inscribe(Glyph g) {
    if (g == Glyph::NONE) {
        int r = Random::Int(13);
        switch (r) {
            case 0: glyph = Glyph::BOUNCE; break;
            case 1: glyph = Glyph::AFFECTION; break;
            case 2: glyph = Glyph::ANTI_ENTROPY; break;
            case 3: glyph = Glyph::AUTO_REPAIR; break;
            case 4: glyph = Glyph::DISPLACEMENT; break;
            case 5: glyph = Glyph::ENTANGLEMENT; break;
            case 6: glyph = Glyph::EVIL_AURORA; break;
            case 7: glyph = Glyph::FLOW; break;
            case 8: glyph = Glyph::POTENTIAL; break;
            case 9: glyph = Glyph::METABOLISM; break;
            case 10: glyph = Glyph::MULTI_VISION; break;
            case 11: glyph = Glyph::STENCH; break;
            case 12: glyph = Glyph::VISCOSITY; break;
        }
    } else {
        glyph = g;
    }
}

std::string Armor::displayName() const {
    std::string s = name;
    if (isInscribed()) {
        switch (glyph) {
            case Glyph::BOUNCE: s += " of Bounce"; break;
            case Glyph::AFFECTION: s += " of Affection"; break;
            case Glyph::ANTI_ENTROPY: s += " of Anti-entropy"; break;
            case Glyph::AUTO_REPAIR: s += " of Auto-repair"; break;
            case Glyph::DISPLACEMENT: s += " of Displacement"; break;
            case Glyph::ENTANGLEMENT: s += " of Entanglement"; break;
            case Glyph::EVIL_AURORA: s += " of Evil Aurora"; break;
            case Glyph::FLOW: s += " of Flow"; break;
            case Glyph::POTENTIAL: s += " of Potential"; break;
            case Glyph::METABOLISM: s += " of Metabolism"; break;
            case Glyph::MULTI_VISION: s += " of Multi-vision"; break;
            case Glyph::STENCH: s += " of Stench"; break;
            case Glyph::VISCOSITY: s += " of Viscosity"; break;
            default: break;
        }
    }
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
        if (hero->belongings.equippedArmor && hero->belongings.equippedArmor != this && hero->belongings.equippedArmor->isCursed()) {
            GLog::w("You can't remove cursed " + hero->belongings.equippedArmor->displayName() + "!");
            return false;
        }
        if (!hero->belongings.equip(this)) {
            return false;
        }
        hero->updateArmorTier();
        cursedKnown = true;
        if (cursed) {
            FX::burst(hero->pos, 0x550055FFu, 8);
            Sample::play(Assets::SND_CURSED);
            GLog::n("The " + displayName() + " is cursed! You cannot take it off!");
        }
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "UNEQUIP") {
        if (cursed && hero->belongings.equippedArmor == this) {
            GLog::w("You can't remove cursed " + displayName() + "!");
            return false;
        }
        if (hero->belongings.isFull()) {
            GLog::w("Your backpack is too full to unequip that!");
            return false;
        }
        hero->belongings.unequip(this);
        hero->updateArmorTier();
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "DROP") {
        if (hero->belongings.equippedArmor == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
        }
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        if (hero->belongings.equippedArmor == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
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

int Armor::DR() const {
    return tier * (2 + level + (isInscribed() ? 1 : 0));
}

int Armor::drRoll() const {
    return Random::IntRange(0, std::max(0, DR()));
}

int Armor::proc(Char* attacker, Char* defender, int damage) {
    (void)attacker;
    if (!levelKnown) {
        if (--hitsToKnow <= 0) {
            identify();
            Hero* h = defender ? defender->asHero() : nullptr;
            notifyIdentified(h);
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
    "Potion of Purification",
    "Potion of Invisibility",
    "Potion of Might",
    "Potion of Frost"
};

static const std::string s_potionTrueDescs[Potion::POTION_COUNT] = {
    "An elixir that will instantly return you to full health and cure poison.",
    "The storied experiences of multitudes of battles reduced to liquid form, this draught will instantly raise your experience level.",
    "Uncorking or shattering this pressurized glass will cause its contents to explode into a deadly cloud of toxic green gas. You might choose to fling this potion at distant enemies instead of uncorking it by hand.",
    "This flask contains an unstable compound which will burst violently into flame upon exposure to open air.",
    "This powerful liquid will course through your muscles, permanently increasing your strength by one point.",
    "Upon exposure to open air, the liquid in this flask will vaporize into a numbing yellow haze. Anyone who inhales the cloud will be paralyzed instantly, unable to move for some time after the cloud dissipates. This item can be thrown at distant enemies to catch them within the effect of the gas.",
    "Drinking this curious liquid will cause you to hover in the air, able to drift effortlessly over traps. Flames and gases fill the air, however, and cannot be bypassed while airborne.",
    "After drinking this, your mind will become attuned to the psychic signature of distant creatures, enabling you to sense biological presences through walls. Also this potion will permit you to see through nearby walls and doors.",
    "This reagent will quickly neutralize all harmful gases in the area of effect. Drinking it will give you a temporary immunity to such gases.",
    "Drinking this potion will render you temporarily invisible. While invisible, enemies will be unable to see you. Attacking an enemy, as well as using a wand or a scroll before enemy's eyes, will dispel the effect.",
    "This powerful liquid will course through your muscles, permanently increasing your strength by one point and health by five points.",
    "Upon exposure to open air, this chemical will evaporate into a freezing cloud, causing any creature that contacts it to be frozen in place, unable to act and move."
};

static const uint32_t s_potionSplashColors[Potion::POTION_COUNT] = {
    0x30e7d7FF, // turquoise
    0xdc143cFF, // crimson
    0x007fffFF, // azure
    0x00a86bFF, // jade
    0xffd700FF, // golden
    0xff00ffFF, // magenta
    0x404040FF, // charcoal
    0xfffff0FF, // ivory
    0xffbf00FF, // amber
    0x856d4dFF, // bistre
    0x4b0082FF, // indigo
    0xc0c0c0FF  // silver
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

bool Potion::allKnown() {
    for (size_t i = 0; i < POTION_COUNT; ++i) {
        if (!s_potionKnown[i]) return false;
    }
    return true;
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
    defaultAction = "DRINK";
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

int Potion::price() const {
    if (!isKnown(type)) {
        return 20 * quantity;
    }
    switch (type) {
        case PotionType::HEALING:       return 30 * quantity;
        case PotionType::EXPERIENCE:    return 80 * quantity;
        case PotionType::TOXIC_GAS:     return 40 * quantity;
        case PotionType::LIQUID_FLAME:  return 40 * quantity;
        case PotionType::STRENGTH:      return 100 * quantity;
        case PotionType::PARALYTIC_GAS: return 40 * quantity;
        case PotionType::LEVITATION:    return 35 * quantity;
        case PotionType::MIND_VISION:   return 35 * quantity;
        case PotionType::PURITY:        return 50 * quantity;
        case PotionType::INVISIBILITY:  return 40 * quantity;
        case PotionType::MIGHT:         return 200 * quantity;
        case PotionType::FROST:         return 50 * quantity;
        default:                        return 20 * quantity;
    }
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

void Potion::heal(Hero* hero) {
    if (!hero) return;
    hero->HP = hero->HT;
    hero->removeBuff(BuffType::Poison);
    hero->removeBuff(BuffType::Cripple);
    hero->removeBuff(BuffType::Weakness);
    hero->removeBuff(BuffType::Bleeding);
    FX::floatingText(hero->pos, "HEALED", 0x38b764FF);
    FX::burst(hero->pos, 0x38b764FF, 12);
}

void Potion::apply(Hero* hero) {
    if (!hero) return;

    switch (type) {
        case PotionType::HEALING: {
            setKnown(type);
            updatePerception();
            heal(hero);
            GLog::p("Your wounds heal completely.");
            break;
        }
        case PotionType::EXPERIENCE: {
            setKnown(type);
            updatePerception();
            hero->earnExp(hero->maxExp - hero->exp);
            FX::burst(hero->pos, 0xffd700FF, 10);
            GLog::p("You feel more experienced!");
            break;
        }
        case PotionType::STRENGTH: {
            setKnown(type);
            updatePerception();
            hero->STR += 1;
            FX::floatingText(hero->pos, "+1 STR", 0xe4a672FF);
            FX::burst(hero->pos, 0xe4a672FF, 8);
            GLog::p("Newfound strength surges through your body.");
            Badges::validateStrengthAttained();
            break;
        }
        case PotionType::MIGHT: {
            setKnown(type);
            updatePerception();
            hero->STR += 1;
            hero->HT += 5;
            hero->HP = std::min(hero->HT, hero->HP + 5);
            FX::floatingText(hero->pos, "+1 str, +5 ht", 0xffd700FF);
            FX::burst(hero->pos, 0xffd700FF, 12);
            GLog::p("Newfound strength surges through your body.");
            Badges::validateStrengthAttained();
            break;
        }
        case PotionType::INVISIBILITY: {
            setKnown(type);
            updatePerception();
            hero->affect<Invisibility>();
            FX::floatingText(hero->pos, "INVISIBLE", 0xa5cde4FF);
            FX::burst(hero->pos, 0xa5cde4FF, 10);
            GLog::i("You see your hands turn invisible!");
            Sample::play(Assets::SND_MELD);
            break;
        }
        case PotionType::LEVITATION: {
            setKnown(type);
            updatePerception();
            hero->affect<Levitation>();
            FX::burst(hero->pos, 0xdda0ddFF, 8);
            GLog::i("You float into the air!");
            break;
        }
        case PotionType::MIND_VISION: {
            setKnown(type);
            updatePerception();
            hero->affect<MindVision>();
            Dungeon::observe();
            FX::burst(hero->pos, 0x9955ffFF, 12);
            if (!Dungeon::mobs.empty()) {
                GLog::i("You can somehow feel the presence of other creatures' minds!");
            } else {
                GLog::i("You can somehow tell that you are alone on this level at the moment.");
            }
            break;
        }
        case PotionType::PURITY: {
            setKnown(type);
            updatePerception();
            GLog::w("You've stopped sensing any smells!");
            hero->prolong<GasesImmunity>(GasesImmunity::DURATION);
            FX::burst(hero->pos, 0xffffffFF, 12);
            break;
        }
        case PotionType::LIQUID_FLAME:
        case PotionType::TOXIC_GAS:
        case PotionType::PARALYTIC_GAS:
        case PotionType::FROST: {
            shatter(hero->pos);
            break;
        }
    }
}

bool Potion::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;

    if (action == "DRINK") {
        HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
        if (hspr) hspr->operate(hero->pos);

        Sample::play(Assets::SND_DRINK);
        apply(hero);

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
    if (Dungeon::hero && Dungeon::hero->pos == cell) {
        apply(Dungeon::hero.get());
        delete this;
        return;
    }
    if (Dungeon::level && (Dungeon::level->map[cell] == Terrain::WELL || Dungeon::level->pit[cell])) {
        Item::onThrow(cell);
        return;
    }
    shatter(cell);
    delete this;
}

void Potion::shatter(int cell) {
    if (cell < 0 || cell >= Level::LENGTH) return;

    bool isObserved = Dungeon::visible[cell];
    uint32_t flaskColor = s_potionSplashColors[s_potionColorMap[static_cast<size_t>(type)]];

    switch (type) {
        case PotionType::TOXIC_GAS: {
            if (isObserved) {
                setKnown(type);
                FX::splash(cell, flaskColor, 5);
                FX::burst(cell, 0x00ff00FF, 16);
                Sample::playAt(Assets::SND_SHATTER, cell);
                GLog::i("The flask shatters and a cloud of toxic gas billows out!");
            }
            FX::seedBlob(cell, Blob::BlobType::TOXIC_GAS, 1000);
            break;
        }
        case PotionType::LIQUID_FLAME: {
            if (isObserved) {
                setKnown(type);
                FX::splash(cell, flaskColor, 5);
                FX::burst(cell, 0xff4500FF, 16);
                Sample::playAt(Assets::SND_SHATTER, cell);
                GLog::i("The flask shatters and liquid flame bursts into a roaring inferno!");
            }
            FX::seedBlob(cell, Blob::BlobType::FIRE, 2);
            Char* targetChar = Actor::findChar(cell);
            if (targetChar) {
                targetChar->affect<Burning>();
            }
            break;
        }
        case PotionType::PARALYTIC_GAS: {
            if (isObserved) {
                setKnown(type);
                FX::splash(cell, flaskColor, 5);
                FX::burst(cell, 0xffff00FF, 16);
                Sample::playAt(Assets::SND_SHATTER, cell);
                GLog::i("The flask shatters into a cloud of paralyzing vapor!");
            }
            FX::seedBlob(cell, Blob::BlobType::PARALYTIC_GAS, 1000);
            break;
        }
        case PotionType::FROST: {
            bool notLosBlocking[Level::LENGTH];
            for (int i = 0; i < Level::LENGTH; ++i) {
                notLosBlocking[i] = Dungeon::level ? !Dungeon::level->losBlocking[i] : true;
            }
            PathFinder::buildDistanceMap(cell, notLosBlocking, 2);

            Blob* fire = FX::getBlob(Blob::BlobType::FIRE);
            bool visibleAny = false;
            for (int i = 0; i < Level::LENGTH; ++i) {
                if (PathFinder::distance[i] < INT_MAX) {
                    visibleAny = Freezing::affect(i, fire) || visibleAny;
                }
            }

            if (visibleAny) {
                FX::splash(cell, flaskColor, 5);
                Sample::playAt(Assets::SND_SHATTER, cell);
                setKnown(type);
                GLog::i("The flask shatters and freezing frost encases the area!");
            }
            break;
        }
        case PotionType::PURITY: {
            bool notLosBlocking[Level::LENGTH];
            for (int i = 0; i < Level::LENGTH; ++i) {
                notLosBlocking[i] = Dungeon::level ? !Dungeon::level->losBlocking[i] : true;
            }
            PathFinder::buildDistanceMap(cell, notLosBlocking, 2);

            bool procd = false;
            Blob* toxic = FX::getBlob(Blob::BlobType::TOXIC_GAS);
            Blob* para = FX::getBlob(Blob::BlobType::PARALYTIC_GAS);

            for (int i = 0; i < Level::LENGTH; ++i) {
                if (PathFinder::distance[i] < INT_MAX) {
                    if (toxic && toxic->cur[i] > 0) {
                        toxic->clear(i);
                        procd = true;
                        if (Dungeon::visible[i]) {
                            FX::burst(i, 0xffffffFF, 2);
                        }
                    }
                    if (para && para->cur[i] > 0) {
                        para->clear(i);
                        procd = true;
                        if (Dungeon::visible[i]) {
                            FX::burst(i, 0xffffffFF, 2);
                        }
                    }
                }
            }

            bool heroAffected = (Dungeon::hero != nullptr) && (PathFinder::distance[Dungeon::hero->pos] < INT_MAX);

            if (procd) {
                if (isObserved) {
                    FX::splash(cell, flaskColor, 5);
                    Sample::playAt(Assets::SND_SHATTER, cell);
                }
                setKnown(type);
                if (heroAffected) {
                    GLog::p("You feel uncommon freshness in the air.");
                }
            } else {
                if (isObserved) {
                    GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                    Sample::playAt(Assets::SND_SHATTER, cell);
                    FX::splash(cell, flaskColor, 5);
                }
                if (heroAffected) {
                    GLog::i("You feel uncommon freshness in the air.");
                    setKnown(type);
                }
            }
            break;
        }
        case PotionType::HEALING: {
            if (isObserved) {
                setKnown(type);
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0x38b764FF, 8);
            }
            break;
        }
        case PotionType::EXPERIENCE: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0xbf55ecFF, 8);
                FX::burst(cell, 0xffd700FF, 6);
            }
            break;
        }
        case PotionType::STRENGTH: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0xcc2222FF, 8);
            }
            break;
        }
        case PotionType::MIGHT: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0xffd700FF, 8);
            }
            break;
        }
        case PotionType::INVISIBILITY: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0xa5cde4FF, 8);
                FX::burst(cell, 0xffffff88, 6);
            }
            break;
        }
        case PotionType::LEVITATION: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0xdda0ddFF, 8);
                FX::burst(cell, 0xdda0dd88, 10);
            }
            break;
        }
        case PotionType::MIND_VISION: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, 0x9955ffFF, 8);
                FX::burst(cell, 0x9955ffFF, 6);
            }
            break;
        }
        default: {
            if (isObserved) {
                GLog::i("The flask shatters and " + colorName(type) + " liquid splashes harmlessly.");
                Sample::playAt(Assets::SND_SHATTER, cell);
                FX::splash(cell, flaskColor, 5);
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
static const std::string s_scrollRunes[Scroll::RUNIC_SCROLL_COUNT] = {
    "KAUNAN", "SOWILO", "LAGUZ", "YNGVI", "GYFU", "RAIDO",
    "ISAZ", "MANNAZ", "NAUDIZ", "BERKANAN", "ODAL", "TIWAZ"
};

static const int s_scrollImages[Scroll::RUNIC_SCROLL_COUNT] = {
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
    "Scroll of Enchantment",
    "Scroll of Wipe Out"
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
    "Infuses an item with rare, mystical enchantments and arcane power.",
    "Read this scroll to unleash the wrath of the dungeon spirits, killing everything on the current level. Well, almost everything. Some of the more powerful creatures may be not affected."
};

static std::vector<int> s_scrollRuneMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static std::vector<bool> s_scrollKnown(Scroll::RUNIC_SCROLL_COUNT, false);

void Scroll::initLabels() {
    s_scrollRuneMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int i = static_cast<int>(RUNIC_SCROLL_COUNT) - 1; i > 0; --i) {
        int j = Random::Int(i + 1);
        std::swap(s_scrollRuneMap[i], s_scrollRuneMap[j]);
    }
    std::fill(s_scrollKnown.begin(), s_scrollKnown.end(), false);
}

bool Scroll::isKnown(ScrollType tp) {
    if (tp == ScrollType::WIPE_OUT) return true;
    size_t idx = static_cast<size_t>(tp);
    return idx < RUNIC_SCROLL_COUNT ? s_scrollKnown[idx] : false;
}

bool Scroll::allKnown() {
    for (size_t i = 0; i < RUNIC_SCROLL_COUNT; ++i) {
        if (!s_scrollKnown[i]) return false;
    }
    return true;
}

void Scroll::setKnown(ScrollType tp) {
    if (tp == ScrollType::WIPE_OUT) return;
    size_t idx = static_cast<size_t>(tp);
    if (idx >= RUNIC_SCROLL_COUNT) return;
    s_scrollKnown[idx] = true;

    // 11 out of 12 deduction rule
    int knownCount = 0;
    int unknownIdx = -1;
    for (size_t i = 0; i < RUNIC_SCROLL_COUNT; ++i) {
        if (s_scrollKnown[i]) {
            knownCount++;
        } else {
            unknownIdx = static_cast<int>(i);
        }
    }
    if (knownCount == static_cast<int>(RUNIC_SCROLL_COUNT) - 1 && unknownIdx >= 0) {
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
    static const std::string emptyRune = "";
    if (tp == ScrollType::WIPE_OUT) return emptyRune;
    size_t idx = static_cast<size_t>(tp);
    int rIdx = (idx < RUNIC_SCROLL_COUNT) ? s_scrollRuneMap[idx] : 0;
    return s_scrollRunes[rIdx];
}

int Scroll::imageForType(ScrollType tp) {
    if (tp == ScrollType::WIPE_OUT) return ItemSpriteSheet::SCROLL_WIPE_OUT;
    size_t idx = static_cast<size_t>(tp);
    int rIdx = (idx < RUNIC_SCROLL_COUNT) ? s_scrollRuneMap[idx] : 0;
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
    if (mapping.size() == RUNIC_SCROLL_COUNT) {
        s_scrollRuneMap = mapping;
    }
    if (known.size() == RUNIC_SCROLL_COUNT) {
        s_scrollKnown = known;
    }
}

Scroll::Scroll(ScrollType tp) : type(tp) {
    defaultAction = "READ";
    updatePerception();
}

void Scroll::updatePerception() {
    image = imageForType(type);
    identified = isKnown(type);
    levelKnown = identified;
    cursedKnown = true;
    name = (type == ScrollType::WIPE_OUT || identified) ? trueName(type) : ("scroll \"" + runeName(type) + "\"");
}

Item* Scroll::identify() {
    setKnown(type);
    updatePerception();
    return this;
}

std::string Scroll::displayName() const {
    std::string s = (type == ScrollType::WIPE_OUT || isKnown(type)) ? trueName(type) : ("scroll \"" + runeName(type) + "\"");
    if (quantity > 1) s += " x" + std::to_string(quantity);
    return s;
}

int Scroll::price() const {
    if (type == ScrollType::WIPE_OUT) return 100 * quantity;
    if (!isKnown(type)) return 15 * quantity;
    switch (type) {
        case ScrollType::IDENTIFY:     return 30 * quantity;
        case ScrollType::MAGIC_MAPPING:return 25 * quantity;
        case ScrollType::REMOVE_CURSE: return 30 * quantity;
        case ScrollType::TELEPORTATION:return 40 * quantity;
        case ScrollType::TERROR:       return 50 * quantity;
        case ScrollType::LULLABY:      return 50 * quantity;
        case ScrollType::PSIONIC_BLAST:return 80 * quantity;
        case ScrollType::RECHARGING:
        case ScrollType::CHALLENGE:
        case ScrollType::MIRROR_IMAGE:
        case ScrollType::UPGRADE:
        case ScrollType::ENCHANTMENT:
        default:
            return 15 * quantity;
    }
}

std::unique_ptr<Scroll> Scroll::createRandom() {
    static const ScrollType types[] = {
        ScrollType::IDENTIFY,
        ScrollType::TELEPORTATION,
        ScrollType::REMOVE_CURSE,
        ScrollType::RECHARGING,
        ScrollType::MAGIC_MAPPING,
        ScrollType::CHALLENGE,
        ScrollType::TERROR,
        ScrollType::LULLABY,
        ScrollType::PSIONIC_BLAST,
        ScrollType::MIRROR_IMAGE,
        ScrollType::UPGRADE,
        ScrollType::ENCHANTMENT
    };
    static const float probs[] = { 30.0f, 10.0f, 15.0f, 10.0f, 15.0f, 12.0f, 8.0f, 8.0f, 4.0f, 6.0f, 0.0f, 1.0f };
    float total = 0.0f;
    for (float p : probs) total += p;
    float roll = Random::Float(total);
    float cum = 0.0f;
    for (size_t i = 0; i < 12; ++i) {
        cum += probs[i];
        if (roll < cum) {
            return std::make_unique<Scroll>(types[i]);
        }
    }
    return std::make_unique<Scroll>(ScrollType::IDENTIFY);
}

Item* Scroll::random() {
    auto rolled = createRandom();
    type = rolled->type;
    image = rolled->image;
    return this;
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
        if (hero->buff(BuffType::Blindness)) {
            GLog::w("You can't read a scroll while blinded");
            return false;
        }
        if (hero->buff(BuffType::Sleep) || hero->buff(BuffType::Paralysis)) {
            return false;
        }

        HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
        if (hspr) hspr->read();

        switch (type) {
            case ScrollType::IDENTIFY: {
                setKnown(type);
                updatePerception();
                if (ui::BottomScreenUI::instance()) {
                    ui::BottomScreenUI::instance()->selectItem(ui::WndBag::Mode::UNIDENTIFIED, "Select an item to identify", [this, hero](Item* target) {
                        if (target) {
                            target->identify();
                            FX::burst(hero->pos, 0x00ffffFFu, 12);
                            FX::floatingText(hero->pos, "IDENTIFIED", 0x00ffffFFu);
                            GLog::i("It is " + target->displayName() + "!");
                            Sample::play(Assets::SND_READ);
                            hero->removeBuff(BuffType::Invisibility);
                            this->quantity--;
                            if (this->quantity <= 0) {
                                hero->belongings.drop(this);
                            }
                            hero->spendAndNext(Actor::TICK);
                            Actor::process();
                        }
                    });
                    return false;
                }
                break;
            }
            case ScrollType::UPGRADE: {
                setKnown(type);
                updatePerception();
                if (ui::BottomScreenUI::instance()) {
                    ui::BottomScreenUI::instance()->selectItem(ui::WndBag::Mode::UPGRADEABLE, "Select an item to upgrade", [this, hero](Item* target) {
                        if (target && target->isUpgradable()) {
                            if (target->cursed) {
                                target->cursed = false;
                                target->cursedKnown = true;
                            }
                            if (target->isBroken()) {
                                target->fix();
                            } else {
                                if (auto* w = target->asWeapon()) {
                                    w->upgrade(false);
                                } else if (auto* a = target->asArmor()) {
                                    a->upgrade(false);
                                } else {
                                    target->upgrade();
                                }
                            }
                            FX::burst(hero->pos, 0xffd700FF, 10);
                            FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                            char buf[128];
                            std::snprintf(buf, sizeof(buf), "your %s certainly looks better now", target->displayName().c_str());
                            GLog::p(buf);
                            Sample::play(Assets::SND_READ);
                            hero->removeBuff(BuffType::Invisibility);
                            this->quantity--;
                            if (this->quantity <= 0) {
                                hero->belongings.drop(this);
                            }
                            hero->spendAndNext(Actor::TICK);
                            Actor::process();
                        }
                    });
                    return false;
                }
                break;
            }
            case ScrollType::ENCHANTMENT: {
                setKnown(type);
                updatePerception();
                if (ui::BottomScreenUI::instance()) {
                    ui::BottomScreenUI::instance()->selectItem(ui::WndBag::Mode::ENCHANTABLE, "Select an enchantable item", [this, hero](Item* target) {
                        if (target && target->isUpgradable() && (target->asArmor() != nullptr || (target->asWeapon() != nullptr && !target->asWand() && (!target->asMissile() || target->asBoomerang())))) {
                            if (target->cursed) {
                                target->cursed = false;
                                target->cursedKnown = true;
                            }
                            if (auto* w = target->asWeapon()) {
                                w->enchant();
                            } else if (auto* a = target->asArmor()) {
                                a->inscribe();
                            }
                            target->fix();
                            FX::burst(hero->pos, 0xff00ffFF, 16);
                            char buf[128];
                            std::snprintf(buf, sizeof(buf), "your %s glows in the dark", target->displayName().c_str());
                            GLog::w(buf);
                            Sample::play(Assets::SND_READ);
                            hero->removeBuff(BuffType::Invisibility);
                            this->quantity--;
                            if (this->quantity <= 0) {
                                hero->belongings.drop(this);
                            }
                            hero->spendAndNext(Actor::TICK);
                            Actor::process();
                        }
                    });
                    return false;
                }
                break;
            }
            case ScrollType::MAGIC_MAPPING: {
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                Sample::play(Assets::SND_READ);
                bool noticed = false;
                if (Dungeon::level) {
                    for (int i = 0; i < Level::LENGTH; ++i) {
                        int terr = Dungeon::level->map[i];
                        if (terr != Terrain::WALL && terr != Terrain::CHASM) {
                            Dungeon::level->mapped[i] = true;
                        }
                        if (Terrain::isSecret(terr)) {
                            Dungeon::level->mapped[i] = true;
                            Dungeon::level->map[i] = Terrain::discover(terr);
                            if (Dungeon::visible[i]) {
                                noticed = true;
                            }
                        }
                    }
                    Dungeon::observe();
                    FX::burst(hero->pos, 0x44aaffFF, 12);
                    if (noticed) {
                        Sample::play(Assets::SND_SECRET);
                    }
                    GLog::i("You are now aware of the level layout.");
                }
                break;
            }
            case ScrollType::RECHARGING: {
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
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
                Sample::play(Assets::SND_READ);
                FX::burst(hero->pos, 0x55ffaaFF, 15);
                if (count > 0) {
                    GLog::i("A surge of energy courses through your pack, recharging your wand" + std::string(count > 1 ? "s" : "") + "!");
                } else {
                    GLog::i("A surge of energy courses through your pack, but nothing happens.");
                }
                break;
            }
            case ScrollType::REMOVE_CURSE: {
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
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
                Sample::play(Assets::SND_READ);
                FX::burst(hero->pos, 0xdda0ddFF, 16);
                if (procced) {
                    GLog::p("Your pack glows with a cleansing light, and a malevolent energy disperses.");
                } else {
                    GLog::i("Your pack glows with a cleansing light, but nothing happens.");
                }
                break;
            }
            case ScrollType::TELEPORTATION: {
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                Sample::play(Assets::SND_READ);
                if (Dungeon::bossLevel() || !Dungeon::level) {
                    GLog::w("Strong magic aura of this place prevents you from teleporting!");
                } else {
                    int target = Dungeon::level->randomRespawnCell();
                    if (target == -1) {
                        GLog::w("Strong magic aura of this place prevents you from teleporting!");
                    } else {
                        FX::burst(hero->pos, 0x9955ffFF, 8);
                        hero->place(target);
                        Dungeon::level->press(target, hero);
                        Dungeon::observe();
                        FX::burst(hero->pos, 0x9955ffFF, 8);
                        GLog::i("In a blink of an eye you were teleported to another location of the level.");
                    }
                }
                break;
            }
            case ScrollType::CHALLENGE: {
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                Sample::play(Assets::SND_CHALLENGE);
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
                if (Dungeon::level) {
                    std::vector<Heap*> mimicHeaps;
                    for (auto& h : Dungeon::level->heaps) {
                        if (h && h->type == Heap::Type::MIMIC) {
                            mimicHeaps.push_back(h.get());
                        }
                    }
                    for (Heap* h : mimicHeaps) {
                        h->open(hero);
                    }
                }
                break;
            }
            case ScrollType::TERROR: {
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                Sample::play(Assets::SND_READ);
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
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                Sample::play(Assets::SND_LULLABY);
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
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                FX::flash(0xFFFFFFFF, 0.5f);
                if (noosa::Camera::main) {
                    noosa::Camera::main->shake(0.35f, 4.0f);
                }
                Sample::play(Assets::SND_BLAST);
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
                setKnown(type);
                updatePerception();
                hero->removeBuff(BuffType::Invisibility);
                Sample::play(Assets::SND_READ);
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
                    if (p >= 0 && p < Level::LENGTH && Dungeon::level && (Dungeon::level->isPassable(p) || Dungeon::level->avoid[p]) && !Actor::findChar(p)) {
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
            case ScrollType::WIPE_OUT: {
                hero->removeBuff(BuffType::Invisibility);
                FX::flash(0xFF6644FF, 0.5f);
                Sample::play(Assets::SND_CURSED);
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && !m->isBoss()) {
                        m->die(hero);
                    }
                }
                if (Dungeon::level) {
                    for (auto& h : Dungeon::level->heaps) {
                        if (h && (h->type == Heap::Type::FOR_SALE || h->type == Heap::Type::MIMIC)) {
                            h->type = Heap::Type::HEAP;
                        }
                    }
                }
                GLog::w("A cataclysmic energy wipes out all foes on this floor!");
                break;
            }
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
    energy = static_cast<float>(Hunger::HUNGRY);
    message = "That food tasted delicious!";
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

        Sample::play(Assets::SND_EAT);

        Hunger* hunger = hero->getBuff<Hunger>();
        if (hunger) {
            hunger->satisfy(energy);
        }
        Statistics::foodEaten++;
        GLog::i(message);
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

int Food::price() const {
    return 10 * quantity;
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
    defaultAction = "LIGHT";
}

std::vector<std::string> Torch::actions(Hero* hero) {
    (void)hero;
    return { "LIGHT", "DROP", "THROW" };
}

bool Torch::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "LIGHT" || action == "USE") {
        hero->spendAndNext(Actor::TICK);
        hero->busy();
        if (auto* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr) {
            hspr->operate(hero->pos);
        }
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_ITEM).get());

        quantity--;
        if (quantity <= 0) {
            hero->belongings.backpack.erase(
                std::remove_if(hero->belongings.backpack.begin(), hero->belongings.backpack.end(),
                               [this](const std::unique_ptr<Item>& it) { return it.get() == this; }),
                hero->belongings.backpack.end());
        }
        hero->affect<Light>();
        GLog::p("You light a torch. The warm glow pushes back the darkness!");
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

int Torch::price() const {
    return 10 * quantity;
}

// -----------------------------------------------------------------------------
// Keys
// -----------------------------------------------------------------------------
Key::Key() : Key(Dungeon::depth) {}

Key::Key(int d) : depth(d) {
    level = d;
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

SkeletonKey::SkeletonKey() : SkeletonKey(Dungeon::depth) {}

SkeletonKey::SkeletonKey(int d) : Key(d) {
    name = "Skeleton Key";
    image = ItemSpriteSheet::SKELETON_KEY;
}

std::string SkeletonKey::info() const {
    return "This key looks serious: its head is shaped like a skull. "
           "Probably it can open some serious door.";
}

std::unique_ptr<Item> SkeletonKey::clone(int qty) const {
    auto k = std::make_unique<SkeletonKey>(depth);
    k->quantity = qty;
    return k;
}

// =============================================================================
// MissileWeapon Implementations
// =============================================================================
MissileWeapon::MissileWeapon(std::string n, int minD, int maxD, int strReq)
    : Weapon(std::move(n), minD, maxD, strReq) {
    defaultAction = "THROW";
}

std::vector<std::string> MissileWeapon::actions(Hero* hero) {
    (void)hero;
    return { "THROW", "DROP" };
}

float MissileWeapon::acuracyFactor(Hero* hero) const {
    return Weapon::acuracyFactor(hero);
}

float MissileWeapon::speedFactor(Hero* hero) const {
    return Weapon::speedFactor(hero);
}

std::string MissileWeapon::info() const {
    std::string s = desc();
    int mn = min();
    int mx = max();
    s += "\n\nAverage damage of this weapon equals to " + std::to_string(mn + (mx - mn) / 2) + " points per hit. ";

    if (Dungeon::hero) {
        if (STR > Dungeon::hero->effectiveSTR()) {
            s += "Because of your inadequate strength the accuracy and speed of your attack with this " + name + " is decreased.";
        } else if (STR < Dungeon::hero->effectiveSTR()) {
            s += "Because of your excess strength the damage of your attack with this " + name + " is increased.";
        }
    }

    if (Dungeon::hero && Dungeon::hero->belongings.equippedWeapon == this) {
        s += "\n\nYou hold the " + name + " at the ready.";
    }
    return s;
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

void MissileWeapon::proc(Char* attacker, Char* defender, int damage) {
    Weapon::proc(attacker, defender, damage);
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

std::string Dart::desc() const {
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
    Weapon::proc(attacker, defender, damage);
    if (defender) {
        defender->affect<Burning>(4);
    }
}

std::string IncendiaryDart::desc() const {
    return "The spike on each of these darts is designed to pin it to its target while the unstable compounds strapped to its length burst into brilliant flames.";
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
    Weapon::proc(attacker, defender, damage);
    if (defender) {
        defender->affect<Paralysis>(3);
    }
}

std::string CurareDart::desc() const {
    return "These little evil darts don't do much damage but they can paralyze the target leaving it helpless and motionless for some time.";
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
    Weapon::proc(attacker, defender, damage);
    if (defender) {
        defender->affect<Cripple>();
    }
}

int Javelin::maxDistance(const Hero* hero) const {
    bool isSniper = (hero && hero->subClass == HeroSubClass::Sniper);
    return isSniper ? 7 : 6;
}

std::string Javelin::desc() const {
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

std::string Shuriken::desc() const {
    return "Star-shaped pieces of metal with razor-sharp blades do significant damage when they hit a target. They can be thrown at very high rate.";
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
    Weapon::proc(attacker, defender, damage);
    if (defender) {
        defender->affect<Bleeding>(damage);
    }
}

int Tamahawk::maxDistance(const Hero* hero) const {
    bool isSniper = (hero && hero->subClass == HeroSubClass::Sniper);
    return isSniper ? 7 : 6;
}

std::string Tamahawk::desc() const {
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

int Boomerang::min() const {
    return isBroken() ? 1 : 1 + level;
}

int Boomerang::max() const {
    return isBroken() ? 4 : 4 + 2 * level;
}

int Boomerang::damageRoll() const {
    return Random::NormalIntRange(min(), max());
}

int Boomerang::price() const {
    return considerState(20);
}

void Boomerang::cast(Hero* hero, int targetCell) {
    throwEquipped = (hero && hero->belongings.equippedWeapon == this);
    Item::cast(hero, targetCell);
}

void Boomerang::onThrow(int cell) {
    Char* enemy = Actor::findChar(cell);
    if (!enemy || enemy == Dungeon::hero.get()) {
        miss(cell);
    } else {
        if (!Dungeon::hero->shoot(enemy, this)) {
            miss(cell);
        }
    }
}

void Boomerang::miss(int cell) {
    circleBack(cell, Dungeon::hero.get());
}

void Boomerang::proc(Char* attacker, Char* defender, int damage) {
    Weapon::proc(attacker, defender, damage);
    if (attacker && defender) {
        Hero* h = (attacker == Dungeon::hero.get()) ? Dungeon::hero.get() : nullptr;
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

    int returnCell = Ballistica::cast(fromCell, heroCell, false, false);

    // Launch return missile sprite arc backwards along path
    FX::missile(fromCell, returnCell, image, [this, owner, heroCell, returnCell]() {
        std::unique_ptr<Item> self(this);
        if (returnCell == heroCell) {
            Sample::play(Assets::SND_ITEM);
            if (throwEquipped) {
                if (!owner->belongings.weaponSlot) {
                    owner->belongings.weaponSlot = std::unique_ptr<Weapon>(static_cast<Weapon*>(self.release()));
                    owner->belongings.equippedWeapon = owner->belongings.weaponSlot.get();
                } else {
                    if (!owner->belongings.collect(self)) {
                        if (Dungeon::level) {
                            Dungeon::level->drop(std::move(self), heroCell);
                        }
                    }
                }
                throwEquipped = false;
            } else if (!owner->belongings.collect(self)) {
                GLog::w("Your backpack is too full for " + displayName());
                if (Dungeon::level) {
                    Dungeon::level->drop(std::move(self), heroCell);
                }
            }
        } else {
            throwEquipped = false;
            if (Dungeon::level) {
                Dungeon::level->drop(std::move(self), returnCell);
            }
        }
    });
}

std::string Boomerang::desc() const {
    return "Thrown to the enemy this flat curved wooden missile will return to the hands of its thrower.";
}

std::string Boomerang::info() const {
    return MissileWeapon::info();
}

std::unique_ptr<Item> Boomerang::clone(int qty) const {
    auto b = std::make_unique<Boomerang>();
    b->quantity = qty;
    b->level = level;
    b->identified = identified;
    b->levelKnown = levelKnown;
    b->cursed = cursed;
    b->cursedKnown = cursedKnown;
    b->enchantment = enchantment;
    b->imbue = imbue;
    return b;
}

// =============================================================================
// PlantSeed Implementation
// =============================================================================
PlantSeed::PlantSeed(PlantType pt) : plantType(pt) {
    defaultAction = "THROW";
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
        PlantType pt = plantType;
        quantity--;
        OnSeedImpact(pt, hero->pos);
        hero->spendAndNext(Actor::TICK);
        return true;
    }
    return Item::execute(hero, action);
}

void PlantSeed::onThrow(int cell) {
    PlantType pt = plantType;
    delete this;
    OnSeedImpact(pt, cell);
}

void PlantSeed::onCollect(Hero* hero) {
    if (plantType == PlantType::ROTBERRY) {
        int heroPos = hero ? hero->pos : (Dungeon::hero ? Dungeon::hero->pos : -1);
        if (Dungeon::level && heroPos != -1) {
            for (auto& mob : Dungeon::mobs) {
                if (mob && mob->isAlive()) {
                    mob->beckon(heroPos);
                }
            }
            GLog::w("The seed emits a roar that echoes throughout the dungeon!");
            FX::burst(heroPos, 0x55aa33FF, 10);
            Sample::play(Assets::SND_CHALLENGE);
        }
    }
}

std::string PlantSeed::info() const {
    const auto& eff = SeedRegistry::get(plantType);
    return "Throw this seed to the place where you want to grow " + eff.plantName + ".\n\n" + eff.description;
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
    defaultAction = "READ";
    unique = true;
}

std::string TomeOfMastery::displayName() const {
    if (Dungeon::hero && Dungeon::hero->subClass != HeroSubClass::None) {
        return "Tome of Remastery";
    }
    return "Tome of Mastery";
}

std::vector<std::string> TomeOfMastery::actions(Hero* hero) {
    (void)hero;
    return { "READ", "DROP", "THROW" };
}

bool TomeOfMastery::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "READ") {
        if (hero->paralysed) {
            GLog::w("You can't read while paralyzed!");
            return false;
        }
        if (hero->buff(BuffType::Sleep)) {
            GLog::w("You can't read while asleep!");
            return false;
        }
        if (hero->buff(BuffType::Blindness)) {
            GLog::w("You can't read while blinded!");
            return false;
        }
        Sample::play(Assets::SND_READ);
        ui::BottomScreenUI::instance()->showChooseSubclass(this);
        return false;
    }
    return Item::execute(hero, action);
}

void TomeOfMastery::onCollect(Hero* hero) {
    (void)hero;
    Badges::validateMastery();
}

void TomeOfMastery::choose(Hero* hero, HeroSubClass way) {
    if (!hero) return;
    hero->subClass = way;
    hero->spendAndNext(TIME_TO_READ * Actor::TICK);

    HeroSprite* hspr = hero->getSprite() ? hero->getSprite()->asHeroSprite() : nullptr;
    if (hspr) {
        hspr->operate(hero->pos);
    }

    Sample::play(Assets::SND_MASTERY);
    FX::burst(hero->pos, 0xffd700FF, 16);
    FX::floatingText(hero->pos, subClassTitle(way), 0xffd700FF);
    GLog::w("You have chosen the path of the " + std::string(subClassTitle(way)) + "!");

    if (way == HeroSubClass::Berserker && hero->HP <= hero->HT * 0.40f) {
        hero->affect<Fury>();
    }

    quantity = 0;
    hero->belongings.drop(this);

    SaveManager::saveGame();
    ui::BottomScreenUI::setLastTarget(nullptr);
    Actor::process();
}

std::string TomeOfMastery::info() const {
    return "This worn leather book is not that thick, but you feel somehow that you can gather a lot from it. Remember though that reading this tome may require some time.";
}

std::unique_ptr<Item> TomeOfMastery::clone(int qty) const {
    auto t = std::make_unique<TomeOfMastery>();
    t->quantity = qty;
    return t;
}

// =============================================================================
// ArmorKit Implementation
// =============================================================================
ArmorKit::ArmorKit() {
    name = "armor kit";
    image = ItemSpriteSheet::KIT;
    unique = true;
    identified = true;
    levelKnown = true;
}

std::vector<std::string> ArmorKit::actions(Hero* hero) {
    (void)hero;
    return { "APPLY", "DROP" };
}

bool ArmorKit::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "APPLY") {
        ui::BottomScreenUI::instance()->selectItem(ui::WndBag::Mode::ARMOR, "Select an armor to upgrade", [this, hero](Item* target) {
            if (!target) return;
            Armor* armor = target->asArmor();
            if (!armor) return;

            if (armor->asClassArmor()) {
                GLog::w("This armor has already been transformed!");
                return;
            }

            // Remove 1 kit from inventory
            for (auto it = hero->belongings.backpack.begin(); it != hero->belongings.backpack.end(); ++it) {
                if (it->get() == this) {
                    quantity--;
                    if (quantity <= 0) {
                        hero->belongings.backpack.erase(it);
                    }
                    break;
                }
            }

            hero->spend(2.0f);
            hero->busy();

            GLog::p("You applied the armor kit to upgrade your " + armor->displayName() + "!");

            std::unique_ptr<ClassArmor> classArmor = ClassArmor::upgradeArmor(hero, armor);
            if (!classArmor) return;

            if (hero->belongings.equippedArmor == armor) {
                hero->belongings.armorSlot = std::move(classArmor);
                hero->belongings.equippedArmor = hero->belongings.armorSlot.get();
                hero->updateArmorTier();
            } else {
                for (auto it = hero->belongings.backpack.begin(); it != hero->belongings.backpack.end(); ++it) {
                    if (it->get() == armor) {
                        *it = std::move(classArmor);
                        break;
                    }
                }
            }

            if (hero->getSprite() && hero->getSprite()->asHeroSprite()) {
                hero->getSprite()->asHeroSprite()->operate(hero->pos);
            }
            Sample::play(Assets::SND_EVOKE);
            FX::burst(hero->pos, 0x55ff55FF, 12);
            Actor::process();
        });
        return false;
    }
    return Item::execute(hero, action);
}

std::string ArmorKit::info() const {
    return "Using this kit of small tools and materials anybody can transform any armor into an \"epic armor\", "
           "which will keep all properties of the original armor, but will also provide its wearer a special ability "
           "depending on his class. No skills in tailoring, leatherworking or blacksmithing are required.";
}

std::unique_ptr<Item> ArmorKit::clone(int qty) const {
    auto k = std::make_unique<ArmorKit>();
    k->quantity = qty;
    return k;
}

// =============================================================================
// ClassArmor Implementation
// =============================================================================
ClassArmor::ClassArmor(std::string n)
    : Armor(std::move(n), 6, 11) {
    identified = true;
    levelKnown = true;
    cursed = false;
    cursedKnown = true;
    hitsToKnow = 0;
}

int ClassArmor::drRoll() const {
    return Random::IntRange(0, std::max(0, DR_val));
}

std::vector<std::string> ClassArmor::actions(Hero* hero) {
    std::vector<std::string> acts;
    if (hero && hero->HP >= 3 && hero->belongings.equippedArmor == this) {
        acts.push_back(special());
    }
    if (hero && hero->belongings.equippedArmor == this) {
        acts.push_back("UNEQUIP");
        acts.push_back("THROW");
        acts.push_back("DROP");
    } else {
        acts.push_back("EQUIP");
        acts.push_back("THROW");
        acts.push_back("DROP");
    }
    return acts;
}

bool ClassArmor::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == special()) {
        if (hero->HP < 3) {
            GLog::w("Your health is too low!");
            return false;
        } else if (hero->belongings.equippedArmor != this) {
            GLog::w("You need to be wearing this armor to use its special power!");
            return false;
        } else {
            return doSpecial(hero);
        }
    }
    return Armor::execute(hero, action);
}

std::string ClassArmor::info() const {
    return desc() + "\n\nDefense: Max DR " + std::to_string(DR_val) +
           " | STR required: " + std::to_string(STR);
}

std::unique_ptr<ClassArmor> ClassArmor::upgradeArmor(Hero* owner, Armor* armor) {
    if (!owner || !armor) return nullptr;

    std::unique_ptr<ClassArmor> classArmor;
    switch (owner->heroClass) {
        case Hero::HeroClass::Warrior:
            classArmor = std::make_unique<WarriorArmor>();
            break;
        case Hero::HeroClass::Rogue:
            classArmor = std::make_unique<RogueArmor>();
            break;
        case Hero::HeroClass::Mage:
            classArmor = std::make_unique<MageArmor>();
            break;
        case Hero::HeroClass::Huntress:
            classArmor = std::make_unique<HuntressArmor>();
            break;
        default:
            classArmor = std::make_unique<WarriorArmor>();
            break;
    }

    classArmor->STR = armor->STR;
    classArmor->DR_val = armor->DR();
    classArmor->level = armor->level;
    classArmor->levelKnown = true;
    classArmor->cursed = false;
    classArmor->cursedKnown = true;
    classArmor->identified = true;
    classArmor->hitsToKnow = 0;
    classArmor->glyph = armor->glyph;
    classArmor->defaultAction = classArmor->special();

    return classArmor;
}

// -----------------------------------------------------------------------------
// WarriorArmor (Heroic Leap)
// -----------------------------------------------------------------------------
WarriorArmor::WarriorArmor()
    : ClassArmor("warrior suit of armor") {
    image = ItemSpriteSheet::ARMOR_WARRIOR;
    defaultAction = special();
}

std::string WarriorArmor::desc() const {
    return "While this armor looks heavy, it allows a warrior to perform heroic leap towards "
           "a targeted location, slamming down to stun all neighbouring enemies.";
}

bool WarriorArmor::execute(Hero* hero, const std::string& action) {
    if (action == "EQUIP") {
        if (hero && hero->heroClass != Hero::HeroClass::Warrior) {
            GLog::w("Only warriors can use this armor!");
            return false;
        }
    }
    return ClassArmor::execute(hero, action);
}

bool WarriorArmor::doSpecial(Hero* hero) {
    if (!hero) return false;
    ui::BottomScreenUI::instance()->startTargeting(this);
    ui::BottomScreenUI::instance()->log("Choose direction to leap", C2D_Color32(0xff, 0xff, 0x66, 0xFF));
    return false;
}

void WarriorArmor::cast(Hero* hero, int targetCell) {
    if (!hero || targetCell == hero->pos) return;

    int cell = Ballistica::cast(hero->pos, targetCell, false, true);
    if (Actor::findChar(cell) != nullptr && cell != hero->pos) {
        if (Ballistica::distance > 1) {
            cell = Ballistica::trace[Ballistica::distance - 2];
        }
    }

    hero->HP -= (hero->HP / 3);
    if (hero->subClass == HeroSubClass::Berserker && hero->HP <= hero->HT * Fury::LEVEL) {
        hero->affect<Fury>();
    }
    hero->removeBuff(BuffType::Invisibility);

    hero->move(cell);
    if (hero->getSprite()) {
        hero->getSprite()->place(cell);
    }
    if (Dungeon::level) {
        Dungeon::level->press(cell, hero);
    }
    Dungeon::observe();

    for (int n : Level::NEIGHBOURS8) {
        int neighborCell = cell + n;
        if (neighborCell >= 0 && neighborCell < Level::LENGTH) {
            Char* mob = Actor::findChar(neighborCell);
            if (mob && mob != hero) {
                mob->prolong<Paralysis>(3.0f);
            }
        }
    }

    FX::burst(cell, 0x887766FF, 12);
    if (noosa::Camera::main) {
        noosa::Camera::main->shake(0.5f, 2.0f);
    }
    Sample::play(Assets::SND_ROCKS);
    hero->spend(1.0f);
    hero->next();
    Actor::process();
}

std::unique_ptr<Item> WarriorArmor::clone(int qty) const {
    auto a = std::make_unique<WarriorArmor>();
    a->STR = STR;
    a->DR_val = DR_val;
    a->level = level;
    a->levelKnown = levelKnown;
    a->cursed = cursed;
    a->cursedKnown = cursedKnown;
    a->identified = identified;
    a->glyph = glyph;
    a->quantity = qty;
    return a;
}

// -----------------------------------------------------------------------------
// MageArmor (Molten Earth)
// -----------------------------------------------------------------------------
MageArmor::MageArmor()
    : ClassArmor("mage robe") {
    image = ItemSpriteSheet::ARMOR_MAGE;
    defaultAction = special();
}

std::string MageArmor::desc() const {
    return "Wearing this gorgeous robe, a mage can cast a spell of molten earth: all the enemies "
           "in his field of view will be set on fire and unable to move at the same time.";
}

bool MageArmor::execute(Hero* hero, const std::string& action) {
    if (action == "EQUIP") {
        if (hero && hero->heroClass != Hero::HeroClass::Mage) {
            GLog::w("Only mages can use this armor!");
            return false;
        }
    }
    return ClassArmor::execute(hero, action);
}

bool MageArmor::doSpecial(Hero* hero) {
    if (!hero) return false;

    for (auto& mob : Dungeon::mobs) {
        if (mob && mob->isAlive() && Dungeon::visible[mob->pos]) {
            mob->affect<Burning>();
            mob->prolong<Roots>(3.0f);
            FX::burst(mob->pos, 0xff4400FF, 6);
        }
    }

    hero->HP -= (hero->HP / 3);

    hero->spend(Actor::TICK);
    if (hero->getSprite() && hero->getSprite()->asHeroSprite()) {
        hero->getSprite()->asHeroSprite()->operate(hero->pos);
    }
    hero->busy();

    FX::burst(hero->pos, 0xff8800FF, 8);
    Sample::play(Assets::SND_READ);
    hero->next();
    Actor::process();
    return true;
}

std::unique_ptr<Item> MageArmor::clone(int qty) const {
    auto a = std::make_unique<MageArmor>();
    a->STR = STR;
    a->DR_val = DR_val;
    a->level = level;
    a->levelKnown = levelKnown;
    a->cursed = cursed;
    a->cursedKnown = cursedKnown;
    a->identified = identified;
    a->glyph = glyph;
    a->quantity = qty;
    return a;
}

// -----------------------------------------------------------------------------
// RogueArmor (Smoke Bomb)
// -----------------------------------------------------------------------------
RogueArmor::RogueArmor()
    : ClassArmor("rogue garb") {
    image = ItemSpriteSheet::ARMOR_ROGUE;
    defaultAction = special();
}

std::string RogueArmor::desc() const {
    return "Wearing this dark garb, a rogue can perform a trick, that is called \"smoke bomb\" "
           "(though no real explosives are used): he blinds enemies who could see him and jumps aside.";
}

bool RogueArmor::execute(Hero* hero, const std::string& action) {
    if (action == "EQUIP") {
        if (hero && hero->heroClass != Hero::HeroClass::Rogue) {
            GLog::w("Only rogues can use this armor!");
            return false;
        }
    }
    return ClassArmor::execute(hero, action);
}

bool RogueArmor::doSpecial(Hero* hero) {
    if (!hero) return false;
    ui::BottomScreenUI::instance()->startTargeting(this);
    ui::BottomScreenUI::instance()->log("Choose a location to jump to", C2D_Color32(0xff, 0xff, 0x66, 0xFF));
    return false;
}

void RogueArmor::cast(Hero* hero, int targetCell) {
    if (!hero) return;

    if (!Dungeon::level || !Dungeon::visible[targetCell] || !(Dungeon::level->passable[targetCell] || Dungeon::level->avoid[targetCell]) || Actor::findChar(targetCell) != nullptr) {
        GLog::w("You can only jump to an empty location in your field of view");
        return;
    }

    hero->HP -= (hero->HP / 3);

    for (auto& mob : Dungeon::mobs) {
        if (mob && mob->isAlive() && Dungeon::visible[mob->pos]) {
            mob->prolong<Blindness>(2.0f);
            mob->state = Mob::State::WANDERING;
            FX::burst(mob->pos, 0xffffffFF, 4);
        }
    }

    hero->move(targetCell);
    if (hero->getSprite()) {
        hero->getSprite()->place(targetCell);
    }
    FX::burst(targetCell, 0xddddddFF, 10);
    Sample::play(Assets::SND_PUFF);
    if (Dungeon::level) {
        Dungeon::level->press(targetCell, hero);
    }
    Dungeon::observe();

    hero->spend(Actor::TICK);
    hero->next();
    Actor::process();
}

std::unique_ptr<Item> RogueArmor::clone(int qty) const {
    auto a = std::make_unique<RogueArmor>();
    a->STR = STR;
    a->DR_val = DR_val;
    a->level = level;
    a->levelKnown = levelKnown;
    a->cursed = cursed;
    a->cursedKnown = cursedKnown;
    a->identified = identified;
    a->glyph = glyph;
    a->quantity = qty;
    return a;
}

// -----------------------------------------------------------------------------
// HuntressArmor (Spectral Blades)
// -----------------------------------------------------------------------------
HuntressArmor::HuntressArmor()
    : ClassArmor("huntress cloak") {
    image = ItemSpriteSheet::ARMOR_HUNTRESS;
    defaultAction = special();
}

std::string HuntressArmor::desc() const {
    return "A huntress in such cloak can create a fan of spectral blades. Each of these blades "
           "will target a single enemy in the huntress's field of view, inflicting damage depending "
           "on her currently equipped melee weapon.";
}

bool HuntressArmor::execute(Hero* hero, const std::string& action) {
    if (action == "EQUIP") {
        if (hero && hero->heroClass != Hero::HeroClass::Huntress) {
            GLog::w("Only huntresses can use this armor!");
            return false;
        }
    }
    return ClassArmor::execute(hero, action);
}

bool HuntressArmor::doSpecial(Hero* hero) {
    if (!hero) return false;

    std::vector<Mob*> targets;
    for (auto& mob : Dungeon::mobs) {
        if (mob && mob->isAlive() && Dungeon::visible[mob->pos]) {
            targets.push_back(mob.get());
        }
    }

    if (targets.empty()) {
        GLog::w("No enemies in sight");
        return false;
    }

    hero->HP -= (hero->HP / 3);

    for (Mob* mob : targets) {
        if (hero && mob && mob->isAlive()) {
            hero->attack(mob);
            FX::burst(mob->pos, 0x88ccffFF, 6);
        }
    }

    if (hero->getSprite() && hero->getSprite()->asHeroSprite()) {
        hero->getSprite()->asHeroSprite()->operate(hero->pos);
    }
    hero->busy();
    Sample::play(Assets::SND_ZAP);
    hero->spend(hero->attackDelay());
    hero->next();
    Actor::process();
    return true;
}

std::unique_ptr<Item> HuntressArmor::clone(int qty) const {
    auto a = std::make_unique<HuntressArmor>();
    a->STR = STR;
    a->DR_val = DR_val;
    a->level = level;
    a->levelKnown = levelKnown;
    a->cursed = cursed;
    a->cursedKnown = cursedKnown;
    a->identified = identified;
    a->glyph = glyph;
    a->quantity = qty;
    return a;
}

// =============================================================================
// Amulet Implementation
// =============================================================================
Amulet::Amulet() {
    name = "Amulet of Yendor";
    image = ItemSpriteSheet::AMULET;
    identified = true;
    levelKnown = true;
}

std::vector<std::string> Amulet::actions(Hero* hero) {
    (void)hero;
    return { "END GAME", "DROP" };
}

bool Amulet::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "END GAME") {
        GLog::h("You hold the Amulet of Yendor high! You have conquered the dungeon!");
        Sample::play(Assets::SND_BADGE);
        return true;
    }
    return Item::execute(hero, action);
}

std::string Amulet::info() const {
    return "The Amulet of Yendor is the most powerful known artifact. "
           "You can end the game now, or challenge yourself to ascend back to the surface!";
}

std::unique_ptr<Item> Amulet::clone(int qty) const {
    (void)qty;
    return std::make_unique<Amulet>();
}

static const char* s_gemNames[12] = {
    "diamond", "opal", "garnet", "ruby",
    "amethyst", "topaz", "onyx", "tourmaline",
    "emerald", "sapphire", "quartz", "agate"
};

static const char* s_ringEffects[12] = {
    "Mending", "Detection", "Shadows", "Power",
    "Herbalism", "Accuracy", "Evasion", "Satiety",
    "Haste", "Haggler", "Elements", "Thorns"
};

static const char* s_standardEffects[10] = {
    "Mending", "Detection", "Shadows", "Power",
    "Herbalism", "Accuracy", "Evasion", "Satiety",
    "Haste", "Elements"
};

static std::vector<int> s_ringGemMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static std::vector<bool> s_ringKnown(12, false);

void Ring::initGems() {
    s_ringGemMap = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int i = 11; i > 0; --i) {
        int j = Random::Int(i + 1);
        std::swap(s_ringGemMap[i], s_ringGemMap[j]);
    }
    std::fill(s_ringKnown.begin(), s_ringKnown.end(), false);
}

Ring::Gem Ring::gemForEffect(const std::string& eff) {
    for (int i = 0; i < 12; ++i) {
        if (eff == s_ringEffects[i]) {
            int gemIdx = s_ringGemMap[i];
            return static_cast<Gem>(gemIdx);
        }
    }
    return Gem::DIAMOND;
}

int Ring::imageForGem(Gem g) {
    switch (g) {
        case Gem::DIAMOND:    return ItemSpriteSheet::RING_DIAMOND;
        case Gem::OPAL:       return ItemSpriteSheet::RING_OPAL;
        case Gem::GARNET:     return ItemSpriteSheet::RING_GARNET;
        case Gem::RUBY:       return ItemSpriteSheet::RING_RUBY;
        case Gem::AMETHYST:   return ItemSpriteSheet::RING_AMETHYST;
        case Gem::TOPAZ:      return ItemSpriteSheet::RING_TOPAZ;
        case Gem::ONYX:       return ItemSpriteSheet::RING_ONYX;
        case Gem::TOURMALINE: return ItemSpriteSheet::RING_TOURMALINE;
        case Gem::EMERALD:    return ItemSpriteSheet::RING_EMERALD;
        case Gem::SAPPHIRE:   return ItemSpriteSheet::RING_SAPPHIRE;
        case Gem::QUARTZ:     return ItemSpriteSheet::RING_QUARTZ;
        case Gem::AGATE:      return ItemSpriteSheet::RING_AGATE;
        default:              return ItemSpriteSheet::RING;
    }
}

std::string Ring::gemName(Gem g) {
    size_t idx = static_cast<size_t>(g);
    return idx < 12 ? s_gemNames[idx] : "gem";
}

std::unique_ptr<Ring> Ring::createForEffect(const std::string& eff) {
    if (eff == "Accuracy") return std::make_unique<RingOfAccuracy>();
    if (eff == "Detection") return std::make_unique<RingOfDetection>();
    if (eff == "Elements") return std::make_unique<RingOfElements>();
    if (eff == "Evasion") return std::make_unique<RingOfEvasion>();
    if (eff == "Haggler") return std::make_unique<RingOfHaggler>();
    if (eff == "Haste") return std::make_unique<RingOfHaste>();
    if (eff == "Herbalism") return std::make_unique<RingOfHerbalism>();
    if (eff == "Mending") return std::make_unique<RingOfMending>();
    if (eff == "Power") return std::make_unique<RingOfPower>();
    if (eff == "Satiety") return std::make_unique<RingOfSatiety>();
    if (eff == "Shadows") return std::make_unique<RingOfShadows>();
    if (eff == "Thorns") return std::make_unique<RingOfThorns>();
    return std::make_unique<Ring>(gemForEffect(eff), eff);
}

bool Ring::isKnown(Gem g) {
    size_t idx = static_cast<size_t>(g);
    return idx < 12 ? s_ringKnown[idx] : false;
}

void Ring::setKnown(Gem g) {
    size_t idx = static_cast<size_t>(g);
    if (idx >= 12) return;
    s_ringKnown[idx] = true;

    // 11 out of 12 deduction rule
    int knownCount = 0;
    int unknownIdx = -1;
    for (size_t i = 0; i < 12; ++i) {
        if (s_ringKnown[i]) {
            knownCount++;
        } else {
            unknownIdx = static_cast<int>(i);
        }
    }
    if (knownCount == 11 && unknownIdx >= 0) {
        s_ringKnown[unknownIdx] = true;
    }
}

bool Ring::allKnown() {
    int knownCount = 0;
    for (size_t i = 0; i < 12; ++i) {
        if (s_ringKnown[i]) knownCount++;
    }
    return knownCount >= 10;
}

const std::vector<bool>& Ring::getKnownTable() {
    return s_ringKnown;
}

void Ring::restoreStatus(const std::vector<bool>& known) {
    if (known.size() >= 12) {
        s_ringKnown = known;
    }
}

bool Ring::isKnown() const {
    return isKnown(gem);
}

void Ring::setKnown() {
    setKnown(gem);
}

std::string Ring::gemName() const {
    return gemName(gem);
}

bool Ring::isUpgradable() const {
    return effect != "Haggler" && effect != "Thorns";
}

Ring::Ring(Gem g, std::string eff)
    : gem(g), effect(std::move(eff)) {
    name = "Ring of " + effect;
    image = imageForGem(gem);
}

Item* Ring::identify() {
    setKnown();
    ticksToKnow = 0;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
    Badges::validateItemLevelAcquired(this);
    Badges::validateAllRingsIdentified();
    return this;
}

Item* Ring::random() {
    int r = Random::Int(10);
    effect = s_standardEffects[r];
    gem = gemForEffect(effect);
    name = "Ring of " + effect;
    image = imageForGem(gem);

    identified = false;
    levelKnown = false;
    cursedKnown = false;
    ticksToKnow = TICKS_TO_KNOW;

    int lvl = Random::Int(1, 3);
    if (Random::Float() < 0.30f) {
        level = -lvl;
        cursed = true;
    } else {
        level = lvl;
        cursed = false;
    }
    return this;
}

void Ring::onCollect(Hero* hero) {
    (void)hero;
}

std::vector<std::string> Ring::actions(Hero* hero) {
    if (hero && (hero->belongings.equippedRing1 == this || hero->belongings.equippedRing2 == this)) {
        return { "UNEQUIP", "DROP", "THROW" };
    }
    return { "EQUIP", "DROP", "THROW" };
}

bool Ring::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "EQUIP") {
        if (hero->belongings.equippedRing1 && hero->belongings.equippedRing2) {
            if (hero->belongings.equippedRing1->isCursed() && hero->belongings.equippedRing2->isCursed()) {
                GLog::w("You can't remove your cursed rings!");
                return false;
            }
        }
        if (hero->belongings.equip(this)) {
            cursedKnown = true;
            if (hero->heroClass == Hero::HeroClass::Rogue && !isKnown()) {
                setKnown();
                GLog::i("This is a " + displayName());
                Badges::validateItemLevelAcquired(this);
            }
            if (cursed) {
                FX::burst(hero->pos, 0x550055FFu, 8);
                Sample::play(Assets::SND_CURSED);
                GLog::n("The " + displayName() + " is cursed! You cannot take it off!");
            } else {
                GLog::i("You put on the " + displayName() + ".");
            }
            if (effect == "Detection" && Dungeon::level) {
                hero->search(Dungeon::level.get(), false);
            }
            hero->spendAndNext(Actor::TICK);
            return true;
        }
        return false;
    } else if (action == "UNEQUIP") {
        if (cursed && (hero->belongings.equippedRing1 == this || hero->belongings.equippedRing2 == this)) {
            GLog::w("You can't remove cursed " + displayName() + "!");
            return false;
        }
        if (hero->belongings.isFull()) {
            GLog::w("Your backpack is too full to unequip that!");
            return false;
        }
        hero->belongings.unequip(this);
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "DROP") {
        if (hero->belongings.equippedRing1 == this || hero->belongings.equippedRing2 == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
        }
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        if (hero->belongings.equippedRing1 == this || hero->belongings.equippedRing2 == this) {
            if (cursed) {
                GLog::w("You can't remove cursed " + displayName() + "!");
                return false;
            }
        }
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Ring::displayName() const {
    std::string s = isKnown() ? name : (gemName() + " ring");
    if (levelKnown) {
        if (cursed && cursedKnown) s = "cursed " + s;
        if (level > 0) s += " +" + std::to_string(level);
        else if (level < 0) s += " " + std::to_string(level);
    } else if (cursed && cursedKnown) {
        s = "cursed " + s;
    }
    return s;
}

std::string Ring::desc() const {
    if (!isKnown()) {
        return "This metal band is adorned with a large " + gemName() + " gem that glitters in the darkness. Who knows what effect it has when worn?";
    }
    if (effect == "Accuracy") {
        return "This ring increases your chance to hit the enemy.";
    } else if (effect == "Detection") {
        return "Wearing this ring will allow the wearer to notice hidden secrets - traps and secret doors - without taking time to search. Degraded rings of detection will dull your senses, making it harder to notice secrets even when actively searching for them.";
    } else if (effect == "Elements") {
        return "This ring provides resistance to different elements, such as fire, electricity, gases etc. Also it decreases duration of negative effects.";
    } else if (effect == "Evasion") {
        return "This ring increases your chance to dodge enemy attack.";
    } else if (effect == "Haggler") {
        return "In fact this ring doesn't provide any magic effect, but it demonstrates to shopkeepers and vendors, that the owner of the ring is a member of The Thieves' Guild. Usually they are glad to give a discount in exchange for temporary immunity guarantee. Upgrading this ring won't give any additional bonuses.";
    } else if (effect == "Haste") {
        return "This ring accelerates the wearer's flow of time, allowing one to perform all actions a little faster.";
    } else if (effect == "Herbalism") {
        return "This ring increases your chance to gather dew and seeds from trampled grass.";
    } else if (effect == "Mending") {
        return "This ring increases the body's regenerative properties, allowing one to recover lost health at an accelerated rate. Degraded rings will decrease or even halt one's natural regeneration.";
    } else if (effect == "Power") {
        return "Your wands will become more powerful in the energy field that radiates from this ring. Degraded rings of power will instead weaken your wands.";
    } else if (effect == "Satiety") {
        return "Wearing this ring you can go without food longer. Degraded rings of satiety will cause the opposite effect.";
    } else if (effect == "Shadows") {
        return "Enemies will be less likely to notice you if you wear this ring. Degraded rings of shadows will alert enemies who might otherwise not have noticed your presence.";
    } else if (effect == "Thorns") {
        return "Though this ring doesn't provide real thorns, an enemy that attacks you will itself be wounded by a fraction of the damage that it inflicts. Upgrading this ring won't give any additional bonuses.";
    }
    return "This metal band is adorned with a large " + gemName() + " gem that glitters in the darkness. Who knows what effect it has when worn?";
}

std::string Ring::info() const {
    if (Dungeon::hero && isEquipped(Dungeon::hero.get())) {
        return desc() + "\n\nThe " + displayName() + " is on your finger" +
               (cursed ? ", and because it is cursed, you are powerless to remove it." : ".");
    } else if (cursed && cursedKnown) {
        return desc() + "\n\nYou can feel a malevolent magic lurking within the " + displayName() + ".";
    } else {
        return desc();
    }
}

std::unique_ptr<Item> Ring::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<Ring>(gem, effect);
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

int Ring::price() const {
    return considerState(80);
}

// -----------------------------------------------------------------------------
// The 12 Canonical Ring Subclasses
// -----------------------------------------------------------------------------
RingOfAccuracy::RingOfAccuracy() : Ring(gemForEffect("Accuracy"), "Accuracy") {}
std::unique_ptr<Item> RingOfAccuracy::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfAccuracy>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfDetection::RingOfDetection() : Ring(gemForEffect("Detection"), "Detection") {}
std::unique_ptr<Item> RingOfDetection::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfDetection>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfElements::RingOfElements() : Ring(gemForEffect("Elements"), "Elements") {}
std::unique_ptr<Item> RingOfElements::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfElements>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfEvasion::RingOfEvasion() : Ring(gemForEffect("Evasion"), "Evasion") {}
std::unique_ptr<Item> RingOfEvasion::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfEvasion>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfHaggler::RingOfHaggler() : Ring(gemForEffect("Haggler"), "Haggler") {
    level = 1;
}
Item* RingOfHaggler::random() {
    level = 1;
    cursed = false;
    return this;
}
void RingOfHaggler::onCollect(Hero* hero) {
    (void)hero;
    identify();
    Badges::validateRingOfHaggler();
    Badges::validateItemLevelAcquired(this);
}
std::unique_ptr<Item> RingOfHaggler::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfHaggler>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfHaste::RingOfHaste() : Ring(gemForEffect("Haste"), "Haste") {}
std::unique_ptr<Item> RingOfHaste::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfHaste>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfHerbalism::RingOfHerbalism() : Ring(gemForEffect("Herbalism"), "Herbalism") {}
std::unique_ptr<Item> RingOfHerbalism::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfHerbalism>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfMending::RingOfMending() : Ring(gemForEffect("Mending"), "Mending") {}
std::unique_ptr<Item> RingOfMending::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfMending>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfPower::RingOfPower() : Ring(gemForEffect("Power"), "Power") {}
std::unique_ptr<Item> RingOfPower::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfPower>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfSatiety::RingOfSatiety() : Ring(gemForEffect("Satiety"), "Satiety") {}
std::unique_ptr<Item> RingOfSatiety::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfSatiety>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfShadows::RingOfShadows() : Ring(gemForEffect("Shadows"), "Shadows") {}
std::unique_ptr<Item> RingOfShadows::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfShadows>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

RingOfThorns::RingOfThorns() : Ring(gemForEffect("Thorns"), "Thorns") {
    level = 1;
}
Item* RingOfThorns::random() {
    level = 1;
    cursed = false;
    return this;
}
void RingOfThorns::onCollect(Hero* hero) {
    (void)hero;
    identify();
    Badges::validateRingOfThorns();
    Badges::validateItemLevelAcquired(this);
}
std::unique_ptr<Item> RingOfThorns::clone(int qty) const {
    (void)qty;
    auto r = std::make_unique<RingOfThorns>();
    r->gem = gem;
    r->image = image;
    r->level = level;
    r->cursed = cursed;
    r->identified = identified;
    r->levelKnown = levelKnown;
    r->cursedKnown = cursedKnown;
    r->ticksToKnow = ticksToKnow;
    return r;
}

// =============================================================================
// DewVial Implementation
// =============================================================================
DewVial::DewVial() {
    name = "Dew Vial";
    image = ItemSpriteSheet::VIAL;
    volume = 0;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
    defaultAction = "DRINK";
}

void DewVial::collectDew(int count) {
    volume = std::min(MAX_VOLUME, volume + count);
    if (volume >= MAX_VOLUME) {
        volume = MAX_VOLUME;
        GLog::p("Your dew vial is full!");
    } else {
        GLog::i("Your dew vial collected a drop of dew (" + std::to_string(volume) + "/10)");
    }
}

std::vector<std::string> DewVial::actions(Hero* hero) {
    (void)hero;
    std::vector<std::string> acts;
    if (volume > 0) acts.push_back("DRINK");
    acts.push_back("DROP");
    acts.push_back("THROW");
    return acts;
}

bool DewVial::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "DRINK") {
        if (volume > 0) {
            Sample::play(Assets::SND_DRINK);
            int value = static_cast<int>(std::ceil(std::pow(volume, std::log10(20.0)) / 20.0 * hero->HT));
            int effect = std::min(hero->HT - hero->HP, value);
            if (effect > 0) {
                hero->HP += effect;
                FX::burst(hero->pos, 0x00ff88FFu, volume > 5 ? 12 : 6);
                FX::floatingText(hero->pos, "+" + std::to_string(effect), 0x38b764FF);
                GLog::p("You drink the pure dew from the vial (+" + std::to_string(effect) + " HP).");
            } else {
                GLog::i("You drink the pure dew from the vial.");
            }
            volume = 0;
            hero->spendAndNext(Actor::TICK);
            return true;
        } else {
            GLog::w("Your dew vial is empty!");
            return false;
        }
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

void DewVial::autoDrink(Hero* hero) {
    if (!hero) return;
    DewVial* vial = nullptr;
    for (auto& it : hero->belongings.backpack) {
        if (it && it->asDewVial()) {
            vial = it->asDewVial();
            break;
        }
    }
    if (vial && vial->isFull()) {
        vial->volume = 0;
        hero->HP = hero->HT;
        Sample::play(Assets::SND_DRINK);
        FX::burst(hero->pos, 0x00ff88FFu, 16);
        FX::floatingText(hero->pos, "+" + std::to_string(hero->HT), 0x38b764FF);
        GLog::p("The dew vial bursts with life-giving liquid, restoring you from the brink of death!");
    }
}

std::string DewVial::displayName() const {
    return "Dew Vial (" + std::to_string(volume) + "/" + std::to_string(MAX_VOLUME) + ")";
}

std::string DewVial::info() const {
    return "You can store excess dew in this tiny vessel for drinking it later. If the vial is full, in a moment of deadly peril the dew will be consumed automatically.";
}

std::unique_ptr<Item> DewVial::clone(int qty) const {
    (void)qty;
    auto dv = std::make_unique<DewVial>();
    dv->volume = volume;
    return dv;
}

// =============================================================================
// Bomb Implementation
// =============================================================================
Bomb::Bomb() {
    name = "Bomb";
    image = ItemSpriteSheet::BOMB;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
    defaultAction = "THROW";
}

Item* Bomb::random() {
    quantity = Random::IntRange(1, 2);
    return this;
}

std::vector<std::string> Bomb::actions(Hero* hero) {
    (void)hero;
    return { "THROW", "DROP" };
}

bool Bomb::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "THROW") {
        return doThrowItem(hero, this);
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    }
    return false;
}

void Bomb::onThrow(int cell) {
    if (Dungeon::level && Dungeon::level->pit[cell]) {
        Item::onThrow(cell);
        return;
    }
    explode(cell);
}

void Bomb::explode(int cell) {
    if (!Dungeon::level) return;
    Sample::playAt(Assets::SND_BLAST, cell, 1.0f, 1.0f);
    FX::burst(cell, 0xff8800FFu, 24);

    for (int n : Level::NEIGHBOURS9) {
        int c = cell + n;
        if (c < 0 || c >= Level::LENGTH) continue;

        if (Dungeon::level->isFlamable(c)) {
            Dungeon::level->destroy(c);
        }

        Char* ch = Actor::findChar(c);
        if (ch) {
            int dmg = Random::IntRange(1 + Dungeon::depth, 10 + Dungeon::depth * 2);
            ch->damage(dmg, nullptr);
            FX::floatingText(c, std::to_string(dmg), 0xff4444FF);
            if (ch->isAlive()) {
                ch->affect<Paralysis>();
            }
        }

        // Check for honeypot on the ground damaged by explosion
        Heap* h = Dungeon::level->getHeap(c);
        if (h && h->type == Heap::Type::HEAP) {
            for (auto it = h->items.begin(); it != h->items.end();) {
                if ((*it)->name == "honeypot" || (*it)->name == "Honeypot") {
                    auto hp = std::move(*it);
                    it = h->items.erase(it);
                    static_cast<Honeypot*>(hp.get())->shatter(c);
                } else {
                    ++it;
                }
            }
            if (h->isEmpty()) {
                Dungeon::level->removeHeap(h);
            }
        }
    }
    Dungeon::observe();
}

std::string Bomb::info() const {
    return "A potent explosive device with a short fuse. Throw it to blast a 3x3 area, destroying terrain and damaging enemies.";
}

std::unique_ptr<Item> Bomb::clone(int qty) const {
    auto b = std::make_unique<Bomb>();
    b->quantity = qty;
    return b;
}

// =============================================================================
// Honeypot Implementation
// =============================================================================
Honeypot::Honeypot() {
    name = "honeypot";
    image = ItemSpriteSheet::HONEYPOT;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
    defaultAction = "THROW";
}

std::vector<std::string> Honeypot::actions(Hero* hero) {
    (void)hero;
    return { "SHATTER", "THROW", "DROP" };
}

bool Honeypot::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "SHATTER") {
        shatter(hero->pos);
        for (auto it = hero->belongings.backpack.begin(); it != hero->belongings.backpack.end(); ++it) {
            if (it->get() == this) {
                quantity--;
                if (quantity <= 0) {
                    hero->belongings.backpack.erase(it);
                }
                break;
            }
        }
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    }
    return false;
}

void Honeypot::onThrow(int cell) {
    if (Dungeon::level && Dungeon::level->pit[cell]) {
        Item::onThrow(cell);
        return;
    }
    shatter(cell);
}

void Honeypot::shatter(int cell) {
    Sample::playAt(Assets::SND_SHATTER, cell);
    Sample::playAt(Assets::SND_BEE, cell);
    FX::splash(cell, 0xffd500FF, 5);
    FX::burst(cell, 0xffd500FFu, 8);

    int newPos = cell;
    if (Actor::findChar(cell) != nullptr) {
        std::vector<int> candidates;
        if (Dungeon::level) {
            for (int n : Level::NEIGHBOURS4) {
                int c = cell + n;
                if (c >= 0 && c < Level::LENGTH && Dungeon::level->isPassable(c) && Actor::findChar(c) == nullptr) {
                    candidates.push_back(c);
                }
            }
        }
        newPos = !candidates.empty() ? candidates[Random::Int(static_cast<int>(candidates.size()))] : -1;
    }

    if (newPos != -1) {
        auto bee = std::make_unique<Bee>();
        bee->spawn(Dungeon::depth);
        bee->place(newPos);
        bee->state = Mob::State::WANDERING;
        Actor::occupyCell(bee.get());
        Actor::add(bee.get(), 1.0f);
        Dungeon::mobs.push_back(std::move(bee));
        GLog::p("The honeypot shatters and a golden bee emerges!");
    } else {
        GLog::w("The honeypot shatters, but there was no room for the bee to emerge!");
    }
}

std::string Honeypot::info() const {
    return "There is not much honey in this small honeypot, but there is a golden bee there and it doesn't want to leave it.";
}

std::unique_ptr<Item> Honeypot::clone(int qty) const {
    auto hp = std::make_unique<Honeypot>();
    hp->quantity = qty;
    return hp;
}

// =============================================================================
// MysteryMeat Implementation
// =============================================================================
MysteryMeat::MysteryMeat() {
    name = "Mystery Meat";
    image = ItemSpriteSheet::MEAT;
    energy = static_cast<float>(Hunger::STARVING - Hunger::HUNGRY);
    message = "That food tasted... strange.";
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

bool MysteryMeat::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "EAT") {
        Food::execute(hero, action);
        switch (Random::Int(5)) {
            case 0:
                GLog::w("Oh it's hot!");
                hero->affect<Burning>(5);
                break;
            case 1:
                GLog::w("You can't feel your legs!");
                hero->affect<Roots>();
                break;
            case 2:
                GLog::w("You are not feeling well.");
                hero->affect<Poison>(std::max(1, hero->HT / 5));
                break;
            case 3:
                GLog::w("You are stuffed.");
                hero->affect<Slow>();
                break;
            case 4:
                break;
        }
        return true;
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

std::string MysteryMeat::info() const {
    return "Eat at your own risk!";
}

std::unique_ptr<Item> MysteryMeat::clone(int qty) const {
    auto m = std::make_unique<MysteryMeat>();
    m->quantity = qty;
    return m;
}

// =============================================================================
// ChargrilledMeat Implementation
// =============================================================================
ChargrilledMeat::ChargrilledMeat() {
    name = "Chargrilled Meat";
    image = ItemSpriteSheet::STEAK;
    energy = static_cast<float>(Hunger::STARVING - Hunger::HUNGRY);
    message = "That food tasted delicious!";
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::string ChargrilledMeat::info() const {
    return "It looks like a decent steak.";
}

std::unique_ptr<Item> ChargrilledMeat::clone(int qty) const {
    auto m = std::make_unique<ChargrilledMeat>();
    m->quantity = qty;
    return m;
}

std::unique_ptr<ChargrilledMeat> ChargrilledMeat::cook(MysteryMeat* ingredient) {
    auto result = std::make_unique<ChargrilledMeat>();
    if (ingredient) {
        result->quantity = ingredient->quantity;
    }
    return result;
}

// =============================================================================
// FrozenCarpaccio Implementation
// =============================================================================
FrozenCarpaccio::FrozenCarpaccio() {
    name = "Frozen Carpaccio";
    image = ItemSpriteSheet::CARPACCIO;
    energy = static_cast<float>(Hunger::STARVING - Hunger::HUNGRY);
    message = "That food tasted delicious!";
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

bool FrozenCarpaccio::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "EAT") {
        Food::execute(hero, action);
        switch (Random::Int(5)) {
            case 0:
                GLog::i("You see your hands turn invisible!");
                hero->affect<Invisibility>();
                FX::floatingText(hero->pos, "INVISIBLE", 0xa5cde4FF);
                FX::burst(hero->pos, 0xa5cde4FF, 10);
                Sample::play(Assets::SND_MELD);
                break;
            case 1:
                GLog::i("You feel your skin hardens!");
                hero->affect<Barkskin>(std::max(1, hero->HT / 4));
                FX::floatingText(hero->pos, "BARKSKIN", 0x8b5a2bFF);
                FX::burst(hero->pos, 0x8b5a2bFF, 8);
                break;
            case 2:
                GLog::i("Refreshing!");
                hero->removeBuff(BuffType::Poison);
                hero->removeBuff(BuffType::Cripple);
                hero->removeBuff(BuffType::Weakness);
                hero->removeBuff(BuffType::Bleeding);
                FX::floatingText(hero->pos, "REFRESHED", 0x38b764FF);
                FX::burst(hero->pos, 0x38b764FF, 8);
                break;
            case 3:
                GLog::i("You feel better!");
                if (hero->HP < hero->HT) {
                    int heal = std::min(hero->HT / 4, hero->HT - hero->HP);
                    hero->HP += heal;
                    FX::floatingText(hero->pos, "+" + std::to_string(heal), 0x38b764FF);
                    FX::burst(hero->pos, 0x38b764FF, 8);
                }
                break;
            case 4:
                break;
        }
        return true;
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

std::string FrozenCarpaccio::info() const {
    return "It's a piece of frozen raw meat. The only way to eat it is by cutting thin slices of it. And this way it's surprisingly good.";
}

std::unique_ptr<Item> FrozenCarpaccio::clone(int qty) const {
    auto m = std::make_unique<FrozenCarpaccio>();
    m->quantity = qty;
    return m;
}

std::unique_ptr<FrozenCarpaccio> FrozenCarpaccio::cook(MysteryMeat* ingredient) {
    auto result = std::make_unique<FrozenCarpaccio>();
    if (ingredient) {
        result->quantity = ingredient->quantity;
    }
    return result;
}

// -----------------------------------------------------------------------------
// OverpricedRation
// -----------------------------------------------------------------------------
OverpricedRation::OverpricedRation() {
    name = "overpriced food ration";
    image = ItemSpriteSheet::OVERPRICED;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::string OverpricedRation::info() const {
    return "It looks exactly like a standard ration of food but smaller.";
}

std::unique_ptr<Item> OverpricedRation::clone(int qty) const {
    auto r = std::make_unique<OverpricedRation>();
    r->quantity = qty;
    return r;
}

// -----------------------------------------------------------------------------
// Ankh
// -----------------------------------------------------------------------------
Ankh::Ankh() {
    name = "Ankh";
    image = ItemSpriteSheet::ANKH;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::string Ankh::info() const {
    return "The ancient symbol of immortality grants an ability to return to life after death. Upon resurrection all non-equipped items are lost.";
}

std::unique_ptr<Item> Ankh::clone(int qty) const {
    auto a = std::make_unique<Ankh>();
    a->quantity = qty;
    return a;
}

// -----------------------------------------------------------------------------
// Weightstone
// -----------------------------------------------------------------------------
Weightstone::Weightstone() {
    name = "weightstone";
    image = ItemSpriteSheet::WEIGHT;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::vector<std::string> Weightstone::actions(Hero* hero) {
    (void)hero;
    return { "APPLY", "DROP", "THROW" };
}

bool Weightstone::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "APPLY") {
        if (hero->belongings.equippedWeapon) {
            Weapon* w = hero->belongings.equippedWeapon->asWeapon();
            if (w) {
                w->imbue = (w->imbue == Weapon::Imbue::SPEED) ? Weapon::Imbue::ACCURACY : Weapon::Imbue::SPEED;
                GLog::p("You balanced your " + w->displayName() + (w->imbue == Weapon::Imbue::SPEED ? " for speed." : " for accuracy."));
                Sample::play(Assets::SND_MISS);
                quantity--;
                if (quantity <= 0) {
                    hero->belongings.backpack.erase(
                        std::remove_if(hero->belongings.backpack.begin(), hero->belongings.backpack.end(),
                                       [this](const std::unique_ptr<Item>& it) { return it.get() == this; }),
                        hero->belongings.backpack.end());
                }
                hero->spendAndNext(2.0f * Actor::TICK);
                return true;
            }
        }
        GLog::w("You must equip a melee weapon first!");
        return false;
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    } else if (action == "THROW") {
        return doThrowItem(hero, this);
    }
    return false;
}

std::string Weightstone::info() const {
    return "Using a weightstone, you can balance your melee weapon to increase its speed or accuracy.";
}

std::unique_ptr<Item> Weightstone::clone(int qty) const {
    auto w = std::make_unique<Weightstone>();
    w->quantity = qty;
    return w;
}

// -----------------------------------------------------------------------------
// Bags
// -----------------------------------------------------------------------------
SeedPouch::SeedPouch() {
    name = "seed pouch";
    image = ItemSpriteSheet::POUCH;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::string SeedPouch::info() const {
    return "This small velvet pouch allows you to store any number of seeds in it. Very convenient.";
}

std::unique_ptr<Item> SeedPouch::clone(int qty) const {
    auto b = std::make_unique<SeedPouch>();
    b->quantity = qty;
    return b;
}

ScrollHolder::ScrollHolder() {
    name = "scroll holder";
    image = ItemSpriteSheet::HOLDER;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::string ScrollHolder::info() const {
    return "You can place any number of scrolls into this tubular container. It saves room in your backpack and protects scrolls from fire.";
}

std::unique_ptr<Item> ScrollHolder::clone(int qty) const {
    auto b = std::make_unique<ScrollHolder>();
    b->quantity = qty;
    return b;
}

WandHolster::WandHolster() {
    name = "wand holster";
    image = ItemSpriteSheet::HOLSTER;
    quantity = 1;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

std::string WandHolster::info() const {
    return "A leather holster for carrying magical wands. Keeps them safe and within easy reach.";
}

std::unique_ptr<Item> WandHolster::clone(int qty) const {
    auto b = std::make_unique<WandHolster>();
    b->quantity = qty;
    return b;
}

// =============================================================================
// Keyring
// =============================================================================
Keyring::Keyring() {
    name = "Key Ring";
    image = ItemSpriteSheet::KEYRING;
    quantity = 1;
    unique = true;
    identified = true;
    levelKnown = true;
    cursedKnown = true;
}

bool Keyring::grab(const Item* item) const {
    return item != nullptr && const_cast<Item*>(item)->asKey() != nullptr;
}

bool Keyring::addKey(std::unique_ptr<Key> key) {
    if (!key) return false;
    for (auto& k : keys) {
        if (k && k->name == key->name && k->depth == key->depth && k->isStackable()) {
            k->quantity += key->quantity;
            return true;
        }
    }
    if (keys.size() >= KEYRING_SIZE) return false;
    keys.push_back(std::move(key));
    std::sort(keys.begin(), keys.end(), [](const std::unique_ptr<Key>& a, const std::unique_ptr<Key>& b) {
        if (!a || !b) return a != nullptr;
        if (a->depth != b->depth) return a->depth < b->depth;
        return a->name < b->name;
    });
    return true;
}

Key* Keyring::getKey(const std::string& keyName, int depth) const {
    for (const auto& k : keys) {
        if (k && k->name == keyName) {
            if (depth <= 0 || k->depth == depth) {
                return k.get();
            }
        }
    }
    return nullptr;
}

bool Keyring::consumeKey(const std::string& keyName, int depth) {
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        if (*it && (*it)->name == keyName) {
            if (depth <= 0 || (*it)->depth == depth) {
                (*it)->quantity--;
                if ((*it)->quantity <= 0) {
                    keys.erase(it);
                }
                return true;
            }
        }
    }
    return false;
}

std::unique_ptr<Key> Keyring::removeKey(Key* key) {
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        if (it->get() == key) {
            std::unique_ptr<Key> removed = std::move(*it);
            keys.erase(it);
            return removed;
        }
    }
    return nullptr;
}

std::vector<std::string> Keyring::actions(Hero* hero) {
    (void)hero;
    return {};
}

bool Keyring::execute(Hero* hero, const std::string& action) {
    (void)hero;
    (void)action;
    return false;
}

std::string Keyring::info() const {
    return "This is a copper key ring, that lets you keep all your keys "
           "separately from the rest of your belongings.";
}

std::unique_ptr<Item> Keyring::clone(int qty) const {
    (void)qty;
    auto kr = std::make_unique<Keyring>();
    for (const auto& k : keys) {
        if (k) {
            auto cloned = k->clone(k->quantity);
            if (cloned && cloned->asKey()) {
                kr->keys.push_back(std::unique_ptr<Key>(static_cast<Key*>(cloned.release())));
            }
        }
    }
    return kr;
}

} // namespace pd
