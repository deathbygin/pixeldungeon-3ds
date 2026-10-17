#include "pd/scenes/BadgesScene.h"
#include "pd/BadgeBanner.h"
#include "pd/Assets.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Sample.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace pd::scenes {

static Badges::Badge resolveTier(const std::vector<Badges::Badge>& tiers, bool& outUnlocked) {
    for (int i = static_cast<int>(tiers.size()) - 1; i >= 0; --i) {
        if (Badges::isUnlocked(tiers[i])) {
            outUnlocked = true;
            return tiers[i];
        }
    }
    outUnlocked = false;
    return tiers[0];
}

BadgesScene::BadgesScene() {
    refresh();
}

void BadgesScene::refresh() {
    Badges::loadGlobal();
    m_items.clear();

    const auto& catalog = Badges::getCatalog();
    for (const auto& entry : catalog) {
        GridItem item;
        Badges::Badge b = entry.badge;
        bool unlocked = false;

        switch (b) {
            case Badges::Badge::MONSTERS_SLAIN_1:
                b = resolveTier({ Badges::Badge::MONSTERS_SLAIN_1, Badges::Badge::MONSTERS_SLAIN_2, Badges::Badge::MONSTERS_SLAIN_3, Badges::Badge::MONSTERS_SLAIN_4 }, unlocked);
                break;
            case Badges::Badge::GOLD_COLLECTED_1:
                b = resolveTier({ Badges::Badge::GOLD_COLLECTED_1, Badges::Badge::GOLD_COLLECTED_2, Badges::Badge::GOLD_COLLECTED_3, Badges::Badge::GOLD_COLLECTED_4 }, unlocked);
                break;
            case Badges::Badge::LEVEL_REACHED_1:
                b = resolveTier({ Badges::Badge::LEVEL_REACHED_1, Badges::Badge::LEVEL_REACHED_2, Badges::Badge::LEVEL_REACHED_3, Badges::Badge::LEVEL_REACHED_4 }, unlocked);
                break;
            case Badges::Badge::STRENGTH_ATTAINED_1:
                b = resolveTier({ Badges::Badge::STRENGTH_ATTAINED_1, Badges::Badge::STRENGTH_ATTAINED_2, Badges::Badge::STRENGTH_ATTAINED_3, Badges::Badge::STRENGTH_ATTAINED_4 }, unlocked);
                break;
            case Badges::Badge::FOOD_EATEN_1:
                b = resolveTier({ Badges::Badge::FOOD_EATEN_1, Badges::Badge::FOOD_EATEN_2, Badges::Badge::FOOD_EATEN_3, Badges::Badge::FOOD_EATEN_4 }, unlocked);
                break;
            case Badges::Badge::ITEM_LEVEL_1:
                b = resolveTier({ Badges::Badge::ITEM_LEVEL_1, Badges::Badge::ITEM_LEVEL_2, Badges::Badge::ITEM_LEVEL_3, Badges::Badge::ITEM_LEVEL_4 }, unlocked);
                break;
            case Badges::Badge::POTIONS_COOKED_1:
                b = resolveTier({ Badges::Badge::POTIONS_COOKED_1, Badges::Badge::POTIONS_COOKED_2, Badges::Badge::POTIONS_COOKED_3, Badges::Badge::POTIONS_COOKED_4 }, unlocked);
                break;
            case Badges::Badge::GAMES_PLAYED_1:
                b = resolveTier({ Badges::Badge::GAMES_PLAYED_1, Badges::Badge::GAMES_PLAYED_2, Badges::Badge::GAMES_PLAYED_3, Badges::Badge::GAMES_PLAYED_4 }, unlocked);
                break;
            default:
                unlocked = Badges::isUnlocked(b);
                break;
        }

        const auto& info = Badges::getInfo(b);
        item.badge = b;
        item.unlocked = unlocked;
        item.image = info.image;
        item.title = info.title;
        item.description = info.description;
        m_items.push_back(item);
    }

    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_items.size())) {
        m_selectedIndex = 0;
    }

    int totalRows = (static_cast<int>(m_items.size()) + COLS - 1) / COLS;
    float totalContentH = totalRows * (m_cellH + m_spacingY);
    float viewH = m_winH - 16.0f;
    m_maxScroll = std::max(0.0f, totalContentH - viewH);
    m_scrollY = 0.0f;
}

