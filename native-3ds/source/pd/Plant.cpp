#include "pd/Plant.h"
#include "pd/Level.h"
#include "pd/Char.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/Buff.h"
#include "pd/Dungeon.h"
#include "pd/FX.h"
#include "pd/Assets.h"
#include "pd/Random.h"
#include "pd/GLog.h"
#include "pd/Blob.h"
#include "pd/Heap.h"
#include "pd/Item.h"
#include "noosa/Audio.h"
#include <unordered_map>
#include <cmath>

namespace pd {

static std::unordered_map<int, SeedEffect> s_registry;
static bool s_registryInitialized = false;

void SeedRegistry::init() {
    if (s_registryInitialized) return;
    s_registryInitialized = true;

    // 1. Firebloom
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::FIREBLOOM;
        e.plantName = "Firebloom";
        e.seedName = "Seed of Firebloom";
        e.image = 0;
        e.particleColor = 0xff4500FF;
        e.aoeRadius = 0;
        e.duration = 5.0f;
        e.tags = SeedTag::Hazard | SeedTag::Elemental;
        e.description = "When something touches a Firebloom, it bursts into flames.";
        e.alchemyPotion = "Liquid Flame";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_BURNING).get());
            if (Dungeon::visible[cell]) {
                FX::burst(cell, 0xff4500FF, 10);
            }
            FX::seedBlob(cell, Blob::BlobType::FIRE, 4);
            if (target) {
                target->affect<Burning>(5);
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 2. Icecap
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::ICECAP;
        e.plantName = "Icecap";
        e.seedName = "Seed of Icecap";
        e.image = 1;
        e.particleColor = 0x00ffffff;
        e.aoeRadius = 1;
        e.duration = 6.0f;
        e.tags = SeedTag::Hazard | SeedTag::Elemental | SeedTag::Disabling;
        e.description = "Upon touching an Icecap excretes a pollen, which freezes everything in its vicinity.";
        e.alchemyPotion = "Frost";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)target;
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_SHATTER).get());

            Blob* fire = FX::getBlob(Blob::BlobType::FIRE);
            int cx = cell % Level::WIDTH;
            int cy = cell / Level::WIDTH;

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < Level::WIDTH && ny >= 0 && ny < Level::HEIGHT) {
                        int c = ny * Level::WIDTH + nx;
                        Freezing::affect(c, fire);
                    }
                }
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 3. Sorrowmoss
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::SORROWMOSS;
        e.plantName = "Sorrowmoss";
        e.seedName = "Seed of Sorrowmoss";
        e.image = 2;
        e.particleColor = 0x38b764FF;
        e.aoeRadius = 0;
        e.duration = 8.0f;
        e.tags = SeedTag::Hazard | SeedTag::Toxic;
        e.description = "A Sorrowmoss is a flower (not a moss) with razor-sharp petals, coated with a deadly venom.";
        e.alchemyPotion = "Toxic Gas";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_STEP).get());
            if (Dungeon::visible[cell]) {
                FX::splash(cell, 0x38b764FF, 8);
            }
            if (target) {
                int duration = 4 + Dungeon::depth / 2;
                target->affect<Poison>(duration);
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 4. Dreamweed
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::DREAMWEED;
        e.plantName = "Dreamweed";
        e.seedName = "Seed of Dreamweed";
        e.image = 3;
        e.particleColor = 0xbf55ecFF;
        e.aoeRadius = 1;
        e.duration = 10.0f;
        e.tags = SeedTag::Hazard | SeedTag::Disabling;
        e.description = "Upon touching a Dreamweed it secretes a glittering cloud of confusing gas.";
        e.alchemyPotion = "Invisibility";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_PUFF).get());
            if (Dungeon::visible[cell]) {
                FX::burst(cell, 0xbf55ecFF, 12);
            }
            FX::seedBlob(cell, Blob::BlobType::CONFUSION_GAS, 400);
            if (target) {
                target->affect<Vertigo>();
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 5. Sungrass
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::SUNGRASS;
        e.plantName = "Sungrass";
        e.seedName = "Seed of Sungrass";
        e.image = 4;
        e.particleColor = 0xffee44FF;
        e.aoeRadius = 0;
        e.duration = 0.0f;
        e.tags = SeedTag::Beneficial;
        e.description = "Sungrass is renowned for its sap's healing properties.";
        e.alchemyPotion = "Healing";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_RAY).get());
            if (Dungeon::visible[cell]) {
                FX::burst(cell, 0xffee44FF, 10);
            }
            if (target) {
                target->affect<HerbalHealing>();
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 6. Earthroot
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::EARTHROOT;
        e.plantName = "Earthroot";
        e.seedName = "Seed of Earthroot";
        e.image = 5;
        e.particleColor = 0x8b5a2bFF;
        e.aoeRadius = 0;
        e.duration = 0.0f;
        e.tags = SeedTag::Beneficial | SeedTag::Disabling;
        e.description = "When a creature touches an Earthroot, its roots create a kind of natural armor around it.";
        e.alchemyPotion = "Paralytic Gas";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_ROCKS).get());
            if (Dungeon::visible[cell]) {
                FX::burst(cell, 0x8b5a2bFF, 12);
            }
            if (target) {
                EarthrootArmor* armor = target->affect<EarthrootArmor>();
                if (armor) {
                    armor->setLevel(target->HT);
                }
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 7. Fadeleaf
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::FADELEAF;
        e.plantName = "Fadeleaf";
        e.seedName = "Seed of Fadeleaf";
        e.image = 6;
        e.particleColor = 0x00ffffff;
        e.aoeRadius = 0;
        e.duration = 0.0f;
        e.tags = SeedTag::Spatial;
        e.description = "Touching a Fadeleaf will teleport any creature to a random place on the current level.";
        e.alchemyPotion = "Mind Vision";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_TELEPORT).get());
            if (Dungeon::visible[cell]) {
                FX::burst(cell, 0x00ffffff, 12);
            }
            if (target && Dungeon::level) {
                int newPos = -1;
                int count = 10;
                do {
                    newPos = Dungeon::level->randomDestination();
                    if (count-- <= 0) break;
                } while (newPos == -1);

                if (newPos != -1) {
                    int oldPos = target->pos;
                    FX::burst(oldPos, 0xaa44ffFF, 8);
                    target->place(newPos);
                    FX::burst(newPos, 0xaa44ffFF, 8);

                    if (target == Dungeon::hero.get()) {
                        GLog::i("In a blink of an eye you were teleported to another location of the level.");
                        Dungeon::observe();
                    } else if (target) {
                        Mob* m = target->asMob();
                        if (m) {
                            m->state = Mob::State::WANDERING;
                            m->target = -1;
                            m->enemy = nullptr;
                            m->enemySeen = false;
                        }
                    }
                }
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }

    // 8. Rotberry
    {
        SeedEffect e;
        e.type = PlantSeed::PlantType::ROTBERRY;
        e.plantName = "Rotberry";
        e.seedName = "Seed of Rotberry";
        e.image = 7;
        e.particleColor = 0x55aa33FF;
        e.aoeRadius = 1;
        e.duration = 3.0f;
        e.tags = SeedTag::Hazard | SeedTag::Toxic | SeedTag::Disabling;
        e.description = "Berries of this shrub taste like sweet, sweet death.";
        e.alchemyPotion = "Strength";
        e.activate = [](int cell, Char* target, bool directHit) {
            (void)directHit;
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_PUFF).get());
            if (Dungeon::visible[cell]) {
                FX::burst(cell, 0x55aa33FF, 10);
            }
            FX::seedBlob(cell, Blob::BlobType::TOXIC_GAS, 100);
            if (target) {
                target->affect<Roots>();
            }
            if (Dungeon::level) {
                Dungeon::level->drop(std::make_unique<PlantSeed>(PlantSeed::PlantType::ROTBERRY), cell);
            }
        };
        s_registry[static_cast<int>(e.type)] = std::move(e);
    }
}

