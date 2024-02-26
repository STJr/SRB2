// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_draw.c
/// \brief miscellaneous drawing (mainly 2d)

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_main.h"
#include "hw_glob.h"
#include "hw_drv.h"

#include "../m_misc.h" //FIL_WriteFile()
#include "../r_main.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../st_stuff.h"
#include "../p_local.h" // stplyr
#include "../g_game.h" // players
#include "../f_finale.h" // fade color factors

#include <fcntl.h>
#include "../i_video.h"

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

static UINT8 softwaretranstogl[11]    = {  0, 25, 51, 76,102,127,153,178,204,229,255};
static UINT8 softwaretranstogl_hi[11] = {  0, 51,102,153,204,255,255,255,255,255,255};
static UINT8 softwaretranstogl_lo[11] = {  0, 12, 24, 36, 48, 60, 71, 83, 95,111,127};

void HWR_DrawStretchyFixedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap)
{
	FOutVector v[4];
	FBITFIELD flags;
	float cx = FIXED_TO_FLOAT(x);
	float cy = FIXED_TO_FLOAT(y);
	UINT8 alphalevel = ((option & V_ALPHAMASK) >> V_ALPHASHIFT);
	UINT8 blendmode = ((option & V_BLENDMASK) >> V_BLENDSHIFT);
	UINT8 opacity = 0xFF;
	GLPatch_t *hwrPatch;

//  3--2
//  | /|
//  |/ |
//  0--1
	float dup, fscalew, fscaleh, fwidth, fheight;

	UINT8 perplayershuffle = 0;

	// make patch ready in hardware cache
	if (!colormap)
		HWR_GetPatch(gpatch);
	else
		HWR_GetMappedPatch(gpatch, colormap);

	hwrPatch = ((GLPatch_t *)gpatch->hardware);

	if (alphalevel)
	{
		if (alphalevel == 10) opacity = softwaretranstogl_lo[st_translucency]; // V_HUDTRANSHALF
		else if (alphalevel == 11) opacity = softwaretranstogl[st_translucency]; // V_HUDTRANS
		else if (alphalevel == 12) opacity = softwaretranstogl_hi[st_translucency]; // V_HUDTRANSDOUBLE
		else opacity = softwaretranstogl[10-alphalevel];
	}

	dup = (float)vid.dup;

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		dup = 1.0f;
		break;
	case V_SMALLSCALEPATCH:
		dup = (float)vid.smalldup;
		break;
	case V_MEDSCALEPATCH:
		dup = (float)vid.meddup;
		break;
	}

	fscalew = fscaleh = FIXED_TO_FLOAT(pscale);
	if (vscale != pscale)
		fscaleh = FIXED_TO_FLOAT(vscale);

	// See my comments in v_video.c's V_DrawFixedPatch
	// -- Monster Iestyn 29/10/18
	{
		float offsetx = 0.0f, offsety = 0.0f;

		// left offset
		if (option & V_FLIP)
			offsetx = (float)(gpatch->width - gpatch->leftoffset) * fscalew;
		else
			offsetx = (float)(gpatch->leftoffset) * fscalew;

		// top offset
		// TODO: make some kind of vertical version of V_FLIP
		offsety = (float)(gpatch->topoffset) * fscaleh;

		cx -= offsetx;
		cy -= offsety;
	}

	if (splitscreen && (option & V_PERPLAYER))
	{
		float adjusty = ((option & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
		fscaleh /= 2;
		cy /= 2;
#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			float adjustx = ((option & V_NOSCALESTART) ? vid.width : BASEVIDWIDTH)/2.0f;
			fscalew /= 2;
			cx /= 2;
			if (stplyr == &players[displayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				option &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				cx += adjustx;
				option &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				cy += adjusty;
				option &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[fourthdisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				cx += adjustx;
				cy += adjusty;
				option &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 1;
				option &= ~V_SNAPTOBOTTOM;
			}
			else //if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 2;
				cy += adjusty;
				option &= ~V_SNAPTOTOP;
			}
		}
	}

	if (!(option & V_NOSCALESTART))
	{
		cx = cx * dup;
		cy = cy * dup;

		if (!(option & V_SCALEPATCHMASK))
		{
			// if it's meant to cover the whole screen, black out the rest (ONLY IF TOP LEFT ISN'T TRANSPARENT)
			// cx and cy are possibly *slightly* off from float maths
			// This is done before here compared to software because we directly alter cx and cy to centre
			if (opacity == 0xFF && cx >= -0.1f && cx <= 0.1f && gpatch->width == BASEVIDWIDTH && cy >= -0.1f && cy <= 0.1f && gpatch->height == BASEVIDHEIGHT)
			{
				const column_t *column = &gpatch->columns[0];
				if (column->num_posts && !column->posts[0].topdelta)
				{
					const UINT8 *source = column->pixels;
					HWR_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, source[0]);
				}
			}
			// centre screen
			if (fabsf((float)vid.width - (float)BASEVIDWIDTH * dup) > 1.0E-36f)
			{
				if (option & V_SNAPTORIGHT)
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dup));
				else if (!(option & V_SNAPTOLEFT))
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dup))/2;
				if (perplayershuffle & 4)
					cx -= ((float)vid.width - ((float)BASEVIDWIDTH * dup))/4;
				else if (perplayershuffle & 8)
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dup))/4;
			}
			if (fabsf((float)vid.height - (float)BASEVIDHEIGHT * dup) > 1.0E-36f)
			{
				if (option & V_SNAPTOBOTTOM)
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup));
				else if (!(option & V_SNAPTOTOP))
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup))/2;
				if (perplayershuffle & 1)
					cy -= ((float)vid.height - ((float)BASEVIDHEIGHT * dup))/4;
				else if (perplayershuffle & 2)
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup))/4;
			}
		}
	}

	if (pscale != FRACUNIT || vscale != FRACUNIT || (splitscreen && option & V_PERPLAYER))
	{
		fwidth = (float)(gpatch->width) * fscalew * dup;
		fheight = (float)(gpatch->height) * fscaleh * dup;
	}
	else
	{
		fwidth = (float)(gpatch->width) * dup;
		fheight = (float)(gpatch->height) * dup;
	}

	// positions of the cx, cy, are between 0 and vid.width/vid.height now, we need them to be between -1 and 1
	cx = -1 + (cx / (vid.width/2));
	cy = 1 - (cy / (vid.height/2));

	// fwidth and fheight are similar
	fwidth /= vid.width / 2;
	fheight /= vid.height / 2;

	// set the polygon vertices to the right positions
	v[0].x = v[3].x = cx;
	v[2].x = v[1].x = cx + fwidth;

	v[0].y = v[1].y = cy;
	v[2].y = v[3].y = cy - fheight;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	if (option & V_FLIP)
	{
		v[0].s = v[3].s = hwrPatch->max_s;
		v[2].s = v[1].s = 0.0f;
	}
	else
	{
		v[0].s = v[3].s = 0.0f;
		v[2].s = v[1].s = hwrPatch->max_s;
	}

	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = hwrPatch->max_t;

	// clip it since it is used for bunny scroll in doom I
	flags = HWR_GetBlendModeFlag(blendmode+1)|PF_NoDepthTest;

	if (alphalevel)
	{
		FSurfaceInfo Surf;
		Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;
		Surf.PolyColor.s.alpha = opacity;
		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawCroppedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h)
{
	FOutVector v[4];
	FBITFIELD flags;
	float cx = FIXED_TO_FLOAT(x);
	float cy = FIXED_TO_FLOAT(y);
	UINT8 alphalevel = ((option & V_ALPHAMASK) >> V_ALPHASHIFT);
	UINT8 blendmode = ((option & V_BLENDMASK) >> V_BLENDSHIFT);
	GLPatch_t *hwrPatch;

//  3--2
//  | /|
//  |/ |
//  0--1
	float dup, fscalew, fscaleh, fwidth, fheight;

	UINT8 perplayershuffle = 0;

	// make patch ready in hardware cache
	if (!colormap)
		HWR_GetPatch(gpatch);
	else
		HWR_GetMappedPatch(gpatch, colormap);

	hwrPatch = ((GLPatch_t *)gpatch->hardware);

	dup = (float)vid.dup;

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		dup = 1.0f;
		break;
	case V_SMALLSCALEPATCH:
		dup = (float)vid.smalldup;
		break;
	case V_MEDSCALEPATCH:
		dup = (float)vid.meddup;
		break;
	}

	fscalew = fscaleh = FIXED_TO_FLOAT(pscale);
	if (vscale != pscale)
		fscaleh = FIXED_TO_FLOAT(vscale);

	cx -= (float)(gpatch->leftoffset) * fscalew;
	cy -= (float)(gpatch->topoffset) * fscaleh;

	if (splitscreen && (option & V_PERPLAYER))
	{
		float adjusty = ((option & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
		fscaleh /= 2;
		cy /= 2;
#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			float adjustx = ((option & V_NOSCALESTART) ? vid.width : BASEVIDWIDTH)/2.0f;
			fscalew /= 2;
			cx /= 2;
			if (stplyr == &players[displayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				option &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				cx += adjustx;
				option &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				cy += adjusty;
				option &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[fourthdisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(option & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				cx += adjustx;
				cy += adjusty;
				option &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 1;
				option &= ~V_SNAPTOBOTTOM;
			}
			else //if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(option & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 2;
				cy += adjusty;
				option &= ~V_SNAPTOTOP;
			}
		}
	}

	if (!(option & V_NOSCALESTART))
	{
		cx = cx * dup;
		cy = cy * dup;

		if (!(option & V_SCALEPATCHMASK))
		{
			// if it's meant to cover the whole screen, black out the rest
			// no the patch is cropped do not do this ever

			// centre screen
			if (fabsf((float)vid.width - (float)BASEVIDWIDTH * dup) > 1.0E-36f)
			{
				if (option & V_SNAPTORIGHT)
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dup));
				else if (!(option & V_SNAPTOLEFT))
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dup))/2;
				if (perplayershuffle & 4)
					cx -= ((float)vid.width - ((float)BASEVIDWIDTH * dup))/4;
				else if (perplayershuffle & 8)
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dup))/4;
			}
			if (fabsf((float)vid.height - (float)BASEVIDHEIGHT * dup) > 1.0E-36f)
			{
				if (option & V_SNAPTOBOTTOM)
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup));
				else if (!(option & V_SNAPTOTOP))
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup))/2;
				if (perplayershuffle & 1)
					cy -= ((float)vid.height - ((float)BASEVIDHEIGHT * dup))/4;
				else if (perplayershuffle & 2)
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup))/4;
			}
		}
	}

	fwidth = FIXED_TO_FLOAT(w);
	fheight = FIXED_TO_FLOAT(h);

	if (sx + w > gpatch->width<<FRACBITS)
		fwidth = FIXED_TO_FLOAT((gpatch->width<<FRACBITS) - sx);

	if (sy + h > gpatch->height<<FRACBITS)
		fheight = FIXED_TO_FLOAT((gpatch->height<<FRACBITS) - sy);

	if (pscale != FRACUNIT || vscale != FRACUNIT || (splitscreen && option & V_PERPLAYER))
	{
		fwidth *= fscalew * dup;
		fheight *= fscaleh * dup;
	}
	else
	{
		fwidth *= dup;
		fheight *= dup;
	}

	// positions of the cx, cy, are between 0 and vid.width/vid.height now, we need them to be between -1 and 1
	cx = -1 + (cx / (vid.width/2));
	cy = 1 - (cy / (vid.height/2));

	// fwidth and fheight are similar
	fwidth /= vid.width / 2;
	fheight /= vid.height / 2;

	// set the polygon vertices to the right positions
	v[0].x = v[3].x = cx;
	v[2].x = v[1].x = cx + fwidth;

	v[0].y = v[1].y = cy;
	v[2].y = v[3].y = cy - fheight;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = (FIXED_TO_FLOAT(sx)/(float)(gpatch->width))*hwrPatch->max_s;
	if (sx + w > gpatch->width<<FRACBITS)
		v[2].s = v[1].s = hwrPatch->max_s;
	else
		v[2].s = v[1].s = (FIXED_TO_FLOAT(sx+w)/(float)(gpatch->width))*hwrPatch->max_s;

	v[0].t = v[1].t = (FIXED_TO_FLOAT(sy)/(float)(gpatch->height))*hwrPatch->max_t;
	if (sy + h > gpatch->height<<FRACBITS)
		v[2].t = v[3].t = hwrPatch->max_t;
	else
		v[2].t = v[3].t = (FIXED_TO_FLOAT(sy+h)/(float)(gpatch->height))*hwrPatch->max_t;

	// Auto-crop at splitscreen borders!
	if (splitscreen && (option & V_PERPLAYER))
	{
#define flerp(a,b,amount) (((a) * (1.0f - (amount))) + ((b) * (amount))) // Float lerp

#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			#error Auto-cropping doesnt take quadscreen into account! Fix it!
			// Hint: For player 1/2, copy player 1's code below. For player 3/4, copy player 2's code below
			// For player 1/3 and 2/4, mangle the below code to apply horizontally instead of vertically
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer]) // Player 1's screen, crop at the bottom
			{
				if ((cy - fheight) < 0) // If the bottom is below the border
				{
					if (cy <= 0) // If the whole patch is beyond the border...
						return; // ...crop away the entire patch, don't draw anything

					if (fheight <= 0) // Don't divide by zero
						return;

					v[2].y = v[3].y = 0; // Clamp the polygon edge vertex position
					// Now for the UV-map... Uh-oh, math time!

					// On second thought, a basic linear interpolation suffices
					//float full_height = fheight;
					//float cropped_height = fheight - cy;
					//float remaining_height = cy;
					//float cropped_percentage = (fheight - cy) / fheight;
					//float remaining_percentage = cy / fheight;
					//v[2].t = v[3].t = lerp(v[2].t, v[0].t, cropped_percentage);
					// By swapping v[2] and v[0], we can use remaining_percentage for less operations
					//v[2].t = v[3].t = lerp(v[0].t, v[2].t, remaining_percentage);

					v[2].t = v[3].t = flerp(v[0].t, v[2].t, cy/fheight);
				}
			}
			else //if (stplyr == &players[secondarydisplayplayer]) // Player 2's screen, crop at the top
			{
				if (cy > 0) // If the top is above the border
				{
					if ((cy - fheight) >= 0) // If the whole patch is beyond the border...
						return; // ...crop away the entire patch, don't draw anything

					if (fheight <= 0) // Don't divide by zero
						return;

					v[0].y = v[1].y = 0; // Clamp the polygon edge vertex position
					// Now for the UV-map... Uh-oh, math time!

					// On second thought, a basic linear interpolation suffices
					//float full_height = fheight;
					//float cropped_height = cy;
					//float remaining_height = fheight - cy;
					//float cropped_percentage = cy / fheight;
					//float remaining_percentage = (fheight - cy) / fheight;
					//v[0].t = v[1].t = lerp(v[0].t, v[2].t, cropped_percentage);

					v[0].t = v[1].t = flerp(v[0].t, v[2].t, cy/fheight);
				}
			}
		}
