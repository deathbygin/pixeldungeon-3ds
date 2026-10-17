#pragma once

namespace pd {

class Statistics {
public:
    static int goldCollected;
    static int deepestFloor;
    static int enemiesSlain;
    static int foodEaten;
    static int potionsCooked;
    static int piranhasKilled;
    static int nightHunt;
    static int ankhsUsed;
    static float duration;
    static bool qualifiedForNoKilling;
    static bool completedWithNoKilling;
    static bool amuletObtained;

    static void reset();
};

} // namespace pd

