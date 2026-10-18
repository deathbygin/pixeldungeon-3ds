#pragma once

#include <3ds.h>
#include <string>
#include <cstdint>

namespace pd {

class SaveManager {
public:
    static constexpr uint32_t SAVE_MAGIC = 0x53334450; // 'PD3S' in little-endian
    static constexpr uint32_t SAVE_VERSION = 7;

    // Initialize APT sleep/suspend hooks for autosaving
    static void init();
    static void exit();

    // Core serialization functions
    static bool saveGame(const std::string& customPath = "");
    static bool loadGame(const std::string& customPath = "");
    static bool hasSave(const std::string& customPath = "");
    static bool deleteSave(const std::string& customPath = "");
    static void wipeAllData();
    static bool getSaveSummary(int& outDepth, int& outLevel, int& outHeroClass, const std::string& customPath = "");

    // Path resolution
    static std::string getSaveDirectory();
    static std::string getSavePath();

    // Check if RomFS was initialized successfully
    static bool isRomfsMounted() { return s_romfsMounted; }
    static bool initRomfs();
    static void exitRomfs();

private:
    static bool ensureDirectory(const std::string& path);
    static void onAptHook(APT_HookType hook, void* param);

    static aptHookCookie s_aptCookie;
    static bool s_aptHookRegistered;
    static bool s_romfsMounted;
};

} // namespace pd

