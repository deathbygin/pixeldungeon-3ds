#include "pd/Dungeon.h"
#include "pd/FogOfWar.h"
#include "pd/Random.h"
#include "pd/Badges.h"
#include "pd/GLog.h"
#include <algorithm>
#include <unordered_map>
#include <ctime>

namespace pd {

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
std::unordered_map<int, std::vector<std::unique_ptr<Item>>> Dungeon::droppedItems;

bool Dungeon::bossLevel(int d) {
    if (d == -1) d = depth;
    return d == 5 || d == 10 || d == 15 || d == 20 || d == 25;
}

void Dungeon::dropToChasm(std::unique_ptr<Item> item) {
    if (!item) return;
    int nextDepth = depth + 1;
    droppedItems[nextDepth].push_back(std::move(item));
}

void Dungeon::placeChasmItems() {
    auto it = droppedItems.find(depth);
    if (it != droppedItems.end() && level) {
        for (auto& item : it->second) {
            if (item) {
                int pos = level->randomDestination();
                level->drop(std::move(item), pos);
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
}

void Dungeon::announceLevelEntrance() {
    if (!level) return;

    if (depth < deepestFloor) {
        GLog::h("Welcome back to level " + std::to_string(depth) + " of Pixel Dungeon!");
    } else {
        GLog::h("Welcome to level " + std::to_string(depth) + " of Pixel Dungeon!");
        deepestFloor = std::max(deepestFloor, depth);
    }

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
    case Level::Feeling::DARK:
        GLog::w("The darkness here is oppressive and suffocating...");
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
    s_cachedLevels.clear();
    s_cachedMobs.clear();
    depth = 1;
    gold = 0;
    deepestFloor = 1;
    Potion::initColors();
    Scroll::initLabels();
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
    observe();
    announceLevelEntrance();
    Actor::process();
}

void Dungeon::switchLevel(int newDepth) {
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

        // Place hero at stairs
        int spawnCell = descending ? level->entrance : level->exit;
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

        int spawnPos = level->entrance;
        for (const auto& r : level->rooms) {
            if (r && r->type == Room::Type::PIT) {
                spawnPos = level->pitCell();
                break;
            }
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
        int cell = level->exit;
        auto dm = std::make_unique<DM300>();
        dm->place(cell);
        Actor::occupyCell(dm.get());
        Actor::add(dm.get(), 1.0f);
        mobs.push_back(std::move(dm));
        return;
    }

    if (depth == 20) {
        // Boss floor: King of Dwarves
        int cell = level->exit;
        auto king = std::make_unique<King>();
        king->place(cell);
        Actor::occupyCell(king.get());
        Actor::add(king.get(), 1.0f);
        mobs.push_back(std::move(king));
        return;
    }

    if (depth == 25) {
        // Boss floor: Yog-Dzewa
        int cell = level->exit;
        auto yog = std::make_unique<Yog>();
        yog->place(cell);
        Actor::occupyCell(yog.get());
        Actor::add(yog.get(), 1.0f);
        mobs.push_back(std::move(yog));
        return;
    }

    int mobCount = count;
    if (level && (level->feeling == Level::Feeling::DARK || (nightMode && !bossLevel()))) {
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

    if (fog) {
        fog->updateVisibility(visible, level->visited, level->mapped);
    }

    for (auto& mob : mobs) {
        if (mob) {
            bool wasVisible = mob->spriteVisible;
            bool isVis = visible[mob->pos];
            if (hero->heroClass == Hero::HeroClass::Huntress && mob->isAlive()) {
                if (PathFinder::distanceChebyshev(hero->pos, mob->pos) <= 2) {
                    isVis = true;
                }
            }
            mob->spriteVisible = isVis;
            if (!wasVisible && mob->spriteVisible && hero->restoreHealth) {
                hero->interrupt();
            }
        }
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
