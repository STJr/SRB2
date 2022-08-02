#ifndef __SDL_I_VIDEO_HWR2_H__
#define __SDL_I_VIDEO_HWR2_H__

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "../i_video.h"
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

hwr2renderer_h I_SDL_CreateGlesHwr2Renderer(SDL_Window* window);
void I_SDL_DestroyGlesHwr2Renderer(hwr2renderer_h renderer);
void I_SDL_Hwr2Present(hwr2renderer_h renderer);

#ifdef __cplusplus
}
#endif

#endif // __SDL_I_VIDEO_HWR2_H__
