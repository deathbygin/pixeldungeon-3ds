#include "pd/scenes/RankingsScene.h"
#include "pd/Assets.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/Sample.h"
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace pd::scenes {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int x, int y, int w, int h) {
    sub.width = static_cast<u16>(w);
    sub.height = static_cast<u16>(h);
    sub.left = static_cast<float>(x) / potW;
    sub.right = static_cast<float>(x + w) / potW;
    sub.top = 1.0f - static_cast<float>(y) / potH;
    sub.bottom = 1.0f - static_cast<float>(y + h) / potH;
}

static std::string formatScore(int score) {
    std::string s = std::to_string(score);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ",");
        n -= 3;
    }
    return s;
}

RankingsScene::RankingsScene() {
    initTextures();
    refresh();
}

void RankingsScene::initTextures() {
    m_texItems = noosa::Texture::get(Assets::ITEMS);
    m_texIcons = noosa::Texture::get(Assets::ICONS);
    m_texWarrior = noosa::Texture::get(Assets::WARRIOR);
    m_texMage = noosa::Texture::get(Assets::MAGE);
    m_texRogue = noosa::Texture::get(Assets::ROGUE);
    m_texHuntress = noosa::Texture::get(Assets::HUNTRESS);
}

void RankingsScene::refresh() {
    Rankings::instance().load();
    const auto& recs = Rankings::instance().records;
    m_selectedRecord = Rankings::instance().lastRecord;
    if (m_selectedRecord < 0 || m_selectedRecord >= static_cast<int>(recs.size())) {
        m_selectedRecord = 0;
    }

    float totalContentH = recs.size() * (m_rowH + 2.0f);
    m_maxScroll = std::max(0.0f, totalContentH - (m_winH - 24.0f));
    m_scrollY = 0.0f;
}

void RankingsScene::update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch) {
    (void)kHeld;
    m_archs.update(dt);

    if (m_wndRanking.isOpen()) {
        m_wndRanking.update(touch, kDown);
        return;
    }

    const auto& recs = Rankings::instance().records;
    int numRecs = static_cast<int>(recs.size());

    // Back to Title: B button
    if (kDown & KEY_B) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_pendingAction = RankingsAction::BACK_TO_TITLE;
        return;
    }

    // Inspect: A button
    if ((kDown & KEY_A) && numRecs > 0 && m_selectedRecord >= 0 && m_selectedRecord < numRecs) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_wndRanking.show(recs[m_selectedRecord]);
        return;
    }

    // Navigation: D-Pad / Circle Pad UP / DOWN
    if (numRecs > 0) {
        if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
            if (m_selectedRecord > 0) {
                m_selectedRecord--;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
            }
        }
        if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
            if (m_selectedRecord + 1 < numRecs) {
                m_selectedRecord++;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
            }
        }

        // Auto-scroll to keep selected record visible
        float selY = m_selectedRecord * (m_rowH + 2.0f);
        if (selY < m_scrollY) {
            m_scrollY = selY;
        } else if (selY + m_rowH > m_scrollY + (m_winH - 24.0f)) {
            m_scrollY = selY + m_rowH - (m_winH - 24.0f);
        }
        m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScroll);
    }

    // Touch handling
    noosa::RectF rBack(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH);
    if (touch.clicked(rBack)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_pendingAction = RankingsAction::BACK_TO_TITLE;
        return;
    }

    noosa::RectF rInspect(m_btnInspectX, m_btnInspectY, m_btnInspectW, m_btnInspectH);
    if (touch.clicked(rInspect) && numRecs > 0 && m_selectedRecord >= 0 && m_selectedRecord < numRecs) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        m_wndRanking.show(recs[m_selectedRecord]);
        return;
    }

    // Touch drag scrolling & row selection within list window
    noosa::RectF rList(m_winX + 4.0f, m_winY + 4.0f, m_winW - 8.0f, m_winH - 8.0f);
    float tx = static_cast<float>(touch.x());
    float ty = static_cast<float>(touch.y());

    if (touch.justPressed() && rList.contains(tx, ty)) {
        m_isDragging = true;
        m_dragStartY = ty;
        m_dragStartScrollY = m_scrollY;
    }

    if (m_isDragging) {
        if (touch.isDown()) {
            float dy = m_dragStartY - ty;
            m_scrollY = std::clamp(m_dragStartScrollY + dy, 0.0f, m_maxScroll);
        } else {
            m_isDragging = false;
        }
    }

    if (touch.clicked(rList)) {
        float relY = (ty - (m_winY + 8.0f)) + m_scrollY;
        int clickedIdx = static_cast<int>(relY / (m_rowH + 2.0f));
        if (clickedIdx >= 0 && clickedIdx < numRecs) {
            if (m_selectedRecord == clickedIdx) {
                // Double tap on selected record opens details
                m_wndRanking.show(recs[m_selectedRecord]);
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            } else {
                m_selectedRecord = clickedIdx;
                Sample::play(Assets::SND_CLICK, 0.5f, 1.1f);
            }
        }
    }
}

