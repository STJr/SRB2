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

musictype_t I_GetMusicType(void)
{
	return MU_NONE;
}

void I_InitMusic(void){}

void I_ShutdownMusic(void){}

void I_SetMIDIMusicVolume(UINT8 volume)
{
	(void)volume;
}

void I_PauseSong(void)
{
	(void)handle;
}

void I_ResumeSong(void)
{
	(void)handle;
}

//
//  MIDI I/O
//

boolean I_LoadSong(char *data, size_t len)
{
	(void)data;
	(void)len;
	return -1;
}

boolean I_PlaySong(boolean looping)
{
	(void)handle;
	(void)looping;
	return false;
}

void I_StopSong(void)
{
	(void)handle;
}

void I_UnloadSong(void)
{
	(void)handle;
}

//
//  DIGMUSIC I/O
//

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