void BadgesScene::ensureVisible(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    int row = index / COLS;
    float rowTop = row * (m_cellH + m_spacingY);
    float rowBottom = rowTop + m_cellH;
    float viewH = m_winH - 16.0f;

    if (rowTop < m_scrollY) {
        m_scrollY = rowTop;
    } else if (rowBottom > m_scrollY + viewH) {
        m_scrollY = rowBottom - viewH;
    }
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScroll);
}

void BadgesScene::wrapText(const std::string& text, float maxW, std::vector<std::string>& outLines) {
    outLines.clear();
    std::stringstream textStream(text);
    std::string paragraph;
    while (std::getline(textStream, paragraph)) {
        if (paragraph.empty()) {
            outLines.push_back("");
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
                outLines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) {
                    currentLine += " ";
                }
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            outLines.push_back(currentLine);
        }
    }
}

void BadgesScene::update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch) {
    (void)kHeld;
    m_archs.update(dt);

    if (m_wndBadge.isOpen()) {
        m_wndBadge.update(touch, kDown);
        return;
    }

    int count = static_cast<int>(m_items.size());

    // Hardware buttons
    if (kDown & KEY_B) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_pendingAction = BadgesAction::BACK_TO_TITLE;
        return;
    }

    if ((kDown & KEY_A) && count > 0 && m_selectedIndex >= 0 && m_selectedIndex < count) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_wndBadge.show(m_items[m_selectedIndex].badge, m_items[m_selectedIndex].unlocked);
        return;
    }

    // D-Pad / Circle Pad navigation
    if (count > 0) {
        if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
            if (m_selectedIndex - COLS >= 0) {
                m_selectedIndex -= COLS;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
                ensureVisible(m_selectedIndex);
            }
        }
        if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
            if (m_selectedIndex + COLS < count) {
                m_selectedIndex += COLS;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
                ensureVisible(m_selectedIndex);
            }
        }
        if (kDown & (KEY_DLEFT | KEY_CPAD_LEFT)) {
            if (m_selectedIndex > 0) {
                m_selectedIndex--;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
                ensureVisible(m_selectedIndex);
            }
        }
        if (kDown & (KEY_DRIGHT | KEY_CPAD_RIGHT)) {
            if (m_selectedIndex + 1 < count) {
                m_selectedIndex++;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
                ensureVisible(m_selectedIndex);
            }
        }
    }

    // Touch handling
    noosa::RectF rBack(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH);
    if (touch.clicked(rBack)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_pendingAction = BadgesAction::BACK_TO_TITLE;
        return;
    }

    noosa::RectF rInspect(m_btnInspectX, m_btnInspectY, m_btnInspectW, m_btnInspectH);
    if (touch.clicked(rInspect) && count > 0 && m_selectedIndex >= 0 && m_selectedIndex < count) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_wndBadge.show(m_items[m_selectedIndex].badge, m_items[m_selectedIndex].unlocked);
        return;
    }

    // Grid scrolling drag
    noosa::RectF rGrid(m_winX + 4.0f, m_winY + 4.0f, m_winW - 8.0f, m_winH - 8.0f);
    float tx = static_cast<float>(touch.x());
    float ty = static_cast<float>(touch.y());

    if (touch.justPressed() && rGrid.contains(tx, ty)) {
        m_isDragging = true;
        m_dragStartY = ty;
        m_dragStartScrollY = m_scrollY;
    }

    if (m_isDragging) {
        if (touch.isDown()) {
            float diff = m_dragStartY - ty;
            m_scrollY = std::clamp(m_dragStartScrollY + diff, 0.0f, m_maxScroll);
        } else {
            m_isDragging = false;
        }
    }

    // Grid cell tap
    if (touch.clicked(rGrid)) {
        float gridStartX = m_winX + 10.0f;
        float gridStartY = m_winY + 8.0f;
        float cellStepX = m_cellW + m_spacingX;
        float cellStepY = m_cellH + m_spacingY;

        int col = static_cast<int>((tx - gridStartX) / cellStepX);
        int row = static_cast<int>((ty + m_scrollY - gridStartY) / cellStepY);

        if (col >= 0 && col < COLS && row >= 0) {
            int idx = row * COLS + col;
            if (idx >= 0 && idx < count) {
                if (idx == m_selectedIndex) {
                    Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                    m_wndBadge.show(m_items[idx].badge, m_items[idx].unlocked);
                } else {
                    m_selectedIndex = idx;
                    Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
                    ensureVisible(m_selectedIndex);
                }
            }
        }
    }
}