#undef flerp
	}

	// clip it since it is used for bunny scroll in doom I
	flags = HWR_GetBlendModeFlag(blendmode+1)|PF_NoDepthTest;

	if (alphalevel)
	{
		FSurfaceInfo Surf;
		Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;

		if (alphalevel == 10) Surf.PolyColor.s.alpha = softwaretranstogl_lo[st_translucency]; // V_HUDTRANSHALF
		else if (alphalevel == 11) Surf.PolyColor.s.alpha = softwaretranstogl[st_translucency]; // V_HUDTRANS
		else if (alphalevel == 12) Surf.PolyColor.s.alpha = softwaretranstogl_hi[st_translucency]; // V_HUDTRANSDOUBLE
		else Surf.PolyColor.s.alpha = softwaretranstogl[10-alphalevel];

		flags |= PF_Modulated;
		HWD.pfnDrawPolygon(&Surf, v, 4, flags);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

// ==========================================================================
//                                                            V_VIDEO.C STUFF
// ==========================================================================

// --------------------------------------------------------------------------
// Fills a box of pixels using a flat texture as a pattern
// --------------------------------------------------------------------------
void HWR_DrawFlatFill (INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum)
{
	FOutVector v[4];
	const size_t len = W_LumpLength(flatlumpnum);
	UINT16 flatflag = R_GetFlatSize(len) - 1;
	double dflatsize = (double)(flatflag + 1);

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = (x - 160.0f)/160.0f;
	v[2].x = v[1].x = ((x+w) - 160.0f)/160.0f;
	v[0].y = v[1].y = -(y - 100.0f)/100.0f;
	v[2].y = v[3].y = -((y+h) - 100.0f)/100.0f;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = (float)((x & flatflag)/dflatsize);
	v[2].s = v[1].s = (float)(v[0].s + w/dflatsize);
	v[0].t = v[1].t = (float)((y & flatflag)/dflatsize);
	v[2].t = v[3].t = (float)(v[0].t + h/dflatsize);

	HWR_GetRawFlat(flatlumpnum);

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
void HWR_FadeScreenMenuBack(UINT16 color, UINT8 strength)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;
	FBITFIELD poly_flags = PF_NoTexture|PF_Modulated|PF_NoDepthTest;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y = -1.0f;
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	if (color & 0xFF00) // Special fade options
	{
		UINT16 option = color & 0x0F00;
		if (option == 0x0A00 || option == 0x0B00) // Tinted fades
		{
			INT32 r, g, b;
			int fade = strength * 8;

			r = FADEREDFACTOR*fade/10;
			g = FADEGREENFACTOR*fade/10;
			b = FADEBLUEFACTOR*fade/10;

			Surf.PolyColor.s.red = min(r, 255);
			Surf.PolyColor.s.green = min(g, 255);
			Surf.PolyColor.s.blue = min(b, 255);
			Surf.PolyColor.s.alpha = 255;

			if (option == 0x0A00) // Tinted subtractive fade
				poly_flags |= PF_ReverseSubtract;
			else if (option == 0x0B00) // Tinted additive fade
				poly_flags |= PF_Additive;
		}
		else // COLORMAP fade
		{
			if (HWR_ShouldUsePaletteRendering())
			{
				const hwdscreentexture_t scr_tex = HWD_SCREENTEXTURE_GENERIC2;

				Surf.LightTableId = HWR_GetLightTableID(NULL);
				Surf.LightInfo.light_level = strength;
				HWD.pfnMakeScreenTexture(scr_tex);
				HWD.pfnSetShader(HWR_GetShaderFromTarget(SHADER_UI_COLORMAP_FADE));
				HWD.pfnDrawScreenTexture(scr_tex, &Surf, PF_ColorMapped|PF_NoDepthTest);
				HWD.pfnUnSetShader();

				return;
			}
			else
			{
				Surf.PolyColor.rgba = UINT2RGBA(0x01010160);
				Surf.PolyColor.s.alpha = (strength*8);
				poly_flags |= PF_Translucent;
			}
		}
	}
	else // Do TRANSMAP** fade.
	{
		RGBA_t *palette = HWR_GetTexturePalette();
		Surf.PolyColor.rgba = palette[color&0xFF].rgba;
		Surf.PolyColor.s.alpha = softwaretranstogl[strength];
		poly_flags |= PF_Translucent;
	}
	HWD.pfnDrawPolygon(&Surf, v, 4, poly_flags);
}

// -----------------+
// HWR_DrawFadeFill : draw flat coloured rectangle, with transparency
// -----------------+
void HWR_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT16 actualcolor, UINT8 strength)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh;

	UINT8 perplayershuffle = 0;

