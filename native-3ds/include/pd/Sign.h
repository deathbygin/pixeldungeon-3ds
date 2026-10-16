#pragma once

#include <string>
#include <functional>

namespace pd {

/**
 * Pixel Dungeon Signpost feature (com.watabou.pixeldungeon.levels.features.Sign).
 * Provides depth-specific tactical tips and handles depth 22+ burning sign behavior.
 */
class Sign {
public:
    using MessageCallback = std::function<void(const std::string&)>;

    static void setMessageCallback(MessageCallback cb);
    static void read(int pos);
    static const char* getTip(int depth);
};

} // namespace pd

