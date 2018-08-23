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

//
//  MUSIC I/O
//
UINT8 music_started = 0;

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

UINT8 midimusic_started = 0;

boolean I_LoadSong(void *data, size_t len)
{
        (void)data;
        (void)len;
        return -1;
}

boolean I_PlaySong(void)
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

UINT8 digmusic_started = 0;

void I_SetDigMusicVolume(INT32 volume)
{
        (void)volume;
}

boolean I_SetSongSpeed(float speed)
{
        (void)speed;
        return false;
}
