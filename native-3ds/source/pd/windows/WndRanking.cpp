#include "pd/windows/WndRanking.h"
#include "pd/Assets.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Sample.h"
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace pd::windows {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int x, int y, int w, int h) {
    sub.width = static_cast<u16>(w);
    sub.height = static_cast<u16>(h);
    sub.left = static_cast<float>(x) / potW;
    sub.right = static_cast<float>(x + w) / potW;
    sub.top = 1.0f - static_cast<float>(y) / potH;
    sub.bottom = 1.0f - static_cast<float>(y + h) / potH;
}

WndRanking::WndRanking() {
    m_texItems = noosa::Texture::get(Assets::ITEMS);
    m_texIcons = noosa::Texture::get(Assets::ICONS);
}

void WndRanking::show(const pd::Rankings::Record& record) {
    m_record = record;
    m_open = true;
    m_tab = Tab::STATS;

    std::string heroSheet = Assets::WARRIOR;
    if (m_record.heroClass == Hero::HeroClass::Mage) heroSheet = Assets::MAGE;
    else if (m_record.heroClass == Hero::HeroClass::Rogue) heroSheet = Assets::ROGUE;
    else if (m_record.heroClass == Hero::HeroClass::Huntress) heroSheet = Assets::HUNTRESS;
    m_texHero = noosa::Texture::get(heroSheet);
}

bool WndRanking::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Toggle/Close via B or START
    if (kDown & (KEY_B | KEY_START)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return true;
    }

    // Tab switching via D-Pad Left/Right or L/R shoulders
    if (kDown & (KEY_DLEFT | KEY_L)) {
        if (m_tab != Tab::STATS) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_tab = Tab::STATS;
        }
    }
    if (kDown & (KEY_DRIGHT | KEY_R)) {
        if (m_tab != Tab::ITEMS) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_tab = Tab::ITEMS;
        }
    }

    // Touch handling for tabs along top edge
    float tabY = m_y - 14.0f;
    float tabW = 60.0f;
    float tabH = 15.0f;
    float tab1X = m_x + 14.0f;
    float tab2X = tab1X + tabW + 6.0f;

    if (touch.clicked(noosa::RectF(tab1X, tabY, tabW, tabH))) {
        if (m_tab != Tab::STATS) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_tab = Tab::STATS;
        }
    }
    if (touch.clicked(noosa::RectF(tab2X, tabY, tabW, tabH))) {
        if (m_tab != Tab::ITEMS) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_tab = Tab::ITEMS;
        }
    }

    // Close button at bottom
    float btnW = 80.0f;
    float btnH = 20.0f;
    float btnX = std::round(m_x + (m_w - btnW) * 0.5f);
    float btnY = m_y + m_h - 24.0f;
    if (touch.clicked(noosa::RectF(btnX, btnY, btnW, btnH)) || (kDown & KEY_A)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return true;
    }

    return true;
}

static void drawStoneTab(float x, float y, float w, float h, float z, bool selected, const char* label) {
    u32 cBorderHi = selected ? C2D_Color32(0x9F, 0xA8, 0x96, 0xFF) : C2D_Color32(0x7A, 0x81, 0x73, 0xFF);
    u32 cBorderSh = selected ? C2D_Color32(0x62, 0x66, 0x5B, 0xFF) : C2D_Color32(0x35, 0x38, 0x31, 0xFF);
    u32 cBevelHi  = selected ? C2D_Color32(0x7A, 0x81, 0x73, 0xFF) : C2D_Color32(0x55, 0x5A, 0x50, 0xFF);
    u32 cBevelSh  = selected ? C2D_Color32(0x35, 0x38, 0x31, 0xFF) : C2D_Color32(0x23, 0x26, 0x21, 0xFF);
    u32 cFill      = selected ? C2D_Color32(0x41, 0x46, 0x3E, 0xFF) : C2D_Color32(0x28, 0x2B, 0x26, 0xFF);

    C2D_DrawRectSolid(x + 1.0f, y + 1.0f, z, w - 2.0f, h - 2.0f, cFill);
    C2D_DrawRectSolid(x + 2.0f, y + 2.0f, z + 0.01f, w - 4.0f, 1.0f, cBevelHi);
    C2D_DrawRectSolid(x + 2.0f, y + 2.0f, z + 0.01f, 1.0f, h - 4.0f, cBevelHi);
    C2D_DrawRectSolid(x + 2.0f, y + h - 3.0f, z + 0.01f, w - 4.0f, 1.0f, cBevelSh);
    C2D_DrawRectSolid(x + w - 3.0f, y + 2.0f, z + 0.01f, 1.0f, h - 4.0f, cBevelSh);

    C2D_DrawRectSolid(x + 1.0f, y, z + 0.02f, w - 2.0f, 1.0f, cBorderHi);
    C2D_DrawRectSolid(x, y + 1.0f, z + 0.02f, 1.0f, h - 2.0f, cBorderHi);
    C2D_DrawRectSolid(x + w - 1.0f, y + 1.0f, z + 0.02f, 1.0f, h - 2.0f, cBorderSh);

    float tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, label);
    float tX = std::round(x + (w - tW) * 0.5f);
    float tY = std::round(y + (h - 8.0f) * 0.5f);
    u32 textCol = selected ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_MUTED;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, label, tX, tY, z + 0.03f, textCol, pd::ui::COLOR_TEXT_SHADOW);
}

