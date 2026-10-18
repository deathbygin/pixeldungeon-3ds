#include "pd/Dungeon.h"
#include "pd/FogOfWar.h"
#include "pd/Random.h"
#include "pd/Badges.h"
#include "pd/GLog.h"
#include "pd/Item.h"
#include <algorithm>
#include <unordered_map>
#include <ctime>

#include "pd/FX.h"
#include "pd/Statistics.h"
#include "pd/Rankings.h"

namespace pd {

LevelTransition Dungeon::pendingTransition = LevelTransition::NONE;

static std::unordered_map<int, std::unique_ptr<Level>> s_cachedLevels;
static std::unordered_map<int, std::vector<std::unique_ptr<Mob>>> s_cachedMobs;

std::unique_ptr<Hero> Dungeon::hero = nullptr;
std::unique_ptr<Level> Dungeon::level = nullptr;
std::vector<std::unique_ptr<Mob>> Dungeon::mobs;
std::unique_ptr<FogOfWar> Dungeon::fog = nullptr;

bool Dungeon::visible[Level::LENGTH] = { false };
int Dungeon::depth = 1;
int Dungeon::gold = 0;
int Dungeon::deepestFloor = 1;
bool Dungeon::nightMode = false;
std::string Dungeon::resultDescription = "";
std::unordered_map<int, std::vector<std::unique_ptr<Item>>> Dungeon::droppedItems;

int Dungeon::scrollsOfUpgrade = 0;
int Dungeon::scrollsOfEnchantment = 0;
int Dungeon::potionOfStrength = 0;

static bool s_gameEnded = false;

void Dungeon::fail(const std::string& desc) {
    if (s_gameEnded) return;
    s_gameEnded = true;
    if (!desc.empty()) {
        resultDescription = desc;
    }
    if (hero) {
        hero->belongings.identify();
    }
    Rankings::instance().submit(false, resultDescription);
}

void Dungeon::win(const std::string& desc) {
    if (s_gameEnded) return;
    s_gameEnded = true;
    if (!desc.empty()) {
        resultDescription = desc;
    }
    if (hero) {
        hero->belongings.identify();
    }
    Rankings::instance().submit(true, resultDescription);
}

static bool quotaChance(const int* quota, int quotaLen, int number) {
    for (int i = 0; i < quotaLen; i += 2) {
        int qDepth = quota[i];
        if (Dungeon::depth <= qDepth) {
            int qNumber = quota[i + 1];
            return Random::Float() < static_cast<float>(qNumber - number) / (qDepth - Dungeon::depth + 1);
        }
    }
    return false;
}

bool Dungeon::posNeeded() {
    static const int quota[] = {4, 2, 9, 4, 14, 6, 19, 8, 24, 9};
    return quotaChance(quota, 10, potionOfStrength);
}

bool Dungeon::souNeeded() {
    static const int quota[] = {5, 3, 10, 6, 15, 9, 20, 12, 25, 13};
    return quotaChance(quota, 10, scrollsOfUpgrade);
}

bool Dungeon::soeNeeded() {
    return Random::Int(12 * (1 + scrollsOfEnchantment)) < depth;
}

bool Dungeon::bossLevel(int d) {
    if (d == -1) d = depth;
    return d == 5 || d == 10 || d == 15 || d == 20 || d == 25;
}

bool Dungeon::shopOnLevel(int d) {
    if (d == -1) d = depth;
    return d == 6 || d == 11 || d == 16 || d == 21;
}

void Dungeon::dropToChasm(std::unique_ptr<Item> item) {
    if (!item) return;
    int nextDepth = depth + 1;
    droppedItems[nextDepth].push_back(std::move(item));
}

void Dungeon::placeChasmItems() {
    auto it = droppedItems.find(depth);
    if (it != droppedItems.end() && level) {
        int pitPos = level->hasPitRoom() ? level->pitCell() : -1;
        for (auto& item : it->second) {
            if (item) {
                int pos = (pitPos != -1) ? pitPos : level->randomRespawnCell();
                if (Potion* p = item->asPotion()) {
                    p->shatter(pos);
                } else if (PlantSeed* s = item->asPlantSeed()) {
                    level->plant(s->plantType, pos);
                } else {
                    level->drop(std::move(item), pos);
                }
            }
        }
        droppedItems.erase(it);
    }
}

void Dungeon::init() {
    std::time_t t = std::time(nullptr);
    struct tm* lt = std::localtime(&t);
    int hour = lt ? lt->tm_hour : 12;
    nightMode = (hour < 7 || hour >= 22);
    deepestFloor = 1;

    Actor::init();
    Badges::init();
    Potion::initColors();
    Scroll::initLabels();
    Wand::initWoods();
}

void Dungeon::announceLevelEntrance() {
    if (!level) return;

    if (depth < deepestFloor) {
        GLog::h("Welcome back to level " + std::to_string(depth) + " of Pixel Dungeon!");
    } else {
        GLog::h("Welcome to level " + std::to_string(depth) + " of Pixel Dungeon!");
        deepestFloor = std::max(deepestFloor, depth);
    }
    Statistics::deepestFloor = std::max(Statistics::deepestFloor, depth);

    switch (level->feeling) {
    case Level::Feeling::CHASM:
        GLog::w("You can hear the wind howling outside.");
        GLog::w("Your steps echo across the dungeon.");
        break;
    case Level::Feeling::WATER:
        GLog::w("You hear the sound of roaring water.");
        GLog::w("You hear the water splashing around you.");
        break;
    case Level::Feeling::GRASS:
        GLog::w("The smell of vegetation is thick in the air.");
        break;
    default:
        break;
    }

    if (level->secretDoors >= 3) {
        GLog::w("The atmosphere hints that this floor hides many secrets.");
    }

    if (nightMode && !bossLevel()) {
        GLog::w("Be cautious, since the dungeon is even more dangerous at night!");
    }
}

void Dungeon::newGame() {
    s_gameEnded = false;
    s_cachedLevels.clear();
    s_cachedMobs.clear();
    depth = 1;
    gold = 0;
    deepestFloor = 1;
    resultDescription.clear();
    Statistics::reset();
    Statistics::deepestFloor = 1;
    scrollsOfUpgrade = 0;
    scrollsOfEnchantment = 0;
    potionOfStrength = 0;
    Potion::initColors();
    Scroll::initLabels();
    Wand::initWoods();
    hero = std::make_unique<Hero>();
    if (hero) {
        if (hero->heroClass == Hero::HeroClass::Warrior) {
            Potion::setKnown(Potion::PotionType::STRENGTH);
        } else if (hero->heroClass == Hero::HeroClass::Mage) {
            Scroll::setKnown(Scroll::ScrollType::IDENTIFY);
        } else if (hero->heroClass == Hero::HeroClass::Rogue) {
            Scroll::setKnown(Scroll::ScrollType::MAGIC_MAPPING);
        } else if (hero->heroClass == Hero::HeroClass::Huntress) {
            Potion::setKnown(Potion::PotionType::MIND_VISION);
        }
    }
    resetLevel();
}

void Dungeon::resetLevel() {
    Actor::clear();
    mobs.clear();
    std::fill_n(visible, Level::LENGTH, false);

    level = Level::create(depth);
    int tries = 0;
    while (!level->build()) {
        if (++tries > 20) break;
    }

    fog = std::make_unique<FogOfWar>(Level::WIDTH, Level::HEIGHT);

    if (hero) {
        hero->viewDistance = level->viewDistance;
        hero->move(level->entrance);
        Actor::occupyCell(hero.get());
        Actor::add(hero.get());
        for (Buff* b : hero->buffs) {
            if (b) Actor::add(b, Actor::TICK);
        }
    }

    spawnMobs(4);
    Actor::add(FX::getBlob(Blob::BlobType::FIRE), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::TOXIC_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::PARALYTIC_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::CONFUSION_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::CORROSIVE_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::REGROWTH), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::FOLIAGE), Actor::TICK);
    observe();
    announceLevelEntrance();
    Actor::process();
}

