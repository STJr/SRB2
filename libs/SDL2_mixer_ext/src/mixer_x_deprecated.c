/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_mixer.h"

/*
    Deprecated SDL Mixer X's functions.
    There are kept to don't break ABI with existing apps.
*/

void MIXCALLCC Mix_Timidity_addToPathList(const char *path)
{
    Mix_SetTimidityCfg(path);
}

int MIXCALLCC Mix_GetMidiDevice()
{
    return Mix_GetMidiPlayer();
}

int MIXCALLCC Mix_GetNextMidiDevice()
{
    return Mix_GetNextMidiPlayer();
}

int MIXCALLCC Mix_SetMidiDevice(int player)
{
    return Mix_SetMidiPlayer(player);
}

int MIXCALLCC Mix_ADLMIDI_getAdLibMode()
{
    return -1;
}

void MIXCALLCC Mix_ADLMIDI_setAdLibMode(int ald)
{
    (void)ald;
}

int MIXCALLCC Mix_ADLMIDI_getLogarithmicVolumes()
{
#ifdef MUSIC_MID_ADLMIDI
    return (Mix_ADLMIDI_getVolumeModel() == ADLMIDI_VM_NATIVE);
#else
    return -1;
#endif
}

void MIXCALLCC Mix_ADLMIDI_setLogarithmicVolumes(int vm)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setVolumeModel(vm ? ADLMIDI_VM_NATIVE : ADLMIDI_VM_AUTO);
#else
    (void)vm;
#endif
}

int MIXCALLCC Mix_SetMusicStreamPosition(Mix_Music *music, double position)
{
    return Mix_SetMusicPositionStream(music, position);
}
