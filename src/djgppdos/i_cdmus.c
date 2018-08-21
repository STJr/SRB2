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
/// \brief cd music interface (uses bcd library)

// Alam_GBC: I hate you, Brennan Underwood :)
#include "../doomtype.h"
#include "bcd.c"
//#include "bcd.h"                // CD-Audio library by Brennan Underwood

#include "../doomdef.h"
#include "../i_sound.h"
#include "../command.h"
#include "../i_system.h"
#include "../s_sound.h"

// ------
// protos
// ------
static void Command_Cd_f (void);


//======================================================================
//                   CD AUDIO MUSIC SUBSYSTEM
//======================================================================

UINT8  cdaudio_started=0;   // for system startup/shutdown

#define MAX_CD_TRACKS 256
static boolean cdPlaying = false;
static int     cdPlayTrack;         // when cdPlaying is true
static boolean cdLooping = false;
static UINT8   cdRemap[MAX_CD_TRACKS];
static boolean cdEnabled=true;      // cd info available
static boolean cdValid;             // true when last cd audio info was ok
static boolean wasPlaying;
static int     cdVolume=0;          // current cd volume (0-31)

// 0-31 like Music & Sfx, though CD hardware volume is 0-255.
consvar_t cd_volume = {"cd_volume","18",CV_SAVE,soundvolume_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// allow Update for next/loop track
// some crap cd drivers take up to
// a second for a simple 'busy' check..
// (on those Update can be disabled)
consvar_t cdUpdate  = {"cd_update","1",CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};


// hour,minutes,seconds
FUNCINLINE static ATTRINLINE char *hms(int hsg)
{
	int hours, minutes, seconds;
	static char s[9];

	seconds = hsg / 75;
	minutes = seconds / 60;
	seconds %= 60;
	hours = minutes / 60;
	minutes %= 60;
	if (hours>0)
		sprintf (s, "%d:%02d:%02d", hours, minutes, seconds);
	else
		sprintf (s, "%2d:%02d", minutes, seconds);
	return s;
}

static void Command_Cd_f (void)
{
	const char *    s;
	int       i,j;

	if (!cdaudio_started)
		return;

	if (COM_Argc()<2)
	{
		CONS_Printf ("cd [on] [off] [remap] [reset] [open]\n"
		             "   [info] [play <track>] [loop <track>]\n"
		             "   [stop] [resume]\n");
		return;
	}

	s = COM_Argv(1);

	// activate cd music
	if (!strncmp(s,"on",2))
	{
		cdEnabled = true;
		return;
	}

	// stop/deactivate cd music
	if (!strncmp(s,"off",3))
	{
		if (cdPlaying)
			I_StopCD ();
		cdEnabled = false;
		return;
	}

	// remap tracks
	if (!strncmp(s,"remap",5))
	{
		i = COM_Argc() - 2;
		if (i <= 0)
		{
			CONS_Printf ("CD tracks remapped in that order :\n");
			for (j = 1; j < MAX_CD_TRACKS; j++)
				if (cdRemap[j] != j)
					CONS_Printf (" %2d -> %2d\n", j, cdRemap[j]);
			return;
		}
		for (j = 1; j <= i; j++)
			cdRemap[j] = atoi (COM_Argv (j+1));
		return;
	}

	// reset the CD driver, useful on some odd cd's
	if (!strncmp(s,"reset",5))
	{
		cdEnabled = true;
		if (cdPlaying)
			I_StopCD ();
		for (i = 0; i < MAX_CD_TRACKS; i++)
			cdRemap[i] = i;
		bcd_reset ();
		cdValid = bcd_get_audio_info ();
		return;
	}

	// any other command is not allowed until we could retrieve cd information
	if (!cdValid)
	{
		CONS_Printf ("CD is not ready.\n");
		return;
	}

	if (!strncmp(s,"open",4))
	{
		if (cdPlaying)
			I_StopCD ();
		bcd_open_door();
		cdValid = false;
		return;
	}

	if (!strncmp(s,"info",4))
	{
		int totaltime = 0;

		if (!bcd_get_audio_info())
		{
			CONS_Printf ("Error getting audio info: %s\n",bcd_error());
			cdValid = false;
			return;
		}

		cdValid = true;

		if (lowest_track == 0)
			CONS_Printf ("No audio tracks\n");
		else
		{
			// display list of tracks
			// highlight current playing track
			for (i=lowest_track; i <= highest_track; i++)
			{
				CONS_Printf    ("%s%2d. %s  %s\n",
								cdPlaying && (cdPlayTrack == i) ? "\2 " : " ",
								i, tracks[i].is_audio ? "audio" : "data ",
								hms(tracks[i].len));
				if (tracks[i].is_audio)
					totaltime += tracks[i].len;
			}

			if (totaltime)
				CONS_Printf ("\2Total time : %s\n", hms(totaltime));
		}
		if (cdPlaying)
		{
			CONS_Printf ("%s track : %d\n", cdLooping ? "looping" : "playing",
			             cdPlayTrack);
		}
		return;
	}

	if (!strncmp(s,"play",4))
	{
		I_PlayCD ((UINT8)atoi (COM_Argv (2)), false);
		return;
	}

	if (!strncmp(s,"stop",4))
	{
		I_StopCD ();
		return;
	}

	if (!strncmp(s,"loop",4))
	{
		I_PlayCD ((UINT8)atoi (COM_Argv (2)), true);
		return;
	}

	if (!strncmp(s,"resume",4))
	{
		I_ResumeCD ();
		return;
	}

	CONS_Printf ("cd command '%s' unknown\n", s);
}


// pause cd music
void I_StopCD (void)
{
	if (!cdaudio_started || !cdEnabled)
		return;

	bcd_stop();

	wasPlaying = cdPlaying;
	cdPlaying = false;
}

// continue after a pause
void I_ResumeCD (void)
{
	if (!cdaudio_started || !cdEnabled)
		return;

	if (!cdValid)
		return;

	if (!wasPlaying)
		return;

	bcd_resume ();
	cdPlaying = true;
}


void I_ShutdownCD (void)
{
	int rc;

	if (!cdaudio_started)
		return;

	I_StopCD ();

	rc = bcd_close ();
	if (!rc)
		CONS_Printf ("Error shuting down cd\n");
}

void I_InitCD (void)
{
	int rc;
	int i;

	rc = bcd_open ();

	if (rc>=0x200)
	{
		CONS_Printf ("MSCDEX version %d.%d\n", rc>>8, rc&255);

		I_AddExitFunc (I_ShutdownCD);
		cdaudio_started = true;
	}
	else
	{
		if (!rc)
			CONS_Printf ("%s\n", bcd_error() );

		cdaudio_started = false;
		return;
	}

	// last saved in config.cfg
	i = cd_volume.value;
	I_SetVolumeCD (0);   // initialize to 0 for some odd cd drivers
	I_SetVolumeCD (i);   // now set the last saved volume

	for (i = 0; i < MAX_CD_TRACKS; i++)
		cdRemap[i] = i;

	if (!bcd_get_audio_info())
	{
		CONS_Printf("\2CD Init: No CD in player.\n");
		cdEnabled = false;
		cdValid = false;
	}
	else
	{
		cdEnabled = true;
		cdValid = true;
	}

	COM_AddCommand ("cd", Command_Cd_f);
}



// loop/go to next track when track is finished (if cd_update var is true)
// update the volume when it has changed (from console/menu)
/// \todo check for cd change and restart music ?

void I_UpdateCD (void)
{
	int     newVolume;
	int     now;
	static  int last;     //game tics (35th of second)

	if (!cdaudio_started)
		return;

	now = I_GetTime ();
	if ((now - last) < 10)        // about 1/4 second
		return;
	last = now;

	// update cd volume changed at console/menu
	newVolume = cd_volume.value & 31;

	if (cdVolume != newVolume)
		I_SetVolumeCD (newVolume);

	// slow drivers exit here
	if (!cdUpdate.value)
		return;

	if (cdPlaying)
	{
		if (!bcd_audio_busy())
		{
			cdPlaying = false;
			if (cdLooping)
				I_PlayCD (cdPlayTrack, true);
			else
			{
				// play the next cd track, or restart the first
				cdPlayTrack++;
				if (cdPlayTrack > highest_track)
					cdPlayTrack = lowest_track;
				while (!tracks[cdPlayTrack].is_audio && cdPlayTrack<highest_track)
					cdPlayTrack++;
				I_PlayCD (cdPlayTrack, true);
			}
		}
	}

}


//
void I_PlayCD (UINT8 track, UINT8 looping)
{
	if (!cdaudio_started || !cdEnabled)
		return;

	if (!cdValid)
		return;

	track = cdRemap[track];

	if (cdPlaying)
	{
		if (cdPlayTrack == track)
			return;
		I_StopCD ();
	}

	if (track < lowest_track || track > highest_track)
	{
		//CONS_Printf ("\2CD Audio: wrong track number %d\n", track);
		// suppose there are not enough tracks for game levels..
		// then do a modulo so we always get something to hear
		track = (track % (highest_track-lowest_track+1)) + 1;
		//return;
	}

	cdPlayTrack = track;

	if (!tracks[track].is_audio)
	{
		CONS_Printf ("\2CD Play: not an audio track\n");
		return;
	}

	cdLooping = looping;

	if (!bcd_play_track (track))
	{
		CONS_Printf ("CD Play: could not play track %d\n", track);
		cdValid = false;
		cdPlaying = false;
		return;
	}

	cdPlaying = true;

}


// volume : logical cd audio volume 0-31 (hardware is 0-255)
boolean I_SetVolumeCD (INT32 volume)
{
	int  hardvol;

	if (!cdaudio_started || !cdEnabled)
		return false;

	// translate to hardware volume
	volume &= 31;

	hardvol = ((volume+1)<<3)-1;  //highest volume is 255
	if (hardvol<=8)
		hardvol=0;                //lowest volume is ZERO

	cdVolume = volume;

	if (bcd_set_volume (hardvol))
	{
		CV_SetValue (&cd_volume, volume);

		return true;
	}
	else
		return false;
}
