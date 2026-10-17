#pragma once

#include <string>

namespace pd {

class Assets {
public:
    // User Interface & HUD
    inline static const std::string ARCS_BG       = "romfs:/arcs1.png";
    inline static const std::string ARCS_FG       = "romfs:/arcs2.png";
    inline static const std::string DASHBOARD     = "romfs:/dashboard.png";
    inline static const std::string BANNERS       = "romfs:/banners.png";
    inline static const std::string BADGES        = "romfs:/badges.png";
    inline static const std::string LOCKED        = "romfs:/locked_badge.png";
    inline static const std::string AMULET        = "romfs:/amulet.png";
    inline static const std::string CHROME        = "romfs:/chrome.png";
    inline static const std::string ICONS         = "romfs:/icons.png";
    inline static const std::string STATUS        = "romfs:/status_pane.png";
    inline static const std::string HP_BAR        = "romfs:/hp_bar.png";
    inline static const std::string XP_BAR        = "romfs:/exp_bar.png";
    inline static const std::string TOOLBAR       = "romfs:/toolbar.png";
    inline static const std::string SHADOW        = "romfs:/shadow.png";
    inline static const std::string FONTS1X       = "romfs:/font1x.png";
    inline static const std::string FONTS15X      = "romfs:/font15x.png";
    inline static const std::string FONTS2X       = "romfs:/font2x.png";
    inline static const std::string FONTS25X      = "romfs:/font25x.png";
    inline static const std::string FONTS3X       = "romfs:/font3x.png";
    inline static const std::string FONTS1X_INK   = "romfs:/font1x_ink.png";
    inline static const std::string FONTS2X_INK   = "romfs:/font2x_ink.png";

    // Hero Spritesheets (12x15 per frame)
    inline static const std::string WARRIOR       = "romfs:/warrior.png";
    inline static const std::string MAGE          = "romfs:/mage.png";
    inline static const std::string ROGUE         = "romfs:/rogue.png";
    inline static const std::string HUNTRESS      = "romfs:/ranger.png";
    inline static const std::string AVATARS       = "romfs:/avatars.png";

    // Common Mobs
    inline static const std::string RAT           = "romfs:/rat.png";
    inline static const std::string GNOLL         = "romfs:/gnoll.png";
    inline static const std::string CRAB          = "romfs:/crab.png";
    inline static const std::string GOO           = "romfs:/goo.png";
    inline static const std::string SWARM         = "romfs:/swarm.png";
    inline static const std::string SKELETON      = "romfs:/skeleton.png";
    inline static const std::string SHAMAN        = "romfs:/shaman.png";
    inline static const std::string THIEF         = "romfs:/thief.png";
    inline static const std::string TENGU         = "romfs:/tengu.png";
    inline static const std::string DM300         = "romfs:/dm300.png";
    inline static const std::string WRAITH        = "romfs:/wraith.png";
    inline static const std::string ELEMENTAL     = "romfs:/elemental.png";
    inline static const std::string BAT           = "romfs:/bat.png";
    inline static const std::string BRUTE         = "romfs:/brute.png";
    inline static const std::string MONK          = "romfs:/monk.png";
    inline static const std::string WARLOCK       = "romfs:/warlock.png";
    inline static const std::string GOLEM         = "romfs:/golem.png";
    inline static const std::string UNDEAD        = "romfs:/undead.png";
    inline static const std::string KING          = "romfs:/king.png";
    inline static const std::string STATUE        = "romfs:/statue.png";
    inline static const std::string PIRANHA       = "romfs:/piranha.png";
    inline static const std::string EYE           = "romfs:/eye.png";
    inline static const std::string SUCCUBUS      = "romfs:/succubus.png";
    inline static const std::string SCORPIO       = "romfs:/scorpio.png";
    inline static const std::string YOG           = "romfs:/yog.png";
    inline static const std::string SHEEP         = "romfs:/sheep.png";
    inline static const std::string MIMIC         = "romfs:/mimic.png";

    // Items & Environmental Objects
    inline static const std::string ITEMS         = "romfs:/items.png";
    inline static const std::string PLANTS        = "romfs:/plants.png";

    // Dungeon Chapter Tilesheets (256x64, 16x16 tiles)
    inline static const std::string TILES_SEWERS  = "romfs:/tiles0.png"; // Depths 1-5
    inline static const std::string TILES_PRISON  = "romfs:/tiles1.png"; // Depths 6-10
    inline static const std::string TILES_CAVES   = "romfs:/tiles2.png"; // Depths 11-15
    inline static const std::string TILES_CITY    = "romfs:/tiles3.png"; // Depths 16-20
    inline static const std::string TILES_HALLS   = "romfs:/tiles4.png"; // Depths 21-25

    // Animated Water Sheets
    inline static const std::string WATER_SEWERS  = "romfs:/water0.png";
    inline static const std::string WATER_PRISON  = "romfs:/water1.png";
    inline static const std::string WATER_CAVES   = "romfs:/water2.png";
    inline static const std::string WATER_CITY    = "romfs:/water3.png";
    inline static const std::string WATER_HALLS   = "romfs:/water4.png";

