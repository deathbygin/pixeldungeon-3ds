Pixel Dungeon 3DS
=============

A vibe-coded native port of Pixel Dungeon to Nintendo 3DS hardware.

W.I.P - No binary release yet.

DISCLAIMER: This app is vibe-coded; use it at your own risk and do not distribute this code anywhere that AI-generated code is not allowed.

Traditional roguelike game with pixel-art graphics and simple interface.

Pixel Dungeon on GooglePlay: 
https://play.google.com/store/apps/details?id=com.watabou.pixeldungeon

Official web-site: 
http://pixeldungeon.watabou.ru/

Developer's blog: 
http://pixeldungeon.tumblr.com/

Building a CIA
-------------

With devkitPro/devkitARM installed and `DEVKITPRO` and `DEVKITARM` set, build
the executable and CIA with:

```sh
make -C native-3ds all
make -C native-3ds cia
```

The CIA build also requires `bannertool`, `makerom`, and `3dstool`. `3dstool`
must be at `$(DEVKITPRO)/tools/bin/3dstool` or supplied through the
`THREEDSTOOL` make variable. The installable package is written to
`native-3ds/PixelDungeon.cia`.
