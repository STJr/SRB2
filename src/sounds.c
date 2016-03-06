// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
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
//  name, singularity, priority, pitch, volume, data, length, skinsound, usefulness, lumpnum
  {"none" ,  false,   0,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // Skin Sounds
  {"altdi1", false, 192, 16, -1, NULL, 0, SKSPLDET1,  -1, LUMPERROR},
  {"altdi2", false, 192, 16, -1, NULL, 0, SKSPLDET2,  -1, LUMPERROR},
  {"altdi3", false, 192, 16, -1, NULL, 0, SKSPLDET3,  -1, LUMPERROR},
  {"altdi4", false, 192, 16, -1, NULL, 0, SKSPLDET4,  -1, LUMPERROR},
  {"altow1", false, 192, 16, -1, NULL, 0, SKSPLPAN1,  -1, LUMPERROR},
  {"altow2", false, 192, 16, -1, NULL, 0, SKSPLPAN2,  -1, LUMPERROR},
  {"altow3", false, 192, 16, -1, NULL, 0, SKSPLPAN3,  -1, LUMPERROR},
  {"altow4", false, 192, 16, -1, NULL, 0, SKSPLPAN4,  -1, LUMPERROR},
  {"victr1", false,  64, 16, -1, NULL, 0, SKSPLVCT1,  -1, LUMPERROR},
  {"victr2", false,  64, 16, -1, NULL, 0, SKSPLVCT2,  -1, LUMPERROR},
  {"victr3", false,  64, 16, -1, NULL, 0, SKSPLVCT3,  -1, LUMPERROR},
  {"victr4", false,  64, 16, -1, NULL, 0, SKSPLVCT4,  -1, LUMPERROR},
  {"gasp" ,  false,  64,  0, -1, NULL, 0,   SKSGASP,  -1, LUMPERROR},
  {"jump" ,  false, 140,  0, -1, NULL, 0,   SKSJUMP,  -1, LUMPERROR},
  {"pudpud", false,  64,  0, -1, NULL, 0, SKSPUDPUD,  -1, LUMPERROR},
  {"putput", false,  64,  0, -1, NULL, 0, SKSPUTPUT,  -1, LUMPERROR}, // not as high a priority
  {"spin" ,  false, 100,  0, -1, NULL, 0,   SKSSPIN,  -1, LUMPERROR},
  {"spndsh", false,  64,  1, -1, NULL, 0, SKSSPNDSH,  -1, LUMPERROR},
  {"thok" ,  false,  96,  0, -1, NULL, 0,   SKSTHOK,  -1, LUMPERROR},
  {"zoom" ,  false, 120,  1, -1, NULL, 0,   SKSZOOM,  -1, LUMPERROR},
  {"skid",   false,  64, 32, -1, NULL, 0,   SKSSKID,  -1, LUMPERROR},

  // Ambience/background objects/etc
  {"ambint",  true,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  {"alarm",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"buzz1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"buzz2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"buzz3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"buzz4",  false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"crumbl",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Platform Crumble Tails 03-16-2001
  {"fire",   false,   8, 32, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"grind",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"laser",   true,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mswing", false,  16,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"pstart", false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"pstop",  false, 100,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"steam1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Tails 06-19-2001
  {"steam2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Tails 06-19-2001
  {"wbreak", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  {"rainin",  true,  24,  4, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"litng1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"litng2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"litng3", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"litng4", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"athun1", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"athun2", false,  16,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},

  {"amwtr1", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr2", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr3", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr4", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr5", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr6", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr7", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"amwtr8", false,  12,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bubbl1", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bubbl2", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bubbl3", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bubbl4", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bubbl5", false,  11,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"floush", false,  16,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"splash", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"splish", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Splish Tails 12-08-2000
  {"wdrip1", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip2", false,   8 , 0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip3", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip4", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip5", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip6", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip7", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdrip8", false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wslap",  false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Water Slap Tails 12-13-2000

  {"doora1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"doorb1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"doorc1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"doorc2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"doord1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"doord2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"eleva1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"eleva2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"eleva3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"elevb1", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"elevb2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"elevb3", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  {"ambin2", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"lavbub", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rocks1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rocks2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rocks3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rocks4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rumbam", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rumble", false,  64, 24, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // Game objects, etc
  {"appear", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bkpoof", false,  70,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bnce1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Boing!
  {"bnce2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Boing!
  {"cannon", false,  64,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"cgot" ,   true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Got Emerald! Tails 09-02-2001
  {"cybdth", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"deton",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"ding",   false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"dmpain", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"drown",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"fizzle", false, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"gbeep",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Grenade beep
  {"gclose", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"ghit" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"gloop",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"gspray", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"gravch", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"itemup",  true, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"jet",    false,   8,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"jshard",  true, 167,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"lose" ,  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"lvpass", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mindig", false,   8, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mixup",   true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"pogo" ,  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"pop"  ,  false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rail1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rail2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"rlaunc", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"shield", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"shldls", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"spdpad", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"spkdth", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"spring", false, 112,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"statu1",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"statu2",  true,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"strpst",  true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Starpost Sound Tails 07-04-2002
  {"supert",  true, 127,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"telept", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"tink" ,  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"token" ,  true, 224,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // SS token
  {"trfire",  true,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"trpowr",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"turhit", false,  40,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wdjump", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mswarp", false,  60, 16, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mspogo", false,  60,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // Menu, interface
  {"chchng", false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"dwnind", false, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"emfind", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"flgcap", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"menu1",   true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"oneup",   true, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"ptally",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Point tally is identical to menu for now
  {"radio",  false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"wepchg",  true,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Weapon switch is identical to menu for now
  {"wtrdng",  true, 212,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // make sure you can hear the DING DING! Tails 03-08-2000
  {"zelda",  false, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // NiGHTS
  {"ideya",  false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"xideya", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Xmas
  {"nbmper", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"nxbump", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Xmas
  {"ncitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"nxitem", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Xmas
  {"ngdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"nxdone",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Xmas
  {"drill1", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"drill2", false,  48,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"ncspec", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Tails 12-15-2003
  {"nghurt", false,  96,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"ngskid", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"hoop1",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"hoop2",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"hoop3",  false, 192,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"hidden", false, 204,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"prloop", false, 104,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"timeup",  true, 256,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // Mario
  {"koopfr" , true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario1", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario2", false, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario3", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario4",  true,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario5", false,  78,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario6", false,  60,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario7", false,  32,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario8", false,  48,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"mario9",  true, 120,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"marioa",  true, 127,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"thwomp",  true, 127,  8, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // Black Eggman
  {"bebomb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bechrg", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"becrsh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bedeen", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bedie1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bedie2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"beeyow", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"befall", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"befire", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"beflap", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"begoop", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"begrnd", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"behurt", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bejet1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"belnch", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"beoutb", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"beragh", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"beshot", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bestep", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bestp2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bewar1", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bewar2", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bewar3", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bewar4", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bexpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"bgxpld", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // Cybrakdemon
  {"beelec", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"brakrl", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"brakrx", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // S3&K sounds
  {"s3k33",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k34",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k35",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k36",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k37",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k38",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k39",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k3a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k3b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k3c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k3d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k3e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k3f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k40",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k41",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k42",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k43",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k44",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k45",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k46",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k47",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k48",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k49",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k4a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k4b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k4c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k4d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k4e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k4f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k50",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k51",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k52",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k53",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k54",  false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR}, // MetalSonic shot fire
  {"s3k55",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k56",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k57",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k58",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k59",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k5a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k5b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k5c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k5d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k5e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k5f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k60",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k61",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k62",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k63",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k64",  false,  64,  2, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k65",  false, 255,  0, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Blue Spheres
  {"s3k66",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k67",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k68",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k69",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k6a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k6b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k6c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k6d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k6e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k6f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k70",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k71",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k72",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k73",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k74",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k75",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k76",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k77",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k78",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k79",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k7a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k7b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k7c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k7d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k7e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k7f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k80",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k81",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k82",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k83",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k84",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k85",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k86",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k87",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k88",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k89",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k8a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k8b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k8c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k8d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k8e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k8f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k90",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k91",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k92",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k93",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k94",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k95",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k96",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k97",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k98",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k99",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k9a",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k9b",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k9c",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k9d",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k9e",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3k9f",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3ka9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kaa",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kab",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kac",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kad",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kae",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kaf",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb0",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb1",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb2",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb3",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb4",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb5",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb6",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb8",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kb9",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kba",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbb",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbcs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbcl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbes", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kbfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc4s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc4l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc7",  false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc8s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc9s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kc9l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcbs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kccs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kccl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcds", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcdl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kces", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcel", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcfs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kcfl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd0s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd0l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd1s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd1l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd2s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd2l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd3s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd3l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd4s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd4l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd5s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd5l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd6s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd6l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd7s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd7l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd8s", false,  64, 64, -1, NULL, 0,        -1,  -1, LUMPERROR}, // Sharp Spin (maybe use the long/L version?)
  {"s3kd8l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd9s", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kd9l", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kdas", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kdal", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kdbs", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},
  {"s3kdbl", false,  64,  0, -1, NULL, 0,        -1,  -1, LUMPERROR},

  // skin sounds free slots to add sounds at run time (Boris HACK!!!)
  // initialized to NULL
};

char freeslotnames[sfx_freeslot0 + NUMSFXFREESLOTS + NUMSKINSFXSLOTS][7];
char musicfreeslotnames[mus_frees0 + NUMMUSFREESLOTS][7];

// Prepare free sfx slots to add sfx at run time  (yellowtd: Include musics too~!)
void S_InitRuntimeSounds (void)
{
	sfxenum_t i;
	musicenum_t x;
	INT32 value;
	char soundname[7];
	char musicname[7];
	
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
	}
	
	for (x = mus_frees0; x <= mus_lstfre; x++)
	{
		value = (x+1) - mus_frees0;

		if (value < 10)
			sprintf(musicname, "fre00%d", value);
		else if (value < 100)
			sprintf(musicname, "fre0%d", value);
		else if (value < 1000)
			sprintf(musicname, "fre%d", value);
		else
			sprintf(musicname, "fr%d", value);

		strcpy(musicfreeslotnames[value-1], musicname);

		S_music[x].name = musicfreeslotnames[value-1];
		S_music[x].data = NULL;
		S_music[x].lumpnum = 0;
		S_music[x].handle = -1;
		S_music[x].dummyval = 0;
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

musicenum_t S_AddMusic(const char *name, INT32 dummyval)
{
	musicenum_t i, slot;

	slot = mus_frees0;

	for (i = slot; i < NUMMUSIC; i++)
	{
        //Dummy value because no .priority exists
		if (!S_music[i].dummyval)
		{
			strncpy(musicfreeslotnames[i-mus_frees0], name, 6);
			S_music[i].dummyval = dummyval;
            S_music[i].handle = -1;
			S_music[i].lumpnum = 0;

			/// \todo if precached load it here
			S_music[i].data = NULL;
			return i;
		}
	}
	CONS_Alert(CONS_WARNING, M_GetText("No more free music slots\n"));
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
