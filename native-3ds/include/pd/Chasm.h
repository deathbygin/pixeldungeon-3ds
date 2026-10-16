#pragma once

namespace pd {

class Hero;
class Mob;

class Chasm {
public:
    static bool jumpConfirmed;
    static void heroFall(int pos);
    static void heroLand();
    static void mobFall(Mob* mob);
};

} // namespace pd

