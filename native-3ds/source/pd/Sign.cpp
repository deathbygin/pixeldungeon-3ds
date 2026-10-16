#include "pd/Sign.h"
#include "pd/Dungeon.h"
#include "pd/Level.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"

namespace pd {

static Sign::MessageCallback s_messageCallback = nullptr;

static const char* TIPS[] = {
    // Sewers (Depths 1-4)
    "Don't overestimate your strength, use weapons and armor you can handle.",
    "Not all doors in the dungeon are visible at first sight. If you are stuck, search for hidden doors.",
    "Remember, that raising your strength is not the only way to access better equipment. You can go "
        "the other way, lowering its strength requirement with Scrolls of Upgrade.",
    "You can spend your gold in shops on deeper levels of the dungeon. The first one is on the 6th level.",

    // Sewer Boss (Depth 5: Goo)
    "Beware of Goo!",

    // Prison (Depths 6-9)
    "Pixel-Mart - all you need for successful adventure!",
    "Identify your potions and scrolls as soon as possible. Don't put it off to the moment "
        "when you actually need them.",
    "Being hungry doesn't hurt, but starving does hurt.",
    "Surprise attack has a better chance to hit. For example, you can ambush your enemy behind "
        "a closed door when you know it is approaching.",

    // Prison Boss (Depth 10: Tengu)
    "Don't let The Tengu out!",

    // Caves (Depths 11-14)
    "Pixel-Mart. Spend money. Live longer.",
    "When you're attacked by several monsters at the same time, try to retreat behind a door.",
    "If you are burning, you can't put out the fire in the water while levitating.",
    "There is no sense in possessing more than one Ankh at the same time, because you will lose them upon resurrecting.",

    // Caves Boss (Depth 15: DM-300)
    "DANGER! Heavy machinery can cause injury, loss of limbs or death!",

    // Metropolis (Depths 16-19)
    "Pixel-Mart. A safer life in dungeon.",
    "When you upgrade an enchanted weapon, there is a chance to destroy that enchantment.",
    "Weapons and armors deteriorate faster than wands and rings, but there are more ways to fix them.",
    "The only way to obtain a Scroll of Wipe Out is to receive it as a gift from the dungeon spirits.",

    // Metropolis Boss (Depth 20: King of Dwarves)
    "No weapons allowed in the presence of His Majesty!",

    // Demon Halls (Depth 21: Last Shop)
    "Pixel-Mart. Special prices for demon hunters!"
};

static constexpr size_t NUM_TIPS = sizeof(TIPS) / sizeof(TIPS[0]);
static const char* TXT_BURN = "As you try to read the sign it bursts into greenish flames.";

void Sign::setMessageCallback(MessageCallback cb) {
    s_messageCallback = std::move(cb);
}

const char* Sign::getTip(int depth) {
    int index = depth - 1;
    if (index >= 0 && static_cast<size_t>(index) < NUM_TIPS) {
        return TIPS[index];
    }
    return nullptr;
}

void Sign::read(int pos) {
    int index = Dungeon::depth - 1;
    if (index >= 0 && static_cast<size_t>(index) < NUM_TIPS) {
        std::string tip = TIPS[index];
        GLog::i(tip);
        if (s_messageCallback) {
            s_messageCallback(tip);
        }
    } else {
        // Depths 22+: Sign bursts into greenish flames
        if (Dungeon::level) {
            Dungeon::level->destroy(pos);
        }
        FX::burst(pos, 0x44dd44FF, 12, 30.0f, 60.0f, -20.0f);
        Sample::playAt(Assets::SND_BURNING, pos);
        GLog::w(TXT_BURN);
    }
}

} // namespace pd

