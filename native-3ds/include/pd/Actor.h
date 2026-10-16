#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include "PathFinder.h"

namespace pd {

class Char;
class Buff;

class Actor {
public:
    static constexpr float TICK = 1.0f;

    float time = 0.0f;
    int id = 0;

    Actor() = default;
    virtual ~Actor() = default;

    virtual bool act() = 0;
    virtual Char* asChar() { return nullptr; }
    virtual Buff* asBuff() { return nullptr; }
    virtual class Blob* asBlob() { return nullptr; }

    virtual void spend(float t) { time += t; }
    void spendAndNext(float t) { spend(t); next(); }
    void postpone(float t);
    float cooldown() const;
    void next();

    // Static turn scheduler
    static float now;
    static Actor* current;
    static std::vector<Actor*> all;
    static Char* chars[PathFinder::LENGTH];

    static void init();
    static void clear();
    static void add(Actor* actor, float delay = 0.0f);
    static void remove(Actor* actor);
    static bool process(); // Executes actors until user input is needed or no actors remain

    static void occupyCell(Char* ch);
    static void freeCell(int pos);
    static Char* findChar(int pos);
};

} // namespace pd