//  3--2
//  | /|
//  |/ |
//  0--1

	if (splitscreen && (color & V_PERPLAYER))
	{
		fixed_t adjusty = ((color & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
		h >>= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			fixed_t adjustx = ((color & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
			w >>= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				color &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				color &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				color &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else //if (stplyr == &players[fourthdisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				color &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				color &= ~V_SNAPTOBOTTOM;
			}
			else //if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				color &= ~V_SNAPTOTOP;
			}
		}
	}

	fx = (float)x;
	fy = (float)y;
	fw = (float)w;
	fh = (float)h;

	if (!(color & V_NOSCALESTART))
	{
		fx *= vid.dup;
		fy *= vid.dup;
		fw *= vid.dup;
		fh *= vid.dup;

		if (fabsf((float)vid.width - (float)BASEVIDWIDTH * vid.dup) > 1.0E-36f)
		{
			if (color & V_SNAPTORIGHT)
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup));
			else if (!(color & V_SNAPTOLEFT))
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup)) / 2;
			if (perplayershuffle & 4)
				fx -= ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup)) / 4;
			else if (perplayershuffle & 8)
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup)) / 4;
		}
		if (fabsf((float)vid.height - (float)BASEVIDHEIGHT * vid.dup) > 1.0E-36f)
		{
			// same thing here
			if (color & V_SNAPTOBOTTOM)
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup));
			else if (!(color & V_SNAPTOTOP))
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup)) / 2;
			if (perplayershuffle & 1)
				fy -= ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup)) / 4;
			else if (perplayershuffle & 2)
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup)) / 4;
		}
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
		fw = (float)vid.width - fx;
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[2].y = v[3].y = fy - fh;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	if (actualcolor & 0xFF00) // Do COLORMAP fade.
	{
		Surf.PolyColor.rgba = UINT2RGBA(0x01010160);
		Surf.PolyColor.s.alpha = (strength*8);
	}
	else // Do TRANSMAP** fade.
	{
		RGBA_t *palette = HWR_GetTexturePalette();
		Surf.PolyColor.rgba = palette[actualcolor&0xFF].rgba;
		Surf.PolyColor.s.alpha = softwaretranstogl[strength];
	}
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

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	Surf.PolyColor.rgba = UINT2RGBA(color);
	Surf.PolyColor.s.alpha = 0x80;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// Very similar to HWR_DrawConsoleBack, except we draw from the middle(-ish) of the screen to the bottom.
