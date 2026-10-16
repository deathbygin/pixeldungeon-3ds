#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "pd/ui/TouchController.h"
#include "pd/Item.h"
#include "noosa/Texture.h"

namespace pd::windows {

struct SpawnItemEntry {
    std::string name;
    int image = 0;
    std::function<std::unique_ptr<pd::Item>()> create;
};

struct SpawnCategory {
    std::string name;
    std::vector<SpawnItemEntry> items;
};

class WndItemSpawner {
public:
    WndItemSpawner();
    ~WndItemSpawner() = default;

    void show();
    void hide();
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf, float baseDepth = 0.85f);

private:
    void initCategories();
    void spawnItem(const SpawnItemEntry& entry);

    bool m_open = false;
    int m_currentCategory = 0;
    int m_currentPage = 0;
    int m_selectedIndex = 0;

    std::vector<SpawnCategory> m_categories;

    // Window layout
    float m_x = 15.0f;
    float m_y = 12.0f;
    float m_w = 290.0f;
    float m_h = 216.0f;

    std::shared_ptr<noosa::Texture> m_texItems;
    mutable Tex3DS_SubTexture m_subItemIcon{};

    static constexpr int ITEMS_PER_PAGE = 10;
};

} // namespace pd::windows

