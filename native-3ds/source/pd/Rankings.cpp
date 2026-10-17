#include "pd/Rankings.h"
#include "pd/Dungeon.h"
#include "pd/Statistics.h"
#include "pd/SaveManager.h"
#include "pd/Badges.h"
#include <cstdio>
#include <algorithm>
#include <cstring>

namespace pd {

Rankings& Rankings::instance() {
    static Rankings s_instance;
    return s_instance;
}

std::string Rankings::getFilePath() const {
#ifdef __3DS__
    return "sdmc:/3ds/PixelDungeon/rankings.dat";
#else
    return "rankings.dat";
#endif
}

int Rankings::score(bool win) {
    int deepest = std::max(1, Statistics::deepestFloor);
    int lvl = (Dungeon::hero) ? Dungeon::hero->level : 1;
    int base = Statistics::goldCollected + lvl * deepest * 100;
    return base * (win ? 2 : 1);
}

void Rankings::submit(bool win, const std::string& customReason) {
    load();

    Record rec;
    rec.win = win;

    if (!customReason.empty()) {
        rec.info = customReason;
    } else if (!Dungeon::resultDescription.empty()) {
        rec.info = Dungeon::resultDescription;
    } else if (win) {
        rec.info = "Obtained the Amulet of Yendor";
    } else {
        rec.info = "Died on level " + std::to_string(Dungeon::depth);
    }

    if (Dungeon::hero) {
        rec.heroClass = Dungeon::hero->heroClass;
        rec.subClass = Dungeon::hero->subClass;
        rec.armorTier = Dungeon::hero->tier();
        rec.level = Dungeon::hero->level;
        rec.str = Dungeon::hero->STR;
        rec.maxHp = Dungeon::hero->HT;

        const auto& b = Dungeon::hero->belongings;
        if (b.equippedWeapon) {
            rec.weaponName = b.equippedWeapon->name;
            rec.weaponImage = b.equippedWeapon->image;
        }
        if (b.equippedArmor) {
            rec.armorName = b.equippedArmor->name;
            rec.armorImage = b.equippedArmor->image;
        }
        if (b.equippedRing1) {
            rec.ring1Name = b.equippedRing1->name;
            rec.ring1Image = b.equippedRing1->image;
        }
        if (b.equippedRing2) {
            rec.ring2Name = b.equippedRing2->name;
            rec.ring2Image = b.equippedRing2->image;
        }
        for (const auto& item : b.backpack) {
            if (item && (item->asWand() || item->category() == ItemCategory::Potion || item->category() == ItemCategory::Scroll)) {
                rec.quickslotName = item->name;
                rec.quickslotImage = item->image;
                break;
            }
        }
    }

    rec.depth = std::max(1, Statistics::deepestFloor);
    rec.gold = Statistics::goldCollected;
    rec.duration = Statistics::duration;
    rec.enemiesSlain = Statistics::enemiesSlain;
    rec.foodEaten = Statistics::foodEaten;
    rec.potionsCooked = Statistics::potionsCooked;
    rec.ankhsUsed = Statistics::ankhsUsed;
    rec.score = score(win);

    records.push_back(rec);

    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.score > b.score;
    });

    lastRecord = -1;
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].score == rec.score && records[i].info == rec.info &&
            records[i].gold == rec.gold && records[i].level == rec.level) {
            lastRecord = static_cast<int>(i);
            break;
        }
    }

    if (records.size() > TABLE_SIZE) {
        records.resize(TABLE_SIZE);
        if (lastRecord >= static_cast<int>(TABLE_SIZE)) {
            lastRecord = -1;
        }
    }

    totalNumber++;
    if (win) {
        wonNumber++;
    }

    save();
}

static void writeString(FILE* fp, const std::string& str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    std::fwrite(&len, sizeof(uint16_t), 1, fp);
    if (len > 0) {
        std::fwrite(str.data(), 1, len, fp);
    }
}

static std::string readString(FILE* fp) {
    uint16_t len = 0;
    if (std::fread(&len, sizeof(uint16_t), 1, fp) != 1) return "";
    if (len == 0 || len > 2048) return "";
    std::string s(len, '\0');
    if (std::fread(&s[0], 1, len, fp) != len) return "";
    return s;
}

