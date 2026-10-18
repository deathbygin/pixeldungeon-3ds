#include "pd/SaveManager.h"
#include "pd/CharSprite.h"
#include "pd/Dungeon.h"
#include "pd/Hero.h"
#include "pd/Level.h"
#include "pd/Mob.h"
#include "pd/Item.h"
#include "pd/Belongings.h"
#include "pd/Actor.h"
#include "pd/Heap.h"
#include "pd/Random.h"
#include "pd/Badges.h"
#include "pd/Preferences.h"
#include "pd/FogOfWar.h"
#include "pd/Rankings.h"
#include "pd/Statistics.h"

#include <3ds.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <vector>

namespace pd {

aptHookCookie SaveManager::s_aptCookie{};
bool SaveManager::s_aptHookRegistered = false;
bool SaveManager::s_romfsMounted = false;

void SaveManager::init() {
    initRomfs();

    if (!s_aptHookRegistered) {
        aptHook(&s_aptCookie, onAptHook, nullptr);
        s_aptHookRegistered = true;
    }
}

void SaveManager::exit() {
    if (s_aptHookRegistered) {
        aptUnhook(&s_aptCookie);
        s_aptHookRegistered = false;
    }

    exitRomfs();
}

bool SaveManager::initRomfs() {
    if (!s_romfsMounted) {
        Result rc = romfsInit();
        if (R_SUCCEEDED(rc)) {
            s_romfsMounted = true;
            return true;
        }
        return false;
    }
    return true;
}

void SaveManager::exitRomfs() {
    if (s_romfsMounted) {
        romfsExit();
        s_romfsMounted = false;
    }
}

void SaveManager::onAptHook(APT_HookType hook, void* param) {
    (void)param;
    // Sleep or suspend triggered (e.g. clamshell lid closed, home button pressed)
    if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP) {
        if (Dungeon::hero && Dungeon::hero->isAlive()) {
            saveGame();
        }
    }
}

bool SaveManager::ensureDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    int res = mkdir(path.c_str(), 0777);
    return (res == 0 || stat(path.c_str(), &st) == 0);
}

std::string SaveManager::getSaveDirectory() {
    static std::string chosenDir = "";
    if (!chosenDir.empty()) return chosenDir;

    // Check / create primary SDMC path: sdmc:/3ds/PixelDungeon/
    ensureDirectory("sdmc:/3ds");
    if (ensureDirectory("sdmc:/3ds/PixelDungeon")) {
        chosenDir = "sdmc:/3ds/PixelDungeon/";
        return chosenDir;
    }

    // Fallback to local directory for emulator or standalone environments
    chosenDir = "./";
    return chosenDir;
}

std::string SaveManager::getSavePath() {
    return getSaveDirectory() + "savegame.dat";
}

bool SaveManager::hasSave(const std::string& customPath) {
    std::string path = customPath.empty() ? getSavePath() : customPath;
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp && customPath.empty()) {
        std::string altPath = getSaveDirectory() + "game.dat";
        fp = std::fopen(altPath.c_str(), "rb");
    }
    if (!fp) return false;

    uint32_t magic = 0;
    uint32_t version = 0;
    size_t r1 = std::fread(&magic, sizeof(uint32_t), 1, fp);
    size_t r2 = std::fread(&version, sizeof(uint32_t), 1, fp);
    std::fclose(fp);

    return (r1 == 1 && r2 == 1 && magic == SAVE_MAGIC && version >= 1 && version <= SAVE_VERSION);
}

bool SaveManager::deleteSave(const std::string& customPath) {
    std::string path = customPath.empty() ? getSavePath() : customPath;
    bool res = (std::remove(path.c_str()) == 0);
    if (customPath.empty()) {
        std::string altPath = getSaveDirectory() + "game.dat";
        std::remove(altPath.c_str());
    }
    return res;
}

void SaveManager::wipeAllData() {
    std::string dir = getSaveDirectory();
    std::remove((dir + "savegame.dat").c_str());
    std::remove((dir + "game.dat").c_str());
    std::remove((dir + "badges.dat").c_str());
    std::remove((dir + "rankings.dat").c_str());
    std::remove((dir + "settings.dat").c_str());
    std::remove("savegame.dat");
    std::remove("game.dat");
    std::remove("badges.dat");
    std::remove("rankings.dat");
    std::remove("settings.dat");

    for (int i = 1; i <= 30; ++i) {
        std::string depthFile = "depth" + std::to_string(i) + ".dat";
        std::remove((dir + depthFile).c_str());
        std::remove(depthFile.c_str());
    }

    // Reset badges
    Badges::reset();
    Rankings::instance().reset();
    Statistics::reset();

    // Reset preferences
    Preferences::resetDefaults();

    // Clear in-memory game state
    Dungeon::hero.reset();
    Dungeon::level.reset();
    Dungeon::mobs.clear();
    Dungeon::fog.reset();
    Dungeon::droppedItems.clear();
    Dungeon::depth = 1;
    Dungeon::gold = 0;
}

bool SaveManager::getSaveSummary(int& outDepth, int& outLevel, int& outHeroClass, const std::string& customPath) {
    std::string path = customPath.empty() ? getSavePath() : customPath;
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp && customPath.empty()) {
        std::string altPath = getSaveDirectory() + "game.dat";
        fp = std::fopen(altPath.c_str(), "rb");
    }
    if (!fp) return false;

    uint32_t magic = 0;
    uint32_t version = 0;
    if (std::fread(&magic, sizeof(uint32_t), 1, fp) != 1 ||
        std::fread(&version, sizeof(uint32_t), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }

    if (magic != SAVE_MAGIC || (version != 6 && version != 7 && version != SAVE_VERSION)) {
        std::fclose(fp);
        return false;
    }

    int32_t depth = 1;
    int32_t gold = 0;
    uint64_t seed = 0;
    std::fread(&depth, sizeof(int32_t), 1, fp);
    std::fread(&gold, sizeof(int32_t), 1, fp);
    std::fread(&seed, sizeof(uint64_t), 1, fp);
    if (version >= 8) {
        uint8_t dVial = 0;
        std::fread(&dVial, sizeof(uint8_t), 1, fp);
    }

    if (version >= 3) {
        // Skip potion map (POTION_COUNT * (sizeof(int32_t) + sizeof(uint8_t)))
        // and scroll map (SCROLL_COUNT * (sizeof(int32_t) + sizeof(uint8_t)))
        long skipBytes = Potion::POTION_COUNT * (sizeof(int32_t) + sizeof(uint8_t)) +
                         Scroll::RUNIC_SCROLL_COUNT * (sizeof(int32_t) + sizeof(uint8_t));
        if (version >= 5) {
            skipBytes += 12 * (sizeof(int32_t) + sizeof(uint8_t));
        }
        if (version >= 6) {
            skipBytes += 12 * sizeof(uint8_t);
        }
        std::fseek(fp, skipBytes, SEEK_CUR);
    }

    int32_t heroClass = 0;
    std::fread(&heroClass, sizeof(int32_t), 1, fp);
    if (version >= 7) {
        int32_t subClass = 0;
        std::fread(&subClass, sizeof(int32_t), 1, fp);
    }
    int32_t pos = 0;
    int32_t hp = 20;
    int32_t ht = 20;
    int32_t str = 10;
    int32_t exp = 0;
    int32_t lvl = 1;

    std::fread(&pos, sizeof(int32_t), 1, fp);
    std::fread(&hp, sizeof(int32_t), 1, fp);
    std::fread(&ht, sizeof(int32_t), 1, fp);
    std::fread(&str, sizeof(int32_t), 1, fp);
    std::fread(&exp, sizeof(int32_t), 1, fp);
    std::fread(&lvl, sizeof(int32_t), 1, fp);

    std::fclose(fp);

    outDepth = depth;
    outLevel = lvl;
    outHeroClass = heroClass;
    return true;
}

