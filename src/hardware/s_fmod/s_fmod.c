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
/// \brief General driver for 3D sound system
///
///	Implementend via FMOD_SOUND API

#ifdef _WINDOWS
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <stdio.h>
FILE *logstream = NULL;

#ifdef __MINGW32__
#include <FMOD/fmod.h>
#else
#include <fmod.h>
#endif

#ifdef FSOUND_INIT_DONTLATENCYADJUST //Alam: why didn't I think about this before? :)
#define FSOUND_Sample_Load(index,data,mode,length) FSOUND_Sample_Load(index,data,mode,0,length)
#else
#define OLDFMOD //Alam: Yea!
#endif

#ifdef __MINGW32__
#include <FMOD/fmod_errors.h>
#else
#include <fmod_errors.h>	/* optional */
#endif

#define MAXCHANNEL 1024

#undef DEBUG_TO_FILE
#if defined ( HAVE_SDL ) && !defined ( LOGMESSAGES )
#define DEBUG_TO_FILE
#endif

#ifndef MORESTUFF
//#define MORESTUFF
#define REL2D
#endif

#define  _CREATE_DLL_
#include "../../doomdef.h"
#include "../hw3dsdrv.h"
#define INT2CHAR(p) (char *)p + 8

//Doom sound sample format
#define FSOUND_UNORMAL (FSOUND_8BITS | FSOUND_MONO | FSOUND_UNSIGNED)
//Load Raw
#define FSOUND_RAWLOAD (FSOUND_LOADRAW | FSOUND_LOADMEMORY)
//Load Sample Raw from pointer
#define FSOUND_DOOMLOAD (FSOUND_UNORMAL | FSOUND_RAWLOAD)
//length of SFX
#define SFXLENGTH (sfx->length-8)

//SRB2 treats +X as east, +Y as north, +Z as up.
//FSOUND treats +X as right, +Y as up, and +Z as forwards.
//So SRB2's XZY is FSOUND's XYZ
//And FSOUND's XZY is SRB2's XYZ

static I_Error_t I_ErrorFMOD = NULL;

static FSOUND_SAMPLE *blankfmsample = NULL;

// Calculate sound pitching
static float recalc_pitch(INT32 doom_pitch)
{
#ifdef MORESTUFF
	return (float)1;
#else
	return doom_pitch < NORMAL_PITCH ?
		(float)(doom_pitch + NORMAL_PITCH) / (NORMAL_PITCH * 2)
			:(float)doom_pitch / (float)NORMAL_PITCH;
#endif
}

/***************************************************************
 *
 * DBG_Printf
 * Output error messages to debug log if DEBUG_TO_FILE is defined,
 * else do nothing
 *
 **************************************************************
 */
// -----------------+
// DBG_Printf       : Output error messages to debug log if DEBUG_TO_FILE is defined,
//                  : else do nothing
// Returns          :
// -----------------+
FUNCPRINTF void DBG_Printf(const char *lpFmt, ...)
{
#ifdef DEBUG_TO_FILE
	char str[4096] = "";
	va_list arglist;

	va_start(arglist, lpFmt);
	vsnprintf(str, 4096, lpFmt, arglist);
	va_end(arglist);

	if (logstream)
		fwrite(str, strlen(str), 1, logstream);
#endif
}

// Start of head_relative system for FMOD3D

typedef struct rellistener_s
{
	float pos[3];
	INT32 active;
	float vel[3];

} rellistener_s_t; //Easy

static rellistener_s_t rellistener[2]; //The 2 listeners

typedef struct relarray_s
{
	INT32             handle;             // handle number
	INT32             chan;               // real channel
	INT32             listener;           // for which listener
	rellistener_s_t pos;                // source position in 3D
} relarray_s_t;

static relarray_s_t relarray[STATIC_SOURCES_NUM+1]; // array of chans


//set listener
static void relset(INT32 which)
{
	INT32 i;

	if (which == -2 || which == -1)
	{
		which = -which;

		memset(&rellistener[which-1], 0, sizeof (rellistener_s_t)); //Delete me
		for (i = 0; i <= STATIC_SOURCES_NUM; i++)
		{
			if (relarray[i].pos.active && relarray[i].listener == which)
			{
				FSOUND_SetPaused(relarray[i].chan,true);
				relarray[i].pos.active = false;
			}
		}
		return;
	}
	else if (which == 1 || which ==2)
	{
		float pos[3], vel[3]; // Temp bin
		which -= 1; //Hello!

		//Get update
		FSOUND_3D_Listener_GetAttributes(&pos[0],&vel[0],NULL,NULL,NULL,NULL,NULL,NULL);
		memcpy(&rellistener[which].pos[0],&pos[0],sizeof (pos));
		memcpy(&rellistener[which].vel[0],&vel[0],sizeof (vel));

		if (!rellistener[which].active)
		for (i = 0; i <= STATIC_SOURCES_NUM; i++)
		{
			if (!relarray[i].pos.active && relarray[i].listener == which)
			{
				FSOUND_SetPaused(relarray[i].chan,false);
				relarray[i].pos.active = true;
			}
		}

		rellistener[which].active = which;
	}
	else
	{
		memset(&rellistener[0], 0, sizeof (rellistener_s_t));
		memset(&rellistener[1], 0, sizeof (rellistener_s_t));
	}
}

