#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "pd/Hero.h"
#include "pd/HeroSubClass.h"

namespace pd {

class Rankings {
public:
    static constexpr size_t TABLE_SIZE = 10;
    static constexpr uint32_t RANK_MAGIC = 0x524E4B53; // 'RNKS'
    static constexpr uint32_t RANK_VERSION = 1;

    struct Record {
        std::string info;
        bool win = false;
        int score = 0;

        Hero::HeroClass heroClass = Hero::HeroClass::Warrior;
        HeroSubClass subClass = HeroSubClass::None;
        int armorTier = 0;
        int level = 1;
        int depth = 1;
        int gold = 0;

        // Statistics snapshot
        int str = 10;
        int maxHp = 20;
        float duration = 0.0f;
        int enemiesSlain = 0;
        int foodEaten = 0;
        int potionsCooked = 0;
        int ankhsUsed = 0;

        // Equipment snapshot
        std::string weaponName;
        int weaponImage = -1;
        std::string armorName;
        int armorImage = -1;
        std::string ring1Name;
        int ring1Image = -1;
        std::string ring2Name;
        int ring2Image = -1;
        std::string quickslotName;
        int quickslotImage = -1;

        std::string gameFile;
    };

    static Rankings& instance();

    std::vector<Record> records;
    int lastRecord = -1;
    int totalNumber = 0;
    int wonNumber = 0;

    void submit(bool win, const std::string& customReason = "");
    void load();
    void save();
    void reset();

    static int score(bool win);

private:
    Rankings() = default;
    bool m_loaded = false;
    std::string getFilePath() const;
};

} // namespace pd