void HWR_DrawTutorialBack(UINT32 color, INT32 boxheight)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;
	INT32 height;
	if (boxheight < 0)
		height = -boxheight;
	else
		height = (boxheight * 4) + (boxheight/2)*5; // 4 lines of space plus gaps between and some leeway

	// setup some neat-o translucency effect

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y =  -1.0f;
	v[2].y = v[3].y =  -1.0f+((height<<1)/(float)vid.height);
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	Surf.PolyColor.rgba = UINT2RGBA(color);
	Surf.PolyColor.s.alpha = (color == 0 ? 0xC0 : 0x80); // make black darker, like software

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// ==========================================================================
//                                                     AM_MAP.C DRAWING STUFF
// ==========================================================================

// -----------------+
// HWR_drawAMline   : draw a line of the automap (the clipping is already done in automap code)
// Arg              : color is a RGB 888 value
// -----------------+
void HWR_drawAMline(const fline_t *fl, INT32 color)
{
	F2DCoord v1, v2;
	RGBA_t color_rgba;
	RGBA_t *palette = HWR_GetTexturePalette();

	color_rgba = palette[color&0xFF];

	v1.x = ((float)fl->a.x-(vid.width/2.0f))*(2.0f/vid.width);
	v1.y = ((float)fl->a.y-(vid.height/2.0f))*(2.0f/vid.height);

	v2.x = ((float)fl->b.x-(vid.width/2.0f))*(2.0f/vid.width);
	v2.y = ((float)fl->b.y-(vid.height/2.0f))*(2.0f/vid.height);

	HWD.pfnDraw2DLine(&v1, &v2, color_rgba);
}