//setup system
static void relsetup()
{
	INT32 i;

	// Setup the rel array for use
	for (i = 0; i <= STATIC_SOURCES_NUM; i++)
	{
		relarray[i].handle = FSOUND_FREE;
		relarray[i].chan = FSOUND_FREE;
		relarray[i].listener = (i == 1 || i == 3)?2:1;
		memset(&relarray[i].pos, 0, sizeof (rellistener_s_t));
	}

	// Set up the Listener array
	relset(0);
}


//checkup for stack
static INT32 relstack(INT32 chan)
{
	INT32 i;

	for (i = 0; i <= STATIC_SOURCES_NUM; i++)
	{
		if (relarray[i].pos.active && relarray[i].handle == chan)
			return i;
	}
	return -1;
}

//checkup for fake rel chan
static INT32 relcheckup(INT32 chan)
{
	INT32 i = relstack(chan);

	if (i == -1)
		return chan;
	else
		return relarray[i].chan;
}

	//Update place rel chan's vol
static void relvol(INT32 chan)
{
	INT32 vol = FSOUND_GetVolume(chan);
	INT32 error = FSOUND_GetError();

	if (chan == relcheckup(chan))
		return;

	if (!vol && error)
		DBG_Printf("FMOD(relvol, FSOUND_GetVolume, Channel # %i): %s\n",chan,FMOD_ErrorString(error));
	else if (!FSOUND_SetVolume(relcheckup(chan), vol))
		DBG_Printf("FMOD(relvol, FSOUND_SetVolume, Channel # %i): %s\n",chan,FMOD_ErrorString(FSOUND_GetError()));
}

//Copy rel chan
static INT32 relcopy(INT32 handle)
{
	FSOUND_SAMPLE *fmsample = NULL;
	INT32 chan = -1;
	float pos[3];
	float vel[3];

	if (!FSOUND_3D_GetAttributes(handle,&pos[0],&vel[0]))
		if (FSOUND_GetError() != FMOD_ERR_NONE) DBG_Printf("FMOD(relcopy, FSOUND_3D_GetAttributes, Channel # %i): %s\n",handle,FMOD_ErrorString(FSOUND_GetError()));

	fmsample = FSOUND_GetCurrentSample(handle);

	if (fmsample)
	{
#ifdef REL2D
		if (!FSOUND_Sample_SetMode(fmsample, FSOUND_2D))
			DBG_Printf("FMOD(relcopy, FSOUND_Sample_SetMode, handle# %i): %s\n",handle,FMOD_ErrorString(FSOUND_GetError()));
#endif
		chan = FSOUND_PlaySoundEx(FSOUND_FREE,fmsample,NULL,true);

		if (chan == -1)
		{
			if (FSOUND_GetError() != FMOD_ERR_NONE) DBG_Printf("FMOD(relcopy, FSOUND_PlaySoundEx, handle# %i): %s\n",handle,FMOD_ErrorString(FSOUND_GetError()));
			return FSOUND_FREE;
		}
#ifdef MORESTUFF
		else
			DBG_Printf("FMOD(relcopy, Main): Copy Handle#%i to channel#%i\n",handle,chan);
#endif
	}
	else
	{
		if (FSOUND_GetError() != FMOD_ERR_NONE)
			DBG_Printf("FMOD(relcopy, FSOUND_GetCurrentSample, handle# %i): %s\n",handle,FMOD_ErrorString(FSOUND_GetError()));
		chan = FSOUND_PlaySoundEx(FSOUND_FREE,blankfmsample,NULL,true);
		//return FSOUND_FREE;
	}

	if (FSOUND_GetCurrentSample(chan))
	{

		if (!FSOUND_SetCurrentPosition(chan, 0))
			DBG_Printf("FMOD(relcopy, FSOUND_SetCurrentPosition, handle#%i, channel# %i): %s\n",handle,chan,FMOD_ErrorString(FSOUND_GetError()));
#ifndef REL2D
		if (!FSOUND_3D_SetAttributes(chan,pos,vel))
			DBG_Printf("FMOD(relcopy, FSOUND_3D_SetAttributes, handle#%i, channel#%i): %s\n",handle,chan,FMOD_ErrorString(FSOUND_GetError()));
#endif
/*
		if (!FSOUND_SetReserved(chan, TURE))
			DBG_Printf("FMOD(relcopy, FSOUND_SetReserved, handle#%i, channel# %i): %s\n",handle,chan,FMOD_ErrorString(FSOUND_GetError()));
*/
		return chan;
	}

	return FSOUND_FREE;
}

//update the rel chans
static void relupdate(INT32 handle)
{
	if (handle == FSOUND_FREE)
	{
		INT32 stack;

		for (stack = 0; stack <= STATIC_SOURCES_NUM;stack++)
		{
			//if (FSOUND_GetCurrentSample(relarray[stack].handle))
			//	relarray[stack].chan = relcopy(relarray[stack].handle);

			if (relarray[stack].chan != FSOUND_FREE)
			{
				relarray[stack].pos.active = false;
				relupdate(relarray[stack].chan);
			}
			else
				relarray[stack].pos.active = false;
		}
	}
	else if (handle == relcheckup(handle))
		return;
#ifndef REL2D
	else
	{
		float pos[3], vel[3];
		INT32 stack = relstack(handle);
		INT32 chan = relcheckup(handle);
		INT32 which = relarray[stack].listener;

		pos[0] = relarray[stack].pos.pos[0] + rellistener[which].pos[0];
		pos[1] = relarray[stack].pos.pos[1] + rellistener[which].pos[1];
		pos[2] = relarray[stack].pos.pos[2] + rellistener[which].pos[2];
		vel[0] = relarray[stack].pos.vel[0] + rellistener[which].vel[0];
		vel[1] = relarray[stack].pos.vel[1] + rellistener[which].vel[1];
		vel[2] = relarray[stack].pos.vel[2] + rellistener[which].vel[2];

		if (!FSOUND_3D_SetAttributes(chan,pos,vel))
			DBG_Printf("FMOD(relupdate, FSOUND_3D_SetAttributes, Handle #%i,Channel #%i): %s\n",handle, chan,FMOD_ErrorString(FSOUND_GetError()));
	}
#endif
}

	//Update place rel chan's pos
