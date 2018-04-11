// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
/// \brief cd music interface
///

#ifdef HAVE_SDL

#if defined (DC) || defined (_WIN32_WCE) || defined(GP2X) || defined(_PS3)
#define NOSDLCD
#endif

#include <stdlib.h>
#ifndef NOSDLCD

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#endif

#include "../doomtype.h"
#include "../i_sound.h"
#include "../command.h"
#include "../m_argv.h"
#include "../s_sound.h"

#define MAX_CD_TRACKS 256

#ifdef _XBOX
INT32  SDL_SYS_CDInit(void)
{
	return(0);
}

void SDL_SYS_CDQuit(void)
{
	return;
}
#endif

UINT8 cdaudio_started = 0;   // for system startup/shutdown

consvar_t cd_volume = {"cd_volume","18",CV_SAVE,soundvolume_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cdUpdate  = {"cd_update","1",CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

#ifndef NOSDLCD
static SDL_bool cdValid     = SDL_FALSE;
static SDL_bool cdPlaying   = SDL_FALSE;
static SDL_bool wasPlaying  = SDL_FALSE;
static SDL_bool cdEnabled   = SDL_FALSE;
static SDL_bool playLooping = SDL_FALSE;
static Uint8    playTrack   = 0;
static Uint8    maxTrack    = MAX_CD_TRACKS-1;
static Uint8    cdRemap[MAX_CD_TRACKS];
static INT32      cdvolume    = -1;
static SDL_CD  *cdrom       = NULL;
static CDstatus cdStatus    = CD_ERROR;

/**************************************************************************
 *
 * function: CDAudio_GetAudioDiskInfo
 *
 * description:
 * set number of tracks if CD is available
 *
 **************************************************************************/
static INT32 CDAudio_GetAudioDiskInfo(void)
{
	cdValid = SDL_FALSE;
	maxTrack = 0;

	if (!cdrom)
		return 0;//Alam: Lies!

	cdStatus = SDL_CDStatus(cdrom);

	if (!CD_INDRIVE(cdStatus))
	{
		CONS_Printf("%s", M_GetText("No CD in drive\n"));
		return -1;
	}

	if (cdStatus == CD_ERROR)
	{
		CONS_Printf(M_GetText("CD Error: %s\n"), SDL_GetError());
		return -1;
	}

	cdValid = SDL_TRUE;
	maxTrack = (Uint8)cdrom->numtracks;

	return 0;
}


/**************************************************************************
 *
 * function: I_EjectCD
 *
 * description:
 *
 *
 **************************************************************************/
static void I_EjectCD(void)
{
	if (!cdrom || !cdEnabled)
		return; // no cd init'd

	I_StopCD();

	if (SDL_CDEject(cdrom))
		CONS_Printf("%s", M_GetText("CD eject failed\n"));
}

/**************************************************************************
 *
 * function: Command_Cd_f
 *
 * description:
 * handles all CD commands from the console
 *
 **************************************************************************/
static void Command_Cd_f (void)
{
	const char *command;
	size_t ret, n;

	if (!cdaudio_started)
		return;

	if (COM_Argc() < 2)
	{
		CONS_Printf ("%s", M_GetText("cd [on] [off] [remap] [reset] [select]\n"
		"   [open] [info] [play <track>] [resume]\n"
		"   [stop] [pause] [loop <track>]\n"));
		return;
	}

	command = COM_Argv (1);

	if (!strncmp(command, "on", 2))
	{
		cdEnabled = SDL_TRUE;
		return;
	}

	if (!strncmp(command, "off", 3))
	{
		I_StopCD();
		cdEnabled = SDL_FALSE;
		return;
	}

	if (!strncmp(command, "select", 6))
	{
		INT32 newcddrive;
		newcddrive = atoi(COM_Argv(2));
		command = SDL_CDName(newcddrive);
		I_StopCD();
		cdEnabled = SDL_FALSE;
		SDL_CDClose(cdrom);
		cdrom = SDL_CDOpen(newcddrive);
		if (cdrom)
		{
			cdEnabled = SDL_TRUE;
			CONS_Printf(M_GetText("Opened CD-ROM drive %s\n"), command ? command : COM_Argv(2));
		}
		else CONS_Printf(M_GetText("Couldn't open CD-ROM drive %s: %s\n"), command ? command : COM_Argv(2), SDL_GetError());
		return;
	}

	if (!strncmp(command, "remap", 5))
	{
		ret = COM_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < MAX_CD_TRACKS; n++)
			{
				if (cdRemap[n] != n)
					CONS_Printf("  %s -> %u\n", sizeu1(n), cdRemap[n]);
			}
			return;
		}
		for (n = 1; n <= ret; n++)
			cdRemap[n] = (Uint8)atoi(COM_Argv (n+1));
		return;
	}

	if (!strncmp(command, "reset", 5))
	{
		if (!cdrom) return;
		cdEnabled = SDL_TRUE;
		I_StopCD();
		for (n = 0; n < MAX_CD_TRACKS; n++)
			cdRemap[n] = (Uint8)n;
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (!cdValid)
	{
		if (CDAudio_GetAudioDiskInfo()==-1 && !cdValid)
		{
			CONS_Printf("%s", M_GetText("No CD in drive\n"));
			return;
		}
	}

	if (!strncmp(command, "open", 4))
	{
		I_EjectCD();
		cdValid = SDL_FALSE;
		return;
	}

	if (!strncmp(command, "info", 4))
	{
		CONS_Printf(M_GetText("%u tracks\n"), maxTrack);
		if (cdPlaying)
			CONS_Printf(M_GetText("Currently %s track %u\n"), playLooping ? M_GetText("looping") : M_GetText("playing"), playTrack);
		else if (wasPlaying)
			CONS_Printf(M_GetText("Paused %s track %u\n"), playLooping ? M_GetText("looping") : M_GetText("playing"), playTrack);
		CONS_Printf(M_GetText("Volume is %d\n"), cdvolume);
		return;
	}

	if (!strncmp(command, "play", 4))
	{
		I_PlayCD((UINT8)atoi(COM_Argv (2)), SDL_FALSE);
		return;
	}

	if (!strncmp(command, "loop", 4))
	{
		I_PlayCD((UINT8)atoi(COM_Argv (2)), true);
		return;
	}

	if (!strncmp(command, "stop", 4))
	{
		I_StopCD();
		return;
	}
	if (!strncmp(command, "pause", 5))
	{
		I_PauseCD();
		return;
	}

	if (!strncmp(command, "resume", 6))
	{
		I_ResumeCD();
		return;
	}

	CONS_Printf(M_GetText("Invalid CD command \"CD %s\"\n"), COM_Argv(1));
}
#endif

/**************************************************************************
 *
 * function: StopCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_StopCD(void)
{
#ifndef NOSDLCD
	if (!cdrom || !cdEnabled)
		return;

	if (!(cdPlaying || wasPlaying))
		return;

	if (SDL_CDStop(cdrom))
		I_OutputMsg("cdromstop failed\n");

	wasPlaying = SDL_FALSE;
	cdPlaying = SDL_FALSE;
#endif
}

/**************************************************************************
 *
 * function: PauseCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_PauseCD (void)
{
#ifndef NOSDLCD
	if (!cdrom || !cdEnabled)
		return;

	if (!cdPlaying)
		return;

	if (SDL_CDPause(cdrom))
		I_OutputMsg("cdrompause failed\n");

	wasPlaying = cdPlaying;
	cdPlaying = SDL_FALSE;
#endif
}

/**************************************************************************
 *
 * function: ResumeCD
 *
 * description:
 *
 *
 **************************************************************************/
// continue after a pause
void I_ResumeCD (void)
{
#ifndef NOSDLCD
	if (!cdrom || !cdEnabled)
		return;

	if (!cdValid)
		return;

	if (!wasPlaying)
		return;

	if (cd_volume.value == 0)
		return;

	if (SDL_CDResume(cdrom))
		I_OutputMsg("cdromresume failed\n");

	cdPlaying = SDL_TRUE;
	wasPlaying = SDL_FALSE;
#endif
}


/**************************************************************************
 *
 * function: ShutdownCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_ShutdownCD (void)
{
#ifndef NOSDLCD
	if (!cdaudio_started)
		return;

	I_StopCD();

	CONS_Printf("I_ShutdownCD: ");
	SDL_CDClose(cdrom);
	cdrom = NULL;
	cdaudio_started = false;
	CONS_Printf("%s", M_GetText("shut down\n"));
	SDL_QuitSubSystem(SDL_INIT_CDROM);
	cdEnabled = SDL_FALSE;
#endif
}

/**************************************************************************
 *
 * function: InitCD
 *
 * description:
 * Initialize the first CD drive SDL detects and add console command 'cd'
 *
 **************************************************************************/
void I_InitCD (void)
{
#ifndef NOSDLCD
	INT32 i;

	// Has been checked in d_main.c, but doesn't hurt here
	if (M_CheckParm ("-nocd"))
		return;

	CONS_Printf("%s", M_GetText("I_InitCD: Init CD audio\n"));

	// Initialize SDL first
	if (SDL_InitSubSystem(SDL_INIT_CDROM) < 0)
	{
		CONS_Printf(M_GetText("Couldn't initialize SDL CDROM: %s\n"), SDL_GetError());
		return;
	}

	// Open drive
	cdrom = SDL_CDOpen(0);

	if (!cdrom)
	{
		const char *cdName = SDL_CDName(0);
		if (!cdName)
			CONS_Printf(M_GetText("Couldn't open CD-ROM drive %s: %s\n"), "\b", SDL_GetError());
		else
			CONS_Printf(M_GetText("Couldn't open CD-ROM drive %s: %s\n"), cdName, SDL_GetError());
		//return;
	}

	for (i = 0; i < MAX_CD_TRACKS; i++)
		cdRemap[i] = (Uint8)i;

	cdaudio_started = true;
	if (cdrom) cdEnabled = SDL_TRUE;

	if (CDAudio_GetAudioDiskInfo()==-1)
	{
		CONS_Printf("%s", M_GetText("No CD in drive\n"));
		cdValid = SDL_FALSE;
	}

	COM_AddCommand ("cd", Command_Cd_f);

	CONS_Printf("%s", M_GetText("CD audio Initialized\n"));
#endif
}



//
/**************************************************************************
 *
 * function: UpdateCD
 *
 * description:
 * sets CD volume (may have changed) and initiates play evey 2 seconds
 * in case the song has elapsed
 *
 **************************************************************************/
void I_UpdateCD (void)
{
#ifndef NOSDLCD
	static Uint32 lastchk = 0;

	if (!cdEnabled || !cdrom)
		return;

	I_SetVolumeCD(cd_volume.value);

	if (cdPlaying && lastchk < SDL_GetTicks())
	{
		lastchk = SDL_GetTicks() + 2000; //two seconds between chks

		if (CDAudio_GetAudioDiskInfo()==-1)
		{
			cdPlaying = SDL_FALSE;
			return;
		}

		if (cdStatus != CD_PLAYING && cdStatus != CD_PAUSED)
		{
			cdPlaying = SDL_FALSE;
			if (playLooping)
				I_PlayCD(playTrack, true);
		}
	}
#endif
}



/**************************************************************************
 *
 * function: PlayCD
 *
 * description:
 * play the requested track and set the looping flag
 * pauses the CD if volume is 0
 *
 **************************************************************************/

void I_PlayCD (UINT8 track, UINT8 looping)
{
#ifdef NOSDLCD
	(void)track;
	(void)looping;
#else
	if (!cdrom || !cdEnabled)
		return;

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
			return;
	}

	track = cdRemap[track];

	if (track < 1 || track > maxTrack)
	{
		CONS_Printf(M_GetText("Bad track number %u.\n"), track);
		return;
	}

	// don't try to play a non-audio track
	if (cdrom->track[track].type == SDL_DATA_TRACK)
	{
		CONS_Printf(M_GetText("Track %u is not audio\n"), track);
		return;
	}

	if (cdPlaying)
	{
		if (playTrack == track)
			return;
		I_StopCD();
	}

	if (SDL_CDPlayTracks(cdrom, track, 0, 1, 0))
	{
		CONS_Printf(M_GetText("Error playing track %d: %s\n"), track, SDL_GetError());
		return;
	}

	playLooping = looping;
	playTrack = (Uint8)track;
	cdPlaying = SDL_TRUE;

	if (cd_volume.value == 0)
		I_PauseCD();
#endif
}


/**************************************************************************
 *
 * function: SetVolumeCD
 *
 * description:
 * SDL does not support setting the CD volume
 * use pause instead and toggle between full and no music
 *
 **************************************************************************/

boolean I_SetVolumeCD (INT32 volume)
{
#ifdef NOSDLCD
	(void)volume;
#else
	if (volume != cdvolume)
	{
		if (volume > 0 && volume < 16)
		{
			CV_SetValue(&cd_volume, 31);
			cdvolume = 31;
			I_ResumeCD();
		}
		else if (volume > 15 && volume < 31)
		{
			CV_SetValue(&cd_volume, 0);
			cdvolume = 0;
			I_PauseCD();
		}
	}
#endif
	return false;
}

#endif
