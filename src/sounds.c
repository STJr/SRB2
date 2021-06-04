// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
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
#include "r_skins.h"
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
  {"none" ,  false,   0,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "///////////////////////////////"}, // maximum length

  // A HUMBLE REQUEST FROM YOUR FRIENDLY NEIGHBORHOOD toaster!
  //
  // If you see a caption that's just "" (shows the lumpname in-game),
  // and you intend to use the sound associated with it in a mod,
  // PLEASE give it a caption through SOC or Lua.
  //
  // If the first character of the caption is '/', no caption will be
  // produced; only do  this for "unimportant" sounds that aren't used
  // to indicate gameplay.
  //
  // (to whomstever updates the sounds list wiki page for 2.2, please
  // either copy this comment across, or make sure its desire is
  // codified in the initial paragraph of the page.)
  //
  // Closed Captioning may be a niche feature, but it's an important one.
  // Thank you! ^u^

  // Skin Sounds
  {"altdi1", false, 192, 16, -1, NULL, 0, SKSPLDET1,  -1, LUMPERROR, "Dying"},
  {"altdi2", false, 192, 16, -1, NULL, 0, SKSPLDET2,  -1, LUMPERROR, "Dying"},
  {"altdi3", false, 192, 16, -1, NULL, 0, SKSPLDET3,  -1, LUMPERROR, "Dying"},
  {"altdi4", false, 192, 16, -1, NULL, 0, SKSPLDET4,  -1, LUMPERROR, "Dying"},
  {"altow1", false, 192, 16, -1, NULL, 0, SKSPLPAN1,  -1, LUMPERROR, "Ring loss"},
  {"altow2", false, 192, 16, -1, NULL, 0, SKSPLPAN2,  -1, LUMPERROR, "Ring loss"},
  {"altow3", false, 192, 16, -1, NULL, 0, SKSPLPAN3,  -1, LUMPERROR, "Ring loss"},
  {"altow4", false, 192, 16, -1, NULL, 0, SKSPLPAN4,  -1, LUMPERROR, "Ring loss"},
  {"victr1", false,  64, 16, -1, NULL, 0, SKSPLVCT1,  -1, LUMPERROR, "/"},
  {"victr2", false,  64, 16, -1, NULL, 0, SKSPLVCT2,  -1, LUMPERROR, "/"},
  {"victr3", false,  64, 16, -1, NULL, 0, SKSPLVCT3,  -1, LUMPERROR, "/"},
  {"victr4", false,  64, 16, -1, NULL, 0, SKSPLVCT4,  -1, LUMPERROR, "/"},
  {"gasp" ,  false,  64,  0, -1, NULL, 0,   SKSGASP,  -1, LUMPERROR, "Bubble gasp"},
  {"jump" ,  false, 140,  0, -1, NULL, 0,   SKSJUMP,  -1, LUMPERROR, "Jump"},
  {"pudpud", false,  64,  0, -1, NULL, 0, SKSPUDPUD,  -1, LUMPERROR, "Tired flight"},
  {"putput", false,  64,  0, -1, NULL, 0, SKSPUTPUT,  -1, LUMPERROR, "Flight"}, // not as high a priority
  {"spin" ,  false, 100,  0, -1, NULL, 0,   SKSSPIN,  -1, LUMPERROR, "Spin"},
  {"spndsh", false,  64,  1, -1, NULL, 0, SKSSPNDSH,  -1, LUMPERROR, "Spindash"},
  {"thok" ,  false,  96,  0, -1, NULL, 0,   SKSTHOK,  -1, LUMPERROR, "Thok"},
  {"zoom" ,  false, 120,  1, -1, NULL, 0,   SKSZOOM,  -1, LUMPERROR, "Spin launch"},
  {"skid",   false,  64, 32, -1, NULL, 0,   SKSSKID,  -1, LUMPERROR, "Skid"},

  // Ambience/background objects/etc
  {"ambint",  true,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Obnoxious disco music"},

  {"alarm",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Alarm"},
  {"buzz1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric zap"},
  {"buzz3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wacky worksurface"},
  {"buzz4",   true,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Buzz"},
  {"crumbl",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crumbling"}, // Platform Crumble Tails 03-16-2001
  {"fire",   false,   8, 32, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"grind",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic grinding"},
  {"laser",   true,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser hum"},
  {"mswing", false,  16,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging mace"},
  {"pstart", false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "/"},
  {"pstop",  false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"steam1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Steam jet"}, // Tails 06-19-2001
  {"steam2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Steam jet"}, // Tails 06-19-2001
  {"wbreak",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wood breaking"},
  {"ambmac", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Machinery"},
  {"spsmsh",  true,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy impact"},

  {"rainin",  true,  24,  4, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rain"},
  {"litng1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng3", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"litng4", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning"},
  {"athun1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder"},
  {"athun2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder"},

  {"amwtr1", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr2", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr3", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr4", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr5", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr6", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr7", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"amwtr8", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Stream"},
  {"bubbl1", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl2", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl3", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl4", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"bubbl5", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"floush", false,  16,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"},
  {"splash", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Glub"}, // labeling sfx_splash as "glub" and sfx_splish as "splash" seems wrong but isn't
  {"splish", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"}, // Splish Tails 12-08-2000
  {"wdrip1", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip2", false,   8 , 0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip3", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip4", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip5", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip6", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip7", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wdrip8", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drip"},
  {"wslap",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"}, // Water Slap Tails 12-13-2000

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
  {"rumbam", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"rumble", false,  64, 24, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},

  // Game objects, etc
  {"appear", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Appearing platform"},
  {"bkpoof", false,  70,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Armageddon pow"},
  {"bnce1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"}, // Boing!
  {"bnce2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Scatter"}, // Boing!
  {"cannon", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful shot"},
  {"cgot" ,   true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Emerald"}, // Got Emerald! Tails 09-02-2001
  {"cybdth", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"deton",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Threatening beeping"},
  {"ding",   false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ding"},
  {"dmpain", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Machine damage"},
  {"drown",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"fizzle", false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric fizzle"},
  {"gbeep",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Threatening beeping"}, // Grenade beep
  {"wepfir", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing weapon"}, // defaults to thok
  {"ghit" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goop splash"},
  {"gloop",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"},
  {"gspray", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Goop sling"},
  {"gravch", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Recycler"},
  {"itemup",  true, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"},
  {"jet",    false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jet engine"},
  {"jshard",  true, 167,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Life transfer"}, // placeholder repurpose; original string was "Got Shard"
  {"lose" ,  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failure"},
  {"lvpass", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning signpost"},
  {"mindig", false,   8, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tunnelling"},
  {"mixup",   true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Teleport"},
  {"monton", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Golden Monitor activated"},
  {"pogo" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical pogo"},
  {"pop"  ,  false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"rail1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing rail"},
  {"rail2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crashing rail"},
  {"rlaunc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing"},
  {"shield", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pity Shield"}, // generic GET!
  {"wirlsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whirlwind Shield"}, // Whirlwind GET!
  {"forcsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Force Shield"}, // Force GET!
  {"frcssg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Weak Force Shield"}, // Force GET...? (consider making a custom shield with this instead of a single-hit force shield!)
  {"elemsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Elemental Shield"}, // Elemental GET!
  {"armasg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Armageddon Shield"}, // Armaggeddon GET!
  {"attrsg", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction Shield"}, // Attract GET!
  {"shldls", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"}, // You LOSE!
  {"spdpad", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Speedpad"},
  {"spkdth", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spiked"},
  {"spring", false, 112,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"statu1",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pushing a statue"},
  {"statu2",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pushing a statue"},
  {"strpst",  true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starpost"},
  {"supert",  true, 127,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Transformation"},
  {"telept", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dash"},
  {"tink" ,  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tink"},
  {"token" ,  true, 224,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Token"},
  {"trfire",  true,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser fired"},
  {"trpowr",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"turhit", false,  40,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laser hit"},
  {"wdjump", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Whirlwind jump"},
  {"shrpsp",  true,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spincushion"},
  {"shrpgo", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"mswarp", false,  60, 16, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning out"},
  {"mspogo",  true,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Breaking through"},
  {"boingf", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bouncing"},
  {"corkp",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cork fired"},
  {"corkh",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cork hit"},
  {"alart",  false, 200,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Caught red handed!"},
  {"vwre",   false, 200,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Clone fighter!"},
  {"bowl",   false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bowling"},
  {"chuchu", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Train horn"},
  {"bsnipe", false, 200,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Home-run smash"},
  {"sprong", false, 112,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power spring"},
  {"lvfal1",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rumble"},
  {"pscree", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "SCREE!"},
  {"iceb",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ice crack"},
  {"shattr",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shattering"},
  {"antiri",  true, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Depletion"},

  // Menu, interface
  {"chchng", false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score"},
  {"dwnind", false, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thinking about air"},
  {"emfind", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Radar beep"},
  {"flgcap", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flag captured"},
  {"menu1",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Menu beep"},
  {"oneup",   true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "One-up"},
  {"ptally",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tally"}, // Point tally is identical to menu for now
  {"radio",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Notification"},
  {"wepchg",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Weapon change"}, // Weapon switch is identical to menu for now
  {"wtrdng",  true, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"}, // make sure you can hear the DING DING! Tails 03-08-2000
  {"zelda",  false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"},
  {"adderr",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Error"},
  {"notadd",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Reject"},
  {"addfil",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Accept"},

  // NiGHTS
  {"ideya",  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Success"},
  {"xideya", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Success"}, // Xmas
  {"nbmper", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},
  {"nxbump", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"}, // Xmas
  {"ncchip", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got chip"},
  {"ncitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got special"},
  {"nxitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got special"}, // Xmas
  {"ngdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonus time start"},
  {"nxdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonus time start"}, // Xmas
  {"drill1", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drill"},
  {"drill2", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drill"},
  {"ncspec", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up"}, // Tails 12-15-2003
  {"nghurt", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"ngskid", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Force stop"},
  {"hoop1",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop"},
  {"hoop2",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop+"},
  {"hoop3",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hoop++"},
  {"hidden", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"},
  {"prloop", false, 104,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gust of wind"},
  {"timeup",  true, 256,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous Countdown"},
  {"ngjump", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"peww",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pew"},

  // Halloween
  {"lntsit", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cacolantern awake"},
  {"lntdie", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Cacolantern death"},
  {"pumpkn", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pumpkin smash"}, // idspispopd
  {"ghosty", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Laughter"},

  // Mario
  {"koopfr" , true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire"},
  {"mario1", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hit"},
  {"mario2", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bonk"},
  {"mario3", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power-up"},
  {"mario4",  true,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got coin"},
  {"mario5", false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Boot-stomp"},
  {"mario6", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"mario7", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Fire"},
  {"mario8", false,  48,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"mario9",  true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Emerging power-up"},
  {"marioa",  true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "One-up"},
  {"thwomp",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thwomp"},

  // Black Eggman
  {"bebomb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Big explosion"},
  {"bechrg", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"becrsh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crash"},
  {"bedeen", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic crash"},
  {"bedie1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"bedie2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman crying"},
  {"beeyow", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failing machinery"},
  {"befall", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic slam"},
  {"befire", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing goop"},
  {"beflap", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical jump"},
  {"begoop", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful shot"},
  {"begrnd", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic grinding"},
  {"behurt", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman shocked"},
  {"bejet1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jetpack charge"},
  {"belnch", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical jump"},
  {"beoutb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failed shot"},
  {"beragh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman screaming"},
  {"beshot", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing missile"},
  {"bestep", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical stomp"},
  {"bestp2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical stomp"},
  {"bewar1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bewar4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eggman laughing"},
  {"bexpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"bgxpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},

  // Cybrakdemon
  {"beelec", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electricity"},
  {"brakrl", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rocket launch"},
  {"brakrx", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rocket explosion"},

  // Sonic 1 sounds
  {"s1a0",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a5",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a6",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a7",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a8",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1a9",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1aa",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ab",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Chomp"},
  {"s1ac",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ad",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ae",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1af",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b0",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b5",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b6",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b7",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b8",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1b9",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ba",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1bb",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1bc",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1bd",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1be",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1bf",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c0",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c1",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c2",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c3",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c4",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c5",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c6",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c7",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c8",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1c9",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ca",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cb",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cc",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cd",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1ce",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s1cf",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Sonic 2 sounds
  {"s220",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s221",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s222",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s223",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s224",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s225",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s226",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s227",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s228",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s229",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s22f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s230",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s231",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s232",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s233",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s234",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s235",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s236",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s237",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s238",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s239",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s23f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s240",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s241",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s242",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s243",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s244",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s245",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s246",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s247",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s248",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s249",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s24f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s250",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s251",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s252",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s253",   false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s254",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s255",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s256",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s257",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s258",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s259",   false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s25f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s260",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // S3&K sounds
  {"s3k2b",   true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Emerald"}, // Got Emerald!
  {"s3k33",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"}, // stereo in original game, identical to latter
  {"s3k34",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"}, // mono in original game, identical to previous
  {"s3k35",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Hurt"},
  {"s3k36",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Skid"},
  {"s3k37",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spiked"},
  {"s3k38",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble gasp"},
  {"s3k39",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"},
  {"s3k3a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shield"},
  {"s3k3b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"s3k3c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spin"},
  {"s3k3d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"s3k3e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame Shield"},
  {"s3k3f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble Shield"},
  {"s3k40",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction blast"},
  {"s3k41",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Thunder Shield"},
  {"s3k42",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Twinspin"},
  {"s3k43",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flame burst"},
  {"s3k44",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble bounce"},
  {"s3k45",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lightning zap"},
  {"s3k46",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Transformation"},
  {"s3k47",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising dust"},
  {"s3k48",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pulse"},
  {"s3k49",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Impact"},
  {"s3k4a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Grab"},
  {"s3k4b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Splash"},
  {"s3k4c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy hit"},
  {"s3k4d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing bullet"},
  {"s3k4e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Big explosion"},
  {"s3k4f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flamethrower"},
  {"s3k50",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Siren"},
  {"s3k51",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling"},
  {"s3k52",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spike"},
  {"s3k53",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"s3k54",  false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing"}, // MetalSonic shot fire
  {"s3k55",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical movement"},
  {"s3k56",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy landing"},
  {"s3k57",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k58",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical movement"},
  {"s3k59",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crumbling"},
  {"s3k5a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"},
  {"s3k5b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Menu beep"},
  {"s3k5c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric spark"},
  {"s3k5d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Heavy hit"},
  {"s3k5e",  false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Releasing charge"},
  {"s3k5f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"s3k60",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Accelerating"},
  {"s3k61",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drilling"},
  {"s3k62",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"s3k63",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starpost"},
  {"s3k64",  false,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR, "Clatter"},
  {"s3k65",  false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got sphere"}, // Blue Spheres
  {"s3k66",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Special stage end"},
  {"s3k67",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Firing missile"},
  {"s3k68",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Discovery"},
  {"s3k69",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Switch click"},
  {"s3k6a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Special stage clear"},
  {"s3k6b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Punch"},
  {"s3k6c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k6d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k6e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Mechanical damage"},
  {"s3k6f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3k70",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k71",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Basic Shield"},
  {"s3k72",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Movement"},
  {"s3k73",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"s3k74",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gong"},
  {"s3k75",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"},
  {"s3k76",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3k77",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Balloon pop"},
  {"s3k78",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnet"},
  {"s3k79",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Electric charge"},
  {"s3k7a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising from lava"},
  {"s3k7b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Soft bounce"},
  {"s3k7c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Magnet"},
  {"s3k7d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3k7e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dust"},
  {"s3k7f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Freeze"},
  {"s3k80",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ice spike burst"},
  {"s3k81",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k82",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Burst"},
  {"s3k83",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collapsing"},
  {"s3k84",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"s3k85",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering down"},
  {"s3k86",  false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Alarm"},
  {"s3k87",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k88",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Metallic squeak"},
  {"s3k89",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Advanced tech"},
  {"s3k8a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Boing"},
  {"s3k8b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powerful hit"},
  {"s3k8c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Humming power"},
  {"s3k8d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "/"},
  {"s3k8e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Accelerating"},
  {"s3k8f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Opening"},
  {"s3k90",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Impact"},
  {"s3k91",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Closed"},
  {"s3k92",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ghost"},
  {"s3k93",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Gas release"},
  {"s3k94",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spike"},
  {"s3k95",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Lava burst"},
  {"s3k96",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Landing"},
  {"s3k97",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind"},
  {"s3k98",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Falling spike"},
  {"s3k99",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bounce"},
  {"s3k9a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Click"},
  {"s3k9b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Crusher stomp"},
  {"s3k9c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Super Emerald"},
  {"s3k9d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Targeting"},
  {"s3k9e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wham"},
  {"s3k9f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Transformation"},
  {"s3ka0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"s3ka1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3ka2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Launch"},
  {"s3ka3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising charge"},
  {"s3ka4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Powering up"},
  {"s3ka5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3ka6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Attraction fizzle"},
  {"s3ka7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Countdown beep"},
  {"s3ka8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"},
  {"s3ka9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"},
  {"s3kaa",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},
  {"s3kab",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab4", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab5", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab6", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab7", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab8", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kab9", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kaba", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabb", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabd", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabe", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kabf", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spindash"},
  {"s3kac",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Got Continue"},
  {"s3kad",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "GO!"},
  {"s3kae",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pinball flipper"},
  {"s3kaf",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "To Special Stage"},
  {"s3kb0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Score"},
  {"s3kb1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"s3kb2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Failure"},
  {"s3kb3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"s3kb4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Explosion"},
  {"s3kb5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Clink"},
  {"s3kb6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spin launch"},
  {"s3kb7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tumbler"},
  {"s3kb8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spinning signpost"},
  {"s3kb9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ring loss"},
  {"s3kba",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flight"},
  {"s3kbb",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Tired flight"},
  {"s3kbcs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kbcl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // long version of previous
  {"s3kbds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying fortress"},
  {"s3kbdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying fortress"}, // ditto
  {"s3kbes", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying"},
  {"s3kbel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Flying"}, // ditto
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
  {"s3kc5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Revving up"},
  {"s3kc5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Revving up"}, // ditto
  {"s3kc6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Orbiting"},
  {"s3kc6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Orbiting"}, // ditto
  {"s3kc7s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"},
  {"s3kc7l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aiming"}, // ditto
  {"s3kc8s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding"},
  {"s3kc8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sliding"}, // ditto
  {"s3kc9s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging"},
  {"s3kc9l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Swinging"}, // ditto
  {"s3kcas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"},
  {"s3kcal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Energy"}, // ditto
  {"s3kcbs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3kcbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"}, // ditto
  {"s3kccs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bursting"},
  {"s3kccl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bursting"}, // ditto
  {"s3kcds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"},
  {"s3kcdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ominous rumbling"}, // ditto
  {"s3kces", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Wind tunnel"},
  {"s3kcel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dust devil"}, // ditto
  {"s3kcfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kcfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"},
  {"s3kd0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Rising"}, // ditto
  {"s3kd1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"s3kd1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""}, // ditto
  {"s3kd2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Turning"},
  {"s3kd2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Moving chain"}, // ditto
  {"s3kd3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Digging"},
  {"s3kd3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Digging"}, // ditto
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

  // 3D Blast sounds (the "missing" ones are direct copies of S3K's, no minor differences what-so-ever)
  {"3db06",  false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Collection"},
  {"3db09",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Peep"},
  {"3db14",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Chirp"},
  {"3db16",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // Sonic CD sounds
  {"cdfm00", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Skid"},
  {"cdfm01", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm02", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Jump"},
  {"cdfm03", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Dying"},
  {"cdfm04", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ring loss"},
  {"cdfm05", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sparkle"},
  {"cdfm06", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop"},
  {"cdfm07", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shield"},
  {"cdfm08", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Spring"},
  {"cdfm09", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm10", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm11", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm12", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm13", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm14", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm15", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm16", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm17", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm18", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm19", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm20", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm21", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm22", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm23", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm24", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm25", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm26", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm27", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm28", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm29", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bubble gasp"},
  {"cdfm30", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Starpost"},
  {"cdfm31", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"cdfm32", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm33", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm34", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm35", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm36", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm37", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm38", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Drowning"},
  {"cdfm39", false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm40", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power up"},
  {"cdfm41", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm42", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm43", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm44", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Extra time"},
  {"cdfm45", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm46", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm47", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm48", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm49", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Aquaphobia"},
  {"cdfm50", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm51", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm52", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm53", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm54", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm55", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm56", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Warp"},
  {"cdfm57", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm58", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm59", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm60", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm61", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm62", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Speed boost"},
  {"cdfm63", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm64", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm65", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm66", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm67", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm68", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm69", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm70", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm71", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm72", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm73", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm74", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm75", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm76", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm77", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm78", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdfm79", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdpcm0", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Future."},
  {"cdpcm1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Past."},
  {"cdpcm2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "All right!"},
  {"cdpcm3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "I'm outta here..."},
  {"cdpcm4", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Yes!"},
  {"cdpcm5", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Yeah!"},
  {"cdpcm6", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Giggles"},
  {"cdpcm7", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Eep!"},
  {"cdpcm8", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"cdpcm9", false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Bumper"},

  // Knuckles Chaotix sounds
  {"kc2a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2e",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc2f",   false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc30",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc31",   false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc32",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc33",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc34",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc35",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc36",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc37",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc38",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc39",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc3f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc40",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc41",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc42",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Confirm"},
  {"kc43",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc44",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc45",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc46",   false,  96,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc47",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc48",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Select"},
  {"kc49",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4a",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Pop-shot"},
  {"kc4d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power up"},
  {"kc4e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc4f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc50",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc51",   false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc52",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc53",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc54",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc55",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc56",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc57",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Sheer terror"},
  {"kc58",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc59",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Shrink"},
  {"kc5a",   false, 128,  8, -1, NULL, 0,        -1,  -1, LUMPERROR, "Grow"},
  {"kc5b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc5f",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc60",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc61",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc62",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc63",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc64",   false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Terrifying rumble"},
  {"kc65",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Power down"},
  {"kc66",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc67",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc68",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc69",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc6b",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, "Ascending"},
  {"kc6c",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc6d",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},
  {"kc6e",   false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR, ""},

  // skin sounds free slots to add sounds at run time (Boris HACK!!!)
  // initialized to NULL
};

char freeslotnames[sfx_freeslot0 + NUMSFXFREESLOTS + NUMSKINSFXSLOTS][7];

// Prepare free sfx slots to add sfx at run time
void S_InitRuntimeSounds (void)
{
	sfxenum_t i;
	INT32 value;
	char soundname[10];

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
		//strlcpy(S_sfx[i].caption, "", 1);
		S_sfx[i].caption[0] = '\0';
	}
}

sfxenum_t sfxfree = sfx_freeslot0;

// Add a new sound fx into a free sfx slot.
//
sfxenum_t S_AddSoundFx(const char *name, boolean singular, INT32 flags, boolean skinsound)
{
	sfxenum_t i;

	if (skinsound)
	{
		for (i = sfx_skinsoundslot0; i < NUMSFX; i++)
		{
			if (S_sfx[i].priority)
				continue;
			break;
		}
	}
	else
		i = sfxfree;

	if (i < NUMSFX)
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

		if (!skinsound)
			sfxfree++;

		return i;
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
