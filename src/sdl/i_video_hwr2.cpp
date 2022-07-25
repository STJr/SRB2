extern "C" {
#include "../i_video.h"
}

#include "../hwr2/gles2/renderer.hpp"

class Sdl2Gles2Renderer : public Gles2Renderer
{
public:
	virtual void Present()
	{}
};

hwr2renderer_h I_GetHwr2Renderer()
{
	return reinterpret_cast<hwr2renderer_h>(new Sdl2Gles2Renderer());
}