void Rankings::save() {
    std::string path = getFilePath();
    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp) {
        fp = std::fopen("rankings.dat", "wb");
    }
    if (!fp) return;

    uint32_t magic = RANK_MAGIC;
    uint32_t version = RANK_VERSION;
    std::fwrite(&magic, sizeof(uint32_t), 1, fp);
    std::fwrite(&version, sizeof(uint32_t), 1, fp);

    int32_t latest = lastRecord;
    int32_t total = totalNumber;
    int32_t won = wonNumber;
    uint32_t count = static_cast<uint32_t>(records.size());

    std::fwrite(&latest, sizeof(int32_t), 1, fp);
    std::fwrite(&total, sizeof(int32_t), 1, fp);
    std::fwrite(&won, sizeof(int32_t), 1, fp);
    std::fwrite(&count, sizeof(uint32_t), 1, fp);

    for (const auto& r : records) {
        uint8_t w = r.win ? 1 : 0;
        int32_t sc = r.score;
        int32_t hClass = static_cast<int32_t>(r.heroClass);
        int32_t sClass = static_cast<int32_t>(r.subClass);
        int32_t aTier = r.armorTier;
        int32_t lvl = r.level;
        int32_t dep = r.depth;
        int32_t g = r.gold;
        int32_t str = r.str;
        int32_t hp = r.maxHp;
        float dur = r.duration;
        int32_t slain = r.enemiesSlain;
        int32_t food = r.foodEaten;
        int32_t pot = r.potionsCooked;
        int32_t ankh = r.ankhsUsed;
        int32_t wImg = r.weaponImage;
        int32_t aImg = r.armorImage;
        int32_t r1Img = r.ring1Image;
        int32_t r2Img = r.ring2Image;
        int32_t qsImg = r.quickslotImage;

        std::fwrite(&w, sizeof(uint8_t), 1, fp);
        std::fwrite(&sc, sizeof(int32_t), 1, fp);
        std::fwrite(&hClass, sizeof(int32_t), 1, fp);
        std::fwrite(&sClass, sizeof(int32_t), 1, fp);
        std::fwrite(&aTier, sizeof(int32_t), 1, fp);
        std::fwrite(&lvl, sizeof(int32_t), 1, fp);
        std::fwrite(&dep, sizeof(int32_t), 1, fp);
        std::fwrite(&g, sizeof(int32_t), 1, fp);
        std::fwrite(&str, sizeof(int32_t), 1, fp);
        std::fwrite(&hp, sizeof(int32_t), 1, fp);
        std::fwrite(&dur, sizeof(float), 1, fp);
        std::fwrite(&slain, sizeof(int32_t), 1, fp);
        std::fwrite(&food, sizeof(int32_t), 1, fp);
        std::fwrite(&pot, sizeof(int32_t), 1, fp);
        std::fwrite(&ankh, sizeof(int32_t), 1, fp);
        std::fwrite(&wImg, sizeof(int32_t), 1, fp);
        std::fwrite(&aImg, sizeof(int32_t), 1, fp);
        std::fwrite(&r1Img, sizeof(int32_t), 1, fp);
        std::fwrite(&r2Img, sizeof(int32_t), 1, fp);
        std::fwrite(&qsImg, sizeof(int32_t), 1, fp);

        writeString(fp, r.info);
        writeString(fp, r.weaponName);
        writeString(fp, r.armorName);
        writeString(fp, r.ring1Name);
        writeString(fp, r.ring2Name);
        writeString(fp, r.quickslotName);
        writeString(fp, r.gameFile);
    }

    std::fclose(fp);
}

