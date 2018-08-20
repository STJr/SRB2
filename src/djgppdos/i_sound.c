// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------
/// \file
/// \brief interface level code for sound

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include "../doomdef.h"
#include "../doomstat.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../z_zone.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../s_sound.h"
#include "../console.h"

//### let's try with Allegro ###
#define  alleg_mouse_unused
#define  alleg_timer_unused
#define  ALLEGRO_NO_KEY_DEFINES
#define  alleg_keyboard_unused
#define  alleg_joystick_unused
#define  alleg_gfx_driver_unused
#define  alleg_palette_unused
#define  alleg_graphics_unused
#define  alleg_vidmem_unused
#define  alleg_flic_unused
//#define  alleg_sound_unused    we use it
#define  alleg_file_unused
#define  alleg_datafile_unused
#define  alleg_math_unused
#define  alleg_gui_unused
#include <allegro.h>
//### end of Allegro include ###

//allegro has 256 virtual voices
// warning should by a power of 2
#define VIRTUAL_VOICES 256
#define VOICESSHIFT 8

// Needed for calling the actual sound output.
#define SAMPLECOUNT    512



//
// this function converts raw 11khz, 8-bit data to a SAMPLE* that allegro uses
// it is need cuz allegro only loads samples from wavs and vocs
//added:11-01-98: now reads the frequency from the rawdata header.
//   dsdata points a 4 UINT16 header:
//    +0 : value 3 what does it mean?
//    +2 : sample rate, either 11025 or 22050.
//    +4 : number of samples, each sample is a single byte since it's 8bit
//    +6 : value 0
static inline SAMPLE *raw2SAMPLE(UINT8 *dsdata, size_t len)
{
	SAMPLE *spl;

	spl=Z_Malloc(sizeof (SAMPLE),PU_SOUND,NULL);
	if (spl==NULL)
		I_Error("Raw2Sample : no more free mem");
	spl->bits = 8;
	spl->stereo = 0;
	spl->freq = *((UINT16 *)dsdata+1);   //mostly 11025, but some at 22050.
	spl->len = len-8;
	spl->priority = 255;                //priority;
	spl->loop_start = 0;
	spl->loop_end = len-8;
	spl->param = -1;
	spl->data=(void *)(dsdata+8);       //skip the 8bytes header

	return spl;
}


//  This function loads the sound data from the WAD lump,
//  for single sound.
//
void *I_GetSfx (sfxinfo_t * sfx)
{
	UINT8 *dssfx;

	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum (sfx);

	sfx->length = W_LumpLength (sfx->lumpnum);

	dssfx = (UINT8 *) W_CacheLumpNum (sfx->lumpnum, PU_SOUND);
	//_go32_dpmi_lock_data(dssfx,size);

	// convert raw data and header from Doom sfx to a SAMPLE for Allegro
	return (void *)raw2SAMPLE (dssfx, sfx->length);
}


void I_FreeSfx (sfxinfo_t *sfx)
{
	if (sfx->lumpnum == LUMPERROR)
		return;

	// free sample data
	if ( sfx->data )
		Z_Free((UINT8 *) ((SAMPLE *)sfx->data)->data - 8);
	Z_Free(sfx->data); // Allegro SAMPLE structure
	sfx->data = NULL;
	sfx->lumpnum = LUMPERROR;
}

FUNCINLINE static ATTRINLINE int Volset(int vol)
{
	return (vol*255/31);
}


void I_SetSfxVolume(INT32 volume)
{
	if (nosound)
		return;

	set_volume (Volset(volume),-1);
}

