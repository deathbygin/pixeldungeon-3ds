#include "pd/Hero.h"
#include "pd/CharSprite.h"
#include "pd/Dungeon.h"
#include "pd/Level.h"
#include "pd/Terrain.h"
#include "pd/Random.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Buff.h"
#include "pd/Heap.h"
#include "pd/Assets.h"
#include "pd/Badges.h"
#include "noosa/Audio.h"
#include <algorithm>

namespace pd {

Hero::Hero() {
    name = "Hero";
    HP = 20;
    HT = 20;
    STR = 11;
    viewDistance = 8;
    waitingForInput = true;
    restoreHealth = false;

    // Starting gear (Warrior default)
    auto dagger = std::make_unique<Weapon>("Dagger", 2, 5, 10);
    dagger->identify();
    auto clothArmor = std::make_unique<Armor>("Cloth Armor", 1, 10);
    clothArmor->identify();

    belongings.equip(dagger.get());
    belongings.equip(clothArmor.get());

    belongings.collect(std::move(dagger));
    belongings.collect(std::move(clothArmor));

    // Starting potion & food ration
    belongings.collect(std::make_unique<Potion>(Potion::PotionType::HEALING));
    belongings.collect(std::make_unique<Food>());

    sprite = CharSprite::createForHero(this);

    live();
}

CharSprite* Hero::getSprite() {
    if (!sprite) {
        sprite = CharSprite::createForHero(this);
        sprite->place(pos);
    }
    return sprite.get();
}

void Hero::updateArmorTier() {
    CharSprite* spr = getSprite();
    if (spr) {
        HeroSprite* heroSprite = spr->asHeroSprite();
        if (heroSprite) {
            heroSprite->updateArmor(tier());
        }
    }
}

void Hero::live() {
    affect<Regeneration>();
    affect<Hunger>();
}

void Hero::interrupt() {
    restoreHealth = false;
}

bool Hero::act() {
    if (restoreHealth) {
        if (isStarving() || HP >= HT) {
            restoreHealth = false;
            waitingForInput = true;
            return false;
        } else {
            spend(TICK);
            next();
            Dungeon::observe();
            return true;
        }
    }

    waitingForInput = true;
    return false;
}

bool Hero::isStarving() const {
    Hunger* h = getBuff<Hunger>();
    return h && h->isStarving();
}

bool Hero::isHungry() const {
    Hunger* h = getBuff<Hunger>();
    return h && h->isHungry();
}

std::string Hero::hungerStatus() const {
    Hunger* h = getBuff<Hunger>();
    if (!h) return "";
    return h->name();
}

bool Hero::stepTo(int nextCell, Level* currentLevel) {
    interrupt();

    if (!currentLevel || nextCell < 0 || nextCell >= Level::LENGTH) {
        return false;
    }

    Char* enemy = Actor::findChar(nextCell);
    if (enemy && enemy != this) {
        attack(enemy);
        next();
        waitingForInput = false;
        return true;
    }

    int tile = currentLevel->get(nextCell);
    if (tile == Terrain::DOOR || tile == Terrain::SECRET_DOOR) {
        currentLevel->destroy(nextCell);
        spend(TICK);
        next();
        Dungeon::observe();
        waitingForInput = false;
        return true;
    }

    // Check for container on adjacent tile BEFORE moving onto it!
    Heap* targetHeap = currentLevel->getHeap(nextCell);
    if (targetHeap && targetHeap->type != Heap::Type::HEAP && targetHeap->type != Heap::Type::FOR_SALE && targetHeap->type != Heap::Type::HIDDEN) {
        // Player opens container from an adjacent tile without stepping onto its tile!
        openContainer(targetHeap, nextCell);
        spend(TICK);
        next();
        waitingForInput = false;
        Dungeon::observe();
        return true;
    }

    if (currentLevel->pit[nextCell]) {
        move(nextCell);
        spend(TICK / speed());
        next();
        currentLevel->press(nextCell, this);
        waitingForInput = false;
        return true;
    }

    if (currentLevel->isPassable(nextCell)) {
        move(nextCell);
        spend(TICK / speed());
        next();

        // Level press (traps, high grass, etc.)
        currentLevel->press(nextCell, this);

        // Passive search upon moving
        search(currentLevel, false);

        Dungeon::observe();

        if (currentLevel->water[nextCell]) {
            FX::ripple(nextCell);
            FX::waterSplash(nextCell, 4);
        }

        // Stepping onto an item tile:
        // When no enemies are aggro'd (hunting the hero), items are automatically picked up!
        Heap* heap = currentLevel->getHeap(nextCell);
        if (heap && !heap->isEmpty() && heap->type == Heap::Type::HEAP) {
            bool hasAggroEnemy = false;
            for (const auto& mob : Dungeon::mobs) {
                if (mob && mob->isAlive() && mob->state == Mob::State::HUNTING) {
                    hasAggroEnemy = true;
                    break;
                }
            }

            if (!hasAggroEnemy) {
                pickUp(currentLevel);
            } else {
                Item* item = heap->peek();
                if (item) {
                    GLog::i("You see " + item->displayName() + " lying here.");
                }
            }
        }

        waitingForInput = false;
        return true;
    }

    return false;
}

void Hero::openContainer(Heap* heap, int targetCell) {
    if (!heap) return;

    if (sprite) {
        sprite->turnTo(pos, targetCell);
        HeroSprite* hspr = sprite->asHeroSprite();
        if (hspr) {
            hspr->operate(targetCell);
        }
    }

    heap->open(this);
}

bool Hero::pickUp(Level* currentLevel) {
    if (!currentLevel) currentLevel = Dungeon::level.get();
    if (!currentLevel) return false;

    Heap* heap = currentLevel->getHeap(pos);
    if (!heap || heap->isEmpty()) {
        GLog::w("There is nothing here to pick up.");
        return false;
    }

    // If standing over an unopened container, open it instead
    if (heap->type != Heap::Type::HEAP && heap->type != Heap::Type::FOR_SALE && heap->type != Heap::Type::HIDDEN) {
        openContainer(heap, pos);
        spend(TICK);
        next();
        waitingForInput = false;
        Dungeon::observe();
        return true;
    }

    std::unique_ptr<Item> item = heap->pickUp();
    if (!item) return false;

    std::string itemName = item->displayName();
    if (item->name == "Dewdrop") {
        HP = std::min(HT, HP + 2);
        FX::floatingText(pos, "+2", 0x38b764FF);
        GLog::p("You drank a dewdrop (+2 HP)");
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_DEWDROP).get());
    } else if (item->name == "Gold") {
        int g = item->quantity;
        gold += g;
        Dungeon::gold += g;
        Badges::validateGoldCollected(Dungeon::gold);
        FX::floatingText(pos, "+" + std::to_string(g), 0xffd700FF);
        GLog::p("You collected " + std::to_string(g) + " gold.");
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_GOLD).get());
    } else if (belongings.collect(std::move(item))) {
        GLog::p("You picked up " + itemName);
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_ITEM).get());
    } else {
        GLog::w("Your pack is too full for " + itemName);
        heap->drop(std::move(item));
        return false;
    }

    if (heap->isEmpty()) {
        currentLevel->removeHeap(heap);
    } else {
        GLog::i("There is something else lying here.");
    }

    spend(TICK);
    next();
    waitingForInput = false;
    Dungeon::observe();
    return true;
}

