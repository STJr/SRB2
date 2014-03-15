// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2001 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief High-level functions of hardware 3D sound

#ifndef __HW3_SOUND_H__
#define __HW3_SOUND_H__

#ifdef HW3SOUND
#include "hws_data.h"
//#include "../s_sound.h"
//#include "../p_mobj.h"

// Default sound mode (original stereo mode)
enum
{
	HWS_DEFAULT_MODE = 0,
	HWS_DS3D,
	HWS_OTHER,
	HWS_FMOD3D,
	HWS_OPENAL,
};

typedef enum
{
	CT_NORMAL = 0,
	CT_ATTACK,
	CT_SCREAM,
	CT_AMBIENT,
} channel_type_t;

extern INT32  hws_mode;           // Current sound mode

#if defined (HW3DS) || defined (DOXYGEN)
INT32 HW3S_Init(I_Error_t FatalErrorFunction, snddev_t *snd_dev);
#endif
void HW3S_Shutdown(void);
INT32 HW3S_GetVersion(void);

// Common case - start 3D or 2D source
void HW3S_StartSound(const void *origin, sfxenum_t sfx_id);

// Special cases of 3D sources
void S_StartAmbientSound(sfxenum_t sfx_id, INT32 volume);
void S_StartAttackSound(const void *origin, sfxenum_t sfx_id);
void S_StartScreamSound(const void *origin, sfxenum_t sfx_id);

// Starts 3D sound with specified parameters
// origin      - for object movement tracking. NULL are possible value (no movement tracking)
// source_parm - 3D source parameters (position etc.)
// cone_parm   - 3D source cone parameters. NULL are possible value (default cone will be used)
// channel     -
// sfx_id      - sfx id
// vol         - sound volume
// pitch       - sound pitching value
// Returns:    - sound id
INT32 HW3S_I_StartSound(const void *origin, source3D_data_t *source_parm, channel_type_t channel, sfxenum_t sfx_id, INT32 vol, INT32 pitch, INT32 sep);
void HW3S_StopSoundByID(void *origin, sfxenum_t sfx_id);
void HW3S_StopSoundByNum(sfxenum_t sfxnum);
void HW3S_StopSound(void *origin);
void HW3S_StopSounds(void);

void HW3S_BeginFrameUpdate(void);
void HW3S_EndFrameUpdate(void);

void HW3S_UpdateSources(void);

void HW3S_SetSfxVolume(INT32 volume);

// Utility functions
INT32  HW3S_SoundIsPlaying(INT32 handle);
void HW3S_SetSourcesNum(void);

INT32  HW3S_OriginPlaying(void *origin);
INT32  HW3S_IdPlaying(sfxenum_t id);
INT32  HW3S_SoundPlaying(void *origin, sfxenum_t id);

void *HW3S_GetSfx(sfxinfo_t *sfx);
void HW3S_FreeSfx(sfxinfo_t *sfx);

#else // HW3SOUND

//#define S_StartAmbientSound(i,v) S_StartSoundAtVolume(NULL, i, v)
#define S_StartAttackSound S_StartSound
#define S_StartScreamSound S_StartSound

#endif // HW3SOUND

INT32 S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source,
	INT32 *vol, INT32 *sep, INT32 *pitch, sfxinfo_t *sfxinfo);

#endif // __HW3_SOUND_H__
