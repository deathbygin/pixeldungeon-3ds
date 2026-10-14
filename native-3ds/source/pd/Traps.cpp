#include "pd/Traps.h"
#include "pd/Terrain.h"
#include "pd/Level.h"
#include "pd/Dungeon.h"
#include "pd/Char.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/Buff.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Random.h"
#include "pd/Assets.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"
#include <algorithm>
#include <vector>

namespace pd {

void Traps::trigger(int tile, int cell, Char* ch) {
    if (cell < 0 || cell >= Level::LENGTH) return;

    bool isSecret = Terrain::isSecret(tile);
    if (isSecret && ch == Dungeon::hero.get()) {
        GLog::i("A hidden plate clicks under your foot!");
    }

    bool isTrap = false;

    switch (tile) {
        case Terrain::SECRET_FIRE_TRAP:
        case Terrain::FIRE_TRAP: {
            isTrap = true;
            FX::seedBlob(cell, Blob::BlobType::FIRE, 20);
            FX::burst(cell, 0xff7708FFu, 6);
            break;
        }

        case Terrain::SECRET_TOXIC_TRAP:
        case Terrain::TOXIC_TRAP: {
            isTrap = true;
            FX::seedBlob(cell, Blob::BlobType::TOXIC_GAS, 300 + 20 * Dungeon::depth);
            break;
        }

        case Terrain::SECRET_PARALYTIC_TRAP:
        case Terrain::PARALYTIC_TRAP: {
            isTrap = true;
            FX::seedBlob(cell, Blob::BlobType::PARALYTIC_GAS, 80 + 5 * Dungeon::depth);
            break;
        }

        case Terrain::SECRET_POISON_TRAP:
        case Terrain::POISON_TRAP: {
            isTrap = true;
            if (ch) {
                auto* p = ch->affect<Poison>();
                if (p) {
                    p->duration = 4 + Dungeon::depth / 2;
                }
            }
            FX::burst(cell, 0xbb66eeFFu, 4);
            break;
        }

        case Terrain::SECRET_ALARM_TRAP:
        case Terrain::ALARM_TRAP: {
            isTrap = true;
            for (auto& m : Dungeon::mobs) {
                if (m && m.get() != ch) {
                    m->beckon(cell);
                }
            }
            if (Dungeon::visible[cell]) {
                GLog::w("The trap emits a piercing sound that echoes throughout the dungeon!");
                FX::burst(cell, 0xdd3333FFu, 6);
            }
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_ALERT).get());
            break;
        }

        case Terrain::SECRET_LIGHTNING_TRAP:
        case Terrain::LIGHTNING_TRAP: {
            isTrap = true;
            if (ch) {
                int dmg = std::max(1, Random::IntRange(ch->HP / 3, 2 * ch->HP / 3));
                ch->damage(dmg, nullptr);
                if (ch == Dungeon::hero.get()) {
                    if (noosa::Camera::main) {
                        noosa::Camera::main->shake(0.3f, 2.0f);
                    }
                    if (!ch->isAlive()) {
                        GLog::n("You were killed by a discharge of a lightning trap...");
                    }
                }
            }
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_LIGHTNING).get());
            FX::burst(cell, 0x66cceeFFu, 6);
            break;
        }

        case Terrain::SECRET_GRIPPING_TRAP:
        case Terrain::GRIPPING_TRAP: {
            isTrap = true;
            if (ch) {
                int dmg = std::max(0, (Dungeon::depth + 3) - Random::IntRange(0, ch->dr() / 2));
                auto* b = ch->affect<Bleeding>();
                if (b) b->level = dmg;
                ch->affect<Cripple>();
            }
            FX::slash(cell);
            break;
        }

        case Terrain::SECRET_SUMMONING_TRAP:
        case Terrain::SUMMONING_TRAP: {
            isTrap = true;
            if (Dungeon::depth % 5 != 0) { // Not boss floors
                int nMobs = 1;
                if (Random::Int(2) == 0) {
                    nMobs++;
                    if (Random::Int(2) == 0) nMobs++;
                }

                // Check 8 neighbors for unoccupied passable spots
                std::vector<int> candidates;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int cx = (cell % Level::WIDTH) + dx;
                        int cy = (cell / Level::WIDTH) + dy;
                        if (cx >= 0 && cx < Level::WIDTH && cy >= 0 && cy < Level::HEIGHT) {
                            int p = cy * Level::WIDTH + cx;
                            if (Dungeon::level && Dungeon::level->isPassable(p) && !Dungeon::level->pit[p] && Actor::findChar(p) == nullptr) {
                                candidates.push_back(p);
                            }
                        }
                    }
                }

                while (nMobs > 0 && !candidates.empty()) {
                    int idx = Random::Int(static_cast<int>(candidates.size()));
                    int spawnPos = candidates[idx];
                    candidates.erase(candidates.begin() + idx);

                    std::unique_ptr<Mob> mob;
                    if (Dungeon::depth <= 2) {
                        mob = (Random::Int(2) == 0) ? std::unique_ptr<Mob>(std::make_unique<Rat>()) : std::unique_ptr<Mob>(std::make_unique<Gnoll>());
                    } else if (Dungeon::depth <= 4) {
                        int r = Random::Int(4);
                        if (r == 0) mob = std::make_unique<Rat>();
                        else if (r == 1) mob = std::make_unique<Gnoll>();
                        else if (r == 2) mob = std::make_unique<Crab>();
                        else mob = std::make_unique<Skeleton>();
                    } else {
                        int r = Random::Int(3);
                        if (r == 0) mob = std::make_unique<Gnoll>();
                        else if (r == 1) mob = std::make_unique<Crab>();
                        else mob = std::make_unique<Skeleton>();
                    }

                    if (mob) {
                        mob->move(spawnPos);
                        mob->state = Mob::State::WANDERING;
                        Actor::occupyCell(mob.get());
                        Actor::add(mob.get(), 1.0f);
                        Dungeon::mobs.push_back(std::move(mob));
                    }
                    nMobs--;
                }
            }
            FX::burst(cell, 0x770088FFu, 8);
            break;
        }

        default:
            break;
    }

    if (isTrap) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_TRAP).get());
        if (ch == Dungeon::hero.get()) {
            Dungeon::hero->interrupt();
        }
        if (Dungeon::level) {
            Dungeon::level->set(cell, Terrain::INACTIVE_TRAP);
        }
        Dungeon::observe();
    }
}

} // namespace pd

