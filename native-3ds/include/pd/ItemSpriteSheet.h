#pragma once

namespace pd {

class ItemSpriteSheet {
public:
    // Placeholders
    static constexpr int ANKH               = 1;
    static constexpr int RATION             = 4;
    static constexpr int WEAPON             = 5;
    static constexpr int ARMOR              = 6;
    static constexpr int RING               = 7;
    static constexpr int GOLD               = 14;
    static constexpr int SMTH               = 127;

    // Keys
    static constexpr int SKELETON_KEY       = 8;
    static constexpr int IRON_KEY           = 9;
    static constexpr int GOLDEN_KEY         = 10;

    // Melee weapons
    static constexpr int SHORT_SWORD        = 2;
    static constexpr int KNUCKLEDUSTER      = 16;
    static constexpr int QUARTERSTAFF       = 17;
    static constexpr int MACE               = 18;
    static constexpr int DAGGER             = 19;
    static constexpr int SWORD              = 20;
    static constexpr int LONG_SWORD         = 21;
    static constexpr int BATTLE_AXE         = 22;
    static constexpr int WAR_HAMMER         = 23;
    static constexpr int SPEAR              = 29;
    static constexpr int GLAIVE             = 30;

    // Missiles
    static constexpr int SHURIKEN           = 15;
    static constexpr int DART               = 31;
    static constexpr int BOOMERANG          = 106;
    static constexpr int TOMAHAWK           = 107;
    static constexpr int INCENDIARY_DART    = 108;
    static constexpr int CURARE_DART        = 109;
    static constexpr int JAVELIN            = 110;

    // Armors
    static constexpr int ARMOR_CLOTH        = 24;
    static constexpr int ARMOR_LEATHER      = 25;
    static constexpr int ARMOR_MAIL         = 26;
    static constexpr int ARMOR_SCALE        = 27;
    static constexpr int ARMOR_PLATE        = 28;
    static constexpr int ARMOR_ROGUE        = 96;
    static constexpr int ARMOR_WARRIOR      = 97;
    static constexpr int ARMOR_MAGE         = 98;
    static constexpr int ARMOR_HUNTRESS     = 99;

    // Wands
    static constexpr int WAND_MAGIC_MISSILE = 3;
    static constexpr int WAND_HOLLY         = 48;
    static constexpr int WAND_YEW           = 49;
    static constexpr int WAND_EBONY         = 50;
    static constexpr int WAND_CHERRY        = 51;
    static constexpr int WAND_TEAK          = 52;
    static constexpr int WAND_ROWAN         = 53;
    static constexpr int WAND_WILLOW        = 54;
    static constexpr int WAND_MAHOGANY      = 55;
    static constexpr int WAND_BAMBOO        = 68;
    static constexpr int WAND_PURPLEHEART   = 69;
    static constexpr int WAND_OAK           = 70;
    static constexpr int WAND_BIRCH         = 71;

    // Rings
    static constexpr int RING_DIAMOND       = 32;
    static constexpr int RING_OPAL          = 33;
    static constexpr int RING_GARNET        = 34;
    static constexpr int RING_RUBY          = 35;
    static constexpr int RING_AMETHYST      = 36;
    static constexpr int RING_TOPAZ         = 37;
    static constexpr int RING_ONYX          = 38;
    static constexpr int RING_TOURMALINE    = 39;
    static constexpr int RING_EMERALD       = 72;
    static constexpr int RING_SAPPHIRE      = 73;
    static constexpr int RING_QUARTZ        = 74;
    static constexpr int RING_AGATE         = 75;

    // Potions
    static constexpr int POTION_TURQUOISE   = 56;
    static constexpr int POTION_CRIMSON     = 57;
    static constexpr int POTION_AZURE       = 58;
    static constexpr int POTION_JADE        = 59;
    static constexpr int POTION_GOLDEN      = 60;
    static constexpr int POTION_MAGENTA     = 61;
    static constexpr int POTION_CHARCOAL    = 62;
    static constexpr int POTION_IVORY       = 63;
    static constexpr int POTION_AMBER       = 64;
    static constexpr int POTION_BISTRE      = 65;
    static constexpr int POTION_INDIGO      = 66;
    static constexpr int POTION_SILVER      = 67;

    // Scrolls
    static constexpr int SCROLL_KAUNAN      = 40;
    static constexpr int SCROLL_SOWILO      = 41;
    static constexpr int SCROLL_LAGUZ       = 42;
    static constexpr int SCROLL_YNGVI       = 43;
    static constexpr int SCROLL_GYFU        = 44;
    static constexpr int SCROLL_RAIDO       = 45;
    static constexpr int SCROLL_ISAZ        = 46;
    static constexpr int SCROLL_MANNAZ      = 47;
    static constexpr int SCROLL_NAUDIZ      = 76;
    static constexpr int SCROLL_BERKANAN    = 77;
    static constexpr int SCROLL_ODAL        = 78;
    static constexpr int SCROLL_TIWAZ       = 79;
    static constexpr int SCROLL_WIPE_OUT    = 117;

    // Seeds & Plants
    static constexpr int SEED_FIREBLOOM     = 88;
    static constexpr int SEED_ICECAP        = 89;
    static constexpr int SEED_SORROWMOSS    = 90;
    static constexpr int SEED_DREAMWEED     = 91;
    static constexpr int SEED_SUNGRASS      = 92;
    static constexpr int SEED_EARTHROOT     = 93;
    static constexpr int SEED_FADELEAF      = 94;
    static constexpr int SEED_ROTBERRY      = 95;

    // Quest Items & Heaps
    static constexpr int BONES              = 0;
    static constexpr int CHEST              = 11;
    static constexpr int LOCKED_CHEST       = 12;
    static constexpr int TOMB               = 13;
    static constexpr int CRYSTAL_CHEST      = 105;
    static constexpr int HIDDEN             = 119;
    static constexpr int DEWDROP            = 81;
    static constexpr int TORCH              = 84;
    static constexpr int PASTY              = 112;
    static constexpr int MEAT               = 113;
    static constexpr int STEAK              = 114;
    static constexpr int OVERPRICED         = 115;
    static constexpr int CARPACCIO          = 116;
    static constexpr int VIAL               = 120;
    static constexpr int BOMB               = 124;
    static constexpr int HONEYPOT           = 125;
    static constexpr int MASTERY            = 82;
};

} // namespace pd

