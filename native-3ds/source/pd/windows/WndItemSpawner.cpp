#include "pd/windows/WndItemSpawner.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/Dungeon.h"
#include "pd/Hero.h"
#include "pd/Level.h"
#include "pd/GLog.h"
#include "pd/FX.h"
#include "pd/ItemSpriteSheet.h"
#include <algorithm>
#include <cmath>

namespace pd::windows {

using namespace pd::ui;

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int fx, int fy, int fw, int fh) {
    sub.width  = static_cast<u16>(fw);
    sub.height = static_cast<u16>(fh);
    sub.left   = static_cast<float>(fx) / static_cast<float>(potW);
    sub.top    = 1.0f - static_cast<float>(fy) / static_cast<float>(potH);
    sub.right  = static_cast<float>(fx + fw) / static_cast<float>(potW);
    sub.bottom = 1.0f - static_cast<float>(fy + fh) / static_cast<float>(potH);
}

WndItemSpawner::WndItemSpawner() {
    m_texItems = noosa::Texture::get(Assets::ITEMS);
    initCategories();
}

void WndItemSpawner::initCategories() {
    m_categories.clear();

    // 1. Weapons & Missiles
    {
        SpawnCategory cat;
        cat.name = "Weapons & Missiles";
        cat.items = {
            { "Dagger", ItemSpriteSheet::DAGGER, []() { return std::make_unique<Weapon>("Dagger", 1, 6, 10); } },
            { "Knuckles", ItemSpriteSheet::KNUCKLEDUSTER, []() { return std::make_unique<Weapon>("Knuckles", 1, 5, 10); } },
            { "Shortsword", ItemSpriteSheet::SHORT_SWORD, []() { return std::make_unique<ShortSword>(); } },
            { "Spear", ItemSpriteSheet::SPEAR, []() { return std::make_unique<Weapon>("Spear", 2, 15, 12); } },
            { "Quarterstaff", ItemSpriteSheet::QUARTERSTAFF, []() { return std::make_unique<Weapon>("Quarterstaff", 2, 12, 12); } },
            { "Sword", ItemSpriteSheet::SWORD, []() { return std::make_unique<Weapon>("Sword", 3, 15, 14); } },
            { "Mace", ItemSpriteSheet::MACE, []() { return std::make_unique<Weapon>("Mace", 3, 18, 14); } },
            { "Longsword", ItemSpriteSheet::LONG_SWORD, []() { return std::make_unique<Weapon>("Longsword", 4, 20, 16); } },
            { "Battle Axe", ItemSpriteSheet::BATTLE_AXE, []() { return std::make_unique<Weapon>("Battle Axe", 4, 24, 16); } },
            { "War Hammer", ItemSpriteSheet::WAR_HAMMER, []() { return std::make_unique<Weapon>("War Hammer", 5, 30, 18); } },
            { "Glaive", ItemSpriteSheet::GLAIVE, []() { return std::make_unique<Weapon>("Glaive", 5, 35, 18); } },
            { "Dart x15", ItemSpriteSheet::DART, []() { return std::make_unique<Dart>(15); } },
            { "Incend Dart x5", ItemSpriteSheet::INCENDIARY_DART, []() { return std::make_unique<IncendiaryDart>(5); } },
            { "Curare Dart x5", ItemSpriteSheet::CURARE_DART, []() { return std::make_unique<CurareDart>(5); } },
            { "Javelin x5", ItemSpriteSheet::JAVELIN, []() { return std::make_unique<Javelin>(5); } },
            { "Shuriken x5", ItemSpriteSheet::SHURIKEN, []() { return std::make_unique<Shuriken>(5); } },
            { "Tomahawk x5", ItemSpriteSheet::TOMAHAWK, []() { return std::make_unique<Tamahawk>(5); } },
            { "Boomerang", ItemSpriteSheet::BOOMERANG, []() { return std::make_unique<Boomerang>(); } }
        };
        m_categories.push_back(std::move(cat));
    }

    // 2. Armor
    {
        SpawnCategory cat;
        cat.name = "Armor (Tiers 1-5)";
        cat.items = {
            { "Cloth Armor", ItemSpriteSheet::ARMOR_CLOTH, []() { return std::make_unique<Armor>("Cloth Armor", 1, 9); } },
            { "Leather Armor", ItemSpriteSheet::ARMOR_LEATHER, []() { return std::make_unique<Armor>("Leather Armor", 2, 11); } },
            { "Mail Armor", ItemSpriteSheet::ARMOR_MAIL, []() { return std::make_unique<Armor>("Mail Armor", 3, 13); } },
            { "Scale Armor", ItemSpriteSheet::ARMOR_SCALE, []() { return std::make_unique<Armor>("Scale Armor", 4, 15); } },
            { "Plate Armor", ItemSpriteSheet::ARMOR_PLATE, []() { return std::make_unique<Armor>("Plate Armor", 5, 17); } }
        };
        m_categories.push_back(std::move(cat));
    }

    // 3. Potions
    {
        SpawnCategory cat;
        cat.name = "Potions";
        cat.items = {
            { "Healing", Potion::imageForType(Potion::PotionType::HEALING), []() { return std::make_unique<Potion>(Potion::PotionType::HEALING); } },
            { "Strength", Potion::imageForType(Potion::PotionType::STRENGTH), []() { return std::make_unique<Potion>(Potion::PotionType::STRENGTH); } },
            { "Experience", Potion::imageForType(Potion::PotionType::EXPERIENCE), []() { return std::make_unique<Potion>(Potion::PotionType::EXPERIENCE); } },
            { "Liquid Flame", Potion::imageForType(Potion::PotionType::LIQUID_FLAME), []() { return std::make_unique<Potion>(Potion::PotionType::LIQUID_FLAME); } },
            { "Frost", Potion::imageForType(Potion::PotionType::FROST), []() { return std::make_unique<Potion>(Potion::PotionType::FROST); } },
            { "Toxic Gas", Potion::imageForType(Potion::PotionType::TOXIC_GAS), []() { return std::make_unique<Potion>(Potion::PotionType::TOXIC_GAS); } },
            { "Paralytic Gas", Potion::imageForType(Potion::PotionType::PARALYTIC_GAS), []() { return std::make_unique<Potion>(Potion::PotionType::PARALYTIC_GAS); } },
            { "Invisibility", Potion::imageForType(Potion::PotionType::INVISIBILITY), []() { return std::make_unique<Potion>(Potion::PotionType::INVISIBILITY); } },
            { "Levitation", Potion::imageForType(Potion::PotionType::LEVITATION), []() { return std::make_unique<Potion>(Potion::PotionType::LEVITATION); } },
            { "Mind Vision", Potion::imageForType(Potion::PotionType::MIND_VISION), []() { return std::make_unique<Potion>(Potion::PotionType::MIND_VISION); } },
            { "Purity", Potion::imageForType(Potion::PotionType::PURITY), []() { return std::make_unique<Potion>(Potion::PotionType::PURITY); } },
            { "Might", Potion::imageForType(Potion::PotionType::MIGHT), []() { return std::make_unique<Potion>(Potion::PotionType::MIGHT); } }
        };
        m_categories.push_back(std::move(cat));
    }

    // 4. Scrolls
    {
        SpawnCategory cat;
        cat.name = "Scrolls";
        cat.items = {
            { "Upgrade", ItemSpriteSheet::SCROLL_KAUNAN, []() { return std::make_unique<Scroll>(Scroll::ScrollType::UPGRADE); } },
            { "Identify", ItemSpriteSheet::SCROLL_SOWILO, []() { return std::make_unique<Scroll>(Scroll::ScrollType::IDENTIFY); } },
            { "Magic Mapping", ItemSpriteSheet::SCROLL_LAGUZ, []() { return std::make_unique<Scroll>(Scroll::ScrollType::MAGIC_MAPPING); } },
            { "Remove Curse", ItemSpriteSheet::SCROLL_YNGVI, []() { return std::make_unique<Scroll>(Scroll::ScrollType::REMOVE_CURSE); } },
            { "Recharging", ItemSpriteSheet::SCROLL_GYFU, []() { return std::make_unique<Scroll>(Scroll::ScrollType::RECHARGING); } },
            { "Teleportation", ItemSpriteSheet::SCROLL_RAIDO, []() { return std::make_unique<Scroll>(Scroll::ScrollType::TELEPORTATION); } },
            { "Challenge", ItemSpriteSheet::SCROLL_ISAZ, []() { return std::make_unique<Scroll>(Scroll::ScrollType::CHALLENGE); } },
            { "Terror", ItemSpriteSheet::SCROLL_MANNAZ, []() { return std::make_unique<Scroll>(Scroll::ScrollType::TERROR); } },
            { "Lullaby", ItemSpriteSheet::SCROLL_NAUDIZ, []() { return std::make_unique<Scroll>(Scroll::ScrollType::LULLABY); } },
            { "Psionic Blast", ItemSpriteSheet::SCROLL_BERKANAN, []() { return std::make_unique<Scroll>(Scroll::ScrollType::PSIONIC_BLAST); } },
            { "Mirror Image", ItemSpriteSheet::SCROLL_ODAL, []() { return std::make_unique<Scroll>(Scroll::ScrollType::MIRROR_IMAGE); } },
            { "Enchantment", ItemSpriteSheet::SCROLL_TIWAZ, []() { return std::make_unique<Scroll>(Scroll::ScrollType::ENCHANTMENT); } }
        };
        m_categories.push_back(std::move(cat));
    }

    // 5. Wands
    {
        SpawnCategory cat;
        cat.name = "Wands";
        cat.items = {
            { "Magic Missile", ItemSpriteSheet::WAND_MAGIC_MISSILE, []() { return std::make_unique<WandOfMagicMissile>(); } },
            { "Lightning", ItemSpriteSheet::WAND_HOLLY, []() { return std::make_unique<Wand>("Wand of Lightning", 4); } },
            { "Firebolt", ItemSpriteSheet::WAND_YEW, []() { return std::make_unique<Wand>("Wand of Firebolt", 4); } },
            { "Teleportation", ItemSpriteSheet::WAND_EBONY, []() { return std::make_unique<Wand>("Wand of Teleportation", 3); } },
            { "Slowness", ItemSpriteSheet::WAND_CHERRY, []() { return std::make_unique<Wand>("Wand of Slowness", 3); } },
            { "Poison", ItemSpriteSheet::WAND_TEAK, []() { return std::make_unique<Wand>("Wand of Poison", 3); } },
            { "Blink", ItemSpriteSheet::WAND_ROWAN, []() { return std::make_unique<Wand>("Wand of Blink", 4); } },
            { "Regrowth", ItemSpriteSheet::WAND_WILLOW, []() { return std::make_unique<Wand>("Wand of Regrowth", 3); } },
            { "Amok", ItemSpriteSheet::WAND_MAHOGANY, []() { return std::make_unique<Wand>("Wand of Amok", 3); } },
            { "Disintegrate", ItemSpriteSheet::WAND_BAMBOO, []() { return std::make_unique<Wand>("Wand of Disintegration", 4); } },
            { "Avalanche", ItemSpriteSheet::WAND_PURPLEHEART, []() { return std::make_unique<Wand>("Wand of Avalanche", 4); } }
        };
        m_categories.push_back(std::move(cat));
    }

    // 6. Rings
    {
        SpawnCategory cat;
        cat.name = "Rings";
        cat.items = {
            { "Shadows", ItemSpriteSheet::RING_OPAL, []() { return std::make_unique<Ring>(Ring::Gem::OPAL, "Shadows"); } },
            { "Haste", ItemSpriteSheet::RING_TOURMALINE, []() { return std::make_unique<Ring>(Ring::Gem::TOURMALINE, "Haste"); } },
            { "Detection", ItemSpriteSheet::RING_GARNET, []() { return std::make_unique<Ring>(Ring::Gem::GARNET, "Detection"); } },
            { "Accuracy", ItemSpriteSheet::RING_RUBY, []() { return std::make_unique<Ring>(Ring::Gem::RUBY, "Accuracy"); } },
            { "Evasion", ItemSpriteSheet::RING_AMETHYST, []() { return std::make_unique<Ring>(Ring::Gem::AMETHYST, "Evasion"); } },
            { "Elements", ItemSpriteSheet::RING_TOPAZ, []() { return std::make_unique<Ring>(Ring::Gem::TOPAZ, "Elements"); } },
            { "Mending", ItemSpriteSheet::RING_DIAMOND, []() { return std::make_unique<Ring>(Ring::Gem::DIAMOND, "Mending"); } },
            { "Haggler", ItemSpriteSheet::RING_ONYX, []() { return std::make_unique<Ring>(Ring::Gem::ONYX, "Haggler"); } },
            { "Herbalism", ItemSpriteSheet::RING_EMERALD, []() { return std::make_unique<Ring>(Ring::Gem::EMERALD, "Herbalism"); } },
            { "Power", ItemSpriteSheet::RING_SAPPHIRE, []() { return std::make_unique<Ring>(Ring::Gem::SAPPHIRE, "Power"); } },
            { "Satiety", ItemSpriteSheet::RING_QUARTZ, []() { return std::make_unique<Ring>(Ring::Gem::QUARTZ, "Satiety"); } },
            { "Thorns", ItemSpriteSheet::RING_AGATE, []() { return std::make_unique<Ring>(Ring::Gem::AGATE, "Thorns"); } }
        };
        m_categories.push_back(std::move(cat));
    }

    // 7. Food & Miscellaneous
    {
        SpawnCategory cat;
        cat.name = "Food & Miscellaneous";
        cat.items = {
            { "Ration", ItemSpriteSheet::RATION, []() { return std::make_unique<Food>(); } },
            { "Mystery Meat", ItemSpriteSheet::MEAT, []() { return std::make_unique<MysteryMeat>(); } },
            { "Dew Vial", ItemSpriteSheet::VIAL, []() { return std::make_unique<DewVial>(); } },
            { "Iron Key", ItemSpriteSheet::IRON_KEY, []() { return std::make_unique<IronKey>(Dungeon::depth); } },
            { "Golden Key", ItemSpriteSheet::GOLDEN_KEY, []() { return std::make_unique<GoldenKey>(Dungeon::depth); } },
            { "Skeleton Key", ItemSpriteSheet::SKELETON_KEY, []() { return std::make_unique<Key>(Dungeon::depth); } },
            { "Torch", ItemSpriteSheet::TORCH, []() { return std::make_unique<Torch>(); } },
            { "Bomb", ItemSpriteSheet::BOMB, []() { return std::make_unique<Bomb>(); } },
            { "Honeypot", ItemSpriteSheet::HONEYPOT, []() { return std::make_unique<Honeypot>(); } },
            { "Amulet Yendor", ItemSpriteSheet::ANKH, []() { return std::make_unique<Amulet>(); } },
            { "Seed Firebloom", ItemSpriteSheet::SEED_FIREBLOOM, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::FIREBLOOM); } },
            { "Seed Icecap", ItemSpriteSheet::SEED_ICECAP, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::ICECAP); } },
            { "Seed Sorrowmoss", ItemSpriteSheet::SEED_SORROWMOSS, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::SORROWMOSS); } },
            { "Seed Dreamweed", ItemSpriteSheet::SEED_DREAMWEED, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::DREAMWEED); } },
            { "Seed Sungrass", ItemSpriteSheet::SEED_SUNGRASS, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::SUNGRASS); } },
            { "Seed Earthroot", ItemSpriteSheet::SEED_EARTHROOT, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::EARTHROOT); } },
            { "Seed Fadeleaf", ItemSpriteSheet::SEED_FADELEAF, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::FADELEAF); } },
            { "Seed Rotberry", ItemSpriteSheet::SEED_ROTBERRY, []() { return std::make_unique<PlantSeed>(PlantSeed::PlantType::ROTBERRY); } }
        };
        m_categories.push_back(std::move(cat));
    }
}