// -------------------+
// HWR_DrawConsoleFill     : draw flat coloured transparent rectangle because that's cool, and hw sucks less than sw for that.
// -------------------+
void HWR_DrawConsoleFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT32 actualcolor)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh;

	UINT8 perplayershuffle = 0;

//  3--2
//  | /|
//  |/ |
//  0--1

	if (splitscreen && (color & V_PERPLAYER))
	{
		fixed_t adjusty = ((color & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
		h >>= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			fixed_t adjustx = ((color & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
			w >>= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				color &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				color &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				color &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else //if (stplyr == &players[fourthdisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				color &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				color &= ~V_SNAPTOBOTTOM;
			}
			else //if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				color &= ~V_SNAPTOTOP;
			}
		}
	}

	fx = (float)x;
	fy = (float)y;
	fw = (float)w;
	fh = (float)h;

	if (!(color & V_NOSCALESTART))
	{
		float dup = (float)vid.dup;

		fx *= dup;
		fy *= dup;
		fw *= dup;
		fh *= dup;

		if (fabsf((float)vid.width - (float)BASEVIDWIDTH * dup) > 1.0E-36f)
		{
			if (color & V_SNAPTORIGHT)
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * dup));
			else if (!(color & V_SNAPTOLEFT))
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * dup)) / 2;
			if (perplayershuffle & 4)
				fx -= ((float)vid.width - ((float)BASEVIDWIDTH * dup)) / 4;
			else if (perplayershuffle & 8)
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * dup)) / 4;
		}
		if (fabsf((float)vid.height - (float)BASEVIDHEIGHT * dup) > 1.0E-36f)
		{
			// same thing here
			if (color & V_SNAPTOBOTTOM)
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup));
			else if (!(color & V_SNAPTOTOP))
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup)) / 2;
			if (perplayershuffle & 1)
				fy -= ((float)vid.height - ((float)BASEVIDHEIGHT * dup)) / 4;
			else if (perplayershuffle & 2)
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * dup)) / 4;
		}
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
		fw = (float)vid.width - fx;
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[2].y = v[3].y = fy - fh;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	Surf.PolyColor.rgba = UINT2RGBA(actualcolor);
	Surf.PolyColor.s.alpha = 0x80;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// -----------------+
