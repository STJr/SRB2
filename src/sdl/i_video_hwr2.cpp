#include "i_video_hwr2.h"

#include "SDL.h"

extern "C" {
#include "../i_video.h"
}

#include "../hwr2/gles2/renderer.hpp"

class Sdl2Gles2Renderer : public Gles2Renderer
{
private:
	SDL_Window* window_ = NULL;
public:
	Sdl2Gles2Renderer(SDL_Window* window) : window_(window) {}

	virtual void Present()
	{
		SDL_GL_SwapWindow(window_);
	}
};

hwr2renderer_h I_SDL_CreateGlesHwr2Renderer(SDL_Window* window)
{
	return reinterpret_cast<hwr2renderer_h>(new Sdl2Gles2Renderer(window));
}

void I_SDL_DestroyGlesHwr2Renderer(hwr2renderer_h renderer)
{
	delete reinterpret_cast<Sdl2Gles2Renderer*>(renderer);
}

void I_SDL_Hwr2Present(hwr2renderer_h renderer)
{
	Sdl2Gles2Renderer* hwr2 = reinterpret_cast<Sdl2Gles2Renderer*>(renderer);
	hwr2->Present();
}
