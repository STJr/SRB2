// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
/// \brief miscellaneous drawing (mainly 2d)

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_drv.h"

#include "../m_misc.h" //FIL_WriteFile()
#include "../r_draw.h" //viewborderlump
#include "../r_main.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../st_stuff.h"

#include <fcntl.h>
#include "../i_video.h"  // for rendermode != render_glide

#ifndef O_BINARY
#define O_BINARY 0
#endif

float gr_patch_scalex;
float gr_patch_scaley;

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	UINT8 id_field_length ; // 1
	UINT8 color_map_type  ; // 2
	UINT8 image_type      ; // 3
	UINT8 dummy[5]        ; // 4,  8
	INT16 x_origin        ; // 9, 10
	INT16 y_origin        ; //11, 12
	INT16 width           ; //13, 14
	INT16 height          ; //15, 16
	UINT8 image_pix_size  ; //17
	UINT8 image_descriptor; //18
} ATTRPACK TGAHeader; // sizeof is 18
#if defined(_MSC_VER)
#pragma pack()
#endif
typedef UINT8 GLRGB[3];

#define BLENDMODE PF_Translucent

static UINT8 softwaretranstogl[11]    = {  0, 25, 51, 76,102,127,153,178,204,229,255};
static UINT8 softwaretranstogl_hi[11] = {  0, 51,102,153,204,255,255,255,255,255,255};
static UINT8 softwaretranstogl_lo[11] = {  0, 12, 24, 36, 48, 60, 71, 83, 95,111,127};

