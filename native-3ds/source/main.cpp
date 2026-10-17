/**
 * Pixel Dungeon - Nintendo 3DS Native Port
 * Phase 4: Dual-Screen UI & Input Decomposition
 *
 * Implements complete decoupling of Pixel Dungeon's single-screen interface:
 * - Top Screen (400x240): Immersive dungeon viewport (camera, tiles, hero, mobs, FOV lighting)
 * - Bottom Screen (320x240): Tactical HUD, active status bars, quickslots, combat log,
 *   and full interactive 19-slot Inventory modal
 * - Input Decomposition: Circle Pad / D-pad movement, hardware buttons [A, B, X, Y, L, R]
 *   supplemented with full stylus touch coordinates translation
 */

#include <3ds.h>
#include <citro2d.h>
#include <cstdio>
#include <cmath>
#include <memory>
#include <vector>
#include <string>

// Engine Subsystems (Phase 2)
#include "noosa/Types.h"
#include "noosa/Texture.h"
#include "noosa/TextureFilm.h"
#include "noosa/Visual.h"
#include "noosa/Image.h"
#include "noosa/MovieClip.h"
#include "noosa/Camera.h"
#include "noosa/Audio.h"
#include "noosa/Music.h"
#include "noosa/Profiler.h"

// Core Game Logic (Phase 3)
#include "pd/Random.h"
#include "pd/Terrain.h"
#include "pd/PathFinder.h"
#include "pd/Level.h"
#include "pd/Actor.h"
#include "pd/Char.h"
#include "pd/CharSprite.h"
#include "pd/Buff.h"
#include "pd/Item.h"
#include "pd/Belongings.h"
#include "pd/Hero.h"
#include "pd/Mob.h"
#include "pd/Dungeon.h"
#include "pd/SaveManager.h"
#include "pd/Assets.h"
#include "pd/DungeonTilemap.h"
#include "pd/ItemSpriteSheet.h"
#include "pd/FogOfWar.h"
#include "pd/ShadowCaster.h"
#include "pd/FX.h"
#include "pd/GLog.h"
#include "pd/Ballistica.h"
#include "pd/MissileSprite.h"
#include "pd/Sample.h"
#include "pd/Chasm.h"

// Dual-Screen UI Subsystems (Phase 4)
#include "pd/ui/TouchController.h"
#include "pd/ui/UIButton.h"
#include "pd/ui/StatusBar.h"
#include "pd/ui/LogConsole.h"
#include "pd/ui/InventoryView.h"
#include "pd/ui/BottomScreenUI.h"
#include "pd/ui/BitmapFont.h"
#include "pd/ui/WndOptions.h"
#include "pd/scenes/TitleScene.h"
#include "pd/scenes/HeroSelectScene.h"
#include "pd/scenes/RankingsScene.h"
#include "pd/scenes/BadgesScene.h"
#include "pd/BadgeBanner.h"
#include "pd/windows/WndStory.h"
#include "pd/windows/WndSettings.h"
#include "pd/Preferences.h"
#include "pd/Rankings.h"
#include "pd/Statistics.h"

using namespace noosa;
using namespace pd;
using namespace pd::ui;

