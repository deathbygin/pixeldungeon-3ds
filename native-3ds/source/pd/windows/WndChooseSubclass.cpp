#include "pd/windows/WndChooseSubclass.h"
#include "pd/Hero.h"
#include "pd/Item.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "pd/GLog.h"
#include "pd/FX.h"
#include "pd/SaveManager.h"
#include "pd/CharSprite.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/ui/BottomScreenUI.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace pd::windows {

WndChooseSubclass::WndChooseSubclass() {
}

void WndChooseSubclass::show(TomeOfMastery* tome, Hero* hero) {
    if (!hero) return;
    m_tome = tome;
    m_hero = hero;
    m_open = true;
    m_confirming = false;
    m_pendingChoice = HeroSubClass::None;
    m_focusIndex = 0;
    m_confirmFocus = 0;

    HeroSubClass scA = HeroSubClass::None;
    HeroSubClass scB = HeroSubClass::None;

    switch (hero->heroClass) {
        case Hero::HeroClass::Warrior:
            scA = HeroSubClass::Gladiator;
            scB = HeroSubClass::Berserker;
            break;
        case Hero::HeroClass::Mage:
            scA = HeroSubClass::Battlemage;
            scB = HeroSubClass::Warlock;
            break;
        case Hero::HeroClass::Rogue:
            scA = HeroSubClass::Assassin;
            scB = HeroSubClass::Freerunner;
            break;
        case Hero::HeroClass::Huntress:
            scA = HeroSubClass::Sniper;
            scB = HeroSubClass::Warden;
            break;
    }

    if (hero->subClass == scA) {
        m_isRespec = true;
        m_sc1 = scB;
        m_sc2 = HeroSubClass::None;
    } else if (hero->subClass == scB) {
        m_isRespec = true;
        m_sc1 = scA;
        m_sc2 = HeroSubClass::None;
    } else {
        m_isRespec = false;
        m_sc1 = scA;
        m_sc2 = scB;
    }

    setupLayout();
}

void WndChooseSubclass::setupLayout() {
    float cardW = m_w - 20.0f;
    float cardX = m_x + 10.0f;

    if (!m_isRespec && m_sc2 != HeroSubClass::None) {
        // Two stacked cards
        float cardH = 76.0f;
        float card1Y = m_y + 24.0f;
        float card2Y = card1Y + cardH + 6.0f;

        m_card1.x = cardX;
        m_card1.y = card1Y;
        m_card1.w = cardW;
        m_card1.h = cardH;
        m_card1.btnW = 140.0f;
        m_card1.btnH = 16.0f;
        m_card1.btnX = std::round(cardX + (cardW - m_card1.btnW) * 0.5f);
        m_card1.btnY = card1Y + cardH - m_card1.btnH - 4.0f;
        m_card1.descLines = wrapText(subClassDesc(m_sc1), cardW - 16.0f);

        m_card2.x = cardX;
        m_card2.y = card2Y;
        m_card2.w = cardW;
        m_card2.h = cardH;
        m_card2.btnW = 140.0f;
        m_card2.btnH = 16.0f;
        m_card2.btnX = std::round(cardX + (cardW - m_card2.btnW) * 0.5f);
        m_card2.btnY = card2Y + cardH - m_card2.btnH - 4.0f;
        m_card2.descLines = wrapText(subClassDesc(m_sc2), cardW - 16.0f);

        m_cancelBtnW = 120.0f;
        m_cancelBtnH = 16.0f;
        m_cancelBtnX = std::round(m_x + (m_w - m_cancelBtnW) * 0.5f);
        m_cancelBtnY = card2Y + cardH + 6.0f;
    } else {
        // Single respec card
        float cardH = 110.0f;
        float card1Y = m_y + 35.0f;

        m_card1.x = cardX;
        m_card1.y = card1Y;
        m_card1.w = cardW;
        m_card1.h = cardH;
        m_card1.btnW = 160.0f;
        m_card1.btnH = 18.0f;
        m_card1.btnX = std::round(cardX + (cardW - m_card1.btnW) * 0.5f);
        m_card1.btnY = card1Y + cardH - m_card1.btnH - 8.0f;
        std::string respecText = std::string(subClassDesc(m_sc1)) + "\n\nDo you want to respec into " + subClassTitle(m_sc1) + "?";
        m_card1.descLines = wrapText(respecText, cardW - 16.0f);

        m_card2.h = 0.0f;

        m_cancelBtnW = 120.0f;
        m_cancelBtnH = 16.0f;
        m_cancelBtnX = std::round(m_x + (m_w - m_cancelBtnW) * 0.5f);
        m_cancelBtnY = card1Y + cardH + 16.0f;
    }
}