bool SaveManager::saveGame(const std::string& customPath) {
    if (!Dungeon::hero || !Dungeon::level) return false;

    Hero* hero = Dungeon::hero.get();
    Level* level = Dungeon::level.get();
    if (!hero->isAlive()) return false;

    std::string path = customPath.empty() ? getSavePath() : customPath;
    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp) return false;

    // 1. Header
    uint32_t magic = SAVE_MAGIC;
    uint32_t version = SAVE_VERSION;
    std::fwrite(&magic, sizeof(uint32_t), 1, fp);
    std::fwrite(&version, sizeof(uint32_t), 1, fp);

    // 2. Dungeon State
    int32_t depth = Dungeon::depth;
    int32_t gold = Dungeon::gold;
    uint64_t seed = Random::getSeed();
    std::fwrite(&depth, sizeof(int32_t), 1, fp);
    std::fwrite(&gold, sizeof(int32_t), 1, fp);
    std::fwrite(&seed, sizeof(uint64_t), 1, fp);
    uint8_t dVial = Dungeon::dewVial ? 1 : 0;
    std::fwrite(&dVial, sizeof(uint8_t), 1, fp);

    // Save Potion & Scroll mappings & known tables
    const auto& potMap = Potion::getColorMapping();
    const auto& potKnown = Potion::getKnownTable();
    for (size_t i = 0; i < Potion::POTION_COUNT; ++i) {
        int32_t m = (i < potMap.size()) ? potMap[i] : static_cast<int32_t>(i);
        uint8_t k = (i < potKnown.size() && potKnown[i]) ? 1 : 0;
        std::fwrite(&m, sizeof(int32_t), 1, fp);
        std::fwrite(&k, sizeof(uint8_t), 1, fp);
    }
    const auto& scrMap = Scroll::getRuneMapping();
    const auto& scrKnown = Scroll::getKnownTable();
    for (size_t i = 0; i < Scroll::RUNIC_SCROLL_COUNT; ++i) {
        int32_t m = (i < scrMap.size()) ? scrMap[i] : static_cast<int32_t>(i);
        uint8_t k = (i < scrKnown.size() && scrKnown[i]) ? 1 : 0;
        std::fwrite(&m, sizeof(int32_t), 1, fp);
        std::fwrite(&k, sizeof(uint8_t), 1, fp);
    }
    const auto& wandMap = Wand::getWoodMapping();
    const auto& wandKnown = Wand::getKnownTable();
    for (size_t i = 0; i < 12; ++i) {
        int32_t m = (i < wandMap.size()) ? wandMap[i] : static_cast<int32_t>(i);
        uint8_t k = (i < wandKnown.size() && wandKnown[i]) ? 1 : 0;
        std::fwrite(&m, sizeof(int32_t), 1, fp);
        std::fwrite(&k, sizeof(uint8_t), 1, fp);
    }
    const auto& ringKnown = Ring::getKnownTable();
    for (size_t i = 0; i < 12; ++i) {
        uint8_t k = (i < ringKnown.size() && ringKnown[i]) ? 1 : 0;
        std::fwrite(&k, sizeof(uint8_t), 1, fp);
    }

    // 3. Hero State
    int32_t heroClass = static_cast<int32_t>(hero->heroClass);
    int32_t subClass = static_cast<int32_t>(hero->subClass);
    int32_t pos = hero->pos;
    int32_t hp = hero->HP;
    int32_t ht = hero->HT;
    int32_t str = hero->STR;
    int32_t exp = hero->exp;
    int32_t lvl = hero->level;
    int32_t maxExp = hero->maxExp;
    int32_t hGold = hero->gold;
    int32_t invisible = hero->invisible;
    uint8_t paralysed = hero->paralysed ? 1 : 0;
    uint8_t rooted = hero->rooted ? 1 : 0;

    std::fwrite(&heroClass, sizeof(int32_t), 1, fp);
    std::fwrite(&subClass, sizeof(int32_t), 1, fp);
    std::fwrite(&pos, sizeof(int32_t), 1, fp);
    std::fwrite(&hp, sizeof(int32_t), 1, fp);
    std::fwrite(&ht, sizeof(int32_t), 1, fp);
    std::fwrite(&str, sizeof(int32_t), 1, fp);
    std::fwrite(&exp, sizeof(int32_t), 1, fp);
    std::fwrite(&lvl, sizeof(int32_t), 1, fp);
    std::fwrite(&maxExp, sizeof(int32_t), 1, fp);
    std::fwrite(&hGold, sizeof(int32_t), 1, fp);
    std::fwrite(&invisible, sizeof(int32_t), 1, fp);
    std::fwrite(&paralysed, sizeof(uint8_t), 1, fp);
    std::fwrite(&rooted, sizeof(uint8_t), 1, fp);

    // 4. Inventory & Belongings
    std::vector<const Item*> allItems;
    if (hero->belongings.weaponSlot) allItems.push_back(hero->belongings.weaponSlot.get());
    if (hero->belongings.armorSlot) allItems.push_back(hero->belongings.armorSlot.get());
    if (hero->belongings.ring1Slot) allItems.push_back(hero->belongings.ring1Slot.get());
    if (hero->belongings.ring2Slot) allItems.push_back(hero->belongings.ring2Slot.get());
    for (const auto& it : hero->belongings.backpack) {
        if (it) allItems.push_back(it.get());
    }
    if (hero->belongings.keyring) {
        for (const auto& k : hero->belongings.keyring->keys) {
            if (k) allItems.push_back(k.get());
        }
    }

    uint32_t itemCount = static_cast<uint32_t>(allItems.size());
    std::fwrite(&itemCount, sizeof(uint32_t), 1, fp);

    for (const auto* it : allItems) {
        uint32_t category = static_cast<uint32_t>(it->category());
        std::fwrite(&category, sizeof(uint32_t), 1, fp);

        uint32_t nameLen = static_cast<uint32_t>(it->name.length());
        std::fwrite(&nameLen, sizeof(uint32_t), 1, fp);
        if (nameLen > 0) {
            std::fwrite(it->name.data(), sizeof(char), nameLen, fp);
        }

        int32_t iLvl = it->level;
        if (const DewVial* dv = const_cast<Item*>(it)->asDewVial()) {
            iLvl = dv->volume;
        }
        int32_t iQty = it->quantity;
        uint8_t identified = it->identified ? 1 : 0;
        uint8_t cursed = it->cursed ? 1 : 0;
        uint8_t isEquippedWeapon = (it == hero->belongings.equippedWeapon) ? 1 :
                                   (it == hero->belongings.equippedRing1) ? 2 :
                                   (it == hero->belongings.equippedRing2) ? 3 : 0;
        uint8_t isEquippedArmor = (it == hero->belongings.equippedArmor) ? 1 : 0;
        uint8_t levelKnown = it->levelKnown ? 1 : 0;
        uint8_t cursedKnown = it->cursedKnown ? 1 : 0;

        std::fwrite(&iLvl, sizeof(int32_t), 1, fp);
        std::fwrite(&iQty, sizeof(int32_t), 1, fp);
        std::fwrite(&identified, sizeof(uint8_t), 1, fp);
        std::fwrite(&cursed, sizeof(uint8_t), 1, fp);
        std::fwrite(&isEquippedWeapon, sizeof(uint8_t), 1, fp);
        std::fwrite(&isEquippedArmor, sizeof(uint8_t), 1, fp);
        std::fwrite(&levelKnown, sizeof(uint8_t), 1, fp);
        std::fwrite(&cursedKnown, sizeof(uint8_t), 1, fp);

        // Subcategory payloads
        if (it->category() == ItemCategory::Weapon) {
            const Weapon* w = static_cast<const Weapon*>(it);
            int32_t minD = w->minDamage;
            int32_t maxD = w->maxDamage;
            int32_t sReq = w->STR;
            int32_t htk = w->hitsToKnow;
            if (const Wand* wand = const_cast<Item*>(it)->asWand()) {
                htk = wand->usagesToKnow;
            }
            std::fwrite(&minD, sizeof(int32_t), 1, fp);
            std::fwrite(&maxD, sizeof(int32_t), 1, fp);
            std::fwrite(&sReq, sizeof(int32_t), 1, fp);
            std::fwrite(&htk, sizeof(int32_t), 1, fp);
        } else if (it->category() == ItemCategory::Armor) {
            const Armor* a = static_cast<const Armor*>(it);
            int32_t tier = a->tier;
            int32_t sReq = a->STR;
            int32_t htk = a->hitsToKnow;
            std::fwrite(&tier, sizeof(int32_t), 1, fp);
            std::fwrite(&sReq, sizeof(int32_t), 1, fp);
            std::fwrite(&htk, sizeof(int32_t), 1, fp);
        } else if (it->category() == ItemCategory::Potion) {
            const Potion* p = static_cast<const Potion*>(it);
            int32_t pType = static_cast<int32_t>(p->type);
            std::fwrite(&pType, sizeof(int32_t), 1, fp);
        } else if (it->category() == ItemCategory::Scroll) {
            const Scroll* s = static_cast<const Scroll*>(it);
            int32_t sType = static_cast<int32_t>(s->type);
            std::fwrite(&sType, sizeof(int32_t), 1, fp);
        } else if (it->category() == ItemCategory::Ring) {
            const Ring* r = const_cast<Item*>(it)->asRing();
            int32_t gem = r ? static_cast<int32_t>(r->gem) : 0;
            int32_t ttk = r ? r->ticksToKnow : Ring::TICKS_TO_KNOW;
            std::fwrite(&gem, sizeof(int32_t), 1, fp);
            std::fwrite(&ttk, sizeof(int32_t), 1, fp);
        }
    }

    // 5. Level State
    int32_t width = Level::WIDTH;
    int32_t height = Level::HEIGHT;
    int32_t entrance = level->entrance;
    int32_t exit = level->exit;
    int32_t feeling = static_cast<int32_t>(level->feeling);
    std::fwrite(&width, sizeof(int32_t), 1, fp);
    std::fwrite(&height, sizeof(int32_t), 1, fp);
    std::fwrite(&entrance, sizeof(int32_t), 1, fp);
    std::fwrite(&exit, sizeof(int32_t), 1, fp);
    std::fwrite(&feeling, sizeof(int32_t), 1, fp);

    // Map tiles
    std::fwrite(level->map, sizeof(int), Level::LENGTH, fp);

    // Visited flags
    uint8_t visitedBuf[Level::LENGTH];
    for (int i = 0; i < Level::LENGTH; ++i) {
        visitedBuf[i] = level->visited[i] ? 1 : 0;
    }
    std::fwrite(visitedBuf, sizeof(uint8_t), Level::LENGTH, fp);

    // Rooms
    uint32_t roomCount = static_cast<uint32_t>(level->rooms.size());
    std::fwrite(&roomCount, sizeof(uint32_t), 1, fp);
    for (const auto& r : level->rooms) {
        if (!r) continue;
        int32_t left = r->left;
        int32_t top = r->top;
        int32_t right = r->right;
        int32_t bottom = r->bottom;
        int32_t rType = static_cast<int32_t>(r->type);
        std::fwrite(&left, sizeof(int32_t), 1, fp);
        std::fwrite(&top, sizeof(int32_t), 1, fp);
        std::fwrite(&right, sizeof(int32_t), 1, fp);
        std::fwrite(&bottom, sizeof(int32_t), 1, fp);
        std::fwrite(&rType, sizeof(int32_t), 1, fp);
    }

    // 5b. Ground Heaps State
    uint32_t heapCount = static_cast<uint32_t>(level->heaps.size());
    std::fwrite(&heapCount, sizeof(uint32_t), 1, fp);
    for (const auto& h : level->heaps) {
        int32_t hPos = h->pos;
        int32_t hType = static_cast<int32_t>(h->type);
        uint32_t hItems = static_cast<uint32_t>(h->items.size());
        std::fwrite(&hPos, sizeof(int32_t), 1, fp);
        std::fwrite(&hType, sizeof(int32_t), 1, fp);
        std::fwrite(&hItems, sizeof(uint32_t), 1, fp);

        for (const auto& it : h->items) {
            uint32_t category = static_cast<uint32_t>(it->category());
            std::fwrite(&category, sizeof(uint32_t), 1, fp);

            uint32_t nameLen = static_cast<uint32_t>(it->name.length());
            std::fwrite(&nameLen, sizeof(uint32_t), 1, fp);
            if (nameLen > 0) {
                std::fwrite(it->name.data(), sizeof(char), nameLen, fp);
            }

            int32_t iLvl = it->level;
            if (const DewVial* dv = const_cast<Item*>(it.get())->asDewVial()) {
                iLvl = dv->volume;
            }
            int32_t iQty = it->quantity;
            uint8_t identified = it->identified ? 1 : 0;
            uint8_t cursed = it->cursed ? 1 : 0;
            uint8_t levelKnown = it->levelKnown ? 1 : 0;
            uint8_t cursedKnown = it->cursedKnown ? 1 : 0;
            std::fwrite(&iLvl, sizeof(int32_t), 1, fp);
            std::fwrite(&iQty, sizeof(int32_t), 1, fp);
            std::fwrite(&identified, sizeof(uint8_t), 1, fp);
            std::fwrite(&cursed, sizeof(uint8_t), 1, fp);
            std::fwrite(&levelKnown, sizeof(uint8_t), 1, fp);
            std::fwrite(&cursedKnown, sizeof(uint8_t), 1, fp);

            if (it->category() == ItemCategory::Weapon) {
                Weapon* w = static_cast<Weapon*>(it.get());
                int32_t minD = w->minDamage;
                int32_t maxD = w->maxDamage;
                int32_t strReq = w->STR;
                int32_t htk = w->hitsToKnow;
                if (const Wand* wand = it->asWand()) {
                    htk = wand->usagesToKnow;
                }
                std::fwrite(&minD, sizeof(int32_t), 1, fp);
                std::fwrite(&maxD, sizeof(int32_t), 1, fp);
                std::fwrite(&strReq, sizeof(int32_t), 1, fp);
                std::fwrite(&htk, sizeof(int32_t), 1, fp);
            } else if (it->category() == ItemCategory::Armor) {
                Armor* a = static_cast<Armor*>(it.get());
                int32_t tier = a->tier;
                int32_t strReq = a->STR;
                int32_t htk = a->hitsToKnow;
                std::fwrite(&tier, sizeof(int32_t), 1, fp);
                std::fwrite(&strReq, sizeof(int32_t), 1, fp);
                std::fwrite(&htk, sizeof(int32_t), 1, fp);
            } else if (it->category() == ItemCategory::Potion) {
                Potion* p = static_cast<Potion*>(it.get());
                int32_t pType = static_cast<int32_t>(p->type);
                std::fwrite(&pType, sizeof(int32_t), 1, fp);
            } else if (it->category() == ItemCategory::Scroll) {
                Scroll* s = static_cast<Scroll*>(it.get());
                int32_t sType = static_cast<int32_t>(s->type);
                std::fwrite(&sType, sizeof(int32_t), 1, fp);
            } else if (it->category() == ItemCategory::Ring) {
                Ring* r = it->asRing();
                int32_t gem = r ? static_cast<int32_t>(r->gem) : 0;
                int32_t ttk = r ? r->ticksToKnow : Ring::TICKS_TO_KNOW;
                std::fwrite(&gem, sizeof(int32_t), 1, fp);
                std::fwrite(&ttk, sizeof(int32_t), 1, fp);
            }
        }
    }

    // 6. Active Living Mobs State
    std::vector<Mob*> livingMobs;
    for (const auto& m : Dungeon::mobs) {
        if (m && m->isAlive()) {
            livingMobs.push_back(m.get());
        }
    }

    uint32_t mobCount = static_cast<uint32_t>(livingMobs.size());
    std::fwrite(&mobCount, sizeof(uint32_t), 1, fp);

    for (Mob* m : livingMobs) {
        uint32_t mobType = static_cast<uint32_t>(m->mobType());
        int32_t mPos = m->pos;
        int32_t mHp = m->HP;
        int32_t mHt = m->HT;
        uint32_t mState = static_cast<uint32_t>(m->state);

        std::fwrite(&mobType, sizeof(uint32_t), 1, fp);
        std::fwrite(&mPos, sizeof(int32_t), 1, fp);
        std::fwrite(&mHp, sizeof(int32_t), 1, fp);
        std::fwrite(&mHt, sizeof(int32_t), 1, fp);
        std::fwrite(&mState, sizeof(uint32_t), 1, fp);
    }

    std::fclose(fp);

    if (customPath.empty()) {
        std::string gameDatPath = getSaveDirectory() + "game.dat";
        if (gameDatPath != path) {
            saveGame(gameDatPath);
        }
    }

    return true;
}

