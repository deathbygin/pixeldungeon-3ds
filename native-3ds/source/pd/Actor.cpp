#include "pd/Actor.h"
#include "pd/Char.h"
#include "pd/Dungeon.h"
#include "pd/Hero.h"
#include <climits>
#include <cfloat>
#include <cstring>

namespace pd {

float Actor::now = 0.0f;
Actor* Actor::current = nullptr;
std::vector<Actor*> Actor::all;
Char* Actor::chars[PathFinder::LENGTH] = { nullptr };

void Actor::postpone(float t) {
    if (time < now + t) {
        time = now + t;
    }
}

float Actor::cooldown() const {
    return time - now;
}

void Actor::next() {
    if (current == this) {
        current = nullptr;
    }
}

void Actor::clear() {
    now = 0.0f;
    current = nullptr;
    all.clear();
    std::fill_n(chars, PathFinder::LENGTH, nullptr);
}

void Actor::init() {
    clear();
}

void Actor::add(Actor* actor, float delay) {
    if (!actor) return;
    if (std::find(all.begin(), all.end(), actor) == all.end()) {
        all.push_back(actor);
        actor->time = now + delay;
    }
}

void Actor::remove(Actor* actor) {
    if (!actor) return;
    auto it = std::find(all.begin(), all.end(), actor);
    if (it != all.end()) {
        all.erase(it);
    }
    if (current == actor) {
        current = nullptr;
    }
}

void Actor::occupyCell(Char* ch) {
    if (ch && ch->pos >= 0 && ch->pos < PathFinder::LENGTH) {
        chars[ch->pos] = ch;
    }
}

void Actor::freeCell(int pos) {
    if (pos >= 0 && pos < PathFinder::LENGTH) {
        chars[pos] = nullptr;
    }
}

Char* Actor::findChar(int pos) {
    if (pos >= 0 && pos < PathFinder::LENGTH) {
        return chars[pos];
    }
    return nullptr;
}

bool Actor::process() {
    if (current != nullptr) {
        return false;
    }

    bool doNext = false;
    do {
        now = FLT_MAX;
        current = nullptr;

        std::fill_n(chars, PathFinder::LENGTH, nullptr);

        for (Actor* a : all) {
            if (!a) continue;
            if (a->time < now) {
                now = a->time;
                current = a;
            }
            Char* ch = a->asChar();
            if (ch && ch->isAlive() && ch->pos >= 0 && ch->pos < PathFinder::LENGTH) {
                chars[ch->pos] = ch;
            }
        }

        if (current != nullptr) {
            doNext = current->act();
            if (doNext && Dungeon::hero && !Dungeon::hero->isAlive()) {
                doNext = false;
                current = nullptr;
            }
        } else {
            doNext = false;
        }

    } while (doNext);

    return true;
}

} // namespace pd