static void relplace(INT32 chan)
{
	float pos[3], vel[3]; // Temp bin

	if (chan == relcheckup(chan))
		return;

	if (!FSOUND_3D_GetAttributes(chan,&pos[0],&vel[0]))
	{
		if (FSOUND_GetError() != FMOD_ERR_NONE) DBG_Printf("FMOD(relplace, FSOUND_3D_GetAttributes, Channel # %i): %s\n",chan,FMOD_ErrorString(FSOUND_GetError()));
	}
	else
	{
		INT32 stack = relstack(chan);

		if (stack == -1)
			return;

		memcpy(&relarray[stack].pos.pos,&pos,sizeof (pos));
		memcpy(&relarray[stack].pos.vel,&vel,sizeof (vel));
		relupdate(chan);
	}
}

//New rel chan
static void reladd(INT32 chan)
{
	if (chan != relcheckup(chan))
		return;
	else
	{
		INT32 stack = STATIC_SOURCES_NUM;
		{
			if (relarray[0].handle == FSOUND_FREE)
				stack = 0;
			else if (relarray[1].handle == FSOUND_FREE)
				stack = 1;
			else if (relarray[2].handle == FSOUND_FREE)
				stack = 2;
			else if (relarray[3].handle == FSOUND_FREE)
				stack = 3;
			else if (relarray[4].handle == FSOUND_FREE)
				stack = 4;
			else if (relarray[5].handle == FSOUND_FREE)
				stack = 5;
			else
				DBG_Printf("No more room in relarray\n");
		}

		relarray[stack].handle = chan;
		relarray[stack].chan = relcopy(chan);

		if (relarray[stack].chan != FSOUND_FREE)
		{
			relarray[stack].pos.active = true;
			relupdate(chan);
		}
		else
			relarray[stack].pos.active = false;
	}
}

// End of head_relative system for FMOD3D


/******************************************************************************
 *
 * Initialise driver and listener
 *
 *****************************************************************************/
