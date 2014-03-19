#include "../doomdef.h"
#include "../command.h"
#include "../i_video.h"
#include "../d_clisrv.h"

#include <SDL.h>

typedef struct sdlmode_s
{
	Uint16 w;
	Uint16 h;
} sdlmode_t;

rendermode_t rendermode = render_soft;

boolean highcolor = true;

boolean allow_fullscreen = true;

consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static SDL_bool graphicsInitialized = SDL_FALSE;

// SDL2 vars
static SDL_Window     *window;
static SDL_Renderer   *renderer;
static Uint16          logicalWidth; // real for windowed
static Uint16          logicalHeight; // real for windowed

#define SDLI_MAX_MODES 32
static int numVidModes = 0;
static sdlmode_t sdlmodes[SDLI_MAX_MODES];

static SDL_bool Impl_CreateWindow()
{
	window = SDL_CreateWindow("Sonic Robo Blast 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 200, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (window == NULL || renderer == NULL)
	{
		return SDL_FALSE;
	}
	else
	{
		return SDL_TRUE;
	}
}

static SDL_bool Impl_DestroyWindow()
{
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}
	window = NULL;
	if (renderer != NULL)
	{
		SDL_DestroyRenderer(renderer);
	}
	renderer = NULL;
	return SDL_TRUE;
}

static SDL_bool Impl_SetMode(Uint16 width, Uint16 height, SDL_bool fullscreen)
{
	logicalWidth = width;
	logicalHeight = height;
	return SDL_TRUE;
}

static SDL_bool Impl_AddSDLMode(Uint16 width, Uint16 height)
{
	if (numVidModes >= SDLI_MAX_MODES)
	{
		return SDL_FALSE;
	}

	numVidModes += 1;
	sdlmodes[numVidModes].w = width;
	sdlmodes[numVidModes].h = height;

	return SDL_TRUE;
}

void I_StartupGraphics(void)
{
	if (graphicsInitialized)
	{
		return;
	}

	if (dedicated)
	{
		rendermode = render_none;
		return;
	}
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		if (!SDL_WasInit(SDL_INIT_VIDEO))
		{
			CONS_Printf(M_GetText("Failed to initialize SDL video: %s\n"), SDL_GetError());
			return;
		}
	}

	// Add 'supported' modes
#ifdef ANDROID
	Impl_AddSDLMode(640, 400);
#else
	Impl_AddSDLMode(320, 200);
	Impl_AddSDLMode(640, 400);
	Impl_AddSDLMode(1280, 800);
#endif

	if (!Impl_CreateWindow() || !Impl_SetMode(640, 400, SDL_FALSE))
	{
		CONS_Printf(M_GetText("SDL: Could not create window and set initial mode: %s\n"), SDL_GetError());
		return;
	}

	graphicsInitialized = SDL_TRUE;

	return;
}

void I_ShutdownGraphics(void)
{
	if (!graphicsInitialized)
	{
		return;
	}

	Impl_DestroyWindow();
	return;
}

void I_SetPalette(RGBA_t *palette)
{
	(void)palette;
}

INT32 VID_NumModes(void)
{
	return 0;
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	(void)w;
	(void)h;
	return 0;
}

void VID_PrepareModeList(void){}

INT32 VID_SetMode(INT32 modenum)
{
	(void)modenum;
	return 0;
}

const char *VID_GetModeName(INT32 modenum)
{
	(void)modenum;
	return NULL;
}

void I_UpdateNoBlit(void){}

void I_FinishUpdate(void){}

void I_UpdateNoVsync(void) {}

void I_WaitVBL(INT32 count)
{
	(void)count;
}

void I_ReadScreen(UINT8 *scr)
{
	(void)scr;
}

void I_BeginRead(void){}

void I_EndRead(void){}

