/*
  SDL_mixer_ext:  An extended audio mixer library, forked from SDL_mixer
  Copyright (C) 2014-2023 Vitaly Novichkov <admin@wohlnet.ru>

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

/*!
 * This fine contains some bindings of SDL for VisualBasic 6.
 * That required for statically linked SDL into SDL2_mixer_ext
*/

#ifdef FORCE_STDCALLS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_mixer.h"
#include "SDL_rwops.h"

/* Needed for ADLMIDI_getBankNames() */
#include "music_midi_adl.h"

#define SDL_INIT_AUDIO          0x00000010
extern DECLSPEC int SDL_Init(Uint32 flags);
extern DECLSPEC void SDL_Quit();
extern DECLSPEC const char * SDL_GetError(void);

__declspec(dllexport) int MIXCALLCC SDL_InitAudio(void)
{
    return SDL_Init(SDL_INIT_AUDIO);
}

__declspec(dllexport) void MIXCALLCC SDL_QuitVB6(void)
{
    SDL_Quit();
}

__declspec(dllexport) int MIXCALLCC Mix_InitVB6(void)
{
    return Mix_Init(0);
}

__declspec(dllexport) int MIXCALLCC Mix_OpenAudioVB6(int frequency, int format, int channels, int chunksize)
{
    return Mix_OpenAudio(frequency, (Uint16)format, channels, chunksize);
}

__declspec(dllexport) Mix_Chunk * MIXCALLCC Mix_LoadWAV_VB6(const char*file)
{
    return Mix_LoadWAV_RW(SDL_RWFromFile(file, "rb"), 1);
}

__declspec(dllexport) const char* MIXCALLCC SDL_GetErrorVB6(void)
{
    return SDL_GetError();
}

/* SDL RWops*/

__declspec(dllexport) SDL_RWops * MIXCALLCC SDL_RWFromFileVB6(const char *file, const char *mode)
{
    return SDL_RWFromFileVB6(file, mode);
}

__declspec(dllexport) SDL_RWops * MIXCALLCC SDL_RWFromMemVB6(void *mem, int size)
{
    return SDL_RWFromMem(mem, size);
}

__declspec(dllexport) SDL_RWops * SDL_AllocRWVB6(void)
{
    return SDL_AllocRW();
}

__declspec(dllexport) void MIXCALLCC SDL_FreeRWVB6(SDL_RWops * area)
{
    SDL_FreeRW(area);
}

__declspec(dllexport) int MIXCALLCC SDL_RWsizeVB6(SDL_RWops * ctx)
{
    return (int)(ctx)->size(ctx);
}

__declspec(dllexport) int MIXCALLCC SDL_RWseekVB6(SDL_RWops * ctx, int offset, int whence)
{
    return (int)(ctx)->seek(ctx, offset, whence);
}

__declspec(dllexport) int MIXCALLCC SDL_RWtellVB6(SDL_RWops * ctx)
{
    return (int)(ctx)->seek(ctx, 0, RW_SEEK_CUR);
}

__declspec(dllexport) int MIXCALLCC SDL_RWreadVB6(SDL_RWops * ctx, void*ptr, int size, int maxnum)
{
    return (int)(ctx)->read(ctx, ptr, size, maxnum);
}

__declspec(dllexport) int MIXCALLCC SDL_RWwriteVB6(SDL_RWops * ctx, void* ptr, int size, int maxnum)
{
    return (int)(ctx)->write(ctx, ptr, size, maxnum);
}

__declspec(dllexport) int MIXCALLCC SDL_RWcloseVB6(SDL_RWops * ctx)
{
    return (int)(ctx)->close(ctx);
}

__declspec(dllexport) const char* MIXCALLCC MIX_ADLMIDI_getBankName(int bankID)
{
    return Mix_ADLMIDI_getBankNames()[bankID];
}

#endif
