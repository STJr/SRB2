// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_light.c
/// \brief Corona/Dynamic/Static lighting add on by Hurdler
///	!!! Under construction !!!

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_light.h"
#include "hw_drv.h"
#include "../i_video.h"
#include "../z_zone.h"
#include "../m_random.h"
#include "../m_bbox.h"
#include "../w_wad.h"
#include "../r_state.h"
#include "../r_main.h"
#include "../p_local.h"

//=============================================================================
//                                                                      DEFINES
//=============================================================================

#define DL_SQRRADIUS(x) dynlights->p_lspr[(x)]->dynamic_sqrradius
#define DL_RADIUS(x) dynlights->p_lspr[(x)]->dynamic_radius
#define LIGHT_POS(i) dynlights->position[(i)]

#define DL_HIGH_QUALITY
//#define STATICLIGHT  //Hurdler: TODO!
#define LIGHTMAPFLAGS (PF_Modulated|PF_Additive)

#ifdef ALAM_LIGHTING
static dynlights_t view_dynlights[2]; // 2 players in splitscreen mode
static dynlights_t *dynlights = &view_dynlights[0];
#endif

#define UNDEFINED_SPR   0x0 // actually just for testing
#define CORONA_SPR      0x1 // a light source which only emit a corona
#define DYNLIGHT_SPR    0x2 // a light source which is only used for dynamic lighting
#define LIGHT_SPR       (DYNLIGHT_SPR|CORONA_SPR)
#define ROCKET_SPR      (DYNLIGHT_SPR|CORONA_SPR|0x10)
//#define MONSTER_SPR     4
//#define AMMO_SPR        8
//#define BONUS_SPR      16

//Hurdler: now we can change those values via FS :)
light_t lspr[NUMLIGHTS] =
{
	// type       offset x,   y  coronas color, c_size,light color,l_radius, sqr radius computed at init
	// NOLIGHT: 0
	{ UNDEFINED_SPR,  0.0f,   0.0f, 0x00000000,  24.0f, 0x00000000,   0.0f, 0.0f},
	// weapons
	// RINGSPARK_L
	{ LIGHT_SPR,      0.0f,   0.0f, 0x0000e0ff,  16.0f, 0x0000e0ff,  32.0f, 0.0f}, // Tails 09-08-2002
	// SUPERSONIC_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0xff00e0ff,  32.0f, 0xff00e0ff, 128.0f, 0.0f}, // Tails 09-08-2002
	// SUPERSPARK_L
	{ LIGHT_SPR,      0.0f,   0.0f, 0xe0ffffff,   8.0f, 0xe0ffffff,  64.0f, 0.0f},
	// INVINCIBLE_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x10ffaaaa,  16.0f, 0x10ffaaaa, 128.0f, 0.0f},
	// GREENSHIELD_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x602b7337,  32.0f, 0x602b7337, 128.0f, 0.0f},
	// BLUESHIELD_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60cb0000,  32.0f, 0x60cb0000, 128.0f, 0.0f},

	// tall lights
	// YELLOWSHIELD_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x601f7baf,  32.0f, 0x601f7baf, 128.0f, 0.0f},

	// REDSHIELD_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x600000cb,  32.0f, 0x600000cb, 128.0f, 0.0f},

	// BLACKSHIELD_L // Black light? lol
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60010101,  32.0f, 0x60ff00ff, 128.0f, 0.0f},

	// WHITESHIELD_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60ffffff,  32.0f, 0x60ffffff, 128.0f, 0.0f},

	// SMALLREDBALL_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x606060f0,   0.0f, 0x302070ff,  32.0f, 0.0f},

	// small lights
	// RINGLIGHT_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60b0f0f0,   0.0f, 0x30b0f0f0, 100.0f, 0.0f},
	// GREENSMALL_L
	{    LIGHT_SPR,   0.0f,  14.0f, 0x6070ff70,  60.0f, 0x4070ff70, 100.0f, 0.0f},
	// REDSMALL_L
	{    LIGHT_SPR,   0.0f,  14.0f, 0x705070ff,  60.0f, 0x405070ff, 100.0f, 0.0f},

	// type       offset x,   y  coronas color, c_size,light color,l_radius, sqr radius computed at init
	// GREENSHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xff00ff00,  64.0f, 0xff00ff00, 256.0f, 0.0f},
	// ORANGESHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xff0080ff,  64.0f, 0xff0080ff, 256.0f, 0.0f},
	// PINKSHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xffe080ff,  64.0f, 0xffe080ff, 256.0f, 0.0f},
	// BLUESHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xffff0000,  64.0f, 0xffff0000, 256.0f, 0.0f},
	// REDSHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xff0000ff,  64.0f, 0xff0000ff, 256.0f, 0.0f},
	// LBLUESHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xffff8080,  64.0f, 0xffff8080, 256.0f, 0.0f},
	// GREYSHINE_L
	{    LIGHT_SPR,   0.0f,   0.0f, 0xffe0e0e0,  64.0f, 0xffe0e0e0, 256.0f, 0.0f},

	// monsters
	// REDBALL_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x606060ff,   0.0f, 0x606060ff, 100.0f, 0.0f},
	// GREENBALL_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x6060ff60, 120.0f, 0x6060ff60, 100.0f, 0.0f},
	// BLUEBALL_L
	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60ff6060, 120.0f, 0x60ff6060, 100.0f, 0.0f},

	// NIGHTSLIGHT_L
	{    LIGHT_SPR,   0.0f,   6.0f, 0x60ffffff,  16.0f, 0x30ffffff,  32.0f, 0.0f},

	// JETLIGHT_L
	{ DYNLIGHT_SPR,   0.0f,   6.0f, 0x60ffaaaa,  16.0f, 0x30ffaaaa,  64.0f, 0.0f},

	// GOOPLIGHT_L
	{ DYNLIGHT_SPR,   0.0f,   6.0f, 0x60ff00ff,  16.0f, 0x30ff00ff,  32.0f, 0.0f},

	// STREETLIGHT_L
	{ LIGHT_SPR,      0.0f,   0.0f, 0xe0ffffff,  64.0f, 0xe0ffffff, 384.0f, 0.0f},
};

