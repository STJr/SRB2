#include "../doomdef.h"
#include "../command.h"
#include "../i_video.h"

rendermode_t rendermode = render_none;
rendermode_t chosenrendermode = render_none;

boolean highcolor = false;

boolean allow_fullscreen = false;

consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

void I_StartupGraphics(void){}
void I_ShutdownGraphics(void){}

void I_StartupOpenGL(void){}

void I_SetPalette(RGBA_t *palette)
{
	(void)palette;
}

INT32 I_NumVideoModes(void)
{
	return 0;
}

INT32 I_GetVideoModeForSize(INT32 w, INT32 h)
{
	(void)w;
	(void)h;
	return 0;
}

INT32 I_SetVideoMode(INT32 modenum)
{
	(void)modenum;
	return 0;
}

boolean I_CheckRenderer(void)
{
	return false;
}

void I_CheckGLLoaded(rendermode_t oldrender) {}

const char *I_GetVideoModeName(INT32 modenum)
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

