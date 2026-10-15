#pragma once

#include "Actor.h"
#include "Buff.h"
#include <string>
#include <vector>
#include <utility>
#include <memory>

namespace pd {

class CharSprite;

class Char : public Actor {
public:
    std::string name = "Character";
    int pos = 0;
    int HP = 20;
    int HT = 20;

    float baseSpeed = 1.0f;
    bool paralysed = false;
    bool rooted = false;
    bool flying = false;
    int invisible = 0;
    int viewDistance = 8;
    bool spriteVisible = true;

    std::unique_ptr<CharSprite> sprite;

    std::vector<Buff*> buffs;

    Char();
    ~Char() override;

    bool act() override;
    Char* asChar() override { return this; }

    virtual CharSprite* getSprite();

    virtual int attackSkill(Char* target);
    virtual int defenseSkill(Char* attacker);
    virtual int dr();
    virtual int damageRoll();
    virtual int attackProc(Char* enemy, int damage);
    virtual int defenseProc(Char* enemy, int damage);

    bool attack(Char* enemy);
    virtual void damage(int dmg, Actor* src);
    virtual void die(Actor* src);
    virtual void interrupt();

    virtual void move(int step);
    virtual void place(int cell);
    virtual int stealth() const { return 0; }
    virtual float speed() const;
    bool isAlive() const { return HP > 0; }

    void addBuff(Buff* b);
    void removeBuff(Buff* b);

    Buff* buff(BuffType type) const;

    template<typename T>
    T* getBuff() const {
        return static_cast<T*>(buff(T::STATIC_TYPE));
    }

    template<typename T, typename... Args>
    T* affect(Args&&... args) {
        T* existing = getBuff<T>();
        if (existing) {
            return existing;
        }
        T* b = new T(std::forward<Args>(args)...);
        addBuff(b);
        return b;
    }
};

} // namespace pd
