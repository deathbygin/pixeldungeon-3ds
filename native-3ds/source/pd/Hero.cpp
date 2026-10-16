#include "pd/Hero.h"
#include "pd/Item.h"
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
#include "pd/Sign.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include <algorithm>

namespace pd {

Hero::Hero() : Hero(HeroClass::Warrior) {}

Hero::Hero(HeroClass cl) {
    name = "Hero";
    viewDistance = 8;
    waitingForInput = true;
    restoreHealth = false;

    initClass(cl);
    live();
}

std::string Hero::className() const {
    if (subClass != HeroSubClass::None) {
        return subClassTitle(subClass);
    }
    switch (heroClass) {
        case HeroClass::Warrior:  return "Warrior";
        case HeroClass::Mage:     return "Mage";
        case HeroClass::Rogue:    return "Rogue";
        case HeroClass::Huntress: return "Huntress";
        default:                  return "Hero";
    }
}

void Hero::updateAwareness() {
    awareness = static_cast<float>(1.0 - std::pow(
        (heroClass == HeroClass::Rogue ? 0.85 : 0.90),
        (1.0 + std::min(level, 9)) * 0.5
    ));
}

void Hero::initClass(HeroClass cl) {
    heroClass = cl;
    subClass = HeroSubClass::None;
    belongings.backpack.clear();
    belongings.equippedWeapon = nullptr;
    belongings.equippedArmor = nullptr;
    belongings.equippedRing1 = nullptr;
    belongings.equippedRing2 = nullptr;

    // Common starter gear
    auto clothArmor = std::make_unique<Armor>("Cloth Armor", 1, 10);
    clothArmor->identify();
    belongings.equip(clothArmor.get());
    belongings.collect(std::move(clothArmor));

    auto ration = std::make_unique<Food>();
    ration->identify();
    belongings.collect(std::move(ration));

    switch (cl) {
        case HeroClass::Warrior: {
            HP = 20;
            HT = 20;
            STR = 11; // Warriors start with 11 STR

            auto sword = std::make_unique<ShortSword>();
            sword->identify();
            belongings.equip(sword.get());
            belongings.collect(std::move(sword));

            auto darts = std::make_unique<Dart>(8);
            darts->identify();
            belongings.collect(std::move(darts));

            // Strength identified
            Potion::setKnown(Potion::PotionType::STRENGTH);
            break;
        }
        case HeroClass::Mage: {
            HP = 20;
            HT = 20;
            STR = 10;

            auto knuckles = std::make_unique<Weapon>("Knuckles", 1, 6, 10);
            knuckles->identify();
            belongings.equip(knuckles.get());
            belongings.collect(std::move(knuckles));

            // Wand of Magic Missile
            auto wand = std::make_unique<WandOfMagicMissile>();
            wand->identify();
            belongings.collect(std::move(wand));

            // Identify scroll identified
            Scroll::setKnown(Scroll::ScrollType::IDENTIFY);
            break;
        }
        case HeroClass::Rogue: {
            HP = 20;
            HT = 20;
            STR = 10;

            auto dagger = std::make_unique<Weapon>("Dagger", 2, 5, 10);
            dagger->identify();
            belongings.equip(dagger.get());
            belongings.collect(std::move(dagger));

            auto darts = std::make_unique<Dart>(8);
            darts->identify();
            belongings.collect(std::move(darts));

            // Magic mapping scroll identified
            Scroll::setKnown(Scroll::ScrollType::MAGIC_MAPPING);

            // Ring of Shadows bonus passive
            affect<Shadows>();
            break;
        }
        case HeroClass::Huntress: {
            HP = 15; // Huntress starts with 15 HP
            HT = 15;
            STR = 10;

            auto dagger = std::make_unique<Weapon>("Dagger", 2, 5, 10);
            dagger->identify();
            belongings.equip(dagger.get());
            belongings.collect(std::move(dagger));

            auto boomerang = std::make_unique<Boomerang>();
            boomerang->identify();
            belongings.collect(std::move(boomerang));
            break;
        }
    }

    if (Badges::isUnlocked(Badges::Badge::BOSS_SLAIN_2)) {
        belongings.collect(std::make_unique<TomeOfMastery>());
    }

    updateAwareness();
    sprite = CharSprite::createForHero(this);
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
    halted = true;
}

bool Hero::act() {
    // Wand recharge progression
    for (auto& it : belongings.backpack) {
        if (it) {
            Wand* w = it->asWand();
            if (w) w->updateCharge(1.0f, this);
        }
    }
    if (belongings.equippedWeapon) {
        Wand* w = belongings.equippedWeapon->asWand();
        if (w) w->updateCharge(1.0f, this);
    }

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
    halted = false;

    if (!currentLevel || nextCell < 0 || nextCell >= Level::LENGTH) {
        return false;
    }

    if (paralysed) {
        GLog::w("You can't move while paralyzed!");
        waitingForInput = true;
        return false;
    }

    // Vertigo scrambles direction into a random adjacent passable step
    if (buff(BuffType::Vertigo) != nullptr && nextCell != pos) {
        std::vector<int> adj;
        int px = pos % Level::WIDTH;
        int py = pos / Level::WIDTH;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = px + dx;
                int ny = py + dy;
                if (nx >= 0 && nx < Level::WIDTH && ny >= 0 && ny < Level::HEIGHT) {
                    int c = ny * Level::WIDTH + nx;
                    if (currentLevel->isPassable(c)) {
                        adj.push_back(c);
                    }
                }
            }
        }
        if (!adj.empty()) {
            nextCell = adj[Random::Int(static_cast<int>(adj.size()))];
        }
    }

    Char* enemy = Actor::findChar(nextCell);
    if (enemy && enemy != this) {
        if (isCharmedBy(enemy)) {
            GLog::w("You are too charmed to attack!");
            waitingForInput = true;
            return false;
        }
        attack(enemy);
        next();
        waitingForInput = false;
        return true;
    }

    if (rooted) {
        GLog::w("You are rooted to the ground and cannot move!");
        waitingForInput = true;
        return false;
    }

    int tile = currentLevel->get(nextCell);
    if (tile == Terrain::DOOR || tile == Terrain::SECRET_DOOR) {
        currentLevel->set(nextCell, Terrain::OPEN_DOOR);
        Sample::playAt(Assets::SND_OPEN, nextCell);
        spend(TICK);
        next();
        Dungeon::observe();
        waitingForInput = false;
        return true;
    }

    if (tile == Terrain::LOCKED_DOOR) {
        if (belongings.consumeKey("Iron Key", Dungeon::depth)) {
            currentLevel->set(nextCell, Terrain::DOOR);
            Sample::playAt(Assets::SND_UNLOCK, nextCell);
            GLog::p("You unlock the door with an iron key.");
            spend(TICK);
            next();
            Dungeon::observe();
            waitingForInput = false;
            return true;
        } else {
            GLog::w("This door is locked. You need an iron key to open it.");
            waitingForInput = true;
            return false;
        }
    }

    if (tile == Terrain::BARRICADE || tile == Terrain::BOOKSHELF) {
        GLog::i("The wooden barricade blocks your way. Perhaps fire can clear it.");
        waitingForInput = true;
        return false;
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
        if (!flying) {
            currentLevel->press(nextCell, this);
        }
        waitingForInput = false;
        return true;
    }

    bool wasHighGrass = (currentLevel->get(nextCell) == Terrain::HIGH_GRASS);

    if (currentLevel->isPassable(nextCell)) {
        move(nextCell);
        spend(TICK / speed());
        next();

        // Level press (traps, high grass, etc.)
        if (!flying) {
            currentLevel->press(nextCell, this);
        }

        // Passive search upon moving
        search(currentLevel, false);

        Dungeon::observe();

        if (currentLevel->map[nextCell] == Terrain::SIGN) {
            pd::Sign::read(nextCell);
        }

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

            Item* item = heap->peek();
            bool canAutoPick = !hasAggroEnemy && !wasHighGrass;
            if (canAutoPick && item && item->name == "Dewdrop" && HP >= HT) {
                canAutoPick = false; // Don't waste dew if already at full health
            }

            if (canAutoPick) {
                pickUp(currentLevel, false);
            } else if (item) {
                GLog::i("You see " + item->displayName() + " lying here.");
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

bool Hero::pickUp(Level* currentLevel, bool spendTurn) {
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
        if (spendTurn) {
            spend(TICK);
            next();
            waitingForInput = false;
            Dungeon::observe();
        }
        return true;
    }

    std::unique_ptr<Item> item = heap->pickUp();
    if (!item) return false;

    std::string itemName = item->displayName();
    if (item->name == "Dewdrop") {
        int value = 1 + (Dungeon::depth - 1) / 5;
        if (heroClass == HeroClass::Huntress) {
            value++;
        }
        int effect = std::min(HT - HP, value * item->quantity);
        if (effect > 0) {
            HP += effect;
            FX::floatingText(pos, "+" + std::to_string(effect), 0x38b764FF);
            FX::burst(pos, 0x38b764FF, 4);
            GLog::p("You drank a dewdrop (+" + std::to_string(effect) + " HP)");
        } else {
            GLog::i("You drank a fresh dewdrop.");
        }
        Sample::play(Assets::SND_DEWDROP);
    } else if (item->name == "Gold") {
        int g = item->quantity;
        gold += g;
        Dungeon::gold += g;
        Badges::validateGoldCollected(Dungeon::gold);
        FX::floatingText(pos, "+" + std::to_string(g), 0xffd700FF);
        GLog::p("You collected " + std::to_string(g) + " gold.");
        Sample::play(Assets::SND_GOLD, 1.0f, 1.0f, Random::Float(0.9f, 1.1f));
    } else if (belongings.collect(std::move(item))) {
        GLog::p("You picked up " + itemName);
        Sample::play(Assets::SND_ITEM);
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

    if (spendTurn) {
        spend(TICK);
        next();
        waitingForInput = false;
        Dungeon::observe();
    }
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
    updateAwareness();

    int positive = 0;
    int negative = 0;
    auto checkRing = [&](Item* r) {
        if (r && r->name == "Ring of Detection") {
            int bonus = r->level;
            if (bonus > positive) {
                positive = bonus;
            } else if (bonus < 0) {
                negative += bonus;
            }
        }
    };
    checkRing(belongings.equippedRing1);
    checkRing(belongings.equippedRing2);

    int distance = 1 + positive + negative;
    float searchSkill = intentional ? (2.0f * awareness - awareness * awareness) : awareness;
    if (distance <= 0) {
        searchSkill /= static_cast<float>(2 - distance);
        distance = 1;
    }

    int cx = pos % Level::WIDTH;
    int cy = pos / Level::WIDTH;

    if (intentional) {
        CharSprite* spr = getSprite();
        if (spr) {
            HeroSprite* heroSprite = spr->asHeroSprite();
            if (heroSprite) heroSprite->operate(pos);
        }
        FX::floatingText(pos, "search", 0xFFFFFFFF);
        FX::ripple(pos);
    }

    int ax = std::max(0, cx - distance);
    int bx = std::min(Level::WIDTH - 1, cx + distance);
    int ay = std::max(0, cy - distance);
    int by = std::min(Level::HEIGHT - 1, cy + distance);

    bool doorFound = false;
    bool trapFound = false;
    bool heapFound = false;

    for (int y = ay; y <= by; ++y) {
        for (int x = ax; x <= bx; ++x) {
            int p = y * Level::WIDTH + x;

            if (Dungeon::visible[p]) {
                if (intentional) {
                    FX::searchCell(p);
                }

                if (currentLevel->secret[p] && (intentional || Random::Float() < searchSkill)) {
                    int oldValue = currentLevel->map[p];
                    int newValue = Terrain::discover(oldValue);
                    currentLevel->set(p, newValue);

                    if (oldValue == Terrain::SECRET_DOOR) {
                        doorFound = true;
                    } else {
                        trapFound = true;
                    }
                    smthFound = true;
                }

                if (intentional) {
                    Heap* heap = currentLevel->getHeap(p);
                    if (heap && heap->type == Heap::Type::HIDDEN) {
                        heap->type = Heap::Type::HEAP;
                        heapFound = true;
                        smthFound = true;
                    }
                }
            }
        }
    }

    if (intentional) {
        float timeSpent = smthFound ? (Random::Float() < searchSkill ? 2.0f * TICK : 4.0f * TICK) : 2.0f * TICK;
        spend(timeSpent);
        next();
        Dungeon::observe();
        waitingForInput = false;
    }

    if (smthFound) {
        if (doorFound && !trapFound && !heapFound) {
            GLog::w("You found a secret door!");
        } else if (trapFound && !doorFound && !heapFound) {
            GLog::w("You found a secret trap!");
        } else {
            GLog::w("You noticed something!");
        }
        Sample::play(Assets::SND_SECRET);
        interrupt();
    }

    return smthFound;
}

void Hero::earnExp(int amount) {
    exp += amount;

    // Warlock soul siphon on enemy kill
    if (subClass == HeroSubClass::Warlock) {
        int value = std::min(HT - HP, 1 + (Dungeon::depth - 1) / 5);
        if (value > 0) {
            HP += value;
            FX::burst(pos, 0x38b764FF, 4);
            FX::floatingText(pos, "+" + std::to_string(value), 0x38b764FF);
        }
        Hunger* hunger = getBuff<Hunger>();
        if (hunger) {
            hunger->satisfy(10);
        }
    }

    while (exp >= maxExp) {
        exp -= maxExp;
        level++;
        maxExp = 5 + level * 5;
        HT += 5;
        HP = std::min(HT, HP + 5);
        if (level % 5 == 0) {
            STR++;
        }
        updateAwareness();
        GLog::h("Welcome to level " + std::to_string(level) + "!");
        Sample::play(Assets::SND_LEVELUP);
        Badges::validateLevelReached(level);
    }
}

void Hero::die(Actor* src) {
    Sample::play(Assets::SND_DEATH);
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

void Hero::damage(int dmg, Actor* src) {
    restoreHealth = false;
    Char::damage(dmg, src);
    if (subClass == HeroSubClass::Berserker && HP > 0 && HP <= HT * 0.40f) {
        if (!getBuff<Fury>()) {
            affect<Fury>();
            GLog::w("You become furious!");
            FX::floatingText(pos, "FURIOUS", 0xff3333FF);
        }
    }
}

float Hero::speed() const {
    int aEnc = belongings.equippedArmor ? belongings.equippedArmor->STR - STR : 0;
    if (aEnc > 0) {
        return static_cast<float>(Char::speed() * std::pow(1.3, -aEnc));
    } else {
        float spd = Char::speed();
        if (subClass == HeroSubClass::Freerunner && !isStarving()) {
            return 1.6f * spd;
        }
        return spd;
    }
}

bool Hero::shoot(Char* enemyChar, MissileWeapon* wep) {
    rangedWeapon = wep;
    bool result = attack(enemyChar);
    rangedWeapon = nullptr;
    return result;
}

int Hero::attackSkill(Char* target) {
    int skill = 10 + level * 2;
    if (rangedWeapon && target && PathFinder::distanceChebyshev(pos, target->pos) == 1) {
        skill /= 2; // Point-blank ranged penalty matching Java (accuracy *= 0.5f)
    }
    if (rangedWeapon) {
        skill = static_cast<int>(skill * rangedWeapon->acuracyFactor(this));
    } else if (belongings.equippedWeapon) {
        skill = static_cast<int>(skill * belongings.equippedWeapon->acuracyFactor(this));
    }
    return skill;
}

int Hero::defenseSkill(Char* attacker) {
    (void)attacker;
    float evasion = 1.0f;
    if (paralysed) {
        evasion /= 2.0f;
    }
    int aEnc = belongings.equippedArmor ? belongings.equippedArmor->STR - STR : 0;
    int baseSkill = 5 + level;
    if (aEnc > 0) {
        return static_cast<int>(baseSkill * evasion / std::pow(1.5, aEnc));
    } else {
        if (heroClass == HeroClass::Rogue) {
            if (subClass == HeroSubClass::Freerunner && !isStarving()) {
                evasion *= 2.0f;
            }
            return static_cast<int>((baseSkill - aEnc) * evasion);
        } else {
            return static_cast<int>(baseSkill * evasion);
        }
    }
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
    int dmg = 0;
    if (rangedWeapon) {
        dmg = rangedWeapon->damageRoll(this);
        if (heroClass == HeroClass::Huntress) {
            int exStr = STR - rangedWeapon->STR;
            if (exStr > 0) {
                dmg += Random::IntRange(0, exStr);
            }
        }
    } else if (belongings.equippedWeapon) {
        dmg = belongings.equippedWeapon->damageRoll(this);
    } else {
        dmg = STR > 10 ? Random::IntRange(1, STR - 9) : 1;
    }
    if (getBuff<Fury>() != nullptr) {
        dmg = static_cast<int>(dmg * 1.5f);
    }
    return dmg;
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
    if (rangedWeapon) {
        rangedWeapon->proc(this, enemy, damage);
        if (subClass == HeroSubClass::Sniper) {
            SnipersMark* sm = affect<SnipersMark>();
            if (sm) {
                sm->targetId = enemy->id;
            }
        }
    } else if (belongings.equippedWeapon) {
        belongings.equippedWeapon->proc(this, enemy, damage);
        if (subClass == HeroSubClass::Gladiator) {
            Combo* combo = affect<Combo>();
            if (combo) {
                damage += combo->hit(enemy, damage);
            }
        } else if (subClass == HeroSubClass::Battlemage) {
            Wand* wand = belongings.equippedWeapon->asWand();
            if (wand) {
                if (wand->curCharges >= wand->maxCharges) {
                    // Maximum charges
                } else if (damage > 0) {
                    wand->restoreCharge(1);
                    GLog::i("Your wand absorbs kinetic essence and regains a charge!");
                }
                damage += wand->curCharges;
            }
        }
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