EXPORT INT32 HWRAPI(Startup) (I_Error_t FatalErrorFunction, snddev_t *snd_dev)
{
	boolean inited = false;
	INT32 i = 0;

	I_ErrorFMOD = FatalErrorFunction;

	if (FSOUND_GetVersion() < FMOD_VERSION)
	{
		DBG_Printf("Error : You are using the wrong DLL version!\nYou should be using FMOD %.02f\n", FMOD_VERSION);
		return inited;
	}
	else
		DBG_Printf("S_FMOD Init(): FMOD_SOUND driver for SRB2 %s\n",VERSIONSTRING);

	if (!FSOUND_SetMinHardwareChannels(STATIC_SOURCES_NUM*4))
		DBG_Printf("FMOD(Startup,FSOUND_SetMinHardwareChannels,# of Channels Min: %i): %s\n",STATIC_SOURCES_NUM*4, FMOD_ErrorString(FSOUND_GetError()));

	if (!FSOUND_SetMaxHardwareChannels(0))
		DBG_Printf("FMOD(Startup,FSOUND_SetMaxHardwareChannels,# of Channels Min: %i): %s\n",0, FMOD_ErrorString(FSOUND_GetError()));

	for (i = 0; i < FSOUND_GetNumDrivers(); i++)
	{
		UINT32 caps = 0;
		DBG_Printf("Driver Caps, if any\n");

		if (FSOUND_GetDriverCaps(i, &caps))
		{
			DBG_Printf("FMOD: Driver# %d - %s\n", i+1, FSOUND_GetDriverName(i));    // print driver names
			if (caps & FSOUND_CAPS_HARDWARE)
				DBG_Printf("This sound hardware supports hardware accelerated 3d sound.\n");

			if (caps & FSOUND_CAPS_EAX2)
				DBG_Printf("This sound hardware supports hardware accelerated 3d sound with EAX 2 reverb.\n");

			if (caps & FSOUND_CAPS_EAX3)
				DBG_Printf("This sound hardware supports hardware accelerated 3d sound with EAX 3 reverb.\n");
		}
		else
			DBG_Printf("FMOD(Startup,FSOUND_GetDriverCaps,%s): %s\n",FSOUND_GetDriverName(i), FMOD_ErrorString(FSOUND_GetError()));
	}
#if defined (_WIN32) || defined (_WIN64)
	if (!FSOUND_SetHWND(snd_dev->hWnd))
	{
		DBG_Printf("FMOD(Startup,FSOUND_SetHWND): %s\n", FMOD_ErrorString(FSOUND_GetError()));
		return inited;
	}
	else
#endif
	{
		DBG_Printf("Initialising FMOD %.02f\n",FMOD_VERSION);
		inited = FSOUND_Init(snd_dev->sample_rate,MAXCHANNEL,0);
	}

	if (!inited)
	{
		DBG_Printf("FMOD(Startup,Main): %s\n", FMOD_ErrorString(FSOUND_GetError()));
	}
	else
	{
#ifdef OLDFMOD
		DBG_Printf("   Maximum hardware mixing buffers %d\n", FSOUND_GetNumHardwareChannels());
#else
		INT32 num2DC, num3DC, numDC;

		if (FSOUND_GetNumHWChannels(&num2DC,&num3DC,&numDC))
		{
			DBG_Printf("   Maximum hardware 2D buffers %d\n", num2DC);
			DBG_Printf("   Maximum hardware 3D buffers %d\n", num3DC);
			DBG_Printf("   Maximum hardware mixing buffers %d\n", numDC);
		}
		else
			DBG_Printf("FMOD(Startup,FSOUND_GetNumHWChannels): %s\n", FMOD_ErrorString(FSOUND_GetError()));
#endif

		DBG_Printf("FMOD is up and running at %i KHZ\n",FSOUND_GetOutputRate());
		DBG_Printf("Sound hardware capabilities:\n");

		if (FSOUND_GetDriverName(FSOUND_GetDriver()))
			DBG_Printf("   Driver is %s\n",FSOUND_GetDriverName(FSOUND_GetDriver()));
		else
			DBG_Printf("FMOD(Startup,FSOUND_GetDriverName): %s\n", FMOD_ErrorString(FSOUND_GetError()));

		FSOUND_3D_SetDistanceFactor(1.0f/72.0f);
		FSOUND_3D_SetRolloffFactor(0);
		FSOUND_3D_SetRolloffFactor(1.6f);
#ifdef MORESTUFF
		FSOUND_3D_SetDopplerFactor(0);
#endif

		switch (FSOUND_GetOutput())
		{
			case FSOUND_OUTPUT_NOSOUND:
				DBG_Printf("FMOD driver: NoSound driver, all calls to this succeed but do nothing.\n");
				break;
			case FSOUND_OUTPUT_WINMM:
				DBG_Printf("FMOD driver: Windows Multimedia driver.\n");
				break;
			case FSOUND_OUTPUT_DSOUND:
				DBG_Printf("FMOD driver: DirectSound driver. You need this to get EAX2 or EAX3 support, or FX api support.\n");
				break;
			case FSOUND_OUTPUT_A3D:
				DBG_Printf("FMOD driver: A3D driver.\n");
				break;
			case FSOUND_OUTPUT_XBOX:
				DBG_Printf("FMOD driver: Xbox driver\n");
				break;
			case FSOUND_OUTPUT_OSS:
				DBG_Printf("FMOD driver: Linux/Unix OSS (Open Sound System) driver, i.e. the kernel sound drivers.\n");
				break;
			case FSOUND_OUTPUT_ESD:
				DBG_Printf("FMOD driver: Linux/Unix ESD (Enlightment Sound Daemon) driver.\n");
				break;
			case FSOUND_OUTPUT_ALSA:
				DBG_Printf("FMOD driver: Linux Alsa driver.\n");
				break;
			case FSOUND_OUTPUT_MAC:
				DBG_Printf("FMOD driver: Mac SoundManager driver\n");
				break;
			case FSOUND_OUTPUT_PS2:
				DBG_Printf("FMOD driver: PlayStation 2 driver\n");
				break;
			case FSOUND_OUTPUT_GC:
				DBG_Printf("FMOD driver: Gamecube driver\n");
				break;
			case FSOUND_OUTPUT_NOSOUND_NONREALTIME:
				DBG_Printf("FMOD driver: This is the same as nosound, but the sound generation is driven by FSOUND_Update\n");
				break;
			case FSOUND_OUTPUT_ASIO:
				DBG_Printf("FMOD driver: Low latency ASIO driver\n");
				break;
			default:
				DBG_Printf("FMOD driver: Unknown Sound Driver\n");
				break;
		}

		relsetup();

		blankfmsample = FSOUND_Sample_Alloc(FSOUND_UNMANAGED,1,FSOUND_UNORMAL,11025,127,FSOUND_STEREOPAN,127);

		if (!blankfmsample)
			DBG_Printf("FMOD(Startup,FSOUND_Sample_Alloc): %s\n", FMOD_ErrorString(FSOUND_GetError()));
		else if (!FSOUND_Sample_SetMaxPlaybacks(blankfmsample,STATIC_SOURCES_NUM*2))
			DBG_Printf("FMOD(Startup,FSOUND_Sample_SetMaxPlaybacks): %s\n", FMOD_ErrorString(FSOUND_GetError()));
	}
	return inited;
}

EXPORT void HWRAPI(Shutdown) (void)
{
	DBG_Printf("Shuting down FMOD\n");
	FSOUND_Sample_Free(blankfmsample);
	DBG_Printf("Shutdown of FMOD done\n");
	FSOUND_Close();
}

/******************************************************************************
 *
 * Creates 3D source
 *
 ******************************************************************************/

