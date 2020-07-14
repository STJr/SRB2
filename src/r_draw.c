// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw.c
/// \brief span / column drawer functions, for 8bpp and 16bpp
///        All drawing to the view buffer is accomplished in this file.
///        The other refresh files only know about ccordinates,
///        not the architecture of the frame buffer.
///        The frame buffer is a linear one, and we need only the base address.

#include "doomdef.h"
#include "doomstat.h"
#include "r_local.h"
#include "st_stuff.h" // need ST_HEIGHT
#include "i_video.h"
#include "v_video.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"
#include "console.h" // Until buffering gets finished

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// ==========================================================================
//                     COMMON DATA FOR 8bpp AND 16bpp
// ==========================================================================

/**	\brief view info
*/
INT32 viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;

/**	\brief pointer to the start of each line of the screen,
*/
UINT8 *ylookup[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view1 (splitscreen)
*/
UINT8 *ylookup1[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view2 (splitscreen)
*/
UINT8 *ylookup2[MAXVIDHEIGHT*4];

/**	\brief  x byte offset for columns inside the viewwindow,
	so the first column starts at (SCRWIDTH - VIEWWIDTH)/2
*/
INT32 columnofs[MAXVIDWIDTH*4];

UINT8 *topleft;

// =========================================================================
//                      COLUMN DRAWING CODE STUFF
// =========================================================================

lighttable_t *dc_colormap;
INT32 dc_x = 0, dc_yl = 0, dc_yh = 0;

fixed_t dc_iscale, dc_texturemid;
UINT8 dc_hires; // under MSVC boolean is a byte, while on other systems, it a bit,
               // soo lets make it a byte on all system for the ASM code
UINT8 *dc_source;

// -----------------------
// translucency stuff here
// -----------------------
#define NUMTRANSTABLES 9 // how many translucency tables are used

UINT8 *transtables; // translucency tables

/**	\brief R_DrawTransColumn uses this
*/
UINT8 *dc_transmap; // one of the translucency tables

// ----------------------
// translation stuff here
// ----------------------


/**	\brief R_DrawTranslatedColumn uses this
*/
UINT8 *dc_translation;

struct r_lightlist_s *dc_lightlist = NULL;
INT32 dc_numlights = 0, dc_maxlights, dc_texheight;

// =========================================================================
//                      SPAN DRAWING CODE STUFF
// =========================================================================

INT32 ds_y, ds_x1, ds_x2;
lighttable_t *ds_colormap;
fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;
UINT16 ds_flatwidth, ds_flatheight;
boolean ds_powersoftwo;

UINT8 *ds_source; // start of a 64*64 tile image
UINT8 *ds_transmap; // one of the translucency tables

pslope_t *ds_slope; // Current slope being used
floatv3_t ds_su[MAXVIDHEIGHT], ds_sv[MAXVIDHEIGHT], ds_sz[MAXVIDHEIGHT]; // Vectors for... stuff?
floatv3_t *ds_sup, *ds_svp, *ds_szp;
float focallengthf, zeroheight;

/**	\brief Variable flat sizes
*/

UINT32 nflatxshift, nflatyshift, nflatshiftup, nflatmask;

// ==========================================================================
//                        OLD DOOM FUZZY EFFECT
// ==========================================================================

// =========================================================================
//                   TRANSLATION COLORMAP CODE
// =========================================================================

#define DEFAULT_TT_CACHE_INDEX MAXSKINS
#define BOSS_TT_CACHE_INDEX (MAXSKINS + 1)
#define METALSONIC_TT_CACHE_INDEX (MAXSKINS + 2)
#define ALLWHITE_TT_CACHE_INDEX (MAXSKINS + 3)
#define RAINBOW_TT_CACHE_INDEX (MAXSKINS + 4)
#define BLINK_TT_CACHE_INDEX (MAXSKINS + 5)
#define DASHMODE_TT_CACHE_INDEX (MAXSKINS + 6)
#define DEFAULT_STARTTRANSCOLOR 96
#define NUM_PALETTE_ENTRIES 256

static UINT8** translationtablecache[MAXSKINS + 7] = {NULL};
UINT8 skincolor_modified[MAXSKINCOLORS];

CV_PossibleValue_t Color_cons_t[MAXSKINCOLORS+1];

/**	\brief The R_InitTranslationTables

  load in color translation tables
*/
void R_InitTranslationTables(void)
{
	// Load here the transparency lookup tables 'TINTTAB'
	// NOTE: the TINTTAB resource MUST BE aligned on 64k for the asm
	// optimised code (in other words, transtables pointer low word is 0)
	transtables = Z_MallocAlign(NUMTRANSTABLES*0x10000, PU_STATIC,
		NULL, 16);

	W_ReadLump(W_GetNumForName("TRANS10"), transtables);
	W_ReadLump(W_GetNumForName("TRANS20"), transtables+0x10000);
	W_ReadLump(W_GetNumForName("TRANS30"), transtables+0x20000);
	W_ReadLump(W_GetNumForName("TRANS40"), transtables+0x30000);
	W_ReadLump(W_GetNumForName("TRANS50"), transtables+0x40000);
	W_ReadLump(W_GetNumForName("TRANS60"), transtables+0x50000);
	W_ReadLump(W_GetNumForName("TRANS70"), transtables+0x60000);
	W_ReadLump(W_GetNumForName("TRANS80"), transtables+0x70000);
	W_ReadLump(W_GetNumForName("TRANS90"), transtables+0x80000);
}


/**	\brief	Generates a translation colormap.

	\param	dest_colormap	colormap to populate
	\param	skinnum		number of skin, TC_DEFAULT or TC_BOSS
	\param	color		translation color

	\return	void
*/

// Define for getting accurate color brightness readings according to how the human eye sees them.
// https://en.wikipedia.org/wiki/Relative_luminance
// 0.2126 to red
// 0.7152 to green
// 0.0722 to blue
// (See this same define in hw_md2.c!)
#define SETBRIGHTNESS(brightness,r,g,b) \
	brightness = (UINT8)(((1063*((UINT16)r)/5000) + (3576*((UINT16)g)/5000) + (361*((UINT16)b)/5000)) / 3)

/** \brief	Generates the rainbow colourmaps that are used when a player has the invincibility power... stolen from kart, with permission

	\param	dest_colormap	colormap to populate
	\param	skincolor		translation color
*/
static void R_RainbowColormap(UINT8 *dest_colormap, UINT16 skincolor)
{
	INT32 i;
	RGBA_t color;
	UINT8 brightness;
	INT32 j;
	UINT8 colorbrightnesses[16];
	UINT16 brightdif;
	INT32 temp;

	// first generate the brightness of all the colours of that skincolour
	for (i = 0; i < 16; i++)
	{
		color = V_GetColor(skincolors[skincolor].ramp[i]);
		SETBRIGHTNESS(colorbrightnesses[i], color.s.red, color.s.green, color.s.blue);
	}

	// next, for every colour in the palette, choose the transcolor that has the closest brightness
	for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		if (i == 0 || i == 31) // pure black and pure white don't change
		{
			dest_colormap[i] = (UINT8)i;
			continue;
		}
		color = V_GetColor(i);
		SETBRIGHTNESS(brightness, color.s.red, color.s.green, color.s.blue);
		brightdif = 256;
		for (j = 0; j < 16; j++)
		{
			temp = abs((INT16)brightness - (INT16)colorbrightnesses[j]);
			if (temp < brightdif)
			{
				brightdif = (UINT16)temp;
				dest_colormap[i] = skincolors[skincolor].ramp[j];
			}
		}
	}
}

#undef SETBRIGHTNESS

static void R_GenerateTranslationColormap(UINT8 *dest_colormap, INT32 skinnum, UINT16 color)
{
	INT32 i, starttranscolor, skinramplength;

	// Handle a couple of simple special cases
	if (skinnum < TC_DEFAULT)
	{
		switch (skinnum)
		{
			case TC_ALLWHITE:
				memset(dest_colormap, 0, NUM_PALETTE_ENTRIES * sizeof(UINT8));
				return;
			case TC_RAINBOW:
				if (color >= numskincolors)
					I_Error("Invalid skin color #%hu.", (UINT16)color);
				if (color != SKINCOLOR_NONE)
				{
					R_RainbowColormap(dest_colormap, color);
					return;
				}
				break;
			case TC_BLINK:
				if (color >= numskincolors)
					I_Error("Invalid skin color #%hu.", (UINT16)color);
				if (color != SKINCOLOR_NONE)
				{
					memset(dest_colormap, skincolors[color].ramp[3], NUM_PALETTE_ENTRIES * sizeof(UINT8));
					return;
				}
				break;
			default:
				break;
		}

		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
			dest_colormap[i] = (UINT8)i;

		// White!
		if (skinnum == TC_BOSS)
		{
			for (i = 0; i < 16; i++)
				dest_colormap[31-i] = i;
		}
		else if (skinnum == TC_METALSONIC)
		{
			for (i = 0; i < 6; i++)
			{
				dest_colormap[skincolors[SKINCOLOR_BLUE].ramp[12-i]] = skincolors[SKINCOLOR_BLUE].ramp[i];
			}
			dest_colormap[159] = dest_colormap[253] = dest_colormap[254] = 0;
			for (i = 0; i < 16; i++)
				dest_colormap[96+i] = dest_colormap[skincolors[SKINCOLOR_COBALT].ramp[i]];
		}
		else if (skinnum == TC_DASHMODE) // This is a long one, because MotorRoach basically hand-picked the indices
		{
			// greens -> ketchups
			dest_colormap[96] = dest_colormap[97] = 48;
			dest_colormap[98] = 49;
			dest_colormap[99] = 51;
			dest_colormap[100] = 52;
			dest_colormap[101] = dest_colormap[102] = 54;
			dest_colormap[103] = 34;
			dest_colormap[104] = 37;
			dest_colormap[105] = 39;
			dest_colormap[106] = 41;
			for (i = 0; i < 5; i++)
				dest_colormap[107 + i] = 43 + i;

			// reds -> steel blues
			dest_colormap[32] = 146;
			dest_colormap[33] = 147;
			dest_colormap[34] = dest_colormap[35] = 170;
			dest_colormap[36] = 171;
			dest_colormap[37] = dest_colormap[38] = 172;
			dest_colormap[39] = dest_colormap[40] = dest_colormap[41] = 173;
			dest_colormap[42] = dest_colormap[43] = dest_colormap[44] = 174;
			dest_colormap[45] = dest_colormap[46] = dest_colormap[47] = 175;
			dest_colormap[71] = 139;

			// steel blues -> oranges
			dest_colormap[170] = 52;
			dest_colormap[171] = 54;
			dest_colormap[172] = 56;
			dest_colormap[173] = 42;
			dest_colormap[174] = 45;
			dest_colormap[175] = 47;
		}
		return;
	}
	else if (color == SKINCOLOR_NONE)
	{
		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
			dest_colormap[i] = (UINT8)i;
		return;
	}

	if (color >= numskincolors)
		I_Error("Invalid skin color #%hu.", (UINT16)color);

	starttranscolor = (skinnum != TC_DEFAULT) ? skins[skinnum].starttranscolor : DEFAULT_STARTTRANSCOLOR;

	if (starttranscolor >= NUM_PALETTE_ENTRIES)
		I_Error("Invalid startcolor #%d.", starttranscolor);

	// Fill in the entries of the palette that are fixed
	for (i = 0; i < starttranscolor; i++)
		dest_colormap[i] = (UINT8)i;

	i = starttranscolor + 16;
	if (i < NUM_PALETTE_ENTRIES)
	{
		for (i = (UINT8)i; i < NUM_PALETTE_ENTRIES; i++)
			dest_colormap[i] = (UINT8)i;
		skinramplength = 16;
	}
	else
		skinramplength = i - NUM_PALETTE_ENTRIES; // shouldn't this be NUM_PALETTE_ENTRIES - starttranscolor?

	// Build the translated ramp
	for (i = 0; i < skinramplength; i++)
		dest_colormap[starttranscolor + i] = (UINT8)skincolors[color].ramp[i];
}


/**	\brief	Retrieves a translation colormap from the cache.

	\param	skinnum	number of skin, TC_DEFAULT or TC_BOSS
	\param	color	translation color
	\param	flags	set GTC_CACHE to use the cache

	\return	Colormap. If not cached, caller should Z_Free.
*/
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags)
{
	UINT8* ret;
	INT32 skintableindex;
	INT32 i;

	// Adjust if we want the default colormap
	switch (skinnum)
	{
		case TC_DEFAULT:    skintableindex = DEFAULT_TT_CACHE_INDEX; break;
		case TC_BOSS:       skintableindex = BOSS_TT_CACHE_INDEX; break;
		case TC_METALSONIC: skintableindex = METALSONIC_TT_CACHE_INDEX; break;
		case TC_ALLWHITE:   skintableindex = ALLWHITE_TT_CACHE_INDEX; break;
		case TC_RAINBOW:    skintableindex = RAINBOW_TT_CACHE_INDEX; break;
		case TC_BLINK:      skintableindex = BLINK_TT_CACHE_INDEX; break;
		case TC_DASHMODE:   skintableindex = DASHMODE_TT_CACHE_INDEX; break;
		     default:       skintableindex = skinnum; break;
	}

	if (flags & GTC_CACHE)
	{

		// Allocate table for skin if necessary
		if (!translationtablecache[skintableindex])
			translationtablecache[skintableindex] = Z_Calloc(MAXSKINCOLORS * sizeof(UINT8**), PU_STATIC, NULL);

		// Get colormap
		ret = translationtablecache[skintableindex][color];

		// Rebuild the cache if necessary
		if (skincolor_modified[color])
		{
			for (i = 0; i < (INT32)(sizeof(translationtablecache) / sizeof(translationtablecache[0])); i++)
				if (translationtablecache[i] && translationtablecache[i][color])
					R_GenerateTranslationColormap(translationtablecache[i][color], i>=MAXSKINS ? MAXSKINS-i-1 : i, color);
			skincolor_modified[color] = false;
		}
	}
	else ret = NULL;

	// Generate the colormap if necessary
	if (!ret)
	{
		ret = Z_MallocAlign(NUM_PALETTE_ENTRIES, (flags & GTC_CACHE) ? PU_LEVEL : PU_STATIC, NULL, 8);
		R_GenerateTranslationColormap(ret, skinnum, color);

		// Cache the colormap if desired
		if (flags & GTC_CACHE)
			translationtablecache[skintableindex][color] = ret;
	}

	return ret;
}