void Hero::rest(bool tillHealthy) {
    restoreHealth = tillHealthy;
    spend(TICK);
    next();
    Dungeon::observe();
    waitingForInput = false;
}

bool Hero::search(Level* currentLevel, bool intentional) {
    if (!currentLevel) return false;

    bool smthFound = false;
    float awareness = (heroClass == HeroClass::Rogue) ? 0.30f : 0.15f;
    float searchSkill = intentional ? (2.0f * awareness - awareness * awareness) : awareness;

    int cx = pos % Level::WIDTH;
    int cy = pos / Level::WIDTH;

    if (intentional) {
        CharSprite* spr = getSprite();
        if (spr) {
            HeroSprite* heroSprite = spr->asHeroSprite();
            if (heroSprite) heroSprite->operate(pos);
        }
    }

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int x = cx + dx;
            int y = cy + dy;
            if (x < 0 || x >= Level::WIDTH || y < 0 || y >= Level::HEIGHT) continue;
            int p = y * Level::WIDTH + x;

            if (Dungeon::visible[p]) {
                if (intentional) {
                    FX::searchCell(p);
                }

                if (currentLevel->secret[p] && (intentional || Random::Float() < searchSkill)) {
                    int oldValue = currentLevel->map[p];
                    int newValue = Terrain::discover(oldValue);
                    currentLevel->set(p, newValue);
                    smthFound = true;
                }

                if (intentional) {
                    Heap* heap = currentLevel->getHeap(p);
                    if (heap && heap->type == Heap::Type::HIDDEN) {
                        heap->type = Heap::Type::HEAP;
                        smthFound = true;
                    }
                }
            }
        }
    }

    if (intentional) {
        FX::ripple(pos);
        spend(smthFound ? (Random::Float() < searchSkill ? TICK : 2.0f * TICK) : TICK);
        next();
        Dungeon::observe();
        waitingForInput = false;
    }

    if (smthFound) {
        GLog::w("You noticed something!");
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_SECRET).get());
        interrupt();
    }

    return smthFound;
}