EXPORT INT32 HWRAPI (Add3DSource) (source3D_data_t *src, sfx_data_t *sfx)
{
	FSOUND_SAMPLE *fmsample = NULL;
	INT32 chan = -1;
	float pos[3];
	float vel[3];
#ifdef MORESTUFF
	src->min_distance = MIN_DISTANCE;
	src->max_distance = MAX_DISTANCE;
#endif

	pos[0] = src->pos.x;
	pos[1] = src->pos.z;
	pos[2] = src->pos.y;
	vel[0] = src->pos.momx;
	vel[1] = src->pos.momz;
	vel[2] = src->pos.momy;
	if (sfx)
		fmsample = FSOUND_Sample_Load(FSOUND_FREE, INT2CHAR(sfx->data), FSOUND_DOOMLOAD, SFXLENGTH);
	else
		fmsample = blankfmsample;

	if (fmsample)
	{
		if (sfx && !FSOUND_Sample_SetDefaults(fmsample,
				(INT32)((*((UINT16 *)sfx->data+1))*recalc_pitch(sfx->pitch)),
				(sfx->volume == -1 ? 255 : sfx->volume),
				(sfx->sep == NORMAL_SEP ? FSOUND_STEREOPAN : sfx->sep),
				(sfx->priority)
				)
			)
			DBG_Printf("FMOD(Add3DSource, FSOUND_Sample_SetDefaults, SFX's ID# %i): %s\n",  sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));
#if 0
		if (!FSOUND_Sample_SetMinMaxDistance(fmsample, src->min_distance, src->max_distance))
			DBG_Printf("FMOD(Add3DSource, FSOUND_Sample_SetMinMaxDistance, SFX's ID# %i): %s\n", sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));
#endif
		chan = FSOUND_PlaySoundEx(FSOUND_FREE,fmsample,NULL,true);

		if (chan == -1)
		{
			DBG_Printf("FMOD(Add3DSource, FSOUND_PlaySoundEx, SFX's ID# %i): %s\n",sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));
			return chan;
		}
		else
		{
			if (!sfx)
				DBG_Printf("FMOD(Add3DSource, Main): Added blank-sound added to channel %i\n",chan);
#ifdef MORESTUFF
			else DBG_Printf("FMOD(Add3DSource, Main): Added sfxid# %i added to channel %i\n",sfx->id,chan);
#endif
		}
	}
	else
	{
		if (sfx)
			DBG_Printf("FMOD(Add3DSource, FSOUND_Sample_Load, sfxid# %i): %s\n",sfx->id,FMOD_ErrorString(FSOUND_GetError()));
		else
			DBG_Printf("FMOD(Add3DSource, FSOUND_Sample_Alloc): %s\n", FMOD_ErrorString(FSOUND_GetError()));

		return chan;
	}

	if (FSOUND_GetCurrentSample(chan))
	{
		if (!FSOUND_SetCurrentPosition(chan, 0))
			DBG_Printf("FMOD(Add3DSource, FSOUND_SetCurrentPosition, channel %i, sfxid# %i): %s\n", chan,sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));

		if (!FSOUND_3D_SetAttributes(chan,pos,vel))
			DBG_Printf("FMOD(Add3DSource, FSOUND_3D_SetAttributes, channel %i, sfxid# %i): %s\n", chan,sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));

		if (!FSOUND_SetReserved(chan, (signed char)src->permanent))
			DBG_Printf("FMOD(Add3DSource, FSOUND_SetReserved, channel %i, sfxid# %i): %s\n", chan,sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));

		if (src->head_relative) reladd(chan);
	}

	return chan;
}

/******************************************************************************
 *
 * Creates 2D (stereo) source
 *
 ******************************************************************************/