// HWR_DrawFill     : draw flat coloured rectangle, with no texture
// -----------------+
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh;
	RGBA_t *palette = HWR_GetTexturePalette();
	UINT8 alphalevel = ((color & V_ALPHAMASK) >> V_ALPHASHIFT);

	UINT8 perplayershuffle = 0;

//  3--2
//  | /|
//  |/ |
//  0--1

	if (splitscreen && (color & V_PERPLAYER))
	{
		fixed_t adjusty = ((color & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
		h >>= 1;
		y >>= 1;
#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			fixed_t adjustx = ((color & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)/2.0f;
			w >>= 1;
			x >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				color &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				color &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				y += adjusty;
				color &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else //if (stplyr == &players[fourthdisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(color & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				x += adjustx;
				y += adjusty;
				color &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				color &= ~V_SNAPTOBOTTOM;
			}
			else //if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(color & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				y += adjusty;
				color &= ~V_SNAPTOTOP;
			}
		}
	}

	fx = (float)x;
	fy = (float)y;
	fw = (float)w;
	fh = (float)h;

	if (!(color & V_NOSCALESTART))
	{
		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{
			RGBA_t rgbaColour = palette[color&0xFF];
			FRGBAFloat clearColour;
			clearColour.red = (float)rgbaColour.s.red / 255;
			clearColour.green = (float)rgbaColour.s.green / 255;
			clearColour.blue = (float)rgbaColour.s.blue / 255;
			clearColour.alpha = 1;
			HWD.pfnClearBuffer(true, false, &clearColour);
			return;
		}

		fx *= vid.dup;
		fy *= vid.dup;
		fw *= vid.dup;
		fh *= vid.dup;

		if (fabsf((float)vid.width - (float)BASEVIDWIDTH * vid.dup) > 1.0E-36f)
		{
			if (color & V_SNAPTORIGHT)
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup));
			else if (!(color & V_SNAPTOLEFT))
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup)) / 2;
			if (perplayershuffle & 4)
				fx -= ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup)) / 4;
			else if (perplayershuffle & 8)
				fx += ((float)vid.width - ((float)BASEVIDWIDTH * vid.dup)) / 4;
		}
		if (fabsf((float)vid.height - (float)BASEVIDHEIGHT * vid.dup) > 1.0E-36f)
		{
			// same thing here
			if (color & V_SNAPTOBOTTOM)
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup));
			else if (!(color & V_SNAPTOTOP))
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup)) / 2;
			if (perplayershuffle & 1)
				fy -= ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup)) / 4;
			else if (perplayershuffle & 2)
				fy += ((float)vid.height - ((float)BASEVIDHEIGHT * vid.dup)) / 4;
		}
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
		fw = (float)vid.width - fx;
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[2].y = v[3].y = fy - fh;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	Surf.PolyColor = palette[color&0xFF];

	if (alphalevel)
	{
		if (alphalevel == 10) Surf.PolyColor.s.alpha = softwaretranstogl_lo[st_translucency]; // V_HUDTRANSHALF
		else if (alphalevel == 11) Surf.PolyColor.s.alpha = softwaretranstogl[st_translucency]; // V_HUDTRANS
		else if (alphalevel == 12) Surf.PolyColor.s.alpha = softwaretranstogl_hi[st_translucency]; // V_HUDTRANSDOUBLE
		else Surf.PolyColor.s.alpha = softwaretranstogl[10-alphalevel];
	}

	HWD.pfnDrawPolygon(&Surf, v, 4,
		PF_Modulated|PF_NoTexture|PF_NoDepthTest|PF_Translucent);
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

	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
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
	int tex = HWR_ShouldUsePaletteRendering() ? HWD_SCREENTEXTURE_GENERIC3 : HWD_SCREENTEXTURE_GENERIC2;

	if (!buf)
		return NULL;
	// returns 24bit 888 RGB
	HWD.pfnReadScreenTexture(tex, (void *)buf);
	return buf;
}

boolean HWR_Screenshot(const char *pathname)
{
	boolean ret;
	UINT8 *buf = malloc(vid.width * vid.height * 3 * sizeof (*buf));
	int tex = HWR_ShouldUsePaletteRendering() ? HWD_SCREENTEXTURE_GENERIC3 : HWD_SCREENTEXTURE_GENERIC2;

	if (!buf)
	{
		CONS_Debug(DBG_RENDER, "HWR_Screenshot: Failed to allocate memory\n");
		return false;
	}

	// returns 24bit 888 RGB
	HWD.pfnReadScreenTexture(tex, (void *)buf);

#ifdef USE_PNG
	ret = M_SavePNG(pathname, buf, vid.width, vid.height, NULL);
#else
	ret = saveTGA(pathname, buf, vid.width, vid.height);
#endif
	free(buf);
	return ret;
}

#endif //HWRENDER
