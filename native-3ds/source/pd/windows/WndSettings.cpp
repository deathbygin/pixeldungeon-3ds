#include "pd/windows/WndSettings.h"
#include "pd/Preferences.h"
#include "pd/SaveManager.h"
#include "pd/ui/Chrome.h"
#include "pd/ui/BitmapFont.h"
#include "pd/Assets.h"
#include "pd/Sample.h"
#include "noosa/Audio.h"
#include <algorithm>
#include <cmath>

namespace pd::windows {

static void setupSubTex(Tex3DS_SubTexture& sub, int potW, int potH, int fx, int fy, int fw, int fh) {
    sub.width = static_cast<u16>(fw);
    sub.height = static_cast<u16>(fh);
    sub.left = static_cast<float>(fx) / static_cast<float>(potW);
    sub.right = static_cast<float>(fx + fw) / static_cast<float>(potW);
    sub.top = 1.0f - (static_cast<float>(fy) / static_cast<float>(potH));
    sub.bottom = 1.0f - (static_cast<float>(fy + fh) / static_cast<float>(potH));
}

WndSettings::WndSettings() {
    m_w = 190.0f;
    m_h = 168.0f;
    m_x = (320.0f - m_w) * 0.5f;
    m_y = (240.0f - m_h) * 0.5f;
    m_texIcons = noosa::Texture::get(pd::Assets::ICONS);
}

void WndSettings::initIcons() {
    if (m_subIconsInit || !m_texIcons || !m_texIcons->isLoaded()) return;
    int potW = m_texIcons->potWidth();
    int potH = m_texIcons->potHeight();
    // Icons.CHECKED: (54, 12, 12, 12), Icons.UNCHECKED: (66, 12, 12, 12)
    setupSubTex(m_subChecked, potW, potH, 54, 12, 12, 12);
    setupSubTex(m_subUnchecked, potW, potH, 66, 12, 12, 12);
    m_subIconsInit = true;
}

void WndSettings::show(bool inGame) {
    m_inGame = inGame;
    m_open = true;
    m_selected = m_inGame ? 1 : 0; // Default highlight on 2x zoom or music
    m_confirmWipe.hide();
}

bool WndSettings::update(const pd::ui::TouchController& touch, u32 kDown) {
    if (!m_open) return false;

    // Route input to confirmation modal if open
    if (m_confirmWipe.isOpen()) {
        m_confirmWipe.update(touch, kDown);
        return true;
    }

    // B or START closes settings modal
    if (kDown & (KEY_B | KEY_START)) {
        Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
        hide();
        return true;
    }

    // Coordinates
    float rowW = m_w - 28.0f;
    float rowX = m_x + 14.0f;
    float btnH = 22.0f;

    if (m_inGame) {
        // Zoom buttons: 1x, 2x, 3x
        float zBtnY = m_y + 24.0f;
        float z1X = rowX;
        float z1W = 46.0f;
        float z2X = rowX + 51.0f;
        float z2W = 60.0f;
        float z3X = rowX + 116.0f;
        float z3W = 46.0f;
        float zH = 20.0f;

        // Checkboxes
        float musicY = m_y + 50.0f;
        float sfxY = m_y + 76.0f;
        float brightY = m_y + 102.0f;

        // Return button
        float retX = m_x + 24.0f;
        float retY = m_y + 132.0f;
        float retW = m_w - 48.0f;
        float retH = 22.0f;

        // Touch interaction
        if (touch.clicked(noosa::RectF(z1X, zBtnY, z1W, zH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setZoom(1.0f);
            m_selected = 0;
            return true;
        }
        if (touch.clicked(noosa::RectF(z2X, zBtnY, z2W, zH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setZoom(2.0f);
            m_selected = 1;
            return true;
        }
        if (touch.clicked(noosa::RectF(z3X, zBtnY, z3W, zH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setZoom(3.0f);
            m_selected = 2;
            return true;
        }
        if (touch.clicked(noosa::RectF(rowX, musicY, rowW, btnH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setMusic(!Preferences::music());
            m_selected = 3;
            return true;
        }
        if (touch.clicked(noosa::RectF(rowX, sfxY, rowW, btnH))) {
            Preferences::setSoundFx(!Preferences::soundFx());
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_selected = 4;
            return true;
        }
        if (touch.clicked(noosa::RectF(rowX, brightY, rowW, btnH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setBrightness(!Preferences::brightness());
            m_selected = 5;
            return true;
        }
        if (touch.clicked(noosa::RectF(retX, retY, retW, retH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            hide();
            return true;
        }

        // Hardware navigation
        if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
            if (m_selected <= 2) m_selected = 6;
            else if (m_selected == 3) m_selected = 1;
            else m_selected--;
        } else if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
            if (m_selected <= 2) m_selected = 3;
            else if (m_selected == 6) m_selected = 1;
            else m_selected++;
        } else if (kDown & (KEY_DLEFT | KEY_CPAD_LEFT)) {
            if (m_selected > 0 && m_selected <= 2) m_selected--;
        } else if (kDown & (KEY_DRIGHT | KEY_CPAD_RIGHT)) {
            if (m_selected < 2) m_selected++;
        } else if (kDown & KEY_A) {
            if (m_selected == 0) Preferences::setZoom(1.0f);
            else if (m_selected == 1) Preferences::setZoom(2.0f);
            else if (m_selected == 2) Preferences::setZoom(3.0f);
            else if (m_selected == 3) Preferences::setMusic(!Preferences::music());
            else if (m_selected == 4) Preferences::setSoundFx(!Preferences::soundFx());
            else if (m_selected == 5) Preferences::setBrightness(!Preferences::brightness());
            else if (m_selected == 6) hide();
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            return true;
        }
    } else {
        // Title Screen layout: Music, SFX, Brightness, Clear All Data, Return
        float musicY = m_y + 26.0f;
        float sfxY = m_y + 52.0f;
        float brightY = m_y + 78.0f;
        float wipeY = m_y + 106.0f;
        float retX = m_x + 24.0f;
        float retY = m_y + 134.0f;
        float retW = m_w - 48.0f;
        float retH = 22.0f;

        auto triggerClearData = [this]() {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_confirmWipe.show(
                "Erase All Data?",
                "Are you sure you want to erase all data? This will permanently delete your active run, rankings, high scores, unlocked badges, and settings.",
                { "Yes", "Cancel" },
                [this](int idx) {
                    if (idx == 0) {
                        SaveManager::wipeAllData();
                        Sample::play(Assets::SND_SHATTER);
                        m_confirmWipe.hide();
                        hide();
                        if (m_onDataWiped) {
                            m_onDataWiped();
                        }
                    } else {
                        m_confirmWipe.hide();
                    }
                });
        };

        // Touch interaction
        if (touch.clicked(noosa::RectF(rowX, musicY, rowW, btnH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setMusic(!Preferences::music());
            m_selected = 0;
            return true;
        }
        if (touch.clicked(noosa::RectF(rowX, sfxY, rowW, btnH))) {
            Preferences::setSoundFx(!Preferences::soundFx());
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            m_selected = 1;
            return true;
        }
        if (touch.clicked(noosa::RectF(rowX, brightY, rowW, btnH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            Preferences::setBrightness(!Preferences::brightness());
            m_selected = 2;
            return true;
        }
        if (touch.clicked(noosa::RectF(rowX, wipeY, rowW, btnH))) {
            m_selected = 3;
            triggerClearData();
            return true;
        }
        if (touch.clicked(noosa::RectF(retX, retY, retW, retH))) {
            Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            hide();
            return true;
        }

        // Hardware navigation: 0..4
        if (kDown & (KEY_DUP | KEY_CPAD_UP)) {
            if (m_selected > 0) m_selected--;
            else m_selected = 4;
        } else if (kDown & (KEY_DDOWN | KEY_CPAD_DOWN)) {
            if (m_selected < 4) m_selected++;
            else m_selected = 0;
        } else if (kDown & KEY_A) {
            if (m_selected == 0) {
                Preferences::setMusic(!Preferences::music());
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            } else if (m_selected == 1) {
                Preferences::setSoundFx(!Preferences::soundFx());
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            } else if (m_selected == 2) {
                Preferences::setBrightness(!Preferences::brightness());
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
            } else if (m_selected == 3) {
                triggerClearData();
            } else if (m_selected == 4) {
                Sample::play(Assets::SND_CLICK, 0.7f, 1.2f);
                hide();
            }
            return true;
        }
    }

    return true; // Absorb input
}

void WndSettings::draw(C2D_TextBuf textBuf) {
    (void)textBuf;
    if (!m_open) return;

    initIcons();

    // 1. Darken background overlay
    C2D_DrawRectSolid(0.0f, 0.0f, 0.80f, 320.0f, 240.0f, C2D_Color32(0x00, 0x00, 0x00, 0xAA));

    // 2. 9-slice stone window chrome from chrome.png
    pd::ui::Chrome::get(pd::ui::Chrome::Type::WINDOW).draw(m_x, m_y, m_w, m_h, 0.82f);

    // 3. Header title "SETTINGS"
    std::string title = "SETTINGS";
    float titleW = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, title);
    float titleX = std::round(m_x + (m_w - titleW) * 0.5f);
    float titleY = m_y + 10.0f;
    pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, title, titleX, titleY, 0.86f, pd::ui::COLOR_TEXT_ACCENT, pd::ui::COLOR_TEXT_SHADOW);

    const auto& btnPatch = pd::ui::Chrome::get(pd::ui::Chrome::Type::BUTTON);
    float rowW = m_w - 28.0f;
    float rowX = m_x + 14.0f;

    auto drawCheckBox = [&](float by, const std::string& label, bool isChecked, int selIdx) {
        bool isFocus = (m_selected == selIdx);
        float brightness = isFocus ? 1.25f : 1.0f;
        btnPatch.draw(rowX, by, rowW, 22.0f, 0.84f, 0xFFFFFFFF, brightness);

        // Text label on left
        float tx = rowX + 10.0f;
        float ty = std::round(by + (22.0f - 8.0f) * 0.5f);
        u32 col = isFocus ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY;
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, label, tx, ty, 0.86f, col, pd::ui::COLOR_TEXT_SHADOW);

        // Checkbox icon on right
        if (m_texIcons && m_texIcons->isLoaded() && m_subIconsInit) {
            Tex3DS_SubTexture& sub = isChecked ? m_subChecked : m_subUnchecked;
            C2D_Image img = { m_texIcons->getTex(), &sub };
            C2D_DrawParams p;
            p.pos.x = rowX + rowW - 20.0f;
            p.pos.y = by + 5.0f;
            p.pos.w = 12.0f;
            p.pos.h = 12.0f;
            p.center.x = p.center.y = 0.0f;
            p.depth = 0.86f;
            p.angle = 0.0f;
            C2D_DrawImage(img, &p, nullptr);
        }
    };

    if (m_inGame) {
        // Zoom selector strip: [ 1x ] [ 2x (Def) ] [ 3x ]
        float zBtnY = m_y + 24.0f;
        float z1X = rowX;
        float z1W = 46.0f;
        float z2X = rowX + 51.0f;
        float z2W = 60.0f;
        float z3X = rowX + 116.0f;
        float z3W = 46.0f;
        float zH = 20.0f;

        float curZoom = Preferences::zoom();

        auto drawZoomBtn = [&](float bx, float bw, const std::string& lbl, bool isCurZoom, int selIdx) {
            bool isFocus = (m_selected == selIdx);
            float brightness = isCurZoom ? 1.4f : (isFocus ? 1.2f : 1.0f);
            btnPatch.draw(bx, zBtnY, bw, zH, 0.84f, 0xFFFFFFFF, brightness);

            float tw = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, lbl);
            float tx = std::round(bx + (bw - tw) * 0.5f);
            float ty = std::round(zBtnY + (zH - 8.0f) * 0.5f);
            u32 col = isCurZoom ? pd::ui::COLOR_TEXT_ACCENT : (isFocus ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY);
            pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, lbl, tx, ty, 0.86f, col, pd::ui::COLOR_TEXT_SHADOW);
        };

        drawZoomBtn(z1X, z1W, "1x", std::abs(curZoom - 1.0f) < 0.1f, 0);
        drawZoomBtn(z2X, z2W, "2x (Def)", std::abs(curZoom - 2.0f) < 0.1f, 1);
        drawZoomBtn(z3X, z3W, "3x", std::abs(curZoom - 3.0f) < 0.1f, 2);

        // Checkboxes (Music, Sound FX, Brightness)
        drawCheckBox(m_y + 50.0f, "Music", Preferences::music(), 3);
        drawCheckBox(m_y + 76.0f, "Sound FX", Preferences::soundFx(), 4);
        drawCheckBox(m_y + 102.0f, "Brightness", Preferences::brightness(), 5);

        // Return Button
        float retX = m_x + 24.0f;
        float retY = m_y + 132.0f;
        float retW = m_w - 48.0f;
        float retH = 22.0f;
        bool isRetFocus = (m_selected == 6);
        btnPatch.draw(retX, retY, retW, retH, 0.84f, 0xFFFFFFFF, isRetFocus ? 1.3f : 1.0f);
        std::string retLabel = "Return";
        float rtw = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, retLabel);
        float rtx = std::round(retX + (retW - rtw) * 0.5f);
        float rty = std::round(retY + (retH - 8.0f) * 0.5f);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, retLabel, rtx, rty, 0.86f,
                                          isRetFocus ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
    } else {
        // Title Screen layout
        drawCheckBox(m_y + 26.0f, "Music", Preferences::music(), 0);
        drawCheckBox(m_y + 52.0f, "Sound FX", Preferences::soundFx(), 1);
        drawCheckBox(m_y + 78.0f, "Brightness", Preferences::brightness(), 2);

        // Clear All Data Button (Dedicated red button)
        float wipeY = m_y + 106.0f;
        bool isWipeFocus = (m_selected == 3);
        btnPatch.draw(rowX, wipeY, rowW, 22.0f, 0.84f, 0xFFFFFFFF, isWipeFocus ? 1.35f : 1.0f);
        std::string wipeLabel = "Clear All Data";
        float wtw = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, wipeLabel);
        float wtx = std::round(rowX + (rowW - wtw) * 0.5f);
        float wty = std::round(wipeY + (22.0f - 8.0f) * 0.5f);
        u32 wipeCol = isWipeFocus ? pd::ui::COLOR_TEXT_WARNING : pd::ui::COLOR_TEXT_PRIMARY;
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, wipeLabel, wtx, wty, 0.86f,
                                          wipeCol, pd::ui::COLOR_TEXT_SHADOW);

        // Return Button
        float retX = m_x + 24.0f;
        float retY = m_y + 134.0f;
        float retW = m_w - 48.0f;
        float retH = 22.0f;
        bool isRetFocus = (m_selected == 4);
        btnPatch.draw(retX, retY, retW, retH, 0.84f, 0xFFFFFFFF, isRetFocus ? 1.3f : 1.0f);
        std::string retLabel = "Return";
        float rtw = pd::ui::BitmapFont::width(pd::ui::BitmapFont::FontType::Font1x, retLabel);
        float rtx = std::round(retX + (retW - rtw) * 0.5f);
        float rty = std::round(retY + (retH - 8.0f) * 0.5f);
        pd::ui::BitmapFont::drawWithShadow(pd::ui::BitmapFont::FontType::Font1x, retLabel, rtx, rty, 0.86f,
                                          isRetFocus ? pd::ui::COLOR_TEXT_ACCENT : pd::ui::COLOR_TEXT_PRIMARY, pd::ui::COLOR_TEXT_SHADOW);
    }

    // Confirmation Modal on top
    if (m_confirmWipe.isOpen()) {
        m_confirmWipe.draw(textBuf, 0.88f);
    }
}

} // namespace pd::windows