//
// -----------------+
// HWR_DrawPatch    : Draw a 'tile' graphic
// Notes            : x,y : positions relative to the original Doom resolution
//                  : textes(console+score) + menus + status bar
// -----------------+
void HWR_DrawPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option)
{
	FOutVector v[4];
	FBITFIELD flags;

//  3--2
//  | /|
//  |/ |
//  0--1
	float sdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	float sdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;
	float pdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	float pdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;

	// make patch ready in hardware cache
	HWR_GetPatch(gpatch);

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		pdupx = pdupy = 2.0f;
		break;
	case V_SMALLSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupy);
		break;
	case V_MEDSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fmeddupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fmeddupy);
		break;
	}

	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx-gpatch->leftoffset*pdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy-gpatch->topoffset*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = gpatch->max_s;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = gpatch->max_t;

	flags = BLENDMODE|PF_Clip|PF_NoZClip|PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	// clip it since it is used for bunny scroll in doom I
	HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawFixedPatch(GLPatch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, INT32 option, const UINT8 *colormap)
{
	FOutVector v[4];
	FBITFIELD flags;
	float cx = FIXED_TO_FLOAT(x);
	float cy = FIXED_TO_FLOAT(y);
	UINT8 alphalevel = ((option & V_ALPHAMASK) >> V_ALPHASHIFT);

//  3--2
//  | /|
//  |/ |
//  0--1
	float sdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	float sdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;
	float pdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f*FIXED_TO_FLOAT(pscale);
	float pdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f*FIXED_TO_FLOAT(pscale);

	if (alphalevel == 12)
		alphalevel = 0;
	else if (alphalevel >= 10 && alphalevel < 13)
		return;

	// make patch ready in hardware cache
	if (!colormap)
		HWR_GetPatch(gpatch);
	else
		HWR_GetMappedPatch(gpatch, colormap);

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		pdupx = pdupy = 2.0f;
		break;
	case V_SMALLSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupy);
		break;
	case V_MEDSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fmeddupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fmeddupy);
		break;
	}

	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	if (option & V_SPLITSCREEN)
		sdupy /= 2.0f;

	if (option & V_FLIP) // Need to flip both this and sow
	{
		v[0].x = v[3].x = (cx*sdupx-(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
		v[2].x = v[1].x = (cx*sdupx+gpatch->leftoffset*pdupx)/vid.width - 1;
	}
	else
	{
		v[0].x = v[3].x = (cx*sdupx-gpatch->leftoffset*pdupx)/vid.width - 1;
		v[2].x = v[1].x = (cx*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	}

	v[0].y = v[1].y = 1-(cy*sdupy-gpatch->topoffset*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(cy*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	if (option & V_FLIP)
	{
		v[0].sow = v[3].sow = gpatch->max_s;
		v[2].sow = v[1].sow = 0.0f;
	}
	else
	{
		v[0].sow = v[3].sow = 0.0f;
		v[2].sow = v[1].sow = gpatch->max_s;
	}

	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = gpatch->max_t;

	flags = BLENDMODE|PF_Clip|PF_NoZClip|PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	// clip it since it is used for bunny scroll in doom I
	if (alphalevel)
	{
		FSurfaceInfo Surf;
		Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
		if (alphalevel == 13) Surf.FlatColor.s.alpha = softwaretranstogl_lo[cv_translucenthud.value];
		else if (alphalevel == 14) Surf.FlatColor.s.alpha = softwaretranstogl[cv_translucenthud.value];
		else if (alphalevel == 15) Surf.FlatColor.s.alpha = softwaretranstogl_hi[cv_translucenthud.value];
		else Surf.FlatColor.s.alpha = softwaretranstogl[10-alphalevel];
		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawCroppedPatch(GLPatch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, INT32 option, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h)
{
	FOutVector v[4];
	FBITFIELD flags;
	float cx = FIXED_TO_FLOAT(x);
	float cy = FIXED_TO_FLOAT(y);
	UINT8 alphalevel = ((option & V_ALPHAMASK) >> V_ALPHASHIFT);

//  3--2
//  | /|
//  |/ |
//  0--1
	float sdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	float sdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;
	float pdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f*FIXED_TO_FLOAT(pscale);
	float pdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f*FIXED_TO_FLOAT(pscale);

	if (alphalevel == 12)
		alphalevel = 0;
	else if (alphalevel >= 10 && alphalevel < 13)
		return;

	// make patch ready in hardware cache
	HWR_GetPatch(gpatch);

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		pdupx = pdupy = 2.0f;
		break;
	case V_SMALLSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupy);
		break;
	case V_MEDSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fmeddupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fmeddupy);
		break;
	}

	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x =     (cx*sdupx -           gpatch->leftoffset  * pdupx) / vid.width - 1;
	v[2].x = v[1].x =     (cx*sdupx + ((w-sx) - gpatch->leftoffset) * pdupx) / vid.width - 1;
	v[0].y = v[1].y = 1 - (cy*sdupy -           gpatch->topoffset   * pdupy) / vid.height;
	v[2].y = v[3].y = 1 - (cy*sdupy + ((h-sy) - gpatch->topoffset)  * pdupy) / vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = ((sx)/(float)gpatch->width )*gpatch->max_s;
	v[2].sow = v[1].sow = ((w )/(float)gpatch->width )*gpatch->max_s;
	v[0].tow = v[1].tow = ((sy)/(float)gpatch->height)*gpatch->max_t;
	v[2].tow = v[3].tow = ((h )/(float)gpatch->height)*gpatch->max_t;

	flags = BLENDMODE|PF_Clip|PF_NoZClip|PF_NoDepthTest;

	if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;

	// clip it since it is used for bunny scroll in doom I
	if (alphalevel)
	{
		FSurfaceInfo Surf;
		Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
		if (alphalevel == 13) Surf.FlatColor.s.alpha = softwaretranstogl_lo[cv_translucenthud.value];
		else if (alphalevel == 14) Surf.FlatColor.s.alpha = softwaretranstogl[cv_translucenthud.value];
		else if (alphalevel == 15) Surf.FlatColor.s.alpha = softwaretranstogl_hi[cv_translucenthud.value];
		else Surf.FlatColor.s.alpha = softwaretranstogl[10-alphalevel];
		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawPic(INT32 x, INT32 y, lumpnum_t lumpnum)
{
	FOutVector      v[4];
	const GLPatch_t    *patch;

	// make pic ready in hardware cache
	patch = HWR_GetPic(lumpnum);

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = 2.0f * (float)x/vid.width - 1;
	v[2].x = v[1].x = 2.0f * (float)(x + patch->width*FIXED_TO_FLOAT(vid.fdupx))/vid.width - 1;
	v[0].y = v[1].y = 1.0f - 2.0f * (float)y/vid.height;
	v[2].y = v[3].y = 1.0f - 2.0f * (float)(y + patch->height*FIXED_TO_FLOAT(vid.fdupy))/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow =  0;
	v[2].sow = v[1].sow =  patch->max_s;
	v[0].tow = v[1].tow =  0;
	v[2].tow = v[3].tow =  patch->max_t;


	//Hurdler: Boris, the same comment as above... but maybe for pics
	// it not a problem since they don't have any transparent pixel
	// if I'm right !?
	// But then, the question is: why not 0 instead of PF_Masked ?
	// or maybe PF_Environment ??? (like what I said above)
	// BP: PF_Environment don't change anything ! and 0 is undifined
	HWD.pfnDrawPolygon(NULL, v, 4, BLENDMODE | PF_NoDepthTest | PF_Clip | PF_NoZClip);
}

// ==========================================================================
//                                                            V_VIDEO.C STUFF
// ==========================================================================


// --------------------------------------------------------------------------
// Fills a box of pixels using a flat texture as a pattern
// --------------------------------------------------------------------------
void HWR_DrawFlatFill (INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum)
{
	FOutVector  v[4];
	double dflatsize;
	INT32 flatflag;
	const size_t len = W_LumpLength(flatlumpnum);

	switch (len)
	{
		case 4194304: // 2048x2048 lump
			dflatsize = 2048.0f;
			flatflag = 2047;
			break;
		case 1048576: // 1024x1024 lump
			dflatsize = 1024.0f;
			flatflag = 1023;
			break;
		case 262144:// 512x512 lump
			dflatsize = 512.0f;
			flatflag = 511;
			break;
		case 65536: // 256x256 lump
			dflatsize = 256.0f;
			flatflag = 255;
			break;
		case 16384: // 128x128 lump
			dflatsize = 128.0f;
			flatflag = 127;
			break;
		case 1024: // 32x32 lump
			dflatsize = 32.0f;
			flatflag = 31;
			break;
		default: // 64x64 lump
			dflatsize = 64.0f;
			flatflag = 63;
			break;
	}

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = (x - 160.0f)/160.0f;
	v[2].x = v[1].x = ((x+w) - 160.0f)/160.0f;
	v[0].y = v[1].y = -(y - 100.0f)/100.0f;
	v[2].y = v[3].y = -((y+h) - 100.0f)/100.0f;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	// flat is 64x64 lod and texture offsets are [0.0, 1.0]
	v[0].sow = v[3].sow = (float)((x & flatflag)/dflatsize);
	v[2].sow = v[1].sow = (float)(v[0].sow + w/dflatsize);
	v[0].tow = v[1].tow = (float)((y & flatflag)/dflatsize);
	v[2].tow = v[3].tow = (float)(v[0].tow + h/dflatsize);

	HWR_GetFlat(flatlumpnum);

	//Hurdler: Boris, the same comment as above... but maybe for pics
	// it not a problem since they don't have any transparent pixel
	// if I'm right !?
	// BTW, I see we put 0 for PFs, and If I'm right, that
	// means we take the previous PFs as default
	// how can we be sure they are ok?
	HWD.pfnDrawPolygon(NULL, v, 4, PF_NoDepthTest); //PF_Translucent);
}


// --------------------------------------------------------------------------
// Fade down the screen so that the menu drawn on top of it looks brighter
// --------------------------------------------------------------------------
//  3--2
//  | /|
//  |/ |
//  0--1
void HWR_FadeScreenMenuBack(UINT32 color, INT32 height)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;

	// setup some neat-o translucency effect
	if (!height) //cool hack 0 height is full height
		height = vid.height;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y =  1.0f-((height<<1)/(float)vid.height);
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = 1.0f;
	v[0].tow = v[1].tow = 1.0f;
	v[2].tow = v[3].tow = 0.0f;

	Surf.FlatColor.rgba = UINT2RGBA(color);
	Surf.FlatColor.s.alpha = (UINT8)((0xff/2) * ((float)height / vid.height)); //calum: varies console alpha
	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// Draw the console background with translucency support
void HWR_DrawConsoleBack(UINT32 color, INT32 height)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;

	// setup some neat-o translucency effect
	if (!height) //cool hack 0 height is full height
		height = vid.height;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y =  1.0f-((height<<1)/(float)vid.height);
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = 1.0f;
	v[0].tow = v[1].tow = 1.0f;
	v[2].tow = v[3].tow = 0.0f;

	Surf.FlatColor.rgba = UINT2RGBA(color);
	Surf.FlatColor.s.alpha = 0x80;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}


// ==========================================================================
//                                                             R_DRAW.C STUFF
// ==========================================================================

// ------------------
// HWR_DrawViewBorder
// Fill the space around the view window with a Doom flat texture, draw the
// beveled edges.
// 'clearlines' is useful to clear the heads up messages, when the view
// window is reduced, it doesn't refresh all the view borders.
// ------------------
void HWR_DrawViewBorder(INT32 clearlines)
{
	INT32 x, y;
	INT32 top, side;
	INT32 baseviewwidth, baseviewheight;
	INT32 basewindowx, basewindowy;
	GLPatch_t *patch;

//    if (gr_viewwidth == vid.width)
//        return;

	if (!clearlines)
		clearlines = BASEVIDHEIGHT; // refresh all

	// calc view size based on original game resolution
	baseviewwidth =  FixedInt(FixedDiv(FLOAT_TO_FIXED(gr_viewwidth), vid.fdupx)); //(cv_viewsize.value * BASEVIDWIDTH/10)&~7;
	baseviewheight = FixedInt(FixedDiv(FLOAT_TO_FIXED(gr_viewheight), vid.fdupy));
	top = FixedInt(FixedDiv(FLOAT_TO_FIXED(gr_baseviewwindowy), vid.fdupy));
	side = FixedInt(FixedDiv(FLOAT_TO_FIXED(gr_viewwindowx), vid.fdupx));

	// top
	HWR_DrawFlatFill(0, 0,
		BASEVIDWIDTH, (top < clearlines ? top : clearlines),
		st_borderpatchnum);

	// left
	if (top < clearlines)
		HWR_DrawFlatFill(0, top, side,
			(clearlines-top < baseviewheight ? clearlines-top : baseviewheight),
			st_borderpatchnum);

	// right
	if (top < clearlines)
		HWR_DrawFlatFill(side + baseviewwidth, top, side,
			(clearlines-top < baseviewheight ? clearlines-top : baseviewheight),
			st_borderpatchnum);

	// bottom
	if (top + baseviewheight < clearlines)
		HWR_DrawFlatFill(0, top + baseviewheight,
			BASEVIDWIDTH, BASEVIDHEIGHT, st_borderpatchnum);

	//
	// draw the view borders
	//

	basewindowx = (BASEVIDWIDTH - baseviewwidth)>>1;
	if (baseviewwidth == BASEVIDWIDTH)
		basewindowy = 0;
	else
		basewindowy = top;

	// top edge
	if (clearlines > basewindowy - 8)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_T], PU_CACHE);
		for (x = 0; x < baseviewwidth; x += 8)
			HWR_DrawPatch(patch, basewindowx + x, basewindowy - 8,
				0);
	}

	// bottom edge
	if (clearlines > basewindowy + baseviewheight)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_B], PU_CACHE);
		for (x = 0; x < baseviewwidth; x += 8)
			HWR_DrawPatch(patch, basewindowx + x,
				basewindowy + baseviewheight, 0);
	}

	// left edge
	if (clearlines > basewindowy)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_L], PU_CACHE);
		for (y = 0; y < baseviewheight && basewindowy + y < clearlines;
			y += 8)
		{
			HWR_DrawPatch(patch, basewindowx - 8, basewindowy + y,
				0);
		}
	}

	// right edge
	if (clearlines > basewindowy)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_R], PU_CACHE);
		for (y = 0; y < baseviewheight && basewindowy+y < clearlines;
			y += 8)
		{
			HWR_DrawPatch(patch, basewindowx + baseviewwidth,
				basewindowy + y, 0);
		}
	}

	// Draw beveled corners.
	if (clearlines > basewindowy - 8)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_TL],
				PU_CACHE),
			basewindowx - 8, basewindowy - 8, 0);

	if (clearlines > basewindowy - 8)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_TR],
				PU_CACHE),
			basewindowx + baseviewwidth, basewindowy - 8, 0);

	if (clearlines > basewindowy+baseviewheight)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_BL],
				PU_CACHE),
			basewindowx - 8, basewindowy + baseviewheight, 0);

	if (clearlines > basewindowy + baseviewheight)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_BR],
				PU_CACHE),
			basewindowx + baseviewwidth,
			basewindowy + baseviewheight, 0);
}