void Dungeon::switchLevel(int newDepth, bool isFall) {
    if (!hero || newDepth < 1) return;

    // 1. Stash current level and mobs into cache
    if (level) {
        s_cachedLevels[depth] = std::move(level);
        s_cachedMobs[depth] = std::move(mobs);
    }

    bool descending = (newDepth > depth);
    depth = newDepth;

    // 2. Clear current timeline (preserve hero & hero buffs)
    Actor::clear();
    std::fill_n(visible, Level::LENGTH, false);

    // 3. Load from cache or generate brand new floor
    auto it = s_cachedLevels.find(depth);
    if (it != s_cachedLevels.end()) {
        level = std::move(it->second);
        s_cachedLevels.erase(it);

        auto itMobs = s_cachedMobs.find(depth);
        if (itMobs != s_cachedMobs.end()) {
            mobs = std::move(itMobs->second);
            s_cachedMobs.erase(itMobs);
        }

        // Place hero at stairs or pit/respawn cell
        int spawnCell;
        if (isFall) {
            spawnCell = level->hasPitRoom() ? level->pitCell() : level->randomRespawnCell();
        } else {
            spawnCell = descending ? level->entrance : level->exit;
        }
        hero->place(spawnCell);
        Actor::occupyCell(hero.get());
        Actor::add(hero.get());
        for (Buff* b : hero->buffs) {
            if (b) Actor::add(b, Actor::TICK);
        }

        // Re-add surviving mobs to timeline
        for (auto& mob : mobs) {
            if (mob && mob->isAlive()) {
                Actor::occupyCell(mob.get());
                Actor::add(mob.get(), Random::Float(1.0f));
            }
        }
    } else {
        level = Level::create(depth);
        int tries = 0;
        while (!level->build()) {
            if (++tries > 20) break;
        }
        mobs.clear();

        int spawnPos;
        if (isFall) {
            spawnPos = level->hasPitRoom() ? level->pitCell() : level->randomRespawnCell();
        } else {
            spawnPos = descending ? level->entrance : level->exit;
        }
        hero->place(spawnPos);
        Actor::occupyCell(hero.get());
        Actor::add(hero.get());
        for (Buff* b : hero->buffs) {
            if (b) Actor::add(b, Actor::TICK);
        }

        spawnMobs(4);
    }

    if (hero && level) {
        hero->viewDistance = level->viewDistance;
    }

    fog = std::make_unique<FogOfWar>(Level::WIDTH, Level::HEIGHT);

    FX::clear();
    for (const auto& pair : level->wells) {
        int cell = pair.first;
        Level::WellType wt = pair.second;
        if (level->map[cell] == Terrain::WELL) {
            Blob::BlobType bt = (wt == Level::WellType::Health) ? Blob::BlobType::WATER_OF_HEALTH :
                                ((wt == Level::WellType::Awareness) ? Blob::BlobType::WATER_OF_AWARENESS :
                                                                      Blob::BlobType::WATER_OF_TRANSMUTATION);
            FX::seedBlob(cell, bt, 1);
        }
    }

    Actor::add(FX::getBlob(Blob::BlobType::FIRE), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::TOXIC_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::PARALYTIC_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::CONFUSION_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::CORROSIVE_GAS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::REGROWTH), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::FOLIAGE), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::WATER_OF_HEALTH), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::WATER_OF_AWARENESS), Actor::TICK);
    Actor::add(FX::getBlob(Blob::BlobType::WATER_OF_TRANSMUTATION), Actor::TICK);
    placeChasmItems();
    observe();
    announceLevelEntrance();
    Actor::process();
}

