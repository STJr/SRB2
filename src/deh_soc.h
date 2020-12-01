// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_soc.h
/// \brief Load SOC file and change tables and text

#ifndef __DEH_SOC_H__
#define __DEH_SOC_H__

#include "doomdef.h"
#include "g_game.h"
#include "sounds.h"
#include "info.h"
#include "d_think.h"
#include "m_argv.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_menu.h"
#include "m_misc.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_system.h"
#include "p_setup.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_draw.h"
#include "r_picformats.h"
#include "r_things.h" // R_Char2Frame
#include "r_sky.h"
#include "fastcmp.h"
#include "lua_script.h" // Reluctantly included for LUA_EvalMath
#include "d_clisrv.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif

#include "info.h"
#include "dehacked.h"
#include "doomdef.h" // MUSICSLOT_COMPATIBILITY, HWRENDER

// Crazy word-reading stuff
/// \todo Put these in a seperate file or something.
mobjtype_t get_mobjtype(const char *word);
statenum_t get_state(const char *word);
spritenum_t get_sprite(const char *word);
playersprite_t get_sprite2(const char *word);
sfxenum_t get_sfx(const char *word);
#ifdef MUSICSLOT_COMPATIBILITY
UINT16 get_mus(const char *word, UINT8 dehacked_mode);
#endif
hudnum_t get_huditem(const char *word);
menutype_t get_menutype(const char *word);
//INT16 get_gametype(const char *word);
//powertype_t get_power(const char *word);
skincolornum_t get_skincolor(const char *word);

void readwipes(MYFILE *f);
void readmaincfg(MYFILE *f);
void readconditionset(MYFILE *f, UINT8 setnum);
void readunlockable(MYFILE *f, INT32 num);
void readextraemblemdata(MYFILE *f, INT32 num);
void reademblemdata(MYFILE *f, INT32 num);
void readsound(MYFILE *f, INT32 num);
void readframe(MYFILE *f, INT32 num);
void readhuditem(MYFILE *f, INT32 num);
void readmenu(MYFILE *f, INT32 num);
void readtextprompt(MYFILE *f, INT32 num);
void readcutscene(MYFILE *f, INT32 num);
void readlevelheader(MYFILE *f, INT32 num);
void readgametype(MYFILE *f, char *gtname);
void readsprite2(MYFILE *f, INT32 num);
void readspriteinfo(MYFILE *f, INT32 num, boolean sprite2);
#ifdef HWRENDER
void readlight(MYFILE *f, INT32 num);
#endif
void readskincolor(MYFILE *f, INT32 num);
void readthing(MYFILE *f, INT32 num);
void readfreeslots(MYFILE *f);
void readPlayer(MYFILE *f, INT32 num);
void clear_levels(void);
void clear_conditionsets(void);
#endif
