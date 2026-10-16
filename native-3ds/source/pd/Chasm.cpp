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
#include "pd/Sample.h"
#include "pd/SaveManager.h"
#include "noosa/Audio.h"
#include "noosa/Camera.h"

namespace pd {

bool Chasm::jumpConfirmed = false;

void Chasm::heroFall(int pos) {
    jumpConfirmed = false;
    Sample::play(Assets::SND_FALLING);

    if (Dungeon::hero && Dungeon::hero->isAlive()) {
        Dungeon::hero->interrupt();
        GLog::n("You fall into the dark abyss!");

        Room* r = Dungeon::level ? Dungeon::level->room(pos) : nullptr;
        if (r && r->type == Room::Type::WEAK_FLOOR) {
            Level::pitRoomNeeded = true;
        }

        Dungeon::pendingTransition = LevelTransition::FALL;
    }
}

void Chasm::heroLand() {
    Hero* hero = Dungeon::hero.get();
    if (!hero) return;

    Sample::play(Assets::SND_BONES);
    Sample::play(Assets::SND_HIT);

    if (noosa::Camera::main) {
        noosa::Camera::main->shake(0.25f, 5.0f);
    }

    FX::burst(hero->pos, 0xcc1111FFu, 16);

    hero->affect<Cripple>();
    hero->affect<Bleeding>(5);

    int dmg = Random::IntRange(hero->HT / 3, hero->HT / 2);
    FX::floatingText(hero->pos, std::to_string(dmg), 0xcc1111FFu);
    hero->damage(dmg, nullptr);

    if (!hero->isAlive()) {
        Sample::play(Assets::SND_DEATH);
        SaveManager::deleteSave();
        GLog::n("You fell to death...");
        Badges::validateDeathFromFalling();
    }
}

void Chasm::mobFall(Mob* mob) {
    if (!mob) return;
    Sample::playAt(Assets::SND_FALLING, mob->pos);
    GLog::i(mob->name + " fell into the chasm!");
    mob->die(nullptr);
}

} // namespace pd

