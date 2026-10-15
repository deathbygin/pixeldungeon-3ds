#include "pd/Blob.h"
#include "pd/Actor.h"
#include "pd/Char.h"
#include "pd/Hero.h"
#include "pd/Buff.h"
#include "pd/Dungeon.h"
#include "pd/Random.h"
#include "pd/Badges.h"
#include "pd/FX.h"
#include "pd/Heap.h"
#include <algorithm>
#include <cstring>

namespace pd {

Blob::Blob(BlobType t) : type(t) {
    clear();
}

void Blob::clear() {
    volume = 0;
    std::fill_n(cur, Level::LENGTH, 0);
    std::fill_n(off, Level::LENGTH, 0);
}

void Blob::seed(int cell, int amount) {
    if (cell >= 0 && cell < Level::LENGTH && amount > 0) {
        cur[cell] += amount;
        volume += amount;
    }
}

void Blob::evolve(const Level* level) {
    if (volume <= 0 || !level) return;

    volume = 0;
    std::fill_n(off, Level::LENGTH, 0);

    for (int y = 1; y < Level::HEIGHT - 1; ++y) {
        int rowStart = y * Level::WIDTH;
        for (int x = 1; x < Level::WIDTH - 1; ++x) {
            int pos = rowStart + x;
            if (level->isPassable(pos)) {
                int count = 1;
                int sum = cur[pos];

                if (level->isPassable(pos - 1)) {
                    sum += cur[pos - 1];
                    count++;
                }
                if (level->isPassable(pos + 1)) {
                    sum += cur[pos + 1];
                    count++;
                }
                if (level->isPassable(pos - Level::WIDTH)) {
                    sum += cur[pos - Level::WIDTH];
                    count++;
                }
                if (level->isPassable(pos + Level::WIDTH)) {
                    sum += cur[pos + Level::WIDTH];
                    count++;
                }

                int val = sum / count;
                if (val > 0) {
                    val--; // gradual dissipation
                }

                off[pos] = val;
                volume += val;
            }
        }
    }

    std::copy_n(off, Level::LENGTH, cur);

    // Apply effects to characters occupying blob tiles
    for (int i = 0; i < Level::LENGTH; ++i) {
        if (cur[i] > 0) {
            Char* ch = Actor::findChar(i);
            if (ch && ch->isAlive()) {
                if (type == BlobType::TOXIC_GAS) {
                    int levelDamage = 5 + Dungeon::depth * 5;
                    int dmg = (ch->HT + levelDamage) / 40;
                    if (Random::Int(40) < (ch->HT + levelDamage) % 40) {
                        dmg++;
                    }
                    if (dmg > 0) {
                        ch->damage(dmg, nullptr);
                        FX::floatingText(ch->pos, std::to_string(dmg), 0x38b764FF);
                        if (!ch->isAlive() && ch == Dungeon::hero.get()) {
                            Badges::validateDeathFromGas();
                        }
                    }
                } else if (type == BlobType::PARALYTIC_GAS) {
                    ch->affect<Paralysis>(1);
                } else if (type == BlobType::FIRE) {
                    ch->affect<Burning>(5);
                }
            }

            if (type == BlobType::FIRE && level) {
                if (level->isFlamable(i)) {
                    const_cast<Level*>(level)->destroy(i);
                }
                Heap* h = level->getHeap(i);
                if (h) {
                    h->burn();
                }
            }
        }
    }
}

uint32_t Blob::getParticleColor() const {
    switch (type) {
    case BlobType::TOXIC_GAS:
        return 0x38b764FF; // Toxic green
    case BlobType::PARALYTIC_GAS:
        return 0xe4d83bFF; // Paralytic yellow
    case BlobType::FIRE:
        return 0xee7722FF; // Flame orange
    default:
        return 0xFFFFFFFF;
    }
}

} // namespace pd