void WndItemSpawner::show() {
    m_open = true;
    m_currentCategory = 0;
    m_currentPage = 0;
    m_selectedIndex = 0;
}

void WndItemSpawner::hide() {
    m_open = false;
}

void WndItemSpawner::spawnItem(const SpawnItemEntry& entry) {
    if (!Dungeon::hero || !entry.create) return;
    Hero* hero = Dungeon::hero.get();

    std::unique_ptr<Item> item = entry.create();
    if (!item) return;

    item->identify();
    std::string itemName = item->displayName();

    if (!hero->belongings.isFull()) {
        hero->belongings.collect(std::move(item));
        Sample::play(Assets::SND_ITEM);
    } else {
        if (item->isStackable()) {
            if (hero->belongings.collect(std::move(item))) {
                Sample::play(Assets::SND_ITEM);
                std::string msg = "[DEBUG] Spawned " + itemName;
                GLog::p(msg);
                FX::floatingText(hero->pos, "+SPAWN", 0x55ff55FF);
                return;
            }
            item = entry.create();
            item->identify();
        }
        if (Dungeon::level) {
            Dungeon::level->drop(std::move(item), hero->pos);
            Sample::play(Assets::SND_STEP);
        }
    }

    std::string msg = "[DEBUG] Spawned " + itemName;
    GLog::p(msg);
    FX::floatingText(hero->pos, "+SPAWN", 0x55ff55FF);
}

