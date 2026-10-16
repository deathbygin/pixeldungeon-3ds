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
#include "pd/Plant.h"
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

void Blob::clear(int cell) {
    if (cell >= 0 && cell < Level::LENGTH) {
        volume -= cur[cell];
        if (volume < 0) volume = 0;
        cur[cell] = 0;
    }
}

void Blob::seed(int cell, int amount) {
    if (cell >= 0 && cell < Level::LENGTH && amount > 0) {
        if (type == BlobType::FIRE) {
            if (cur[cell] == 0) {
                volume += amount;
                cur[cell] = amount;
            }
        } else {
            cur[cell] += amount;
            volume += amount;
        }
    }
}

bool Blob::act() {
    spend(TICK);
    if (volume > 0 && Dungeon::level) {
        evolve(Dungeon::level.get());
    }
    return true;
}

void Blob::evolve(const Level* level) {
    if (volume <= 0 || !level) return;

    if (type == BlobType::FIRE) {
        // =====================================================================
        // 1:1 Pixel Dungeon Fire Spread
        // =====================================================================
        bool observeNeeded = false;
        volume = 0;
        std::fill_n(off, Level::LENGTH, 0);

        for (int y = 1; y < Level::HEIGHT - 1; ++y) {
            int rowStart = y * Level::WIDTH;
            for (int x = 1; x < Level::WIDTH - 1; ++x) {
                int pos = rowStart + x;
                int fire = 0;

                // Water tiles extinguish fire immediately
                if (level->water[pos]) {
                    off[pos] = 0;
                    continue;
                }

                bool flamable = level->isFlamable(pos);

                if (cur[pos] > 0) {
                    // Burn characters standing in the fire
                    Char* ch = Actor::findChar(pos);
                    if (ch && ch->isAlive()) {
                        ch->affect<Burning>(5);
                    }

                    // Burn heaps on the tile
                    Heap* heap = level->getHeap(pos);
                    if (heap) {
                        heap->burn();
                    }

                    // Burn and activate any plants on the tile
                    Plant* plant = level->getPlant(pos);
                    if (plant) {
                        plant->activate(nullptr);
                    }

                    fire = cur[pos] - 1;
                    if (fire <= 0 && flamable) {
                        // Destroy flammable terrain (grass, doors, barricades) turning to embers
                        const_cast<Level*>(level)->destroy(pos);
                        observeNeeded = true;
                    }
                } else {
                    // Spread fire to adjacent flammable cells (4-way orthogonal)
                    if (flamable && (cur[pos - 1] > 0 ||
                                     cur[pos + 1] > 0 ||
                                     cur[pos - Level::WIDTH] > 0 ||
                                     cur[pos + Level::WIDTH] > 0)) {
                        fire = 4;

                        Char* ch = Actor::findChar(pos);
                        if (ch && ch->isAlive()) {
                            ch->affect<Burning>(5);
                        }

                        Heap* heap = level->getHeap(pos);
                        if (heap) {
                            heap->burn();
                        }

                        Plant* plant = level->getPlant(pos);
                        if (plant) {
                            plant->activate(nullptr);
                        }
                    } else {
                        fire = 0;
                    }
                }

                off[pos] = fire;
                volume += fire;
            }
        }

        std::copy_n(off, Level::LENGTH, cur);

        if (observeNeeded) {
            Dungeon::observe();
        }
        return;
    }

    if (type == BlobType::FOLIAGE) {
        // =====================================================================
        // Foliage blob (regrowth of embers and shadows buff)
        // =====================================================================
        volume = 0;
        bool mapUpdated = false;

        for (int pos = 0; pos < Level::LENGTH; ++pos) {
            if (cur[pos] > 0) {
                off[pos] = cur[pos];
                volume += off[pos];

                if (level->map[pos] == Terrain::EMBERS) {
                    const_cast<Level*>(level)->set(pos, Terrain::GRASS);
                    mapUpdated = true;
                }
            } else {
                off[pos] = 0;
            }
        }

        std::copy_n(off, Level::LENGTH, cur);

        if (Dungeon::hero && Dungeon::hero->isAlive() && cur[Dungeon::hero->pos] > 0) {
            Dungeon::hero->affect<Invisibility>();
        }

        if (mapUpdated) {
            Dungeon::observe();
        }
        return;
    }

    // =========================================================================
    // Cellular Automata Diffusion for Gas and Regrowth
    // =========================================================================
    volume = 0;
    std::fill_n(off, Level::LENGTH, 0);

    for (int y = 1; y < Level::HEIGHT - 1; ++y) {
        int rowStart = y * Level::WIDTH;
        for (int x = 1; x < Level::WIDTH - 1; ++x) {
            int pos = rowStart + x;
            bool notBlocking = level->isPassable(pos) || level->pit[pos];
            if (notBlocking) {
                int count = 1;
                int sum = cur[pos];

                if (level->isPassable(pos - 1) || level->pit[pos - 1]) {
                    sum += cur[pos - 1];
                    count++;
                }
                if (level->isPassable(pos + 1) || level->pit[pos + 1]) {
                    sum += cur[pos + 1];
                    count++;
                }
                if (level->isPassable(pos - Level::WIDTH) || level->pit[pos - Level::WIDTH]) {
                    sum += cur[pos - Level::WIDTH];
                    count++;
                }
                if (level->isPassable(pos + Level::WIDTH) || level->pit[pos + Level::WIDTH]) {
                    sum += cur[pos + Level::WIDTH];
                    count++;
                }

                int val = sum >= count ? (sum / count) - 1 : 0;
                off[pos] = val;
                volume += val;
            } else {
                off[pos] = 0;
            }
        }
    }

    // Regrowth specific logic: sprouts high grass/grass and roots characters
    if (type == BlobType::REGROWTH) {
        bool mapUpdated = false;
        for (int i = 0; i < Level::LENGTH; ++i) {
            if (off[i] > 0) {
                int c = level->map[i];
                int c1 = c;
                if (c == Terrain::EMPTY || c == Terrain::EMBERS || c == Terrain::EMPTY_DECO) {
                    c1 = (cur[i] > 9) ? Terrain::HIGH_GRASS : Terrain::GRASS;
                } else if (c == Terrain::GRASS && cur[i] > 9) {
                    c1 = Terrain::HIGH_GRASS;
                }

                if (c1 != c) {
                    const_cast<Level*>(level)->set(i, c1);
                    mapUpdated = true;
                }

                Char* ch = Actor::findChar(i);
                if (ch && ch->isAlive()) {
                    ch->affect<Roots>();
                }
            }
        }
        std::copy_n(off, Level::LENGTH, cur);
        if (mapUpdated) {
            Dungeon::observe();
        }
        return;
    }

    // Apply gas effects to characters currently standing in the cloud
    for (int i = 0; i < Level::LENGTH; ++i) {
        if (cur[i] > 0) {
            Char* ch = Actor::findChar(i);
            if (ch && ch->isAlive()) {
                if (ch->buff(BuffType::GasesImmunity) != nullptr) {
                    continue;
                }
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
                } else if (type == BlobType::CONFUSION_GAS) {
                    ch->affect<Vertigo>();
                } else if (type == BlobType::CORROSIVE_GAS) {
                    int levelDamage = 5 + Dungeon::depth * 5;
                    int dmg = (ch->HT + levelDamage) / 40;
                    if (Random::Int(40) < (ch->HT + levelDamage) % 40) {
                        dmg++;
                    }
                    ch->affect<Ooze>();
                    if (dmg > 0) {
                        ch->damage(dmg, nullptr);
                        FX::floatingText(ch->pos, std::to_string(dmg), 0x22aa44FF);
                    }
                }
            }
        }
    }

    // Mutual Gas Annihilation: ToxicGas and ParalyticGas neutralize each other tile-by-tile
    if (type == BlobType::TOXIC_GAS) {
        Blob* paralyticBlob = FX::getBlob(BlobType::PARALYTIC_GAS);
        if (paralyticBlob && paralyticBlob->volume > 0) {
            int* par = paralyticBlob->cur;
            for (int i = 0; i < Level::LENGTH; ++i) {
                int t = cur[i];
                int p = par[i];
                if (t > 0 && p > 0) {
                    if (p >= t) {
                        volume -= t;
                        cur[i] = 0;
                    } else {
                        paralyticBlob->volume -= p;
                        par[i] = 0;
                    }
                }
            }
            if (volume < 0) volume = 0;
            if (paralyticBlob->volume < 0) paralyticBlob->volume = 0;
        }
    }

    // Double buffer swap: cur becomes off
    std::copy_n(off, Level::LENGTH, cur);
}

