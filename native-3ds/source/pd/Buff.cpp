#include "pd/Buff.h"
#include "pd/Char.h"
#include "pd/Hero.h"
#include "pd/Dungeon.h"
#include "pd/Terrain.h"
#include "pd/Random.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Mob.h"
#include <algorithm>

namespace pd {

void Buff::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, TICK);
}

void Buff::detach() {
    if (target) {
        target->removeBuff(this);
        target = nullptr;
    }
    Actor::remove(this);
    delete this;
}

bool FlavourBuff::act() {
    detach();
    return true;
}

// -----------------------------------------------------------------------------
// Hunger
// -----------------------------------------------------------------------------
std::string Hunger::desc() const {
    if (isStarving()) {
        return "You are starving! You will periodically take damage and cannot regenerate health naturally.";
    } else if (isHungry()) {
        return "You are hungry. Find and eat food before you begin to starve.";
    }
    return "You are well-nourished.";
}

bool Hunger::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    if (isStarving()) {
        if (Random::Float() < 0.3f && (target->HP > 1 || !target->paralysed)) {
            GLog::n("You are starving!");
            target->damage(1, this);
            target->interrupt();
        }
    } else {
        float newLevel = level + STEP;
        if (newLevel >= STARVING) {
            GLog::n("You are starving!");
            target->interrupt();
        } else if (newLevel >= HUNGRY && level < HUNGRY) {
            GLog::w("You are hungry.");
        }
        level = newLevel;
    }

    float step = STEP;
    if (target == Dungeon::hero.get()) {
        Hero* hero = static_cast<Hero*>(target);
        if (hero->heroClass == Hero::HeroClass::Rogue) {
            step *= 1.2f;
        }
    }
    if (target->buff(BuffType::Shadows) != nullptr) {
        step *= 1.5f;
    }
    spend(step * TICK);
    return true;
}

void Hunger::satisfy(float energy) {
    level = std::max(0.0f, level - energy);
    if (level > STARVING) {
        level = STARVING;
    }
}

// -----------------------------------------------------------------------------
// Regeneration
// -----------------------------------------------------------------------------
std::string Regeneration::desc() const {
    return "Your wounds are slowly closing over time, restoring 1 HP every 10 turns.";
}

bool Regeneration::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    if (target->HP < target->HT) {
        Hunger* h = target->getBuff<Hunger>();
        if (!h || !h->isStarving()) {
            target->HP++;
            FX::floatingText(target->pos, "+1", 0x38b764FF);
        }
    }

    spend(REGENERATION_DELAY * TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Burning
// -----------------------------------------------------------------------------
std::string Burning::desc() const {
    return "Flames are scorching your flesh, inflicting 1-5 fire damage per turn and burning scrolls in your pack.";
}

void Burning::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        FX::floatingText(target->pos, "burning", 0xee7722FF);
    }
}

