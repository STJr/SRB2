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
/// \brief NDS 3D API for SRB2.
//
// In an ideal world, we would share as much code as possible with r_opengl.c,
// but this will do for now.

#include "../doomtype.h"
#include "../hardware/hw_defs.h"
#include "../hardware/hw_dll.h"
#include "../hardware/hw_md2.h"
#include "r_nds3d.h"

static I_Error_t I_Error_GL = NULL;

#define NOTEXTURE_NUM     0     // small white texture
#define FIRST_TEX_AVAIL   (NOTEXTURE_NUM + 1)
#define MAX_SRB2_TEXTURES      256

FCOORD NEAR_CLIPPING_PLANE = 0.9f;
float fov = 90.0f;

static FBITFIELD CurrentPolyFlags = 0xFFFFFFFF;
static UINT32 CurrentGLPolyFmt = POLY_CULL_NONE;
static UINT8 CurrentPolyAlpha = 31;
static UINT16 myPaletteData[256];
static FTextureInfo* gr_cachetail = NULL;
static FTextureInfo* gr_cachehead = NULL;
static INT32 NextTexAvail = FIRST_TEX_AVAIL;
static UINT32 tex_downloaded = 0;
static INT32 texids[MAX_SRB2_TEXTURES];
static boolean scalehack = false;


static void GenerateTextureNames(void)
{
	glGenTextures(MAX_SRB2_TEXTURES - 1, texids + 1);
	texids[NOTEXTURE_NUM] = 0;
}

static void Flush(void)
{
	// Delete all textures at once, since libnds's glDeleteTextures seems to be buggy.
	glResetTextures();
	GenerateTextureNames();
	while (gr_cachehead)
	{
		gr_cachehead->downloaded = 0;
		gr_cachehead = gr_cachehead->nextmipmap;
	}
	gr_cachetail = gr_cachehead = NULL;
	NextTexAvail = FIRST_TEX_AVAIL;
	tex_downloaded = 0;
}

static void SetNoTexture(void)
{
	// Set small white texture.
	if (tex_downloaded != NOTEXTURE_NUM)
	{
		glBindTexture(GL_TEXTURE_2D, texids[NOTEXTURE_NUM]);
		tex_downloaded = NOTEXTURE_NUM;
	}
}


static void SetAlpha(UINT8 alpha)
{
	CurrentPolyAlpha = alpha >> 3;
	glPolyFmt(CurrentGLPolyFmt | POLY_ALPHA(CurrentPolyAlpha));
}



boolean NDS3D_Init(I_Error_t ErrorFunction)
{
	I_Error_GL = ErrorFunction;
	glPolyFmt(CurrentGLPolyFmt | POLY_ALPHA(CurrentPolyAlpha));
	GenerateTextureNames();
	return true;
}

void NDS3D_Shutdown(void) {}

void NDS3D_SetPalette(RGBA_t *ppal, RGBA_t *pgamma)
{
	INT32 i;

	for (i = 0; i < 256; i++)
	{
		UINT8 red   = (UINT8)min((ppal[i].s.red*pgamma->s.red)/127,     255) >> 3;
		UINT8 green = (UINT8)min((ppal[i].s.green*pgamma->s.green)/127, 255) >> 3;
		UINT8 blue  = (UINT8)min((ppal[i].s.blue*pgamma->s.blue)/127,   255) >> 3;

		myPaletteData[i] = ARGB16(ppal[i].s.alpha ? 1 : 0, red, green, blue);
	}

	Flush();
}

void NDS3D_FinishUpdate(INT32 waitvbl)
{
	(void)waitvbl;

	glFlush(0);
}

void NDS3D_Draw2DLine(F2DCoord *v1, F2DCoord *v2, RGBA_t Color)
{
	(void)v1;
	(void)v2;
	(void)Color;
}