/**	\brief	Flushes cache of translation colormaps.

	Flushes cache of translation colormaps, but doesn't actually free the
	colormaps themselves. These are freed when PU_LEVEL blocks are purged,
	at or before which point, this function should be called.

	\return	void
*/
void R_FlushTranslationColormapCache(void)
{
	INT32 i;

	for (i = 0; i < (INT32)(sizeof(translationtablecache) / sizeof(translationtablecache[0])); i++)
		if (translationtablecache[i])
			memset(translationtablecache[i], 0, MAXSKINCOLORS * sizeof(UINT8**));
}

UINT16 R_GetColorByName(const char *name)
{
	UINT16 color = (UINT16)atoi(name);
	if (color > 0 && color < numskincolors)
		return color;
	for (color = 1; color < numskincolors; color++)
		if (!stricmp(skincolors[color].name, name))
			return color;
	return SKINCOLOR_NONE;
}

UINT16 R_GetSuperColorByName(const char *name)
{
	UINT16 i, color = SKINCOLOR_NONE;
	char *realname = Z_Malloc(MAXCOLORNAME+1, PU_STATIC, NULL);
	snprintf(realname, MAXCOLORNAME+1, "Super %s 1", name);
	for (i = 1; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name, realname)) {
			color = i;
			break;
		}
	Z_Free(realname);
	return color;
}

