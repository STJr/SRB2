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
/// \file  sounds.h
/// \brief Sound and music info

#ifndef __SOUNDS__
#define __SOUNDS__

// Customisable sounds for Skins
typedef enum
{
	SKSSPIN,
	SKSPUTPUT,
	SKSPUDPUD,
	SKSPLPAN1, // Ouchies
	SKSPLPAN2,
	SKSPLPAN3,
	SKSPLPAN4,
	SKSPLDET1, // Deaths
	SKSPLDET2,
	SKSPLDET3,
	SKSPLDET4,
	SKSPLVCT1, // Victories
	SKSPLVCT2,
	SKSPLVCT3,
	SKSPLVCT4,
	SKSTHOK,
	SKSSPNDSH,
	SKSZOOM,
	SKSSKID,
	SKSGASP,
	SKSJUMP,
	NUMSKINSOUNDS
} skinsound_t;

// free sfx for S_AddSoundFx()
#define NUMSFXFREESLOTS 800 // Matches SOC Editor.
#define NUMSKINSFXSLOTS (MAXSKINS*NUMSKINSOUNDS)

//
// SoundFX struct.
//
typedef struct sfxinfo_struct sfxinfo_t;

struct sfxinfo_struct
{
	// up to 6-character name
	const char *name;

	// Sfx singularity (only one at a time)
	boolean singularity;

	// Sfx priority
	INT32 priority;

	// pitch if a link
	INT32 pitch;

	// volume if a link
	INT32 volume;

	// sound data
	void *data;

	// length of sound data
	size_t length;

	// sound that can be remapped for a skin, indexes skins[].skinsounds
	// 0 up to (NUMSKINSOUNDS-1), -1 = not skin specifc
	INT32 skinsound;

	// this is checked every second to see if sound
	// can be thrown out (if 0, then decrement, if -1,
	// then throw out, if > 0, then it is in use)
	INT32 usefulness;

	// lump number of sfx
	lumpnum_t lumpnum;
};

// the complete set of sound effects
extern sfxinfo_t S_sfx[];

//
// Identifiers for all sfx in game.
//

