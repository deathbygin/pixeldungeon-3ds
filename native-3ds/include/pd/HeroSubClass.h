#pragma once

#include <string>

namespace pd {

enum class HeroSubClass {
    None,
    Gladiator,
    Berserker,
    Warlock,
    Battlemage,
    Assassin,
    Freerunner,
    Sniper,
    Warden
};

inline const char* subClassTitle(HeroSubClass sc) {
    switch (sc) {
        case HeroSubClass::Gladiator:  return "Gladiator";
        case HeroSubClass::Berserker:  return "Berserker";
        case HeroSubClass::Warlock:    return "Warlock";
        case HeroSubClass::Battlemage: return "Battlemage";
        case HeroSubClass::Assassin:   return "Assassin";
        case HeroSubClass::Freerunner: return "Freerunner";
        case HeroSubClass::Sniper:     return "Sniper";
        case HeroSubClass::Warden:     return "Warden";
        default:                       return "";
    }
}

inline const char* subClassDesc(HeroSubClass sc) {
    switch (sc) {
        case HeroSubClass::Gladiator:
            return "A successful attack with a melee weapon allows the Gladiator to start a combo, in which every next successful hit inflicts more damage.";
        case HeroSubClass::Berserker:
            return "When severely wounded, the Berserker enters a state of wild fury significantly increasing his damage output.";
        case HeroSubClass::Warlock:
            return "After killing an enemy the Warlock consumes its soul. It heals his wounds and satisfies his hunger.";
        case HeroSubClass::Battlemage:
            return "When fighting with a wand in his hands, the Battlemage inflicts additional damage depending on the current number of charges. Every successful hit restores 1 charge to this wand.";
        case HeroSubClass::Assassin:
            return "When performing a surprise attack, the Assassin inflicts additional damage to his target.";
        case HeroSubClass::Freerunner:
            return "The Freerunner can move almost twice faster than most monsters. When running, he is much harder to hit, provided he is unencumbered and not starving.";
        case HeroSubClass::Sniper:
            return "Snipers are able to detect weak points in an enemy's armor, effectively ignoring it when using a missile weapon.";
        case HeroSubClass::Warden:
            return "Having a strong connection with forces of nature gives Wardens an ability to gather dewdrops and seeds from plants. Trampling high grass also grants a temporary armor buff.";
        default:
            return "";
    }
}

} // namespace pd