// ==========================================================================
//                                                     AM_MAP.C DRAWING STUFF
// ==========================================================================

// Clear the automap part of the screen
void HWR_clearAutomap(void)
{
	FRGBAFloat fColor = {0, 0, 0, 1};

	// minx,miny,maxx,maxy
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
	HWD.pfnClearBuffer(true, true, &fColor);
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}


// -----------------+
// HWR_drawAMline   : draw a line of the automap (the clipping is already done in automap code)
// Arg              : color is a RGB 888 value
// -----------------+
void HWR_drawAMline(const fline_t *fl, INT32 color)
{
	F2DCoord v1, v2;
	RGBA_t color_rgba;

	color_rgba = V_GetColor(color);

	v1.x = ((float)fl->a.x-(vid.width/2.0f))*(2.0f/vid.width);
	v1.y = ((float)fl->a.y-(vid.height/2.0f))*(2.0f/vid.height);

	v2.x = ((float)fl->b.x-(vid.width/2.0f))*(2.0f/vid.width);
	v2.y = ((float)fl->b.y-(vid.height/2.0f))*(2.0f/vid.height);

	HWD.pfnDraw2DLine(&v1, &v2, color_rgba);
}


// -----------------+
// HWR_DrawFill     : draw flat coloured rectangle, with no texture
// -----------------+
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float sdupx, sdupy;

	if (w < 0 || h < 0)
		return; // consistency w/ software

