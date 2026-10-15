#pragma once

#include <citro2d.h>
#include <memory>
#include <vector>
#include <string>
#include "pd/ui/Archs.h"
#include "pd/ui/TouchController.h"
#include "pd/Hero.h"
#include "noosa/Texture.h"
#include "noosa/TextureFilm.h"

namespace pd::scenes {

enum class SelectAction {
    NONE,
    START_GAME,
    BACK_TO_TITLE
};

class HeroSelectScene {
public:
    HeroSelectScene();
    ~HeroSelectScene() = default;

    void update(float dt, u32 kDown, u32 kHeld, const pd::ui::TouchController& touch);
    void drawTop(C2D_TextBuf textBuf);
    void drawBottom(C2D_TextBuf textBuf);

    Hero::HeroClass getSelectedClass() const { return m_currentClass; }
    SelectAction getPendingAction() const { return m_pendingAction; }
    void clearAction() { m_pendingAction = SelectAction::NONE; }
    void refreshUnlocked();

private:
    SelectAction m_pendingAction = SelectAction::NONE;
    Hero::HeroClass m_currentClass = Hero::HeroClass::Warrior;

    pd::ui::Archs m_archs;

    // Assets
    std::shared_ptr<noosa::Texture> m_texBanners;
    std::shared_ptr<noosa::Texture> m_texAvatars;
    std::shared_ptr<noosa::Texture> m_texWarrior;
    std::shared_ptr<noosa::Texture> m_texMage;
    std::shared_ptr<noosa::Texture> m_texRogue;
    std::shared_ptr<noosa::Texture> m_texHuntress;

    Tex3DS_SubTexture m_subSelectHeroBanner;
    Tex3DS_SubTexture m_subAvatars[4]; // 24x28 per hero
    bool m_texturesInit = false;

    // Animation timer for idle sprite preview
    float m_animTimer = 0.0f;
    int m_animFrame = 0;

    bool m_huntressUnlocked = false;

    // Bottom screen tabs layout
    struct ClassTab {
        float x;
        float y;
        float w;
        float h;
        Hero::HeroClass cl;
        const char* name;
        bool locked;
    };
    std::vector<ClassTab> m_tabs;

    // Bottom action buttons
    float m_btnStartX = 0.0f;
    float m_btnStartY = 0.0f;
    float m_btnStartW = 120.0f;
    float m_btnStartH = 24.0f;

    float m_btnBackX = 0.0f;
    float m_btnBackY = 0.0f;
    float m_btnBackW = 80.0f;
    float m_btnBackH = 24.0f;

    void initTextures();
    void layoutUI();
};

} // namespace pd::scenes

