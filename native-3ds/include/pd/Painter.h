#pragma once

#include "pd/Rect.h"
#include "pd/Room.h"

namespace pd {

class Level;

class Painter {
public:
    static void set(Level* level, int cell, int value);
    static void set(Level* level, int x, int y, int value);
    static void set(Level* level, Point p, int value);

    static void fill(Level* level, int x, int y, int w, int h, int value);
    static void fill(Level* level, const Rect& rect, int value);
    static void fill(Level* level, const Rect& rect, int m, int value);
    static void fill(Level* level, const Rect& rect, int l, int t, int r, int b, int value);

    static Point drawInside(Level* level, const Room& room, Point from, int n, int value);

    static void paintRoom(Level* level, Room* room);
    static void paintStandard(Level* level, Room* room);
    static void paintTunnel(Level* level, Room* room);
    static void paintPassage(Level* level, Room* room);
    static void paintEntrance(Level* level, Room* room);
    static void paintExit(Level* level, Room* room);
    static void paintBossExit(Level* level, Room* room);
    static void paintRatKing(Level* level, Room* room);
    static void paintArmory(Level* level, Room* room);
    static void paintLaboratory(Level* level, Room* room);
    static void paintLibrary(Level* level, Room* room);
    static void paintGarden(Level* level, Room* room);
    static void paintMagicWell(Level* level, Room* room);
    static void paintPool(Level* level, Room* room);
    static void paintPit(Level* level, Room* room);
    static void paintStorage(Level* level, Room* room);
    static void paintStatue(Level* level, Room* room);
    static void paintTraps(Level* level, Room* room);
    static void paintTreasury(Level* level, Room* room);
    static void paintVault(Level* level, Room* room);
    static void paintWeakFloor(Level* level, Room* room);
    static void paintAltar(Level* level, Room* room);
    static void paintCrypt(Level* level, Room* room);
    static void paintShop(Level* level, Room* room);

private:
    static void paintBurned(Level* level, Room* room);
    static void paintGraveyard(Level* level, Room* room);
    static void paintStriped(Level* level, Room* room);
    static void paintStudy(Level* level, Room* room);
    static void paintBridge(Level* level, Room* room);
    static void paintFissure(Level* level, Room* room);

    static int xy2p(const Room& room, const Point& xy, int pasWidth, int pasHeight);
    static Point p2xy(const Room& room, int p, int pasWidth, int pasHeight);
};

} // namespace pd

