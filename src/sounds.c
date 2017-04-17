// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  sounds.c
/// \brief music/sound tables, and related sound routines

#include "doomtype.h"
#include "i_sound.h"
#include "sounds.h"
#include "r_defs.h"
#include "r_things.h"
#include "z_zone.h"
#include "w_wad.h"
#include "lua_script.h"

//
// Information about all the sfx
//

sfxinfo_t S_sfx[NUMSFX] =
{

/*****
	Legacy doesn't use the PITCH variable, so now it is used for
	various flags. See soundflags_t.
*****/
  // S_sfx[0] needs to be a dummy for odd reasons. (don't modify this comment)
//  name, singularity, priority, pitch, volume, data, length, skinsound, usefulness, lumpnum, caption
  {"none" ,  false,   0,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}, // maximum length

  // Skin Sounds
  {"altdi1", false, 192, 16, -1, NULL, 0, SKSPLDET1,  -1, LUMPERROR, "Dying"},
  {"altdi2", false, 192, 16, -1, NULL, 0, SKSPLDET2,  -1, LUMPERROR, "Dying"},
  {"altdi3", false, 192, 16, -1, NULL, 0, SKSPLDET3,  -1, LUMPERROR, "Dying"},
  {"altdi4", false, 192, 16, -1, NULL, 0, SKSPLDET4,  -1, LUMPERROR, "Dying"},
  {"altow1", false, 192, 16, -1, NULL, 0, SKSPLPAN1,  -1, LUMPERROR, "Spilled rings"},
  {"altow2", false, 192, 16, -1, NULL, 0, SKSPLPAN2,  -1, LUMPERROR, "Spilled rings"},
  {"altow3", false, 192, 16, -1, NULL, 0, SKSPLPAN3,  -1, LUMPERROR, "Spilled rings"},
  {"altow4", false, 192, 16, -1, NULL, 0, SKSPLPAN4,  -1, LUMPERROR, "Spilled rings"},
  {"victr1", false,  64, 16, -1, NULL, 0, SKSPLVCT1,  -1, LUMPERROR, "/"},
  {"victr2", false,  64, 16, -1, NULL, 0, SKSPLVCT2,  -1, LUMPERROR, "/"},
  {"victr3", false,  64, 16, -1, NULL, 0, SKSPLVCT3,  -1, LUMPERROR, "/"},
  {"victr4", false,  64, 16, -1, NULL, 0, SKSPLVCT4,  -1, LUMPERROR, "/"},
  {"gasp" ,  false,  64,  0, -1, NULL, 0,   SKSGASP,  -1, LUMPERROR, "Bubble gasp"},
  {"jump" ,  false, 140,  0, -1, NULL, 0,   SKSJUMP,  -1, LUMPERROR, "Jump"},
  {"pudpud", false,  64,  0, -1, NULL, 0, SKSPUDPUD,  -1, LUMPERROR, "Tired flight"},
  {"putput", false,  64,  0, -1, NULL, 0, SKSPUTPUT,  -1, LUMPERROR, "Flight"}, // not as high a priority
  {"spin" ,  false, 100,  0, -1, NULL, 0,   SKSSPIN,  -1, LUMPERROR, "Spin"},
  {"spndsh", false,  64,  1, -1, NULL, 0, SKSSPNDSH,  -1, LUMPERROR, "Spindash charge"},
  {"thok" ,  false,  96,  0, -1, NULL, 0,   SKSTHOK,  -1, LUMPERROR, "Thok"},
  {"zoom" ,  false, 120,  1, -1, NULL, 0,   SKSZOOM,  -1, LUMPERROR, "Spindash launch"},
  {"skid",   false,  64, 32, -1, NULL, 0,   SKSSKID,  -1, LUMPERROR, "Skid"},

  // Ambience/background objects/etc
  {"ambint",  true,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Obnoxious disco music"},

  {"alarm",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Alarm"},
  {"buzz1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz4",  false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Buzz"},
  {"crumbl",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crumbling"}, // Platform Crumble Tails 03-16-2001
  {"fire",   false,   8, 32, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"grind",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic grinding"},
  {"laser",   true,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser hum"},
  {"mswing", false,  16,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging mace"},
  {"pstart", false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "/"},
  {"pstop",  false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"steam1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Steam jet"}, // Tails 06-19-2001
  {"steam2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Steam jet"}, // Tails 06-19-2001
  {"wbreak", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wood breaking"},

  {"rainin",  true,  24,  4, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rain"},
  {"litng1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng3", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng4", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"athun1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder"},
  {"athun2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder"},

  {"amwtr1", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr2", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr3", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr4", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr5", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr6", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr7", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"amwtr8", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running water"},
  {"bubbl1", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Small bubble"},
  {"bubbl2", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Small bubble"},
  {"bubbl3", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Small bubble"},
  {"bubbl4", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Small bubble"},
  {"bubbl5", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Small bubble"},
  {"floush", false,  16,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shallow water"},
  {"splash", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water splash"},
  {"splish", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water splash"}, // Splish Tails 12-08-2000
  {"wdrip1", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip2", false,   8 , 0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip3", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip4", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip5", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip6", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip7", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wdrip8", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dripping water"},
  {"wslap",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water slap"}, // Water Slap Tails 12-13-2000

  {"doora1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding open"},
  {"doorb1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding open"},
  {"doorc1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wooden door opening"},
  {"doorc2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Slamming shut"},
  {"doord1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Creaking open"},
  {"doord2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Slamming shut"},
  {"eleva1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starting elevator"},
  {"eleva2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving elevator"},
  {"eleva3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stopping elevator"},
  {"elevb1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starting elevator"},
  {"elevb2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving elevator"},
  {"elevb3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stopping elevator"},

  {"ambin2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Natural vibrations"},
  {"lavbub", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubbling lava"},
  {"rocks1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rocks2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rocks3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rocks4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rocks"},
  {"rumbam", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Violent rumbling"},
  {"rumble", false,  64, 24, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},

  // Game objects, etc
  {"appear", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Appearing platform"},
  {"bkpoof", false,  70,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Armageddon explosion"},
  {"bnce1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"}, // Boing!
  {"bnce2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ricochet"}, // Boing!
  {"cannon", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful shot"},
  {"cgot" ,   true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Chaos Emerald"}, // Got Emerald! Tails 09-02-2001
  {"cybdth", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"deton",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous beeping"},
  {"ding",   false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ding"},
  {"dmpain", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Machine damage"},
  {"drown",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"fizzle", false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric fizzle"},
  {"gbeep",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous beeping"}, // Grenade beep
  {"wepfir", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Weapon firing"}, // defaults to thok
  {"ghit" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goo splash"},
  {"gloop",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water splash"},
  {"gspray", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goop sling"},
  {"gravch", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Recycler"},
  {"itemup",  true, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"},
  {"jet",    false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying jet"},
  {"jshard",  true, 167,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got shard"},
  {"lose" ,  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failure"},
  {"lvpass", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning signpost"},
  {"mindig", false,   8, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tunnelling"},
  {"mixup",   true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Teleport"},
  {"monton", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Golden Monitor activated"},
  {"pogo" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical pogo"},
  {"pop"  ,  false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"rail1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rail weapon fired"},
  {"rail2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rail weapon crash"},
  {"rlaunc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Missile fired"},
  {"shield", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pity Shield"}, // generic GET!
  {"wirlsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whirlwind Shield"}, // Whirlwind GET!
  {"forcsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Force Shield"}, // Force GET!
  {"elemsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Elemental Shield"}, // Elemental GET!
  {"armasg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Armageddon Shield"}, // Armaggeddon GET!
  {"attrsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction Shield"}, // Attract GET!
  {"shldls", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shield loss"}, // You LOSE!
  {"spdpad", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Speedpad"},
  {"spkdth", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spiked"},
  {"spring", false, 112,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"statu1",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pushing a statue"},
  {"statu2",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pushing a statue"},
  {"strpst",  true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starpost"}, // Starpost Sound Tails 07-04-2002
  {"supert",  true, 127,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Super transformation"},
  {"telept", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dash"},
  {"tink" ,  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tink"},
  {"token" ,  true, 224,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Token"}, // SS token
  {"trfire",  true,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser fired"},
  {"trpowr",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Charging"},
  {"turhit", false,  40,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser hit"},
  {"wdjump", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whirlwind jump"},
  {"mswarp", false,  60, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning out"},
  {"mspogo", false,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crunch"},
  {"boingf", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bouncing"},
  {"corkp",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cork fired"},
  {"corkh",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cork hit"},

  // Menu, interface
  {"chchng", false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score tally done"},
  {"dwnind", false, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"},
  {"emfind", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Radar beep"},
  {"flgcap", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flag capture"},
  {"menu1",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Menu beep"},
  {"oneup",   true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Extra life"},
  {"ptally",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score tally"}, // Point tally is identical to menu for now
  {"radio",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Chatter"},
  {"wepchg",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Weapon change"}, // Weapon switch is identical to menu for now
  {"wtrdng",  true, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thinking about air"}, // make sure you can hear the DING DING! Tails 03-08-2000
  {"zelda",  false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"},

  // NiGHTS
  {"ideya",  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Finishing the stage"},
  {"xideya", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Finishing the stage"}, // Xmas
  {"nbmper", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},
  {"nxbump", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"}, // Xmas
  {"ncitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got logo"},
  {"nxitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got logo"}, // Xmas
  {"ngdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonus time start"},
  {"nxdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonus time start"}, // Xmas
  {"drill1", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drill start"},
  {"drill2", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drill"},
  {"ncspec", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up"}, // Tails 12-15-2003
  {"nghurt", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"ngskid", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Force stop"},
  {"hoop1",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop"},
  {"hoop2",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop+"},
  {"hoop3",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop++"},
  {"hidden", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up discovery"},
  {"prloop", false, 104,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Paraloop"},
  {"timeup",  true, 256,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Countdown"},

  // Mario
  {"koopfr" , true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire"},
  {"mario1", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hitting a ceiling"},
  {"mario2", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Koopa shell"},
  {"mario3", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up"},
  {"mario4",  true,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got coin"},
  {"mario5", false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goomba stomp"},
  {"mario6", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"mario7", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire"},
  {"mario8", false,  48,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"mario9",  true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collecting power-up"},
  {"marioa",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Extra life"},
  {"thwomp",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thwomp stomp"},

  // Black Eggman
  {"bebomb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bomb explosion"},
  {"bechrg", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Charging"},
  {"becrsh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crash"},
  {"bedeen", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic crash"},
  {"bedie1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"bedie2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"beeyow", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"befall", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic slam"},
  {"befire", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing goop"},
  {"beflap", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical jump"},
  {"begoop", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful shot"},
  {"begrnd", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic hit"},
  {"behurt", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic crash"},
  {"bejet1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jetpack charge"},
  {"belnch", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical jump"},
  {"beoutb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Empty chamber"},
  {"beragh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman screaming"},
  {"beshot", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing missile"},
  {"bestep", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical stomp"},
  {"bestp2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical stomp"},
  {"bewar1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bexpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Missile explosion"},
  {"bgxpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},

  // Cybrakdemon
  {"beelec", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electricity"},
  {"brakrl", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rocket launch"},
  {"brakrx", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rocket explosion"},

  // S3&K sounds
  {"s3k33",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"}, // stereo in original game, identical to latter
  {"s3k34",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"}, // mono in original game, identical to previous
  {"s3k35",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"s3k36",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Skid"},
  {"s3k37",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spiked"},
  {"s3k38",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble gasp"},
  {"s3k39",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water splash"},
  {"s3k3a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k3b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"s3k3c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning"},
  {"s3k3d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"s3k3e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire Shield"},
  {"s3k3f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water Shield"},
  {"s3k40",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction zoom"},
  {"s3k41",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning shield"},
  {"s3k42",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Twinspin"},
  {"s3k43",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire burst"},
  {"s3k44",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k45",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning zap"},
  {"s3k46",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Super transformation"},
  {"s3k47",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising sand"},
  {"s3k48",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic clink"},
  {"s3k49",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling rock"},
  {"s3k4a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Grab"},
  {"s3k4b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water splash"},
  {"s3k4c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy landing"},
  {"s3k4d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing bullet"},
  {"s3k4e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bomb explosion"},
  {"s3k4f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"s3k50",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Siren"},
  {"s3k51",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling bomb"},
  {"s3k52",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spike"},
  {"s3k53",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Charging"},
  {"s3k54",  false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing"}, // MetalSonic shot fire
  {"s3k55",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving on track"},
  {"s3k56",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k57",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Water"},
  {"s3k58",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical movement"},
  {"s3k59",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collapsing platform"},
  {"s3k5a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"},
  {"s3k5b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Menu beep"},
  {"s3k5c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric spark"},
  {"s3k5d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bouncing missile"},
  {"s3k5e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing laser"},
  {"s3k5f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"s3k60",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying away"},
  {"s3k61",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drilling"},
  {"s3k62",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"s3k63",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starpost"},
  {"s3k64",  false,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"},
  {"s3k65",  false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got blue sphere"}, // Blue Spheres
  {"s3k66",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Special stage clear"},
  {"s3k67",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing missile"},
  {"s3k68",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Super possibilities"},
  {"s3k69",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Switch click"},
  {"s3k6a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Special stage clear"},
  {"s3k6b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k6c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bursting from the ground"},
  {"s3k6d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k6e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical damage"},
  {"s3k6f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3k70",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Seperation"},
  {"s3k71",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k72",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"},
  {"s3k73",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"s3k74",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical clink"},
  {"s3k75",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"},
  {"s3k76",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3k77",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Balloon pop"},
  {"s3k78",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnet"},
  {"s3k79",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric charge"},
  {"s3k7a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising from lava"},
  {"s3k7b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper bounce"},
  {"s3k7c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnet"},
  {"s3k7d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k7e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eating dirt"},
  {"s3k7f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Freeze"},
  {"s3k80",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ice spike burst"},
  {"s3k81",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing large object"},
  {"s3k82",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bursting from the ground"},
  {"s3k83",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collapsing bridge"},
  {"s3k84",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k85",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k86",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Alarm"},
  {"s3k87",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k88",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic clink"},
  {"s3k89",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sufficiently advanced tech."},
  {"s3k8a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Boing"},
  {"s3k8b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Chop"},
  {"s3k8c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k8d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k8e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k8f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Opening"},
  {"s3k90",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Closing"},
  {"s3k91",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Closed"},
  {"s3k92",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ghost"},
  {"s3k93",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rebuilding"},
  {"s3k94",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spike"},
  {"s3k95",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising from lava"},
  {"s3k96",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling"},
  {"s3k97",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind"},
  {"s3k98",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling spike"},
  {"s3k99",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k9a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k9b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"s3k9c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Super Emerald"},
  {"s3k9d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Targetting"},
  {"s3k9e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic clink"},
  {"s3k9f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Super transformation"},
  {"s3ka0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"s3ka1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3ka2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"s3ka3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lift"},
  {"s3ka4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Robotic power up"},
  {"s3ka5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3ka6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3ka7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Countdown beep"},
  {"s3ka8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"},
  {"s3ka9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"},
  {"s3kaa",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper bounce"},
  {"s3kab",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash charge"},
  {"s3kac",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Continue"},
  {"s3kad",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "GO!"},
  {"s3kae",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pinball flipper"},
  {"s3kaf",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "To Special Stage"},
  {"s3kb0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score tally done"},
  {"s3kb1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"s3kb2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failure"},
  {"s3kb3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Big ring warp"},
  {"s3kb4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"s3kb5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kb6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash launch"},
  {"s3kb7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rolling tumbler"},
  {"s3kb8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling signpost"},
  {"s3kb9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spilled rings"},
  {"s3kba",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flight"},
  {"s3kbb",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tired flight"},
  {"s3kbcs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kbcl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // long version of previous
  {"s3kbds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying fortress"},
  {"s3kbdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying fortress"}, // ditto
  {"s3kbes", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying away"},
  {"s3kbel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying away"}, // ditto
  {"s3kbfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"},
  {"s3kbfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"}, // ditto
  {"s3kc0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"},
  {"s3kc0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turbine"}, // ditto
  {"s3kc1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fan"},
  {"s3kc1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fan"}, // ditto
  {"s3kc2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"s3kc2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"}, // ditto
  {"s3kc3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Levitation"},
  {"s3kc3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Levitation"}, // ditto
  {"s3kc4s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing laser"},
  {"s3kc4l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing laser"}, // ditto
  {"s3kc5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kc5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kc6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Orbiting"},
  {"s3kc6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Orbiting"}, // ditto
  {"s3kc7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"},
  {"s3kc8s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding"},
  {"s3kc8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding"}, // ditto
  {"s3kc9s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging"},
  {"s3kc9l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging"}, // ditto
  {"s3kcas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"},
  {"s3kcal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"}, // ditto
  {"s3kcbs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kcbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kccs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collapsing"},
  {"s3kccl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collapsing"}, // ditto
  {"s3kcds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3kcdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"}, // ditto
  {"s3kces", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind tunnel"},
  {"s3kcel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind tunnel"}, // ditto
  {"s3kcfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kcfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"},
  {"s3kd0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"}, // ditto
  {"s3kd1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kd1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turning"},
  {"s3kd2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turning"}, // ditto
  {"s3kd3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kd3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd4s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Engine"},
  {"s3kd4l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Engine"}, // ditto
  {"s3kd5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling lava"},
  {"s3kd5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling lava"}, // ditto
  {"s3kd6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kd6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd7s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"},
  {"s3kd7l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"}, // ditto
  {"s3kd8s", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Acceleration"}, // Sharp Spin (maybe use the long/L version?)
  {"s3kd8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Acceleration"}, // ditto
  {"s3kd9s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnetism"},
  {"s3kd9l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnetism"}, // ditto
  {"s3kdas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3kdal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"}, // ditto
  {"s3kdbs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running on water"},
  {"s3kdbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Running on water"}, // ditto

  // skin sounds free slots to add sounds at run time (Boris HACK!!!)
  // initialized to NULL
};

char freeslotnames[sfx_freeslot0 + NUMSFXFREESLOTS + NUMSKINSFXSLOTS][7];

// Prepare free sfx slots to add sfx at run time
void S_InitRuntimeSounds (void)
{
	sfxenum_t i;
	INT32 value;
	char soundname[7];

	for (i = sfx_freeslot0; i <= sfx_lastskinsoundslot; i++)
	{
		value = (i+1) - sfx_freeslot0;

		if (value < 10)
			sprintf(soundname, "fre00%d", value);
		else if (value < 100)
			sprintf(soundname, "fre0%d", value);
		else if (value < 1000)
			sprintf(soundname, "fre%d", value);
		else
			sprintf(soundname, "fr%d", value);

		strcpy(freeslotnames[value-1], soundname);

		S_sfx[i].name = freeslotnames[value-1];
		S_sfx[i].singularity = false;
		S_sfx[i].priority = 0;
		S_sfx[i].pitch = 0;
		S_sfx[i].volume = -1;
		S_sfx[i].data = NULL;
		S_sfx[i].length = 0;
		S_sfx[i].skinsound = -1;
		S_sfx[i].usefulness = -1;
		S_sfx[i].lumpnum = LUMPERROR;
		strlcpy(S_sfx[i].caption, "", 9);
	}
}

// Add a new sound fx into a free sfx slot.
//
sfxenum_t S_AddSoundFx(const char *name, boolean singular, INT32 flags, boolean skinsound)
{
	sfxenum_t i, slot;

	if (skinsound)
		slot = sfx_skinsoundslot0;
	else
		slot = sfx_freeslot0;

	for (i = slot; i < NUMSFX; i++)
	{
		if (!S_sfx[i].priority)
		{
			strncpy(freeslotnames[i-sfx_freeslot0], name, 6);
			S_sfx[i].singularity = singular;
			S_sfx[i].priority = 60;
			S_sfx[i].pitch = flags;
			S_sfx[i].volume = -1;
			S_sfx[i].lumpnum = LUMPERROR;
			S_sfx[i].skinsound = -1;
			S_sfx[i].usefulness = -1;

			/// \todo if precached load it here
			S_sfx[i].data = NULL;
			return i;
		}
	}
	CONS_Alert(CONS_WARNING, M_GetText("No more free sound slots\n"));
	return 0;
}

void S_RemoveSoundFx(sfxenum_t id)
{
	if (id >= sfx_freeslot0 && id <= sfx_lastskinsoundslot
		&& S_sfx[id].priority != 0)
	{
		S_sfx[id].lumpnum = LUMPERROR;
		I_FreeSfx(&S_sfx[id]);
		S_sfx[id].priority = 0;
	}
}
