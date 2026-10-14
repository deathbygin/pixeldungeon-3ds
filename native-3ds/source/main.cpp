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

// Dual-Screen UI Subsystems (Phase 4)
#include "pd/ui/TouchController.h"
#include "pd/ui/UIButton.h"
#include "pd/ui/StatusBar.h"
#include "pd/ui/LogConsole.h"
#include "pd/ui/InventoryView.h"
#include "pd/ui/BottomScreenUI.h"

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
    auto itemsTex = Texture::get(Assets::ITEMS);

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
    std::shared_ptr<TextureFilm> itemsFilm;

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
    if (itemsTex && itemsTex->isLoaded())     itemsFilm    = std::make_shared<TextureFilm>(itemsTex, 16, 16);

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

    // 4. Audio feedback
    SoundSample sndStep;    sndStep.synthesizeTone(240.0f, 0.03f, 0.3f);
    SoundSample sndHit;     sndHit.synthesizeTone(180.0f, 0.08f, 0.6f);

    bool loadedSave = false;
    if (SaveManager::hasSave()) {
        loadedSave = SaveManager::loadGame();
        if (loadedSave) {
            Dungeon::fog = std::make_unique<FogOfWar>(Level::WIDTH, Level::HEIGHT);
            Dungeon::observe();
        }
    }

    if (!loadedSave) {
        pd::Random::seed(1337);
        Dungeon::newGame();

        // Add extra items to Hero backpack for starting kit
        if (Dungeon::hero) {
            Dungeon::hero->belongings.collect(std::make_unique<Weapon>("Longsword", 4, 12, 14));
            Dungeon::hero->belongings.collect(std::make_unique<Armor>("Leather Armor", 2, 12));
            Dungeon::hero->belongings.collect(std::make_unique<Scroll>(Scroll::ScrollType::UPGRADE));
            Dungeon::hero->belongings.collect(std::make_unique<Scroll>(Scroll::ScrollType::IDENTIFY));
            Dungeon::hero->belongings.collect(std::make_unique<Scroll>(Scroll::ScrollType::REMOVE_CURSE));
            Dungeon::hero->belongings.collect(std::make_unique<Potion>(Potion::PotionType::STRENGTH));
        }
        SaveManager::saveGame();
    }

    // 5. Setup Camera & Dual-Screen UI
    Camera topCamera;
    topCamera.setZoom(2.0f);
    topCamera.setBounds(0.0f, 0.0f, Level::WIDTH * 16.0f, Level::HEIGHT * 16.0f);

    if (Dungeon::hero) {
        float hx = (Dungeon::hero->pos % Level::WIDTH) * 16.0f + 8.0f;
        float hy = (Dungeon::hero->pos / Level::WIDTH) * 16.0f + 8.0f;
        topCamera.setCenter(hx, hy);
    }

    BottomScreenUI bottomUI;
    if (loadedSave) {
        bottomUI.log("Resumed Depth " + std::to_string(Dungeon::depth) + " from SD Card!", C2D_Color32(0x38, 0x9e, 0x38, 0xFF));
    } else {
        bottomUI.log("Welcome to Pixel Dungeon 3DS! Autosave active.");
    }

    char strBuf[256];
    C2D_Text textObj;

    Image tileImg(fallbackFilm);
    tileImg.targetScreen = ScreenTarget::Top;

    Image charImg(fallbackFilm);
    charImg.targetScreen = ScreenTarget::Top;
    charImg.z = 0.6f;

    u64 lastTime = osGetTime();

    // -------------------------------------------------------------------------
    // Main Game Loop
    // -------------------------------------------------------------------------
    while (aptMainLoop()) {
        u64 curTime = osGetTime();
        float dt = (curTime - lastTime) / 1000.0f;
        lastTime = curTime;
        if (dt > 0.1f) dt = 0.1f;

        Profiler::beginUpdate();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp   = hidKeysUp();

        if (kDown & KEY_START) {
            Profiler::endUpdate();
            break;
        }

        // Toggle on-screen performance profiler via [SELECT]
        if (kDown & KEY_SELECT) {
            Profiler::toggleOverlay();
        }

        Hero* hero = Dungeon::hero.get();
        Level* level = Dungeon::level.get();

        // ---------------------------------------------------------------------
        // 1. Bottom Screen UI & Hardware Buttons [A, B, X, Y, L, R]
        // ---------------------------------------------------------------------
        // Resting interrupt on directional / input actions
        if (hero && hero->restoreHealth) {
            if ((kDown & ~KEY_A) || (kHeld & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT))) {
                hero->interrupt();
            }
        }

        bool actionConsumed = bottomUI.update(hero, level, kDown, kHeld, kUp);
        if (actionConsumed) {
            Dungeon::mobs.erase(
                std::remove_if(Dungeon::mobs.begin(), Dungeon::mobs.end(),
                               [](const std::unique_ptr<Mob>& m) { return !m || !m->isAlive(); }),
                Dungeon::mobs.end()
            );
            FX::evolveBlobs(level);
        } else if (hero && hero->restoreHealth) {
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
                    sndHit.play(0.8f, 0.0f);
                    SaveManager::deleteSave();
                    GLog::n("HERO PERISHED! Save erased.");
                }
            }
        }

        // ---------------------------------------------------------------------
        // 2. D-pad & Circle Pad Movement (Supports 8-directional analog movement)
        // ---------------------------------------------------------------------
        if (!actionConsumed && !bottomUI.isInventoryOpen() && hero && hero->isAlive() && !hero->restoreHealth) {
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
                    if (targetEnemy) {
                        sndHit.play(0.6f, 0.0f);
                        int dealt = prevTargetHp - targetEnemy->HP;
                        if (targetEnemy->isAlive()) {
                            GLog::i("Hero hit " + targetEnemy->name + " for " + std::to_string(dealt) + " dmg!");
                        } else {
                            GLog::p("Hero defeated " + targetEnemy->name + "!");
                        }
                    } else {
                        sndStep.play(0.25f, 0.0f);
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
                        sndHit.play(0.8f, 0.0f);
                        SaveManager::deleteSave();
                        GLog::n("HERO PERISHED! Save erased.");
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
            topCamera.setCenter(cx + (hx - cx) * 0.2f, cy + (hy - cy) * 0.2f);
        }
        topCamera.update(dt);
        FX::update(dt);
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

                        if (!level->visited[cellIdx]) continue;
                        drawnTiles++;

                        bool inFov = level->fieldOfView[cellIdx];
                        tileImg.x = x * 16.0f;
                        tileImg.y = y * 16.0f;
                        tileImg.z = 0.2f;

                        int t = level->get(cellIdx);
                        if (t == Terrain::WALL || t == Terrain::WALL_DECO) tileImg.setFrame(fWall);
                        else if (t == Terrain::DOOR || t == Terrain::SECRET_DOOR) tileImg.setFrame(fDoor);
                        else if (t == Terrain::EXIT) tileImg.setFrame(fStairs);
                        else if (t == Terrain::WATER) tileImg.setFrame(fWater);
                        else tileImg.setFrame(fFloor);

                        tileImg.tint = inFov ? Color(255, 255, 255, 255) : Color(100, 100, 115, 255);
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
                        bool inFov = Dungeon::visible[heap->pos];
                        charImg.x = (heap->pos % Level::WIDTH) * 16.0f;
                        charImg.y = (heap->pos / Level::WIDTH) * 16.0f;
                        charImg.z = 0.25f;
                        charImg.tint = inFov ? Color(255, 255, 255, 255) : Color(100, 100, 115, 255);
                        charImg.setFilm(itemsFilm, heap->image());
                        charImg.draw(&topCamera);
                    }
                }
            }

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

            // Render Core Visual Effects & Particles
            FX::draw(&topCamera, textBuf);
        }

        // Minimalist Top Screen HUD Header
        C2D_DrawRectSolid(0.0f, 0.0f, 0.8f, 400.0f, 18.0f, C2D_Color32(0x18, 0x14, 0x25, 0xDC));
        std::snprintf(strBuf, sizeof(strBuf), "DEPTH %d  |  FOV Active  |  SELECT: Stats  |  Y: Items  |  A: Rest",
                      Dungeon::depth);
        C2D_TextParse(&textObj, textBuf, strBuf);
        C2D_TextOptimize(&textObj);
        C2D_DrawText(&textObj, C2D_WithColor, 8.0f, 2.0f, 0.85f, 0.40f, 0.40f, C2D_Color32(0xe4, 0xa6, 0x72, 0xFF));

        // Real-time Performance Profiling Overlay (toggled via [SELECT])
        if (Profiler::isOverlayEnabled()) {
            C2D_DrawRectSolid(0.0f, 18.0f, 0.95f, 400.0f, 16.0f, C2D_Color32(0x0a, 0x0a, 0x14, 0xEE));
            char profBuf[128];
            std::snprintf(profBuf, sizeof(profBuf), "FPS: %.1f | Frame: %.2fms (Upd: %.2fms, Ren: %.2fms) | Culled: %lu/%d",
                          Profiler::getFps(), Profiler::getFrameTimeMs(),
                          Profiler::getUpdateTimeMs(), Profiler::getRenderTimeMs(),
                          static_cast<unsigned long>(Profiler::getTilesCulled()), Level::LENGTH);
            C2D_Text profText;
            C2D_TextParse(&profText, textBuf, profBuf);
            C2D_TextOptimize(&profText);
            C2D_DrawText(&profText, C2D_WithColor, 6.0f, 19.0f, 0.96f, 0.38f, 0.38f, C2D_Color32(0x40, 0xf0, 0x60, 0xFF));
        }

        // =====================================================================
        // BOTTOM SCREEN (320x240): Tactical HUD, Quickslots, Inventory & Log
        // =====================================================================
        C2D_TargetClear(bottomTarget, C2D_Color32(0x22, 0x20, 0x34, 0xFF));
        C2D_SceneBegin(bottomTarget);

        bottomUI.draw(hero, Dungeon::depth, textBuf);

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