void Dungeon::nextLevel() {
    switchLevel(depth + 1);
}

void Dungeon::prevLevel() {
    if (depth > 1) {
        switchLevel(depth - 1);
    }
}

void Dungeon::spawnMobs(int count) {
    if (!level) return;

    if (depth == 5) {
        // Boss floor: Goo
        int cell = -1;
        for (int attempts = 0; attempts < 100; ++attempts) {
            int candidate = Random::Int(Level::LENGTH);
            if (level->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
                if (!hero || PathFinder::distanceChebyshev(candidate, hero->pos) >= 5) {
                    cell = candidate;
                    break;
                }
            }
        }
        if (cell != -1) {
            auto goo = std::make_unique<Goo>();
            goo->place(cell);
            Actor::occupyCell(goo.get());
            Actor::add(goo.get(), 1.0f);
            mobs.push_back(std::move(goo));
        }
        return;
    }

    if (depth == 10) {
        // Boss floor: Tengu
        int cell = -1;
        for (int attempts = 0; attempts < 100; ++attempts) {
            int candidate = Random::Int(Level::LENGTH);
            if (level->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
                if (!hero || PathFinder::distanceChebyshev(candidate, hero->pos) >= 5) {
                    cell = candidate;
                    break;
                }
            }
        }
        if (cell != -1) {
            auto tengu = std::make_unique<Tengu>();
            tengu->place(cell);
            Actor::occupyCell(tengu.get());
            Actor::add(tengu.get(), 1.0f);
            mobs.push_back(std::move(tengu));
        }
        return;
    }

    if (depth == 15) {
        // Boss floor: DM-300
        int cell = -1;
        for (int attempts = 0; attempts < 100; ++attempts) {
            int candidate = Random::Int(Level::LENGTH);
            if (level->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
                if (!hero || PathFinder::distanceChebyshev(candidate, hero->pos) >= 5) {
                    cell = candidate;
                    break;
                }
            }
        }
        if (cell != -1) {
            auto dm = std::make_unique<DM300>();
            dm->place(cell);
            Actor::occupyCell(dm.get());
            Actor::add(dm.get(), 1.0f);
            mobs.push_back(std::move(dm));
        }
        return;
    }

    if (depth == 20) {
        // Boss floor: King of Dwarves
        int cell = -1;
        for (int attempts = 0; attempts < 100; ++attempts) {
            int candidate = Random::Int(Level::LENGTH);
            if (level->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
                if (!hero || PathFinder::distanceChebyshev(candidate, hero->pos) >= 5) {
                    cell = candidate;
                    break;
                }
            }
        }
        if (cell != -1) {
            auto king = std::make_unique<King>();
            king->place(cell);
            Actor::occupyCell(king.get());
            Actor::add(king.get(), 1.0f);
            mobs.push_back(std::move(king));
        }
        return;
    }

    if (depth == 25) {
        // Boss floor: Yog-Dzewa
        int cell = -1;
        for (int attempts = 0; attempts < 100; ++attempts) {
            int candidate = Random::Int(Level::LENGTH);
            if (level->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
                if (!hero || PathFinder::distanceChebyshev(candidate, hero->pos) >= 5) {
                    cell = candidate;
                    break;
                }
            }
        }
        if (cell != -1) {
            auto yog = std::make_unique<Yog>();
            yog->place(cell);
            Actor::occupyCell(yog.get());
            Actor::add(yog.get(), 1.0f);
            mobs.push_back(std::move(yog));
        }
        return;
    }

    int mobCount = count;
    if (nightMode && !bossLevel()) {
        mobCount = count + 2;
    }

    for (int i = 0; i < mobCount; ++i) {
        int cell = -1;
        for (int attempts = 0; attempts < 100; ++attempts) {
            int candidate = Random::Int(Level::LENGTH);
            if (level->isPassable(candidate) && Actor::findChar(candidate) == nullptr) {
                if (hero) {
                    int dist = PathFinder::distanceChebyshev(candidate, hero->pos);
                    if (dist < 4) continue;
                }
                cell = candidate;
                break;
            }
        }
        if (cell == -1) continue;

        std::unique_ptr<Mob> mob = createMobForDepth(depth);
        if (!mob) continue;

        mob->place(cell);
        mob->state = (Random::Int(2) == 0) ? Mob::State::SLEEPING : Mob::State::WANDERING;
        Actor::occupyCell(mob.get());
        Actor::add(mob.get(), Random::Float(1.0f));
        mobs.push_back(std::move(mob));
    }
}