std::vector<std::string> WndChooseSubclass::wrapText(const std::string& text, float maxW) {
    std::vector<std::string> lines;
    std::stringstream textStream(text);
    std::string paragraph;
    while (std::getline(textStream, paragraph)) {
        if (paragraph.empty()) {
            lines.push_back("");
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
                lines.push_back(currentLine);
                currentLine = word;
            } else {
                if (!currentLine.empty()) {
                    currentLine += " ";
                }
                currentLine += word;
            }
        }
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }
    }
    return lines;
}

void WndChooseSubclass::confirmSelection(HeroSubClass way) {
    m_pendingChoice = way;
    m_confirming = true;
    m_confirmFocus = 0;
    Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
}

void WndChooseSubclass::applySelection(HeroSubClass way) {
    if (!m_hero) return;

    if (m_tome) {
        m_tome->choose(m_hero, way);
        m_tome = nullptr;
    } else {
        m_hero->subClass = way;
        m_hero->spendAndNext(TomeOfMastery::TIME_TO_READ * Actor::TICK);

        HeroSprite* hspr = m_hero->getSprite() ? m_hero->getSprite()->asHeroSprite() : nullptr;
        if (hspr) {
            hspr->operate(m_hero->pos);
        }

        Sample::play(Assets::SND_MASTERY);
        FX::burst(m_hero->pos, 0xffd700FF, 16);
        FX::floatingText(m_hero->pos, subClassTitle(way), 0xffd700FF);
        GLog::w("You have chosen the path of the " + std::string(subClassTitle(way)) + "!");

        if (way == HeroSubClass::Berserker && m_hero->HP <= m_hero->HT * 0.40f) {
            m_hero->affect<Fury>();
        }

        SaveManager::saveGame();
        ui::BottomScreenUI::setLastTarget(nullptr);
        Actor::process();
    }

    hide();
}

bool WndChooseSubclass::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // -------------------------------------------------------------------------
    // Confirmation Sub-Modal Handling
    // -------------------------------------------------------------------------
    if (m_confirming) {
        if (kDown & (KEY_DLEFT | KEY_DRIGHT | KEY_CPAD_LEFT | KEY_CPAD_RIGHT | KEY_DUP | KEY_DDOWN | KEY_CPAD_UP | KEY_CPAD_DOWN)) {
            m_confirmFocus = (m_confirmFocus == 0) ? 1 : 0;
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            return true;
        }

        if (kDown & KEY_A) {
            if (m_confirmFocus == 0) {
                applySelection(m_pendingChoice);
            } else {
                m_confirming = false;
                Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
            }
            return true;
        }

        if (kDown & (KEY_B | KEY_START)) {
            m_confirming = false;
            Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
            return true;
        }

        // Touch handling on confirmation modal
        float boxW = 220.0f;
        float boxH = 90.0f;
        float boxX = std::round((320.0f - boxW) * 0.5f);
        float boxY = std::round((240.0f - boxH) * 0.5f);
        float btnW = 85.0f;
        float btnH = 18.0f;
        float btnY = boxY + boxH - btnH - 10.0f;
        noosa::RectF rYes(boxX + 18.0f, btnY, btnW, btnH);
        noosa::RectF rNo(boxX + boxW - btnW - 18.0f, btnY, btnW, btnH);

        if (touch.clicked(rYes)) {
            applySelection(m_pendingChoice);
            return true;
        }
        if (touch.clicked(rNo)) {
            m_confirming = false;
            Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
            return true;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Main Selection View Handling
    // -------------------------------------------------------------------------
    int maxFocus = (m_isRespec || m_sc2 == HeroSubClass::None) ? 1 : 2;

    if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
        m_focusIndex = (m_focusIndex > 0) ? (m_focusIndex - 1) : maxFocus;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        return true;
    }
    if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
        m_focusIndex = (m_focusIndex < maxFocus) ? (m_focusIndex + 1) : 0;
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        return true;
    }

    if (kDown & KEY_A) {
        if (m_focusIndex == 0) {
            confirmSelection(m_sc1);
        } else if (m_focusIndex == 1 && !m_isRespec) {
            confirmSelection(m_sc2);
        } else {
            hide();
            Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
        }
        return true;
    }

    if (kDown & (KEY_B | KEY_START)) {
        hide();
        Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
        return true;
    }

    // Stylus Touch handling
    noosa::RectF rCard1Btn(m_card1.btnX, m_card1.btnY, m_card1.btnW, m_card1.btnH);
    if (touch.clicked(rCard1Btn)) {
        confirmSelection(m_sc1);
        return true;
    }

    if (!m_isRespec && m_sc2 != HeroSubClass::None) {
        noosa::RectF rCard2Btn(m_card2.btnX, m_card2.btnY, m_card2.btnW, m_card2.btnH);
        if (touch.clicked(rCard2Btn)) {
            confirmSelection(m_sc2);
            return true;
        }
    }

    noosa::RectF rCancel(m_cancelBtnX, m_cancelBtnY, m_cancelBtnW, m_cancelBtnH);
    if (touch.clicked(rCancel)) {
        hide();
        Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
        return true;
    }

    noosa::RectF rWin(m_x, m_y, m_w, m_h);
    if (touch.justReleased() && !rWin.contains(static_cast<float>(touch.x()), static_cast<float>(touch.y()))) {
        hide();
        Sample::play(Assets::SND_CLICK, 0.7f, 1.0f);
        return true;
    }

    return true;
}

