#include "pd/Item.h"
#include "pd/Hero.h"
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

static bool doThrowItem(Hero* hero, Item* item) {
    if (!hero || !item) return false;
    std::string itemName = item->displayName();
    std::unique_ptr<Item> thrown = nullptr;

    if (item->quantity > 1) {
        item->quantity--;
        if (item->category() == ItemCategory::Potion) {
            thrown = std::make_unique<Potion>(static_cast<Potion*>(item)->type);
        } else if (item->category() == ItemCategory::Scroll) {
            thrown = std::make_unique<Scroll>(static_cast<Scroll*>(item)->type);
        } else if (item->name == "Dewdrop") {
            thrown = std::make_unique<Dewdrop>();
        } else {
            thrown = std::make_unique<Food>();
        }
    } else {
        thrown = hero->belongings.drop(item);
    }

    if (thrown && Dungeon::level) {
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
                if (Dungeon::level->isPassable(n)) {
                    targetCell = n;
                    break;
                }
            }
        }

        float fx = (hero->pos % Level::WIDTH) * 16.0f + 8.0f;
        float fy = (hero->pos / Level::WIDTH) * 16.0f + 8.0f;
        float tx = (targetCell % Level::WIDTH) * 16.0f + 8.0f;
        float ty = (targetCell / Level::WIDTH) * 16.0f + 8.0f;
        FX::burst((fx + tx) * 0.5f, (fy + ty) * 0.5f, 0xffffffFF, 4);

        if (nearestEnemy && targetCell == nearestEnemy->pos) {
            int dmg = Random::IntRange(1, 4);
            nearestEnemy->damage(dmg, hero);
            FX::floatingText(targetCell, std::to_string(dmg), 0xe43b44FF);
            GLog::p("You hit " + nearestEnemy->name + " with " + itemName + " for " + std::to_string(dmg) + " dmg!");
        }

        if (thrown->category() == ItemCategory::Potion) {
            Potion* pot = static_cast<Potion*>(thrown.get());
            if (pot->type == Potion::PotionType::TOXIC_GAS ||
                pot->type == Potion::PotionType::LIQUID_FLAME ||
                pot->type == Potion::PotionType::FROST ||
                pot->type == Potion::PotionType::PARALYTIC_GAS) {

                if (Dungeon::visible[targetCell]) {
                    Potion::setKnown(pot->type);
                }

                if (pot->type == Potion::PotionType::TOXIC_GAS) {
                    FX::burst(targetCell, 0x00ff00FF, 16);
                    GLog::i("The flask shatters and a cloud of toxic gas billows out!");
                    if (nearestEnemy && targetCell == nearestEnemy->pos) {
                        nearestEnemy->damage(Random::IntRange(4, 10), hero);
                    }
                } else if (pot->type == Potion::PotionType::LIQUID_FLAME) {
                    FX::burst(targetCell, 0xff4500FF, 16);
                    GLog::i("The flask shatters and liquid flame bursts into a roaring inferno!");
                    if (nearestEnemy && targetCell == nearestEnemy->pos) {
                        nearestEnemy->damage(Random::IntRange(6, 14), hero);
                    }
                } else if (pot->type == Potion::PotionType::FROST) {
                    FX::burst(targetCell, 0x00ffffff, 16);
                    GLog::i("The flask shatters and freezing frost encases the area!");
                    if (nearestEnemy && targetCell == nearestEnemy->pos) {
                        nearestEnemy->damage(Random::IntRange(3, 8), hero);
                    }
                } else if (pot->type == Potion::PotionType::PARALYTIC_GAS) {
                    FX::burst(targetCell, 0xffff00FF, 16);
                    GLog::i("The flask shatters into a cloud of paralyzing vapor!");
                    if (nearestEnemy && targetCell == nearestEnemy->pos) {
                        nearestEnemy->spend(4.0f * Actor::TICK);
                    }
                }
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_SHATTER).get());
            } else {
                if (Dungeon::visible[targetCell]) {
                    GLog::i("The flask shatters and " + Potion::colorName(pot->type) + " liquid splashes harmlessly.");
                    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_SHATTER).get());
                    FX::burst(targetCell, 0x88ccffFF, 6);
                }
            }
            hero->spendAndNext(Actor::TICK);
            return true;
        }

        Dungeon::level->drop(std::move(thrown), targetCell);
        GLog::i("You threw " + itemName);
    }

    hero->spendAndNext(Actor::TICK);
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
    else image = ItemSpriteSheet::WEAPON;
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

