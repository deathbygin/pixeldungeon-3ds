#include "pd/Heap.h"
#include "pd/Hero.h"
#include "pd/Dungeon.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
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
    case Type::MIMIC:
        Sample::playAt(Assets::SND_MIMIC, pos);
        FX::burst(pos, 0x9955ffFFu, 8);
        type = Type::HEAP;
        break;
    case Type::SKELETON:
        GLog::i("You search the skeletal remains...");
        FX::burst(pos, 0xd0c090FFu, 8);
        Sample::playAt(Assets::SND_BONES, pos);
        type = Type::HEAP;
        break;
    case Type::TOMB:
        GLog::w("You disturbed the ancient tomb!");
        FX::burst(pos, 0x505050FFu, 10);
        Sample::playAt(Assets::SND_TOMB, pos);
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

void Heap::burn() {
    if (type != Type::HEAP) return;

    bool burnt = false;
    for (auto it = items.begin(); it != items.end();) {
        if ((*it)->category() == ItemCategory::Scroll) {
            it = items.erase(it);
            burnt = true;
        } else if ((*it)->name == "Dewdrop") {
            it = items.erase(it);
            burnt = true;
        } else {
            ++it;
        }
    }

    if (burnt) {
        Sample::playAt(Assets::SND_BURNING, pos);
        FX::burst((pos % 32) * 16.0f + 8.0f, (pos / 32) * 16.0f + 8.0f, 0xee7722FF, 6);
    }
}

void Heap::freeze() {
    if (type != Type::HEAP) return;

    for (auto& item : items) {
        if (item && item->name == "Mystery Meat") {
            item->name = "Frozen Carpaccio";
            item->image = ItemSpriteSheet::CARPACCIO;
        }
    }
}

} // namespace pd
