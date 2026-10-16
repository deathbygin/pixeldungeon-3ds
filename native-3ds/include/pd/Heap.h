#pragma once

#include "Item.h"
#include <vector>
#include <memory>
#include <string>

namespace pd {

class Hero;

class Heap {
public:
    enum class Type {
        HEAP,
        FOR_SALE,
        CHEST,
        LOCKED_CHEST,
        CRYSTAL_CHEST,
        TOMB,
        SKELETON,
        MIMIC,
        HIDDEN
    };

    Type type = Type::HEAP;
    int pos = 0;
    std::vector<std::unique_ptr<Item>> items;

    Heap() = default;
    explicit Heap(int cell, Type tp = Type::HEAP);
    ~Heap() = default;

    int image() const;
    void open(Hero* hero);
    int size() const { return static_cast<int>(items.size()); }
    bool isEmpty() const { return items.empty(); }

    std::unique_ptr<Item> pickUp();
    Item* peek() const;
    void drop(std::unique_ptr<Item> item);
    void replace(Item* oldItem, std::unique_ptr<Item> newItem);

    void burn();
    void freeze();
};

} // namespace pd