bool Burning::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    int dmg = Random::IntRange(1, 5);
    target->damage(dmg, this);
    FX::burst((target->pos % Level::WIDTH) * 16.0f + 8.0f,
              (target->pos / Level::WIDTH) * 16.0f + 8.0f,
              0xee7722FF, 4, 15.0f, 35.0f, -40.0f);
    FX::floatingText(target->pos, std::to_string(dmg), 0xee7722FF);

    // Flammable inventory check for Hero
    if (target == Dungeon::hero.get()) {
        Hero* hero = Dungeon::hero.get();
        std::vector<size_t> scrollIndices;
        std::vector<size_t> meatIndices;
        for (size_t i = 0; i < hero->belongings.backpack.size(); ++i) {
            if (hero->belongings.backpack[i]) {
                if (hero->belongings.backpack[i]->category() == ItemCategory::Scroll) {
                    scrollIndices.push_back(i);
                } else if (hero->belongings.backpack[i]->name == "Mystery Meat" || hero->belongings.backpack[i]->name == "mystery meat") {
                    meatIndices.push_back(i);
                }
            }
        }
        if (!meatIndices.empty()) {
            size_t idx = meatIndices[Random::Int(static_cast<int>(meatIndices.size()))];
            int qty = hero->belongings.backpack[idx]->quantity;
            auto steak = std::make_unique<ChargrilledMeat>();
            steak->quantity = qty;
            hero->belongings.backpack[idx] = std::move(steak);
            GLog::w("Your mystery meat is chargrilled!");
            FX::burst(hero->pos, 0xee7722FF, 6);
        } else if (!scrollIndices.empty() && Random::Int(2) == 0) {
            size_t idx = scrollIndices[Random::Int(static_cast<int>(scrollIndices.size()))];
            std::string name = hero->belongings.backpack[idx]->displayName();
            hero->belongings.backpack.erase(hero->belongings.backpack.begin() + idx);
            GLog::w(name + " burns up!");
            FX::burst(hero->pos, 0xee7722FF, 6);
        }
    }

    // Stepping into water immediately extinguishes flame
    if (Dungeon::level && Dungeon::level->water[target->pos] && !target->flying) {
        FX::waterSplash(target->pos, 6);
        detach();
        return true;
    }

    durationTicks--;
    if (durationTicks <= 0) {
        detach();
    } else {
        spend(TICK);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Poison
// -----------------------------------------------------------------------------
std::string Poison::desc() const {
    return "Toxic venom courses through your veins, dealing damage on each turn.";
}

void Poison::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        FX::floatingText(target->pos, "poisoned", 0x38b764FF);
    }
}

bool Poison::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    int dmg = (durationTicks / 3) + 1;
    target->damage(dmg, this);
    FX::floatingText(target->pos, std::to_string(dmg), 0x38b764FF);

    durationTicks--;
    if (durationTicks <= 0) {
        detach();
    } else {
        spend(TICK);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Bleeding
// -----------------------------------------------------------------------------
std::string Bleeding::desc() const {
    return "Deep open wounds cause you to bleed profusely, taking damage every turn and each time you move.";
}

void Bleeding::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        FX::floatingText(target->pos, "bleeding", 0xd63030FF);
    }
}

bool Bleeding::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    level = Random::IntRange(level / 2, level);
    if (level > 0) {
        target->damage(level, this);
        FX::blood(target->pos, std::min(10 * level / std::max(1, target->HT), 10));
        FX::floatingText(target->pos, std::to_string(level), 0xd63030FF);
        spend(TICK);
    } else {
        detach();
    }
    return true;
}

// -----------------------------------------------------------------------------
// Ooze
// -----------------------------------------------------------------------------
std::string Ooze::desc() const {
    return "Corrosive caustic ooze clings to you, steadily burning your flesh. Step into water to wash it away!";
}

void Ooze::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        FX::floatingText(target->pos, "caustic ooze", 0x000000FFu);
    }
}

bool Ooze::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    target->damage(damage, this);
    if (target == Dungeon::hero.get()) {
        GLog::n("Caustic ooze burns your skin! (-" + std::to_string(damage) + " HP)");
    }
    FX::burst(target->pos, 0x000000FFu, 4);

    if (Dungeon::level && Dungeon::level->water[target->pos]) {
        if (target == Dungeon::hero.get()) {
            GLog::p("The water washes away the caustic ooze!");
        }
        detach();
        return true;
    }

    spend(TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Paralysis
// -----------------------------------------------------------------------------
std::string Paralysis::desc() const {
    return "You are completely paralyzed and unable to move, attack, or cast. Taking damage may snap you out of it.";
}

void Paralysis::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        target->paralysed = true;
        FX::floatingText(target->pos, "paralysed", 0xeecc22FF);
    }
}

void Paralysis::detach() {
    Char* ch = target;
    Buff::detach();
    if (ch) {
        ch->unfreeze();
    }
}