light_t *t_lspr[NUMSPRITES] =
{
	&lspr[NOLIGHT],     // SPR_NULL
	&lspr[NOLIGHT],     // SPR_UNKN

	&lspr[NOLIGHT],     // SPR_THOK
	&lspr[SUPERSONIC_L],// SPR_PLAY

	// Enemies
	&lspr[NOLIGHT],     // SPR_POSS
	&lspr[NOLIGHT],     // SPR_SPOS
	&lspr[NOLIGHT],     // SPR_FISH
	&lspr[NOLIGHT],     // SPR_BUZZ
	&lspr[NOLIGHT],     // SPR_RBUZ
	&lspr[NOLIGHT],     // SPR_JETB
	&lspr[NOLIGHT],     // SPR_JETG
	&lspr[NOLIGHT],     // SPR_CCOM
	&lspr[NOLIGHT],     // SPR_DETN
	&lspr[NOLIGHT],     // SPR_SKIM
	&lspr[NOLIGHT],     // SPR_TRET
	&lspr[NOLIGHT],     // SPR_TURR
	&lspr[NOLIGHT],     // SPR_SHRP
	&lspr[NOLIGHT],     // SPR_CRAB
	&lspr[NOLIGHT],     // SPR_CR2B
	&lspr[NOLIGHT],     // SPR_CSPR
	&lspr[NOLIGHT],     // SPR_JJAW
	&lspr[NOLIGHT],     // SPR_SNLR
	&lspr[NOLIGHT],     // SPR_VLTR
	&lspr[NOLIGHT],     // SPR_PNTY
	&lspr[NOLIGHT],     // SPR_ARCH
	&lspr[NOLIGHT],     // SPR_CBFS
	&lspr[JETLIGHT_L],  // SPR_STAB
	&lspr[NOLIGHT],     // SPR_SPSH
	&lspr[NOLIGHT],     // SPR_ESHI
	&lspr[NOLIGHT],     // SPR_GSNP
	&lspr[NOLIGHT],     // SPR_GSNL
	&lspr[NOLIGHT],     // SPR_GSNH
	&lspr[NOLIGHT],     // SPR_MNUS
	&lspr[NOLIGHT],     // SPR_MNUD
	&lspr[NOLIGHT],     // SPR_SSHL
	&lspr[NOLIGHT],     // SPR_UNID
	&lspr[NOLIGHT],     // SPR_CANA
	&lspr[NOLIGHT],     // SPR_CANG
	&lspr[NOLIGHT],     // SPR_PYRE
	&lspr[NOLIGHT],     // SPR_PTER
	&lspr[NOLIGHT],     // SPR_DRAB

	// Generic Boos Items
	&lspr[JETLIGHT_L],     // SPR_JETF // Boss jet fumes

	// Boss 1, (Greenflower)
	&lspr[NOLIGHT],     // SPR_EGGM
	&lspr[NOLIGHT],     // SPR_EGLZ

	// Boss 2, (Techno Hill)
	&lspr[NOLIGHT],     // SPR_EGGN
	&lspr[NOLIGHT],     // SPR_TANK
	&lspr[NOLIGHT],     // SPR_GOOP

	// Boss 3 (Deep Sea)
	&lspr[NOLIGHT],     // SPR_EGGO
	&lspr[NOLIGHT],     // SPR_SEBH
	&lspr[NOLIGHT],     // SPR_FAKE
	&lspr[LBLUESHINE_L],// SPR_SHCK

	// Boss 4 (Castle Eggman)
	&lspr[NOLIGHT],     // SPR_EGGP
	&lspr[REDBALL_L],   // SPR_EFIR
	&lspr[NOLIGHT],     // SPR_EGR1

	// Boss 5 (Arid Canyon)
	&lspr[NOLIGHT],      // SPR_FANG // replaces EGGQ
	&lspr[NOLIGHT],      // SPR_BRKN
	&lspr[NOLIGHT],      // SPR_WHAT
	&lspr[INVINCIBLE_L], // SPR_VWRE
	&lspr[INVINCIBLE_L], // SPR_PROJ
	&lspr[NOLIGHT],      // SPR_FBOM
	&lspr[NOLIGHT],      // SPR_FSGN
	&lspr[REDBALL_L],    // SPR_BARX // bomb explosion (also used by barrel)
	&lspr[NOLIGHT],      // SPR_BARD // bomb dust (also used by barrel)

	// Boss 6 (Red Volcano)
	&lspr[NOLIGHT],     // SPR_EEGR

	// Boss 7 (Dark City)
	&lspr[NOLIGHT],     // SPR_BRAK
	&lspr[NOLIGHT],     // SPR_BGOO
	&lspr[NOLIGHT],     // SPR_BMSL

	// Boss 8 (Egg Rock)
	&lspr[NOLIGHT],     // SPR_EGGT

	&lspr[NOLIGHT], //SPR_RCKT
	&lspr[NOLIGHT], //SPR_ELEC
	&lspr[NOLIGHT], //SPR_TARG
	&lspr[NOLIGHT], //SPR_NPLM
	&lspr[NOLIGHT], //SPR_MNPL

	// Metal Sonic
	&lspr[NOLIGHT],     // SPR_METL
	&lspr[NOLIGHT],     // SPR_MSCF
	&lspr[NOLIGHT],     // SPR_MSCB

	// Collectible Items
	&lspr[NOLIGHT],     // SPR_RING
	&lspr[NOLIGHT],     // SPR_TRNG
	&lspr[NOLIGHT],     // SPR_TOKE
	&lspr[REDBALL_L],   // SPR_RFLG
	&lspr[BLUEBALL_L],  // SPR_BFLG
	&lspr[NOLIGHT],     // SPR_SPHR
	&lspr[NOLIGHT],     // SPR_NCHP
	&lspr[NOLIGHT],     // SPR_NSTR
	&lspr[NOLIGHT],     // SPR_EMBM
	&lspr[NOLIGHT],     // SPR_CEMG
	&lspr[NOLIGHT],     // SPR_SHRD

	// Interactive Objects
	&lspr[NOLIGHT],     // SPR_BBLS
	&lspr[NOLIGHT],     // SPR_SIGN
	&lspr[NOLIGHT],     // SPR_SPIK
	&lspr[NOLIGHT],     // SPR_SFLM
	&lspr[NOLIGHT],     // SPR_TFLM
	&lspr[NOLIGHT],     // SPR_USPK
	&lspr[NOLIGHT],     // SPR_WSPK
	&lspr[NOLIGHT],     // SPR_WSPB
	&lspr[NOLIGHT],     // SPR_STPT
	&lspr[NOLIGHT],     // SPR_BMNE
	&lspr[NOLIGHT],     // SPR_PUMI

	// Monitor Boxes
	&lspr[NOLIGHT],     // SPR_MSTV
	&lspr[NOLIGHT],     // SPR_XLTV

	&lspr[NOLIGHT],     // SPR_TRRI
	&lspr[NOLIGHT],     // SPR_TBRI

	&lspr[NOLIGHT],     // SPR_TVRI
	&lspr[NOLIGHT],     // SPR_TVPI
	&lspr[NOLIGHT],     // SPR_TVAT
	&lspr[NOLIGHT],     // SPR_TVFO
	&lspr[NOLIGHT],     // SPR_TVAR
	&lspr[NOLIGHT],     // SPR_TVWW
	&lspr[NOLIGHT],     // SPR_TVEL
	&lspr[NOLIGHT],     // SPR_TVSS
	&lspr[NOLIGHT],     // SPR_TVIV
	&lspr[NOLIGHT],     // SPR_TV1U
	&lspr[NOLIGHT],     // SPR_TV1P
	&lspr[NOLIGHT],     // SPR_TVEG
	&lspr[NOLIGHT],     // SPR_TVMX
	&lspr[NOLIGHT],     // SPR_TVMY
	&lspr[NOLIGHT],     // SPR_TVGV
	&lspr[NOLIGHT],     // SPR_TVRC
	&lspr[NOLIGHT],     // SPR_TV1K
	&lspr[NOLIGHT],     // SPR_TVTK
	&lspr[NOLIGHT],     // SPR_TVFL
	&lspr[NOLIGHT],     // SPR_TVBB
	&lspr[NOLIGHT],     // SPR_TVZP

	// Projectiles
	&lspr[NOLIGHT],     // SPR_MISL
	&lspr[SMALLREDBALL_L], // SPR_LASR
	&lspr[REDSHINE_L],  // SPR_LASF
	&lspr[NOLIGHT],     // SPR_TORP
	&lspr[NOLIGHT],     // SPR_ENRG
	&lspr[NOLIGHT],     // SPR_MINE
	&lspr[NOLIGHT],     // SPR_JBUL
	&lspr[SMALLREDBALL_L], // SPR_TRLS
	&lspr[NOLIGHT],     // SPR_CBLL
	&lspr[NOLIGHT],     // SPR_AROW
	&lspr[NOLIGHT],     // SPR_CFIR

	// Greenflower Scenery
	&lspr[NOLIGHT],     // SPR_FWR1
	&lspr[NOLIGHT],     // SPR_FWR2
	&lspr[NOLIGHT],     // SPR_FWR3
	&lspr[NOLIGHT],     // SPR_FWR4
	&lspr[NOLIGHT],     // SPR_BUS1
	&lspr[NOLIGHT],     // SPR_BUS2
	&lspr[NOLIGHT],     // SPR_BUS3
	// Trees (both GFZ and misc)
	&lspr[NOLIGHT],     // SPR_TRE1
	&lspr[NOLIGHT],     // SPR_TRE2
	&lspr[NOLIGHT],     // SPR_TRE3
	&lspr[NOLIGHT],     // SPR_TRE4
	&lspr[NOLIGHT],     // SPR_TRE5
	&lspr[NOLIGHT],     // SPR_TRE6

	// Techno Hill Scenery
	&lspr[NOLIGHT],     // SPR_THZP
	&lspr[NOLIGHT],     // SPR_FWR5
	&lspr[REDBALL_L],   // SPR_ALRM

	// Deep Sea Scenery
	&lspr[NOLIGHT],     // SPR_GARG
	&lspr[NOLIGHT],     // SPR_SEWE
	&lspr[NOLIGHT],     // SPR_DRIP
	&lspr[NOLIGHT],     // SPR_CRL1
	&lspr[NOLIGHT],     // SPR_CRL2
	&lspr[NOLIGHT],     // SPR_CRL3
	&lspr[NOLIGHT],     // SPR_BCRY

	// Castle Eggman Scenery
	&lspr[NOLIGHT],     // SPR_CHAN
	&lspr[REDBALL_L],   // SPR_FLAM
	&lspr[NOLIGHT],     // SPR_ESTA
	&lspr[NOLIGHT],     // SPR_SMCH
	&lspr[NOLIGHT],     // SPR_BMCH
	&lspr[NOLIGHT],     // SPR_SMCE
	&lspr[NOLIGHT],     // SPR_BMCE
	&lspr[NOLIGHT],     // SPR_YSPB
	&lspr[NOLIGHT],     // SPR_RSPB
	&lspr[REDBALL_L],   // SPR_SFBR
	&lspr[REDBALL_L],   // SPR_BFBR
	&lspr[NOLIGHT],     // SPR_BANR
	&lspr[NOLIGHT],     // SPR_PINE
	&lspr[NOLIGHT],     // SPR_CEZB
	&lspr[REDBALL_L],   // SPR_CNDL
	&lspr[NOLIGHT],     // SPR_FLMH
	&lspr[REDBALL_L],   // SPR_CTRC
	&lspr[NOLIGHT],     // SPR_CFLG
	&lspr[NOLIGHT],     // SPR_CSTA
	&lspr[NOLIGHT],     // SPR_CBBS

	// Arid Canyon Scenery
	&lspr[NOLIGHT],     // SPR_BTBL
	&lspr[NOLIGHT],     // SPR_STBL
	&lspr[NOLIGHT],     // SPR_CACT
	&lspr[NOLIGHT],     // SPR_WWSG
	&lspr[NOLIGHT],     // SPR_WWS2
	&lspr[NOLIGHT],     // SPR_WWS3
	&lspr[NOLIGHT],     // SPR_OILL
	&lspr[NOLIGHT],     // SPR_OILF
	&lspr[NOLIGHT],     // SPR_BARR
	&lspr[NOLIGHT],     // SPR_REMT
	&lspr[NOLIGHT],     // SPR_TAZD
	&lspr[NOLIGHT],     // SPR_ADST
	&lspr[NOLIGHT],     // SPR_MCRT
	&lspr[NOLIGHT],     // SPR_MCSP
	&lspr[NOLIGHT],     // SPR_SALD
	&lspr[NOLIGHT],     // SPR_TRAE
	&lspr[NOLIGHT],     // SPR_TRAI
	&lspr[NOLIGHT],     // SPR_STEA

	// Red Volcano Scenery
	&lspr[REDBALL_L],   // SPR_FLME
	&lspr[REDBALL_L],   // SPR_DFLM
	&lspr[NOLIGHT],     // SPR_LFAL
	&lspr[NOLIGHT],     // SPR_JPLA
	&lspr[NOLIGHT],     // SPR_TFLO
	&lspr[NOLIGHT],     // SPR_WVIN

	// Dark City Scenery

	// Egg Rock Scenery

	// Christmas Scenery
	&lspr[NOLIGHT],     // SPR_XMS1
	&lspr[NOLIGHT],     // SPR_XMS2
	&lspr[NOLIGHT],     // SPR_XMS3
	&lspr[NOLIGHT],     // SPR_XMS4
	&lspr[NOLIGHT],     // SPR_XMS5
	&lspr[NOLIGHT],     // SPR_XMS6
	&lspr[NOLIGHT],     // SPR_FHZI
	&lspr[NOLIGHT],     // SPR_ROSY

	// Halloween Scenery
	&lspr[RINGLIGHT_L], // SPR_PUMK
	&lspr[NOLIGHT],     // SPR_HHPL
	&lspr[NOLIGHT],     // SPR_SHRM
	&lspr[NOLIGHT],     // SPR_HHZM

	// Azure Temple Scenery
	&lspr[NOLIGHT],     // SPR_BGAR
	&lspr[NOLIGHT],     // SPR_RCRY
	&lspr[GREENBALL_L], // SPR_CFLM

	// Botanic Serenity Scenery
	&lspr[NOLIGHT],     // SPR_BSZ1
	&lspr[NOLIGHT],     // SPR_BSZ2
	&lspr[NOLIGHT],     // SPR_BSZ3
	//&lspr[NOLIGHT],     -- SPR_BSZ4
	&lspr[NOLIGHT],     // SPR_BST1
	&lspr[NOLIGHT],     // SPR_BST2
	&lspr[NOLIGHT],     // SPR_BST3
	&lspr[NOLIGHT],     // SPR_BST4
	&lspr[NOLIGHT],     // SPR_BST5
	&lspr[NOLIGHT],     // SPR_BST6
	&lspr[NOLIGHT],     // SPR_BSZ5
	&lspr[NOLIGHT],     // SPR_BSZ6
	&lspr[NOLIGHT],     // SPR_BSZ7
	&lspr[NOLIGHT],     // SPR_BSZ8

	// Misc Scenery
	&lspr[NOLIGHT],     // SPR_STLG
	&lspr[NOLIGHT],     // SPR_DBAL

	// Powerup Indicators
	&lspr[NOLIGHT],     // SPR_ARMA
	&lspr[NOLIGHT],     // SPR_ARMF
	&lspr[NOLIGHT],     // SPR_ARMB
	&lspr[NOLIGHT],     // SPR_WIND
	&lspr[NOLIGHT],     // SPR_MAGN
	&lspr[NOLIGHT],     // SPR_ELEM
	&lspr[NOLIGHT],     // SPR_FORC
	&lspr[NOLIGHT],     // SPR_PITY
	&lspr[NOLIGHT],     // SPR_FIRS
	&lspr[NOLIGHT],     // SPR_BUBS
	&lspr[NOLIGHT],     // SPR_ZAPS
	&lspr[INVINCIBLE_L], // SPR_IVSP
	&lspr[SUPERSPARK_L], // SPR_SSPK

	&lspr[NOLIGHT],     // SPR_GOAL

	// Flickies
	&lspr[NOLIGHT],     // SPR_FBUB
	&lspr[NOLIGHT],     // SPR_FL01
	&lspr[NOLIGHT],     // SPR_FL02
	&lspr[NOLIGHT],     // SPR_FL03
	&lspr[NOLIGHT],     // SPR_FL04
	&lspr[NOLIGHT],     // SPR_FL05
	&lspr[NOLIGHT],     // SPR_FL06
	&lspr[NOLIGHT],     // SPR_FL07
	&lspr[NOLIGHT],     // SPR_FL08
	&lspr[NOLIGHT],     // SPR_FL09
	&lspr[NOLIGHT],     // SPR_FL10
	&lspr[NOLIGHT],     // SPR_FL11
	&lspr[NOLIGHT],     // SPR_FL12
	&lspr[NOLIGHT],     // SPR_FL13
	&lspr[NOLIGHT],     // SPR_FL14
	&lspr[NOLIGHT],     // SPR_FL15
	&lspr[NOLIGHT],     // SPR_FL16
	&lspr[NOLIGHT],     // SPR_FS01
	&lspr[NOLIGHT],     // SPR_FS02

	// Springs
	&lspr[NOLIGHT],     // SPR_FANS
	&lspr[NOLIGHT],     // SPR_STEM
	&lspr[NOLIGHT],     // SPR_BUMP
	&lspr[NOLIGHT],     // SPR_BLON
	&lspr[NOLIGHT],     // SPR_SPRY
	&lspr[NOLIGHT],     // SPR_SPRR
	&lspr[NOLIGHT],     // SPR_SPRB
	&lspr[NOLIGHT],     // SPR_YSPR
	&lspr[NOLIGHT],     // SPR_RSPR
	&lspr[NOLIGHT],     // SPR_BSPR
	&lspr[NOLIGHT],     // SPR_SSWY
	&lspr[NOLIGHT],     // SPR_SSWR
	&lspr[NOLIGHT],     // SPR_SSWB
	&lspr[NOLIGHT],     // SPR_BSTY
	&lspr[NOLIGHT],     // SPR_BSTR

	// Environmental Effects
	&lspr[NOLIGHT],     // SPR_RAIN
	&lspr[NOLIGHT],     // SPR_SNO1
	&lspr[NOLIGHT],     // SPR_SPLH
	&lspr[NOLIGHT],     // SPR_LSPL
	&lspr[NOLIGHT],     // SPR_SPLA
	&lspr[NOLIGHT],     // SPR_SMOK
	&lspr[NOLIGHT],     // SPR_BUBL
	&lspr[RINGLIGHT_L], // SPR_WZAP
	&lspr[NOLIGHT],     // SPR_DUST
	&lspr[NOLIGHT],     // SPR_FPRT
	&lspr[SUPERSPARK_L], // SPR_TFOG
	&lspr[NIGHTSLIGHT_L], // SPR_SEED
	&lspr[NOLIGHT],     // SPR_PRTL

	// Game Indicators
	&lspr[NOLIGHT],     // SPR_SCOR
	&lspr[NOLIGHT],     // SPR_DRWN
	&lspr[NOLIGHT],     // SPR_FLII
	&lspr[NOLIGHT],     // SPR_LCKN
	&lspr[NOLIGHT],     // SPR_TTAG
	&lspr[NOLIGHT],     // SPR_GFLG
	&lspr[NOLIGHT],     // SPR_FNSF

	&lspr[NOLIGHT],     // SPR_CORK
	&lspr[NOLIGHT],     // SPR_LHRT

	// Ring Weapons
	&lspr[RINGLIGHT_L],     // SPR_RRNG
	&lspr[RINGLIGHT_L],     // SPR_RNGB
	&lspr[RINGLIGHT_L],     // SPR_RNGR
	&lspr[RINGLIGHT_L],     // SPR_RNGI
	&lspr[RINGLIGHT_L],     // SPR_RNGA
	&lspr[RINGLIGHT_L],     // SPR_RNGE
	&lspr[RINGLIGHT_L],     // SPR_RNGS
	&lspr[RINGLIGHT_L],     // SPR_RNGG

	&lspr[RINGLIGHT_L],     // SPR_PIKB
	&lspr[RINGLIGHT_L],     // SPR_PIKR
	&lspr[RINGLIGHT_L],     // SPR_PIKA
	&lspr[RINGLIGHT_L],     // SPR_PIKE
	&lspr[RINGLIGHT_L],     // SPR_PIKS
	&lspr[RINGLIGHT_L],     // SPR_PIKG

	&lspr[RINGLIGHT_L],     // SPR_TAUT
	&lspr[RINGLIGHT_L],     // SPR_TGRE
	&lspr[RINGLIGHT_L],     // SPR_TSCR

	// Mario-specific stuff
	&lspr[NOLIGHT],     // SPR_COIN
	&lspr[NOLIGHT],     // SPR_CPRK
	&lspr[NOLIGHT],     // SPR_GOOM
	&lspr[NOLIGHT],     // SPR_BGOM
	&lspr[REDBALL_L],     // SPR_FFWR
	&lspr[SMALLREDBALL_L], // SPR_FBLL
	&lspr[NOLIGHT],     // SPR_SHLL
	&lspr[REDBALL_L],   // SPR_PUMA
	&lspr[NOLIGHT],     // SPR_HAMM
	&lspr[NOLIGHT],     // SPR_KOOP
	&lspr[REDBALL_L],   // SPR_BFLM
	&lspr[NOLIGHT],     // SPR_MAXE
	&lspr[NOLIGHT],     // SPR_MUS1
	&lspr[NOLIGHT],     // SPR_MUS2
	&lspr[NOLIGHT],     // SPR_TOAD

	// NiGHTS Stuff
	&lspr[SUPERSONIC_L], // SPR_NDRN // NiGHTS drone
	&lspr[NOLIGHT],     // SPR_NSPK
	&lspr[NOLIGHT],     // SPR_NBMP
	&lspr[NOLIGHT],     // SPR_HOOP
	&lspr[NOLIGHT],     // SPR_HSCR
	&lspr[NOLIGHT],     // SPR_NPRU
	&lspr[NOLIGHT],     // SPR_CAPS
	&lspr[INVINCIBLE_L], // SPR_IDYA
	&lspr[NOLIGHT],     // SPR_NTPN
	&lspr[NOLIGHT],     // SPR_SHLP

	// Secret badniks and hazards, shhhh
	&lspr[NOLIGHT],     // SPR_PENG
	&lspr[NOLIGHT],     // SPR_POPH,
	&lspr[NOLIGHT],     // SPR_HIVE
	&lspr[NOLIGHT],     // SPR_BUMB,
	&lspr[NOLIGHT],     // SPR_BBUZ
	&lspr[NOLIGHT],     // SPR_FMCE,
	&lspr[NOLIGHT],     // SPR_HMCE,
	&lspr[NOLIGHT],     // SPR_CACO,
	&lspr[BLUEBALL_L],  // SPR_BAL2,
	&lspr[NOLIGHT],     // SPR_SBOB,
	&lspr[BLUEBALL_L],  // SPR_SBFL,
	&lspr[BLUEBALL_L],  // SPR_SBSK,
	&lspr[NOLIGHT],     // SPR_BATT,

	// Debris
	&lspr[RINGSPARK_L],  // SPR_SPRK
	&lspr[NOLIGHT],      // SPR_BOM1
	&lspr[SUPERSPARK_L], // SPR_BOM2
	&lspr[SUPERSPARK_L], // SPR_BOM3
	&lspr[NOLIGHT],      // SPR_BOM4
	&lspr[REDBALL_L],    // SPR_BMNB

	// Crumbly rocks
	&lspr[NOLIGHT],     // SPR_ROIA
	&lspr[NOLIGHT],     // SPR_ROIB
	&lspr[NOLIGHT],     // SPR_ROIC
	&lspr[NOLIGHT],     // SPR_ROID
	&lspr[NOLIGHT],     // SPR_ROIE
	&lspr[NOLIGHT],     // SPR_ROIF
	&lspr[NOLIGHT],     // SPR_ROIG
	&lspr[NOLIGHT],     // SPR_ROIH
	&lspr[NOLIGHT],     // SPR_ROII
	&lspr[NOLIGHT],     // SPR_ROIJ
	&lspr[NOLIGHT],     // SPR_ROIK
	&lspr[NOLIGHT],     // SPR_ROIL
	&lspr[NOLIGHT],     // SPR_ROIM
	&lspr[NOLIGHT],     // SPR_ROIN
	&lspr[NOLIGHT],     // SPR_ROIO
	&lspr[NOLIGHT],     // SPR_ROIP

	// Level debris
	&lspr[NOLIGHT], // SPR_GFZD
	&lspr[NOLIGHT], // SPR_BRIC
	&lspr[NOLIGHT], // SPR_WDDB
	&lspr[NOLIGHT], // SPR_BRIR
	&lspr[NOLIGHT], // SPR_BRIB
	&lspr[NOLIGHT], // SPR_BRIY

	// Gravity Well Objects
	&lspr[NOLIGHT],     // SPR_GWLG
	&lspr[NOLIGHT],     // SPR_GWLR

	// Free slots
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
};

