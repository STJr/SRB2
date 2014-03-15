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
/// \file  s_sound.h
/// \brief The not so system specific sound interface

#ifndef __S_SOUND__
#define __S_SOUND__

#include "sounds.h"
#include "m_fixed.h"
#include "command.h"
#include "tables.h" // angle_t

// mask used to indicate sound origin is player item pickup
#define PICKUP_SOUND 0x8000

extern consvar_t stereoreverse;
extern consvar_t cv_soundvolume, cv_digmusicvolume, cv_midimusicvolume;
extern consvar_t cv_numChannels;

#ifdef SNDSERV
extern consvar_t sndserver_cmd, sndserver_arg;
#endif
#ifdef MUSSERV
extern consvar_t musserver_cmd, musserver_arg;
#endif

extern CV_PossibleValue_t soundvolume_cons_t[];
//part of i_cdmus.c
extern consvar_t cd_volume, cdUpdate;

#if defined (macintosh) && !defined (SDL)
typedef enum
{
	music_normal,
	playlist_random,
	playlist_normal
} playmode_t;

extern consvar_t play_mode;
#endif

//in case you're wondering why:  I need to define this as extern so P_RestoreMusic can get to it so we don't do stupid song/speed changes
extern musicinfo_t *mus_playing;

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

// register sound vars and commands at game startup
void S_RegisterSoundStuff(void);

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume, allocates channel buffer, sets S_sfx lookup.
//
void S_Init(INT32 sfxVolume, INT32 digMusicVolume, INT32 midiMusicVolume);

//
// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
//
void S_StopSounds(void);
void S_ClearSfx(void);
void S_Start(void);

//
// Basically a W_GetNumForName that adds "ds" at the beginning of the string. Returns a lumpnum.
//
lumpnum_t S_GetSfxLumpNum(sfxinfo_t *sfx);

//
// Start sound for thing at <origin> using <sound_id> from sounds.h
//
void S_StartSound(const void *origin, sfxenum_t sound_id);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(const void *origin, sfxenum_t sound_id, INT32 volume);

// Stop sound for thing at <origin>
void S_StopSound(void *origin);

// Start music using <music_id> from sounds.h, and set whether looping
// note: music slot is first 16 bits for songnum,
//       next 15 bits for tracknum (gme, other formats with more than one track)
//       and the last bit we ignore (internal game flag for resetting music on reload)
void S_ChangeMusic(UINT32 mslotnum, boolean looping);

// Set Speed of Music
boolean S_SpeedMusic(float speed);

// Stops the music.
void S_StopMusic(void);

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

//
// Updates music & sounds
//
void S_UpdateSounds(void);

fixed_t S_CalculateSoundDistance(fixed_t px1, fixed_t py1, fixed_t pz1, fixed_t px2, fixed_t py2, fixed_t pz2);

void S_SetDigMusicVolume(INT32 volume);
void S_SetMIDIMusicVolume(INT32 volume);
void S_SetSfxVolume(INT32 volume);

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

#endif