void BadgesScene::drawTop(C2D_TextBuf textBuf) {
    (void)textBuf;
    m_archs.drawTop(0.05f);

    float zBanner = 0.20f;
    float zContent = 0.30f;
    float zText = 0.40f;

    // Header banner: "BADGES"
    float bannerW = 160.0f;
    float bannerH = 22.0f;
    float bannerX = (400.0f - bannerW) * 0.5f;
    float bannerY = 8.0f;

    pd::ui::Chrome::get(pd::ui::Chrome::Type::TOAST).draw(bannerX, bannerY, bannerW, bannerH, zBanner);
    float tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font2x, "BADGES");
    float tX = std::round((400.0f - tW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font2x, "BADGES", tX, bannerY + 2.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    if (m_items.empty() || m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_items.size())) {
        return;
    }

    const auto& curItem = m_items[m_selectedIndex];

    // Detail Showcase Card for Selected Badge
    float cardW = 368.0f;
    float cardH = 176.0f;
    float cardX = 16.0f;
    float cardY = 34.0f;
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(cardX, cardY, cardW, cardH, zContent);

    // Left Column: 64x64 Box with 48x48 Medal and Status Pill
    float boxX = cardX + 16.0f;
    float boxY = cardY + 24.0f;
    C2D_DrawRectSolid(boxX, boxY, zContent + 0.01f, 64.0f, 64.0f, C2D_Color32(0x35, 0x38, 0x31, 0xFF));
    C2D_DrawRectSolid(boxX + 2.0f, boxY + 2.0f, zContent + 0.02f, 60.0f, 60.0f, C2D_Color32(0x24, 0x27, 0x21, 0xFF));

    // 48x48 Medal (scale = 3.0f, 16x3=48) centered inside 64x64 box
    float medalX = boxX + 8.0f;
    float medalY = boxY + 8.0f;
    BadgeBanner::drawBadge(curItem.image, medalX, medalY, 3.0f, 1.0f, !curItem.unlocked, zContent + 0.03f);

    // Status under medal box
    std::string statusStr = curItem.unlocked ? "[UNLOCKED]" : "[LOCKED]";
    u32 statusCol = curItem.unlocked ? pd::ui::COLOR_TEXT_POSITIVE : pd::ui::COLOR_TEXT_MUTED;
    float statW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, statusStr);
    float statX = std::round(boxX + (64.0f - statW) * 0.5f);
    float statY = boxY + 70.0f;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, statusStr, statX, statY, zText, statusCol, pd::ui::COLOR_TEXT_SHADOW);

    // Right Column: Title and Multiline Description
    float textX = cardX + 96.0f;
    float titleY = cardY + 20.0f;

    // Title (Font2x accent)
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font2x, curItem.title, textX, titleY, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    // Divider Line
    float divY = titleY + 20.0f;
    C2D_DrawRectSolid(textX, divY, zContent + 0.01f, cardW - 112.0f, 1.0f, C2D_Color32(0x4A, 0x4D, 0x44, 0xFF));

    // Multiline Description
    std::vector<std::string> descLines;
    wrapText(curItem.description, cardW - 116.0f, descLines);
    float lineY = divY + 8.0f;
    for (const auto& line : descLines) {
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, line, textX, lineY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
        lineY += 11.0f;
    }

    // Bottom prompt hint on Top Screen
    std::string hint = "D-Pad: Navigate    (A): Inspect    (B): Return";
    float hW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, hint);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, hint, std::round((400.0f - hW) * 0.5f), 220.0f, zText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
}