#ifdef ALAM_LIGHTING

//=============================================================================
//                                                                       PROTOS
//=============================================================================

static void HWR_SetLight(void);

// --------------------------------------------------------------------------
// calcul la projection d'un point sur une droite (determin� par deux
// points) et ensuite calcul la distance (au carr� de ce point au point
// project�sur cette droite
// --------------------------------------------------------------------------
static float HWR_DistP2D(FOutVector *p1, FOutVector *p2, FVector *p3, FVector *inter)
{
	if (p1->z == p2->z)
	{
		inter->x = p3->x;
		inter->z = p1->z;
	}
	else if (p1->x == p2->x)
	{
		inter->x = p1->x;
		inter->z = p3->z;
	}
	else
	{
		register float local, pente;
		// Wat een mooie formula! Hurdler's math;-)
		pente = (p1->z-p2->z) / (p1->x-p2->x);
		local = p1->z - p1->x*pente;
		inter->x = (p3->z - local + p3->x/pente) * (pente/(pente*pente+1));
		inter->z = inter->x*pente + local;
	}

	return (p3->x-inter->x)*(p3->x-inter->x) + (p3->z-inter->z)*(p3->z-inter->z);
}

// check if sphere (radius r) centred in p3 touch the bounding box defined by p1, p2
static boolean SphereTouchBBox3D(FOutVector *p1, FOutVector *p2, FVector *p3, float r)
{
	float minx = p1->x,maxx = p2->x,miny = p2->y,maxy = p1->y,minz = p2->z,maxz = p1->z;

	if (minx > maxx)
	{
		minx = maxx;
		maxx = p1->x;
	}
	if (miny > maxy)
	{
		miny = maxy;
		maxy = p2->y;
	}
	if (minz > maxz)
	{
		minz = maxz;
		maxz = p2->z;
	}

	if (minx-r > p3->x) return false;
	if (maxx+r < p3->x) return false;
	if (miny-r > p3->y) return false;
	if (maxy+r < p3->y) return false;
	if (minz-r > p3->z) return false;
	if (maxz+r < p3->z) return false;
	return true;
}

