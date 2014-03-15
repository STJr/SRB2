// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 graphics stuff for NDS


#include "../doomdef.h"
#include "../command.h"
#include "../i_video.h"

#include "../hardware/hw_drv.h"
#include "../hardware/hw_main.h"
#include "r_nds3d.h"

rendermode_t rendermode = render_opengl;

boolean highcolor = false;

boolean allow_fullscreen = false;

consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

void I_StartupGraphics(void)
{
	vid.width = 256;
	vid.height = 192;
	vid.bpp = 1;
	vid.rowbytes = vid.width * vid.bpp;
	vid.recalc = true;

	HWD.pfnInit             = NDS3D_Init;
	HWD.pfnShutdown         = NDS3D_Shutdown;
	HWD.pfnFinishUpdate     = NDS3D_FinishUpdate;
	HWD.pfnDraw2DLine       = NDS3D_Draw2DLine;
	HWD.pfnDrawPolygon      = NDS3D_DrawPolygon;
	HWD.pfnSetBlend         = NDS3D_SetBlend;
	HWD.pfnClearBuffer      = NDS3D_ClearBuffer;
	HWD.pfnSetTexture       = NDS3D_SetTexture;
	HWD.pfnReadRect         = NDS3D_ReadRect;
	HWD.pfnGClipRect        = NDS3D_GClipRect;
	HWD.pfnClearMipMapCache = NDS3D_ClearMipMapCache;
	HWD.pfnSetSpecialState  = NDS3D_SetSpecialState;
	HWD.pfnSetPalette       = NDS3D_SetPalette;
	HWD.pfnGetTextureUsed   = NDS3D_GetTextureUsed;
	HWD.pfnDrawMD2          = NDS3D_DrawMD2;
	HWD.pfnDrawMD2i         = NDS3D_DrawMD2i;
	HWD.pfnSetTransform     = NDS3D_SetTransform;
	HWD.pfnGetRenderVersion = NDS3D_GetRenderVersion;

	videoSetMode(MODE_0_3D);
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_TEXTURE);
	vramSetBankC(VRAM_C_TEXTURE);
	vramSetBankD(VRAM_D_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);

	glInit();

	glEnable(GL_TEXTURE_2D);

	glClearColor(16,16,16,31);
	glClearPolyID(63);
	glClearDepth(0x7FFF);

	glViewport(0, 0, vid.width - 1, vid.height - 1);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(fov, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(1.0f, 1.0f, -1.0f);

	HWD.pfnInit(I_Error);
	HWR_Startup();
}

void I_ShutdownGraphics(void){}

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

void I_FinishUpdate(void)
{
	HWD.pfnFinishUpdate(true);
}

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
