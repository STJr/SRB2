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
/// \file  s_sound.h
/// \brief The not so system specific sound interface

#ifndef __S_SOUND__
#define __S_SOUND__

#include "i_sound.h" // musictype_t
#include "sounds.h"
#include "m_fixed.h"
#include "command.h"
#include "tables.h" // angle_t

#ifdef HAVE_OPENMPT
#include "libopenmpt/libopenmpt.h"
extern openmpt_module *openmpt_mhandle;
#endif

// mask used to indicate sound origin is player item pickup
#define PICKUP_SOUND 0x8000

extern consvar_t stereoreverse;
extern consvar_t cv_soundvolume, cv_closedcaptioning, cv_digmusicvolume, cv_midimusicvolume;
extern consvar_t cv_numChannels;

extern consvar_t cv_resetmusic;
extern consvar_t cv_resetmusicbyheader;

extern consvar_t cv_1upsound;

#define RESETMUSIC (!modeattacking && \
	(cv_resetmusicbyheader.value ? \
		(mapheaderinfo[gamemap-1]->musforcereset != -1 ? mapheaderinfo[gamemap-1]->musforcereset : cv_resetmusic.value) \
		: cv_resetmusic.value) \
	)

extern consvar_t cv_gamedigimusic;
extern consvar_t cv_gamemidimusic;
extern consvar_t cv_gamesounds;
extern consvar_t cv_musicpref;

extern consvar_t cv_playmusicifunfocused;
extern consvar_t cv_playsoundsifunfocused;

#ifdef HAVE_OPENMPT
extern consvar_t cv_modfilter;
#endif

#ifdef HAVE_MIXERX
extern consvar_t cv_midiplayer;
extern consvar_t cv_midisoundfontpath;
extern consvar_t cv_miditimiditypath;
#endif

extern CV_PossibleValue_t soundvolume_cons_t[];

typedef enum
{
	SF_TOTALLYSINGLE =  1, // Only play one of these sounds at a time...GLOBALLY
	SF_NOMULTIPLESOUND =  2, // Like SF_NOINTERRUPT, but doesnt care what the origin is
	SF_OUTSIDESOUND  =  4, // Volume is adjusted depending on how far away you are from 'outside'
	SF_X4AWAYSOUND   =  8, // Hear it from 4x the distance away
	SF_X8AWAYSOUND   = 16, // Hear it from 8x the distance away
	SF_NOINTERRUPT   = 32, // Only play this sound if it isn't already playing on the origin
	SF_X2AWAYSOUND   = 64, // Hear it from 2x the distance away
} soundflags_t;

typedef struct {
	fixed_t x, y, z;
	angle_t angle;
} listener_t;

typedef struct
{
	// sound information (if null, channel avail.)
	sfxinfo_t *sfxinfo;

	// origin of sound
	const void *origin;

	// initial volume of sound, which is applied after distance and direction
	INT32 volume;

	// handle of the sound being played
	INT32 handle;

} channel_t;

typedef struct {
	channel_t *c;
	sfxinfo_t *s;
	UINT16 t;
	UINT8 b;
} caption_t;

#define NUMCAPTIONS 8
#define MAXCAPTIONTICS (2*TICRATE)
#define CAPTIONFADETICS 20

extern caption_t closedcaptions[NUMCAPTIONS];
void S_StartCaption(sfxenum_t sfx_id, INT32 cnum, UINT16 lifespan);
void S_ResetCaptions(void);

// register sound vars and commands at game startup
void S_RegisterSoundStuff(void);

//
// Initializes sound stuff, including volume
// Sets channels, SFX, allocates channel buffer, sets S_sfx lookup.
//
void S_InitSfxChannels(INT32 sfxVolume);

//
// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
//
void S_StopSounds(void);
void S_ClearSfx(void);
void S_StartEx(boolean reset);
#define S_Start() S_StartEx(false)

//
// Basically a W_GetNumForName that adds "ds" at the beginning of the string. Returns a lumpnum.
//
lumpnum_t S_GetSfxLumpNum(sfxinfo_t *sfx);

//
// Sound Status
//

boolean S_SoundDisabled(void);

//
// Start sound for thing at <origin> using <sound_id> from sounds.h
//
void S_StartSound(const void *origin, sfxenum_t sound_id);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(const void *origin, sfxenum_t sound_id, INT32 volume);

// Stop sound for thing at <origin>
void S_StopSound(void *origin);

//
// Music Status
//

boolean S_DigMusicDisabled(void);
boolean S_MIDIMusicDisabled(void);
boolean S_MusicDisabled(void);
boolean S_MusicPlaying(void);
boolean S_MusicPaused(void);
boolean S_MusicNotInFocus(void);
musictype_t S_MusicType(void);
const char *S_MusicName(void);
boolean S_MusicExists(const char *mname, boolean checkMIDI, boolean checkDigi);
#define S_DigExists(a) S_MusicExists(a, false, true)
#define S_MIDIExists(a) S_MusicExists(a, true, false)

// Returns whether the preferred format a (true = MIDI, false = Digital)
// exists and is enabled for musicname b
#define S_PrefAvailable(a, b) (a ? \
	(!S_MIDIMusicDisabled() && S_MIDIExists(b)) : \
	(!S_DigMusicDisabled() && S_DigExists(b)))