void NDS3D_DrawPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags)
{
	FUINT i;

	NDS3D_SetBlend(PolyFlags);

	// If Modulated, mix the surface colour to the texture
	if ((CurrentPolyFlags & PF_Modulated) && pSurf)
	{
		glColor3b(pSurf->FlatColor.s.red, pSurf->FlatColor.s.green, pSurf->FlatColor.s.blue);
		SetAlpha(pSurf->FlatColor.s.alpha);
	}

	// libnds doesn't have GL_TRIANGLE_FAN, so use GL_TRIANGLE_STRIP instead
	glBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i < iNumPts; i++)
	{
		FUINT index = (i & 1) ? (i >> 1) : (iNumPts - 1 - (i >> 1));
		FLOAT x, y, z;

		if (scalehack)
		{
			x = pOutVerts[index].x/4096.0f;
			y = pOutVerts[index].y/4096.0f;
			z = pOutVerts[index].z/4096.0f;
		}
		else
		{
			x = pOutVerts[index].x;
			y = pOutVerts[index].y;
			z = pOutVerts[index].z;
		}

		glTexCoord2f(pOutVerts[index].sow, pOutVerts[index].tow);
		glVertex3f(x,y,z);
	}
	glEnd();
}

void NDS3D_SetBlend(FBITFIELD PolyFlags)
{
	FBITFIELD Xor = PolyFlags ^ CurrentPolyFlags;

	if (Xor & (PF_NoTexture|PF_Modulated))
	{
		if (Xor&PF_Modulated)
		{
			if(!(PolyFlags & PF_Modulated))
			{
				glColor3b(255, 255, 255);
				CurrentPolyAlpha = 31;
			}
		}

		if (PolyFlags & PF_NoTexture)
		{
			SetNoTexture();
		}
	}

	CurrentPolyFlags = PolyFlags;
	glPolyFmt(CurrentGLPolyFmt | POLY_ALPHA(CurrentPolyAlpha));
}

void NDS3D_ClearBuffer(FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor)
{
	(void)ClearColor;

	if (ColorMask && ClearColor)
	{
		// TODO: Fixed-ify
		glClearColor((uint8)(ClearColor->red*31),
			(uint8)(ClearColor->green*31),
			(uint8)(ClearColor->blue*31),
			(uint8)(ClearColor->alpha*31));
	}

	if (DepthMask)
		glClearDepth(GL_MAX_DEPTH);

	NDS3D_SetBlend(DepthMask ? PF_Occlude | CurrentPolyFlags : CurrentPolyFlags&~PF_Occlude);
}

void NDS3D_SetTexture(FTextureInfo *TexInfo)
{
	if (!TexInfo)
	{
		SetNoTexture();
		return;
	}
	else if (TexInfo->downloaded)
	{
		if (TexInfo->downloaded != tex_downloaded)
		{
			glBindTexture(GL_TEXTURE_2D, texids[TexInfo->downloaded]);
			tex_downloaded = TexInfo->downloaded;
		}
	}
	else if (TexInfo->grInfo.data)
	{
		UINT8 wtype, htype;
		INT32 texparam = GL_TEXTURE_COLOR0_TRANSPARENT;

		// We rely on the numerical values of GL_TEXTURE_SIZE_ENUM here.
		wtype = TEXTURE_SIZE_8;
		while(TexInfo->width > 1 << (wtype + 3)) wtype++;

		htype = TEXTURE_SIZE_8;
		while(TexInfo->height > 1 << (htype + 3)) htype++;

		TexInfo->downloaded = NextTexAvail++;
		tex_downloaded = TexInfo->downloaded;
		glBindTexture(GL_TEXTURE_2D, texids[TexInfo->downloaded]);

		if(!glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB256, wtype, htype, 0, TEXGEN_TEXCOORD, TexInfo->grInfo.data))
		{
			// HACK: If we're out of memory, flush and try again.
			// This will result in artefacts for one frame.
			Flush();
			TexInfo->downloaded = 0;
			NDS3D_SetTexture(TexInfo);
			return;
		}

		if (TexInfo->downloaded > FIRST_TEX_AVAIL)
		{
			// We already have a texture using the palette, so it's already in VRAM
			glAssignColorTable(GL_TEXTURE_2D, texids[FIRST_TEX_AVAIL]);
		}
		else
		{
			// Generate the palette in hardware
			glColorTableEXT(0, 0, 256, 0, 0, myPaletteData);
		}

		if (TexInfo->flags & TF_WRAPX)
			texparam |= GL_TEXTURE_WRAP_S;

		if (TexInfo->flags & TF_WRAPY)
			texparam |= GL_TEXTURE_WRAP_T;

		glTexParameter(0, texparam);

		TexInfo->nextmipmap = NULL;
		if (gr_cachetail)
		{
			gr_cachetail->nextmipmap = TexInfo;
			gr_cachetail = TexInfo;
		}
		else
			gr_cachetail = gr_cachehead = TexInfo;
	}
}

