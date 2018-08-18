#include "../i_sound.h"

UINT8 sound_started = 0;

void *I_GetSfx(sfxinfo_t *sfx)
{
	(void)sfx;
	return NULL;
}

void I_FreeSfx(sfxinfo_t *sfx)
{
	(void)sfx;
}

void I_StartupSound(void){}

void I_ShutdownSound(void){}

void I_UpdateSound(void){};

//
//  SFX I/O
//

INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority)
{
	(void)id;
	(void)vol;
	(void)sep;
	(void)pitch;
	(void)priority;
	return -1;
}

void I_StopSound(INT32 handle)
{
	(void)handle;
}

boolean I_SoundIsPlaying(INT32 handle)
{
	(void)handle;
	return false;
}

void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch)
{
	(void)handle;
	(void)vol;
	(void)sep;
	(void)pitch;
}

void I_SetSfxVolume(UINT8 volume)
{
	(void)volume;
}

//
//  MUSIC I/O
//

void I_InitMusic(void){}

void I_ShutdownMusic(void){}

void I_PauseSong(INT32 handle)
{
	(void)handle;
}

void I_ResumeSong(INT32 handle)
{
	(void)handle;
}

//
//  MIDI I/O
//

void I_InitMIDIMusic(void){}

void I_ShutdownMIDIMusic(void){}

void I_SetMIDIMusicVolume(UINT8 volume)
{
	(void)volume;
}

INT32 I_RegisterSong(void *data, size_t len)
{
	(void)data;
	(void)len;
	return -1;
}

boolean I_PlaySong(INT32 handle, boolean looping)
{
	(void)handle;
	(void)looping;
	return false;
}

void I_StopSong(INT32 handle)
{
	(void)handle;
}

void I_UnRegisterSong(INT32 handle)
{
	(void)handle;
}

//
//  DIGMUSIC I/O
//

void I_InitDigMusic(void){}

void I_ShutdownDigMusic(void){}

boolean I_StartDigSong(const char *musicname, boolean looping)
{
	(void)musicname;
	(void)looping;
	return false;
}

void I_StopDigSong(void){}

void I_SetDigMusicVolume(UINT8 volume)
{
	(void)volume;
}

boolean I_SetSongSpeed(float speed)
{
	(void)speed;
	return false;
}

boolean I_SetSongTrack(int track)
{
	(void)track;
	return false;
}

UINT32 I_GetMusicLength(void)
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

boolean I_MIDIPlaying(void)
{
	return false;
}

boolean I_MusicPlaying(void)
{
	return false;
}

boolean I_MusicPaused(void)
{
	return false;
}
