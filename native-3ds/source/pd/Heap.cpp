#include "pd/Heap.h"
#include "pd/Hero.h"
#include "pd/Dungeon.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/Mob.h"
#include "pd/Buff.h"
#include "noosa/Audio.h"
#include <algorithm>

namespace pd {

Heap::Heap(int cell, Type tp)
    : type(tp), pos(cell) {
}

int Heap::image() const {
    switch (type) {
    case Type::HEAP:
    case Type::FOR_SALE:
        return !items.empty() ? items.front()->image : 0;
    case Type::CHEST:
    case Type::MIMIC:
        return ItemSpriteSheet::CHEST;
    case Type::LOCKED_CHEST:
        return ItemSpriteSheet::LOCKED_CHEST;
    case Type::CRYSTAL_CHEST:
        return ItemSpriteSheet::CRYSTAL_CHEST;
    case Type::TOMB:
        return ItemSpriteSheet::TOMB;
    case Type::SKELETON:
        return ItemSpriteSheet::BONES;
    case Type::HIDDEN:
        return ItemSpriteSheet::HIDDEN;
    default:
        return 0;
    }
}

void Heap::open(Hero* hero) {
    switch (type) {
    case Type::CHEST:
        GLog::i("You opened the chest.");
        Sample::playAt(Assets::SND_UNLOCK, pos);
        FX::burst(pos, 0xffd700FFu, 6);
        type = Type::HEAP;
        break;
    case Type::MIMIC: {
        int p = pos;
        auto itemsToMove = std::move(items);
        if (Mimic::spawnAt(p, std::move(itemsToMove))) {
            if (Dungeon::level) {
                Dungeon::level->removeHeap(this);
            }
            return;
        } else {
            type = Type::CHEST;
        }
        break;
    }
    case Type::SKELETON: {
        GLog::i("You search the skeletal remains...");
        FX::burst(pos, 0xd0c090FFu, 8);
        Sample::playAt(Assets::SND_BONES, pos);
        bool hasCursed = false;
        for (auto& item : items) {
            if (item && item->isCursed()) {
                hasCursed = true;
                break;
            }
        }
        if (hasCursed) {
            if (!Wraith::spawnAt(pos)) {
                if (hero) {
                    hero->damage(hero->HP / 2, nullptr);
                }
            }
            Sample::playAt(Assets::SND_CURSED, pos);
        }
        type = Type::HEAP;
        break;
    }
    case Type::TOMB:
        GLog::w("You disturbed the ancient tomb!");
        FX::burst(pos, 0x505050FFu, 10);
        Sample::playAt(Assets::SND_TOMB, pos);
        Wraith::spawnAround(hero ? hero->pos : pos);
        type = Type::HEAP;
        break;
    case Type::LOCKED_CHEST:
    case Type::CRYSTAL_CHEST: {
        if (!hero || !hero->belongings.getKey("Golden Key", Dungeon::depth)) {
            GLog::w("You need a golden key to open this chest!");
            return;
        }
        hero->belongings.consumeKey("Golden Key", Dungeon::depth);
        if (type == Type::CRYSTAL_CHEST) {
            GLog::p("You unlock the crystal chest with your golden key!");
        } else {
            GLog::p("You unlock the chest with your golden key!");
        }
        Sample::playAt(Assets::SND_UNLOCK, pos);
        FX::burst(pos, 0xffd700FFu, 10);
        type = Type::HEAP;
        break;
    }
    default:
        type = Type::HEAP;
        break;
    }
}

std::unique_ptr<Item> Heap::pickUp() {
    if (items.empty()) return nullptr;
    auto it = std::move(items.front());
    items.erase(items.begin());
    return it;
}

Item* Heap::peek() const {
    return items.empty() ? nullptr : items.front().get();
}

void Heap::drop(std::unique_ptr<Item> item) {
    if (!item) return;

    if (item->isStackable()) {
        for (auto& existing : items) {
            if (existing && existing->category() == item->category()) {
                if (item->category() == ItemCategory::Potion) {
                    if (static_cast<Potion*>(existing.get())->type == static_cast<Potion*>(item.get())->type) {
                        existing->quantity += item->quantity;
                        return;
                    }
                } else if (item->category() == ItemCategory::Scroll) {
                    if (static_cast<Scroll*>(existing.get())->type == static_cast<Scroll*>(item.get())->type) {
                        existing->quantity += item->quantity;
                        return;
                    }
                } else if (existing->name == item->name) {
                    existing->quantity += item->quantity;
                    return;
                }
            }
        }
    }

    if (item->name == "Dewdrop") {
        items.push_back(std::move(item));
    } else {
        items.insert(items.begin(), std::move(item));
    }
}

void Heap::replace(Item* oldItem, std::unique_ptr<Item> newItem) {
    if (!oldItem || !newItem) return;
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->get() == oldItem) {
            *it = std::move(newItem);
            return;
        }
    }
}

void Heap::burn() {
    if (type == Type::MIMIC) {
        int p = pos;
        auto itemsToMove = std::move(items);
        Mimic* m = Mimic::spawnAt(p, std::move(itemsToMove));
        if (m) {
            m->affect<Burning>(5);
            FX::burst(p, 0xee7722FF, 6);
            if (Dungeon::level) {
                Dungeon::level->removeHeap(this);
            }
            return;
        }
    }
    if (type != Type::HEAP) return;

    bool burnt = false;
    for (auto it = items.begin(); it != items.end();) {
        if ((*it)->category() == ItemCategory::Scroll) {
            it = items.erase(it);
            burnt = true;
        } else if ((*it)->name == "Dewdrop") {
            it = items.erase(it);
            burnt = true;
        } else if ((*it)->name == "Mystery Meat" || (*it)->name == "mystery meat") {
            int qty = (*it)->quantity;
            auto steak = std::make_unique<ChargrilledMeat>();
            steak->quantity = qty;
            *it = std::move(steak);
            burnt = true;
            ++it;
        } else if ((*it)->name == "honeypot" || (*it)->name == "Honeypot") {
            auto hp = std::move(*it);
            it = items.erase(it);
            burnt = true;
            static_cast<Honeypot*>(hp.get())->shatter(pos);
        } else {
            ++it;
        }
    }

    if (burnt) {
        Sample::playAt(Assets::SND_BURNING, pos);
        FX::burst(pos, 0xee7722FFu, 6);
        if (items.empty() && Dungeon::level) {
            Dungeon::level->removeHeap(this);
        }
    }
}

void Heap::freeze() {
    if (type == Type::MIMIC) {
        int p = pos;
        auto itemsToMove = std::move(items);
        Mimic* m = Mimic::spawnAt(p, std::move(itemsToMove));
        if (m) {
            m->affect<Frost>();
            FX::burst(p, 0x66ccffFF, 6);
            if (Dungeon::level) {
                Dungeon::level->removeHeap(this);
            }
            return;
        }
    }
    if (type != Type::HEAP) return;

    bool frozen = false;
    for (auto& item : items) {
        if (item && (item->name == "Mystery Meat" || item->name == "mystery meat")) {
            int qty = item->quantity;
            auto carpaccio = std::make_unique<FrozenCarpaccio>();
            carpaccio->quantity = qty;
            item = std::move(carpaccio);
            frozen = true;
        }
    }

    if (frozen) {
        FX::burst(pos, 0x66ccffFFu, 6);
    }
}

} // namespace pd