std::unique_ptr<Mob> Dungeon::createMobForDepth(int depth) {
    if (depth <= 1) {
        return std::make_unique<Rat>();
    } else if (depth <= 2) {
        return (Random::Int(2) == 0) ? std::unique_ptr<Mob>(std::make_unique<Rat>()) : std::unique_ptr<Mob>(std::make_unique<Gnoll>());
    } else if (depth <= 4) {
        int r = Random::Int(5);
        if (r == 0) return std::make_unique<Rat>();
        else if (r == 1) return std::make_unique<Gnoll>();
        else if (r == 2) return std::make_unique<Crab>();
        else if (r == 3) return std::make_unique<Swarm>();
        else return std::make_unique<Thief>();
    } else if (depth <= 9) {
        // Prison: Skeletons, Thieves, Shamans, Swarms, Bats
        int r = Random::Int(5);
        if (r == 0) return std::make_unique<Skeleton>();
        else if (r == 1) return std::make_unique<Thief>();
        else if (r == 2) return std::make_unique<Shaman>();
        else if (r == 3) return std::make_unique<Swarm>();
        else return std::make_unique<Bat>();
    } else if (depth <= 14) {
        // Caves: Bats, Brutes, Shamans, Monks
        int r = Random::Int(4);
        if (r == 0) return std::make_unique<Bat>();
        else if (r == 1) return std::make_unique<Brute>();
        else if (r == 2) return std::make_unique<Shaman>();
        else return std::make_unique<Monk>();
    } else if (depth <= 19) {
        // Metropolis: Monks, Warlocks, Golems, Succubus
        int r = Random::Int(4);
        if (r == 0) return std::make_unique<Monk>();
        else if (r == 1) return std::make_unique<Warlock>();
        else if (r == 2) return std::make_unique<Golem>();
        else return std::make_unique<Succubus>();
    } else {
        // Demon Halls: Succubus, Evil Eye, Scorpio
        int r = Random::Int(3);
        if (r == 0) return std::make_unique<Succubus>();
        else if (r == 1) return std::make_unique<Eye>();
        else return std::make_unique<Scorpio>();
    }
}

