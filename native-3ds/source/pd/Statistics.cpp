#include "pd/Statistics.h"

namespace pd {

int Statistics::goldCollected = 0;
int Statistics::deepestFloor = 0;
int Statistics::enemiesSlain = 0;
int Statistics::foodEaten = 0;
int Statistics::potionsCooked = 0;
int Statistics::piranhasKilled = 0;
int Statistics::nightHunt = 0;
int Statistics::ankhsUsed = 0;
float Statistics::duration = 0.0f;
bool Statistics::qualifiedForNoKilling = false;
bool Statistics::completedWithNoKilling = false;
bool Statistics::amuletObtained = false;

void Statistics::reset() {
    goldCollected = 0;
    deepestFloor = 0;
    enemiesSlain = 0;
    foodEaten = 0;
    potionsCooked = 0;
    piranhasKilled = 0;
    nightHunt = 0;
    ankhsUsed = 0;
    duration = 0.0f;
    qualifiedForNoKilling = false;
    completedWithNoKilling = false;
    amuletObtained = false;
}

} // namespace pd