//
// Music Effects
//

// Set Speed of Music
boolean S_SpeedMusic(float speed);

// Music definitions
typedef struct musicdef_s
{
	char name[7];
	char title[32];
	char alttitle[64];
	char authors[256];
	//char usage[256]; -- probably never going to be relevant to vanilla
	/*
	the trouble here is that kart combines what we call "title"
	and "authors" into one string. we need to split it for sound
	test reasons. they might split it later like we did, but...
	*/
	//char source[256];
	UINT8 soundtestpage;
	INT16 soundtestcond; // +ve for map, -ve for conditionset, 0 for already here
	tic_t stoppingtics;
	fixed_t bpm;
	UINT32 loop_ms;/* override LOOPPOINT/LOOPMS */
	boolean allowed; // question marks or listenable on sound test?
	struct musicdef_s *next;
} musicdef_t;

extern musicdef_t soundtestsfx;
extern musicdef_t *musicdefstart;
extern musicdef_t **soundtestdefs;
extern INT32 numsoundtestdefs;
extern UINT8 soundtestpage;

void S_LoadMusicDefs(UINT16 wadnum);
void S_InitMusicDefs(void);

boolean S_PrepareSoundTest(void);

//
// Music Seeking
//

// Get Length of Music
UINT32 S_GetMusicLength(void);

// Set LoopPoint of Music
boolean S_SetMusicLoopPoint(UINT32 looppoint);

// Get LoopPoint of Music
UINT32 S_GetMusicLoopPoint(void);

// Set Position of Music
boolean S_SetMusicPosition(UINT32 position);

// Get Position of Music
UINT32 S_GetMusicPosition(void);

//
// Music Stacking (Jingles)
//

typedef struct musicstack_s
{
	char musname[7];
	UINT16 musflags;
	boolean looping;
	UINT32 position;
	tic_t tic;
	UINT16 status;
	lumpnum_t mlumpnum;
	boolean noposition; // force music stack resuming from zero (like music_stack_noposition)

    struct musicstack_s *prev;
    struct musicstack_s *next;
} musicstack_t;

extern char music_stack_nextmusname[7];
extern boolean music_stack_noposition;
extern UINT32 music_stack_fadeout;
extern UINT32 music_stack_fadein;

void S_SetStackAdjustmentStart(void);
void S_AdjustMusicStackTics(void);
void S_RetainMusic(const char *mname, UINT16 mflags, boolean looping, UINT32 position, UINT16 status);
boolean S_RecallMusic(UINT16 status, boolean fromfirst);

//
// Music Playback
//

// Start music track, arbitrary, given its name, and set whether looping
// note: music flags 12 bits for tracknum (gme, other formats with more than one track)
//       13-15 aren't used yet
//       and the last bit we ignore (internal game flag for resetting music on reload)
void S_ChangeMusicEx(const char *mmusic, UINT16 mflags, boolean looping, UINT32 position, UINT32 prefadems, UINT32 fadeinms);
#define S_ChangeMusicInternal(a,b) S_ChangeMusicEx(a,0,b,0,0,0)
#define S_ChangeMusic(a,b,c) S_ChangeMusicEx(a,b,c,0,0,0)

// Stops the music.
void S_StopMusic(void);

// Stop and resume music, during game PAUSE.
void S_PauseAudio(void);
void S_ResumeAudio(void);

//
// Music Fading
//

void S_SetInternalMusicVolume(INT32 volume);
void S_StopFadingMusic(void);
boolean S_FadeMusicFromVolume(UINT8 target_volume, INT16 source_volume, UINT32 ms);
#define S_FadeMusic(a, b) S_FadeMusicFromVolume(a, -1, b)
#define S_FadeInChangeMusic(a,b,c,d) S_ChangeMusicEx(a,b,c,0,0,d)
boolean S_FadeOutStopMusic(UINT32 ms);

//
// Updates music & sounds
//
void S_UpdateSounds(void);
void S_UpdateClosedCaptions(void);

FUNCMATH fixed_t S_CalculateSoundDistance(fixed_t px1, fixed_t py1, fixed_t pz1, fixed_t px2, fixed_t py2, fixed_t pz2);

void S_SetSfxVolume(INT32 volume);
void S_SetMusicVolume(INT32 digvolume, INT32 seqvolume);
#define S_SetDigMusicVolume(a) S_SetMusicVolume(a,-1)
#define S_SetMIDIMusicVolume(a) S_SetMusicVolume(-1,a)
#define S_InitMusicVolume() S_SetMusicVolume(-1,-1)

INT32 S_OriginPlaying(void *origin);
INT32 S_IdPlaying(sfxenum_t id);
INT32 S_SoundPlaying(void *origin, sfxenum_t id);

void S_StartSoundName(void *mo, const  char *soundname);

void S_StopSoundByID(void *origin, sfxenum_t sfx_id);
void S_StopSoundByNum(sfxenum_t sfxnum);

#ifndef HW3SOUND
#define S_StartAttackSound S_StartSound
#define S_StartScreamSound S_StartSound
#endif

#ifdef MUSICSLOT_COMPATIBILITY
// For compatibility with code/scripts relying on older versions
// This is a list of all the "special" slot names and their associated numbers
extern const char *compat_special_music_slots[16];
#endif

#endif