// Hurdler: The old code was removed by me because I don't think it will be used one day.
//          (It's still available on the CVS for educational purpose: Revision 1.8)

// --------------------------------------------------------------------------
// calcul du dynamic lighting sur les murs
// lVerts contient les coords du mur sans le mlook (up/down)
// --------------------------------------------------------------------------
void HWR_WallLighting(FOutVector *wlVerts)
{
	int             i, j;

	// dynlights->nb == 0 if cv_gldynamiclighting.value is not set
	for (j = 0; j < dynlights->nb; j++)
	{
		FVector         inter;
		FSurfaceInfo    Surf;
		float           dist_p2d, d[4], s;

		if (!dynlights->mo[j])
			continue;
		if (P_MobjWasRemoved(dynlights->mo[j]))
		{
			P_SetTarget(&dynlights->mo[j], NULL);
			continue;
		}

		// check bounding box first
		if (SphereTouchBBox3D(&wlVerts[2], &wlVerts[0], &LIGHT_POS(j), DL_RADIUS(j))==false)
			continue;
		d[0] = wlVerts[2].x - wlVerts[0].x;
		d[1] = wlVerts[2].z - wlVerts[0].z;
		d[2] = LIGHT_POS(j).x - wlVerts[0].x;
		d[3] = LIGHT_POS(j).z - wlVerts[0].z;
		// backface cull
		if (d[2]*d[1] - d[3]*d[0] < 0)
			continue;
		// check exact distance
		dist_p2d = HWR_DistP2D(&wlVerts[2], &wlVerts[0],  &LIGHT_POS(j), &inter);
		if (dist_p2d >= DL_SQRRADIUS(j))
			continue;

		d[0] = (float)sqrt((wlVerts[0].x-inter.x)*(wlVerts[0].x-inter.x)
			+ (wlVerts[0].z-inter.z)*(wlVerts[0].z-inter.z));
		d[1] = (float)sqrt((wlVerts[2].x-inter.x)*(wlVerts[2].x-inter.x)
			+ (wlVerts[2].z-inter.z)*(wlVerts[2].z-inter.z));
		//dAB = sqrtf((wlVerts[0].x-wlVerts[2].x)*(wlVerts[0].x-wlVerts[2].x)+(wlVerts[0].z-wlVerts[2].z)*(wlVerts[0].z-wlVerts[2].z));
		//if ((d[0] < dAB) && (d[1] < dAB)) // test if the intersection is on the wall
		//{
		//    d[0] = -d[0]; // if yes, the left distcance must be negative for texcoord
		//}
		// test if the intersection is on the wall
		if ((wlVerts[0].x < inter.x && wlVerts[2].x > inter.x) ||
		     (wlVerts[0].x > inter.x && wlVerts[2].x < inter.x) ||
		     (wlVerts[0].z < inter.z && wlVerts[2].z > inter.z) ||
		     (wlVerts[0].z > inter.z && wlVerts[2].z < inter.z))
		{
			d[0] = -d[0]; // if yes, the left distcance must be negative for texcoord
		}
		d[2] = d[1]; d[3] = d[0];
#ifdef DL_HIGH_QUALITY
		s = 0.5f / DL_RADIUS(j);
#else
		s = 0.5f / sqrtf(DL_SQRRADIUS(j)-dist_p2d);
#endif
		for (i = 0; i < 4; i++)
		{
			wlVerts[i].s = (float)(0.5f + d[i]*s);
			wlVerts[i].t = (float)(0.5f + (wlVerts[i].y-LIGHT_POS(j).y)*s*1.2f);
		}

		HWR_SetLight();

		Surf.PolyColor.rgba = LONG(dynlights->p_lspr[j]->dynamic_color);
#ifdef DL_HIGH_QUALITY
		Surf.PolyColor.s.alpha = (UINT8)((1-dist_p2d/DL_SQRRADIUS(j))*Surf.PolyColor.s.alpha);
#endif
		// next state is null so fade out with alpha
		if (dynlights->mo[j]->state->nextstate == S_NULL)
			Surf.PolyColor.s.alpha = (UINT8)(((float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics)*Surf.PolyColor.s.alpha);

		HWD.pfnDrawPolygon (&Surf, wlVerts, 4, LIGHTMAPFLAGS);

	} // end for (j = 0; j < dynlights->nb; j++)
}

// --------------------------------------------------------------------------
// calcul du dynamic lighting sur le sol
// clVerts contient les coords du sol avec le mlook (up/down)
// --------------------------------------------------------------------------
void HWR_PlaneLighting(FOutVector *clVerts, int nrClipVerts)
{
	int     i, j;
	FOutVector p1,p2;

	p1.z = FIXED_TO_FLOAT(hwbbox[BOXTOP   ]);
	p1.x = FIXED_TO_FLOAT(hwbbox[BOXLEFT  ]);
	p2.z = FIXED_TO_FLOAT(hwbbox[BOXBOTTOM]);
	p2.x = FIXED_TO_FLOAT(hwbbox[BOXRIGHT ]);
	p2.y = clVerts[0].y;
	p1.y = clVerts[0].y;

	for (j = 0; j < dynlights->nb; j++)
	{
		FSurfaceInfo    Surf;
		float           dist_p2d, s;

		if (!dynlights->mo[j])
			continue;
		if (P_MobjWasRemoved(dynlights->mo[j]))
		{
			P_SetTarget(&dynlights->mo[j], NULL);
			continue;
		}

		// BP: The kickass Optimization: check if light touch bounding box
		if (SphereTouchBBox3D(&p1, &p2, &dynlights->position[j], DL_RADIUS(j))==false)
			continue;
		// backface cull
		//Hurdler: doesn't work with new TANDL code
		if ((clVerts[0].y > atransform.z)       // true mean it is a ceiling false is a floor
			 ^ (LIGHT_POS(j).y < clVerts[0].y)) // true mean light is down plane false light is up plane
			 continue;
		dist_p2d = (clVerts[0].y-LIGHT_POS(j).y);
		dist_p2d *= dist_p2d;
		// done in SphereTouchBBox3D
		//if (dist_p2d >= DL_SQRRADIUS(j))
		//    continue;

#ifdef DL_HIGH_QUALITY
		s = 0.5f / DL_RADIUS(j);
#else
		s = 0.5f / sqrtf(DL_SQRRADIUS(j)-dist_p2d);
#endif
		for (i = 0; i < nrClipVerts; i++)
		{
			clVerts[i].s = 0.5f + (clVerts[i].x-LIGHT_POS(j).x)*s;
			clVerts[i].t = 0.5f + (clVerts[i].z-LIGHT_POS(j).z)*s*1.2f;
		}

		HWR_SetLight();

		Surf.PolyColor.rgba = LONG(dynlights->p_lspr[j]->dynamic_color);
#ifdef DL_HIGH_QUALITY
		Surf.PolyColor.s.alpha = (unsigned char)((1 - dist_p2d/DL_SQRRADIUS(j))*Surf.PolyColor.s.alpha);
#endif
		// next state is null so fade out with alpha
		if ((dynlights->mo[j]->state->nextstate == S_NULL))
			Surf.PolyColor.s.alpha = (unsigned char)(((float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics)*Surf.PolyColor.s.alpha);

		HWD.pfnDrawPolygon (&Surf, clVerts, nrClipVerts, LIGHTMAPFLAGS);

	} // end for (j = 0; j < dynlights->nb; j++)
}


static lumpnum_t coronalumpnum = LUMPERROR;
#ifndef NEWCORONAS
// --------------------------------------------------------------------------
// coronas lighting
// --------------------------------------------------------------------------
void HWR_DoCoronasLighting(FOutVector *outVerts, gl_vissprite_t *spr)
{
	light_t   *p_lspr;

	if (coronalumpnum == LUMPERROR)
		return;

	//CONS_Debug(DBG_RENDER, "sprite (type): %d (%s)\n", spr->type, sprnames[spr->type]);
	p_lspr = t_lspr[spr->mobj->sprite];
	if ((spr->mobj->state>=&states[S_EXPLODE1] && spr->mobj->state<=&states[S_EXPLODE3])
	 || (spr->mobj->state>=&states[S_FATSHOTX1] && spr->mobj->state<=&states[S_FATSHOTX3]))
	{
		p_lspr = &lspr[ROCKETEXP_L];
	}

	if (cv_glcoronas.value && (p_lspr->type & CORONA_SPR))
	{ // it's an object which emits light
		FOutVector      light[4];
		FSurfaceInfo    Surf;
		float           cx = 0.0f, cy = 0.0f, cz = 0.0f; // gravity center
		float           size;
		UINT8           i;

		switch (p_lspr->type)
		{
			case LIGHT_SPR:
				size  = p_lspr->corona_radius  * ((outVerts[0].z+120.0f)/950.0f); // d'ou vienne ces constante ?
				break;
			case ROCKET_SPR:
				p_lspr->corona_color = (((M_RandomByte()>>1)&0xff)<<24)|0x0040ff;
				// don't need a break
			case CORONA_SPR:
				size  = p_lspr->corona_radius  * ((outVerts[0].z+60.0f)/100.0f); // d'ou vienne ces constante ?
				break;
			default:
				I_Error("HWR_DoCoronasLighting: unknow light type %d",p_lspr->type);
				return;
		}
		if (size > p_lspr->corona_radius)
			size = p_lspr->corona_radius;
		size *= FIXED_TO_FLOAT(cv_glcoronasize.value<<1);

		// compute position doing average
		for (i = 0; i < 4; i++)
		{
			cx += outVerts[i].x;
			cy += outVerts[i].y;
			cz += outVerts[i].z;
		}
		cx /= 4.0f;  cy /= 4.0f;  cz /= 4.0f;

		// more realistique corona !
		if (cz >= 255*8+250)
			return;
		Surf.PolyColor.rgba = p_lspr->corona_color;
		if (cz > 250.0f)
			Surf.PolyColor.s.alpha = 0xff-((int)cz-250)/8;
		else
			Surf.PolyColor.s.alpha = 0xff;

		// do not be hide by sprite of the light itself !
		cz = cz - 2.0f;

		// Bp; je comprend pas, ou est la rotation haut/bas ?
		//     tu ajoute un offset a y mais si la tu la reguarde de haut
		//     sa devrais pas marcher ... comprend pas :(
		//     (...) bon je croit que j'ai comprit il est tout pourit le code ?
		//           car comme l'offset est minime sa ce voit pas !
		light[0].x = cx-size;  light[0].z = cz;
		light[0].y = cy-size*1.33f+p_lspr->light_yoffset;
		light[0].s = 0.0f;   light[0].t = 0.0f;

		light[1].x = cx+size;  light[1].z = cz;
		light[1].y = cy-size*1.33f+p_lspr->light_yoffset;
		light[1].s = 1.0f;   light[1].t = 0.0f;

		light[2].x = cx+size;  light[2].z = cz;
		light[2].y = cy+size*1.33f+p_lspr->light_yoffset;
		light[2].s = 1.0f;   light[2].t = 1.0f;

		light[3].x = cx-size;  light[3].z = cz;
		light[3].y = cy+size*1.33f+p_lspr->light_yoffset;
		light[3].s = 0.0f;   light[3].t = 1.0f;

		HWR_GetPic(coronalumpnum);  /// \todo use different coronas

		HWD.pfnDrawPolygon (&Surf, light, 4, PF_Modulated | PF_Additive | PF_Corona | PF_NoDepthTest);
	}
}
#endif

#ifdef NEWCORONAS
// use the lightlist of the frame to draw the coronas at the top of everythink
void HWR_DrawCoronas(void)
{
	int       j;

	if (!cv_glcoronas.value || dynlights->nb <= 0 || coronalumpnum == LUMPERROR)
		return;

	HWR_GetPic(coronalumpnum);  /// \todo use different coronas
	for (j = 0;j < dynlights->nb;j++)
	{
		FOutVector      light[4];
		FSurfaceInfo    Surf;
		float           cx = LIGHT_POS(j).x;
		float           cy = LIGHT_POS(j).y;
		float           cz = LIGHT_POS(j).z; // gravity center
		float           size;
		light_t         *p_lspr = dynlights->p_lspr[j];

		// it's an object which emits light
		if (!(p_lspr->type & CORONA_SPR))
			continue;

		if (!dynlights->mo[j])
			continue;
		if (P_MobjWasRemoved(dynlights->mo[j]))
		{
			P_SetTarget(&dynlights->mo[j], NULL);
			continue;
		}

		transform(&cx,&cy,&cz);

		// more realistique corona !
		if (cz >= 255*8+250)
			continue;
		Surf.PolyColor.rgba = p_lspr->corona_color;
		if (cz > 250.0f)
			Surf.PolyColor.s.alpha = (UINT8)(0xff-(UINT8)(((int)cz-250)/8));
		else
			Surf.PolyColor.s.alpha = 0xff;

		switch (p_lspr->type)
		{
			case LIGHT_SPR:
				size  = p_lspr->corona_radius  * ((cz+120.0f)/950.0f); // d'ou vienne ces constante ?
				break;
			case ROCKET_SPR:
				Surf.PolyColor.s.alpha = (UINT8)((M_RandomByte()>>1)&0xff);
				// don't need a break
			case CORONA_SPR:
				size  = p_lspr->corona_radius  * ((cz+60.0f)/100.0f); // d'ou vienne ces constante ?
				break;
			default:
				I_Error("HWR_DoCoronasLighting: unknow light type %d",p_lspr->type);
				continue;
		}
		if (size > p_lspr->corona_radius)
			size = p_lspr->corona_radius;
		size = (float)(FIXED_TO_FLOAT(cv_glcoronasize.value<<1)*size);

		// put light little forward the sprite so there is no
		// z-buffer problem (coplanar polygons)
		// BP: use PF_Decal do not help :(
		cz = cz - 5.0f;

		light[0].x = cx-size;  light[0].z = cz;
		light[0].y = cy-size*1.33f;
		light[0].s = 0.0f;   light[0].t = 0.0f;

		light[1].x = cx+size;  light[1].z = cz;
		light[1].y = cy-size*1.33f;
		light[1].s = 1.0f;   light[1].t = 0.0f;

		light[2].x = cx+size;  light[2].z = cz;
		light[2].y = cy+size*1.33f;
		light[2].s = 1.0f;   light[2].t = 1.0f;

		light[3].x = cx-size;  light[3].z = cz;
		light[3].y = cy+size*1.33f;
		light[3].s = 0.0f;   light[3].t = 1.0f;

		HWD.pfnDrawPolygon (&Surf, light, 4, PF_Modulated | PF_Additive | PF_NoDepthTest | PF_Corona);
	}
}
#endif

// --------------------------------------------------------------------------
// Remove all the dynamic lights at eatch frame
// --------------------------------------------------------------------------
void HWR_ResetLights(void)
{
	while (dynlights->nb)
		P_SetTarget(&dynlights->mo[--dynlights->nb], NULL);
}

// --------------------------------------------------------------------------
// Change view, thus change lights (splitscreen)
// --------------------------------------------------------------------------
void HWR_SetLights(int viewnumber)
{
	dynlights = &view_dynlights[viewnumber];
}

// --------------------------------------------------------------------------
// Add a light for dynamic lighting
// The light position is already transformed execpt for mlook
// --------------------------------------------------------------------------
void HWR_DL_AddLight(gl_vissprite_t *spr, GLPatch_t *patch)
{
	light_t   *p_lspr;

	//Hurdler: moved here because it's better;-)
	(void)patch;
	if (!cv_gldynamiclighting.value)
		return;

	if (!spr->mobj)
#ifdef PARANOIA
		I_Error("vissprite without mobj !!!");
#else
		return;
#endif

	if (dynlights->nb >= DL_MAX_LIGHT)
		return;

	// check if sprite contain dynamic light
	p_lspr = t_lspr[spr->mobj->sprite];
	if (!(p_lspr->type & DYNLIGHT_SPR))
		return;
	if ((p_lspr->type != LIGHT_SPR) || cv_glstaticlighting.value)
		return;

	LIGHT_POS(dynlights->nb).x = FIXED_TO_FLOAT(spr->mobj->x);
	LIGHT_POS(dynlights->nb).y = FIXED_TO_FLOAT(spr->mobj->z)+FIXED_TO_FLOAT(spr->mobj->height>>1)+p_lspr->light_yoffset;
	LIGHT_POS(dynlights->nb).z = FIXED_TO_FLOAT(spr->mobj->y);

	P_SetTarget(&dynlights->mo[dynlights->nb], spr->mobj);

	dynlights->p_lspr[dynlights->nb] = p_lspr;

	dynlights->nb++;
}

static GLMipmap_t lightmappatchmipmap;
static GLPatch_t lightmappatch = { .mipmap = &lightmappatchmipmap };

void HWR_InitLight(void)
{
	size_t i;

	// precalculate sqr radius
	for (i = 0;i < NUMLIGHTS;i++)
		lspr[i].dynamic_sqrradius = lspr[i].dynamic_radius*lspr[i].dynamic_radius;

	lightmappatch.mipmap->downloaded = false;
	coronalumpnum = W_CheckNumForName("CORONA");
}

// -----------------+
// HWR_SetLight     : Download a disc shaped alpha map for rendering fake lights
// -----------------+
static void HWR_SetLight(void)
{
	int    i, j;

	if (!lightmappatch.mipmap->downloaded && !lightmappatch.mipmap->data)
	{

		UINT16 *Data = Z_Malloc(129*128*sizeof (UINT16), PU_HWRCACHE, &lightmappatch.mipmap->data);

		for (i = 0; i < 128; i++)
		{
			for (j = 0; j < 128; j++)
			{
				int pos = ((i-64)*(i-64))+((j-64)*(j-64));
				if (pos <= 63*63)
					Data[i*128+j] = (UINT16)(((UINT8)(255-(4*(float)sqrt((float)pos)))) << 8 | 0xff);
				else
					Data[i*128+j] = 0;
			}
		}
		lightmappatch.mipmap->format = GL_TEXFMT_ALPHA_INTENSITY_88;

		lightmappatch.width = 128;
		lightmappatch.height = 128;
		lightmappatch.mipmap->width = 128;
		lightmappatch.mipmap->height = 128;
		lightmappatch.mipmap->flags = 0; //TF_WRAPXY; // DEBUG: view the overdraw !
	}
	HWD.pfnSetTexture(lightmappatch.mipmap);

	// The system-memory data can be purged now.
	Z_ChangeTag(lightmappatch.mipmap->data, PU_HWRCACHE_UNLOCKED);
}

//**********************************************************
// Hurdler: new code for faster static lighting and and T&L
//**********************************************************

#ifdef STATICLIGHT
// is this really necessary?
static sector_t *lgl_backsector;
static seg_t *lgl_curline;
#endif

// p1 et p2 c'est le deux bou du seg en float
static inline void HWR_BuildWallLightmaps(FVector *p1, FVector *p2, int lighnum, seg_t *line)
{
	lightmap_t *lp;

	// (...) calcul presit de la projection et de la distance

//	if (dist_p2d >= DL_SQRRADIUS(lightnum))
//		return;

	// (...) attention faire le backfase cull histoir de faire mieux que Q3 !

	(void)lighnum;
	(void)p1;
	(void)p2;
	lp = malloc(sizeof (*lp));
	lp->next = line->lightmaps;
	line->lightmaps = lp;

	// (...) encore des b�calcul bien lourd et on stock tout sa dans la lightmap
}

#ifdef STATICLIGHT
static void HWR_AddLightMapForLine(int lightnum, seg_t *line)
{
	/*
	int                 x1;
	int                 x2;
	angle_t             angle1;
	angle_t             angle2;
	angle_t             span;
	angle_t             tspan;
	*/
	FVector             p1,p2;

	lgl_curline = line;
	lgl_backsector = line->backsector;

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides,
	//  identical light levels on both sides,
	//  and no middle texture.
/*
	if ( lgl_backsector->ceilingpic == gl_frontsector->ceilingpic
	  && lgl_backsector->floorpic == gl_frontsector->floorpic
	  && lgl_backsector->lightlevel == gl_frontsector->lightlevel
	  && lgl_curline->sidedef->midtexture == 0)
	{
		return;
	}
*/

	p1.y = FIXED_TO_FLOAT(lgl_curline->v1->y);
	p1.x = FIXED_TO_FLOAT(lgl_curline->v1->x);
	p2.y = FIXED_TO_FLOAT(lgl_curline->v2->y);
	p2.x = FIXED_TO_FLOAT(lgl_curline->v2->x);

	// check bbox of the seg
//	if (CircleTouchBBox(&p1, &p2, &LIGHT_POS(lightnum), DL_RADIUS(lightnum))==false)
//		return;

	HWR_BuildWallLightmaps(&p1, &p2, lightnum, line);
}

/// \todo see what HWR_AddLine does
static void HWR_CheckSubsector(size_t num, fixed_t *bbox)
{
	int         count;
	seg_t       *line;
	subsector_t *sub;
	FVector     p1,p2;
	int         lightnum;

	p1.y = FIXED_TO_FLOAT(bbox[BOXTOP   ]);
	p1.x = FIXED_TO_FLOAT(bbox[BOXLEFT  ]);
	p2.y = FIXED_TO_FLOAT(bbox[BOXBOTTOM]);
	p2.x = FIXED_TO_FLOAT(bbox[BOXRIGHT ]);


	if (num < numsubsectors)
	{
		sub = &subsectors[num];         // subsector
		for (lightnum = 0; lightnum < dynlights->nb; lightnum++)
		{
//			if (CircleTouchBBox(&p1, &p2, &LIGHT_POS(lightnum), DL_RADIUS(lightnum))==false)
//				continue;

			if (!dynlights->mo[lightnum])
				continue;
			if (P_MobjWasRemoved(dynlights->mo[lightnum]))
			{
				P_SetTarget(&dynlights->mo[lightnum], NULL);
				continue;
			}

			count = sub->numlines;          // how many linedefs
			line = &segs[sub->firstline];   // first line seg
			while (count--)
			{
				HWR_AddLightMapForLine (lightnum, line);       // compute lightmap
				line++;
			}
		}
	}
}


// --------------------------------------------------------------------------
// Hurdler: this adds lights by mobj.
// --------------------------------------------------------------------------
static void HWR_AddMobjLights(mobj_t *thing)
{
	if (dynlights->nb >= DL_MAX_LIGHT)
		return;
	if (!(t_lspr[thing->sprite]->type & CORONA_SPR))
		return;

	LIGHT_POS(dynlights->nb).x = FIXED_TO_FLOAT(thing->x);
	LIGHT_POS(dynlights->nb).y = FIXED_TO_FLOAT(thing->z) + t_lspr[thing->sprite]->light_yoffset;
	LIGHT_POS(dynlights->nb).z = FIXED_TO_FLOAT(thing->y);

	P_SetTarget(&dynlights->mo[dynlights->nb], thing);

	dynlights->p_lspr[dynlights->nb] = t_lspr[thing->sprite];

	dynlights->nb++;
}

//Hurdler: The goal of this function is to walk through all the bsp starting
//         on the top.
//         We need to do that to know all the lights in the map and all the walls
static void HWR_ComputeLightMapsInBSPNode(int bspnum, fixed_t *bbox)
{
	if (bspnum & NF_SUBSECTOR) // Found a subsector?
	{
		if (bspnum == -1)
			HWR_CheckSubsector(0, bbox);  // probably unecessary: see boris' comment in hw_bsp
		else
			HWR_CheckSubsector(bspnum&(~NF_SUBSECTOR), bbox);
		return;
	}
	HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[0], nodes[bspnum].bbox[0]);
	HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[1], nodes[bspnum].bbox[1]);
}