// ==========================================================================
//               COMMON DRAWER FOR 8 AND 16 BIT COLOR MODES
// ==========================================================================

// in a perfect world, all routines would be compatible for either mode,
// and optimised enough
//
// in reality, the few routines that can work for either mode, are
// put here

/**	\brief	The R_InitViewBuffer function

	Creates lookup tables for getting the framebuffer address
	of a pixel to draw.

	\param	width	witdh of buffer
	\param	height	hieght of buffer

	\return	void


*/

void R_InitViewBuffer(INT32 width, INT32 height)
{
	INT32 i, bytesperpixel = vid.bpp;

	if (width > MAXVIDWIDTH)
		width = MAXVIDWIDTH;
	if (height > MAXVIDHEIGHT)
		height = MAXVIDHEIGHT;
	if (bytesperpixel < 1 || bytesperpixel > 4)
		I_Error("R_InitViewBuffer: wrong bytesperpixel value %d\n", bytesperpixel);

	// Handle resize, e.g. smaller view windows with border and/or status bar.
	viewwindowx = (vid.width - width) >> 1;

	// Column offset for those columns of the view window, but relative to the entire screen
	for (i = 0; i < width; i++)
		columnofs[i] = (viewwindowx + i) * bytesperpixel;

	// Same with base row offset.
	if (width == vid.width)
		viewwindowy = 0;
	else
		viewwindowy = (vid.height - height) >> 1;

	// Precalculate all row offsets.
	for (i = 0; i < height; i++)
	{
		ylookup[i] = ylookup1[i] = screens[0] + (i+viewwindowy)*vid.width*bytesperpixel;
		ylookup2[i] = screens[0] + (i+(vid.height>>1))*vid.width*bytesperpixel; // for splitscreen
	}
}