// Procedural 64x64 texture atlas containing tiles, hero, and mob sprites
static std::vector<uint8_t> generateGameSpriteSheet() {
    std::vector<uint8_t> pixels(64 * 64 * 4, 0);

    auto setPixel = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (x >= 0 && x < 64 && y >= 0 && y < 64) {
            int idx = (y * 64 + x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
    };

    // Frame 0: Floor tile (0, 0) - 16x16
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            uint8_t noise = static_cast<uint8_t>((((x * 7) ^ (y * 11)) % 14) + 44);
            setPixel(x, y, noise, noise - 6, noise + 10, 0xFF);
        }
    }

    // Frame 1: Wall tile (16, 0) - 16x16
    for (int y = 0; y < 16; ++y) {
        for (int x = 16; x < 32; ++x) {
            bool edge = (x == 16 || x == 31 || y == 0 || y == 15 || y == 8);
            uint8_t c = edge ? 0x24 : 0x56;
            setPixel(x, y, c + 15, c + 10, c + 25, 0xFF);
        }
    }

    // Frame 2: Door tile (32, 0) - 16x16
    for (int y = 0; y < 16; ++y) {
        for (int x = 32; x < 48; ++x) {
            bool frame = (x == 32 || x == 47 || y == 0 || y == 15);
            if (frame) setPixel(x, y, 0x4a, 0x35, 0x28, 0xFF);
            else setPixel(x, y, 0x7a, 0x58, 0x38, 0xFF);
        }
    }

    // Frame 3: Stairs Down (48, 0) - 16x16
    for (int y = 0; y < 16; ++y) {
        for (int x = 48; x < 64; ++x) {
            int step = (y / 3);
            uint8_t c = static_cast<uint8_t>(20 + step * 25);
            setPixel(x, y, c, c, c + 10, 0xFF);
        }
    }

    // Frame 4: Hero (0, 16) - 16x16
    for (int y = 18; y <= 22; ++y) {
        for (int x = 5; x <= 10; ++x) setPixel(x, y, 0xf6, 0xc6, 0xa5, 0xFF);
    }
    setPixel(6, 20, 0x22, 0x20, 0x34, 0xFF);
    setPixel(9, 20, 0x22, 0x20, 0x34, 0xFF);
    for (int y = 23; y <= 27; ++y) {
        for (int x = 4; x <= 11; ++x) setPixel(x, y, 0x5a, 0xa5, 0xcd, 0xFF);
    }
    for (int x = 4; x <= 11; ++x) setPixel(x, 26, 0xe4, 0xa6, 0x72, 0xFF);
    for (int y = 28; y <= 30; ++y) {
        setPixel(5, y, 0x39, 0x31, 0x4b, 0xFF);
        setPixel(10, y, 0x39, 0x31, 0x4b, 0xFF);
    }

    // Frame 5: Marsupial Rat (16, 16) - 16x16
    for (int y = 22; y <= 28; ++y) {
        for (int x = 18; x <= 29; ++x) setPixel(x, y, 0x8a, 0x6f, 0x5a, 0xFF);
    }
    setPixel(28, 23, 0xff, 0x22, 0x22, 0xFF);
    setPixel(29, 25, 0xf6, 0xc6, 0xa5, 0xFF);
    for (int x = 16; x <= 18; ++x) setPixel(x, 24, 0xf6, 0xc6, 0xa5, 0xFF);

    // Frame 6: Gnoll Scout (32, 16) - 16x16
    for (int y = 18; y <= 28; ++y) {
        for (int x = 36; x <= 44; ++x) setPixel(x, y, 0xa5, 0x82, 0x48, 0xFF);
    }
    setPixel(38, 20, 0xe0, 0x10, 0x10, 0xFF);
    for (int y = 24; y <= 28; ++y) {
        setPixel(45, y, 0xbd, 0xbd, 0xbd, 0xFF);
    }

    // Frame 7: Water / Grass (48, 16) - 16x16
    for (int y = 16; y < 32; ++y) {
        for (int x = 48; x < 64; ++x) {
            uint8_t w = static_cast<uint8_t>((((x * 5) + (y * 9)) % 15) + 50);
            setPixel(x, y, 0x20, w, 0x90, 0xFF);
        }
    }

    return pixels;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // 1. Initialize 3DS hardware services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C2D_SetTintMode(C2D_TintMult);

    Audio::init();
    Profiler::init();

    C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    C2D_TextBuf textBuf = C2D_TextBufNew(4096);

    // 2. Initialize Core Game Logic & Mount RomFS
    SaveManager::init(); // Mounts RomFS and registers APT sleep/suspend hooks
    Dungeon::init();
    FX::init();

    // 3. Asset Pipeline & Sprite Sheets (Step 1 Parity)
    DungeonTilemap tilemap;
    tilemap.loadForDepth(Dungeon::depth);

    auto heroTex  = Texture::get(Assets::WARRIOR);
    auto ratTex   = Texture::get(Assets::RAT);
    auto gnollTex = Texture::get(Assets::GNOLL);
    auto crabTex  = Texture::get(Assets::CRAB);
    auto skelTex  = Texture::get(Assets::SKELETON);
    auto swarmTex = Texture::get(Assets::SWARM);
    auto thiefTex = Texture::get(Assets::THIEF);
    auto shamanTex = Texture::get(Assets::SHAMAN);
    auto batTex   = Texture::get(Assets::BAT);
    auto bruteTex = Texture::get(Assets::BRUTE);
    auto monkTex  = Texture::get(Assets::MONK);
    auto warlockTex = Texture::get(Assets::WARLOCK);
    auto golemTex = Texture::get(Assets::GOLEM);
    auto succubusTex = Texture::get(Assets::SUCCUBUS);
    auto eyeTex   = Texture::get(Assets::EYE);
    auto scorpioTex = Texture::get(Assets::SCORPIO);
    auto piranhaTex = Texture::get(Assets::PIRANHA);
    auto wraithTex = Texture::get(Assets::WRAITH);
    auto gooTex   = Texture::get(Assets::GOO);
    auto tenguTex = Texture::get(Assets::TENGU);
    auto dm300Tex = Texture::get(Assets::DM300);
    auto kingTex  = Texture::get(Assets::KING);
    auto yogTex   = Texture::get(Assets::YOG);
    auto mimicTex = Texture::get(Assets::MIMIC);
    auto itemsTex = Texture::get(Assets::ITEMS);
    auto plantsTex = Texture::get(Assets::PLANTS);
    auto bannersTex = Texture::get(Assets::BANNERS);

    Tex3DS_SubTexture subGameOverBanner;
    bool subGameOverBannerInit = false;

    std::shared_ptr<TextureFilm> heroFilm;
    std::shared_ptr<TextureFilm> ratFilm;
    std::shared_ptr<TextureFilm> gnollFilm;
    std::shared_ptr<TextureFilm> crabFilm;
    std::shared_ptr<TextureFilm> skelFilm;
    std::shared_ptr<TextureFilm> swarmFilm;
    std::shared_ptr<TextureFilm> thiefFilm;
    std::shared_ptr<TextureFilm> shamanFilm;
    std::shared_ptr<TextureFilm> batFilm;
    std::shared_ptr<TextureFilm> bruteFilm;
    std::shared_ptr<TextureFilm> monkFilm;
    std::shared_ptr<TextureFilm> warlockFilm;
    std::shared_ptr<TextureFilm> golemFilm;
    std::shared_ptr<TextureFilm> succubusFilm;
    std::shared_ptr<TextureFilm> eyeFilm;
    std::shared_ptr<TextureFilm> scorpioFilm;
    std::shared_ptr<TextureFilm> piranhaFilm;
    std::shared_ptr<TextureFilm> wraithFilm;
    std::shared_ptr<TextureFilm> gooFilm;
    std::shared_ptr<TextureFilm> tenguFilm;
    std::shared_ptr<TextureFilm> dm300Film;
    std::shared_ptr<TextureFilm> kingFilm;
    std::shared_ptr<TextureFilm> yogFilm;
    std::shared_ptr<TextureFilm> mimicFilm;
    std::shared_ptr<TextureFilm> itemsFilm;
    std::shared_ptr<TextureFilm> plantsFilm;

    if (heroTex && heroTex->isLoaded())       heroFilm     = std::make_shared<TextureFilm>(heroTex, 12, 15);
    if (ratTex && ratTex->isLoaded())         ratFilm      = std::make_shared<TextureFilm>(ratTex, 16, 15);
    if (gnollTex && gnollTex->isLoaded())     gnollFilm    = std::make_shared<TextureFilm>(gnollTex, 12, 15);
    if (crabTex && crabTex->isLoaded())       crabFilm     = std::make_shared<TextureFilm>(crabTex, 16, 16);
    if (skelTex && skelTex->isLoaded())       skelFilm     = std::make_shared<TextureFilm>(skelTex, 12, 15);
    if (swarmTex && swarmTex->isLoaded())     swarmFilm    = std::make_shared<TextureFilm>(swarmTex, 16, 16);
    if (thiefTex && thiefTex->isLoaded())     thiefFilm    = std::make_shared<TextureFilm>(thiefTex, 12, 13);
    if (shamanTex && shamanTex->isLoaded())   shamanFilm   = std::make_shared<TextureFilm>(shamanTex, 12, 15);
    if (batTex && batTex->isLoaded())         batFilm      = std::make_shared<TextureFilm>(batTex, 15, 15);
    if (bruteTex && bruteTex->isLoaded())     bruteFilm    = std::make_shared<TextureFilm>(bruteTex, 12, 16);
    if (monkTex && monkTex->isLoaded())       monkFilm     = std::make_shared<TextureFilm>(monkTex, 15, 14);
    if (warlockTex && warlockTex->isLoaded()) warlockFilm  = std::make_shared<TextureFilm>(warlockTex, 12, 15);
    if (golemTex && golemTex->isLoaded())     golemFilm    = std::make_shared<TextureFilm>(golemTex, 16, 16);
    if (succubusTex && succubusTex->isLoaded()) succubusFilm = std::make_shared<TextureFilm>(succubusTex, 12, 15);
    if (eyeTex && eyeTex->isLoaded())         eyeFilm      = std::make_shared<TextureFilm>(eyeTex, 16, 18);
    if (scorpioTex && scorpioTex->isLoaded()) scorpioFilm  = std::make_shared<TextureFilm>(scorpioTex, 18, 17);
    if (piranhaTex && piranhaTex->isLoaded()) piranhaFilm  = std::make_shared<TextureFilm>(piranhaTex, 12, 16);
    if (wraithTex && wraithTex->isLoaded())   wraithFilm   = std::make_shared<TextureFilm>(wraithTex, 14, 15);
    if (gooTex && gooTex->isLoaded())         gooFilm      = std::make_shared<TextureFilm>(gooTex, 20, 14);
    if (tenguTex && tenguTex->isLoaded())     tenguFilm    = std::make_shared<TextureFilm>(tenguTex, 14, 16);
    if (dm300Tex && dm300Tex->isLoaded())     dm300Film    = std::make_shared<TextureFilm>(dm300Tex, 22, 20);
    if (kingTex && kingTex->isLoaded())       kingFilm     = std::make_shared<TextureFilm>(kingTex, 16, 16);
    if (yogTex && yogTex->isLoaded())         yogFilm      = std::make_shared<TextureFilm>(yogTex, 20, 19);
    if (mimicTex && mimicTex->isLoaded())     mimicFilm    = std::make_shared<TextureFilm>(mimicTex, 16, 16);
    if (itemsTex && itemsTex->isLoaded())     itemsFilm    = std::make_shared<TextureFilm>(itemsTex, 16, 16);
    if (plantsTex && plantsTex->isLoaded())   plantsFilm   = std::make_shared<TextureFilm>(plantsTex, 16, 16);

    // Procedural fallback spritesheet if RomFS is unavailable
    auto sheetData = generateGameSpriteSheet();
    auto fallbackTex = std::make_shared<Texture>();
    fallbackTex->loadFromRGBA8(sheetData.data(), 64, 64);
    auto fallbackFilm = std::make_shared<TextureFilm>(fallbackTex);
    int fFloor    = fallbackFilm->addFrame(0, 0, 16, 16);
    int fWall     = fallbackFilm->addFrame(16, 0, 16, 16);
    int fDoor     = fallbackFilm->addFrame(32, 0, 16, 16);
    int fStairs   = fallbackFilm->addFrame(48, 0, 16, 16);
    fallbackFilm->addFrame(0, 16, 16, 16);
    fallbackFilm->addFrame(16, 16, 16, 16);
    fallbackFilm->addFrame(32, 16, 16, 16);
    int fWater    = fallbackFilm->addFrame(48, 16, 16, 16);

    if (!heroFilm)     heroFilm     = fallbackFilm;
    if (!ratFilm)      ratFilm      = fallbackFilm;
    if (!gnollFilm)    gnollFilm    = fallbackFilm;
    if (!crabFilm)     crabFilm     = fallbackFilm;
    if (!skelFilm)     skelFilm     = fallbackFilm;
    if (!swarmFilm)    swarmFilm    = fallbackFilm;
    if (!thiefFilm)    thiefFilm    = fallbackFilm;
    if (!shamanFilm)   shamanFilm   = fallbackFilm;
    if (!batFilm)      batFilm      = fallbackFilm;
    if (!bruteFilm)    bruteFilm    = fallbackFilm;
    if (!monkFilm)     monkFilm     = fallbackFilm;
    if (!warlockFilm)  warlockFilm  = fallbackFilm;
    if (!golemFilm)    golemFilm    = fallbackFilm;
    if (!succubusFilm) succubusFilm = fallbackFilm;
    if (!eyeFilm)      eyeFilm      = fallbackFilm;
    if (!scorpioFilm)  scorpioFilm  = fallbackFilm;
    if (!piranhaFilm)  piranhaFilm  = fallbackFilm;
    if (!wraithFilm)   wraithFilm   = fallbackFilm;
    if (!gooFilm)      gooFilm      = fallbackFilm;
    if (!tenguFilm)    tenguFilm    = fallbackFilm;
    if (!dm300Film)    dm300Film    = fallbackFilm;
    if (!kingFilm)     kingFilm     = fallbackFilm;
    if (!yogFilm)      yogFilm      = fallbackFilm;
    if (!itemsFilm)    itemsFilm    = fallbackFilm;
    MissileSprite::initFilm(itemsFilm);

    // 5. Setup Preferences, Camera & Dual-Screen UI
    Preferences::init();

    Camera topCamera;
    topCamera.setZoom(Preferences::zoom());
    topCamera.setBounds(0.0f, 0.0f, Level::WIDTH * 16.0f, Level::HEIGHT * 16.0f);

    if (Dungeon::hero) {
        float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
        float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
        topCamera.setCenter(hx, hy);
    }

    BottomScreenUI bottomUI;

    Image tileImg(fallbackFilm);
    tileImg.targetScreen = ScreenTarget::Top;

    Image charImg(fallbackFilm);
    charImg.targetScreen = ScreenTarget::Top;
    charImg.z = 0.6f;

    enum class AppState {
        TITLE,
        PROMPT_LOAD,
        CONFIRM_ERASE,
        HERO_SELECT,
        INTERLEVEL,
        GAME,
        RANKINGS,
        BADGES
    };
    AppState appState = AppState::TITLE;
    pd::scenes::TitleScene titleScene;
    pd::scenes::HeroSelectScene heroSelectScene;
    pd::scenes::RankingsScene rankingsScene;
    pd::scenes::BadgesScene badgesScene;
    pd::ui::WndOptions wndOptions;
    pd::windows::WndStory wndStory;
    Hero::HeroClass chosenHeroClass = Hero::HeroClass::Warrior;
    pd::windows::WndSettings wndTitleSettings;
    wndTitleSettings.setOnDataWiped([&]() {
        heroSelectScene.refreshUnlocked();
        rankingsScene.refresh();
        badgesScene.refresh();
        chosenHeroClass = Hero::HeroClass::Warrior;
        titleScene.clearAction();
    });
    TouchController screenTouch;

    // Interlevel transition state
    enum class InterlevelMode {
        NEW_GAME,
        RESUME,
        DESCEND,
        ASCEND,
        FALL
    };
    InterlevelMode interlevelMode = InterlevelMode::NEW_GAME;
    float interlevelTimer = 0.0f;
    static constexpr float INTERLEVEL_DURATION = 1.0f; // seconds
    bool interlevelStarted = false;

    auto startNewDungeonRun = [&](Hero::HeroClass hClass) {
        pd::Random::seed(osGetTime());
        Dungeon::newGame();
        Dungeon::hero = std::make_unique<Hero>(hClass);
        Dungeon::fog = std::make_unique<FogOfWar>(Level::WIDTH, Level::HEIGHT);
        tilemap.loadForDepth(Dungeon::depth);

        // Place hero on level entrance stairs
        if (Dungeon::level) {
            Dungeon::hero->pos = Dungeon::level->entrance;
            float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
            float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
            topCamera.setCenter(hx, hy);
        }
        Dungeon::observe();
        SaveManager::saveGame();

        bottomUI.reset();
        bottomUI.clearQuickslots();
        bottomUI.log("Entered Depth 1 as " + Dungeon::hero->name + ".");
    };

    auto resumeExistingRun = [&]() {
        bool ok = SaveManager::loadGame();
        if (ok) {
            Dungeon::fog = std::make_unique<FogOfWar>(Level::WIDTH, Level::HEIGHT);
            Dungeon::observe();
            tilemap.loadForDepth(Dungeon::depth);
            if (Dungeon::hero) {
                float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
                float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
                topCamera.setCenter(hx, hy);
            }
            bottomUI.reset();
            bottomUI.log("Resumed Depth " + std::to_string(Dungeon::depth) + " from SD Card!", C2D_Color32(0x38, 0x9e, 0x38, 0xFF));
        }
    };

    // Start Title Screen Background Music
    noosa::Music::play(pd::Assets::THEME, true);

    u64 lastTime = osGetTime();

    // -------------------------------------------------------------------------
    // Main Game Loop
    // -------------------------------------------------------------------------
    while (aptMainLoop()) {
        u64 curTime = osGetTime();
        float dt = (curTime - lastTime) / 1000.0f;
        lastTime = curTime;
        if (dt > 0.1f) dt = 0.1f;

        pd::Sample::update(dt);
        noosa::Music::update();
        BadgeBanner::update(dt);

        Profiler::beginUpdate();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp   = hidKeysUp();

        screenTouch.update(kDown, kHeld, kUp);

        // ---------------------------------------------------------------------
        // STATE 1: TITLE SCREEN
        // ---------------------------------------------------------------------
        if (appState == AppState::TITLE) {
            if (wndTitleSettings.isOpen()) {
                wndTitleSettings.update(screenTouch, kDown);
            } else {
                titleScene.update(dt, kDown, kHeld, screenTouch);

                if (titleScene.getPendingAction() == pd::scenes::TitleAction::PLAY) {
                    titleScene.clearAction();
                    if (SaveManager::hasSave()) {
                        // Save exists: prompt Load Game vs. Erase & Start New
                        int sDepth = 1, sLvl = 1, sClass = 0;
                        SaveManager::getSaveSummary(sDepth, sLvl, sClass);
                        const char* clNames[] = { "Warrior", "Mage", "Rogue", "Huntress" };
                        std::string clName = (sClass >= 0 && sClass < 4) ? clNames[sClass] : "Hero";

                        char msgBuf[128];
                        std::snprintf(msgBuf, sizeof(msgBuf), "%s (Depth %d, Level %d)", clName.c_str(), sDepth, sLvl);

                        wndOptions.show("ACTIVE ADVENTURE", msgBuf,
                            { "Load Game", "New Game", "Cancel" },
                            [&](int idx) {
                                if (idx == 0) {
                                    // Load Game -> Interlevel loading -> Resume
                                    appState = AppState::INTERLEVEL;
                                    interlevelTimer = 0.0f;
                                    interlevelMode = InterlevelMode::RESUME;
                                    interlevelStarted = false;
                                } else if (idx == 1) {
                                    // New Game with existing save -> Prompt Confirmation
                                    appState = AppState::CONFIRM_ERASE;
                                    wndOptions.show("Start New Game?",
                                        "Your current game progress will be permanently erased.",
                                        { "Yes, start new game", "No, return" },
                                        [&](int confIdx) {
                                            if (confIdx == 0) {
                                                SaveManager::deleteSave();
                                                appState = AppState::HERO_SELECT;
                                            } else {
                                                appState = AppState::TITLE;
                                            }
                                        });
                                } else {
                                    appState = AppState::TITLE;
                                }
                            });
                        appState = AppState::PROMPT_LOAD;
                    } else {
                        // No save: straight to Hero Selection
                        appState = AppState::HERO_SELECT;
                    }
                } else if (titleScene.getPendingAction() == pd::scenes::TitleAction::RANKINGS) {
                    titleScene.clearAction();
                    rankingsScene.refresh();
                    appState = AppState::RANKINGS;
                } else if (titleScene.getPendingAction() == pd::scenes::TitleAction::BADGES) {
                    titleScene.clearAction();
                    badgesScene.refresh();
                    appState = AppState::BADGES;
                } else if (titleScene.getPendingAction() == pd::scenes::TitleAction::SETTINGS) {
                    titleScene.clearAction();
                    wndTitleSettings.show(false);
                } else if (titleScene.getPendingAction() != pd::scenes::TitleAction::NONE) {
                    titleScene.clearAction();
                }
            }

            Profiler::beginRender();
            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            C2D_TargetClear(topTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(topTarget);
            titleScene.drawTop(textBuf);

            C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(bottomTarget);
            titleScene.drawBottom(textBuf);
            if (wndTitleSettings.isOpen()) {
                wndTitleSettings.draw(textBuf);
            }

            C3D_FrameEnd(0);
            Profiler::endRender();
            Profiler::endFrame();
            continue;
        }

        // ---------------------------------------------------------------------
        // STATE 2 & 3: SAVE DETECTION & CONFIRMATION MODALS
        // ---------------------------------------------------------------------
        if (appState == AppState::PROMPT_LOAD || appState == AppState::CONFIRM_ERASE) {
            titleScene.update(dt, 0, 0, screenTouch);
            wndOptions.update(screenTouch, kDown);

            Profiler::beginRender();
            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            C2D_TargetClear(topTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(topTarget);
            titleScene.drawTop(textBuf);

            C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(bottomTarget);
            titleScene.drawBottom(textBuf);
            wndOptions.draw(textBuf);

            C3D_FrameEnd(0);
            Profiler::endRender();
            Profiler::endFrame();
            continue;
        }

        // ---------------------------------------------------------------------
        // STATE 4: HERO SELECTION SCREEN
        // ---------------------------------------------------------------------
        if (appState == AppState::HERO_SELECT) {
            heroSelectScene.update(dt, kDown, kHeld, screenTouch);

            if (heroSelectScene.getPendingAction() == pd::scenes::SelectAction::START_GAME) {
                heroSelectScene.clearAction();
                chosenHeroClass = heroSelectScene.getSelectedClass();
                appState = AppState::INTERLEVEL;
                interlevelTimer = 0.0f;
                interlevelMode = InterlevelMode::NEW_GAME;
                interlevelStarted = false;
            } else if (heroSelectScene.getPendingAction() == pd::scenes::SelectAction::BACK_TO_TITLE) {
                heroSelectScene.clearAction();
                appState = AppState::TITLE;
            }

            Profiler::beginRender();
            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            C2D_TargetClear(topTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(topTarget);
            heroSelectScene.drawTop(textBuf);

            C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(bottomTarget);
            heroSelectScene.drawBottom(textBuf);

            C3D_FrameEnd(0);
            Profiler::endRender();
            Profiler::endFrame();
            continue;
        }

        // ---------------------------------------------------------------------
        // STATE: RANKINGS / HIGH SCORES
        // ---------------------------------------------------------------------
        if (appState == AppState::RANKINGS) {
            rankingsScene.update(dt, kDown, kHeld, screenTouch);

            if (rankingsScene.getPendingAction() == pd::scenes::RankingsAction::BACK_TO_TITLE) {
                rankingsScene.clearAction();
                appState = AppState::TITLE;
            }

            Profiler::beginRender();
            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            C2D_TargetClear(topTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(topTarget);
            rankingsScene.drawTop(textBuf);

            C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(bottomTarget);
            rankingsScene.drawBottom(textBuf);

            C3D_FrameEnd(0);
            Profiler::endRender();
            Profiler::endFrame();
            continue;
        }

        // ---------------------------------------------------------------------
        // STATE: BADGES / ACHIEVEMENTS
        // ---------------------------------------------------------------------
        if (appState == AppState::BADGES) {
            badgesScene.update(dt, kDown, kHeld, screenTouch);

            if (badgesScene.getPendingAction() == pd::scenes::BadgesAction::BACK_TO_TITLE) {
                badgesScene.clearAction();
                appState = AppState::TITLE;
            }

            Profiler::beginRender();
            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            C2D_TargetClear(topTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(topTarget);
            badgesScene.drawTop(textBuf);

            C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x0c, 0x18, 0xFF));
            C2D_SceneBegin(bottomTarget);
            badgesScene.drawBottom(textBuf);

            C3D_FrameEnd(0);
            Profiler::endRender();
            Profiler::endFrame();
            continue;
        }

        // ---------------------------------------------------------------------
        // STATE 5: INTERLEVEL TRANSITION ("Descending..." / "Ascending...")
        // ---------------------------------------------------------------------
        if (appState == AppState::INTERLEVEL) {
            interlevelTimer += dt;
            float p = interlevelTimer / INTERLEVEL_DURATION;

            // Generate, resume, descend, or ascend dungeon once at start of transition
            if (!interlevelStarted) {
                interlevelStarted = true;
                if (interlevelMode == InterlevelMode::NEW_GAME) {
                    startNewDungeonRun(chosenHeroClass);
                } else if (interlevelMode == InterlevelMode::RESUME) {
                    resumeExistingRun();
                } else if (interlevelMode == InterlevelMode::DESCEND) {
                    Sample::play(Assets::SND_DESCEND);
                    if (Dungeon::hero) {
                        Hunger* hunger = Dungeon::hero->getBuff<Hunger>();
                        if (hunger && !hunger->isStarving()) {
                            hunger->satisfy(-Hunger::STARVING / 10.0f);
                        }
                    }
                    SaveManager::saveGame();
                    int nextDepth = Dungeon::depth + 1;
                    if (Dungeon::bossLevel(nextDepth)) {
                        Sample::play(Assets::SND_BOSS);
                    }
                    Dungeon::switchLevel(nextDepth);
                    tilemap.loadForDepth(Dungeon::depth);
                    if (Dungeon::hero) {
                        float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
                        float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
                        topCamera.setCenter(hx, hy);
                    }
                    bottomUI.reset();
                    bottomUI.log("Descended to Depth " + std::to_string(Dungeon::depth) + " as " + Dungeon::hero->name + ".");
                } else if (interlevelMode == InterlevelMode::ASCEND) {
                    Sample::play(Assets::SND_DESCEND);
                    if (Dungeon::hero) {
                        Hunger* hunger = Dungeon::hero->getBuff<Hunger>();
                        if (hunger && !hunger->isStarving()) {
                            hunger->satisfy(-Hunger::STARVING / 10.0f);
                        }
                    }
                    SaveManager::saveGame();
                    int prevDepth = Dungeon::depth - 1;
                    Dungeon::switchLevel(prevDepth);
                    tilemap.loadForDepth(Dungeon::depth);
                    if (Dungeon::hero) {
                        float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
                        float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
                        topCamera.setCenter(hx, hy);
                    }
                    bottomUI.reset();
                    bottomUI.log("Ascended to Depth " + std::to_string(Dungeon::depth) + " as " + Dungeon::hero->name + ".");
                } else if (interlevelMode == InterlevelMode::FALL) {
                    Sample::play(Assets::SND_FALLING);
                    if (Dungeon::hero) {
                        Hunger* hunger = Dungeon::hero->getBuff<Hunger>();
                        if (hunger && !hunger->isStarving()) {
                            hunger->satisfy(-Hunger::STARVING / 10.0f);
                        }
                    }
                    SaveManager::saveGame();
                    int nextDepth = Dungeon::depth + 1;
                    if (Dungeon::bossLevel(nextDepth)) {
                        Sample::play(Assets::SND_BOSS);
                    }
                    Dungeon::switchLevel(nextDepth, /*isFall=*/true);
                    tilemap.loadForDepth(Dungeon::depth);
                    if (Dungeon::hero) {
                        float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
                        float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
                        topCamera.setCenter(hx, hy);
                    }
                    bottomUI.reset();
                    bottomUI.log("Fell to Depth " + std::to_string(Dungeon::depth) + " as " + Dungeon::hero->name + ".");
                }
            }

            // Pulse text alpha
            float textAlpha = (p < 0.5f) ? (p * 2.0f) : ((1.0f - p) * 2.0f);
            textAlpha = std::clamp(textAlpha, 0.0f, 1.0f);

            if (interlevelTimer >= INTERLEVEL_DURATION) {
                appState = AppState::GAME;
                noosa::Music::play(pd::Assets::TUNE, true);
                if (interlevelMode == InterlevelMode::NEW_GAME) {
                    wndStory.show(pd::windows::WndStory::ID_SEWERS);
                } else if (interlevelMode == InterlevelMode::DESCEND) {
                    if (Dungeon::depth == 6) wndStory.show(pd::windows::WndStory::ID_PRISON);
                    else if (Dungeon::depth == 11) wndStory.show(pd::windows::WndStory::ID_CAVES);
                    else if (Dungeon::depth == 16) wndStory.show(pd::windows::WndStory::ID_METROPOLIS);
                    else if (Dungeon::depth == 21) wndStory.show(pd::windows::WndStory::ID_HALLS);
                } else if (interlevelMode == InterlevelMode::FALL) {
                    Chasm::heroLand();
                }
            }

            Profiler::beginRender();
            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            // Top Screen: Clean black screen with centered pulsing loading message
            C2D_TargetClear(topTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
            C2D_SceneBegin(topTarget);

            std::string loadingMsg = (interlevelMode == InterlevelMode::ASCEND) ? "Ascending..." :
                                     ((interlevelMode == InterlevelMode::RESUME) ? "Loading..." :
                                     ((interlevelMode == InterlevelMode::FALL) ? "Falling..." : "Descending..."));
            float dW = BitmapFont::width(BitmapFont::FontType::Font2x, loadingMsg);
            float dH = BitmapFont::height(BitmapFont::FontType::Font2x);
            float dX = std::round((400.0f - dW) * 0.5f);
            float dY = std::round((240.0f - dH) * 0.5f);
            u8 a = static_cast<u8>(textAlpha * 255.0f);
            u32 col = C2D_Color32(0xFF, 0xFF, 0xFF, a);
            BitmapFont::draw(BitmapFont::FontType::Font2x, loadingMsg, dX, dY, 0.5f, col);

            // Bottom Screen: Black fade
            C2D_TargetClear(bottomTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
            C2D_SceneBegin(bottomTarget);

            C3D_FrameEnd(0);
            Profiler::endRender();
            Profiler::endFrame();
            continue;
        }

        // ---------------------------------------------------------------------
        // STATE 6: ACTIVE GAMEPLAY
        // ---------------------------------------------------------------------
        Statistics::duration += dt;

        bool storyWasOpen = wndStory.isOpen();
        if (storyWasOpen) {
            wndStory.update(screenTouch, kDown);
        }

        Hero* hero = Dungeon::hero.get();
        Level* level = Dungeon::level.get();

        if (!storyWasOpen) {
            // -----------------------------------------------------------------
            // 1. Bottom Screen UI & Hardware Buttons [A, B, X, Y, L, R]
            // -----------------------------------------------------------------
            // Resting interrupt on directional / input actions
            if (hero && hero->restoreHealth) {
                if ((kDown & ~KEY_A) || (kHeld & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT))) {
                    hero->interrupt();
                }
            }

            bool actionConsumed = bottomUI.update(hero, level, kDown, kHeld, kUp);

            // Handle level transitions (Descent, Ascent, Victory, Fall)
            LevelTransition trans = bottomUI.getPendingTransition();
            if (trans == LevelTransition::NONE) {
                trans = Dungeon::pendingTransition;
            }
            if (trans == LevelTransition::DESCEND) {
                bottomUI.clearPendingTransition();
                Dungeon::pendingTransition = LevelTransition::NONE;
                appState = AppState::INTERLEVEL;
                interlevelMode = InterlevelMode::DESCEND;
                interlevelTimer = 0.0f;
                interlevelStarted = false;
                continue;
            } else if (trans == LevelTransition::ASCEND) {
                bottomUI.clearPendingTransition();
                Dungeon::pendingTransition = LevelTransition::NONE;
                appState = AppState::INTERLEVEL;
                interlevelMode = InterlevelMode::ASCEND;
                interlevelTimer = 0.0f;
                interlevelStarted = false;
                continue;
            } else if (trans == LevelTransition::FALL) {
                bottomUI.clearPendingTransition();
                Dungeon::pendingTransition = LevelTransition::NONE;
                appState = AppState::INTERLEVEL;
                interlevelMode = InterlevelMode::FALL;
                interlevelTimer = 0.0f;
                interlevelStarted = false;
                continue;
            } else if (trans == LevelTransition::WIN) {
                bottomUI.clearPendingTransition();
                Dungeon::pendingTransition = LevelTransition::NONE;
                Dungeon::win("Obtained the Amulet of Yendor");
                SaveManager::deleteSave();
                noosa::Music::play(pd::Assets::HAPPY, true);
                wndOptions.show("HAPPY ENDING!",
                    "You have retrieved the Amulet of Yendor and ascended back to the surface! You are victorious!",
                    { "View Rankings", "Return to Title" },
                    [&](int optIdx) {
                        if (optIdx == 0) {
                            rankingsScene.refresh();
                            appState = AppState::RANKINGS;
                        } else {
                            appState = AppState::TITLE;
                            noosa::Music::play(pd::Assets::THEME, true);
                        }
                    });
                appState = AppState::PROMPT_LOAD;
                continue;
            }

            // Handle pause menu actions (Main Menu, Exit, Rankings, or Start New Game)
            if (bottomUI.getPendingGameAction() == pd::windows::GameAction::MAIN_MENU) {
                bottomUI.clearPendingGameAction();
                if (Dungeon::hero && Dungeon::hero->isAlive()) {
                    SaveManager::saveGame();
                }
                appState = AppState::TITLE;
                noosa::Music::play(pd::Assets::THEME, true);
                continue;
            } else if (bottomUI.getPendingGameAction() == pd::windows::GameAction::EXIT_GAME) {
                bottomUI.clearPendingGameAction();
                if (Dungeon::hero && Dungeon::hero->isAlive()) {
                    SaveManager::saveGame();
                }
                break; // Terminate cleanly to Home Menu
            } else if (bottomUI.getPendingGameAction() == pd::windows::GameAction::RANKINGS) {
                bottomUI.clearPendingGameAction();
                rankingsScene.refresh();
                appState = AppState::RANKINGS;
                continue;
            } else if (bottomUI.getPendingGameAction() == pd::windows::GameAction::START_NEW_GAME) {
                bottomUI.clearPendingGameAction();
                appState = AppState::HERO_SELECT;
                continue;
            }

            if (actionConsumed) {
                Dungeon::mobs.erase(
                    std::remove_if(Dungeon::mobs.begin(), Dungeon::mobs.end(),
                                   [](const std::unique_ptr<Mob>& m) { return !m || !m->isAlive(); }),
                    Dungeon::mobs.end()
                );
                FX::evolveBlobs(level);
                if (Dungeon::pendingTransition == LevelTransition::FALL) {
                    bottomUI.clearPendingTransition();
                    Dungeon::pendingTransition = LevelTransition::NONE;
                    appState = AppState::INTERLEVEL;
                    interlevelMode = InterlevelMode::FALL;
                    interlevelTimer = 0.0f;
                    interlevelStarted = false;
                    continue;
                }
            } else if (hero && hero->restoreHealth && !bottomUI.isPauseMenuOpen()) {
                // Hero is resting: advance turns smoothly
                static float restDelay = 0.0f;
                restDelay += dt;
                if (restDelay >= 0.12f) {
                    restDelay = 0.0f;
                    Actor::process();
                    FX::evolveBlobs(level);
                    Dungeon::mobs.erase(
                        std::remove_if(Dungeon::mobs.begin(), Dungeon::mobs.end(),
                                       [](const std::unique_ptr<Mob>& m) { return !m || !m->isAlive(); }),
                        Dungeon::mobs.end()
                    );
                    if (!hero->isAlive()) {
                        hero->restoreHealth = false;
                    }
                    if (Dungeon::pendingTransition == LevelTransition::FALL) {
                        bottomUI.clearPendingTransition();
                        Dungeon::pendingTransition = LevelTransition::NONE;
                        appState = AppState::INTERLEVEL;
                        interlevelMode = InterlevelMode::FALL;
                        interlevelTimer = 0.0f;
                        interlevelStarted = false;
                        continue;
                    }
                }
            }

            // -----------------------------------------------------------------
            // 2. D-pad & Circle Pad Movement (Supports 8-directional analog movement)
            // -----------------------------------------------------------------
            if (!actionConsumed && !bottomUI.isModalOpen() && !bottomUI.isInventoryOpen() && !bottomUI.isTargeting() && !bottomUI.isExamineMode() && !bottomUI.isPauseMenuOpen() && !bottomUI.isWndHeroOpen() && !FX::isMissileActive() && hero && hero->isAlive() && !hero->restoreHealth) {
                int heroX = hero->pos % Level::WIDTH;
                int heroY = hero->pos / Level::WIDTH;
                int targetCell = -1;

                static float moveDelay = 0.0f;
                moveDelay -= dt;

                circlePosition cpad;
                hidCircleRead(&cpad);

                int dx = 0;
                int dy = 0;
                if (kHeld & KEY_DUP)    dy = -1;
                if (kHeld & KEY_DDOWN)  dy = 1;
                if (kHeld & KEY_DLEFT)  dx = -1;
                if (kHeld & KEY_DRIGHT) dx = 1;

                // Circle pad analog thresholds (supports diagonal directions)
                if (cpad.dx > 45)  dx = 1;
                if (cpad.dx < -45) dx = -1;
                if (cpad.dy > 45)  dy = -1;
                if (cpad.dy < -45) dy = 1;

                if (dx != 0 || dy != 0) {
                    targetCell = (heroY + dy) * Level::WIDTH + (heroX + dx);
                }

                if (targetCell >= 0 && moveDelay <= 0.0f) {
                    bool isInitialTap = (kDown & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT)) != 0;
                    moveDelay = isInitialTap ? 0.20f : 0.12f;

                    Char* targetEnemy = Actor::findChar(targetCell);
                    int prevTargetHp = targetEnemy ? targetEnemy->HP : 0;

                    if (hero->stepTo(targetCell, level)) {
                        if (hero->halted) {
                            hero->halted = false;
                            moveDelay = 0.40f;
                        }

                        if (targetEnemy) {
                            Sample::playAt(Assets::SND_HIT, targetEnemy->pos, 1.0f, Random::Float(0.8f, 1.25f));
                            int dealt = prevTargetHp - targetEnemy->HP;
                            if (targetEnemy->isAlive()) {
                                GLog::i("Hero hit " + targetEnemy->name + " for " + std::to_string(dealt) + " dmg!");
                            } else {
                                GLog::p("Hero defeated " + targetEnemy->name + "!");
                            }
                        } else if (hero->lastStepPickedUp()) {
                            // Item was automatically picked up; pickUp() already triggered the authentic sound effect
                            // (SND_ITEM, SND_GOLD, or SND_DEWDROP). Suppress footstep so the pickup sound is crystal clear.
                        } else {
                            Sample::play((level && level->water[targetCell]) ? Assets::SND_WATER : Assets::SND_STEP, 0.8f, Random::Float(0.95f, 1.05f));
                        }

                        // Run continuous turn timeline scheduler
                        Actor::process();
                        FX::evolveBlobs(level);
                        Dungeon::mobs.erase(
                            std::remove_if(Dungeon::mobs.begin(), Dungeon::mobs.end(),
                                           [](const std::unique_ptr<Mob>& m) {
                                               return !m || (!m->isAlive() && (!m->sprite || m->sprite->isDead()));
                                           }),
                            Dungeon::mobs.end()
                        );

                        if (!hero->isAlive()) {
                            // Dead hero remains on gameplay screen with full UI inspection access
                        } else if (level) {
                            if (level->map[hero->pos] == Terrain::EXIT || level->map[hero->pos] == Terrain::UNLOCKED_EXIT) {
                                bottomUI.promptStairs(true);
                            } else if (level->map[hero->pos] == Terrain::ENTRANCE) {
                                bottomUI.promptStairs(false);
                            }
                        }

                        if (Dungeon::pendingTransition == LevelTransition::FALL) {
                            bottomUI.clearPendingTransition();
                            Dungeon::pendingTransition = LevelTransition::NONE;
                            appState = AppState::INTERLEVEL;
                            interlevelMode = InterlevelMode::FALL;
                            interlevelTimer = 0.0f;
                            interlevelStarted = false;
                            continue;
                        }
                    }
                }
            }
        }

        // Update Sprite Animations & Movement Interpolation
        if (hero) {
            CharSprite* hspr = hero->getSprite();
            if (hspr) hspr->update(dt);
        }
        for (const auto& mob : Dungeon::mobs) {
            if (mob) {
                CharSprite* mspr = mob->getSprite();
                if (mspr) mspr->update(dt);
            }
        }

        // Camera follow & update with smooth sprite tracking
        if (hero) {
            CharSprite* hspr = hero->getSprite();
            float hx = hspr ? (hspr->getX() + hspr->getWidth() * 0.5f) : ((hero->pos % Level::WIDTH) * 16.0f + 8.0f);
            float hy = hspr ? (hspr->getY() + hspr->getHeight() * 0.5f) : ((hero->pos / Level::WIDTH) * 16.0f + 8.0f);
            float cx = topCamera.getCenter().x;
            float cy = topCamera.getCenter().y;
            if (bottomUI.isExamineMode()) {
                int ec = bottomUI.getExamineCell();
                float ex = (ec % Level::WIDTH) * 16.0f + 8.0f;
                float ey = (ec / Level::WIDTH) * 16.0f + 8.0f;
                topCamera.setCenter(cx + (ex - cx) * 0.25f, cy + (ey - cy) * 0.25f);
            } else if (bottomUI.isTargeting()) {
                int tc = bottomUI.getTargetCell();
                float tx = (tc % Level::WIDTH) * 16.0f + 8.0f;
                float ty = (tc / Level::WIDTH) * 16.0f + 8.0f;
                float targetCx = (hx + tx) * 0.5f;
                float targetCy = (hy + ty) * 0.5f;
                topCamera.setCenter(cx + (targetCx - cx) * 0.2f, cy + (targetCy - cy) * 0.2f);
            } else {
                topCamera.setCenter(cx + (hx - cx) * 0.2f, cy + (hy - cy) * 0.2f);
            }
        }
        topCamera.update(dt);
        FX::update(dt);
        Sample::update(dt);
        tilemap.update(dt);

        Profiler::beginRender();

        C2D_TextBufClear(textBuf);

        // ---------------------------------------------------------------------
        // BEGIN FRAME
        // ---------------------------------------------------------------------
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // =====================================================================
        // TOP SCREEN (400x240): Clean, Immersive Dungeon Viewport
        // =====================================================================
        C2D_TargetClear(topTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_SceneBegin(topTarget);

        static int lastDungeonDepth = Dungeon::depth;
        if (Dungeon::depth != lastDungeonDepth) {
            tilemap.loadForDepth(Dungeon::depth);
            lastDungeonDepth = Dungeon::depth;
        }

        if (level && hero) {
            uint32_t drawnTiles = 0;
            uint32_t culledTiles = 0;

            if (tilemap.getFilm() && tilemap.getTexture()->isLoaded()) {
                tilemap.draw(level, &topCamera, drawnTiles, culledTiles);
            } else {
                float camL, camT, camR, camB;
                topCamera.getVisibleWorldBounds(camL, camT, camR, camB);
                int minTileX = std::max(0, static_cast<int>(camL / 16.0f));
                int maxTileX = std::min(Level::WIDTH - 1, static_cast<int>(camR / 16.0f) + 1);
                int minTileY = std::max(0, static_cast<int>(camT / 16.0f));
                int maxTileY = std::min(Level::HEIGHT - 1, static_cast<int>(camB / 16.0f) + 1);

                for (int y = minTileY; y <= maxTileY; ++y) {
                    for (int x = minTileX; x <= maxTileX; ++x) {
                        int cellIdx = y * Level::WIDTH + x;

                        drawnTiles++;

                        tileImg.x = x * 16.0f;
                        tileImg.y = y * 16.0f;
                        tileImg.z = 0.2f;

                        int t = level->get(cellIdx);
                        if (t == Terrain::WALL || t == Terrain::WALL_DECO || t == Terrain::SECRET_DOOR) tileImg.setFrame(fWall);
                        else if (t == Terrain::DOOR) tileImg.setFrame(fDoor);
                        else if (t == Terrain::EXIT) tileImg.setFrame(fStairs);
                        else if (t == Terrain::WATER) tileImg.setFrame(fWater);
                        else tileImg.setFrame(fFloor);

                        uint8_t tileB = Preferences::brightness() ? 255 : 215;
                        tileImg.tint = Color(tileB, tileB, tileB, 255);
                        tileImg.draw(&topCamera);
                    }
                }
                culledTiles = Level::LENGTH - drawnTiles;
            }
            Profiler::recordTiles(drawnTiles, culledTiles);

            // Render Ground Heaps (chests, skeletons, dropped/placed items)
            if (itemsFilm) {
                for (const auto& heap : level->heaps) {
                    if (heap && (!heap->isEmpty() || heap->type != Heap::Type::HEAP) && level->visited[heap->pos]) {
                        charImg.x = (heap->pos % Level::WIDTH) * 16.0f;
                        charImg.y = (heap->pos / Level::WIDTH) * 16.0f;
                        charImg.z = 0.25f;
                        charImg.tint = Color(255, 255, 255, 255);
                        charImg.setFilm(itemsFilm, heap->image());
                        charImg.draw(&topCamera);
                    }
                }
            }

            // Render Active Plants on the ground
            if (plantsFilm) {
                for (const auto& plant : level->plants) {
                    if (plant && (level->visited[plant->pos] || Dungeon::visible[plant->pos])) {
                        charImg.x = (plant->pos % Level::WIDTH) * 16.0f;
                        charImg.y = (plant->pos / Level::WIDTH) * 16.0f;
                        charImg.z = 0.24f;
                        charImg.tint = Color(255, 255, 255, 255);
                        charImg.setFilm(plantsFilm, plant->image);
                        charImg.draw(&topCamera);
                    }
                }
            }

            // Render Ground FX (Water ripples & search checked cell highlights)
            FX::drawGround(&topCamera);

            // Render Mobs (strictly visible in hero's field of view / line of sight)
            for (const auto& mob : Dungeon::mobs) {
                if (mob) {
                    CharSprite* spr = mob->getSprite();
                    if (spr && !spr->isDead()) {
                        if (mob->spriteVisible && Dungeon::visible[mob->pos]) {
                            spr->draw(&topCamera);

                            // Sleeping indicator matching Java "Zzz"
                            if (mob->isAlive() && mob->state == Mob::State::SLEEPING) {
                                FX::floatingText(mob->pos, "Zzz", 0x88ccffff);
                            }
                        }
                    }
                }
            }

            // Render Hero with full animated state machine & armor tier
            if (hero) {
                CharSprite* spr = hero->getSprite();
                if (spr && !spr->isDead()) {
                    spr->draw(&topCamera);
                }

                // Resting "Zzz" indicator
                static float heroSleepTimer = 0.0f;
                if (hero->restoreHealth) {
                    heroSleepTimer += dt;
                    if (heroSleepTimer >= 0.8f) {
                        heroSleepTimer = 0.0f;
                        FX::floatingText(hero->pos, "Zzz", 0xa5cde4FF);
                    }
                } else {
                    heroSleepTimer = 0.0f;
                }
            }

            // Smooth tile-based Fog of War
            if (Dungeon::fog) {
                Dungeon::fog->draw(&topCamera);
            }

            // Ranged Targeting Trajectory & Reticle Overlay
            if (bottomUI.isTargeting() && hero) {
                int targetCell = bottomUI.getTargetCell();
                Item* targetItem = bottomUI.getTargetItem();
                bool magic = false;
                bool hitChars = true;
                if (targetItem) {
                    if (Wand* w = targetItem->asWand()) {
                        magic = true;
                        hitChars = w->hitsChars();
                    }
                }
                int collisionCell = Ballistica::cast(hero->pos, targetCell, magic, hitChars);

                // Draw trajectory line / dots along Ballistica::trace
                for (int i = 1; i < Ballistica::distance; ++i) {
                    int c = Ballistica::trace[i];
                    float wx = (c % Level::WIDTH) * 16.0f + 8.0f;
                    float wy = (c / Level::WIDTH) * 16.0f + 8.0f;
                    float sx, sy;
                    topCamera.worldToScreen(wx, wy, sx, sy);

                    bool isPastCollision = (i > Ballistica::distTo(collisionCell));
                    u32 dotCol = isPastCollision
                        ? C2D_Color32(0xdd, 0x44, 0x44, 0xAA)
                        : ((collisionCell == targetCell) ? C2D_Color32(0x56, 0xbb, 0x77, 0xEE) : C2D_Color32(0xee, 0xaa, 0x33, 0xEE));

                    C2D_DrawCircleSolid(sx, sy, 0.78f, 2.5f * topCamera.getZoom(), dotCol);
                }

                // Draw Reticle Brackets on targetCell
                float rx = (targetCell % Level::WIDTH) * 16.0f;
                float ry = (targetCell / Level::WIDTH) * 16.0f;
                float sx1, sy1, sx2, sy2;
                topCamera.worldToScreen(rx, ry, sx1, sy1);
                topCamera.worldToScreen(rx + 16.0f, ry + 16.0f, sx2, sy2);

                u32 reticleCol = (collisionCell == targetCell)
                    ? C2D_Color32(0x56, 0xbb, 0x77, 0xFF)
                    : C2D_Color32(0xee, 0x55, 0x55, 0xFF);

                float thick = 2.0f;
                float clen = 5.0f * topCamera.getZoom();
                // Top-Left
                C2D_DrawRectSolid(sx1, sy1, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx1, sy1, 0.79f, thick, clen, reticleCol);
                // Top-Right
                C2D_DrawRectSolid(sx2 - clen, sy1, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx2 - thick, sy1, 0.79f, thick, clen, reticleCol);
                // Bottom-Left
                C2D_DrawRectSolid(sx1, sy2 - thick, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx1, sy2 - clen, 0.79f, thick, clen, reticleCol);
                // Bottom-Right
                C2D_DrawRectSolid(sx2 - clen, sy2 - thick, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx2 - thick, sy2 - clen, 0.79f, thick, clen, reticleCol);

                // If collision blocked before target, draw collision indicator at collisionCell
                if (collisionCell != targetCell) {
                    float cx = (collisionCell % Level::WIDTH) * 16.0f + 8.0f;
                    float cy = (collisionCell / Level::WIDTH) * 16.0f + 8.0f;
                    float csx, csy;
                    topCamera.worldToScreen(cx, cy, csx, csy);
                    C2D_DrawCircleSolid(csx, csy, 0.79f, 3.5f * topCamera.getZoom(), C2D_Color32(0xff, 0x22, 0x22, 0xDD));
                }
            }

            // Top-Screen Examine Mode Reticle Overlay (pulsing gold/white brackets)
            if (bottomUI.isExamineMode()) {
                int examineCell = bottomUI.getExamineCell();
                float rx = (examineCell % Level::WIDTH) * 16.0f;
                float ry = (examineCell / Level::WIDTH) * 16.0f;
                float sx1, sy1, sx2, sy2;
                topCamera.worldToScreen(rx, ry, sx1, sy1);
                topCamera.worldToScreen(rx + 16.0f, ry + 16.0f, sx2, sy2);

                // Pulsing white and gold color animation
                static float examinePulse = 0.0f;
                examinePulse += dt * 4.0f;
                float p = 0.5f + 0.5f * std::sin(examinePulse);
                u8 r = static_cast<u8>(255 * p + 255 * (1.0f - p));
                u8 g = static_cast<u8>(215 * p + 255 * (1.0f - p));
                u8 b = static_cast<u8>(0 * p + 255 * (1.0f - p));
                u32 reticleCol = C2D_Color32(r, g, b, 0xFF);
                u32 innerTint = C2D_Color32(r, g, b, 0x25);

                // Faint inner tile fill
                C2D_DrawRectSolid(sx1, sy1, 0.78f, sx2 - sx1, sy2 - sy1, innerTint);

                float thick = 2.0f;
                float clen = 5.0f * topCamera.getZoom();
                // Top-Left corner
                C2D_DrawRectSolid(sx1, sy1, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx1, sy1, 0.79f, thick, clen, reticleCol);
                // Top-Right corner
                C2D_DrawRectSolid(sx2 - clen, sy1, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx2 - thick, sy1, 0.79f, thick, clen, reticleCol);
                // Bottom-Left corner
                C2D_DrawRectSolid(sx1, sy2 - thick, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx1, sy2 - clen, 0.79f, thick, clen, reticleCol);
                // Bottom-Right corner
                C2D_DrawRectSolid(sx2 - clen, sy2 - thick, 0.79f, clen, thick, reticleCol);
                C2D_DrawRectSolid(sx2 - thick, sy2 - clen, 0.79f, thick, clen, reticleCol);
            }

            // Render Core Visual Effects & Particles
            FX::draw(&topCamera, textBuf);

            // Full-Screen Flash Overlay (Psionic Blast, Wipe Out, etc.)
            if (FX::isFlashing()) {
                C2D_DrawRectSolid(0.0f, 0.0f, 0.88f, 400.0f, 240.0f, FX::getFlashColor());
            }

            // Game Over Banner overlay if hero perished
            if (hero && !hero->isAlive()) {
                if (!subGameOverBannerInit && bannersTex && bannersTex->isLoaded()) {
                    float potW = static_cast<float>(bannersTex->potWidth());
                    float potH = static_cast<float>(bannersTex->potHeight());
                    subGameOverBanner.width = 128;
                    subGameOverBanner.height = 35;
                    subGameOverBanner.left = 0.0f;
                    subGameOverBanner.right = 128.0f / potW;
                    subGameOverBanner.top = 1.0f - (105.0f / potH);
                    subGameOverBanner.bottom = 1.0f - (140.0f / potH);
                    subGameOverBannerInit = true;
                }

                if (bannersTex && bannersTex->isLoaded() && bannersTex->getTex() && subGameOverBannerInit) {
                    C2D_Image img;
                    img.tex = bannersTex->getTex();
                    img.subtex = &subGameOverBanner;

                    float bScale = 1.5f;
                    float bW = 128.0f * bScale;
                    float bH = 35.0f * bScale;
                    C2D_DrawParams params;
                    params.pos.x = (400.0f - bW) * 0.5f;
                    params.pos.y = (240.0f - bH) / 3.0f;
                    params.pos.w = bW;
                    params.pos.h = bH;
                    params.center.x = 0.0f;
                    params.center.y = 0.0f;
                    params.depth = 0.90f;
                    params.angle = 0.0f;
                    C2D_DrawImage(img, &params, nullptr);
                }
            }

            // Real-time Badge Unlock Banner Overlay
            if (BadgeBanner::isActive()) {
                BadgeBanner::drawTop(textBuf);
            }
        }

        // Real-time Performance Profiling Overlay (toggled via [SELECT])
        if (Profiler::isOverlayEnabled()) {
            C2D_DrawRectSolid(0.0f, 0.0f, 0.95f, 400.0f, 16.0f, C2D_Color32(0x0a, 0x0a, 0x14, 0xEE));
            char profBuf[128];
            std::snprintf(profBuf, sizeof(profBuf), "FPS: %.1f | Frame: %.2fms (Upd: %.2fms, Ren: %.2fms) | Culled: %lu/%d",
                          Profiler::getFps(), Profiler::getFrameTimeMs(),
                          Profiler::getUpdateTimeMs(), Profiler::getRenderTimeMs(),
                          static_cast<unsigned long>(Profiler::getTilesCulled()), Level::LENGTH);
            C2D_Text profText;
            C2D_TextParse(&profText, textBuf, profBuf);
            C2D_TextOptimize(&profText);
            C2D_DrawText(&profText, C2D_WithColor, 6.0f, 1.0f, 0.96f, 0.38f, 0.38f, C2D_Color32(0x40, 0xf0, 0x60, 0xFF));
        }

        // =====================================================================
        // BOTTOM SCREEN (320x240): Tactical HUD, Quickslots, Inventory & Log
        // =====================================================================
        C2D_TargetClear(bottomTarget, C2D_Color32(0x14, 0x14, 0x14, 0xFF));
        C2D_SceneBegin(bottomTarget);

        bottomUI.draw(hero, Dungeon::depth, textBuf);

        if (wndStory.isOpen()) {
            wndStory.draw(textBuf);
        }

        C3D_FrameEnd(0);
        Profiler::endRender();
        Profiler::endFrame();
    }

    // 6. Shutdown & Persistence
    if (Dungeon::hero && Dungeon::hero->isAlive()) {
        SaveManager::saveGame();
    } else {
        SaveManager::deleteSave();
    }
    SaveManager::exit();

    Audio::exit();
    C2D_TextBufDelete(textBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