void WndChooseSubclass::draw(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open) return;

    float zDim = 0.85f;
    float zWin = 0.87f;
    float zCard = 0.88f;
    float zBtn = 0.89f;
    float zText = 0.92f;

    // 1. Dim background
    C2D_DrawRectSolid(0.0f, 0.0f, zDim, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xB8));

    // 2. Main Window Chrome
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, zWin);

    // 3. Header title
    std::string titleStr = m_isRespec ? "Tome of Remastery" : "Tome of Mastery";
    float titleW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, titleStr);
    float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, titleStr, titleX, m_y + 10.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    // 4. Render Card 1
    C2D_DrawRectSolid(m_card1.x, m_card1.y, zCard, m_card1.w, m_card1.h, C2D_Color32(0x1B, 0x1D, 0x1E, 0xEE));
    C2D_DrawRectSolid(m_card1.x, m_card1.y, zCard + 0.005f, m_card1.w, 1.0f, C2D_Color32(0x3B, 0x3E, 0x3F, 0xFF));
    C2D_DrawRectSolid(m_card1.x, m_card1.y + m_card1.h - 1.0f, zCard + 0.005f, m_card1.w, 1.0f, C2D_Color32(0x11, 0x12, 0x13, 0xFF));

    std::string card1Title = subClassTitle(m_sc1);
    for (auto& c : card1Title) c = static_cast<char>(std::toupper(c));
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, card1Title, m_card1.x + 8.0f, m_card1.y + 5.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    float curY = m_card1.y + 16.0f;
    for (const auto& line : m_card1.descLines) {
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, line, m_card1.x + 8.0f, curY, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
        curY += 9.0f;
    }

    // Button 1
    bool focus1 = (m_focusIndex == 0 && !m_confirming);
    u32 b1Col = focus1 ? C2D_Color32(0x56, 0x3B, 0x22, 0xFF) : C2D_Color32(0x2D, 0x25, 0x1E, 0xFF);
    u32 b1Border = focus1 ? pd::ui::COLOR_TEXT_ACCENT : C2D_Color32(0x55, 0x48, 0x38, 0xFF);
    C2D_DrawRectSolid(m_card1.btnX, m_card1.btnY, zBtn, m_card1.btnW, m_card1.btnH, b1Col);
    C2D_DrawRectSolid(m_card1.btnX, m_card1.btnY, zBtn + 0.005f, m_card1.btnW, 1.0f, b1Border);
    C2D_DrawRectSolid(m_card1.btnX, m_card1.btnY + m_card1.btnH - 1.0f, zBtn + 0.005f, m_card1.btnW, 1.0f, b1Border);
    C2D_DrawRectSolid(m_card1.btnX, m_card1.btnY, zBtn + 0.005f, 1.0f, m_card1.btnH, b1Border);
    C2D_DrawRectSolid(m_card1.btnX + m_card1.btnW - 1.0f, m_card1.btnY, zBtn + 0.005f, 1.0f, m_card1.btnH, b1Border);

    std::string btn1Text = m_isRespec ? "Respec to " + std::string(subClassTitle(m_sc1)) : "Choose " + std::string(subClassTitle(m_sc1));
    float b1tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, btn1Text);
    float b1tX = std::round(m_card1.btnX + (m_card1.btnW - b1tW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, btn1Text, b1tX, m_card1.btnY + 4.0f, zText, focus1 ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

    // 5. Render Card 2 if present
    if (!m_isRespec && m_sc2 != HeroSubClass::None) {
        C2D_DrawRectSolid(m_card2.x, m_card2.y, zCard, m_card2.w, m_card2.h, C2D_Color32(0x1B, 0x1D, 0x1E, 0xEE));
        C2D_DrawRectSolid(m_card2.x, m_card2.y, zCard + 0.005f, m_card2.w, 1.0f, C2D_Color32(0x3B, 0x3E, 0x3F, 0xFF));
        C2D_DrawRectSolid(m_card2.x, m_card2.y + m_card2.h - 1.0f, zCard + 0.005f, m_card2.w, 1.0f, C2D_Color32(0x11, 0x12, 0x13, 0xFF));

        std::string card2Title = subClassTitle(m_sc2);
        for (auto& c : card2Title) c = static_cast<char>(std::toupper(c));
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, card2Title, m_card2.x + 8.0f, m_card2.y + 5.0f, zText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

        float cur2Y = m_card2.y + 16.0f;
        for (const auto& line : m_card2.descLines) {
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, line, m_card2.x + 8.0f, cur2Y, zText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
            cur2Y += 9.0f;
        }

        // Button 2
        bool focus2 = (m_focusIndex == 1 && !m_confirming);
        u32 b2Col = focus2 ? C2D_Color32(0x56, 0x3B, 0x22, 0xFF) : C2D_Color32(0x2D, 0x25, 0x1E, 0xFF);
        u32 b2Border = focus2 ? pd::ui::COLOR_TEXT_ACCENT : C2D_Color32(0x55, 0x48, 0x38, 0xFF);
        C2D_DrawRectSolid(m_card2.btnX, m_card2.btnY, zBtn, m_card2.btnW, m_card2.btnH, b2Col);
        C2D_DrawRectSolid(m_card2.btnX, m_card2.btnY, zBtn + 0.005f, m_card2.btnW, 1.0f, b2Border);
        C2D_DrawRectSolid(m_card2.btnX, m_card2.btnY + m_card2.btnH - 1.0f, zBtn + 0.005f, m_card2.btnW, 1.0f, b2Border);
        C2D_DrawRectSolid(m_card2.btnX, m_card2.btnY, zBtn + 0.005f, 1.0f, m_card2.btnH, b2Border);
        C2D_DrawRectSolid(m_card2.btnX + m_card2.btnW - 1.0f, m_card2.btnY, zBtn + 0.005f, 1.0f, m_card2.btnH, b2Border);

        std::string btn2Text = "Choose " + std::string(subClassTitle(m_sc2));
        float b2tW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, btn2Text);
        float b2tX = std::round(m_card2.btnX + (m_card2.btnW - b2tW) * 0.5f);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, btn2Text, b2tX, m_card2.btnY + 4.0f, zText, focus2 ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
    }

    // 6. Cancel Button (Decide Later)
    bool focusCancel = ((m_focusIndex == 2 || (m_isRespec && m_focusIndex == 1)) && !m_confirming);
    u32 cCol = focusCancel ? C2D_Color32(0x55, 0x22, 0x22, 0xFF) : C2D_Color32(0x28, 0x1A, 0x1A, 0xFF);
    u32 cBorder = focusCancel ? pd::ui::COLOR_TEXT_WARNING : C2D_Color32(0x44, 0x33, 0x33, 0xFF);
    C2D_DrawRectSolid(m_cancelBtnX, m_cancelBtnY, zBtn, m_cancelBtnW, m_cancelBtnH, cCol);
    C2D_DrawRectSolid(m_cancelBtnX, m_cancelBtnY, zBtn + 0.005f, m_cancelBtnW, 1.0f, cBorder);
    C2D_DrawRectSolid(m_cancelBtnX, m_cancelBtnY + m_cancelBtnH - 1.0f, zBtn + 0.005f, m_cancelBtnW, 1.0f, cBorder);
    C2D_DrawRectSolid(m_cancelBtnX, m_cancelBtnY, zBtn + 0.005f, 1.0f, m_cancelBtnH, cBorder);
    C2D_DrawRectSolid(m_cancelBtnX + m_cancelBtnW - 1.0f, m_cancelBtnY, zBtn + 0.005f, 1.0f, m_cancelBtnH, cBorder);

    std::string cancelText = m_isRespec ? "Maybe Later [B]" : "Decide Later [B]";
    float ctW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, cancelText);
    float ctX = std::round(m_cancelBtnX + (m_cancelBtnW - ctW) * 0.5f);
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, cancelText, ctX, m_cancelBtnY + 4.0f, zText, focusCancel ? pd::ui::COLOR_TEXT_WARNING : pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);

    // -------------------------------------------------------------------------
    // 7. Confirmation Dialog Overlay
    // -------------------------------------------------------------------------
    if (m_confirming) {
        float zConfDim = 0.94f;
        float zConfBox = 0.95f;
        float zConfBtn = 0.96f;
        float zConfText = 0.98f;

        C2D_DrawRectSolid(0.0f, 0.0f, zConfDim, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xD0));

        float boxW = 230.0f;
        float boxH = 92.0f;
        float boxX = std::round((320.0f - boxW) * 0.5f);
        float boxY = std::round((240.0f - boxH) * 0.5f);

        pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(boxX, boxY, boxW, boxH, zConfBox);

        std::string confTitle = "Confirm Subclass";
        float confTW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, confTitle);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, confTitle, std::round(boxX + (boxW - confTW) * 0.5f), boxY + 10.0f, zConfText, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

        std::string confMsg = "Follow the way of the " + std::string(subClassTitle(m_pendingChoice)) + "?";
        float confMW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, confMsg);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, confMsg, std::round(boxX + (boxW - confMW) * 0.5f), boxY + 26.0f, zConfText, pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

        std::string confWarn = "This choice locks permanently for this run.";
        float confWW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, confWarn);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, confWarn, std::round(boxX + (boxW - confWW) * 0.5f), boxY + 38.0f, zConfText, pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);

        // Yes and Cancel Buttons
        float btnW = 85.0f;
        float btnH = 18.0f;
        float btnY = boxY + boxH - btnH - 12.0f;
        float btnYesX = boxX + 20.0f;
        float btnNoX = boxX + boxW - btnW - 20.0f;

        bool yesFocus = (m_confirmFocus == 0);
        u32 yesBg = yesFocus ? C2D_Color32(0x2d, 0x66, 0x3d, 0xFF) : C2D_Color32(0x1a, 0x33, 0x22, 0xFF);
        u32 yesBorder = yesFocus ? pd::ui::COLOR_TEXT_POSITIVE : C2D_Color32(0x33, 0x55, 0x33, 0xFF);
        C2D_DrawRectSolid(btnYesX, btnY, zConfBtn, btnW, btnH, yesBg);
        C2D_DrawRectSolid(btnYesX, btnY, zConfBtn + 0.005f, btnW, 1.0f, yesBorder);
        C2D_DrawRectSolid(btnYesX, btnY + btnH - 1.0f, zConfBtn + 0.005f, btnW, 1.0f, yesBorder);
        C2D_DrawRectSolid(btnYesX, btnY, zConfBtn + 0.005f, 1.0f, btnH, yesBorder);
        C2D_DrawRectSolid(btnYesX + btnW - 1.0f, btnY, zConfBtn + 0.005f, 1.0f, btnH, yesBorder);
        std::string yesText = "Confirm [A]";
        float ytW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, yesText);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, yesText, std::round(btnYesX + (btnW - ytW) * 0.5f), btnY + 4.0f, zConfText, yesFocus ? pd::ui::COLOR_TEXT_POSITIVE : pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);

        bool noFocus = (m_confirmFocus == 1);
        u32 noBg = noFocus ? C2D_Color32(0x55, 0x22, 0x22, 0xFF) : C2D_Color32(0x28, 0x1A, 0x1A, 0xFF);
        u32 noBorder = noFocus ? pd::ui::COLOR_TEXT_WARNING : C2D_Color32(0x44, 0x33, 0x33, 0xFF);
        C2D_DrawRectSolid(btnNoX, btnY, zConfBtn, btnW, btnH, noBg);
        C2D_DrawRectSolid(btnNoX, btnY, zConfBtn + 0.005f, btnW, 1.0f, noBorder);
        C2D_DrawRectSolid(btnNoX, btnY + btnH - 1.0f, zConfBtn + 0.005f, btnW, 1.0f, noBorder);
        C2D_DrawRectSolid(btnNoX, btnY, zConfBtn + 0.005f, 1.0f, btnH, noBorder);
        C2D_DrawRectSolid(btnNoX + btnW - 1.0f, btnY, zConfBtn + 0.005f, 1.0f, btnH, noBorder);
        std::string noText = "Cancel [B]";
        float ntW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, noText);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, noText, std::round(btnNoX + (btnW - ntW) * 0.5f), btnY + 4.0f, zConfText, noFocus ? pd::ui::COLOR_TEXT_WARNING : pd::ui::COLOR_TEXT_MUTED, pd::ui::COLOR_TEXT_SHADOW);
    }
}

} // namespace pd::windows