void BadgesScene::drawBottom(C2D_TextBuf textBuf) {
    (void)textBuf;
    m_archs.drawBottom(0.05f);

    float zHeader = 0.25f;
    float zContent = 0.30f;
    float zText = 0.40f;

    // Header Mini-Tab: "Badges"
    float tabW = 80.0f;
    float tabH = 16.0f;
    float tabX = (320.0f - tabW) * 0.5f;
    float tabY = 8.0f;
    pd::ui::Chrome::get(pd::ui::Chrome::Type::TOAST).draw(tabX, tabY, tabW, tabH, zHeader);

    float tabTW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Badges");
    float tabTX = std::round((320.0f - tabTW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Badges", tabTX, tabY + 3.0f, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // Window Container for badges grid
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_winX, m_winY, m_winW, m_winH, zContent);

    float gridStartX = m_winX + 10.0f;
    float gridStartY = m_winY + 8.0f;
    float cellStepX = m_cellW + m_spacingX;
    float cellStepY = m_cellH + m_spacingY;

    float listClipY1 = m_winY + 6.0f;
    float listClipY2 = m_winY + m_winH - 6.0f;

    for (size_t i = 0; i < m_items.size(); ++i) {
        int col = static_cast<int>(i % COLS);
        int row = static_cast<int>(i / COLS);

        float cellX = gridStartX + col * cellStepX;
        float cellY = gridStartY + row * cellStepY - m_scrollY;

        if (cellY + m_cellH < listClipY1 || cellY > listClipY2) continue;

        const auto& item = m_items[i];
        bool isSel = (static_cast<int>(i) == m_selectedIndex);

        // Cell background
        if (isSel) {
            // Selected highlight: gold border with subtle fill
            C2D_DrawRectSolid(cellX, cellY, zContent + 0.01f, m_cellW, m_cellH, C2D_Color32(0x4C, 0x54, 0x47, 0xFF));
            C2D_DrawRectSolid(cellX, cellY, zContent + 0.02f, m_cellW, 1.0f, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
            C2D_DrawRectSolid(cellX, cellY + m_cellH - 1.0f, zContent + 0.02f, m_cellW, 1.0f, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
            C2D_DrawRectSolid(cellX, cellY, zContent + 0.02f, 1.0f, m_cellH, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
            C2D_DrawRectSolid(cellX + m_cellW - 1.0f, cellY, zContent + 0.02f, 1.0f, m_cellH, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
        } else {
            // Unselected slot background
            C2D_DrawRectSolid(cellX, cellY, zContent + 0.01f, m_cellW, m_cellH, C2D_Color32(0x28, 0x2B, 0x26, 0xFF));
            C2D_DrawRectSolid(cellX, cellY, zContent + 0.02f, m_cellW, 1.0f, C2D_Color32(0x38, 0x3C, 0x34, 0xFF));
            C2D_DrawRectSolid(cellX, cellY, zContent + 0.02f, 1.0f, m_cellH, C2D_Color32(0x38, 0x3C, 0x34, 0xFF));
        }

        // Medal icon (24x24, scale 1.5) centered in cell
        float iconX = std::round(cellX + (m_cellW - 24.0f) * 0.5f);
        float iconY = std::round(cellY + (m_cellH - 24.0f) * 0.5f);
        BadgeBanner::drawBadge(item.image, iconX, iconY, 1.5f, 1.0f, !item.unlocked, zContent + 0.03f);
    }

    // Scrollbar indicator
    if (m_maxScroll > 0.0f) {
        float scrollTrackH = m_winH - 20.0f;
        float scrollThumbH = std::max(12.0f, scrollTrackH * ((m_winH - 16.0f) / ((m_winH - 16.0f) + m_maxScroll)));
        float thumbY = (m_winY + 10.0f) + (m_scrollY / m_maxScroll) * (scrollTrackH - scrollThumbH);
        float thumbX = m_winX + m_winW - 6.0f;

        C2D_DrawRectSolid(thumbX, m_winY + 10.0f, zContent + 0.01f, 3.0f, scrollTrackH, C2D_Color32(0x22, 0x24, 0x20, 0xFF));
        C2D_DrawRectSolid(thumbX, thumbY, zContent + 0.02f, 3.0f, scrollThumbH, C2D_Color32(0x7A, 0x81, 0x73, 0xFF));
    }

    // Bottom Action Buttons
    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);

    // Inspect Button
    btnPatch.draw(m_btnInspectX, m_btnInspectY, m_btnInspectW, m_btnInspectH, zContent, 0xFFFFFFFF, 1.0f);
    float inspW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Inspect (A)");
    float inspX = std::round(m_btnInspectX + (m_btnInspectW - inspW) * 0.5f);
    float inspY = std::round(m_btnInspectY + (m_btnInspectH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Inspect (A)", inspX, inspY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // Back Button
    btnPatch.draw(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH, zContent, 0xFFFFFFFF, 1.0f);
    float backW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Back (B)");
    float backX = std::round(m_btnBackX + (m_btnBackW - backW) * 0.5f);
    float backY = std::round(m_btnBackY + (m_btnBackH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Back (B)", backX, backY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // Modal Badge Window if open
    if (m_wndBadge.isOpen()) {
        m_wndBadge.draw(textBuf);
    }
}

} // namespace pd::scenes