uint32_t Blob::getParticleColor() const {
    switch (type) {
    case BlobType::TOXIC_GAS:
        return 0x50ff60FF; // Classic toxic green
    case BlobType::PARALYTIC_GAS:
        return 0xffff66FF; // Paralytic pale yellow
    case BlobType::FIRE:
        return 0xee7722FF; // Flame orange
    case BlobType::CONFUSION_GAS:
        return 0xbf55ecFF; // Shimmering purple
    case BlobType::CORROSIVE_GAS:
        return 0x22aa44FF; // Caustic dark green
    case BlobType::REGROWTH:
        return 0x44aa33FF; // Leaf green
    case BlobType::FOLIAGE:
        return 0x88cc44FF; // Foliage bright green
    default:
        return 0xFFFFFFFF;
    }
}

const char* Blob::tileDesc() const {
    switch (type) {
    case BlobType::FIRE:          return "A fire is raging here.";
    case BlobType::TOXIC_GAS:     return "A greenish cloud of toxic gas is swirling here.";
    case BlobType::PARALYTIC_GAS: return "A cloud of paralytic gas is swirling here.";
    case BlobType::CONFUSION_GAS: return "A cloud of confusion gas is swirling here.";
    case BlobType::CORROSIVE_GAS: return "A cloud of corrosive acidic vapor is hissing here.";
    case BlobType::REGROWTH:      return "Magical roots and vegetation are overgrown here.";
    case BlobType::FOLIAGE:       return "Shafts of light pierce the gloom of the underground garden.";
    default:                      return nullptr;
    }
}

// =============================================================================
// Freezing Helper (1:1 with Freezing.java)
// =============================================================================
bool Freezing::affect(int cell, Blob* fire) {
    if (cell < 0 || cell >= Level::LENGTH) return false;

    Char* ch = Actor::findChar(cell);
    if (ch && ch->isAlive()) {
        ch->affect<Frost>();
    }

    if (!fire) {
        fire = FX::getBlob(Blob::BlobType::FIRE);
    }
    if (fire && fire->cur[cell] > 0) {
        fire->clear(cell);
    }

    if (Dungeon::level) {
        Heap* heap = Dungeon::level->getHeap(cell);
        if (heap) {
            heap->freeze();
        }
    }

    if (Dungeon::visible[cell]) {
        FX::burst(cell, 0xd0f0ffFF, 6, 20.0f, 50.0f, 20.0f);
        return true;
    }
    return false;
}

} // namespace pd