/**	\brief viewborder patches lump numbers
*/
lumpnum_t viewborderlump[8];

/**	\brief Store the lumpnumber of the viewborder patches
*/

void R_InitViewBorder(void)
{
	viewborderlump[BRDR_T] = W_GetNumForName("brdr_t");
	viewborderlump[BRDR_B] = W_GetNumForName("brdr_b");
	viewborderlump[BRDR_L] = W_GetNumForName("brdr_l");
	viewborderlump[BRDR_R] = W_GetNumForName("brdr_r");
	viewborderlump[BRDR_TL] = W_GetNumForName("brdr_tl");
	viewborderlump[BRDR_BL] = W_GetNumForName("brdr_bl");
	viewborderlump[BRDR_TR] = W_GetNumForName("brdr_tr");
	viewborderlump[BRDR_BR] = W_GetNumForName("brdr_br");
}

#if 0
/**	\brief R_FillBackScreen

	Fills the back screen with a pattern for variable screen sizes
	Also draws a beveled edge.
*/
void R_FillBackScreen(void)
{
	UINT8 *src, *dest;
	patch_t *patch;
	INT32 x, y, step, boff;

	// quickfix, don't cache lumps in both modes
	if (rendermode != render_soft)
		return;

	// draw pattern around the status bar too (when hires),
	// so return only when in full-screen without status bar.
	if (scaledviewwidth == vid.width && viewheight == vid.height)
		return;

	src = scr_borderpatch;
	dest = screens[1];

	for (y = 0; y < vid.height; y++)
	{
		for (x = 0; x < vid.width/128; x++)
		{
			M_Memcpy (dest, src+((y&127)<<7), 128);
			dest += 128;
		}

		if (vid.width&127)
		{
			M_Memcpy(dest, src+((y&127)<<7), vid.width&127);
			dest += (vid.width&127);
		}
	}

	// don't draw the borders when viewwidth is full vid.width.
	if (scaledviewwidth == vid.width)
		return;

	step = 8;
	boff = 8;

	patch = W_CacheLumpNum(viewborderlump[BRDR_T], PU_CACHE);
	for (x = 0; x < scaledviewwidth; x += step)
		V_DrawPatch(viewwindowx + x, viewwindowy - boff, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_B], PU_CACHE);
	for (x = 0; x < scaledviewwidth; x += step)
		V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_L], PU_CACHE);
	for (y = 0; y < viewheight; y += step)
		V_DrawPatch(viewwindowx - boff, viewwindowy + y, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_R],PU_CACHE);
	for (y = 0; y < viewheight; y += step)
		V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, 1,
			patch);

	// Draw beveled corners.
	V_DrawPatch(viewwindowx - boff, viewwindowy - boff, 1,
		W_CacheLumpNum(viewborderlump[BRDR_TL], PU_CACHE));
	V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - boff, 1,
		W_CacheLumpNum(viewborderlump[BRDR_TR], PU_CACHE));
	V_DrawPatch(viewwindowx - boff, viewwindowy + viewheight, 1,
		W_CacheLumpNum(viewborderlump[BRDR_BL], PU_CACHE));
	V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, 1,
		W_CacheLumpNum(viewborderlump[BRDR_BR], PU_CACHE));
}
#endif

