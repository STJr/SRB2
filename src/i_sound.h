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
/// \file  i_sound.h
/// \brief System interface, sound, music and CD

#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomdef.h"
#include "sounds.h"
#include "command.h"

/**	\brief Sound subsystem runing and waiting
*/
extern UINT8 sound_started;

/**	\brief info of samplerate
*/
extern consvar_t cv_samplerate;
//extern consvar_t cv_rndsoundpitch;

/**	\brief	The I_GetSfx function

	\param	sfx	sfx to setup

	\return	data for sfx
*/
void *I_GetSfx(sfxinfo_t *sfx);

/**	\brief	The I_FreeSfx function

	\param	sfx	sfx to be freed up

	\return	void
*/
void I_FreeSfx(sfxinfo_t *sfx);

/**	\brief Init at program start...
*/
void I_StartupSound(void);

/**	\brief ... shut down and relase at program termination.
*/
void I_ShutdownSound(void);

//
//  SFX I/O
//

/**	\brief	Starts a sound in a particular sound channel.
	\param	id	sfxid
	\param	vol	volume for sound
	\param	sep	left-right balancle
	\param	pitch	not used
	\param	priority	not used

	\return	sfx handle
*/
INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority);

/**	\brief	Stops a sound channel.

	\param	handle	stop sfx handle

	\return	void
*/
void I_StopSound(INT32 handle);

/**	\brief Some digital sound drivers need this.
*/
void I_UpdateSound(void);

/**	\brief	Called by S_*() functions to see if a channel is still playing.

	\param	handle	sfx handle

	\return	0 if no longer playing, 1 if playing.
*/
boolean I_SoundIsPlaying(INT32 handle);

/**	\brief	Updates the sfx handle

	\param	handle	sfx handle
	\param	vol	volume
	\param	sep	separation
	\param	pitch	ptich

	\return	void
*/
void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch);

/**	\brief	The I_SetSfxVolume function

	\param	volume	volume to set at

	\return	void
*/
void I_SetSfxVolume(UINT8 volume);

//
//  MUSIC I/O
//
/** \brief Init the music systems
*/
void I_InitMusic(void);

/** \brief Shutdown the music systems
*/
void I_ShutdownMusic(void);

/**	\brief	PAUSE game handling.

	\param	handle	song handle

	\return	void
*/
void I_PauseSong(INT32 handle);

/**	\brief	RESUME game handling

	\param	handle	song handle

	\return	void
*/
void I_ResumeSong(INT32 handle);

/**	\brief Get MIDI music status

	\return boolean
*/
boolean I_MIDIPlaying(void);

/**	\brief Get general music status

	\return boolean
*/
boolean I_MusicPlaying(void);

/**	\brief Get music pause status

	\return boolean
*/
boolean I_MusicPaused(void);

//
//  MIDI I/O
//

/**	\brief Startup the MIDI music system
*/
void I_InitMIDIMusic(void);

/**	\brief Shutdown the MIDI music system
*/
void I_ShutdownMIDIMusic(void);

/**	\brief	The I_SetMIDIMusicVolume function

	\param	volume	volume to set at

	\return	void
*/
void I_SetMIDIMusicVolume(UINT8 volume);

/**	\brief	Registers a song handle to song data.

	\param	data	pointer to song data
	\param	len	len of data

	\return	song handle

	\todo Remove this
*/
INT32 I_RegisterSong(void *data, size_t len);

/**	\brief	Called by anything that wishes to start music

	\param	handle	Song handle
	\param	looping	looping it if true

	\return	if true, it's playing the song

	\todo pass music name, not handle
*/
boolean I_PlaySong(INT32 handle, boolean looping);

/**	\brief	Stops a song over 3 seconds

	\param	handle	Song handle
	\return	void

	/todo drop handle
*/
void I_StopSong(INT32 handle);

/**	\brief	See ::I_RegisterSong, then think backwards

	\param	handle	song handle

	\sa I_RegisterSong
	\todo remove midi handle
*/
void I_UnRegisterSong(INT32 handle);

//
//  DIGMUSIC I/O
//

/**	\brief Startup the music system
*/
void I_InitDigMusic(void);

/**	\brief Shutdown the music system
*/
void I_ShutdownDigMusic(void);

boolean I_SetSongSpeed(float speed);

UINT32 I_GetMusicLength(void);

boolean I_SetMusicPosition(UINT32 position);

UINT32 I_GetMusicPosition(void);

boolean I_SetSongTrack(INT32 track);

/**	\brief The I_StartDigSong function

	\param	musicname	music lump name
	\param	looping	if true, loop the song

	\return	if true, song playing
*/
boolean I_StartDigSong(const char *musicname, boolean looping);

/**	\brief stop non-MIDI song
*/
void I_StopDigSong(void);

/**	\brief The I_SetDigMusicVolume function

	\param	volume	volume to set at

	\return	void
*/
void I_SetDigMusicVolume(UINT8 volume);

//
// CD MUSIC I/O
//


/**	\brief  cd music interface
*/
extern UINT8 cdaudio_started;

/**	\brief Startup the CD system
*/
void I_InitCD(void);

/**	\brief Stop the CD playback
*/
void I_StopCD(void);

/**	\brief Pause the CD playback
*/
void I_PauseCD(void);

/**	\brief Resume the CD playback
*/
void I_ResumeCD(void);

/**	\brief Shutdown the CD system
*/
void I_ShutdownCD(void);

/**	\brief Update the CD info
*/
void I_UpdateCD(void);

/**	\brief	The I_PlayCD function

	\param	track	CD track number
	\param	looping	if true, loop the track

	\return	void
*/
void I_PlayCD(UINT8 track, UINT8 looping);

/**	\brief	The I_SetVolumeCD function

	\param	volume	volume level to set at

	\return	return 0 on failure
*/
boolean I_SetVolumeCD(INT32 volume);

#endif