void WndRanking::draw(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open) return;

    float zDim = 0.85f;
    float zWin = 0.87f;
    float zTab = 0.89f;

    // 1. Darken background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, zDim, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xB0));

    // 2. Tabs along top edge
    float tabY = m_y - 14.0f;
    float tabW = 60.0f;
    float tabH = 15.0f;
    float tab1X = m_x + 14.0f;
    float tab2X = tab1X + tabW + 6.0f;

    drawStoneTab(tab1X, tabY, tabW, tabH, zTab, m_tab == Tab::STATS, "Stats");
    drawStoneTab(tab2X, tabY, tabW, tabH, zTab, m_tab == Tab::ITEMS, "Items");

    // 3. 9-slice stone window chrome from chrome.png
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, zWin);

    // 4. Tab contents
    if (m_tab == Tab::STATS) {
        drawStatsTab();
    } else {
        drawItemsTab();
    }

    // 5. Close button at bottom
    float btnW = 80.0f;
    float btnH = 20.0f;
    float btnX = std::round(m_x + (m_w - btnW) * 0.5f);
    float btnY = m_y + m_h - 24.0f;
    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);
    btnPatch.draw(btnX, btnY, btnW, btnH, 0.90f, 0xFFFFFFFF, 1.0f);

    float closeW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Close");
    float closeX = std::round(btnX + (btnW - closeW) * 0.5f);
    float closeY = std::round(btnY + (btnH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Close", closeX, closeY, 0.92f, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
}

void WndRanking::drawStatsTab() {
    float zContent = 0.90f;
    float zText = 0.92f;

    // Hero Avatar (16x16)
    float curY = m_y + 12.0f;
    if (m_texHero && m_texHero->isLoaded()) {
        Tex3DS_SubTexture subHero;
        setupSubTex(subHero, m_texHero->potWidth(), m_texHero->potHeight(), 0, 0, 12, 15);
        C2D_Image imgHero = { m_texHero->getTex(), &subHero };
        C2D_DrawParams pHero;
        pHero.pos.x = m_x + 14.0f; pHero.pos.y = curY;
        pHero.pos.w = 16.0f; pHero.pos.h = 16.0f;
        pHero.center.x = pHero.center.y = 0.0f; pHero.depth = zContent; pHero.angle = 0.0f;
        C2D_DrawImage(imgHero, &pHero, nullptr);
    }

    // Title: "LEVEL X CLASS"
    const char* classNames[] = { "WARRIOR", "MAGE", "ROGUE", "HUNTRESS" };
    int cIdx = static_cast<int>(m_record.heroClass);
    std::string clName = (cIdx >= 0 && cIdx < 4) ? classNames[cIdx] : "HERO";
    std::string titleStr = "LEVEL " + std::to_string(m_record.level) + " " + clName;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, titleStr, m_x + 36.0f, curY + 4.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    curY += 20.0f;

    // Cause of death / Outcome
    u32 outcomeCol = m_record.win ? pd::ui::COLOR_TEXT_POSITIVE : pd::ui::COLOR_TEXT_MUTED;
    float maxOutcomeW = m_w - 28.0f;
    auto outcomeLines = pd::ui::BitmapFont::splitMultiline(pd::ui::BitmapFont::FontType::Font1x, m_record.info, maxOutcomeW);
    for (auto& line : outcomeLines) {
        if (pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, line) > maxOutcomeW) {
            while (!line.empty() && pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, line + "...") > maxOutcomeW) {
                line.pop_back();
            }
            line += "...";
        }
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, line, m_x + 14.0f, curY, zText, outcomeCol, pd::ui::COLOR_TEXT_SHADOW);
        curY += 10.0f;
    }
    curY += 4.0f;

    // Divider line
    C2D_DrawRectSolid(m_x + 12.0f, curY, zContent, m_w - 24.0f, 1.0f, C2D_Color32(0x4A, 0x4D, 0x44, 0xFF));
    curY += 6.0f;

    // Stat slots: Label on left, Value on right
    int durationSec = static_cast<int>(m_record.duration);
    int m = durationSec / 60;
    int s = durationSec % 60;
    char durBuf[32];
    std::snprintf(durBuf, sizeof(durBuf), "%dm %02ds", m, s);

    struct StatEntry {
        std::string label;
        std::string value;
        u32 valColor;
    };

    std::vector<StatEntry> stats = {
        { "Strength:",       std::to_string(m_record.str), pd::ui::COLOR_TEXT_PRIMARY },
        { "Health:",         std::to_string(m_record.maxHp), pd::ui::COLOR_TEXT_PRIMARY },
        { "Game Duration:",  durBuf, pd::ui::COLOR_TEXT_PRIMARY },
        { "Maximum Depth:",  std::to_string(m_record.depth), pd::ui::COLOR_TEXT_PRIMARY },
        { "Mobs Killed:",    std::to_string(m_record.enemiesSlain), pd::ui::COLOR_TEXT_PRIMARY },
        { "Gold Collected:", std::to_string(m_record.gold), pd::ui::COLOR_TEXT_ACCENT },
        { "Food Eaten:",     std::to_string(m_record.foodEaten), pd::ui::COLOR_TEXT_PRIMARY },
        { "Potions Cooked:", std::to_string(m_record.potionsCooked), pd::ui::COLOR_TEXT_PRIMARY }
    };

    float leftX = m_x + 14.0f;
    float rightX = m_x + m_w - 14.0f;

    for (const auto& entry : stats) {
        pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, entry.label, leftX, curY, zText, pd::ui::COLOR_TEXT_MUTED);
        float vW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, entry.value);
        pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, entry.value, rightX - vW, curY, zText, entry.valColor);
        curY += 10.0f;
    }
}