const SeedEffect& SeedRegistry::get(PlantSeed::PlantType type) {
    init();
    auto it = s_registry.find(static_cast<int>(type));
    if (it != s_registry.end()) {
        return it->second;
    }
    return s_registry[0];
}

bool SeedRegistry::isPlantable(const Level* level, int cell) {
    if (!level || cell < 0 || cell >= Level::LENGTH) return false;
    int t = level->map[cell];
    if (t == Terrain::WALL || t == Terrain::CHASM || level->pit[cell] ||
        t == Terrain::DOOR || t == Terrain::LOCKED_DOOR || t == Terrain::BARRICADE ||
        t == Terrain::STATUE || t == Terrain::STATUE_SP || t == Terrain::BOOKSHELF ||
        t == Terrain::EMPTY_WELL) {
        return false;
    }
    return level->isPassable(cell);
}

// =============================================================================
// Plant Implementation
// =============================================================================
Plant::Plant(PlantSeed::PlantType t, int p) : type(t), pos(p) {
    const auto& eff = SeedRegistry::get(t);
    image = eff.image;
    plantName = eff.plantName;
}

void Plant::activate(Char* ch) {
    if (ch && ch == Dungeon::hero.get() && Dungeon::hero->subClass == HeroSubClass::Warden) {
        Barkskin* b = ch->affect<Barkskin>();
        if (b) {
            b->setLevel(ch->HT / 3);
        }
    }

    const SeedEffect& effect = SeedRegistry::get(type);
    if (effect.activate) {
        effect.activate(pos, ch, false);
    }

    wither();
}

void Plant::wither() {
    int p = pos;
    if (Dungeon::visible[p]) {
        FX::burst(p, 0x44aa33FF, 6);
    }

    if (Dungeon::hero && Dungeon::hero->subClass == HeroSubClass::Warden) {
        if (Random::Int(5) == 0 && Dungeon::level) {
            auto randomSeed = static_cast<PlantSeed::PlantType>(Random::Int(8));
            Dungeon::level->drop(std::make_unique<PlantSeed>(randomSeed), p);
        }
        if (Random::Int(5) == 0 && Dungeon::level) {
            Dungeon::level->drop(std::make_unique<Dewdrop>(), p);
        }
    }

    if (Dungeon::level) {
        Dungeon::level->uproot(p);
    }
}