bool SaveManager::loadGame(const std::string& customPath) {
    std::string path = customPath.empty() ? getSavePath() : customPath;
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp && customPath.empty()) {
        std::string altPath = getSaveDirectory() + "game.dat";
        fp = std::fopen(altPath.c_str(), "rb");
    }
    if (!fp) return false;

    // 1. Header
    uint32_t magic = 0;
    uint32_t version = 0;
    if (std::fread(&magic, sizeof(uint32_t), 1, fp) != 1 ||
        std::fread(&version, sizeof(uint32_t), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }

    if (magic != SAVE_MAGIC || (version != 6 && version != 7 && version != SAVE_VERSION)) {
        std::fclose(fp);
        return false;
    }

    // Clear existing active game world
    Actor::clear();
    Dungeon::mobs.clear();

    // 2. Dungeon State
    int32_t depth = 1;
    int32_t gold = 0;
    uint64_t seed = 0;
    std::fread(&depth, sizeof(int32_t), 1, fp);
    std::fread(&gold, sizeof(int32_t), 1, fp);
    std::fread(&seed, sizeof(uint64_t), 1, fp);

    if (version >= 8) {
        uint8_t dVial = 1;
        std::fread(&dVial, sizeof(uint8_t), 1, fp);
        Dungeon::dewVial = (dVial != 0);
    } else {
        Dungeon::dewVial = true;
    }

    Dungeon::depth = depth;
    Dungeon::gold = gold;
    Random::seed(seed);

    if (version >= 3) {
        std::vector<int> potMap(Potion::POTION_COUNT);
        std::vector<bool> potKnown(Potion::POTION_COUNT);
        for (size_t i = 0; i < Potion::POTION_COUNT; ++i) {
            int32_t m = 0;
            uint8_t k = 0;
            std::fread(&m, sizeof(int32_t), 1, fp);
            std::fread(&k, sizeof(uint8_t), 1, fp);
            potMap[i] = m;
            potKnown[i] = (k != 0);
        }
        Potion::restoreStatus(potMap, potKnown);

        std::vector<int> scrMap(Scroll::RUNIC_SCROLL_COUNT);
        std::vector<bool> scrKnown(Scroll::RUNIC_SCROLL_COUNT);
        for (size_t i = 0; i < Scroll::RUNIC_SCROLL_COUNT; ++i) {
            int32_t m = 0;
            uint8_t k = 0;
            std::fread(&m, sizeof(int32_t), 1, fp);
            std::fread(&k, sizeof(uint8_t), 1, fp);
            scrMap[i] = m;
            scrKnown[i] = (k != 0);
        }
        Scroll::restoreStatus(scrMap, scrKnown);

        if (version >= 5) {
            std::vector<int> wandMap(12);
            std::vector<bool> wandKnown(12);
            for (size_t i = 0; i < 12; ++i) {
                int32_t m = 0;
                uint8_t k = 0;
                std::fread(&m, sizeof(int32_t), 1, fp);
                std::fread(&k, sizeof(uint8_t), 1, fp);
                wandMap[i] = m;
                wandKnown[i] = (k != 0);
            }
            Wand::restoreStatus(wandMap, wandKnown);
        } else {
            Wand::initWoods();
        }

        if (version >= 6) {
            std::vector<bool> ringKnown(12);
            for (size_t i = 0; i < 12; ++i) {
                uint8_t k = 0;
                std::fread(&k, sizeof(uint8_t), 1, fp);
                ringKnown[i] = (k != 0);
            }
            Ring::restoreStatus(ringKnown);
        } else {
            Ring::restoreStatus(std::vector<bool>(12, false));
        }
    } else {
        Potion::initColors();
        Scroll::initLabels();
        Wand::initWoods();
        Ring::restoreStatus(std::vector<bool>(12, false));
    }

    // 3. Hero State
    int32_t heroClass = 0;
    int32_t subClass = 0;
    int32_t pos = 0;
    int32_t hp = 20;
    int32_t ht = 20;
    int32_t str = 10;
    int32_t exp = 0;
    int32_t lvl = 1;
    int32_t maxExp = 5;
    int32_t hGold = 0;
    int32_t invisible = 0;
    uint8_t paralysed = 0;
    uint8_t rooted = 0;

    std::fread(&heroClass, sizeof(int32_t), 1, fp);
    if (version >= 7) {
        std::fread(&subClass, sizeof(int32_t), 1, fp);
    }
    std::fread(&pos, sizeof(int32_t), 1, fp);
    std::fread(&hp, sizeof(int32_t), 1, fp);
    std::fread(&ht, sizeof(int32_t), 1, fp);
    std::fread(&str, sizeof(int32_t), 1, fp);
    std::fread(&exp, sizeof(int32_t), 1, fp);
    std::fread(&lvl, sizeof(int32_t), 1, fp);
    std::fread(&maxExp, sizeof(int32_t), 1, fp);
    std::fread(&hGold, sizeof(int32_t), 1, fp);
    std::fread(&invisible, sizeof(int32_t), 1, fp);
    std::fread(&paralysed, sizeof(uint8_t), 1, fp);
    std::fread(&rooted, sizeof(uint8_t), 1, fp);

    Dungeon::hero = std::make_unique<Hero>();
    Hero* hero = Dungeon::hero.get();
    hero->heroClass = static_cast<Hero::HeroClass>(heroClass);
    hero->subClass = static_cast<HeroSubClass>(subClass);
    hero->pos = pos;
    hero->HP = hp;
    hero->HT = ht;
    hero->STR = str;
    hero->exp = exp;
    hero->level = lvl;
    hero->maxExp = maxExp;
    hero->gold = hGold;
    hero->invisible = invisible;
    hero->paralysed = (paralysed != 0);
    hero->rooted = (rooted != 0);
    hero->place(pos);
    Actor::occupyCell(hero);
    Actor::add(hero);

    // 4. Inventory & Belongings
    hero->belongings.backpack.clear();
    if (hero->belongings.keyring) {
        hero->belongings.keyring->keys.clear();
    } else {
        hero->belongings.keyring = std::make_unique<Keyring>();
    }
    hero->belongings.weaponSlot.reset();
    hero->belongings.armorSlot.reset();
    hero->belongings.ring1Slot.reset();
    hero->belongings.ring2Slot.reset();
    hero->belongings.equippedWeapon = nullptr;
    hero->belongings.equippedArmor = nullptr;
    hero->belongings.equippedRing1 = nullptr;
    hero->belongings.equippedRing2 = nullptr;

    uint32_t itemCount = 0;
    std::fread(&itemCount, sizeof(uint32_t), 1, fp);

    for (uint32_t i = 0; i < itemCount; ++i) {
        uint32_t category = 0;
        uint32_t nameLen = 0;
        std::fread(&category, sizeof(uint32_t), 1, fp);
        std::fread(&nameLen, sizeof(uint32_t), 1, fp);

        std::string iName = "";
        if (nameLen > 0 && nameLen < 256) {
            std::vector<char> nameChars(nameLen + 1, 0);
            std::fread(nameChars.data(), sizeof(char), nameLen, fp);
            iName = std::string(nameChars.data());
        }

        int32_t iLvl = 0;
        int32_t iQty = 1;
        uint8_t identified = 0;
        uint8_t cursed = 0;
        uint8_t isEquippedWeapon = 0;
        uint8_t isEquippedArmor = 0;
        uint8_t levelKnown = 0;
        uint8_t cursedKnown = 0;

        std::fread(&iLvl, sizeof(int32_t), 1, fp);
        std::fread(&iQty, sizeof(int32_t), 1, fp);
        std::fread(&identified, sizeof(uint8_t), 1, fp);
        std::fread(&cursed, sizeof(uint8_t), 1, fp);
        std::fread(&isEquippedWeapon, sizeof(uint8_t), 1, fp);
        std::fread(&isEquippedArmor, sizeof(uint8_t), 1, fp);
        if (version >= 3) {
            std::fread(&levelKnown, sizeof(uint8_t), 1, fp);
            std::fread(&cursedKnown, sizeof(uint8_t), 1, fp);
        }

        ItemCategory cat = static_cast<ItemCategory>(category);
        std::unique_ptr<Item> item = nullptr;

        if (cat == ItemCategory::Weapon) {
            int32_t minD = 1, maxD = 6, sReq = 10, htk = 20;
            std::fread(&minD, sizeof(int32_t), 1, fp);
            std::fread(&maxD, sizeof(int32_t), 1, fp);
            std::fread(&sReq, sizeof(int32_t), 1, fp);
            if (version >= 3) {
                std::fread(&htk, sizeof(int32_t), 1, fp);
            }
            auto wand = Wand::createByName(iName);
            if (wand) {
                wand->usagesToKnow = htk;
                item = std::move(wand);
            } else {
                auto weapon = Weapon::createByName(iName);
                if (!weapon) weapon = std::make_unique<Weapon>(iName, minD, maxD, sReq);
                weapon->hitsToKnow = htk;
                weapon->STR = sReq;
                item = std::move(weapon);
            }
        } else if (cat == ItemCategory::Armor) {
            int32_t tier = 1, sReq = 11, htk = 10;
            std::fread(&tier, sizeof(int32_t), 1, fp);
            std::fread(&sReq, sizeof(int32_t), 1, fp);
            if (version >= 3) {
                std::fread(&htk, sizeof(int32_t), 1, fp);
            }
            auto armor = std::make_unique<Armor>(iName, tier, sReq);
            armor->hitsToKnow = htk;
            item = std::move(armor);
        } else if (cat == ItemCategory::Potion) {
            int32_t pType = 0;
            std::fread(&pType, sizeof(int32_t), 1, fp);
            item = std::make_unique<Potion>(static_cast<Potion::PotionType>(pType));
        } else if (cat == ItemCategory::Scroll) {
            int32_t sType = 0;
            std::fread(&sType, sizeof(int32_t), 1, fp);
            item = std::make_unique<Scroll>(static_cast<Scroll::ScrollType>(sType));
        } else if (cat == ItemCategory::Ring) {
            int32_t gem = 0, ttk = Ring::TICKS_TO_KNOW;
            if (version >= 6) {
                std::fread(&gem, sizeof(int32_t), 1, fp);
                std::fread(&ttk, sizeof(int32_t), 1, fp);
            }
            std::string eff = "Mending";
            if (iName.rfind("Ring of ", 0) == 0) {
                eff = iName.substr(8);
            }
            auto ring = std::make_unique<Ring>(static_cast<Ring::Gem>(gem), eff);
            ring->ticksToKnow = ttk;
            item = std::move(ring);
        } else {
            if (iName == "Ration of Food" || iName == "Food") {
                item = std::make_unique<Food>();
            } else if (iName == "Dewdrop") {
                item = std::make_unique<Dewdrop>();
            } else if (iName == "Gold") {
                item = std::make_unique<Gold>(iQty);
            } else if (iName == "Torch") {
                item = std::make_unique<Torch>();
            } else if (iName == "Golden Key") {
                item = std::make_unique<GoldenKey>();
            } else if (iName == "Iron Key") {
                item = std::make_unique<IronKey>();
            } else if (iName == "Skeleton Key") {
                item = std::make_unique<SkeletonKey>();
            } else if (iName == "Armor Kit") {
                item = std::make_unique<ArmorKit>();
            } else if (iName == "Tome of Mastery" || iName == "Tome of Remastery") {
                item = std::make_unique<TomeOfMastery>();
            } else if (iName == "Amulet of Yendor") {
                item = std::make_unique<Amulet>();
            } else if (iName == "Honeypot" || iName == "honeypot") {
                item = std::make_unique<Honeypot>();
            } else if (iName == "Bomb" || iName == "bomb") {
                item = std::make_unique<Bomb>();
            } else if (iName == "Dew Vial" || iName == "dew vial") {
                item = std::make_unique<DewVial>();
            } else {
                item = std::make_unique<Item>();
            }
        }

        if (item) {
            item->name = iName;
            item->level = iLvl;
            item->quantity = iQty;
            item->identified = (identified != 0);
            item->cursed = (cursed != 0);
            if (version >= 3) {
                item->levelKnown = (levelKnown != 0);
                item->cursedKnown = (cursedKnown != 0);
            } else {
                item->levelKnown = item->identified;
                item->cursedKnown = item->identified;
            }
            if (Key* k = item->asKey()) {
                k->depth = (iLvl > 0) ? iLvl : Dungeon::depth;
            } else if (DewVial* dv = item->asDewVial()) {
                dv->volume = std::clamp<int>(iLvl, 0, DewVial::MAX_VOLUME);
            }
            if (item->category() == ItemCategory::Potion) {
                static_cast<Potion*>(item.get())->updatePerception();
            } else if (item->category() == ItemCategory::Scroll) {
                static_cast<Scroll*>(item.get())->updatePerception();
            } else if (item->asWand()) {
                item->asWand()->updatePerception();
            }
            if (hero->heroClass == Hero::HeroClass::Warrior) {
                if (ShortSword* ss = item->asShortSword()) {
                    ss->isWarriorStarter = true;
                }
            }
            if (isEquippedWeapon == 1 && item->category() == ItemCategory::Weapon && !item->asWand()) {
                hero->belongings.weaponSlot = std::unique_ptr<Weapon>(static_cast<Weapon*>(item.release()));
                hero->belongings.equippedWeapon = hero->belongings.weaponSlot.get();
            } else if (isEquippedArmor == 1 && item->category() == ItemCategory::Armor) {
                hero->belongings.armorSlot = std::unique_ptr<Armor>(static_cast<Armor*>(item.release()));
                hero->belongings.equippedArmor = hero->belongings.armorSlot.get();
            } else if (isEquippedWeapon == 2) {
                hero->belongings.ring1Slot = std::move(item);
                hero->belongings.equippedRing1 = hero->belongings.ring1Slot.get();
            } else if (isEquippedWeapon == 3) {
                hero->belongings.ring2Slot = std::move(item);
                hero->belongings.equippedRing2 = hero->belongings.ring2Slot.get();
            } else if (item->asKey()) {
                if (hero->belongings.keyring) {
                    std::unique_ptr<Key> keyPtr(static_cast<Key*>(item.release()));
                    hero->belongings.keyring->addKey(std::move(keyPtr));
                } else {
                    hero->belongings.backpack.push_back(std::move(item));
                }
            } else {
                hero->belongings.backpack.push_back(std::move(item));
            }
        }
    }

    for (const auto& bIt : hero->belongings.backpack) {
        if (bIt && bIt->asDewVial()) {
            Dungeon::dewVial = false;
            break;
        }
    }

    // 5. Level State
    Dungeon::level = Level::create(Dungeon::depth);
    Level* level = Dungeon::level.get();

    int32_t width = 0;
    int32_t height = 0;
    int32_t entrance = 0;
    int32_t exit = 0;
    std::fread(&width, sizeof(int32_t), 1, fp);
    std::fread(&height, sizeof(int32_t), 1, fp);
    std::fread(&entrance, sizeof(int32_t), 1, fp);
    std::fread(&exit, sizeof(int32_t), 1, fp);
    if (version >= 4) {
        int32_t feeling = 0;
        std::fread(&feeling, sizeof(int32_t), 1, fp);
        level->feeling = static_cast<Level::Feeling>(feeling);
    }

    level->entrance = entrance;
    level->exit = exit;

    // Map tiles
    std::fread(level->map, sizeof(int), Level::LENGTH, fp);

    // Visited flags
    uint8_t visitedBuf[Level::LENGTH];
    std::fread(visitedBuf, sizeof(uint8_t), Level::LENGTH, fp);
    for (int i = 0; i < Level::LENGTH; ++i) {
        level->visited[i] = (visitedBuf[i] != 0);
    }

    // Rooms
    uint32_t roomCount = 0;
    std::fread(&roomCount, sizeof(uint32_t), 1, fp);
    level->rooms.clear();
    for (uint32_t r = 0; r < roomCount; ++r) {
        int32_t left = 0, top = 0, right = 0, bottom = 0, rType = 0;
        std::fread(&left, sizeof(int32_t), 1, fp);
        std::fread(&top, sizeof(int32_t), 1, fp);
        std::fread(&right, sizeof(int32_t), 1, fp);
        std::fread(&bottom, sizeof(int32_t), 1, fp);
        std::fread(&rType, sizeof(int32_t), 1, fp);
        level->rooms.push_back(std::make_shared<Room>(left, top, right, bottom, static_cast<Room::Type>(rType)));
    }

    // 5b. Ground Heaps State (v2+)
    level->heaps.clear();
    level->plants.clear();
    if (version >= 2) {
        uint32_t heapCount = 0;
        std::fread(&heapCount, sizeof(uint32_t), 1, fp);
        for (uint32_t hIdx = 0; hIdx < heapCount; ++hIdx) {
            int32_t hPos = 0;
            int32_t hType = 0;
            uint32_t hItems = 0;
            std::fread(&hPos, sizeof(int32_t), 1, fp);
            std::fread(&hType, sizeof(int32_t), 1, fp);
            std::fread(&hItems, sizeof(uint32_t), 1, fp);

            auto heap = std::make_unique<Heap>(hPos, static_cast<Heap::Type>(hType));
            for (uint32_t itIdx = 0; itIdx < hItems; ++itIdx) {
                uint32_t category = 0;
                uint32_t nameLen = 0;
                std::fread(&category, sizeof(uint32_t), 1, fp);
                std::fread(&nameLen, sizeof(uint32_t), 1, fp);

                std::string iName = "";
                if (nameLen > 0) {
                    std::vector<char> nameBuf(nameLen + 1, '\0');
                    std::fread(nameBuf.data(), sizeof(char), nameLen, fp);
                    iName = nameBuf.data();
                }

                int32_t iLvl = 0;
                int32_t iQty = 1;
                uint8_t identified = 0;
                uint8_t cursed = 0;
                uint8_t levelKnown = 0;
                uint8_t cursedKnown = 0;
                std::fread(&iLvl, sizeof(int32_t), 1, fp);
                std::fread(&iQty, sizeof(int32_t), 1, fp);
                std::fread(&identified, sizeof(uint8_t), 1, fp);
                std::fread(&cursed, sizeof(uint8_t), 1, fp);
                if (version >= 3) {
                    std::fread(&levelKnown, sizeof(uint8_t), 1, fp);
                    std::fread(&cursedKnown, sizeof(uint8_t), 1, fp);
                }

                ItemCategory cat = static_cast<ItemCategory>(category);
                std::unique_ptr<Item> item = nullptr;

                if (cat == ItemCategory::Weapon) {
                    int32_t minD = 1, maxD = 6, sReq = 10, htk = 20;
                    std::fread(&minD, sizeof(int32_t), 1, fp);
                    std::fread(&maxD, sizeof(int32_t), 1, fp);
                    std::fread(&sReq, sizeof(int32_t), 1, fp);
                    if (version >= 3) {
                        std::fread(&htk, sizeof(int32_t), 1, fp);
                    }
                    auto wand = Wand::createByName(iName);
                    if (wand) {
                        wand->usagesToKnow = htk;
                        item = std::move(wand);
                    } else {
                        auto weapon = Weapon::createByName(iName);
                        if (!weapon) weapon = std::make_unique<Weapon>(iName, minD, maxD, sReq);
                        weapon->hitsToKnow = htk;
                        weapon->STR = sReq;
                        item = std::move(weapon);
                    }
                } else if (cat == ItemCategory::Armor) {
                    int32_t tier = 1, sReq = 11, htk = 10;
                    std::fread(&tier, sizeof(int32_t), 1, fp);
                    std::fread(&sReq, sizeof(int32_t), 1, fp);
                    if (version >= 3) {
                        std::fread(&htk, sizeof(int32_t), 1, fp);
                    }
                    auto armor = std::make_unique<Armor>(iName, tier, sReq);
                    armor->hitsToKnow = htk;
                    item = std::move(armor);
                } else if (cat == ItemCategory::Potion) {
                    int32_t pType = 0;
                    std::fread(&pType, sizeof(int32_t), 1, fp);
                    item = std::make_unique<Potion>(static_cast<Potion::PotionType>(pType));
                } else if (cat == ItemCategory::Scroll) {
                    int32_t sType = 0;
                    std::fread(&sType, sizeof(int32_t), 1, fp);
                    item = std::make_unique<Scroll>(static_cast<Scroll::ScrollType>(sType));
                } else if (cat == ItemCategory::Ring) {
                    int32_t gem = 0, ttk = Ring::TICKS_TO_KNOW;
                    if (version >= 6) {
                        std::fread(&gem, sizeof(int32_t), 1, fp);
                        std::fread(&ttk, sizeof(int32_t), 1, fp);
                    }
                    std::string eff = "Mending";
                    if (iName.rfind("Ring of ", 0) == 0) {
                        eff = iName.substr(8);
                    }
                    auto ring = std::make_unique<Ring>(static_cast<Ring::Gem>(gem), eff);
                    ring->ticksToKnow = ttk;
                    item = std::move(ring);
                } else {
                    if (iName == "Ration of Food" || iName == "Food") {
                        item = std::make_unique<Food>();
                    } else if (iName == "Dewdrop") {
                        item = std::make_unique<Dewdrop>();
                    } else if (iName == "Gold") {
                        item = std::make_unique<Gold>(iQty);
                    } else if (iName == "Torch") {
                        item = std::make_unique<Torch>();
                    } else if (iName == "Golden Key") {
                        item = std::make_unique<GoldenKey>();
                    } else if (iName == "Iron Key") {
                        item = std::make_unique<IronKey>();
                    } else if (iName == "Skeleton Key") {
                        item = std::make_unique<SkeletonKey>();
                    } else if (iName == "Armor Kit") {
                        item = std::make_unique<ArmorKit>();
                    } else if (iName == "Tome of Mastery" || iName == "Tome of Remastery") {
                        item = std::make_unique<TomeOfMastery>();
                    } else if (iName == "Amulet of Yendor") {
                        item = std::make_unique<Amulet>();
                    } else if (iName == "Honeypot" || iName == "honeypot") {
                        item = std::make_unique<Honeypot>();
                    } else if (iName == "Bomb" || iName == "bomb") {
                        item = std::make_unique<Bomb>();
                    } else if (iName == "Dew Vial" || iName == "dew vial") {
                        item = std::make_unique<DewVial>();
                    } else {
                        item = std::make_unique<Item>();
                    }
                }

                if (item) {
                    item->name = iName;
                    item->level = iLvl;
                    item->quantity = iQty;
                    item->identified = (identified != 0);
                    item->cursed = (cursed != 0);
                    if (version >= 3) {
                        item->levelKnown = (levelKnown != 0);
                        item->cursedKnown = (cursedKnown != 0);
                    } else {
                        item->levelKnown = item->identified;
                        item->cursedKnown = item->identified;
                    }
                    if (Key* k = item->asKey()) {
                        k->depth = (iLvl > 0) ? iLvl : Dungeon::depth;
                    } else if (DewVial* dv = item->asDewVial()) {
                        dv->volume = std::clamp<int>(iLvl, 0, DewVial::MAX_VOLUME);
                    }
                    if (item->category() == ItemCategory::Potion) {
                        static_cast<Potion*>(item.get())->updatePerception();
                    } else if (item->category() == ItemCategory::Scroll) {
                        static_cast<Scroll*>(item.get())->updatePerception();
                    } else if (item->asWand()) {
                        item->asWand()->updatePerception();
                    }
                    heap->items.push_back(std::move(item));
                }
            }
            level->heaps.push_back(std::move(heap));
        }
    }

    level->buildFlags();

    // Add hero to Actor timeline
    Actor::add(hero);
    level->updateFieldOfView(hero->pos, hero->viewDistance);

    // 6. Active Living Mobs State
    uint32_t mobCount = 0;
    std::fread(&mobCount, sizeof(uint32_t), 1, fp);

    for (uint32_t m = 0; m < mobCount; ++m) {
        uint32_t mobType = 0;
        int32_t mPos = 0;
        int32_t mHp = 0;
        int32_t mHt = 0;
        uint32_t mState = 0;

        std::fread(&mobType, sizeof(uint32_t), 1, fp);
        std::fread(&mPos, sizeof(int32_t), 1, fp);
        std::fread(&mHp, sizeof(int32_t), 1, fp);
        std::fread(&mHt, sizeof(int32_t), 1, fp);
        std::fread(&mState, sizeof(uint32_t), 1, fp);

        std::unique_ptr<Mob> mob = nullptr;
        switch (static_cast<Mob::MobType>(mobType)) {
            case Mob::MobType::Rat:      mob = std::make_unique<Rat>(); break;
            case Mob::MobType::Gnoll:    mob = std::make_unique<Gnoll>(); break;
            case Mob::MobType::Crab:     mob = std::make_unique<Crab>(); break;
            case Mob::MobType::Skeleton: mob = std::make_unique<Skeleton>(); break;
            case Mob::MobType::Swarm:    mob = std::make_unique<Swarm>(); break;
            case Mob::MobType::Thief:    mob = std::make_unique<Thief>(); break;
            case Mob::MobType::Shaman:   mob = std::make_unique<Shaman>(); break;
            case Mob::MobType::Bat:      mob = std::make_unique<Bat>(); break;
            case Mob::MobType::Brute:    mob = std::make_unique<Brute>(); break;
            case Mob::MobType::Monk:     mob = std::make_unique<Monk>(); break;
            case Mob::MobType::Warlock:  mob = std::make_unique<Warlock>(); break;
            case Mob::MobType::Golem:    mob = std::make_unique<Golem>(); break;
            case Mob::MobType::Succubus: mob = std::make_unique<Succubus>(); break;
            case Mob::MobType::Eye:      mob = std::make_unique<Eye>(); break;
            case Mob::MobType::Scorpio:  mob = std::make_unique<Scorpio>(); break;
            case Mob::MobType::Piranha:  mob = std::make_unique<Piranha>(); break;
            case Mob::MobType::Statue:   mob = std::make_unique<Statue>(); break;
            case Mob::MobType::Wraith:   mob = std::make_unique<Wraith>(); break;
            case Mob::MobType::Goo:      mob = std::make_unique<Goo>(); break;
            case Mob::MobType::Tengu:    mob = std::make_unique<Tengu>(); break;
            case Mob::MobType::DM300:    mob = std::make_unique<DM300>(); break;
            case Mob::MobType::King:     mob = std::make_unique<King>(); break;
            case Mob::MobType::Yog:         mob = std::make_unique<Yog>(); break;
            case Mob::MobType::BurningFist: mob = std::make_unique<BurningFist>(); break;
            case Mob::MobType::RottingFist: mob = std::make_unique<RottingFist>(); break;
            case Mob::MobType::Mimic:       mob = std::make_unique<Mimic>(); break;
            case Mob::MobType::Bee:         mob = std::make_unique<Bee>(); break;
            default:                        mob = std::make_unique<Rat>(); break;
        }

        if (mob) {
            mob->place(mPos);
            mob->HP = mHp;
            mob->HT = mHt;
            mob->state = static_cast<Mob::State>(mState);
            Actor::add(mob.get(), Random::Float(1.0f));
            Actor::occupyCell(mob.get());
            Dungeon::mobs.push_back(std::move(mob));
        }
    }

    std::fclose(fp);
    return true;
}

} // namespace pd

