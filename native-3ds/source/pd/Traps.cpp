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
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"
#include <algorithm>
#include <vector>

namespace pd {

void Traps::trigger(int tile, int cell, Char* ch) {
    if (cell < 0 || cell >= Level::LENGTH) return;
    if (ch && ch->flying) return; // Flying / levitating characters pass harmlessly over traps

    bool isSecret = Terrain::isSecret(tile);
    if (isSecret && ch == Dungeon::hero.get()) {
        GLog::i("A hidden pressure plate clicks!");
    }

    bool isTrap = false;

    switch (tile) {
        case Terrain::SECRET_FIRE_TRAP:
        case Terrain::FIRE_TRAP: {
            isTrap = true;
            GLog::w("A fire trap was activated!");
            FX::seedBlob(cell, Blob::BlobType::FIRE, 2);
            FX::burst(cell, 0xff7708FFu, 5);
            Sample::playAt(Assets::SND_BURNING, cell);

            if (Dungeon::level) {
                Heap* h = Dungeon::level->getHeap(cell);
                if (h) {
                    h->burn();
                }
            }

            if (ch) {
                ch->affect<Burning>(5);
            }
            break;
        }

        case Terrain::SECRET_TOXIC_TRAP:
        case Terrain::TOXIC_TRAP: {
            isTrap = true;
            GLog::w("A toxic gas trap was activated!");
            FX::seedBlob(cell, Blob::BlobType::TOXIC_GAS, 300 + 20 * Dungeon::depth);
            FX::splash(cell, 0x40cc55FF, 10);
            FX::burst(cell, 0x40cc55FF, 8);
            break;
        }

        case Terrain::SECRET_PARALYTIC_TRAP:
        case Terrain::PARALYTIC_TRAP: {
            isTrap = true;
            GLog::w("A paralytic gas trap was activated!");
            FX::seedBlob(cell, Blob::BlobType::PARALYTIC_GAS, 80 + 5 * Dungeon::depth);
            FX::splash(cell, 0xcccc55FF, 10);
            FX::burst(cell, 0xcccc55FF, 8);
            break;
        }

        case Terrain::SECRET_POISON_TRAP:
        case Terrain::POISON_TRAP: {
            isTrap = true;
            GLog::w("A poison dart trap was activated!");
            if (ch) {
                auto* p = ch->affect<Poison>();
                if (p) {
                    p->durationTicks = 4 + Dungeon::depth / 2;
                }
            }
            FX::splash(cell, 0xbb66eeFFu, 8);
            FX::burst(cell, 0xbb66eeFFu, 6);
            break;
        }

        case Terrain::SECRET_ALARM_TRAP:
        case Terrain::ALARM_TRAP: {
            isTrap = true;
            GLog::w("An alarm trap was activated!");
            for (auto& m : Dungeon::mobs) {
                if (m && m.get() != ch) {
                    m->beckon(cell);
                    m->state = Mob::State::HUNTING;
                    m->target = (ch ? ch->pos : cell);
                    m->alerted = true;
                    m->notice();
                }
            }
            if (Dungeon::visible[cell]) {
                GLog::w("The trap emits a piercing sound that echoes throughout the dungeon!");
                FX::burst(cell, 0xdd3333FFu, 8);
            }
            Sample::playAt(Assets::SND_ALERT, cell);
            break;
        }

        case Terrain::SECRET_LIGHTNING_TRAP:
        case Terrain::LIGHTNING_TRAP: {
            isTrap = true;
            GLog::w("A lightning trap was activated!");
            Sample::playAt(Assets::SND_LIGHTNING, cell);
            FX::burst(cell, 0x66cceeFFu, 8);
            FX::burst(cell, 0xffffffffu, 6);

            if (ch) {
                int dmg = std::max(1, Random::IntRange(ch->HP / 3, 2 * ch->HP / 3));
                if (Dungeon::level && Dungeon::level->water[ch->pos] && !ch->flying) {
                    dmg *= 2;
                }
                ch->damage(dmg, nullptr);
                FX::floatingText(ch->pos, std::to_string(dmg), 0x66cceeFF);

                if (ch == Dungeon::hero.get()) {
                    if (noosa::Camera::main) {
                        noosa::Camera::main->shake(0.3f, 2.0f);
                    }
                    if (!ch->isAlive()) {
                        GLog::n("You were killed by a discharge of a lightning trap...");
                    }
                }
            }

            // Conductive shock propagation along connected water cells
            if (Dungeon::level) {
                bool inWater = Dungeon::level->water[cell];
                const int neighbours8[8] = {
                    -Level::WIDTH - 1, -Level::WIDTH, -Level::WIDTH + 1,
                    -1,                               +1,
                    +Level::WIDTH - 1, +Level::WIDTH, +Level::WIDTH + 1
                };

                for (int i = 0; i < 8; ++i) {
                    int n = cell + neighbours8[i];
                    if (n >= 0 && n < Level::LENGTH) {
                        if (inWater || Dungeon::level->water[n]) {
                            Char* nch = Actor::findChar(n);
                            if (nch && nch != ch && !nch->flying && nch->isAlive()) {
                                int ndmg = std::max(1, Random::IntRange(nch->HP / 4, nch->HP / 2));
                                if (Dungeon::level->water[n]) ndmg *= 2;
                                nch->damage(ndmg, nullptr);
                                FX::floatingText(n, std::to_string(ndmg), 0x66cceeFF);
                                FX::burst(n, 0x66cceeFFu, 6);
                                if (nch == Dungeon::hero.get() && !nch->isAlive()) {
                                    GLog::n("You were killed by a discharge of a lightning trap...");
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case Terrain::SECRET_GRIPPING_TRAP:
        case Terrain::GRIPPING_TRAP: {
            isTrap = true;
            GLog::w("A gripping trap was activated!");
            if (ch) {
                int dmg = std::max(0, (Dungeon::depth + 3) - Random::IntRange(0, ch->dr() / 2));
                if (dmg > 0) {
                    ch->damage(dmg, nullptr);
                    FX::floatingText(ch->pos, std::to_string(dmg), 0xe43b44FF);
                    auto* b = ch->affect<Bleeding>();
                    if (b) b->level = dmg;
                }
                ch->affect<Roots>();
                ch->affect<Cripple>();
                FX::blood(ch->pos, 10);
                FX::slash(ch->pos);
            } else {
                FX::slash(cell);
                FX::burst(cell, 0x888888FF, 6);
            }
            break;
        }

        case Terrain::SECRET_SUMMONING_TRAP:
        case Terrain::SUMMONING_TRAP: {
            isTrap = true;
            GLog::w("A summoning trap was activated!");
            if (Dungeon::depth % 5 != 0) { // Not on boss floors
                int nMobs = 2;
                if (Random::Int(2) == 0) {
                    nMobs++;
                    if (Random::Int(2) == 0) nMobs++;
                }

                // Check 8 neighbors for unoccupied passable spots
                std::vector<int> candidates;
                const int neighbours8[8] = {
                    -Level::WIDTH - 1, -Level::WIDTH, -Level::WIDTH + 1,
                    -1,                               +1,
                    +Level::WIDTH - 1, +Level::WIDTH, +Level::WIDTH + 1
                };
                for (int i = 0; i < 8; ++i) {
                    int p = cell + neighbours8[i];
                    if (p >= 0 && p < Level::LENGTH && Dungeon::level) {
                        if (Dungeon::level->isPassable(p) && !Dungeon::level->pit[p] && Actor::findChar(p) == nullptr) {
                            candidates.push_back(p);
                        }
                    }
                }

                while (nMobs > 0 && !candidates.empty()) {
                    int idx = Random::Int(static_cast<int>(candidates.size()));
                    int spawnPos = candidates[idx];
                    candidates.erase(candidates.begin() + idx);

                    std::unique_ptr<Mob> mob = Dungeon::createMobForDepth(Dungeon::depth);
                    if (mob) {
                        mob->place(spawnPos);
                        mob->state = Mob::State::WANDERING;
                        Actor::occupyCell(mob.get());
                        Actor::add(mob.get(), 1.0f);
                        FX::burst(spawnPos, 0x770088FFu, 8);
                        Dungeon::mobs.push_back(std::move(mob));
                    }
                    nMobs--;
                }
            }
            FX::burst(cell, 0x770088FFu, 8);
            Sample::playAt(Assets::SND_TELEPORT, cell);
            break;
        }

        case Terrain::SECRET_TELEPORTATION_TRAP:
        case Terrain::TELEPORTATION_TRAP: {
            isTrap = true;
            GLog::w("A teleportation trap was activated!");
            if (ch) {
                int newPos = -1;
                int count = 10;
                do {
                    newPos = Dungeon::level ? Dungeon::level->randomDestination() : -1;
                    if (count-- <= 0) break;
                } while (newPos == -1);

                if (newPos != -1) {
                    int oldPos = ch->pos;
                    FX::burst(oldPos, 0xaa44ffFF, 8);
                    ch->move(newPos);
                    FX::burst(newPos, 0xaa44ffFF, 8);

                    if (ch == Dungeon::hero.get()) {
                        GLog::i("In a blink of an eye you were teleported to another location of the level.");
                        Dungeon::observe();
                    } else {
                        Mob* m = static_cast<Mob*>(ch);
                        if (m) {
                            m->state = Mob::State::WANDERING;
                            m->target = -1;
                            m->enemy = nullptr;
                            m->enemySeen = false;
                        }
                    }
                }
            } else {
                FX::burst(cell, 0xaa44ffFF, 6);
            }
            Sample::playAt(Assets::SND_TELEPORT, cell);
            break;
        }

        default:
            break;
    }

    if (isTrap) {
        Sample::playAt(Assets::SND_TRAP, cell);
        if (ch == Dungeon::hero.get()) {
            Dungeon::hero->interrupt();
        }
        if (Dungeon::level) {
            Dungeon::level->set(cell, Terrain::INACTIVE_TRAP);
            Heap* h = Dungeon::level->getHeap(cell);
            if (h && h->type == Heap::Type::HEAP) {
                for (auto it = h->items.begin(); it != h->items.end();) {
                    if ((*it)->name == "honeypot" || (*it)->name == "Honeypot") {
                        auto hp = std::move(*it);
                        it = h->items.erase(it);
                        static_cast<Honeypot*>(hp.get())->shatter(cell);
                    } else {
                        ++it;
                    }
                }
                if (h->isEmpty()) {
                    Dungeon::level->removeHeap(h);
                }
            }
        }
        Dungeon::observe();
    }
}

} // namespace pd