/**	\brief	The R_VideoErase function

	Copy a screen buffer.

	\param	ofs	offest from buffer
	\param	count	bytes to erase

	\return	void


*/
void R_VideoErase(size_t ofs, INT32 count)
{
	// LFB copy.
	// This might not be a good idea if memcpy
	//  is not optimal, e.g. byte by byte on
	//  a 32bit CPU, as GNU GCC/Linux libc did
	//  at one point.
	M_Memcpy(screens[0] + ofs, screens[1] + ofs, count);
}

#if 0
/**	\brief The R_DrawViewBorder

  Draws the border around the view
	for different size windows?
*/
void R_DrawViewBorder(void)
{
	INT32 top, side, ofs;

	if (rendermode == render_none)
		return;
#ifdef HWRENDER
	if (rendermode != render_soft)
	{
		HWR_DrawViewBorder(0);
		return;
	}
	else
#endif

#ifdef DEBUG
	fprintf(stderr,"RDVB: vidwidth %d vidheight %d scaledviewwidth %d viewheight %d\n",
		vid.width, vid.height, scaledviewwidth, viewheight);
#endif

	if (scaledviewwidth == vid.width)
		return;

	top = (vid.height - viewheight)>>1;
	side = (vid.width - scaledviewwidth)>>1;

	// copy top and one line of left side
	R_VideoErase(0, top*vid.width+side);

	// copy one line of right side and bottom
	ofs = (viewheight+top)*vid.width - side;
	R_VideoErase(ofs, top*vid.width + side);

	// copy sides using wraparound
	ofs = top*vid.width + vid.width-side;
	side <<= 1;

    // simpler using our VID_Blit routine
	VID_BlitLinearScreen(screens[1] + ofs, screens[0] + ofs, side, viewheight - 1,
		vid.width, vid.width);
}
#endif

// ==========================================================================
//                   INCLUDE 8bpp DRAWING CODE HERE
// ==========================================================================

#include "r_draw8.c"
#include "r_draw8_npo2.c"

// ==========================================================================
//                   INCLUDE 16bpp DRAWING CODE HERE
// ==========================================================================

#ifdef HIGHCOLOR
#include "r_draw16.c"
#endif