// List of sounds (don't modify this comment!)
typedef enum
{
	sfx_None,

	// Skin Sounds
	sfx_altdi1,
	sfx_altdi2,
	sfx_altdi3,
	sfx_altdi4,
	sfx_altow1,
	sfx_altow2,
	sfx_altow3,
	sfx_altow4,
	sfx_victr1,
	sfx_victr2,
	sfx_victr3,
	sfx_victr4,
	sfx_gasp,
	sfx_jump,
	sfx_pudpud,
	sfx_putput,
	sfx_spin,
	sfx_spndsh,
	sfx_thok,
	sfx_zoom,
	sfx_skid,

	// Ambience/background objects/etc
	sfx_ambint,

	sfx_alarm,
	sfx_buzz1,
	sfx_buzz2,
	sfx_buzz3,
	sfx_buzz4,
	sfx_crumbl,
	sfx_fire,
	sfx_grind,
	sfx_laser,
	sfx_mswing,
	sfx_pstart,
	sfx_pstop,
	sfx_steam1,
	sfx_steam2,
	sfx_wbreak,

	sfx_rainin,
	sfx_litng1,
	sfx_litng2,
	sfx_litng3,
	sfx_litng4,
	sfx_athun1,
	sfx_athun2,

	sfx_amwtr1,
	sfx_amwtr2,
	sfx_amwtr3,
	sfx_amwtr4,
	sfx_amwtr5,
	sfx_amwtr6,
	sfx_amwtr7,
	sfx_amwtr8,
	sfx_bubbl1,
	sfx_bubbl2,
	sfx_bubbl3,
	sfx_bubbl4,
	sfx_bubbl5,
	sfx_floush,
	sfx_splash,
	sfx_splish,
	sfx_wdrip1,
	sfx_wdrip2,
	sfx_wdrip3,
	sfx_wdrip4,
	sfx_wdrip5,
	sfx_wdrip6,
	sfx_wdrip7,
	sfx_wdrip8,
	sfx_wslap,

	sfx_doora1,
	sfx_doorb1,
	sfx_doorc1,
	sfx_doorc2,
	sfx_doord1,
	sfx_doord2,
	sfx_eleva1,
	sfx_eleva2,
	sfx_eleva3,
	sfx_elevb1,
	sfx_elevb2,
	sfx_elevb3,

	sfx_ambin2,
	sfx_lavbub,
	sfx_rocks1,
	sfx_rocks2,
	sfx_rocks3,
	sfx_rocks4,
	sfx_rumbam,
	sfx_rumble,

	// Game objects, etc
	sfx_appear,
	sfx_bkpoof,
	sfx_bnce1,
	sfx_bnce2,
	sfx_cannon,
	sfx_cgot,
	sfx_cybdth,
	sfx_deton,
	sfx_ding, // old sfx_appear sound
	sfx_dmpain,
	sfx_drown,
	sfx_fizzle,
	sfx_gbeep,
	sfx_gclose,
	sfx_ghit,
	sfx_gloop,
	sfx_gspray,
	sfx_gravch,
	sfx_itemup,
	sfx_jet,
	sfx_jshard,
	sfx_lose,
	sfx_lvpass,
	sfx_mindig,
	sfx_mixup,
	sfx_monton,
	sfx_pogo,
	sfx_pop,
	sfx_rail1,
	sfx_rail2,
	sfx_rlaunc,
	sfx_shield,
	sfx_wirlsg,
	sfx_forcsg,
	sfx_elemsg,
	sfx_armasg,
	sfx_attrsg,
	sfx_shldls,
	sfx_spdpad,
	sfx_spkdth,
	sfx_spring,
	sfx_statu1,
	sfx_statu2,
	sfx_strpst,
	sfx_supert,
	sfx_telept,
	sfx_tink,
	sfx_token,
	sfx_trfire,
	sfx_trpowr,
	sfx_turhit,
	sfx_wdjump,
	sfx_mswarp,
	sfx_mspogo,
	sfx_boingf,

	// Menu, interface
	sfx_chchng,
	sfx_dwnind,
	sfx_emfind,
	sfx_flgcap,
	sfx_menu1,
	sfx_oneup,
	sfx_ptally,
	sfx_radio,
	sfx_wepchg,
	sfx_wtrdng,
	sfx_zelda,

	// NiGHTS
	sfx_ideya,
	sfx_xideya, // Xmas
	sfx_nbmper,
	sfx_nxbump, // Xmas
	sfx_ncitem,
	sfx_nxitem, // Xmas
	sfx_ngdone,
	sfx_nxdone, // Xmas
	sfx_drill1,
	sfx_drill2,
	sfx_ncspec,
	sfx_nghurt,
	sfx_ngskid,
	sfx_hoop1,
	sfx_hoop2,
	sfx_hoop3,
	sfx_hidden,
	sfx_prloop,
	sfx_timeup, // Was gonna be played when less than ten seconds are on the clock; uncomment uses of this to see it in-context

	// Mario
	sfx_koopfr,
	sfx_mario1,
	sfx_mario2,
	sfx_mario3,
	sfx_mario4,
	sfx_mario5,
	sfx_mario6,
	sfx_mario7,
	sfx_mario8,
	sfx_mario9,
	sfx_marioa,
	sfx_thwomp,

	// Black Eggman
	sfx_bebomb,
	sfx_bechrg,
	sfx_becrsh,
	sfx_bedeen,
	sfx_bedie1,
	sfx_bedie2,
	sfx_beeyow,
	sfx_befall,
	sfx_befire,
	sfx_beflap,
	sfx_begoop,
	sfx_begrnd,
	sfx_behurt,
	sfx_bejet1,
	sfx_belnch,
	sfx_beoutb,
	sfx_beragh,
	sfx_beshot,
	sfx_bestep,
	sfx_bestp2,
	sfx_bewar1,
	sfx_bewar2,
	sfx_bewar3,
	sfx_bewar4,
	sfx_bexpld,
	sfx_bgxpld,

	// Cy-Brak-Demon
	sfx_beelec, // Electric barrier ambience
	sfx_brakrl, // Rocket launcher
	sfx_brakrx, // Rocket explodes

	// S3&K sounds
	sfx_s3k33,
	sfx_s3k34,
	sfx_s3k35,
	sfx_s3k36,
	sfx_s3k37,
	sfx_s3k38,
	sfx_s3k39,
	sfx_s3k3a,
	sfx_s3k3b,
	sfx_s3k3c,
	sfx_s3k3d,
	sfx_s3k3e,
	sfx_s3k3f,
	sfx_s3k40,
	sfx_s3k41,
	sfx_s3k42,
	sfx_s3k43,
	sfx_s3k44,
	sfx_s3k45,
	sfx_s3k46,
	sfx_s3k47,
	sfx_s3k48,
	sfx_s3k49,
	sfx_s3k4a,
	sfx_s3k4b,
	sfx_s3k4c,
	sfx_s3k4d,
	sfx_s3k4e,
	sfx_s3k4f,
	sfx_s3k50,
	sfx_s3k51,
	sfx_s3k52,
	sfx_s3k53,
	sfx_s3k54,
	sfx_s3k55,
	sfx_s3k56,
	sfx_s3k57,
	sfx_s3k58,
	sfx_s3k59,
	sfx_s3k5a,
	sfx_s3k5b,
	sfx_s3k5c,
	sfx_s3k5d,
	sfx_s3k5e,
	sfx_s3k5f,
	sfx_s3k60,
	sfx_s3k61,
	sfx_s3k62,
	sfx_s3k63,
	sfx_s3k64,
	sfx_s3k65,
	sfx_s3k66,
	sfx_s3k67,
	sfx_s3k68,
	sfx_s3k69,
	sfx_s3k6a,
	sfx_s3k6b,
	sfx_s3k6c,
	sfx_s3k6d,
	sfx_s3k6e,
	sfx_s3k6f,
	sfx_s3k70,
	sfx_s3k71,
	sfx_s3k72,
	sfx_s3k73,
	sfx_s3k74,
	sfx_s3k75,
	sfx_s3k76,
	sfx_s3k77,
	sfx_s3k78,
	sfx_s3k79,
	sfx_s3k7a,
	sfx_s3k7b,
	sfx_s3k7c,
	sfx_s3k7d,
	sfx_s3k7e,
	sfx_s3k7f,
	sfx_s3k80,
	sfx_s3k81,
	sfx_s3k82,
	sfx_s3k83,
	sfx_s3k84,
	sfx_s3k85,
	sfx_s3k86,
	sfx_s3k87,
	sfx_s3k88,
	sfx_s3k89,
	sfx_s3k8a,
	sfx_s3k8b,
	sfx_s3k8c,
	sfx_s3k8d,
	sfx_s3k8e,
	sfx_s3k8f,
	sfx_s3k90,
	sfx_s3k91,
	sfx_s3k92,
	sfx_s3k93,
	sfx_s3k94,
	sfx_s3k95,
	sfx_s3k96,
	sfx_s3k97,
	sfx_s3k98,
	sfx_s3k99,
	sfx_s3k9a,
	sfx_s3k9b,
	sfx_s3k9c,
	sfx_s3k9d,
	sfx_s3k9e,
	sfx_s3k9f,
	sfx_s3ka0,
	sfx_s3ka1,
	sfx_s3ka2,
	sfx_s3ka3,
	sfx_s3ka4,
	sfx_s3ka5,
	sfx_s3ka6,
	sfx_s3ka7,
	sfx_s3ka8,
	sfx_s3ka9,
	sfx_s3kaa,
	sfx_s3kab,
	sfx_s3kac,
	sfx_s3kad,
	sfx_s3kae,
	sfx_s3kaf,
	sfx_s3kb0,
	sfx_s3kb1,
	sfx_s3kb2,
	sfx_s3kb3,
	sfx_s3kb4,
	sfx_s3kb5,
	sfx_s3kb6,
	sfx_s3kb7,
	sfx_s3kb8,
	sfx_s3kb9,
	sfx_s3kba,
	sfx_s3kbb,
	sfx_s3kbcs,
	sfx_s3kbcl,
	sfx_s3kbds,
	sfx_s3kbdl,
	sfx_s3kbes,
	sfx_s3kbel,
	sfx_s3kbfs,
	sfx_s3kbfl,
	sfx_s3kc0s,
	sfx_s3kc0l,
	sfx_s3kc1s,
	sfx_s3kc1l,
	sfx_s3kc2s,
	sfx_s3kc2l,
	sfx_s3kc3s,
	sfx_s3kc3l,
	sfx_s3kc4s,
	sfx_s3kc4l,
	sfx_s3kc5s,
	sfx_s3kc5l,
	sfx_s3kc6s,
	sfx_s3kc6l,
	sfx_s3kc7,
	sfx_s3kc8s,
	sfx_s3kc8l,
	sfx_s3kc9s,
	sfx_s3kc9l,
	sfx_s3kcas,
	sfx_s3kcal,
	sfx_s3kcbs,
	sfx_s3kcbl,
	sfx_s3kccs,
	sfx_s3kccl,
	sfx_s3kcds,
	sfx_s3kcdl,
	sfx_s3kces,
	sfx_s3kcel,
	sfx_s3kcfs,
	sfx_s3kcfl,
	sfx_s3kd0s,
	sfx_s3kd0l,
	sfx_s3kd1s,
	sfx_s3kd1l,
	sfx_s3kd2s,
	sfx_s3kd2l,
	sfx_s3kd3s,
	sfx_s3kd3l,
	sfx_s3kd4s,
	sfx_s3kd4l,
	sfx_s3kd5s,
	sfx_s3kd5l,
	sfx_s3kd6s,
	sfx_s3kd6l,
	sfx_s3kd7s,
	sfx_s3kd7l,
	sfx_s3kd8s,
	sfx_s3kd8l,
	sfx_s3kd9s,
	sfx_s3kd9l,
	sfx_s3kdas,
	sfx_s3kdal,
	sfx_s3kdbs,
	sfx_s3kdbl,

	// free slots for S_AddSoundFx() at run-time --------------------
	sfx_freeslot0,
	//
	// ... 60 free sounds here ...
	//
	sfx_lastfreeslot = sfx_freeslot0 + NUMSFXFREESLOTS-1,
	// end of freeslots ---------------------------------------------

	sfx_skinsoundslot0,
	sfx_lastskinsoundslot = sfx_skinsoundslot0 + NUMSKINSFXSLOTS-1,
	NUMSFX
} sfxenum_t;


void S_InitRuntimeSounds(void);
sfxenum_t S_AddSoundFx(const char *name, boolean singular, INT32 flags, boolean skinsound);
void S_RemoveSoundFx(sfxenum_t id);

#endif