bool WndItemSpawner::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    if (m_categories.empty()) return true;

    const auto& currentCat = m_categories[m_currentCategory];
    int totalItems = static_cast<int>(currentCat.items.size());
    int totalPages = std::max(1, (totalItems + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);

    // Hardware B button: return
    if (kDown & KEY_B) {
        Sample::play(Assets::SND_CLICK);
        hide();
        return true;
    }

    // Hardware L / R shoulder buttons: switch category
    if (kDown & KEY_L) {
        if (m_currentCategory > 0) m_currentCategory--;
        else m_currentCategory = static_cast<int>(m_categories.size()) - 1;
        m_currentPage = 0;
        m_selectedIndex = 0;
        Sample::play(Assets::SND_CLICK);
        return true;
    }
    if (kDown & KEY_R) {
        if (m_currentCategory + 1 < static_cast<int>(m_categories.size())) m_currentCategory++;
        else m_currentCategory = 0;
        m_currentPage = 0;
        m_selectedIndex = 0;
        Sample::play(Assets::SND_CLICK);
        return true;
    }

    // Category navigation buttons touch hitboxes
    noosa::RectF prevCatRect(m_x + 10.0f, m_y + 28.0f, 26.0f, 20.0f);
    if (touch.clicked(prevCatRect)) {
        if (m_currentCategory > 0) m_currentCategory--;
        else m_currentCategory = static_cast<int>(m_categories.size()) - 1;
        m_currentPage = 0;
        m_selectedIndex = 0;
        Sample::play(Assets::SND_CLICK);
        return true;
    }

    noosa::RectF nextCatRect(m_x + m_w - 36.0f, m_y + 28.0f, 26.0f, 20.0f);
    if (touch.clicked(nextCatRect)) {
        if (m_currentCategory + 1 < static_cast<int>(m_categories.size())) m_currentCategory++;
        else m_currentCategory = 0;
        m_currentPage = 0;
        m_selectedIndex = 0;
        Sample::play(Assets::SND_CLICK);
        return true;
    }

    // Pagination buttons touch hitboxes
    noosa::RectF prevPageRect(m_x + 12.0f, m_y + 190.0f, 50.0f, 20.0f);
    if (m_currentPage > 0 && touch.clicked(prevPageRect)) {
        m_currentPage--;
        m_selectedIndex = 0;
        Sample::play(Assets::SND_CLICK);
        return true;
    }

    noosa::RectF nextPageRect(m_x + 140.0f, m_y + 190.0f, 50.0f, 20.0f);
    if (m_currentPage + 1 < totalPages && touch.clicked(nextPageRect)) {
        m_currentPage++;
        m_selectedIndex = 0;
        Sample::play(Assets::SND_CLICK);
        return true;
    }

    // Return button touch hitbox
    noosa::RectF returnRect(m_x + 208.0f, m_y + 190.0f, 70.0f, 20.0f);
    if (touch.clicked(returnRect)) {
        Sample::play(Assets::SND_CLICK);
        hide();
        return true;
    }

    // Items grid touch hitboxes
    int startIdx = m_currentPage * ITEMS_PER_PAGE;
    int endIdx = std::min(totalItems, startIdx + ITEMS_PER_PAGE);
    float colW = 130.0f;
    float rowH = 23.0f;
    float col1X = m_x + 12.0f;
    float col2X = m_x + 148.0f;
    float gridStartY = m_y + 54.0f;

    for (int i = startIdx; i < endIdx; ++i) {
        int pageOffset = i - startIdx;
        int col = pageOffset % 2;
        int row = pageOffset / 2;
        float btnX = (col == 0) ? col1X : col2X;
        float btnY = gridStartY + row * (rowH + 3.0f);

        noosa::RectF itemRect(btnX, btnY, colW, rowH);
        if (touch.clicked(itemRect)) {
            spawnItem(currentCat.items[i]);
            return true;
        }
    }

    // Dismiss if tapped outside
    noosa::RectF winRect(m_x, m_y, m_w, m_h);
    if (touch.justReleased() && !winRect.contains(static_cast<float>(touch.x()), static_cast<float>(touch.y()))) {
        Sample::play(Assets::SND_CLICK);
        hide();
        return true;
    }

    return true; // Consume input while open
}

