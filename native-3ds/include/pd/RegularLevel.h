#pragma once

#include "pd/Level.h"
#include "pd/Room.h"
#include <vector>
#include <memory>

namespace pd {

class RegularLevel : public Level {
public:
    std::shared_ptr<Room> roomEntrance;
    std::shared_ptr<Room> roomExit;

    int minRoomSize = 7;
    int maxRoomSize = 9;

    RegularLevel() = default;
    ~RegularLevel() override = default;

    bool build() override;

protected:
    bool initRooms();
    void split(const Rect& rect);
    virtual void assignRoomType();
    void paint();
    void placeDoors(Room* r);
    void paintDoors(Room* r);
    bool joinRooms(Room* r, Room* n);
    Room* randomRoom(Room::Type type, int tries);
    void placeTraps();
    void createItems();
};

} // namespace pd
