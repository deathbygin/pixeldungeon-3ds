#include "pd/windows/WndStory.h"

namespace pd::windows {

const char* WndStory::getChapterText(int chapterId) {
    switch (chapterId) {
        case ID_SEWERS:
            return "The Dungeon lies right beneath the City, its upper levels actually constitute the City's sewer system. "
                   "Being nominally a part of the City, these levels are not that dangerous. No one will call it a safe place, "
                   "but at least you won't need to deal with evil magic here.";
        case ID_PRISON:
            return "Many years ago an underground prison was built here for the most dangerous criminals. At the time it seemed "
                   "like a very clever idea, because this place indeed was very hard to escape. But soon dark miasma started to permeate "
                   "from below, driving prisoners and guards insane. In the end the prison was abandoned, though some convicts "
                   "were left locked up here.";
        case ID_CAVES:
            return "The caves, which stretch down under the abandoned prison, are sparsely populated. They lie too deep to be exploited "
                   "by the City and they are too poor in minerals to interest the dwarves. In the past there was a trade outpost "
                   "somewhere here on the route between these two states, but it has perished since the decline of Dwarven Metropolis. "
                   "Only omnipresent gnolls and subterranean animals dwell here now.";
        case ID_METROPOLIS:
            return "Dwarven Metropolis was once the greatest of dwarven city-states. In its heyday the mechanized army of dwarves "
                   "has successfully repelled the invasion of the old god and his demon army. But it is said, that the returning warriors "
                   "have brought seeds of corruption with them, and that victory was the beginning of the end for the underground kingdom.";
        case ID_HALLS:
            return "In the past these levels were the outskirts of Metropolis. After the costly victory in the war with the old god "
                   "dwarves were too weakened to clear them of remaining demons. Gradually demons have tightened their grip on this place "
                   "and now it's called Demon Halls.\n\n"
                   "Very few adventurers have ever descended this far...";
        default:
            return "";
    }
}

void WndStory::show(int chapterId) {
    m_chapterId = chapterId;
    m_open = true;

    switch (chapterId) {
        case ID_SEWERS:     m_chapterTitle = "CHAPTER 1: THE SEWERS"; break;
        case ID_PRISON:     m_chapterTitle = "CHAPTER 2: THE PRISON"; break;
        case ID_CAVES:      m_chapterTitle = "CHAPTER 3: THE CAVES"; break;
        case ID_METROPOLIS: m_chapterTitle = "CHAPTER 4: THE METROPOLIS"; break;
        case ID_HALLS:      m_chapterTitle = "CHAPTER 5: THE DEMON HALLS"; break;
        default:            m_chapterTitle = "STORY"; break;
    }

    // Wrap story lines cleanly for 3DS screen
    m_lines.clear();
    if (chapterId == ID_SEWERS) {
        m_lines = {
            "The Dungeon lies right beneath the City,",
            "its upper levels actually constitute the",
            "City's sewer system.",
            "",
            "Being nominally a part of the City,",
            "these levels are not that dangerous.",
            "No one will call it a safe place, but at",
            "least you won't need to deal with evil",
            "magic here."
        };
    } else {
        m_lines = {
            getChapterText(chapterId)
        };
    }

    m_w = 280.0f;
    m_h = 196.0f;
    m_x = (320.0f - m_w) * 0.5f;
    m_y = (240.0f - m_h) * 0.5f;
}

bool WndStory::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Tap anywhere or press A / B / START to dismiss
    if (touch.justReleased() || (kDown & (KEY_A | KEY_B | KEY_START))) {
        hide();
        return true;
    }

    return true; // Absorb input
}

void WndStory::draw(C2D_TextBuf textBuf) {
    if (!m_open) return;

    // 1. Dim background
    C2D_DrawRectSolid(0.0f, 0.0f, 0.85f, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xBB));

    // 2. Parchment Scroll Background (exact rgb: 0.77, 0.73, 0.62 -> #C4BA9E)
    u32 parchmentBg = C2D_Color32(0xc4, 0xba, 0x9e, 0xFF);
    u32 scrollBorder = C2D_Color32(0x8a, 0x76, 0x5a, 0xFF);
    u32 scrollInner = C2D_Color32(0xe2, 0xd8, 0xbe, 0xFF);

    C2D_DrawRectSolid(m_x, m_y, 0.86f, m_w, m_h, parchmentBg);
    // Outer border
    C2D_DrawRectSolid(m_x, m_y, 0.87f, m_w, 2.0f, scrollBorder);
    C2D_DrawRectSolid(m_x, m_y + m_h - 2.0f, 0.87f, m_w, 2.0f, scrollBorder);
    C2D_DrawRectSolid(m_x, m_y, 0.87f, 2.0f, m_h, scrollBorder);
    C2D_DrawRectSolid(m_x + m_w - 2.0f, m_y, 0.87f, 2.0f, m_h, scrollBorder);
    // Inner margin
    C2D_DrawRectSolid(m_x + 4.0f, m_y + 4.0f, 0.87f, m_w - 8.0f, 1.0f, scrollInner);
    C2D_DrawRectSolid(m_x + 4.0f, m_y + m_h - 5.0f, 0.87f, m_w - 8.0f, 1.0f, scrollInner);

    // 3. Chapter Title
    C2D_Text titleObj;
    C2D_TextParse(&titleObj, textBuf, m_chapterTitle.c_str());
    C2D_TextOptimize(&titleObj);
    float tW = 0.0f, tH = 0.0f;
    C2D_TextGetDimensions(&titleObj, 0.42f, 0.42f, &tW, &tH);
    float titleX = m_x + (m_w - tW) * 0.5f;
    float titleY = m_y + 12.0f;
    C2D_DrawText(&titleObj, C2D_WithColor, titleX, titleY, 0.88f, 0.42f, 0.42f, C2D_Color32(0x4a, 0x36, 0x22, 0xFF));

    // 4. Parchment Story Text (Dark brown ink color)
    float lineY = titleY + tH + 12.0f;
    for (const auto& line : m_lines) {
        if (!line.empty()) {
            C2D_Text lineObj;
            C2D_TextParse(&lineObj, textBuf, line.c_str());
            C2D_TextOptimize(&lineObj);
            C2D_DrawText(&lineObj, C2D_WithColor, m_x + 16.0f, lineY, 0.88f, 0.36f, 0.36f, C2D_Color32(0x2d, 0x24, 0x18, 0xFF));
        }
        lineY += 14.0f;
    }

    // 5. "Tap to continue" prompt
    C2D_Text tapObj;
    C2D_TextParse(&tapObj, textBuf, "-- Tap or press [A] to continue --");
    C2D_TextOptimize(&tapObj);
    float cW = 0.0f, cH = 0.0f;
    C2D_TextGetDimensions(&tapObj, 0.34f, 0.34f, &cW, &cH);
    C2D_DrawText(&tapObj, C2D_WithColor, m_x + (m_w - cW) * 0.5f, m_y + m_h - 20.0f, 0.88f, 0.34f, 0.34f, C2D_Color32(0x7a, 0x62, 0x48, 0xFF));
}

} // namespace pd::windows