void WndRanking::drawItemRow(float rx, float ry, int imageIdx, const std::string& name, const std::string& slotType) {
    float zContent = 0.90f;
    float zText = 0.92f;

    // Slot box (22x22)
    C2D_DrawRectSolid(rx, ry, zContent, 22.0f, 22.0f, C2D_Color32(0x35, 0x38, 0x31, 0xFF));
    C2D_DrawRectSolid(rx + 1.0f, ry + 1.0f, zContent + 0.005f, 20.0f, 20.0f, C2D_Color32(0x24, 0x27, 0x21, 0xFF));

    // Item sprite
    if (imageIdx >= 0 && m_texItems && m_texItems->isLoaded()) {
        Tex3DS_SubTexture sub;
        int potW = m_texItems->potWidth();
        int potH = m_texItems->potHeight();
        setupSubTex(sub, potW, potH, (imageIdx % 8) * 16, (imageIdx / 8) * 16, 16, 16);
        C2D_Image img = { m_texItems->getTex(), &sub };
        C2D_DrawParams p;
        p.pos.x = rx + 3.0f; p.pos.y = ry + 3.0f;
        p.pos.w = 16.0f; p.pos.h = 16.0f;
        p.center.x = p.center.y = 0.0f; p.depth = zContent + 0.01f; p.angle = 0.0f;
        C2D_DrawImage(img, &p, nullptr);
    }

    // Slot label & item name
    float textX = rx + 26.0f;
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, slotType, textX, ry + 1.0f, zText, pd::ui::COLOR_TEXT_MUTED);

    std::string displayName = name.empty() ? "(none)" : name;
    u32 nameColor = name.empty() ? pd::ui::COLOR_TEXT_MUTED : pd::ui::COLOR_TEXT_PRIMARY;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, displayName, textX, ry + 11.0f, zText, nameColor, pd::ui::COLOR_TEXT_SHADOW);
}

void WndRanking::drawItemsTab() {
    float startY = m_y + 12.0f;
    float rowH = 25.0f;
    float rx = m_x + 14.0f;

    drawItemRow(rx, startY, m_record.weaponImage, m_record.weaponName, "Weapon");
    drawItemRow(rx, startY + rowH, m_record.armorImage, m_record.armorName, "Armor");
    drawItemRow(rx, startY + rowH * 2, m_record.ring1Image, m_record.ring1Name, "Ring 1");
    drawItemRow(rx, startY + rowH * 3, m_record.ring2Image, m_record.ring2Name, "Ring 2");
    drawItemRow(rx, startY + rowH * 4, m_record.quickslotImage, m_record.quickslotName, "Quickslot");
}

} // namespace pd::windows

