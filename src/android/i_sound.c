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

//
//  SFX I/O
//

INT32 I_StartSound(sfxenum_t id, INT32 vol, INT32 sep, INT32 pitch, INT32 priority)
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

INT32 I_SoundIsPlaying(INT32 handle)
{
        (void)handle;
        return false;
}

void I_UpdateSoundParams(INT32 handle, INT32 vol, INT32 sep, INT32 pitch)
{
        (void)handle;
        (void)vol;
        (void)sep;
        (void)pitch;
}

void I_SetSfxVolume(INT32 volume)
{
        (void)volume;
}

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

UINT8 music_started = 0;
UINT8 digmusic_started = 0;

void I_InitMusic(void){}

void I_ShutdownMusic(void){}

/// ------------------------
//  MUSIC PROPERTIES
/// ------------------------

musictype_t I_SongType(void)
{
	return MU_NONE;
}

boolean I_SongPlaying(void)
{
	return false;
}

boolean I_SongPaused(void)
{
	return false;
}

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

boolean I_SetSongSpeed(float speed)
{
        (void)speed;
        return false;
}

/// ------------------------
//  MUSIC PLAYBACK
/// ------------------------

UINT8 midimusic_started = 0;

boolean I_LoadSong(char *data, size_t len)
{
        (void)data;
        (void)len;
        return -1;
}

void I_UnloadSong()
{

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

void I_PauseSong(void)
{
        (void)handle;
}

void I_ResumeSong(void)
{
        (void)handle;
}

void I_SetMusicVolume(INT32 volume)
{
        (void)volume;
}