void NDS3D_ReadRect(INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT16 *dst_data)
{
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	(void)dst_stride;
	(void)dst_data;
}

void NDS3D_GClipRect(INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip)
{
	(void)minx;
	(void)miny;
	(void)maxx;
	(void)maxy;
	//glViewport(minx, vid.height-maxy, maxx-minx, maxy-miny);
	NEAR_CLIPPING_PLANE = nearclip;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
	glMatrixMode(GL_MODELVIEW);
}

void NDS3D_ClearMipMapCache(void) {}

void NDS3D_SetSpecialState(hwdspecialstate_t IdState, INT32 Value)
{
	(void)IdState;
	(void)Value;
}

void NDS3D_DrawMD2(INT32 *gl_cmd_buffer, md2_frame_t *frame, FTransform *pos, float scale)
{
	(void)gl_cmd_buffer;
	(void)frame;
	(void)pos;
	(void)scale;
}

void NDS3D_DrawMD2i(INT32 *gl_cmd_buffer, md2_frame_t *frame, UINT32 duration, UINT32 tics, md2_frame_t *nextframe, FTransform *pos, float scale, UINT8 flipped, UINT8 *color)
{
	(void)gl_cmd_buffer;
	(void)frame;
	(void)duration;
	(void)tics;
	(void)nextframe;
	(void)pos;
	(void)scale;
	(void)flipped;
	(void)color;
}

void NDS3D_SetTransform(FTransform *ptransform)
{
	static INT32 special_splitscreen;
	glLoadIdentity();
	if (ptransform)
	{
		scalehack = true;

		glScalef(ptransform->scalex*4096.0f, ptransform->scaley*4096.0f, -ptransform->scalez*4096.0f);
		glRotatef(ptransform->anglex       , 1.0f, 0.0f, 0.0f);
		glRotatef(ptransform->angley+270.0f, 0.0f, 1.0f, 0.0f);
		glTranslatef(-ptransform->x/4096.0f, -ptransform->z/4096.0f, -ptransform->y/4096.0f);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		special_splitscreen = (ptransform->splitscreen && ptransform->fovxangle == 90.0f);
		if (special_splitscreen)
			gluPerspective(53.13l, 2*ASPECT_RATIO,  // 53.13 = 2*atan(0.5)
			                NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
		else
			gluPerspective(ptransform->fovxangle, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);

		glMatrixMode(GL_MODELVIEW);
	}
	else
	{
		scalehack = false;

		glScalef(1.0f, 1.0f, -1.0f);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if (special_splitscreen)
			gluPerspective(53.13l, 2*ASPECT_RATIO,  // 53.13 = 2*atan(0.5)
			                NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
		else
			//Hurdler: is "fov" correct?
			gluPerspective(fov, ASPECT_RATIO, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);

		glMatrixMode(GL_MODELVIEW);
	}
}

INT32 NDS3D_GetTextureUsed(void)
{
	return 0;
}

INT32 NDS3D_GetRenderVersion(void)
{
	return 0;
}