    // Visual Effects & Buff Icons
    inline static const std::string FIREBALL      = "romfs:/fireball.png";
    inline static const std::string SPECKS        = "romfs:/specks.png";
    inline static const std::string EFFECTS       = "romfs:/effects.png";
    inline static const std::string BUFFS_SMALL   = "romfs:/buffs.png";
    inline static const std::string BUFFS_LARGE   = "romfs:/large_buffs.png";
    inline static const std::string SPELL_ICONS   = "romfs:/spell_icons.png";

    // Audio SFX Paths
    inline static const std::string SND_CLICK     = "romfs:/snd_click.mp3";
    inline static const std::string SND_BADGE     = "romfs:/snd_badge.mp3";
    inline static const std::string SND_GOLD      = "romfs:/snd_gold.mp3";
    inline static const std::string SND_OPEN      = "romfs:/snd_door_open.mp3";
    inline static const std::string SND_UNLOCK    = "romfs:/snd_unlock.mp3";
    inline static const std::string SND_ITEM      = "romfs:/snd_item.mp3";
    inline static const std::string SND_HIT       = "romfs:/snd_hit.mp3";
    inline static const std::string SND_MISS      = "romfs:/snd_miss.mp3";
    inline static const std::string SND_STEP      = "romfs:/snd_step.mp3";
    inline static const std::string SND_WATER     = "romfs:/snd_water.mp3";
    inline static const std::string SND_DESCEND   = "romfs:/snd_descend.mp3";
    inline static const std::string SND_EAT       = "romfs:/snd_eat.mp3";
    inline static const std::string SND_READ      = "romfs:/snd_read.mp3";
    inline static const std::string SND_DRINK     = "romfs:/snd_drink.mp3";
    inline static const std::string SND_SHATTER   = "romfs:/snd_shatter.mp3";
    inline static const std::string SND_ZAP       = "romfs:/snd_zap.mp3";
    inline static const std::string SND_LIGHTNING = "romfs:/snd_lightning.mp3";
    inline static const std::string SND_LEVELUP   = "romfs:/snd_levelup.mp3";
    inline static const std::string SND_DEATH     = "romfs:/snd_death.mp3";
    inline static const std::string SND_TRAP      = "romfs:/snd_trap.mp3";
    inline static const std::string SND_ALERT     = "romfs:/snd_alert.mp3";
    inline static const std::string SND_FALLING   = "romfs:/snd_falling.mp3";
    inline static const std::string SND_SECRET    = "romfs:/snd_secret.mp3";
    inline static const std::string SND_BONES     = "romfs:/snd_bones.mp3";
    inline static const std::string SND_TOMB      = "romfs:/snd_tomb.mp3";
    inline static const std::string SND_CURSED    = "romfs:/snd_cursed.mp3";
    inline static const std::string SND_DEWDROP   = "romfs:/snd_dewdrop.mp3";
    inline static const std::string SND_BLAST     = "romfs:/snd_blast.mp3";
    inline static const std::string SND_ROCKS     = "romfs:/snd_rocks.mp3";
    inline static const std::string SND_BURNING   = "romfs:/snd_burning.mp3";
    inline static const std::string SND_TELEPORT  = "romfs:/snd_teleport.mp3";
    inline static const std::string SND_PUFF      = "romfs:/snd_puff.mp3";
    inline static const std::string SND_EVOKE     = "romfs:/snd_evoke.mp3";
    inline static const std::string SND_MASTERY   = "romfs:/snd_mastery.mp3";
    inline static const std::string SND_MELD      = "romfs:/snd_meld.mp3";
    inline static const std::string SND_DEGRADE   = "romfs:/snd_degrade.mp3";
    inline static const std::string SND_CHARMS    = "romfs:/snd_charms.mp3";
    inline static const std::string SND_CHALLENGE = "romfs:/snd_challenge.mp3";
    inline static const std::string SND_MIMIC     = "romfs:/snd_mimic.mp3";
    inline static const std::string SND_PLANT     = "romfs:/snd_plant.mp3";
    inline static const std::string SND_RAY       = "romfs:/snd_ray.mp3";
    inline static const std::string SND_LULLABY   = "romfs:/snd_lullaby.mp3";
    inline static const std::string SND_GHOST     = "romfs:/snd_ghost.mp3";
    inline static const std::string SND_BOSS      = "romfs:/snd_boss.mp3";
    inline static const std::string SND_BEACON    = "romfs:/snd_beacon.mp3";
    inline static const std::string SND_BEE       = "romfs:/snd_bee.mp3";

    // Background Music Tracks
    inline static const std::string THEME         = "romfs:/theme.mp3";
    inline static const std::string TUNE          = "romfs:/game.mp3";
    inline static const std::string HAPPY         = "romfs:/surface.mp3";

    // Helper returning depth-appropriate tilesheet
    static const std::string& getTilesetForDepth(int depth) {
        if (depth <= 5)  return TILES_SEWERS;
        if (depth <= 10) return TILES_PRISON;
        if (depth <= 15) return TILES_CAVES;
        if (depth <= 20) return TILES_CITY;
        return TILES_HALLS;
    }

    // Helper returning depth-appropriate water sheet
    static const std::string& getWaterForDepth(int depth) {
        if (depth <= 5)  return WATER_SEWERS;
        if (depth <= 10) return WATER_PRISON;
        if (depth <= 15) return WATER_CAVES;
        if (depth <= 20) return WATER_CITY;
        return WATER_HALLS;
    }
};

} // namespace pd

