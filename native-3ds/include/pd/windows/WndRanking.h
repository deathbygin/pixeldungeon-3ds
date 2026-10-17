#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include <memory>
#include "pd/Rankings.h"
#include "pd/ui/TouchController.h"
#include "noosa/Texture.h"

namespace pd::windows {

class WndRanking {
public:
    enum class Tab {
        STATS,
        ITEMS
    };

    WndRanking();
    ~WndRanking() = default;

    void show(const pd::Rankings::Record& record);
    void hide() { m_open = false; }
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

private:
    bool m_open = false;
    Tab m_tab = Tab::STATS;
    pd::Rankings::Record m_record;

    float m_x = 40.0f;
    float m_y = 20.0f;
    float m_w = 240.0f;
    float m_h = 200.0f;

    std::shared_ptr<noosa::Texture> m_texItems;
    std::shared_ptr<noosa::Texture> m_texIcons;
    std::shared_ptr<noosa::Texture> m_texHero;

    void drawStatsTab();
    void drawItemsTab();
    void drawItemRow(float rx, float ry, int imageIdx, const std::string& name, const std::string& slotType);
};

} // namespace pd::windows