bool Paralysis::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    durationTicks--;
    if (durationTicks <= 0) {
        detach();
    } else {
        spend(TICK);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Roots
// -----------------------------------------------------------------------------
std::string Roots::desc() const {
    return "Thick bramble and roots anchor you to the ground. You cannot walk, but you can still fight and use items.";
}

void Roots::attachTo(Char* ch) {
    if (!ch || ch->flying) {
        detach();
        return;
    }
    target = ch;
    Actor::add(this, TICK * 5.0f);
    target->rooted = true;
    FX::floatingText(target->pos, "rooted", 0x88bb44FF);
}

void Roots::detach() {
    if (target) {
        target->rooted = false;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Cripple
// -----------------------------------------------------------------------------
std::string Cripple::desc() const {
    return "A severe wound cripples your legs, halving your movement speed.";
}

void Cripple::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "crippled", 0x88bb44FF);
    }
}

// -----------------------------------------------------------------------------
// Frost
// -----------------------------------------------------------------------------
std::string Frost::desc() const {
    return "Encased in solid ice! You cannot take any action until you thaw, or fire/damage melts the ice.";
}

void Frost::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        target->paralysed = true;
        Buff* b = target->buff(BuffType::Burning);
        if (b) b->detach();
        FX::floatingText(target->pos, "frozen", 0x64beffff);

        if (target == Dungeon::hero.get()) {
            Hero* hero = Dungeon::hero.get();
            for (auto& it : hero->belongings.backpack) {
                if (it && (it->name == "Mystery Meat" || it->name == "mystery meat")) {
                    int qty = it->quantity;
                    auto carpaccio = std::make_unique<FrozenCarpaccio>();
                    carpaccio->quantity = qty;
                    it = std::move(carpaccio);
                    GLog::w("Your mystery meat is frozen into carpaccio!");
                    FX::burst(hero->pos, 0x66ccffFF, 6);
                    break;
                }
            }
        }
    }
}

void Frost::detach() {
    Char* ch = target;
    Buff::detach();
    if (ch) {
        ch->unfreeze();
    }
}

// -----------------------------------------------------------------------------
// Vertigo
// -----------------------------------------------------------------------------
std::string Vertigo::desc() const {
    return "Your head spins uncontrollably! Movement attempts are scrambled into random directions.";
}

void Vertigo::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "vertigo", 0xeecc22FF);
    }
}

// -----------------------------------------------------------------------------
// Slow
// -----------------------------------------------------------------------------
std::string Slow::desc() const {
    return "Your actions and movements take twice as long to complete.";
}

void Slow::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "slowed", 0x888899FF);
    }
}

// -----------------------------------------------------------------------------
// Speed (Haste)
// -----------------------------------------------------------------------------
std::string Speed::desc() const {
    return "You move and react with blinding speed, doubling the pace of your actions.";
}

void Speed::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "speed", 0x38b764FF);
    }
}

// -----------------------------------------------------------------------------
// Sleep
// -----------------------------------------------------------------------------
std::string Sleep::desc() const {
    return "Soundly asleep and dreaming. Taking damage or loud noise will awaken you.";
}

void Sleep::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, TICK);
    if (target) {
        FX::floatingText(target->pos, "sleep", 0x88aaeeFF);
    }
}

void Sleep::detach() {
    Buff::detach();
}

bool Sleep::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }
    spend(TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Amok
// -----------------------------------------------------------------------------
std::string Amok::desc() const {
    return "Blinded by sheer bloodlust and frenzy, lashing out at the closest entity regardless of allegiance.";
}

void Amok::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "amok", 0xee4444FF);
    }
}

// -----------------------------------------------------------------------------
// Rage
// -----------------------------------------------------------------------------
std::string Rage::desc() const {
    return "Blinded with rage, this creature will recklessly pursue and attack.";
}

void Rage::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "rage", 0xff3333FF);
    }
}

// -----------------------------------------------------------------------------
// Terror
// -----------------------------------------------------------------------------
std::string Terror::desc() const {
    return "Overcome with horrifying panic, compelled to turn and flee blindly from danger.";
}

void Terror::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "fleeing", 0xcc88eeFF);
    }
}

// -----------------------------------------------------------------------------
// Charm
// -----------------------------------------------------------------------------
std::string Charm::desc() const {
    return "Enamored and pacified, unable to muster any intent or action to harm your charmer.";
}