void Dungeon::observe() {
    if (!hero || !level) return;

    level->updateFieldOfView(hero->pos, hero->viewDistance);
    std::copy_n(level->fieldOfView, Level::LENGTH, visible);

    bool mindVision = hero->buff(BuffType::MindVision) != nullptr;

    for (auto& mob : mobs) {
        if (mob) {
            bool wasVisible = mob->spriteVisible;
            bool isVis = visible[mob->pos];
            if (hero->heroClass == Hero::HeroClass::Huntress && mob->isAlive()) {
                if (PathFinder::distanceChebyshev(hero->pos, mob->pos) <= 2) {
                    isVis = true;
                    visible[mob->pos] = true;
                }
            }
            if (mindVision && mob->isAlive()) {
                isVis = true;
                visible[mob->pos] = true;
            }
            mob->spriteVisible = isVis;
            if (!wasVisible && mob->spriteVisible && hero->restoreHealth) {
                hero->interrupt();
            }
        }
    }

    if (fog) {
        fog->updateVisibility(visible, level->visited, level->mapped);
    }
}

int Dungeon::findPath(Char* ch, int from, int to, const bool* pass, const bool* visible) {
    if (PathFinder::distanceChebyshev(from, to) <= 1) {
        if (!Actor::findChar(to) && (pass[to] || (level && level->avoid[to]))) {
            return to;
        }
        return -1;
    }

    static bool passableBuf[Level::LENGTH];
    if (ch && ch->flying) {
        for (int i = 0; i < Level::LENGTH; ++i) {
            passableBuf[i] = pass[i] || (level && level->avoid[i]);
        }
    } else {
        std::copy_n(pass, Level::LENGTH, passableBuf);
    }

    // Visible characters block pathfinding
    for (Actor* actor : Actor::all) {
        Char* other = actor->asChar();
        if (other && other != ch) {
            int p = other->pos;
            if (p >= 0 && p < Level::LENGTH && visible[p]) {
                passableBuf[p] = false;
            }
        }
    }

    return PathFinder::getStep(from, to, passableBuf);
}

int Dungeon::flee(Char* ch, int cur, int from, const bool* pass, const bool* visible) {
    static bool passableBuf[Level::LENGTH];
    if (ch && ch->flying) {
        for (int i = 0; i < Level::LENGTH; ++i) {
            passableBuf[i] = pass[i] || (level && level->avoid[i]);
        }
    } else {
        std::copy_n(pass, Level::LENGTH, passableBuf);
    }

    for (Actor* actor : Actor::all) {
        Char* other = actor->asChar();
        if (other && other != ch) {
            int p = other->pos;
            if (p >= 0 && p < Level::LENGTH && visible[p]) {
                passableBuf[p] = false;
            }
        }
    }
    passableBuf[cur] = true;

    return PathFinder::getStepBack(cur, from, passableBuf);
}

} // namespace pd
