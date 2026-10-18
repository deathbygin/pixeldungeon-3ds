#include "pd/Hero.h"
#include "pd/Mob.h"
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
#include "pd/Debug.h"
#include "pd/Statistics.h"
#include "pd/SaveManager.h"
#include "pd/ui/BottomScreenUI.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"
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
    belongings.weaponSlot.reset();
    belongings.armorSlot.reset();
    belongings.ring1Slot.reset();
    belongings.ring2Slot.reset();
    belongings.equippedWeapon = nullptr;
    belongings.equippedArmor = nullptr;
    belongings.equippedRing1 = nullptr;
    belongings.equippedRing2 = nullptr;
    if (!belongings.keyring) {
        belongings.keyring = std::make_unique<Keyring>();
    } else {
        belongings.keyring->keys.clear();
    }

    // Common starter gear
    auto clothArmor = std::make_unique<Armor>("Cloth Armor", 1, 10);
    clothArmor->identify();
    belongings.equip(std::move(clothArmor));

    auto ration = std::make_unique<Food>();
    ration->identify();
    belongings.collect(std::move(ration));

    switch (cl) {
        case HeroClass::Warrior: {
            HP = 20;
            HT = 20;
            STR = 11; // Warriors start with 11 STR

            auto sword = std::make_unique<ShortSword>();
            sword->isWarriorStarter = true;
            sword->identify();
            belongings.equip(std::move(sword));

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

            auto knuckles = std::make_unique<Knuckles>();
            knuckles->identify();
            belongings.equip(std::move(knuckles));

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

            auto dagger = std::make_unique<Dagger>();
            dagger->identify();
            belongings.equip(std::move(dagger));

            auto ring = std::make_unique<Ring>(Ring::Gem::ONYX, "Shadows");
            ring->upgrade();
            ring->identify();
            belongings.ring1Slot = std::move(ring);
            belongings.equippedRing1 = belongings.ring1Slot.get();

            auto darts = std::make_unique<Dart>(8);
            darts->identify();
            belongings.collect(std::move(darts));

            // Magic mapping scroll identified
            Scroll::setKnown(Scroll::ScrollType::MAGIC_MAPPING);
            break;
        }
        case HeroClass::Huntress: {
            HP = 15; // Huntress starts with 15 HP
            HT = 15;
            STR = 10;

            auto dagger = std::make_unique<Dagger>();
            dagger->identify();
            belongings.equip(std::move(dagger));

            auto boomerang = std::make_unique<Boomerang>();
            boomerang->identify();
            belongings.collect(std::move(boomerang));
            break;
        }
    }

    Badges::Badge masteryBadge = Badges::Badge::COUNT;
    switch (cl) {
        case HeroClass::Warrior:  masteryBadge = Badges::Badge::MASTERY_WARRIOR; break;
        case HeroClass::Mage:     masteryBadge = Badges::Badge::MASTERY_MAGE; break;
        case HeroClass::Rogue:    masteryBadge = Badges::Badge::MASTERY_ROGUE; break;
        case HeroClass::Huntress: masteryBadge = Badges::Badge::MASTERY_HUNTRESS; break;
    }
    if (masteryBadge != Badges::Badge::COUNT && Badges::isUnlocked(masteryBadge)) {
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

    // Ring identification progression
    auto updateRing = [this](Item* item) {
        if (!item) return;
        Ring* r = item->asRing();
        if (r && !r->isIdentified()) {
            if (--r->ticksToKnow <= 0) {
                r->identify();
                r->notifyIdentified(this);
            }
        }
    };
    updateRing(belongings.equippedRing1);
    updateRing(belongings.equippedRing2);

    if (paralysed && !buff(BuffType::Paralysis) && !buff(BuffType::Frost)) {
        paralysed = false;
    }
    if (rooted && !buff(BuffType::Roots)) {
        rooted = false;
    }

    if (restoreHealth) {
        if (paralysed || buff(BuffType::Paralysis) != nullptr || buff(BuffType::Frost) != nullptr || isStarving() || HP >= HT) {
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

void Hero::swapWith(Mob* ally, Level* currentLevel) {
    if (!ally || !currentLevel) return;

    int heroPos = pos;
    int allyPos = ally->pos;

    Actor::freeCell(heroPos);
    Actor::freeCell(allyPos);

    ally->pos = heroPos;
    Actor::chars[heroPos] = ally;
    if (ally->getSprite()) {
        ally->getSprite()->move(allyPos, heroPos);
    }
    ally->spriteVisible = (heroPos >= 0 && heroPos < Level::LENGTH) ? Dungeon::visible[heroPos] : false;
    if (currentLevel->get(heroPos) == Terrain::HIGH_GRASS) {
        currentLevel->trample(heroPos, ally);
    }
    if (!ally->flying) {
        currentLevel->press(heroPos, ally);
    }
    if (currentLevel->water[heroPos] && !ally->flying) {
        FX::ripple(heroPos);
        FX::waterSplash(heroPos, 4);
    }

    if (allyPos != heroPos) {
        enableAutoPick(heroPos);
    }

    pos = allyPos;
    Actor::chars[allyPos] = this;
    if (getSprite()) {
        getSprite()->move(heroPos, allyPos);
    }
    if (currentLevel->get(allyPos) == Terrain::HIGH_GRASS) {
        currentLevel->trample(allyPos, this);
    }
    if (!flying) {
        currentLevel->press(allyPos, this);
    }

    EarthrootArmor* earthArmor = getBuff<EarthrootArmor>();
    if (earthArmor) earthArmor->detach();
    HerbalHealing* herbalHealing = getBuff<HerbalHealing>();
    if (herbalHealing) herbalHealing->detach();

    Bleeding* b = getBuff<Bleeding>();
    if (b && isAlive()) {
        damage(Random::Int(b->level), b);
    }

    if (currentLevel->water[allyPos] && !flying) {
        if (buff(BuffType::Burning)) {
            removeBuff(BuffType::Burning);
            FX::waterSplash(allyPos, 6);
        }
        if (buff(BuffType::Ooze)) {
            removeBuff(BuffType::Ooze);
            GLog::p("The water washes away the caustic ooze!");
        }
    }

    search(currentLevel, false);
    Dungeon::observe();

    if (currentLevel->map[allyPos] == Terrain::SIGN) {
        pd::Sign::read(allyPos);
    }

    if (currentLevel->water[allyPos] && !flying) {
        FX::ripple(allyPos);
        FX::waterSplash(allyPos, 4);
    }

    Heap* heap = currentLevel->getHeap(allyPos);
    if (heap && !heap->isEmpty() && heap->type == Heap::Type::HEAP) {
        Item* item = heap->peek();
        if (isAutoPickDisabled(allyPos)) {
            if (item) {
                GLog::i("You see " + item->displayName() + " lying here.");
            }
        } else if (pickUp(currentLevel, false)) {
            m_lastStepPickedUp = true;
        } else if (item) {
            GLog::i("You see " + item->displayName() + " lying here.");
        }
    }

    m_lastStepAttacked = false;
    m_lastStepSwapped = true;

    spend(TICK / std::max(0.1f, speed()));
    next();
    waitingForInput = false;
}

bool Hero::stepTo(int nextCell, Level* currentLevel) {
    interrupt();
    halted = false;
    m_lastStepPickedUp = false;
    m_lastStepAttacked = false;
    m_lastStepSwapped = false;

    if (!currentLevel || nextCell < 0 || nextCell >= Level::LENGTH) {
        return false;
    }

    if (paralysed && !buff(BuffType::Paralysis) && !buff(BuffType::Frost)) {
        paralysed = false;
    }

    if (paralysed || buff(BuffType::Paralysis) != nullptr || buff(BuffType::Frost) != nullptr) {
        GLog::w(buff(BuffType::Frost) != nullptr ? "You can't move while frozen!" : "You can't move while paralyzed!");
        spend(TICK);
        next();
        waitingForInput = false;
        return true;
    }

    // Vertigo scrambles direction into a random adjacent step across all 8 neighbors
    bool hasVertigo = (buff(BuffType::Vertigo) != nullptr);
    if (hasVertigo && nextCell != pos) {
        static const int dirX[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
        static const int dirY[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };
        int px = pos % Level::WIDTH;
        int py = pos / Level::WIDTH;
        int r = Random::Int(8);
        int nx = px + dirX[r];
        int ny = py + dirY[r];
        if (nx >= 0 && nx < Level::WIDTH && ny >= 0 && ny < Level::HEIGHT) {
            nextCell = ny * Level::WIDTH + nx;
        }
    }

    Char* occupant = Actor::findChar(nextCell);
    if (occupant && occupant != this) {
        Mob* mob = occupant->asMob();
        bool heroAmok = (buff(BuffType::Amok) != nullptr);
        bool isHostileTarget = heroAmok || !mob || mob->isHostile();

        if (isHostileTarget) {
            if (isCharmedBy(occupant)) {
                GLog::w("You are too charmed to attack!");
                waitingForInput = true;
                return false;
            }
            ui::BottomScreenUI::setLastTarget(occupant);
            attack(occupant);
            m_lastStepAttacked = true;
            next();
            waitingForInput = false;
            return true;
        } else {
            // Non-hostile / friendly entity
            if (mob->mobType() == Mob::MobType::Shopkeeper) {
                static_cast<Shopkeeper*>(mob)->interact();
                waitingForInput = true;
                return false;
            }
            if (mob->mobType() == Mob::MobType::Sheep) {
                static_cast<Sheep*>(mob)->interact();
                waitingForInput = true;
                return false;
            }

            // Swapping with an ally/pet/friendly mob
            if (rooted && !buff(BuffType::Roots)) {
                rooted = false;
            }
            if (rooted) {
                if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
                GLog::w("You are rooted to the ground and cannot move!");
                float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : TICK;
                spend(stepTime);
                next();
                waitingForInput = false;
                return true;
            }
            if (mob->rooted && !mob->buff(BuffType::Roots)) {
                mob->rooted = false;
            }
            if (mob->paralysed || mob->rooted) {
                if (hasVertigo) {
                    if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
                    GLog::w("You stumble into " + mob->name + "!");
                    float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : (TICK / std::max(0.1f, speed()));
                    spend(stepTime);
                    next();
                    waitingForInput = false;
                    return true;
                }
                GLog::w(mob->name + " is unable to move!");
                waitingForInput = true;
                return false;
            }

            bool heroCanEnter = flying || (currentLevel->isPassable(nextCell) && !currentLevel->pit[nextCell]);
            bool allyCanEnter = mob->flying || (currentLevel->isPassable(pos) && !currentLevel->pit[pos]);

            if (!heroCanEnter || !allyCanEnter) {
                if (hasVertigo) {
                    if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
                    GLog::w("You stumble into " + mob->name + "!");
                    float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : (TICK / std::max(0.1f, speed()));
                    spend(stepTime);
                    next();
                    waitingForInput = false;
                    return true;
                }
                waitingForInput = true;
                return false;
            }

            swapWith(mob, currentLevel);
            return true;
        }
    }

    // 2-tile reach check for polearms (Spear, Glaive)
    Weapon* equippedWep = belongings.equippedWeapon ? belongings.equippedWeapon->asWeapon() : nullptr;
    if (equippedWep && equippedWep->reach() >= 2) {
        int hx = pos % Level::WIDTH;
        int hy = pos / Level::WIDTH;
        int nx = nextCell % Level::WIDTH;
        int ny = nextCell / Level::WIDTH;
        int dx = nx - hx;
        int dy = ny - hy;
        if (dx != 0 || dy != 0) {
            int rx = hx + dx * 2;
            int ry = hy + dy * 2;
            if (rx >= 0 && rx < Level::WIDTH && ry >= 0 && ry < Level::HEIGHT) {
                int reachCell = ry * Level::WIDTH + rx;
                Char* reachEnemy = Actor::findChar(reachCell);
                if (reachEnemy && reachEnemy != this) {
                    Mob* reachMob = reachEnemy->asMob();
                    bool heroAmok = (buff(BuffType::Amok) != nullptr);
                    bool isReachHostile = heroAmok || !reachMob || reachMob->isHostile();
                    if (isReachHostile) {
                        int midTile = currentLevel->get(nextCell);
                        bool blocked = (midTile == Terrain::WALL || midTile == Terrain::STATUE ||
                                        midTile == Terrain::DOOR || midTile == Terrain::LOCKED_DOOR ||
                                        midTile == Terrain::SECRET_DOOR);
                        if (!blocked) {
                            if (isCharmedBy(reachEnemy)) {
                                GLog::w("You are too charmed to attack!");
                                waitingForInput = true;
                                return false;
                            }
                            ui::BottomScreenUI::setLastTarget(reachEnemy);
                            attack(reachEnemy);
                            m_lastStepAttacked = true;
                            next();
                            waitingForInput = false;
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (rooted && !buff(BuffType::Roots)) {
        rooted = false;
    }
    if (rooted) {
        if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
        GLog::w("You are rooted to the ground and cannot move!");
        float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : TICK;
        spend(stepTime);
        next();
        waitingForInput = false;
        return true;
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
            if (hasVertigo) {
                if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
                GLog::w("You stumble into a locked door!");
                float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : (TICK / std::max(0.1f, speed()));
                spend(stepTime);
                next();
                waitingForInput = false;
                return true;
            }
            GLog::w("This door is locked. You need an iron key to open it.");
            waitingForInput = true;
            return false;
        }
    }

    if (tile == Terrain::LOCKED_EXIT) {
        if (belongings.consumeKey("Skeleton Key", Dungeon::depth)) {
            currentLevel->set(nextCell, Terrain::UNLOCKED_EXIT);
            Sample::playAt(Assets::SND_UNLOCK, nextCell);
            GLog::p("You unlock the exit stairs with a skeleton key.");
            spend(TICK);
            next();
            Dungeon::observe();
            waitingForInput = false;
            return true;
        } else {
            if (hasVertigo) {
                if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
                GLog::w("You stumble into the locked stairs!");
                float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : (TICK / std::max(0.1f, speed()));
                spend(stepTime);
                next();
                waitingForInput = false;
                return true;
            }
            GLog::w("Heavy bars block the stairs. You need a skeleton key to unlock it.");
            waitingForInput = true;
            return false;
        }
    }

    if (tile == Terrain::BARRICADE || tile == Terrain::BOOKSHELF) {
        if (hasVertigo) {
            if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
            GLog::w("You stumble into a barricade!");
            float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : (TICK / std::max(0.1f, speed()));
            spend(stepTime);
            next();
            waitingForInput = false;
            return true;
        }
        GLog::i("The wooden barricade blocks your way. Perhaps fire can clear it.");
        waitingForInput = true;
        return false;
    }

    // Check for container on adjacent tile BEFORE moving onto it!
    Heap* targetHeap = currentLevel->getHeap(nextCell);
    if (targetHeap && targetHeap->type == Heap::Type::FOR_SALE && targetHeap->peek() && targetHeap->peek()->price() > 0) {
        if (ui::BottomScreenUI::instance()) {
            ui::BottomScreenUI::instance()->showTradeItem(targetHeap);
        }
        waitingForInput = true;
        return false;
    }
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
        spend(TICK / std::max(0.1f, speed()));
        next();
        if (!flying) {
            currentLevel->press(nextCell, this);
        }
        waitingForInput = false;
        return true;
    }

    if (currentLevel->isPassable(nextCell)) {
        int fromCell = pos;
        if (nextCell != fromCell) {
            enableAutoPick(fromCell);
        }

        move(nextCell);
        spend(TICK / std::max(0.1f, speed()));
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

        // Stepping onto an item tile:
        // Automatically pick up items lying on the floor!
        Heap* heap = currentLevel->getHeap(nextCell);
        if (heap && !heap->isEmpty() && heap->type == Heap::Type::HEAP) {
            Item* item = heap->peek();
            if (isAutoPickDisabled(nextCell)) {
                if (item) {
                    GLog::i("You see " + item->displayName() + " lying here.");
                }
            } else if (pickUp(currentLevel, false)) {
                m_lastStepPickedUp = true;
            } else if (item) {
                GLog::i("You see " + item->displayName() + " lying here.");
            }
        }

        waitingForInput = false;
        return true;
    }

    if (hasVertigo) {
        if (noosa::Camera::main) noosa::Camera::main->shake(0.15f, 2.0f);
        GLog::w("You stumble into a wall!");
        float stepTime = (buff(BuffType::Cripple) != nullptr) ? (TICK * 2.0f) : (TICK / std::max(0.1f, speed()));
        spend(stepTime);
        next();
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

    if (heap->type == Heap::Type::FOR_SALE && heap->peek() && heap->peek()->price() > 0) {
        if (ui::BottomScreenUI::instance()) {
            ui::BottomScreenUI::instance()->showTradeItem(heap);
        }
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

    Item* peeked = heap->peek();
    if (!peeked) return false;

    // Check if backpack can accept the item before taking it from the ground!
    if (peeked->name == "Dewdrop") {
        DewVial* vial = nullptr;
        for (auto& bIt : belongings.backpack) {
            if (bIt && bIt->asDewVial()) {
                vial = bIt->asDewVial();
                break;
            }
        }
        if (HP >= HT && vial && vial->isFull()) {
            return false; // Leave on ground, hero steps over harmlessly
        }
    } else if (peeked->name != "Gold") {
        if (!belongings.canCollect(peeked)) {
            GLog::w("Your pack is too full for " + peeked->displayName());
            return false;
        }
    }

    std::unique_ptr<Item> item = heap->pickUp();
    if (!item) return false;

    std::string itemName = item->displayName();
    if (item->name == "Dewdrop") {
        DewVial* vial = nullptr;
        for (auto& bIt : belongings.backpack) {
            if (bIt && bIt->asDewVial()) {
                vial = bIt->asDewVial();
                break;
            }
        }
        if (HP < HT || vial == nullptr) {
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
        } else if (!vial->isFull()) {
            vial->collectDew(item->quantity);
            Sample::play(Assets::SND_DEWDROP);
        } else {
            if (currentLevel) {
                currentLevel->drop(std::move(item), pos);
            }
            return false;
        }
    } else if (item->name == "Gold") {
        int g = item->quantity;
        gold += g;
        Dungeon::gold += g;
        Statistics::goldCollected += g;
        Badges::validateGoldCollected(Dungeon::gold);
        FX::floatingText(pos, "+" + std::to_string(g), 0xffd700FF);
        GLog::p("You collected " + std::to_string(g) + " gold.");
        Sample::play(Assets::SND_GOLD, 1.0f, 1.0f, Random::Float(0.9f, 1.1f));
    } else {
        bool wasKey = (item->asKey() != nullptr);
        if (belongings.collect(item)) {
            if (wasKey) {
                GLog::p("You found a " + itemName + " (it was added to your key ring)");
            } else {
                GLog::p("You picked up " + itemName);
            }
            Sample::play(Assets::SND_ITEM);
        } else {
            if (wasKey) {
                GLog::w("Your key ring is full!");
            } else {
                GLog::w("Your pack is too full for " + itemName);
            }
            currentLevel->drop(std::move(item), pos);
            return false;
        }
    }

    if (heap->isEmpty()) {
        currentLevel->removeHeap(heap);
        enableAutoPick(pos);
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
        if (currentLevel->water[pos] && !flying) {
            FX::ripple(pos);
        }
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
    if (Debug::godMode) {
        HP = HT;
        return;
    }

    DewVial* vial = nullptr;
    for (auto& it : belongings.backpack) {
        if (it && it->asDewVial()) {
            vial = it->asDewVial();
            break;
        }
    }
    if (vial && vial->isFull()) {
        vial->volume = 0;
        HP = HT;
        Sample::play(Assets::SND_DRINK);
        FX::burst(pos, 0x00ff88FFu, 16);
        FX::floatingText(pos, "+" + std::to_string(HT), 0x38b764FF);
        GLog::p("The dew vial bursts with life-giving liquid, restoring you from the brink of death!");
        return;
    }

    Sample::play(Assets::SND_DEATH);
    GLog::n("You died...");
    std::string deathReason = "";
    if (src) {
        Buff* b = src->asBuff();
        if (b) {
            switch (b->type()) {
                case BuffType::Burning:
                    Badges::validateDeathFromFire();
                    deathReason = "Burned to death on level " + std::to_string(Dungeon::depth);
                    break;
                case BuffType::Poison:
                    Badges::validateDeathFromPoison();
                    deathReason = "Died from poison on level " + std::to_string(Dungeon::depth);
                    break;
                case BuffType::Hunger:
                    Badges::validateDeathFromHunger();
                    deathReason = "Starved to death on level " + std::to_string(Dungeon::depth);
                    break;
                case BuffType::Bleeding:
                    deathReason = "Bled to death on level " + std::to_string(Dungeon::depth);
                    break;
                case BuffType::Ooze:
                    deathReason = "Killed by a caustic ooze on level " + std::to_string(Dungeon::depth);
                    break;
                default: break;
            }
        }
        Char* ch = src->asChar();
        if (ch) {
            Mob* m = ch->asMob();
            if (m) {
                if (Dungeon::bossLevel(Dungeon::depth)) {
                    deathReason = "Killed by the " + m->name + " on level " + std::to_string(Dungeon::depth);
                } else {
                    char first = m->name.empty() ? 'x' : std::tolower(static_cast<unsigned char>(m->name[0]));
                    bool vowel = (first == 'a' || first == 'e' || first == 'i' || first == 'o' || first == 'u');
                    deathReason = "Killed by " + std::string(vowel ? "an " : "a ") + m->name + " on level " + std::to_string(Dungeon::depth);
                }
            }
        }
    }
    if (deathReason.empty()) {
        if (!Dungeon::resultDescription.empty()) {
            deathReason = Dungeon::resultDescription;
        } else {
            deathReason = "Died on level " + std::to_string(Dungeon::depth);
        }
    }
    belongings.identify();
    Dungeon::fail(deathReason);
    SaveManager::deleteSave();
    Char::die(src);
}

void Hero::damage(int dmg, Actor* src) {
    if (Debug::godMode) {
        return;
    }
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
    int aEnc = belongings.equippedArmor ? belongings.equippedArmor->STR - effectiveSTR() : 0;
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
    if (enemyChar) {
        ui::BottomScreenUI::setLastTarget(enemyChar);
    }
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
    int aEnc = belongings.equippedArmor ? belongings.equippedArmor->STR - effectiveSTR() : 0;
    int baseSkill = 5 + level;
    if (belongings.equippedWeapon) {
        baseSkill += belongings.equippedWeapon->defenseBonus();
    }
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
    } else if (belongings.equippedWeapon) {
        dmg = belongings.equippedWeapon->damageRoll(this);
    } else {
        dmg = effectiveSTR() > 10 ? Random::IntRange(1, effectiveSTR() - 9) : 1;
    }
    if (getBuff<Fury>() != nullptr) {
        dmg = static_cast<int>(dmg * 1.5f);
    }
    return dmg;
}

float Hero::attackDelay() const {
    if (rangedWeapon) {
        return rangedWeapon->speedFactor(const_cast<Hero*>(this));
    } else if (belongings.equippedWeapon) {
        return belongings.equippedWeapon->speedFactor(const_cast<Hero*>(this));
    }
    return 1.0f;
}

int Hero::stealth() const {
    int s = 0;
    if (heroClass == HeroClass::Rogue) {
        s += 1;
    }
    if (belongings.equippedRing1 && belongings.equippedRing1->asRing() && belongings.equippedRing1->asRing()->effect == "Shadows") {
        s += belongings.equippedRing1->level;
    }
    if (belongings.equippedRing2 && belongings.equippedRing2->asRing() && belongings.equippedRing2->asRing()->effect == "Shadows") {
        s += belongings.equippedRing2->level;
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
            SnipersMark* sm = prolong<SnipersMark>(attackDelay() * 1.1f);
            if (sm) {
                sm->targetId = enemy->id;
                sm->dur = attackDelay() * 1.1f;
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
                    wand->onZap(enemy->pos);
                    if (wand->curCharges > 0) {
                        wand->curCharges--;
                    }
                    if (!wand->isIdentified() && --wand->usagesToKnow <= 0) {
                        wand->identify();
                        wand->notifyIdentified(this);
                    }
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