int Weapon::damageRoll() const {
    return Random::IntRange(minDamage, maxDamage) + level;
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
        default: image = ItemSpriteSheet::ARMOR; break;
    }
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
                FX::floatingText(hero->pos, "HEALED", 0x38b764FF);
                FX::burst(hero->pos, 0x38b764FF, 12);
                GLog::p("Your wounds heal completely.");
                break;
            }
            case PotionType::STRENGTH:
                hero->STR += 1;
                FX::floatingText(hero->pos, "+1 STR", 0xe4a672FF);
                FX::burst(hero->pos, 0xe4a672FF, 8);
                GLog::p("You feel stronger now!");
                break;
            case PotionType::INVISIBILITY:
                hero->invisible = 15;
                FX::floatingText(hero->pos, "INVISIBLE", 0xa5cde4FF);
                GLog::i("You vanish from sight!");
                break;
            case PotionType::EXPERIENCE:
                hero->earnExp(hero->maxExp - hero->exp);
                FX::burst(hero->pos, 0xffd700FF, 10);
                GLog::p("You feel more experienced!");
                break;
            case PotionType::TOXIC_GAS: {
                FX::burst(hero->pos, 0x00ff00FF, 16);
                hero->damage(Random::IntRange(3, 8), hero);
                GLog::n("You choke on the noxious fumes!");
                break;
            }
            case PotionType::LIQUID_FLAME: {
                FX::burst(hero->pos, 0xff4500FF, 16);
                hero->damage(Random::IntRange(4, 10), hero);
                GLog::n("The liquid burns your throat and sets you ablaze!");
                break;
            }
            case PotionType::PARALYTIC_GAS: {
                FX::burst(hero->pos, 0xffff00FF, 12);
                hero->spend(5.0f * Actor::TICK);
                GLog::n("You are paralyzed and unable to move!");
                break;
            }
            case PotionType::FROST: {
                FX::burst(hero->pos, 0x00ffffff, 12);
                hero->damage(Random::IntRange(2, 6), hero);
                GLog::n("Ice crystals freeze your innards!");
                break;
            }
            case PotionType::LEVITATION: {
                FX::burst(hero->pos, 0xdda0ddFF, 8);
                GLog::p("You begin floating gently in the air!");
                break;
            }
            case PotionType::MIND_VISION: {
                FX::burst(hero->pos, 0x9955ffFF, 12);
                for (const auto& m : Dungeon::mobs) {
                    if (m && m->isAlive()) {
                        Dungeon::visible[m->pos] = true;
                    }
                }
                Dungeon::observe();
                GLog::p("You can sense the minds of creatures on this floor!");
                break;
            }
            case PotionType::PURITY: {
                FX::burst(hero->pos, 0xffffffFF, 12);
                GLog::p("A pure aura washes away all impurities.");
                break;
            }
            case PotionType::MIGHT: {
                hero->STR += 1;
                hero->HT += 5;
                hero->HP = std::min(hero->HT, hero->HP + 5);
                FX::burst(hero->pos, 0xffd700FF, 12);
                GLog::p("You feel a surge of extraordinary might!");
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

std::string Potion::info() const {
    if (isKnown(type)) {
        return trueDesc(type);
    } else {
        return "This flask contains a swirling " + colorName(type) + " liquid. Who knows what it will do when drunk or thrown?";
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
            case ScrollType::IDENTIFY:
                break;
            case ScrollType::UPGRADE:
                if (hero->belongings.equippedWeapon) {
                    hero->belongings.equippedWeapon->upgrade();
                    FX::burst(hero->pos, 0xffd700FF, 10);
                    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                } else if (hero->belongings.equippedArmor) {
                    hero->belongings.equippedArmor->upgrade();
                    FX::burst(hero->pos, 0xffd700FF, 10);
                    FX::floatingText(hero->pos, "+1 LVL", 0xffd700FF);
                }
                break;
            case ScrollType::TELEPORTATION: {
                if (Dungeon::level) {
                    for (int tries = 0; tries < 100; ++tries) {
                        int cell = Random::Int(Level::LENGTH);
                        if (Dungeon::level->isPassable(cell) && !Actor::findChar(cell)) {
                            FX::burst(hero->pos, 0x9955ffFF, 8);
                            hero->place(cell);
                            FX::burst(hero->pos, 0x9955ffFF, 8);
                            Dungeon::observe();
                            break;
                        }
                    }
                }
                break;
            }
            case ScrollType::MAGIC_MAPPING: {
                if (Dungeon::level) {
                    for (int i = 0; i < Level::LENGTH; ++i) {
                        if (Dungeon::level->map[i] != Terrain::WALL) {
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
                for (auto& item : hero->belongings.backpack) {
                    if (item && item->cursed) {
                        item->cursed = false;
                        procced = true;
                    }
                }
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_READ).get());
                FX::burst(hero->pos, 0xdda0ddFFu, 16);
                if (procced) {
                    GLog::p("Your pack glows with a cleansing light, and a malevolent energy disperses.");
                } else {
                    GLog::i("Your pack glows with a cleansing light, but nothing happens.");
                }
                break;
            }
            case ScrollType::RECHARGING:
                FX::burst(hero->pos, 0x55ffaaFF, 8);
                GLog::p("A surge of magical energy courses through your belongings!");
                break;
            case ScrollType::CHALLENGE:
                FX::burst(hero->pos, 0xff0000FF, 16);
                GLog::w("A deafening roar echoes through the dungeon!");
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive()) {
                        m->state = Mob::State::HUNTING;
                    }
                }
                break;
            case ScrollType::TERROR:
                FX::burst(hero->pos, 0x550055FF, 16);
                GLog::w("Creatures flee in terror from your terrifying presence!");
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                        m->state = Mob::State::FLEEING;
                    }
                }
                break;
            case ScrollType::LULLABY:
                FX::burst(hero->pos, 0x6699ffFF, 12);
                GLog::i("A soothing melody lulls nearby creatures to sleep.");
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                        m->state = Mob::State::SLEEPING;
                    }
                }
                break;
            case ScrollType::PSIONIC_BLAST:
                FX::burst(hero->pos, 0xffffffFF, 20);
                GLog::w("A burst of psychic energy explodes around you!");
                for (auto& m : Dungeon::mobs) {
                    if (m && m->isAlive() && Dungeon::visible[m->pos]) {
                        m->damage(Random::IntRange(10, 25), hero);
                    }
                }
                break;
            case ScrollType::MIRROR_IMAGE:
                FX::burst(hero->pos, 0x00ffffFF, 12);
                GLog::p("Illusory mirror images appear to aid you!");
                break;
            case ScrollType::ENCHANTMENT:
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
    return { "DRINK", "DROP" };
}

bool Dewdrop::execute(Hero* hero, const std::string& action) {
    if (!hero) return false;
    if (action == "DRINK") {
        hero->HP = std::min(hero->HT, hero->HP + 2);
        FX::floatingText(hero->pos, "+2", 0x38b764FF);
        quantity--;
        hero->spendAndNext(Actor::TICK);
        return true;
    } else if (action == "DROP") {
        return doDropItem(hero, this);
    }
    return false;
}

std::string Dewdrop::info() const {
    return "A fresh drop of crystalline dew gathered from trampled tall grass.";
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

IronKey::IronKey() : IronKey(Dungeon::depth) {}

IronKey::IronKey(int d) : Key(d) {
    name = "Iron Key";
    image = ItemSpriteSheet::IRON_KEY;
}

std::string IronKey::info() const {
    return "A simple iron key that can open a locked door.";
}

} // namespace pd