void Hero::earnExp(int amount) {
    exp += amount;
    while (exp >= maxExp) {
        exp -= maxExp;
        level++;
        maxExp = 5 + level * 5;
        HT += 5;
        HP = std::min(HT, HP + 5);
        if (level % 5 == 0) {
            STR++;
        }
        GLog::h("Welcome to level " + std::to_string(level) + "!");
        Badges::validateLevelReached(level);
    }
}

void Hero::die(Actor* src) {
    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_DEATH).get());
    GLog::n("You died...");
    if (src) {
        Buff* b = src->asBuff();
        if (b) {
            switch (b->type()) {
                case BuffType::Burning: Badges::validateDeathFromFire(); break;
                case BuffType::Poison:  Badges::validateDeathFromPoison(); break;
                case BuffType::Hunger:  Badges::validateDeathFromHunger(); break;
                default: break;
            }
        }
    }
    Char::die(src);
}

int Hero::attackSkill(Char* target) {
    (void)target;
    return 10 + level * 2;
}

int Hero::defenseSkill(Char* attacker) {
    (void)attacker;
    return 5 + level;
}

int Hero::dr() {
    int totalDr = 0;
    if (belongings.equippedArmor) {
        totalDr += belongings.equippedArmor->drRoll();
    }
    Barkskin* b = getBuff<Barkskin>();
    if (b) {
        totalDr += b->getLevel();
    }
    return totalDr;
}

int Hero::damageRoll() {
    if (belongings.equippedWeapon) {
        return belongings.equippedWeapon->damageRoll();
    }
    return Random::IntRange(1, 2) + (STR / 2);
}

int Hero::stealth() const {
    int s = 0;
    if (heroClass == HeroClass::Rogue) {
        s += 1;
    }
    if (invisible > 0) {
        s += 5;
    }
    return s;
}

int Hero::attackProc(Char* enemy, int damage) {
    if (belongings.equippedWeapon) {
        belongings.equippedWeapon->proc(this, enemy, damage);
    }
    return damage;
}

int Hero::defenseProc(Char* enemy, int damage) {
    if (belongings.equippedArmor) {
        damage = belongings.equippedArmor->proc(enemy, this, damage);
    }
    return damage;
}

} // namespace pd