void RankingsScene::drawHeroAvatarTop(float x, float y, Hero::HeroClass cl, int tier) {
    (void)tier;
    std::shared_ptr<noosa::Texture> tex = m_texWarrior;
    if (cl == Hero::HeroClass::Mage) tex = m_texMage;
    else if (cl == Hero::HeroClass::Rogue) tex = m_texRogue;
    else if (cl == Hero::HeroClass::Huntress) tex = m_texHuntress;

    if (tex && tex->isLoaded()) {
        Tex3DS_SubTexture sub;
        setupSubTex(sub, tex->potWidth(), tex->potHeight(), 0, 0, 12, 15);
        C2D_Image img = { tex->getTex(), &sub };
        C2D_DrawParams p;
        p.pos.x = x; p.pos.y = y;
        p.pos.w = 24.0f; p.pos.h = 28.0f;
        p.center.x = p.center.y = 0.0f; p.depth = 0.35f; p.angle = 0.0f;
        C2D_DrawImage(img, &p, nullptr);
    }
}

void RankingsScene::drawTop(C2D_TextBuf textBuf) {
    (void)textBuf;
    m_archs.drawTop(0.05f);

    float zBanner = 0.20f;
    float zContent = 0.30f;
    float zText = 0.40f;

    // Header banner: "TOP RANKINGS"
    float bannerW = 160.0f;
    float bannerH = 22.0f;
    float bannerX = (400.0f - bannerW) * 0.5f;
    float bannerY = 8.0f;

    pd::ui::Chrome::get(pd::ui::Chrome::Type::TOAST).draw(bannerX, bannerY, bannerW, bannerH, zBanner);
    float tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font2x, "HALL OF FAME");
    float tX = std::round((400.0f - tW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font2x, "HALL OF FAME", tX, bannerY + 2.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    const auto& recs = Rankings::instance().records;
    if (recs.empty()) {
        float cardW = 320.0f;
        float cardH = 120.0f;
        float cardX = (400.0f - cardW) * 0.5f;
        float cardY = 56.0f;
        pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(cardX, cardY, cardW, cardH, zContent);

        std::string msg1 = "No games have been played yet.";
        std::string msg2 = "Descend into the dungeon and claim your glory!";
        float m1W = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, msg1);
        float m2W = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, msg2);

        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, msg1, std::round((400.0f - m1W) * 0.5f), cardY + 40.0f, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, msg2, std::round((400.0f - m2W) * 0.5f), cardY + 60.0f, zText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
        return;
    }

    if (m_selectedRecord < 0 || m_selectedRecord >= static_cast<int>(recs.size())) {
        m_selectedRecord = 0;
    }
    const auto& rec = recs[m_selectedRecord];

    // Detail Card for Selected Hero
    float cardW = 368.0f;
    float cardH = 176.0f;
    float cardX = 16.0f;
    float cardY = 34.0f;
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(cardX, cardY, cardW, cardH, zContent);

    // Left Column: Avatar, Class Title, Outcome, Score
    float col1X = cardX + 14.0f;
    float col1Y = cardY + 14.0f;

    // Avatar Box
    C2D_DrawRectSolid(col1X, col1Y, zContent + 0.01f, 32.0f, 34.0f, C2D_Color32(0x35, 0x38, 0x31, 0xFF));
    C2D_DrawRectSolid(col1X + 1.0f, col1Y + 1.0f, zContent + 0.02f, 30.0f, 32.0f, C2D_Color32(0x24, 0x27, 0x21, 0xFF));
    drawHeroAvatarTop(col1X + 4.0f, col1Y + 2.0f, rec.heroClass, rec.armorTier);

    // Rank & Level Title
    const char* classNames[] = { "Warrior", "Mage", "Rogue", "Huntress" };
    int cIdx = static_cast<int>(rec.heroClass);
    std::string clName = (cIdx >= 0 && cIdx < 4) ? classNames[cIdx] : "Hero";
    std::string rankStr = "#" + std::to_string(m_selectedRecord + 1) + "  Level " + std::to_string(rec.level) + " " + clName;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font2x, rankStr, col1X + 40.0f, col1Y + 4.0f, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // Score
    std::string scoreStr = "Score: " + formatScore(rec.score);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, scoreStr, col1X + 40.0f, col1Y + 22.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    // Divider line between columns
    float divX = cardX + 190.0f;
    C2D_DrawRectSolid(divX, cardY + 12.0f, zContent + 0.01f, 1.0f, cardH - 24.0f, C2D_Color32(0x4A, 0x4D, 0x44, 0xFF));

    // Status icon (Tombstone / Amulet) & Outcome string (Method of Death / Win)
    float outY = col1Y + 44.0f;
    float textStartX = col1X + 22.0f;
    float maxInfoW = std::max(20.0f, divX - textStartX - 6.0f);
    auto infoLines = pd::ui::BitmapFont::splitMultiline(pd::ui::BitmapFont::FontType::Font1x, rec.info, maxInfoW);
    for (auto& line : infoLines) {
        if (pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, line) > maxInfoW) {
            while (!line.empty() && pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, line + "...") > maxInfoW) {
                line.pop_back();
            }
            line += "...";
        }
    }

    float totalOutcomeH = std::max(16.0f, static_cast<float>(infoLines.size()) * 10.0f);
    float iconY = outY + (totalOutcomeH - 16.0f) * 0.5f;

    int statusImgIdx = rec.win ? ItemSpriteSheet::AMULET : ItemSpriteSheet::TOMB;
    if (m_texItems && m_texItems->isLoaded()) {
        Tex3DS_SubTexture sub;
        int potW = m_texItems->potWidth();
        int potH = m_texItems->potHeight();
        setupSubTex(sub, potW, potH, (statusImgIdx % 8) * 16, (statusImgIdx / 8) * 16, 16, 16);
        C2D_Image img = { m_texItems->getTex(), &sub };
        C2D_DrawParams p;
        p.pos.x = col1X; p.pos.y = iconY;
        p.pos.w = 16.0f; p.pos.h = 16.0f;
        p.center.x = p.center.y = 0.0f; p.depth = zContent + 0.05f; p.angle = 0.0f;
        C2D_DrawImage(img, &p, nullptr);
    }

    u32 outColor = rec.win ? pd::ui::COLOR_TEXT_POSITIVE : pd::ui::COLOR_TEXT_MUTED;
    float curLineY = (infoLines.size() == 1) ? (outY + 4.0f) : outY;
    for (const auto& line : infoLines) {
        if (!line.empty()) {
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, line, textStartX, curLineY, zText, outColor, pd::ui::COLOR_TEXT_SHADOW);
        }
        curLineY += 10.0f;
    }

    // Left Column stats summary
    float stY = std::max(outY + 26.0f, outY + totalOutcomeH + 6.0f);
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, "Dungeon Depth:", col1X, stY, zText, pd::ui::COLOR_TEXT_MUTED);
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, std::to_string(rec.depth), col1X + 105.0f, stY, zText, pd::ui::COLOR_TEXT_PRIMARY);

    stY += 12.0f;
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, "Gold Collected:", col1X, stY, zText, pd::ui::COLOR_TEXT_MUTED);
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, std::to_string(rec.gold), col1X + 105.0f, stY, zText, pd::ui::COLOR_TEXT_ACCENT);

    stY += 12.0f;
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, "Monsters Slain:", col1X, stY, zText, pd::ui::COLOR_TEXT_MUTED);
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, std::to_string(rec.enemiesSlain), col1X + 105.0f, stY, zText, pd::ui::COLOR_TEXT_PRIMARY);

    stY += 12.0f;
    int durationSec = static_cast<int>(rec.duration);
    int m = durationSec / 60;
    int s = durationSec % 60;
    char durBuf[32];
    std::snprintf(durBuf, sizeof(durBuf), "%dm %02ds", m, s);
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, "Time Elapsed:", col1X, stY, zText, pd::ui::COLOR_TEXT_MUTED);
    pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, durBuf, col1X + 105.0f, stY, zText, pd::ui::COLOR_TEXT_PRIMARY);

    // Right Column: Loadout snapshot
    float col2X = divX + 12.0f;
    float col2Y = cardY + 12.0f;

    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "EQUIPMENT LOADOUT", col2X, col2Y, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);
    col2Y += 16.0f;

    auto drawLoadoutItem = [&](int imgIdx, const std::string& name, const std::string& slot) {
        // Small slot icon
        C2D_DrawRectSolid(col2X, col2Y, zContent + 0.01f, 18.0f, 18.0f, C2D_Color32(0x35, 0x38, 0x31, 0xFF));
        C2D_DrawRectSolid(col2X + 1.0f, col2Y + 1.0f, zContent + 0.02f, 16.0f, 16.0f, C2D_Color32(0x24, 0x27, 0x21, 0xFF));

        if (imgIdx >= 0 && m_texItems && m_texItems->isLoaded()) {
            Tex3DS_SubTexture sub;
            int potW = m_texItems->potWidth();
            int potH = m_texItems->potHeight();
            setupSubTex(sub, potW, potH, (imgIdx % 8) * 16, (imgIdx / 8) * 16, 16, 16);
            C2D_Image img = { m_texItems->getTex(), &sub };
            C2D_DrawParams p;
            p.pos.x = col2X; p.pos.y = col2Y;
            p.pos.w = 16.0f; p.pos.h = 16.0f;
            p.center.x = p.center.y = 0.0f; p.depth = zContent + 0.05f; p.angle = 0.0f;
            C2D_DrawImage(img, &p, nullptr);
        }

        std::string dName = name.empty() ? "(none)" : name;
        u32 col = name.empty() ? pd::ui::COLOR_TEXT_MUTED : pd::ui::COLOR_TEXT_PRIMARY;
        pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, slot + ":", col2X + 22.0f, col2Y - 1.0f, zText, pd::ui::COLOR_TEXT_MUTED);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, dName, col2X + 22.0f, col2Y + 8.0f, zText, col, pd::ui::COLOR_TEXT_SHADOW);
        col2Y += 25.0f;
    };

    drawLoadoutItem(rec.weaponImage, rec.weaponName, "Weapon");
    drawLoadoutItem(rec.armorImage, rec.armorName, "Armor");
    drawLoadoutItem(rec.ring1Image, rec.ring1Name, "Ring");
    drawLoadoutItem(rec.quickslotImage, rec.quickslotName, "Quickslot");

    // Bottom prompt hint
    std::string hint = "Press (A) for Full Details   Press (B) to Return";
    float hW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, hint);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, hint, std::round((400.0f - hW) * 0.5f), 220.0f, zText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
}