//  3--2
//  | /|
//  |/ |
//  0--1
	sdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	sdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;

	if (color & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx + w*sdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy + h*sdupy)/vid.height;

	//Hurdler: do we still use this argb color? if not, we should remove it
	v[0].argb = v[1].argb = v[2].argb = v[3].argb = 0xff00ff00; //;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].sow = v[3].sow = 0.0f;
	v[2].sow = v[1].sow = 1.0f;
	v[0].tow = v[1].tow = 0.0f;
	v[2].tow = v[3].tow = 1.0f;

	Surf.FlatColor = V_GetColor(color);

	HWD.pfnDrawPolygon(&Surf, v, 4,
		PF_Modulated|PF_NoTexture|PF_NoDepthTest);
}

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "png.h"
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // PNG is only used if write is supported (see ../m_misc.c)
 #endif
#endif

#ifndef USE_PNG
// --------------------------------------------------------------------------
// save screenshots with TGA format
// --------------------------------------------------------------------------
static inline boolean saveTGA(const char *file_name, void *buffer,
	INT32 width, INT32 height)
{
	INT32 fd;
	TGAHeader tga_hdr;
	INT32 i;
	UINT8 *buf8 = buffer;

	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (fd < 0)
		return false;

	memset(&tga_hdr, 0, sizeof (tga_hdr));
	tga_hdr.width = SHORT(width);
	tga_hdr.height = SHORT(height);
	tga_hdr.image_pix_size = 24;
	tga_hdr.image_type = 2;
	tga_hdr.image_descriptor = 32;

	if ( -1 == write(fd, &tga_hdr, sizeof (TGAHeader)))
	{
		close(fd);
		return false;
	}
	// format to 888 BGR
	for (i = 0; i < width * height * 3; i+=3)
	{
		const UINT8 temp = buf8[i];
		buf8[i] = buf8[i+2];
		buf8[i+2] = temp;
	}
	if ( -1 == write(fd, buffer, width * height * 3))
	{
		close(fd);
		return false;
	}
	close(fd);
	return true;
}
#endif

// --------------------------------------------------------------------------
// screen shot
// --------------------------------------------------------------------------

UINT8 *HWR_GetScreenshot(void)
{
	UINT8 *buf = malloc(vid.width * vid.height * 3 * sizeof (*buf));

	if (!buf)
		return NULL;
	// returns 24bit 888 RGB
	HWD.pfnReadRect(0, 0, vid.width, vid.height, vid.width * 3, (void *)buf);
	return buf;
}

boolean HWR_Screenshot(const char *lbmname)
{
	boolean ret;
	UINT8 *buf = malloc(vid.width * vid.height * 3 * sizeof (*buf));

	if (!buf)
		return false;

	// returns 24bit 888 RGB
	HWD.pfnReadRect(0, 0, vid.width, vid.height, vid.width * 3, (void *)buf);

#ifdef USE_PNG
	ret = M_SavePNG(lbmname, buf, vid.width, vid.height, false);
#else
	ret = saveTGA(lbmname, buf, vid.width, vid.height);
#endif
	free(buf);
	return ret;
}

#endif //HWRENDER
