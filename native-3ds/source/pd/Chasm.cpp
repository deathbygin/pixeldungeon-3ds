#include "pd/Chasm.h"
#include "pd/Dungeon.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/Buff.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Random.h"
#include "pd/Assets.h"
#include "pd/Badges.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"

namespace pd {

void Chasm::heroFall(int pos) {
    noosa::Audio::play(noosa::Audio::getSample(Assets::SND_FALLING).get());

    if (Dungeon::hero && Dungeon::hero->isAlive()) {
        Dungeon::hero->interrupt();
        GLog::n("You fall into the dark abyss!");

        Room* r = Dungeon::level ? Dungeon::level->room(pos) : nullptr;
        if (r && r->type == Room::Type::WEAK_FLOOR) {
            Level::pitRoomNeeded = true;
        }

        Dungeon::nextLevel();
        heroLand();
    }
}

void Chasm::heroLand() {
    Hero* hero = Dungeon::hero.get();
    if (!hero) return;

    FX::burst(hero->pos, 0xcc2222FFu, 10);
    if (noosa::Camera::main) {
        noosa::Camera::main->shake(0.2f, 4.0f);
    }

    hero->affect<Cripple>();
    int dmg = Random::IntRange(hero->HT / 3, hero->HT / 2);
    hero->damage(dmg, nullptr);

    if (!hero->isAlive()) {
        GLog::n("You fell to death...");
        Badges::validateDeathFromFalling();
    }
}

void Chasm::mobFall(Mob* mob) {
    if (!mob) return;
    GLog::i(mob->name + " fell into the chasm!");
    mob->die(nullptr);
}

} // namespace pd