void Rankings::load() {
    if (m_loaded) return;
    m_loaded = true;

    records.clear();
    lastRecord = -1;
    totalNumber = 0;
    wonNumber = 0;

    std::string path = getFilePath();
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        fp = std::fopen("rankings.dat", "rb");
    }
    if (!fp) return;

    uint32_t magic = 0;
    uint32_t version = 0;
    if (std::fread(&magic, sizeof(uint32_t), 1, fp) != 1 || magic != RANK_MAGIC) {
        std::fclose(fp);
        return;
    }
    if (std::fread(&version, sizeof(uint32_t), 1, fp) != 1 || version != RANK_VERSION) {
        std::fclose(fp);
        return;
    }

    int32_t latest = -1;
    int32_t total = 0;
    int32_t won = 0;
    uint32_t count = 0;

    std::fread(&latest, sizeof(int32_t), 1, fp);
    std::fread(&total, sizeof(int32_t), 1, fp);
    std::fread(&won, sizeof(int32_t), 1, fp);
    std::fread(&count, sizeof(uint32_t), 1, fp);

    lastRecord = latest;
    totalNumber = total;
    wonNumber = won;

    count = std::min(count, static_cast<uint32_t>(TABLE_SIZE));
    for (uint32_t i = 0; i < count; ++i) {
        Record r;
        uint8_t w = 0;
        int32_t sc = 0, hClass = 0, sClass = 0, aTier = 0;
        int32_t lvl = 1, dep = 1, g = 0, str = 10, hp = 20;
        float dur = 0.0f;
        int32_t slain = 0, food = 0, pot = 0, ankh = 0;
        int32_t wImg = -1, aImg = -1, r1Img = -1, r2Img = -1, qsImg = -1;

        std::fread(&w, sizeof(uint8_t), 1, fp);
        std::fread(&sc, sizeof(int32_t), 1, fp);
        std::fread(&hClass, sizeof(int32_t), 1, fp);
        std::fread(&sClass, sizeof(int32_t), 1, fp);
        std::fread(&aTier, sizeof(int32_t), 1, fp);
        std::fread(&lvl, sizeof(int32_t), 1, fp);
        std::fread(&dep, sizeof(int32_t), 1, fp);
        std::fread(&g, sizeof(int32_t), 1, fp);
        std::fread(&str, sizeof(int32_t), 1, fp);
        std::fread(&hp, sizeof(int32_t), 1, fp);
        std::fread(&dur, sizeof(float), 1, fp);
        std::fread(&slain, sizeof(int32_t), 1, fp);
        std::fread(&food, sizeof(int32_t), 1, fp);
        std::fread(&pot, sizeof(int32_t), 1, fp);
        std::fread(&ankh, sizeof(int32_t), 1, fp);
        std::fread(&wImg, sizeof(int32_t), 1, fp);
        std::fread(&aImg, sizeof(int32_t), 1, fp);
        std::fread(&r1Img, sizeof(int32_t), 1, fp);
        std::fread(&r2Img, sizeof(int32_t), 1, fp);
        std::fread(&qsImg, sizeof(int32_t), 1, fp);

        r.win = (w != 0);
        r.score = sc;
        r.heroClass = static_cast<Hero::HeroClass>(hClass);
        r.subClass = static_cast<HeroSubClass>(sClass);
        r.armorTier = aTier;
        r.level = lvl;
        r.depth = dep;
        r.gold = g;
        r.str = str;
        r.maxHp = hp;
        r.duration = dur;
        r.enemiesSlain = slain;
        r.foodEaten = food;
        r.potionsCooked = pot;
        r.ankhsUsed = ankh;
        r.weaponImage = wImg;
        r.armorImage = aImg;
        r.ring1Image = r1Img;
        r.ring2Image = r2Img;
        r.quickslotImage = qsImg;

        r.info = readString(fp);
        r.weaponName = readString(fp);
        r.armorName = readString(fp);
        r.ring1Name = readString(fp);
        r.ring2Name = readString(fp);
        r.quickslotName = readString(fp);
        r.gameFile = readString(fp);

        records.push_back(r);
    }

    std::fclose(fp);
}

void Rankings::reset() {
    records.clear();
    lastRecord = -1;
    totalNumber = 0;
    wonNumber = 0;
    m_loaded = false;
    std::remove(getFilePath().c_str());
    std::remove("rankings.dat");
}

} // namespace pd