void Charm::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "charmed", 0xff66ccFF);
    }
}

// -----------------------------------------------------------------------------
// Blindness
// -----------------------------------------------------------------------------
std::string Blindness::desc() const {
    return "Pitch darkness surrounds you; your field of view is reduced to your immediate surroundings.";
}

void Blindness::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        ch->viewDistance = 1;
        Dungeon::observe();
        FX::floatingText(target->pos, "blinded", 0x555555FF);
    }
}

void Blindness::detach() {
    if (target && Dungeon::level) {
        target->viewDistance = Dungeon::level->viewDistance;
        Dungeon::observe();
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Invisibility
// -----------------------------------------------------------------------------
std::string Invisibility::desc() const {
    return "You are completely invisible to enemies. Attacking or throwing will break your cloaking.";
}

void Invisibility::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        target->invisible++;
        FX::floatingText(target->pos, "invisible", 0xa5cde4FF);
    }
}

void Invisibility::detach() {
    if (target) {
        target->invisible--;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Levitation
// -----------------------------------------------------------------------------
std::string Levitation::desc() const {
    return "Floating gracefully above the ground, immune to pitfall chasms, floor traps, and liquids.";
}

void Levitation::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        target->flying = true;
        Buff* r = target->buff(BuffType::Roots);
        if (r) r->detach();
        FX::floatingText(target->pos, "levitating", 0x56bb77FF);
    }
}

void Levitation::detach() {
    if (target) {
        target->flying = false;
        if (Dungeon::level) {
            Dungeon::level->press(target->pos, target);
        }
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Light (Torch)
// -----------------------------------------------------------------------------
std::string Light::desc() const {
    return "The radiant glow of a torch extends your vision distance across dark subterranean passages.";
}

void Light::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (ch) {
        ch->viewDistance = std::max(ch->viewDistance, DISTANCE);
        Dungeon::observe();
        FX::floatingText(target->pos, "illuminated", 0xffeb88FF);
    }
}

void Light::detach() {
    if (target && Dungeon::level) {
        target->viewDistance = Dungeon::level->viewDistance;
        Dungeon::observe();
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Shadows (Rogue stealth)
// -----------------------------------------------------------------------------
std::string Shadows::desc() const {
    return "Melding into the gloom and shadows, masking your presence from prowling creatures.";
}

void Shadows::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, TICK);
    if (target) {
        target->invisible++;
        FX::floatingText(target->pos, "shadows", 0x7777aaFF);
    }
}

void Shadows::detach() {
    if (target) {
        target->invisible--;
    }
    Buff::detach();
}

bool Shadows::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }
    spend(TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Mind Vision
// -----------------------------------------------------------------------------
std::string MindVision::desc() const {
    return "Your consciousness expands across the floor, sensing the exact location of all living beings.";
}

void MindVision::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    Dungeon::observe();
    if (target) {
        FX::floatingText(target->pos, "mind vision", 0xaa66ccFF);
    }
}

void MindVision::detach() {
    Buff::detach();
    Dungeon::observe();
}

// -----------------------------------------------------------------------------
// Awareness
// -----------------------------------------------------------------------------
std::string Awareness::desc() const {
    return "Heightened spatial perception instantly detects hidden traps and secret doors.";
}

void Awareness::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "aware", 0x56bb77FF);
    }
}

// -----------------------------------------------------------------------------
// GasesImmunity
// -----------------------------------------------------------------------------
std::string GasesImmunity::desc() const {
    return "You are immune to toxic gas, paralytic vapors, and suffocating fumes.";
}

void GasesImmunity::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "immune to gas", 0xffffffFF);
    }
}

// -----------------------------------------------------------------------------
// Barkskin
// -----------------------------------------------------------------------------
std::string Barkskin::desc() const {
    return "Tough wood-like bark hardens your skin, providing bonus damage absorption that chips away per hit.";
}

void Barkskin::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (target) {
        FX::floatingText(target->pos, "barkskin", 0x886633FF);
    }
}