void RankingsScene::drawBottom(C2D_TextBuf textBuf) {
    (void)textBuf;
    m_archs.drawBottom(0.05f);

    float zHeader = 0.25f;
    float zContent = 0.30f;
    float zText = 0.40f;

    // Header Mini-Tab: "Rankings"
    float tabW = 80.0f;
    float tabH = 16.0f;
    float tabX = (320.0f - tabW) * 0.5f;
    float tabY = 8.0f;
    pd::ui::Chrome::get(pd::ui::Chrome::Type::TOAST).draw(tabX, tabY, tabW, tabH, zHeader);

    float tabTW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "Rankings");
    float tabTX = std::round((320.0f - tabTW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "Rankings", tabTX, tabY + 3.0f, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // Window Container for records list
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_winX, m_winY, m_winW, m_winH, zContent);

    const auto& recs = Rankings::instance().records;
    float listClipY1 = m_winY + 6.0f;
    float listClipY2 = m_winY + m_winH - 6.0f;

    for (size_t i = 0; i < recs.size(); ++i) {
        float rowY = (m_winY + 8.0f) + i * (m_rowH + 2.0f) - m_scrollY;
        if (rowY + m_rowH < listClipY1 || rowY > listClipY2) continue;

        const auto& r = recs[i];
        bool isSel = (static_cast<int>(i) == m_selectedRecord);

        float rowX = m_winX + 8.0f;
        float rowW = m_winW - 16.0f;

        // Row background
        if (isSel) {
            C2D_DrawRectSolid(rowX, rowY, zContent + 0.01f, rowW, m_rowH, C2D_Color32(0x4C, 0x54, 0x47, 0xFF));
            C2D_DrawRectSolid(rowX, rowY, zContent + 0.02f, rowW, 1.0f, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
            C2D_DrawRectSolid(rowX, rowY + m_rowH - 1.0f, zContent + 0.02f, rowW, 1.0f, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
            C2D_DrawRectSolid(rowX, rowY, zContent + 0.02f, 1.0f, m_rowH, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
            C2D_DrawRectSolid(rowX + rowW - 1.0f, rowY, zContent + 0.02f, 1.0f, m_rowH, C2D_Color32(0xFF, 0xAA, 0x22, 0xFF));
        } else if (i % 2 == 1) {
            C2D_DrawRectSolid(rowX, rowY, zContent + 0.01f, rowW, m_rowH, C2D_Color32(0x28, 0x2B, 0x25, 0x90));
        }

        // Rank #
        std::string rankStr = "#" + std::to_string(i + 1);
        u32 rankCol = isSel ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_MUTED;
        pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, rankStr, rowX + 4.0f, rowY + 9.0f, zText, rankCol);

        // Class badge icon from icons.png
        float iconX = rowX + 26.0f;
        if (m_texIcons && m_texIcons->isLoaded()) {
            Tex3DS_SubTexture sub;
            int potW = m_texIcons->potWidth();
            int potH = m_texIcons->potHeight();
            int cOffset = static_cast<int>(r.heroClass);
            if (cOffset < 0 || cOffset > 3) cOffset = 0;
            setupSubTex(sub, potW, potH, cOffset * 16, 29, 16, 16);
            C2D_Image img = { m_texIcons->getTex(), &sub };
            C2D_DrawParams p;
            p.pos.x = iconX; p.pos.y = rowY + 6.0f;
            p.pos.w = 16.0f; p.pos.h = 16.0f;
            p.center.x = p.center.y = 0.0f; p.depth = zContent + 0.05f; p.angle = 0.0f;
            C2D_DrawImage(img, &p, nullptr);
        }

        // Status icon (Amulet or Tomb)
        float statusX = rowX + 46.0f;
        int statusImg = r.win ? ItemSpriteSheet::AMULET : ItemSpriteSheet::TOMB;
        if (m_texItems && m_texItems->isLoaded()) {
            Tex3DS_SubTexture sub;
            int potW = m_texItems->potWidth();
            int potH = m_texItems->potHeight();
            setupSubTex(sub, potW, potH, (statusImg % 8) * 16, (statusImg / 8) * 16, 16, 16);
            C2D_Image img = { m_texItems->getTex(), &sub };
            C2D_DrawParams p;
            p.pos.x = statusX; p.pos.y = rowY + 6.0f;
            p.pos.w = 16.0f; p.pos.h = 16.0f;
            p.center.x = p.center.y = 0.0f; p.depth = zContent + 0.05f; p.angle = 0.0f;
            C2D_DrawImage(img, &p, nullptr);
        }

        // Score (right aligned)
        std::string scStr = formatScore(r.score);
        float scW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, scStr);
        float scoreX = rowX + rowW - scW - 6.0f;
        u32 scCol = pd::ui::COLOR_TEXT_ACCENT;
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, scStr, scoreX, rowY + 9.0f, zText, scCol, pd::ui::COLOR_TEXT_SHADOW);

        // Hero Info: Level & Class Name
        const char* clNames[] = { "Warrior", "Mage", "Rogue", "Huntress" };
        int clIdx = static_cast<int>(r.heroClass);
        std::string nameStr = "Lvl " + std::to_string(r.level) + " " + ((clIdx >= 0 && clIdx < 4) ? clNames[clIdx] : "Hero");
        u32 nameCol = pd::ui::COLOR_TEXT_PRIMARY;

        float textStartX = rowX + 66.0f;
        float maxTextW = std::max(20.0f, scoreX - textStartX - 8.0f);

        if (pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, nameStr) > maxTextW) {
            while (!nameStr.empty() && pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, nameStr + "...") > maxTextW) {
                nameStr.pop_back();
            }
            nameStr += "...";
        }
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, nameStr, textStartX, rowY + 3.0f, zText, nameCol, pd::ui::COLOR_TEXT_SHADOW);

        // Truncated info string (method of death / win)
        std::string subStr = r.info;
        if (pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, subStr) > maxTextW) {
            while (!subStr.empty() && pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, subStr + "...") > maxTextW) {
                subStr.pop_back();
            }
            subStr += "...";
        }
        pd::ui::BitmapFont::draw(pd::ui::BitmapFont::FontType::Font1x, subStr, textStartX, rowY + 14.0f, zText, pd::ui::COLOR_TEXT_MUTED);
    }

    if (recs.empty()) {
        std::string emptyMsg = "No rankings recorded yet.";
        float eW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, emptyMsg);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, emptyMsg, std::round((320.0f - eW) * 0.5f), m_winY + 80.0f, zText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
    }

    // Bottom Action Buttons
    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);

    // Inspect Button
    btnPatch.draw(m_btnInspectX, m_btnInspectY, m_btnInspectW, m_btnInspectH, zContent, 0xFFFFFFFF, 1.0f);
    float insW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "(A) Details");
    float insX = std::round(m_btnInspectX + (m_btnInspectW - insW) * 0.5f);
    float insY = std::round(m_btnInspectY + (m_btnInspectH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "(A) Details", insX, insY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // Back Button
    btnPatch.draw(m_btnBackX, m_btnBackY, m_btnBackW, m_btnBackH, zContent, 0xFFFFFFFF, 1.0f);
    float backW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, "(B) Back");
    float backX = std::round(m_btnBackX + (m_btnBackW - backW) * 0.5f);
    float backY = std::round(m_btnBackY + (m_btnBackH - 8.0f) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, "(B) Back", backX, backY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // If detail modal is open, draw modal on top!
    if (m_wndRanking.isOpen()) {
        m_wndRanking.draw(textBuf);
    }
}

} // namespace pd::scenes