static void HWR_SearchLightsInMobjs(void)
{
	thinker_t *         th;
	//mobj_t *            mobj;

	// search in the list of thinkers
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			HWR_AddMobjLights((mobj_t *)th);
}
#endif

//
// HWR_CreateStaticLightmaps()
//
void HWR_CreateStaticLightmaps(int bspnum)
{
#ifdef STATICLIGHT
	CONS_Debug(DBG_RENDER, "HWR_CreateStaticLightmaps\n");

	HWR_ResetLights();

	// First: Searching for lights
	// BP: if i was you, I will make it in create mobj since mobj can be create
	//     at runtime now with fragle scipt
	HWR_SearchLightsInMobjs();
	CONS_Debug(DBG_RENDER, "%d lights found\n", dynlights->nb);

	// Second: Build all lightmap for walls covered by lights
	validcount++; // to be sure
	HWR_ComputeLightMapsInBSPNode(bspnum, NULL);
#else
	(void)bspnum;
#endif
}

/**
 \todo

  - Les coronas ne sont pas g�er avec le nouveau systeme, seul le dynamic lighting l'est
  - calculer l'offset des coronas au chargement du level et non faire la moyenne
	au moment de l'afficher
	 BP: euh non en fait il faux encoder la position de la light dans le sprite
		 car c'est pas focement au mileux de plus il peut en y avoir plusieur (chandelier)
  - changer la comparaison pour l'affichage des coronas (+ un epsilon)
	BP: non non j'ai trouver mieux :) : lord du AddSprite tu rajoute aussi la coronas
		dans la sprite list ! avec un z de epsilon (attention au ZCLIP_PLANE) et donc on
		l'affiche en dernier histoir qu'il puisse etre cacher par d'autre sprite :)
		Bon fait metre pas mal de code special dans hwr_project sprite mais sa vaux le
		coup
  - gerer dynamic et static : retenir le nombre de lightstatic et clearer toute les
		light > lightstatic (les dynamique) et les lightmap correspondant dans les segs
		puit refaire une passe avec le code si dessus mais rien que pour les dynamiques
		(tres petite modification)
  - finalement virer le hack splitscreen, il n'est plus necessaire !
*/
#endif
#endif // HWRENDER
