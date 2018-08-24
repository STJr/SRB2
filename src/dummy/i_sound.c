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

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

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

boolean I_LoadSong(char *data, size_t len)
{
	(void)data;
	(void)len;
	return -1;
}

void I_UnloadSong(void)
{
	(void)handle;
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

void I_SetMusicVolume(UINT8 volume)
{
	(void)volume;
}

boolean I_SetSongTrack(int track)
{
	(void)track;
	return false;
}

void I_SetInternalMusicVolume(UINT8 volume)
{
	(void)volume;
}

void I_StopFadingSong(void)
{
}

boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, boolean stopafterfade)
{
	(void)target_volume;
	(void)source_volume;
	(void)ms;
	return false;
}

boolean I_FadeSong(UINT8 target_volume, UINT32 ms)
{
	(void)target_volume;
	(void)ms;
	return false;
}

boolean I_FadeOutStopSong(UINT32 ms)
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

UINT32 I_GetSongLength(void)
{
	return 0;
}

boolean I_SetSongLoopPoint(UINT32 looppoint)
{
        (void)looppoint;
        return false;
}

UINT32 I_GetSongLoopPoint(void)
{
	return 0;
}

boolean I_SetSongPosition(UINT32 position)
{
    (void)position;
    return false;
}

UINT32 I_GetSongPosition(void)
{
    return 0;
}

boolean I_SongPlaying(void)
{
	return false;
}

boolean I_SongPaused(void)
{
	return false;
}

musictype_t I_SongType(void)
{
	return MU_NONE;
}
