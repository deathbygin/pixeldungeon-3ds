#include "pd/windows/WndStory.h"
#include "pd/ui/BitmapFont.h"
#include "pd/ui/Chrome.h"
#include "noosa/Audio.h"
#include "pd/Assets.h"
#include <sstream>
#include <algorithm>
#include <cmath>

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

void WndStory::wrapText(const std::string& text, float maxW) {
    m_lines.clear();
    std::stringstream textStream(text);
    std::string paragraph;
    while (std::getline(textStream, paragraph)) {
        if (paragraph.empty()) {
            m_lines.push_back("");
            continue;
        }
        std::stringstream ss(paragraph);
        std::string word;
        std::string currentLine;
        while (ss >> word) {
            float wordW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, word);
            float spaceW = 4.0f;
            float lineW = currentLine.empty() ? 0.0f : (pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, currentLine) + spaceW);
            if (!currentLine.empty() && (lineW + wordW) > maxW) {
                m_lines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            m_lines.push_back(currentLine);
        }
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

    m_w = 260.0f;
    m_x = (320.0f - m_w) * 0.5f;

    float maxTextW = m_w - 36.0f;
    wrapText(getChapterText(chapterId), maxTextW);

    float textH = m_lines.size() * 10.0f;
    m_h = 16.0f + 14.0f + textH + 26.0f;
    m_h = std::clamp(m_h, 130.0f, 216.0f);
    m_y = std::round((240.0f - m_h) * 0.5f);
}

bool WndStory::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Tap anywhere or press A / B / X / Y / START / SELECT to dismiss
    if (touch.justReleased() || (kDown & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_START | KEY_SELECT))) {
        noosa::Audio::play(noosa::Audio::getSample(Assets::SND_CLICK).get());
        hide();
        return true;
    }

    return true; // Absorb input
}

void WndStory::draw(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open) return;

    // 1. Dim background
    C2D_DrawRectSolid(0.0f, 0.0f, 0.84f, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xBB));

    // 2. Authentic Parchment Scroll 9-Patch Chrome from chrome.png
    pd::ui::Chrome::get(pd::ui::Chrome::Type::SCROLL).draw(m_x, m_y, m_w, m_h, 0.86f);

    // 3. Chapter Title (Dark sepia ink in authentic BitmapFont)
    float tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, m_chapterTitle);
    float titleX = std::round(m_x + (m_w - tW) * 0.5f);
    float titleY = m_y + 16.0f;
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, m_chapterTitle, titleX, titleY, 0.88f, C2D_Color32(0x4a, 0x2e, 0x16, 0xFF));

    // 4. Parchment Story Text (Deep dark ink matching source Pixel Dungeon)
    float lineY = titleY + 14.0f;
    for (const auto& line : m_lines) {
        if (!line.empty()) {
            pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, line, m_x + 18.0f, lineY, 0.88f, C2D_Color32(0x28, 0x1e, 0x12, 0xFF));
        }
        lineY += 10.0f;
    }

    // 5. "Tap to continue" prompt (Faded ink in authentic BitmapFont)
    std::string promptStr = "-- Tap or press [A] to continue --";
    float pW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, promptStr);
    float promptX = std::round(m_x + (m_w - pW) * 0.5f);
    float promptY = m_y + m_h - 18.0f;
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, promptStr, promptX, promptY, 0.88f, C2D_Color32(0x7a, 0x62, 0x48, 0xFF));
}

} // namespace pd::windows