std::string Plant::desc() const {
    return SeedRegistry::get(type).description;
}

// =============================================================================
// Unified Seed Impact Dispatcher
// =============================================================================
void OnSeedImpact(PlantSeed::PlantType type, int cell) {
    if (!Dungeon::level || cell < 0 || cell >= Level::LENGTH) return;

    // 1. Chasm / Pit
    if (Dungeon::level->map[cell] == Terrain::CHASM || Dungeon::level->pit[cell]) {
        Dungeon::dropToChasm(std::make_unique<PlantSeed>(type));
        return;
    }

    // 2. Alchemy Pot: collect seeds & brew when 3 seeds present
    if (Dungeon::level->map[cell] == Terrain::ALCHEMY) {
        Dungeon::level->drop(std::make_unique<PlantSeed>(type), cell);
        Heap* heap = Dungeon::level->getHeap(cell);
        if (heap) {
            int seedCount = 0;
            std::vector<PlantSeed::PlantType> seedTypes;
            for (const auto& it : heap->items) {
                auto* ps = it ? it->asPlantSeed() : nullptr;
                if (ps) {
                    for (int i = 0; i < ps->quantity; ++i) {
                        seedTypes.push_back(ps->plantType);
                        seedCount++;
                    }
                }
            }

            if (seedCount >= 3) {
                int toRemove = 3;
                auto it = heap->items.begin();
                while (it != heap->items.end() && toRemove > 0) {
                    auto* ps = (*it) ? (*it)->asPlantSeed() : nullptr;
                    if (ps) {
                        if (ps->quantity <= toRemove) {
                            toRemove -= ps->quantity;
                            it = heap->items.erase(it);
                        } else {
                            ps->quantity -= toRemove;
                            toRemove = 0;
                            ++it;
                        }
                    } else {
                        ++it;
                    }
                }

                Potion::PotionType brewedType;
                if (Random::Int(seedCount) == 0) {
                    brewedType = static_cast<Potion::PotionType>(Random::Int(11));
                } else {
                    PlantSeed::PlantType chosenSeed = seedTypes[Random::Int(static_cast<int>(seedTypes.size()))];
                    switch (chosenSeed) {
                        case PlantSeed::PlantType::FIREBLOOM:  brewedType = Potion::PotionType::LIQUID_FLAME;   break;
                        case PlantSeed::PlantType::ICECAP:     brewedType = Potion::PotionType::FROST;          break;
                        case PlantSeed::PlantType::SORROWMOSS: brewedType = Potion::PotionType::TOXIC_GAS;      break;
                        case PlantSeed::PlantType::DREAMWEED:  brewedType = Potion::PotionType::INVISIBILITY;   break;
                        case PlantSeed::PlantType::SUNGRASS:   brewedType = Potion::PotionType::HEALING;        break;
                        case PlantSeed::PlantType::EARTHROOT:  brewedType = Potion::PotionType::PARALYTIC_GAS; break;
                        case PlantSeed::PlantType::FADELEAF:   brewedType = Potion::PotionType::MIND_VISION;    break;
                        case PlantSeed::PlantType::ROTBERRY:   brewedType = Potion::PotionType::STRENGTH;       break;
                        default:                               brewedType = Potion::PotionType::HEALING;        break;
                    }
                }

                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_PUFF).get());
                noosa::Audio::play(noosa::Audio::getSample(Assets::SND_EVOKE).get());
                FX::burst(cell, 0xffffff88, 12);
                FX::burst(cell, 0x44aa33FF, 8);
                GLog::p("The alchemy pot bubbles vigorously and brews a potion!");
                Dungeon::level->drop(std::make_unique<Potion>(brewedType), cell);
            }
        }
        return;
    }

    // 3. Direct Hit on Entity (Hero or Mob)
    Char* target = Actor::findChar(cell);
    if (target != nullptr) {
        const SeedEffect& effect = SeedRegistry::get(type);
        if (effect.activate) {
            effect.activate(cell, target, true);
        }
        if (Dungeon::visible[cell]) {
            GLog::i("The " + effect.seedName + " strikes " + target->name + " and bursts!");
        }
        return;
    }

    // 4. Planting on valid ground
    if (SeedRegistry::isPlantable(Dungeon::level.get(), cell)) {
        Dungeon::level->plant(type, cell);
        if (Dungeon::visible[cell]) {
            noosa::Audio::play(noosa::Audio::getSample(Assets::SND_PLANT).get());
            FX::burst(cell, 0x44aa33FF, 6);
            GLog::i("The " + SeedRegistry::get(type).seedName + " sprouts into the ground.");
        }
        return;
    }

    // 5. Invalid surface / untillable obstacle
    if (Dungeon::visible[cell]) {
        GLog::i("The seed bounces off and drops onto the floor.");
    }
    Dungeon::level->drop(std::make_unique<PlantSeed>(type), cell);
}

} // namespace pd