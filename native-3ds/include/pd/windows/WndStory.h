#pragma once

#include <citro2d.h>
#include <string>
#include <vector>
#include "pd/ui/TouchController.h"

namespace pd::windows {

/**
 * Story parchment scroll overlay matching com.watabou.pixeldungeon.windows.WndStory.
 * Displays introductory lore chapter, tap to continue, with authentic parchment coloring.
 */
class WndStory {
public:
    static constexpr int ID_SEWERS     = 0;
    static constexpr int ID_PRISON     = 1;
    static constexpr int ID_CAVES      = 2;
    static constexpr int ID_METROPOLIS = 3;
    static constexpr int ID_HALLS      = 4;

    WndStory() = default;
    ~WndStory() = default;

    void show(int chapterId);
    void hide() { m_open = false; }
    bool isOpen() const { return m_open; }

    bool update(const pd::ui::TouchController& touch, u32 kDown);
    void draw(C2D_TextBuf textBuf);

    static const char* getChapterText(int chapterId);

private:
    void wrapText(const std::string& text, float maxW);

    bool m_open = false;
    int m_chapterId = 0;
    std::string m_chapterTitle;
    std::vector<std::string> m_lines;

    float m_x = 24.0f;
    float m_y = 24.0f;
    float m_w = 272.0f;
    float m_h = 192.0f;
};

} // namespace pd::windows