EXPORT INT32 HWRAPI (Add2DSource) (sfx_data_t *sfx)
{
	FSOUND_SAMPLE *fmsample = NULL;
	INT32 chan = -1;

	if (!sfx)
		return chan;

	fmsample = FSOUND_Sample_Load(FSOUND_FREE, INT2CHAR(sfx->data), FSOUND_DOOMLOAD, SFXLENGTH);

	if (fmsample)
	{
		if (!FSOUND_Sample_SetDefaults(fmsample,
		 (INT32)((float)(*((UINT16 *)sfx->data+1)) * recalc_pitch(sfx->pitch)),
		 sfx->volume == -1 ? 255 : sfx->volume,
		 sfx->sep == NORMAL_SEP ? FSOUND_STEREOPAN : sfx->sep,
		 sfx->priority))
			DBG_Printf("FMOD(Add2DSource, FSOUND_Sample_SetDefaults, sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));

		if (!FSOUND_Sample_SetMode(fmsample,FSOUND_2D))
			DBG_Printf("FMOD(Add2DSource, FSOUND_Sample_SetMode, sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));

		chan = FSOUND_PlaySoundEx(FSOUND_FREE,fmsample,NULL,true);

		if (chan == -1)
		{
			DBG_Printf("FMOD(Add2DSource, FSOUND_PlaySoundEx, sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));
			return chan;
		}
#ifdef MORESTUFF
		else DBG_Printf("FMOD(Add2DSource, FSOUND_PlaySoundEx): sfxid# %i is playing on channel %i\n", sfx->id,chan);
#endif
	}
	else
	{
		DBG_Printf("FMOD(Add2DSource,FSOUND_Sample_Load, sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));
		return chan;
	}

	if (FSOUND_GetCurrentSample(chan))
	{
		if (!FSOUND_SetCurrentPosition(chan, 0))
			DBG_Printf("FMOD(Add2DSource, FSOUND_SetCurrentPosition, channel %i, sfxid# %i): %s\n", chan,sfx->id,FMOD_ErrorString(FSOUND_GetError()));
	}

	return chan;
}

EXPORT INT32 HWRAPI (StartSource) (INT32 chan)
{
	FSOUND_SAMPLE *fmsample;
	if (chan < 0)
		return -1;

	fmsample = FSOUND_GetCurrentSample(chan);

	if (!fmsample)
		return -1;

#ifdef MORESTUFF
	if (FSOUND_Sample_GetMode(fmsample) & FSOUND_2D)
	{
		DBG_Printf("FMOD(StartSource,Main): Starting 2D channel %i?\n",chan);
		//return -1;
	}
	else
	{
		DBG_Printf("FMOD(StartSource,Main): Starting 3D Channel %i?\n",chan);
		//return -1;
	}
#endif

	if (FSOUND_GetPaused(relcheckup(chan)))
	{
		if (!FSOUND_SetPaused(relcheckup(chan), false))
			DBG_Printf("FMOD(StartSource,FSOUND_SetPaused, channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError()));
		else if (relstack(chan) != -1)
			relarray[relstack(chan)].pos.active = false;
	}
	else
		DBG_Printf("FMOD(StartSource,FSOUND_GetPaused): Channel %i is playing already",chan);

	return chan;
}

EXPORT void HWRAPI (StopSource) (INT32 chan)
{
	FSOUND_SAMPLE *fmsample;

	if (chan < 0)
		return;

	fmsample = FSOUND_GetCurrentSample(chan);

	if (!fmsample)
		return;

	if (!FSOUND_GetPaused(relcheckup(chan)))
	{
		if (!FSOUND_SetPaused(relcheckup(chan),true))
		{
			DBG_Printf("FMOD(StopSource,FSOUND_SetPaused, channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError()));
		}
		else if (relstack(chan) != -1)
		{
			relarray[relstack(chan)].pos.active = false;
		}
	}
#ifdef MORESTUFF
	else
		DBG_Printf("FMOD(StopSource,FSOUND_GetPaused): Channel %i is stopped already\n",chan);
#endif
}

EXPORT INT32 HWRAPI (GetHW3DSVersion) (void)
{
	return VERSION;
}

EXPORT void HWRAPI (BeginFrameUpdate) (void)
{
	FSOUND_Update();
}

EXPORT void HWRAPI (EndFrameUpdate) (void)
{
	relupdate(FSOUND_FREE);
}

EXPORT INT32 HWRAPI (IsPlaying) (INT32 chan)
{
	if (chan < 0)
		return false;

	if (!FSOUND_GetCurrentSample(chan))
		return false;

	return FSOUND_IsPlaying(relcheckup(chan));
}

/******************************************************************************
 * UpdateListener
 *
 * Set up main listener properties:
 * - position
 * - orientation
 * - velocity
 *****************************************************************************/
EXPORT void HWRAPI (UpdateListener) (listener_data_t *data)
{
	if (data)
	{
		float pos[3];
		float vel[3];
		double f_angle = 0;
		if (data->f_angle) f_angle = (data->f_angle) / 180 * M_PI;
		pos[0] = (float)data->x;
		pos[1] = (float)data->z;
		pos[2] = (float)data->y;
		vel[0] = (float)data->momx;
		vel[1] = (float)data->momz;
		vel[2] = (float)data->momy;

		FSOUND_3D_Listener_SetAttributes(pos,vel,(float)cos(f_angle),0.0f,(float)sin(f_angle), 0.0f , 1.0f, 0.0f);
		relset(1);
	}
	else
	{
		relset(-1);
		DBG_Printf("Error: 1st listener data is missing\n");
	}
}

EXPORT void HWRAPI (UpdateListener2) (listener_data_t *data)
{
	if (data)
	{
		float pos[3];
		float vel[3];
		double f_angle = 0;
		if (data->f_angle) f_angle = (data->f_angle) / 180 * M_PI;
		pos[0] = (float)data->x;
		pos[1] = (float)data->z;
		pos[2] = (float)data->y;
		vel[0] = (float)data->momx;
		vel[1] = (float)data->momz;
		vel[2] = (float)data->momy;

		FSOUND_3D_Listener_SetCurrent(1,2);
		FSOUND_3D_Listener_SetAttributes(pos,vel,(float)cos(f_angle),0.0f,(float)sin(f_angle), 0.0f , 1.0f, 0.0f);
		relset(2);
		FSOUND_3D_Listener_SetCurrent(0,2);
	}
	else
	{
		relset(-2);
		FSOUND_3D_Listener_SetCurrent(0,1);
	}
}

EXPORT void HWRAPI (UpdateSourceVolume) (INT32 chan, INT32 vol)
{
	if (chan < 0)
		return;

	if (!FSOUND_GetCurrentSample(chan))
		return;

	if (!FSOUND_SetVolume(chan,vol))
		DBG_Printf("FMOD(UpdateSourceVolume, FSOUND_SetVolume, channel %i to volume %i): %s\n", chan,vol,FMOD_ErrorString(FSOUND_GetError()));
	else
	{
#ifdef MORESTUFF
		DBG_Printf("FMOD(UpdateSourceVolume, Main): channel %i is set to the volume of %i", chan,vol);
#endif
		relvol(chan);
	}
}

/******************************************************************************
 *
 * Update volume and separation (panning) of 2D source
 *
 *****************************************************************************/
EXPORT void HWRAPI (Update2DSoundParms) (INT32 chan, INT32 vol, INT32 sep)
{
	FSOUND_SAMPLE *fmsample;

	if (chan < 0)
		return;

	fmsample = FSOUND_GetCurrentSample(chan);

	if (fmsample)
	{
		if (!FSOUND_Sample_GetMode(fmsample) & FSOUND_2D)
		{
			DBG_Printf("FMOD(Update2DSoundParms,Main): 2D Vol/Pan on 3D channel %i?\n",chan);
			//return;
		}
	}
	else
		return;

	if (!FSOUND_SetPaused(chan, true))
		DBG_Printf("FMOD(Update2DSoundParms, FSOUND_SetPaused, Pause, channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError()));

	if (!FSOUND_SetVolume(chan,vol))
		DBG_Printf("FMOD(Update2DSoundParms, , channel %i to volume %i): %s\n", chan,vol,FMOD_ErrorString(FSOUND_GetError()));

	if (!FSOUND_SetPan(chan, sep == NORMAL_SEP ? FSOUND_STEREOPAN : sep))
		DBG_Printf("FMOD(Update2DSoundParms, FSOUND_SetPan, channel %i to sep %i): %s\n", chan,sep,FMOD_ErrorString(FSOUND_GetError()));

	if (!FSOUND_SetPaused(chan, false))
		DBG_Printf("FMOD(Update2DSoundParms, FSOUND_SetPaused, Resume, channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError()));
}

// --------------------------------------------------------------------------
// Set the global volume for sound effects
// --------------------------------------------------------------------------
EXPORT void HWRAPI (SetGlobalSfxVolume) (INT32 vol)
{
	INT32 realvol = (vol<<3)+(vol>>2);

	FSOUND_SetSFXMasterVolume(realvol);
#ifdef MORESTUFF
	DBG_Printf("FMOD(SetGlobalSfxVolume, Main, the volume is set to %i): %s\n", realvol,FMOD_ErrorString(FSOUND_GetError()));
#endif
}

//Alam_GBC: Not Used?
EXPORT INT32 HWRAPI (SetCone) (INT32 chan, cone_def_t *cone_def)
{
	FSOUND_SAMPLE *fmsample;

	if (chan < 0)
		return -1;

	fmsample = FSOUND_GetCurrentSample(chan);

	if (fmsample)
	{
		if (FSOUND_Sample_GetMode(fmsample) & FSOUND_2D)
		{
			DBG_Printf("FMOD(Update3DSource,Main): 3D Cone on 2D Channel %i?\n",chan);
			return -1;
		}
	}

	if (cone_def)
	{
		return -1;
	}
	else
	{
		return 1;
	}

}

EXPORT void HWRAPI (Update3DSource) (INT32 chan, source3D_pos_t *sfx)
{
	float pos[3];
	float vel[3];
	FSOUND_SAMPLE *fmsample;

	pos[0] = sfx->x;
	pos[1] = sfx->z;
	pos[2] = sfx->y;
	vel[0] = sfx->momx;
	vel[1] = sfx->momz;
	vel[2] = sfx->momy;
	if (chan < 0)
		return;

	fmsample = FSOUND_GetCurrentSample(chan);

	if (!fmsample)
		return;

	if (fmsample)
	{
		if (FSOUND_Sample_GetMode(fmsample) & FSOUND_2D)
		{
			//DBG_Printf("FMOD(Update3DSource,Main): 3D Pos/Vel on 2D Channel %i?\n",chan);
			//return;
		}
	}
	else
	{
		return;
	}

	if (!FSOUND_3D_SetAttributes(chan,pos,vel))
		DBG_Printf("FMOD(Update3DSource,FSOUND_3D_SetAttributes, onto channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError()));
	else
		relplace(chan);
}

//-------------------------------------------------------------
// Load new sound data into source
//-------------------------------------------------------------
EXPORT INT32 HWRAPI (Reload3DSource) (INT32 handle, sfx_data_t *sfx)
{
	source3D_data_t src;
	float pos[3], vel[3];
	INT32 chan = relcheckup(handle);
	FSOUND_SAMPLE *fmsample;

	if (handle < 0 || !sfx)
		return -1;

	fmsample = FSOUND_GetCurrentSample(chan);

	if (!fmsample)
		return -1;

	if (FSOUND_Sample_GetMode(fmsample) & FSOUND_2D)
	{
		DBG_Printf("FMOD(Reload3DSource,Main): New 3D Sound on 2D Channel %i?\n",handle);
		return -1;
	}
#ifdef MORESTUFF
	else
		DBG_Printf("FMOD(Reload3DSource, Main, sending new sfx# %i to chan %i)\n",sfx->id,handle);
#endif

#ifdef OLDFMOD
	src.min_distance = MIN_DISTANCE;
	src.max_distance = MAX_DISTANCE;
#else
	if (!FSOUND_3D_GetMinMaxDistance(handle,&src.min_distance,&src.min_distance))
		DBG_Printf("FMOD(Reload3DSource, FSOUND_3D_GetMinMaxDistance, channel %i, for sfxid# %i): %s\n",handle, sfx->id,FMOD_ErrorString(FSOUND_GetError()));
#endif
	src.head_relative = (handle != chan);
	src.permanent = FSOUND_GetReserved(handle);
	if (!FSOUND_3D_GetAttributes(handle,&pos[0],&vel[0]))
		DBG_Printf("FMOD(Reload3DSource, FSOUND_3D_GetAttributes, channel %i, for sfxid# %i): %s\n",handle,sfx->id,FMOD_ErrorString(FSOUND_GetError()));

	//if (handle == chan)
	{
		if (fmsample != blankfmsample) FSOUND_Sample_Free(fmsample);
	}
	//else
	{
	//	fmsample = FSOUND_GetCurrentSample(handle);
	}

	fmsample = FSOUND_Sample_Load(FSOUND_FREE, INT2CHAR(sfx->data), FSOUND_DOOMLOAD, SFXLENGTH);

	if (!FSOUND_StopSound(chan))
		DBG_Printf("FMOD(Reload3DSource,FSOUND_StopSound, of channel %i for sfxid# %i): %s\n", handle,sfx->id,FMOD_ErrorString(FSOUND_GetError()));

	if (fmsample)
	{
		if (!FSOUND_Sample_SetDefaults(fmsample,
		 (INT32)((float)(*((UINT16 *)sfx->data+1)) * recalc_pitch(sfx->pitch)),
		 sfx->volume == -1 ? 255 : sfx->volume,
		 sfx->sep == NORMAL_SEP ? FSOUND_STEREOPAN : sfx->sep,
		 sfx->priority))
			DBG_Printf("FMOD(Reload3DSource, FSOUND_Sample_SetDefaults, for sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));

		if (!FSOUND_Sample_SetMinMaxDistance(fmsample, src.min_distance, src.max_distance))
			DBG_Printf("FMOD(Reload3DSource, FSOUND_Sample_SetMinMaxDistance, SFX's ID# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));

		chan = FSOUND_PlaySoundEx(chan,fmsample,NULL,true);

		if (chan == -1)
		{
			DBG_Printf("FMOD(Reload3DSource, FSOUND_PlaySoundEx, for sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));
			return chan;
		}

		if (FSOUND_GetCurrentSample(chan))
		{
			if (!FSOUND_SetCurrentPosition(chan, 0))
				DBG_Printf("FMOD(Reload3DSource, FSOUND_SetCurrentPosition, channel %i, sfxid# %i): %s\n", chan,sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));

			if (!FSOUND_3D_SetAttributes(chan,pos,vel))
				DBG_Printf("FMOD(Reload3DSource, FSOUND_3D_SetAttributes, channel %i, sfxid# %i): %s\n", chan,sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));

			if (!FSOUND_SetReserved(chan, (signed char)src.permanent))
				DBG_Printf("FMOD(Reload3DSource, FSOUND_SetReserved, channel %i, sfxid# %i): %s\n", chan,sfx?sfx->id:0,FMOD_ErrorString(FSOUND_GetError()));

			if (src.head_relative) relupdate(handle);
		}
	}
	else
		DBG_Printf("FMOD(Reload3DSource,FSOUND_Sample_Load, for sfxid# %i): %s\n", sfx->id,FMOD_ErrorString(FSOUND_GetError()));

	return chan;
}

/******************************************************************************
 *
 * Destroy source and remove it from stack if it is a 2D source.
 * Otherwise put source into cache
 *
 *****************************************************************************/
EXPORT void HWRAPI (KillSource) (INT32 chan)
{
	FSOUND_SAMPLE *fmsample;
	INT32 relchan;

	if (chan < 0)
		return;

	relchan = relcheckup(chan);

	fmsample = FSOUND_GetCurrentSample(chan);

	if (!fmsample)
		return;

#ifdef MORESTUFF
	if (FSOUND_IsPlaying(relchan))
		if (!FSOUND_SetLoopMode(relchan, FSOUND_LOOP_OFF))
			DBG_Printf("FMOD(KillSource,FSOUND_SetLoopMode, for channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError())); //Alam_GBC: looping off
	else
#endif
	if (!FSOUND_StopSound(relchan))
	{
		DBG_Printf("FMOD(KillSource,FSOUND_StopSound, for channel %i): %s\n", chan,FMOD_ErrorString(FSOUND_GetError()));
	}
#ifdef MORESTUFF
	else DBG_Printf("FMOD(KillSource, Main, for channel %i)\n", chan);
#endif
	if (fmsample != blankfmsample) FSOUND_Sample_Free(fmsample);
}

#ifdef _WINDOWS
BOOL WINAPI DllMain(HINSTANCE hinstDLL, // handle to DLL module
                    DWORD fdwReason,    // reason for calling function
                    LPVOID lpvReserved) // reserved
{
	// Perform actions based on the reason for calling.
	UNREFERENCED_PARAMETER(lpvReserved);
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.
#ifdef DEBUG_TO_FILE
			logstream = fopen("s_fmod.log", "wt");
			if (logstream == NULL)
				return FALSE;
#endif
			DisableThreadLibraryCalls(hinstDLL);
			break;

		case DLL_THREAD_ATTACH:
			// Do thread-specific initialization.
			break;

		case DLL_THREAD_DETACH:
			// Do thread-specific cleanup.
			break;

		case DLL_PROCESS_DETACH:
			// Perform any necessary cleanup.
#ifdef DEBUG_TO_FILE
			if (logstream)
			{
				fclose(logstream);
				logstream = NULL;
			}
#endif
			break;
	}
	return TRUE; // Successful DLL_PROCESS_ATTACH.
}
#elif !defined (HAVE_SDL)

// **************************************************************************
//                                                                  FUNCTIONS
// **************************************************************************

EXPORT void _init()
{
#ifdef DEBUG_TO_FILE
	logstream = fopen("s_fmod.log","wt");
#endif
}

EXPORT void _fini()
{
#ifdef DEBUG_TO_FILE
	if (logstream)
		fclose(logstream);
#endif
}
#endif