void WndItemSpawner::draw(C2D_TextBuf textBuf, float baseDepth) {
    (void)textBuf;
    if (!m_open || m_categories.empty()) return;

    // 1. Dim background
    C2D_DrawRectSolid(0.0f, 0.0f, baseDepth, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xBB));

    // 2. 9-slice stone window chrome
    Chrome::get(Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, baseDepth + 0.02f);

    // 3. Title text
    std::string title = "ITEM SPAWNER";
    float titleW = BitmapFont::width(BitmapFont::FontType::Font1x, title);
    float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, title, titleX, m_y + 11.0f, baseDepth + 0.06f, 0xFFEBA4FF, 0x000000FF);

    // 4. Category selector bar
    const auto& cat = m_categories[m_currentCategory];
    const auto& btnPatch = Chrome::get(Chrome::Type::BUTTON);

    // "<" button
    btnPatch.draw(m_x + 10.0f, m_y + 28.0f, 26.0f, 20.0f, baseDepth + 0.04f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "<", m_x + 20.0f, m_y + 33.0f, baseDepth + 0.06f, 0xFFFFFFFF, 0x000000FF);

    // Category title
    float catW = BitmapFont::width(BitmapFont::FontType::Font1x, cat.name);
    float catX = std::round(m_x + (m_w - catW) * 0.5f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, cat.name, catX, m_y + 33.0f, baseDepth + 0.06f, 0x55FF55FF, 0x000000FF);

    // ">" button
    btnPatch.draw(m_x + m_w - 36.0f, m_y + 28.0f, 26.0f, 20.0f, baseDepth + 0.04f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, ">", m_x + m_w - 24.0f, m_y + 33.0f, baseDepth + 0.06f, 0xFFFFFFFF, 0x000000FF);

    // 5. Items Grid
    int totalItems = static_cast<int>(cat.items.size());
    int totalPages = std::max(1, (totalItems + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    int startIdx = m_currentPage * ITEMS_PER_PAGE;
    int endIdx = std::min(totalItems, startIdx + ITEMS_PER_PAGE);

    float colW = 130.0f;
    float rowH = 23.0f;
    float col1X = m_x + 12.0f;
    float col2X = m_x + 148.0f;
    float gridStartY = m_y + 54.0f;

    int potW = m_texItems ? m_texItems->potWidth() : 128;
    int potH = m_texItems ? m_texItems->potHeight() : 256;
    C3D_Tex* iTex = m_texItems ? m_texItems->getTex() : nullptr;

    for (int i = startIdx; i < endIdx; ++i) {
        int pageOffset = i - startIdx;
        int col = pageOffset % 2;
        int row = pageOffset / 2;
        float btnX = (col == 0) ? col1X : col2X;
        float btnY = gridStartY + row * (rowH + 3.0f);

        // Button background
        btnPatch.draw(btnX, btnY, colW, rowH, baseDepth + 0.04f);

        const auto& itemEntry = cat.items[i];

        // Draw item sprite icon
        if (iTex) {
            setupSubTex(m_subItemIcon, potW, potH, (itemEntry.image % 8) * 16, (itemEntry.image / 8) * 16, 16, 16);
            C2D_Image img = { iTex, &m_subItemIcon };
            C2D_DrawParams p;
            p.pos.x = btnX + 4.0f;
            p.pos.y = btnY + 3.5f;
            p.pos.w = 16.0f;
            p.pos.h = 16.0f;
            p.center.x = 0.0f;
            p.center.y = 0.0f;
            p.depth = baseDepth + 0.06f;
            p.angle = 0.0f;
            C2D_DrawImage(img, &p, nullptr);
        }

        // Draw item name text
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, itemEntry.name, btnX + 23.0f, btnY + 7.0f, baseDepth + 0.06f, 0xFFFFFFFF, 0x000000FF);
    }

    // 6. Pagination & Return row
    if (m_currentPage > 0) {
        btnPatch.draw(m_x + 12.0f, m_y + 190.0f, 50.0f, 20.0f, baseDepth + 0.04f);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "< Prev", m_x + 20.0f, m_y + 195.0f, baseDepth + 0.06f, 0xFFFFFFFF, 0x000000FF);
    }

    std::string pageStr = std::to_string(m_currentPage + 1) + "/" + std::to_string(totalPages);
    float pW = BitmapFont::width(BitmapFont::FontType::Font1x, pageStr);
    float pX = std::round(m_x + 70.0f + (60.0f - pW) * 0.5f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, pageStr, pX, m_y + 195.0f, baseDepth + 0.06f, 0xCACFC2FF, 0x000000FF);

    if (m_currentPage + 1 < totalPages) {
        btnPatch.draw(m_x + 140.0f, m_y + 190.0f, 50.0f, 20.0f, baseDepth + 0.04f);
        BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Next >", m_x + 148.0f, m_y + 195.0f, baseDepth + 0.06f, 0xFFFFFFFF, 0x000000FF);
    }

    // Return button
    btnPatch.draw(m_x + 208.0f, m_y + 190.0f, 70.0f, 20.0f, baseDepth + 0.04f);
    BitmapFont::drawWithShadow(BitmapFont::FontType::Font1x, "Return", m_x + 225.0f, m_y + 195.0f, baseDepth + 0.06f, 0xFFEBA4FF, 0x000000FF);
}

} // namespace pd::windows