void I_SetMIDIMusicVolume(INT32 volume)
{
	if (nomidimusic)
		return;

	// Now set volume on output device.
	set_volume (-1, Volset(volume));
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
INT32 I_StartSound ( sfxenum_t     id,
                   INT32         vol,
                   INT32         sep,
                   INT32         pitch,
                   INT32         priority )
{
	int voice;

	if (nosound)
	return 0;

	// UNUSED
	priority = 0;

	pitch = (pitch-128)/2+128;
	voice = play_sample(S_sfx[id].data,vol,sep,(pitch*1000)/128,0);

	// Returns a handle
	return (id<<VOICESSHIFT)+voice;
}

void I_StopSound (INT32 handle)
{
	// You need the handle returned by StartSound.
	// Would be looping all channels,
	//  tracking down the handle,
	//  an setting the channel to zero.
	int voice=handle & (VIRTUAL_VOICES-1);

	if (nosound)
		return;

	if (voice_check(voice)==S_sfx[handle>>VOICESSHIFT].data)
		deallocate_voice(voice);
}

INT32 I_SoundIsPlaying(INT32 handle)
{
	if (nosound)
		return FALSE;

	if (voice_check(handle & (VIRTUAL_VOICES-1))==S_sfx[handle>>VOICESSHIFT].data)
		return TRUE;

	return FALSE;
}

// cut and past from ALLEGRO he don't share it :(
static inline int absolute_freq(int freq, SAMPLE *spl)
{
	if (freq == 1000)
		return spl->freq;
	else
		return (spl->freq * freq) / 1000;
}

void I_UpdateSoundParams( INT32 handle,
                          INT32 vol,
                          INT32 sep,
                          INT32 pitch)
{
	// I fail too see that this is used.
	// Would be using the handle to identify
	//  on which channel the sound might be active,
	//  and resetting the channel parameters.
	int voice=handle & (VIRTUAL_VOICES-1);
	int numsfx=handle>>VOICESSHIFT;

	if (nosound)
		return;

	if (voice_check(voice)==S_sfx[numsfx].data)
	{
		voice_set_volume(voice, vol);
		voice_set_pan(voice, sep);
		voice_set_frequency(voice, absolute_freq(pitch*1000/128,
		                    S_sfx[numsfx].data));
	}
}


void I_ShutdownSound(void)
{
	// Wait till all pending sounds are finished.

	//added:03-01-98:
	if ( !sound_started )
		return;

	//added:08-01-98: remove_sound() explicitly because we don't use
	//                Allegro's allegro_exit();
	remove_sound();
	sound_started = false;
}

static char soundheader[] = "sound";
#if ALLEGRO_VERSION == 3
static char soundvar[] = "sb_freq";
#else
static char soundvar[] = "sound_freq";
#endif

void I_StartupSound(void)
{
	int    sfxcard,midicard;
#if ALLEGRO_VERSION == 3
	char   err[255];
#endif

	if (nosound)
		sfxcard=DIGI_NONE;
	else
		sfxcard=DIGI_AUTODETECT;

	if (nomidimusic)
		midicard=MIDI_NONE;
	else
		midicard=MIDI_AUTODETECT; //DetectMusicCard();

	nodigimusic=true; //Alam: No OGG/MP3/IT/MOD support

	// Secure and configure sound device first.
	CONS_Printf("I_StartupSound: ");

	//Fab:25-04-98:note:install_sound will check for sound settings
	//    in the sound.cfg or allegro.cfg, in the current directory,
	//    or the directory pointed by 'ALLEGRO' env var.
#if ALLEGRO_VERSION == 3
	if (install_sound(sfxcard,midicard,NULL)!=0)
	{
		sprintf (err,"Sound init error : %s\n",allegro_error);
		CONS_Error (err);
		nosound=true;
		nomidimusic=true;
	}
	else
	{
		CONS_Printf(" configured audio device\n" );
	}

	//added:08-01-98:we use a similar startup/shutdown scheme as Allegro.
	I_AddExitFunc(I_ShutdownSound);
#endif
	sound_started = true;
	CV_SetValue(&cv_samplerate,get_config_int(soundheader,soundvar,cv_samplerate.value));
}




//
// MUSIC API.
// Still no music done.
// Remains. Dummies.
//

static MIDI* currsong;   //im assuming only 1 song will be played at once
static int      islooping=0;
static int      musicdies=-1;
UINT8           music_started=0;


/* load_midi_mem:
 *  Loads a standard MIDI file from memory, returning a pointer to
 *  a MIDI structure, *  or NULL on error.
 *  It is the load_midi from Allegro modified to load it from memory
 */
static MIDI *load_midi_mem(char *mempointer,int *e)
{
	int c = *e;
	long data=0;
	unsigned char *fp;
	MIDI *midi;
	int num_tracks=0;

	fp = (void *)mempointer;
	if (!fp)
		return NULL;

	midi = malloc(sizeof (MIDI));              /* get some memory */
	if (!midi)
		return NULL;

	for (c=0; c<MIDI_TRACKS; c++)
	{
		midi->track[c].data = NULL;
		midi->track[c].len = 0;
	}

	fp+=4+4;   // header size + 'chunk' size

	swab(fp,&data,2);     // convert to intel-endian
	fp+=2;                                      /* MIDI file type */
	if ((data != 0) && (data != 1)) // only type 0 and 1 are suported
		return NULL;

	swab(fp,&num_tracks,2);                     /* number of tracks */
	fp+=2;
	if ((num_tracks < 1) || (num_tracks > MIDI_TRACKS))
		return NULL;

	swab(fp,&data,2);                          /* beat divisions */
	fp+=2;
	midi->divisions = ABS(data);

	for (c=0; c<num_tracks; c++)
	{            /* read each track */
		if (memcmp(fp, "MTrk", 4))
			return NULL;
		fp+=4;

		//swab(fp,&data,4);       don't work !!!!??
		((char *)&data)[0]=fp[3];
		((char *)&data)[1]=fp[2];
		((char *)&data)[2]=fp[1];
		((char *)&data)[3]=fp[0];
		fp+=4;

		midi->track[c].len = data;

		midi->track[c].data = fp;
		fp+=data;
	}

	lock_midi(midi);
	return midi;
}

void I_InitMIDIMusic(void)
{
	if (nomidimusic)
		return;

	I_AddExitFunc(I_ShutdownMusic);
	music_started = true;
}

void I_ShutdownMIDIMusic(void)
{
	if ( !music_started )
		return;

	I_StopSong(1);

	music_started=false;
}

void I_InitDigMusic(void)
{
//	CONS_Printf("Digital music not yet supported under DOS.\n");
}

void I_ShutdownDigMusic(void)
{
//	CONS_Printf("Digital music not yet supported under DOS.\n");
}

void I_InitMusic(void)
{
	if (!nodigimusic)
		I_InitDigMusic();
	if (!nomidimusic)
		I_InitMIDIMusic();
}

void I_ShutdownMusic(void)
{
	I_ShutdownMIDIMusic();
	I_ShutdownDigMusic();
}

boolean I_PlaySong(INT32 handle, INT32 looping)
{
	handle = 0;
	if (nomidimusic)
		return false;

	islooping = looping;
	musicdies = gametic + NEWTICRATE*30;
	if (play_midi(currsong,looping)==0)
		return true;
	return false;
}

void I_PauseSong (INT32 handle)
{
	handle = 0;
	if (nomidimusic)
		return;

	midi_pause();
}

void I_ResumeSong (INT32 handle)
{
	handle = 0;
	if (nomidimusic)
		return;

	midi_resume();
}

void I_StopSong(INT32 handle)
{
	handle = 0;
	if (nomidimusic)
		return;

	islooping = 0;
	musicdies = 0;
	stop_midi();
}


// Is the song playing?
#if 0
int I_QrySongPlaying(int handle)
{
	if (nomidimusic)
		return 0;

	//return islooping || musicdies > gametic;
	return (midi_pos==-1);
}
#endif

void I_UnRegisterSong(INT32 handle)
{
	handle = 0;
	if (nomidimusic)
		return;

	//destroy_midi(currsong);
}

INT32 I_RegisterSong(void *data, size_t len)
{
	int e = len; //Alam: For error
	if (nomidimusic)
		return 0;

	if (memcmp(data,"MThd",4)==0) // support mid file in WAD !!!
	{
		currsong=load_midi_mem(data,&e);
	}
	else
	{
		CONS_Printf("Music Lump is not a MIDI lump\n");
		return 0;
	}

	if (currsong==NULL)
	{
		CONS_Printf("Not a valid mid file : %d\n",e);
		return 0;
	}

	return 1;
}

/// \todo Add OGG/MP3 support for dos
boolean I_StartDigSong(const char *musicname, INT32 looping)
{
	musicname = NULL;
	looping = 0;
	//CONS_Printf("I_StartDigSong: Not yet supported under DOS.\n");
	return false;
}

void I_StopDigSong(void)
{
//	CONS_Printf("I_StopDigSong: Not yet supported under DOS.\n");
}

void I_SetDigMusicVolume(INT32 volume)
{
	volume = 0;
	if (nodigimusic)
		return;

	// Now set volume on output device.
//	CONS_Printf("Digital music not yet supported under DOS.\n");
}

boolean I_SetSongSpeed(float speed)
{
	(void)speed;
	return false;
}

UINT32 I_GetMusicLength(void)
{
	return 0;
}

boolean I_SetMusicLoopPoint(UINT32 looppoint)
{
        (void)looppoint;
        return false;
}

UINT32 I_GetMusicLoopPoint(void)
{
	return 0;
}

boolean I_SetMusicPosition(UINT32 position)
{
    (void)position;
    return false;
}

UINT32 I_GetMusicPosition(void)
{
    return 0;
}

boolean I_MusicPlaying(void)
{
	return (boolean)currsong && music_started;
}

boolean I_MusicPaused(void)
{
	return false;
}

musictype_t I_MusicType(void)
{
	return MU_NONE;
}

void I_SetInternalMusicVolume(UINT8 volume)
{
	(void)volume;
}

void I_StopFadingMusic(void)
{
}

boolean I_FadeMusicFromLevel(UINT8 target_volume, UINT8 source_volume, UINT32 ms, boolean stopafterfade)
{
	(void)target_volume;
	(void)source_volume;
	(void)ms;
	return false;
}

boolean I_FadeMusic(UINT8 target_volume, UINT32 ms)
{
	(void)target_volume;
	(void)ms;
	return false;
}

boolean I_FadeOutStopMusic(UINT32 ms)
{
	(void)ms;
	return false;
}

boolean I_FadeInStartDigSong(const char *musicname, UINT16 track, boolean looping, UINT32 position, UINT32 fadeinms, boolean queuepostfade)
{
	(void)musicname;
	(void)looping;
	(void)fadeinms;
	return false;
}