bool Barkskin::act() {
    if (!target || !target->isAlive()) {
        detach();
        return true;
    }

    spend(TICK);
    level--;
    if (level <= 0) {
        detach();
    }
    return true;
}

// -----------------------------------------------------------------------------
// Weakness
// -----------------------------------------------------------------------------
std::string Weakness::desc() const {
    return "Severe exhaustion saps your physical strength, reducing your effective STR by 2.";
}

void Weakness::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        if (target == Dungeon::hero.get()) {
            static_cast<Hero*>(target)->weakened = true;
        }
        FX::floatingText(target->pos, "weakened", 0xbb5555FF);
    }
}

void Weakness::detach() {
    if (target && target == Dungeon::hero.get()) {
        static_cast<Hero*>(target)->weakened = false;
    }
    Buff::detach();
}

// -----------------------------------------------------------------------------
// Combo (Gladiator)
// -----------------------------------------------------------------------------
std::string Combo::desc() const {
    return "Unleash continuous consecutive melee blows to build bonus damage against your opponents.";
}

int Combo::hit(Char* enemy, int damage) {
    (void)enemy;
    count++;
    if (count >= 3) {
        GLog::p(std::to_string(count) + " hit combo!");
        postpone(std::max(0.5f, 1.41f - count / 10.0f));
        return static_cast<int>(damage * (count - 2) / 5.0f);
    } else {
        postpone(1.1f);
        return 0;
    }
}

bool Combo::act() {
    detach();
    return true;
}

// -----------------------------------------------------------------------------
// Fury (Berserker)
// -----------------------------------------------------------------------------
std::string Fury::desc() const {
    return "Near death, a surge of adrenaline and berserker rage increases your melee damage by 50%!";
}

bool Fury::act() {
    if (!target || !target->isAlive() || target->HP > target->HT * LEVEL) {
        detach();
        return true;
    }
    spend(TICK);
    return true;
}

// -----------------------------------------------------------------------------
// Sniper's Mark
// -----------------------------------------------------------------------------
std::string SnipersMark::desc() const {
    return "A lethal crosshair locks onto the target, ignoring enemy armor on subsequent ranged shots.";
}

void SnipersMark::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, dur);
    if (target) {
        FX::floatingText(target->pos, "snipers mark", 0x38b764FF);
    }
}

// -----------------------------------------------------------------------------
// Soul Mark (Warlock)
// -----------------------------------------------------------------------------
std::string SoulMark::desc() const {
    return "Target's soul is marked; inflicting damage siphons life force and satisfies hunger.";
}

void SoulMark::attachTo(Char* ch) {
    target = ch;
    Actor::add(this, DURATION);
    if (target) {
        FX::floatingText(target->pos, "soul marked", 0x9944ccFF);
    }
}

// -----------------------------------------------------------------------------
// Plant Buffs (Sungrass & Earthroot)
// -----------------------------------------------------------------------------
std::string HerbalHealing::desc() const {
    return "Soothing sap is slowly mending your wounds as long as you remain stationary.";
}

void HerbalHealing::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (ch) pos = ch->pos;
    Actor::add(this, STEP);
}

bool HerbalHealing::act() {
    if (!target || target->pos != pos || target->HP >= target->HT) {
        detach();
    } else {
        target->HP = std::min(target->HT, target->HP + std::max(1, target->HT / 10));
        FX::burst(target->pos, 0xffee44FF, 3);
        spend(STEP);
    }
    return true;
}

std::string EarthrootArmor::desc() const {
    return "Tough, pliable roots encase you in an earthen shell, absorbing physical damage until you move.";
}

void EarthrootArmor::attachTo(Char* ch) {
    Buff::attachTo(ch);
    if (ch) pos = ch->pos;
    Actor::add(this, STEP);
}

bool EarthrootArmor::act() {
    if (!target || target->pos != pos) {
        detach();
    } else {
        spend(STEP);
    }
    return true;
}

int EarthrootArmor::absorb(int damage) {
    if (damage >= level) {
        int rem = damage - level;
        detach();
        return rem;
    } else {
        level -= damage;
        return 0;
    }
}

} // namespace pd
