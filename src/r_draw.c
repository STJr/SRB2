// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw.c
/// \brief span / column drawer functions
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
#include "libdivide.h" // used by NPO2 tilted span functions

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// ==========================================================================
//                              COMMON DATA
// ==========================================================================

/**	\brief view info
*/
INT32 viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;

/**	\brief pointer to the start of each line of the screen
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

/**	\brief pointer to the top left of the current view
*/
UINT8 *topleft;

// =========================================================================
//                            TRUECOLOR STUFF
// =========================================================================

/**	\brief use 32 bpp colormaps
*/
boolean tc_colormaps;
boolean tc_spritecolormaps;

/**	\brief pointer to the top left of the current view
*/
UINT32 *topleft_u32;

UINT8 dp_lighting;
extracolormap_t *dp_extracolormap;
extracolormap_t *defaultextracolormap;

// =========================================================================
//                        COLUMN DRAWING CODE STUFF
// =========================================================================

lighttable_t *dc_colormap;
INT32 dc_x = 0, dc_yl = 0, dc_yh = 0;

fixed_t dc_iscale, dc_texturemid;
UINT8 dc_hires; // under MSVC boolean is a byte, while on other systems, it a bit,
               // soo lets make it a byte on all system for the ASM code
UINT8 dc_picfmt = PICFMT_PATCH;
UINT8 dc_colmapstyle = TC_COLORMAPSTYLE_8BPP;
UINT8 *dc_source;

// -----------------------
// translucency stuff here
// -----------------------
boolean usetranstables;

#define NUMTRANSTABLES 9 // how many translucency tables are used

// translucency tables
UINT8 *transtables;
UINT8 *blendtables[NUMBLENDMAPS];

/**	\brief R_DrawTranslucentColumn uses this
*/
UINT8 *dc_transmap; // one of the translucency tables
UINT8 dc_alpha; // column alpha

UINT32 (*R_BlendModeMix)(UINT32, UINT32, UINT8) = NULL;

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
lighttable_t *ds_translation; // Lactozilla: Sprite splat drawer

fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;
INT32 ds_waterofs, ds_bgofs;

UINT16 ds_flatwidth, ds_flatheight;
boolean ds_powersoftwo, ds_solidcolor;
UINT8 ds_picfmt = PICFMT_FLAT;
UINT8 ds_colmapstyle = TC_COLORMAPSTYLE_8BPP;

UINT8 *ds_source; // points to the start of a flat
UINT8 *ds_transmap; // one of the translucency tables
UINT8 ds_alpha; // span alpha

// Vectors for Software's tilted slope drawers
floatv3_t *ds_su, *ds_sv, *ds_sz;
floatv3_t *ds_sup, *ds_svp, *ds_szp;
float focallengthf, zeroheight;

/**	\brief Variable flat sizes
*/

UINT32 nflatxshift, nflatyshift, nflatshiftup, nflatmask;

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

static UINT8 **translationtablecache[MAXSKINS + 7] = {NULL};
UINT8 skincolor_modified[MAXSKINCOLORS];

static INT32 SkinToCacheIndex(INT32 skinnum)
{
	switch (skinnum)
	{
		case TC_DEFAULT:    return DEFAULT_TT_CACHE_INDEX;
		case TC_BOSS:       return BOSS_TT_CACHE_INDEX;
		case TC_METALSONIC: return METALSONIC_TT_CACHE_INDEX;
		case TC_ALLWHITE:   return ALLWHITE_TT_CACHE_INDEX;
		case TC_RAINBOW:    return RAINBOW_TT_CACHE_INDEX;
		case TC_BLINK:      return BLINK_TT_CACHE_INDEX;
		case TC_DASHMODE:   return DASHMODE_TT_CACHE_INDEX;
		     default:       break;
	}

	return skinnum;
}

static INT32 CacheIndexToSkin(INT32 ttc)
{
	switch (ttc)
	{
		case DEFAULT_TT_CACHE_INDEX:    return TC_DEFAULT;
		case BOSS_TT_CACHE_INDEX:       return TC_BOSS;
		case METALSONIC_TT_CACHE_INDEX: return TC_METALSONIC;
		case ALLWHITE_TT_CACHE_INDEX:   return TC_ALLWHITE;
		case RAINBOW_TT_CACHE_INDEX:    return TC_RAINBOW;
		case BLINK_TT_CACHE_INDEX:      return TC_BLINK;
		case DASHMODE_TT_CACHE_INDEX:   return TC_DASHMODE;
		     default:                   break;
	}

	return ttc;
}

CV_PossibleValue_t Color_cons_t[MAXSKINCOLORS+1];

/** \brief Initializes the translucency tables used by the Software renderer.
*/
void R_InitTranslucencyTables(void)
{
	// Load here the transparency lookup tables 'TRANSx0'
	// NOTE: the TRANSx0 resources MUST BE aligned on 64k for the asm
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

	R_GenerateBlendTables();
}

static colorlookup_t transtab_lut;

static void BlendTab_Translucent(UINT8 *table, int style, UINT8 blendamt)
{
	INT16 bg, fg;

	if (table == NULL)
		I_Error("BlendTab_Translucent: input table was NULL!");

	for (bg = 0; bg < 0xFF; bg++)
	{
		for (fg = 0; fg < 0xFF; fg++)
		{
			RGBA_t backrgba = V_GetMasterColor(bg);
			RGBA_t frontrgba = V_GetMasterColor(fg);
			RGBA_t result;

			result.rgba = ASTBlendPixel(backrgba, frontrgba, style, 0xFF);
			result.rgba = ASTBlendPixel(result, frontrgba, AST_TRANSLUCENT, blendamt);

			table[((bg * 0x100) + fg)] = GetColorLUT(&transtab_lut, result.s.red, result.s.green, result.s.blue);
		}
	}
}

static void BlendTab_Subtractive(UINT8 *table, int style, UINT8 blendamt)
{
	INT16 bg, fg;

	if (table == NULL)
		I_Error("BlendTab_Subtractive: input table was NULL!");

	if (blendamt == 0xFF)
	{
		memset(table, GetColorLUT(&transtab_lut, 0, 0, 0), 0x10000);
		return;
	}

	for (bg = 0; bg < 0xFF; bg++)
	{
		for (fg = 0; fg < 0xFF; fg++)
		{
			RGBA_t backrgba = V_GetMasterColor(bg);
			RGBA_t frontrgba = V_GetMasterColor(fg);
			RGBA_t result;

			result.rgba = ASTBlendPixel(backrgba, frontrgba, style, 0xFF);
			result.s.red = max(0, result.s.red - blendamt);
			result.s.green = max(0, result.s.green - blendamt);
			result.s.blue = max(0, result.s.blue - blendamt);

			//probably incorrect, but does look better at lower opacity...
			//result.rgba = ASTBlendPixel(result, frontrgba, AST_TRANSLUCENT, blendamt);

			table[((bg * 0x100) + fg)] = GetColorLUT(&transtab_lut, result.s.red, result.s.green, result.s.blue);
		}
	}
}

static void BlendTab_Modulative(UINT8 *table)
{
	INT16 bg, fg;

	if (table == NULL)
		I_Error("BlendTab_Modulative: input table was NULL!");

	for (bg = 0; bg < 0xFF; bg++)
	{
		for (fg = 0; fg < 0xFF; fg++)
		{
			RGBA_t backrgba = V_GetMasterColor(bg);
			RGBA_t frontrgba = V_GetMasterColor(fg);
			RGBA_t result;
			result.rgba = ASTBlendPixel(backrgba, frontrgba, AST_MODULATE, 0);
			table[((bg * 0x100) + fg)] = GetColorLUT(&transtab_lut, result.s.red, result.s.green, result.s.blue);
		}
	}
}

static INT32 BlendTab_Count[NUMBLENDMAPS] =
{
	NUMTRANSTABLES+1, // blendtab_add
	NUMTRANSTABLES+1, // blendtab_subtract
	NUMTRANSTABLES+1, // blendtab_reversesubtract
	1                 // blendtab_modulate
};

static INT32 BlendTab_FromStyle[] =
{
	0,                        // AST_COPY
	0,                        // AST_TRANSLUCENT
	blendtab_add,             // AST_ADD
	blendtab_subtract,        // AST_SUBTRACT
	blendtab_reversesubtract, // AST_REVERSESUBTRACT
	blendtab_modulate,        // AST_MODULATE
	0                         // AST_OVERLAY
};

static void BlendTab_GenerateMaps(INT32 tab, INT32 style, void (*genfunc)(UINT8 *, int, UINT8))
{
	INT32 i = 0, num = BlendTab_Count[tab];
	const float amtmul = (256.0f / (float)(NUMTRANSTABLES + 1));
	for (; i < num; i++)
	{
		const size_t offs = (0x10000 * i);
		const UINT16 alpha = min(amtmul * i, 0xFF);
		genfunc(blendtables[tab] + offs, style, alpha);
	}
}

void R_GenerateBlendTables(void)
{
	INT32 i;

	for (i = 0; i < NUMBLENDMAPS; i++)
		blendtables[i] = Z_MallocAlign(BlendTab_Count[i] * 0x10000, PU_STATIC, NULL, 16);

	InitColorLUT(&transtab_lut, pMasterPalette, false);

	// Additive
	BlendTab_GenerateMaps(blendtab_add, AST_ADD, BlendTab_Translucent);

	// Subtractive
#if 1
	BlendTab_GenerateMaps(blendtab_subtract, AST_SUBTRACT, BlendTab_Subtractive);
#else
	BlendTab_GenerateMaps(blendtab_subtract, AST_SUBTRACT, BlendTab_Translucent);
#endif

	// Reverse subtractive
	BlendTab_GenerateMaps(blendtab_reversesubtract, AST_REVERSESUBTRACT, BlendTab_Translucent);

	// Modulative blending only requires a single table
	BlendTab_Modulative(blendtables[blendtab_modulate]);
}

#define ClipBlendLevel(style, trans) max(min((trans), BlendTab_Count[BlendTab_FromStyle[style]]-1), 0)
#define ClipTransLevel(trans) max(min((trans), NUMTRANSMAPS-2), 0)

UINT8 *R_GetTranslucencyTable(INT32 alphalevel)
{
	return transtables + (ClipTransLevel(alphalevel-1) << FF_TRANSSHIFT);
}

UINT8 *R_GetBlendTable(int style, INT32 alphalevel)
{
	size_t offs;

	if (style <= AST_COPY || style >= AST_OVERLAY)
		return NULL;

	offs = (ClipBlendLevel(style, alphalevel) << FF_TRANSSHIFT);

	// Lactozilla: Returns the equivalent to AST_TRANSLUCENT
	// if no alpha style matches any of the blend tables.
	switch (style)
	{
		case AST_ADD:
			return blendtables[blendtab_add] + offs;
		case AST_SUBTRACT:
			return blendtables[blendtab_subtract] + offs;
		case AST_REVERSESUBTRACT:
			return blendtables[blendtab_reversesubtract] + offs;
		case AST_MODULATE:
			return blendtables[blendtab_modulate];
		default:
			break;
	}

	// Return a normal translucency table
	if (--alphalevel >= 0)
		return transtables + (ClipTransLevel(alphalevel) << FF_TRANSSHIFT);
	else
		return NULL;
}

boolean R_BlendLevelVisible(INT32 blendmode, INT32 alphalevel)
{
	if (blendmode <= AST_COPY || blendmode == AST_SUBTRACT || blendmode == AST_MODULATE || blendmode >= AST_OVERLAY)
		return true;

	return (alphalevel > 0);
}

INT32 R_AlphaToTransnum(UINT8 alpha)
{
	// Hacked up support for alpha value in software mode Tails 09-24-2002
	if (alpha < 12)
		return -1;
	else if (alpha < 38)
		return tr_trans90;
	else if (alpha < 64)
		return tr_trans80;
	else if (alpha < 89)
		return tr_trans70;
	else if (alpha < 115)
		return tr_trans60;
	else if (alpha < 140)
		return tr_trans50;
	else if (alpha < 166)
		return tr_trans40;
	else if (alpha < 192)
		return tr_trans30;
	else if (alpha < 217)
		return tr_trans20;
	else if (alpha < 243)
		return tr_trans10;
	else
		return 0;
}

UINT8 R_TransnumToAlpha(INT32 num)
{
	num = max(min(num, tr_trans90), 0);

	switch (num)
	{
		case tr_trans10: return 0xE6;
		case tr_trans20: return 0xCC;
		case tr_trans30: return 0xB3;
		case tr_trans40: return 0x99;
		case tr_trans50: return 0x80;
		case tr_trans60: return 0x66;
		case tr_trans70: return 0x4C;
		case tr_trans80: return 0x33;
		case tr_trans90: return 0x19;
	}

	return 0xFF;
}

UINT8 R_BlendModeTransnumToAlpha(int style, INT32 num)
{
	return R_TransnumToAlpha(ClipBlendLevel(style, num));
}

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

/**	\brief	Generates a translation colormap.

	\param	dest_colormap	colormap to populate
	\param	skinnum		skin number, or a translation mode
	\param	color		translation color

	\return	void
*/
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
			UINT8 *originalColormap = R_GetTranslationColormap(TC_DEFAULT, (skincolornum_t)color, GTC_CACHE);
			for (i = 0; i < 16; i++)
			{
				dest_colormap[DEFAULT_STARTTRANSCOLOR + i] = originalColormap[DEFAULT_STARTTRANSCOLOR + i];
				dest_colormap[31-i] = i;
			}
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

	if (skinnum < 0 && skinnum > TC_DEFAULT)
		I_Error("Invalid translation colormap index %d.", skinnum);

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
	INT32 skintableindex = SkinToCacheIndex(skinnum); // Adjust if we want the default colormap
	INT32 i;

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
					R_GenerateTranslationColormap(translationtablecache[i][color], CacheIndexToSkin(i), color);

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
	if (!VID_InSoftwareRenderer())
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
	if (!VID_InSoftwareRenderer())
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

// R_CalcTiltedLighting
// Exactly what it says on the tin. I wish I wasn't too lazy to explain things properly.
static INT32 tiltlighting[MAXVIDWIDTH];

static void R_CalcTiltedLighting(fixed_t start, fixed_t end)
{
	// ZDoom uses a different lighting setup to us, and I couldn't figure out how to adapt their version
	// of this function. Here's my own.
	INT32 left = ds_x1, right = ds_x2;
	fixed_t step = (end-start)/(right-left+1);

	// I wanna do some optimizing by checking for out-of-range segments on either side to fill in all at once,
	// but I'm too bad at coding to not crash the game trying to do that. I guess this is fast enough for now...
	for (INT32 i = left; i <= right; i++)
	{
		tiltlighting[i] = (start += step) >> FRACBITS;
		if (tiltlighting[i] < 0)
			tiltlighting[i] = 0;
		else if (tiltlighting[i] >= MAXLIGHTSCALE)
			tiltlighting[i] = MAXLIGHTSCALE-1;
	}
}

// Lighting is simple. It's just linear interpolation from start to end
#define CALC_SLOPE_LIGHT { \
	float planelightfloat = PLANELIGHTFLOAT; \
	float lightstart, lightend; \
	lightend = (iz + ds_szp->x*width) * planelightfloat; \
	lightstart = iz * planelightfloat; \
	R_CalcTiltedLighting(FloatToFixed(lightstart), FloatToFixed(lightend)); \
}

// ==========================================================================
//                              COLOR MATH
// ==========================================================================

static colorlookup_t r_draw_lut;

#define clamp(c) max(min(c, 0xFF), 0x00)

static inline UINT32 Blend_Copy(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	(void)bg;
	(void)alpha;
	return (0xFF000000 | fg);
}

static inline UINT32 Blend_Translucent(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	fg = R_TranslucentMix(bg, fg, alpha);
	return (0xFF000000 | fg);
}

static inline UINT32 Blend_Additive(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	UINT8 r = clamp(R_GetRgbaR(bg) + R_GetRgbaR(fg));
	UINT8 g = clamp(R_GetRgbaG(bg) + R_GetRgbaG(fg));
	UINT8 b = clamp(R_GetRgbaB(bg) + R_GetRgbaB(fg));

	fg = R_PutRgbaRGB(r, g, b);
	fg = R_TranslucentMix(bg, fg, alpha);

	return (0xFF000000 | fg);
}

static inline UINT32 Blend_Subtractive(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	INT32 mixR, mixG, mixB;
	UINT8 r, g, b;

	if (alpha == 0)
		return 0xFF000000;

	mixR = R_GetRgbaR(fg);
	mixG = R_GetRgbaG(fg);
	mixB = R_GetRgbaB(fg);

	r = clamp((INT32)(-R_GetRgbaR(bg) + mixR));
	g = clamp((INT32)(-R_GetRgbaG(bg) + mixG));
	b = clamp((INT32)(-R_GetRgbaB(bg) + mixB));

	alpha = (0xFF - alpha);

	r = clamp((INT32)(r - alpha));
	g = clamp((INT32)(g - alpha));
	b = clamp((INT32)(b - alpha));

	return (0xFF000000 | R_PutRgbaRGB(r, g, b));
}

static inline UINT32 Blend_ReverseSubtractive(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	INT32 mixR, mixG, mixB;
	UINT8 r, g, b;

	if (alpha == 0)
		return bg;

	mixR = R_GetRgbaR(fg);
	mixG = R_GetRgbaG(fg);
	mixB = R_GetRgbaB(fg);

	r = clamp((INT32)(R_GetRgbaR(bg) - mixR));
	g = clamp((INT32)(R_GetRgbaG(bg) - mixG));
	b = clamp((INT32)(R_GetRgbaB(bg) - mixB));

	fg = R_PutRgbaRGB(r, g, b);
	fg = R_TranslucentMix(bg, fg, alpha);

	return (0xFF000000 | fg);
}

static inline UINT32 Blend_Multiplicative(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	float mixR = ((float)R_GetRgbaR(fg) / 256.0f);
	float mixG = ((float)R_GetRgbaG(fg) / 256.0f);
	float mixB = ((float)R_GetRgbaB(fg) / 256.0f);

	UINT8 r = clamp((int)(R_GetRgbaR(bg) * mixR));
	UINT8 g = clamp((int)(R_GetRgbaG(bg) * mixG));
	UINT8 b = clamp((int)(R_GetRgbaB(bg) * mixB));

	(void)alpha;

	return (0xFF000000 | R_PutRgbaRGB(r, g, b));
}

static inline void R_SetBlendingFunction(INT32 blendmode)
{
	switch (blendmode)
	{
		case AST_COPY:
		case AST_OVERLAY:
			R_BlendModeMix = Blend_Copy;
			break;
		case AST_TRANSLUCENT:
			R_BlendModeMix = Blend_Translucent;
			break;
		case AST_ADD:
			R_BlendModeMix = Blend_Additive;
			break;
		case AST_SUBTRACT:
			R_BlendModeMix = Blend_Subtractive;
			break;
		case AST_REVERSESUBTRACT:
			R_BlendModeMix = Blend_ReverseSubtractive;
			break;
		case AST_MODULATE:
			R_BlendModeMix = Blend_Multiplicative;
			break;
	}
}

static void R_AlphaBlend_8(UINT8 src, UINT8 alpha, UINT8 *dest)
{
	RGBA_t result;
	result.rgba = R_BlendModeMix(GetTrueColor(src), GetTrueColor(*dest), alpha);
	*dest = GetColorLUT(&r_draw_lut, result.s.red, result.s.green, result.s.blue);
}

static UINT8 R_AlphaBlend_s8d8(UINT8 src, UINT8 alpha, UINT8 dest)
{
	RGBA_t result;
	result.rgba = R_BlendModeMix(GetTrueColor(src), GetTrueColor(dest), alpha);
	return GetColorLUT(&r_draw_lut, result.s.red, result.s.green, result.s.blue);
}

#ifdef TRUECOLOR
static inline void R_SetBlendingFunction_ColorMix(INT32 blendmode);
#endif

void R_SetColumnBlendingFunction(INT32 blendmode)
{
#ifdef TRUECOLOR
	if (truecolor && dc_picfmt == PICFMT_PATCH32)
		R_SetBlendingFunction_ColorMix(blendmode);
	else
#endif
		R_SetBlendingFunction(blendmode);
}

void R_SetSpanBlendingFunction(INT32 blendmode)
{
#ifdef TRUECOLOR
	if (truecolor && ds_picfmt == PICFMT_FLAT32)
		R_SetBlendingFunction_ColorMix(blendmode);
	else
#endif
		R_SetBlendingFunction(blendmode);
}

void R_InitAlphaLUT(void)
{
	InitColorLUT(&r_draw_lut, pMasterPalette, false);
}

// ==========================================================================
//                      INCLUDE 8bpp DRAWING CODE HERE
// ==========================================================================

#include "r_draw8.c"
#include "r_draw8_npo2.c"

// ==========================================================================
//                     INCLUDE 32bpp DRAWING CODE HERE
// ==========================================================================

#ifdef TRUECOLOR
typedef struct
{
	UINT32 fg, bg;
	UINT32 result;
} tc_mixcache_t;

static tc_mixcache_t tc_mixcache[2];

static UINT32 TC_ColorMix(UINT32 fg, UINT32 bg)
{
	RGBA_t rgba;
	UINT32 pixel, origpixel = fg;
	UINT8 tint, alpha = R_GetRgbaA(origpixel);
	tc_mixcache_t *cache = &tc_mixcache[0];

	if (!alpha)
		return bg;
	else if (alpha == 0xFF && fg == cache->fg)
		return cache->result;
	else if (alpha < 0xFF && fg == cache->fg && bg == cache->bg)
		return cache->result;

	cache->fg = fg;
	cache->bg = bg;

	pixel = rgba.rgba = fg;
	tint = R_GetRgbaA(dp_extracolormap->rgba) * 10;

	// Mix pixel with blend color
	if (tint > 0)
		pixel = TC_TintTrueColor(rgba, (UINT32)(dp_extracolormap->rgba), tint);

	// Mix pixel with fade color
	fg = R_TranslucentMix(pixel, (UINT32)(dp_extracolormap->fadergba), (0xFF - dp_lighting));

	// Mix pixel with its own alpha value
	fg = R_TranslucentMix(bg, fg, alpha);

	// Apply the color cube
	fg = ColorCube_ApplyRGBA(fg);

	cache->result = (0xFF000000 | fg);
	return cache->result;
}

static UINT32 TC_ColorMix2(UINT32 fg, UINT32 bg)
{
	RGBA_t rgba;
	UINT32 pixel, origpixel = fg;
	UINT8 tint, alpha = R_GetRgbaA(origpixel);
	tc_mixcache_t *cache = &tc_mixcache[1];

	if (!alpha)
		return bg;
	else if (alpha == 0xFF && fg == cache->fg)
		return cache->result;
	else if (alpha < 0xFF && fg == cache->fg && bg == cache->bg)
		return cache->result;

	cache->fg = fg;
	cache->bg = bg;

	pixel = rgba.rgba = fg;
	tint = R_GetRgbaA(dp_extracolormap->rgba) * 10;

	// Mix pixel with blend color
	if (tint > 0)
		pixel = TC_TintTrueColor(rgba, (UINT32)(dp_extracolormap->rgba), tint);

	// Mix pixel with fade color
	fg = R_TranslucentMix(pixel, (UINT32)(dp_extracolormap->fadergba), (0xFF - dp_lighting));

	// Apply the color cube
	fg = ColorCube_ApplyRGBA(fg);

	cache->result = (0xFF000000 | fg);
	return cache->result;
}

void TC_ClearMixCache(void)
{
#define CLEAR(i) tc_mixcache[i].fg = tc_mixcache[i].bg = tc_mixcache[i].result = 0x00000000
	CLEAR(0);
	CLEAR(1);
#undef CLEAR
}

FUNCMATH UINT32 TC_TintTrueColor(RGBA_t rgba, UINT32 blendcolor, UINT8 tintamt)
{
	fixed_t r, g, b;
	fixed_t cmaskr, cmaskg, cmaskb;
	fixed_t cbrightness;
	fixed_t maskamt, othermask;
	UINT32 origpixel = rgba.rgba;

	r = cmaskr = rgba.s.red;
	g = cmaskg = rgba.s.green;
	b = cmaskb = rgba.s.blue;

	cbrightness = (r+r+r+b+g+g+g+g)<<13;
	maskamt = tintamt<<8;
	othermask = (0xFF-tintamt)<<8;

	cmaskr <<= FRACBITS;
	cmaskg <<= FRACBITS;
	cmaskb <<= FRACBITS;

	maskamt = FixedDiv(maskamt, 0xFF0000);
	cmaskr = FixedMul(cmaskr, maskamt);
	cmaskg = FixedMul(cmaskg, maskamt);
	cmaskb = FixedMul(cmaskb, maskamt);

	r <<= FRACBITS;
	g <<= FRACBITS;
	b <<= FRACBITS;

	rgba.s.red = (FixedMul(cbrightness, cmaskr)>>FRACBITS) + (FixedMul(r, othermask)>>FRACBITS);
	rgba.s.green = (FixedMul(cbrightness, cmaskg)>>FRACBITS) + (FixedMul(g, othermask)>>FRACBITS);
	rgba.s.blue = (FixedMul(cbrightness, cmaskb)>>FRACBITS) + (FixedMul(b, othermask)>>FRACBITS);

	return R_TranslucentMix(origpixel, R_TranslucentMix(rgba.rgba, blendcolor, (cbrightness>>FRACBITS)), tintamt);
}

static inline UINT32 TC_Colormap32Mix(UINT32 color)
{
	return ColorCube_ApplyRGBA(color);
}

static inline UINT32 Blend_ColorMix_Copy(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	(void)alpha;
	fg = TC_ColorMix(fg, bg);
	return (0xFF000000 | fg);
}

static inline UINT32 Blend_ColorMix_Translucent(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	fg = TC_ColorMix(fg, bg);
	fg = R_TranslucentMix(bg, fg, alpha);
	return (0xFF000000 | fg);
}

static inline UINT32 Blend_ColorMix_Additive(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	float mixR, mixG, mixB, mixA = (float)R_GetRgbaA(fg) / 255;
	UINT8 r, g, b;

	fg = TC_ColorMix2(fg, bg);
	mixR = (float)R_GetRgbaR(fg) * mixA;
	mixG = (float)R_GetRgbaG(fg) * mixA;
	mixB = (float)R_GetRgbaB(fg) * mixA;

	r = clamp((INT32)(R_GetRgbaR(bg) + mixR));
	g = clamp((INT32)(R_GetRgbaG(bg) + mixG));
	b = clamp((INT32)(R_GetRgbaB(bg) + mixB));

	fg = R_PutRgbaRGB(r, g, b);
	fg = R_TranslucentMix(bg, fg, alpha);

	return (0xFF000000 | fg);
}

static inline UINT32 Blend_ColorMix_Subtractive(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	INT32 mixR, mixG, mixB, mixA = R_GetRgbaA(fg);
	UINT8 r, g, b;

	if (alpha == 0)
		return 0xFF000000;

	fg = TC_ColorMix2(fg, bg);
	mixR = R_GetRgbaR(fg);
	mixG = R_GetRgbaG(fg);
	mixB = R_GetRgbaB(fg);
	mixA = (0xFF - mixA);

	r = clamp((INT32)(-R_GetRgbaR(bg) + mixR));
	g = clamp((INT32)(-R_GetRgbaG(bg) + mixG));
	b = clamp((INT32)(-R_GetRgbaB(bg) + mixB));

	r = clamp((INT32)(r - mixA));
	g = clamp((INT32)(g - mixA));
	b = clamp((INT32)(b - mixA));
	fg = R_PutRgbaRGBA(r, g, b, 0xFF);

	if (alpha != 0xFF)
		fg = R_TranslucentMix(0, fg, alpha);

	return (0xFF000000 | fg);
}

static inline UINT32 Blend_ColorMix_ReverseSubtractive(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	float mixR, mixG, mixB, mixA = (float)R_GetRgbaA(fg) / 255;
	UINT8 r, g, b;

	fg = TC_ColorMix2(fg, bg);
	mixR = (float)R_GetRgbaR(fg) * mixA;
	mixG = (float)R_GetRgbaG(fg) * mixA;
	mixB = (float)R_GetRgbaB(fg) * mixA;

	r = clamp((INT32)(R_GetRgbaR(bg) - mixR));
	g = clamp((INT32)(R_GetRgbaG(bg) - mixG));
	b = clamp((INT32)(R_GetRgbaB(bg) - mixB));

	fg = R_PutRgbaRGB(r, g, b);
	fg = R_TranslucentMix(bg, fg, alpha);

	return (0xFF000000 | fg);
}

static inline UINT32 Blend_ColorMix_Multiplicative(UINT32 fg, UINT32 bg, UINT8 alpha)
{
	return Blend_Multiplicative(TC_ColorMix(fg, bg), bg, alpha);
}

static inline void R_SetBlendingFunction_ColorMix(INT32 blendmode)
{
	switch (blendmode)
	{
		case AST_COPY:
		case AST_OVERLAY:
			R_BlendModeMix = Blend_ColorMix_Copy;
			break;
		case AST_TRANSLUCENT:
			R_BlendModeMix = Blend_ColorMix_Translucent;
			break;
		case AST_ADD:
			R_BlendModeMix = Blend_ColorMix_Additive;
			break;
		case AST_SUBTRACT:
			R_BlendModeMix = Blend_ColorMix_Subtractive;
			break;
		case AST_REVERSESUBTRACT:
			R_BlendModeMix = Blend_ColorMix_ReverseSubtractive;
			break;
		case AST_MODULATE:
			R_BlendModeMix = Blend_ColorMix_Multiplicative;
			break;
	}
}

#undef clamp

#define WriteTranslucentColumn(idx) *dest = R_BlendModeMix(GetTrueColor(idx), *(UINT32 *)dest, dc_alpha)
#define WriteTranslucentColumn32(idx) *dest = R_BlendModeMix(idx, *(UINT32 *)dest, dc_alpha)

#define WriteTranslucentSpan(idx) *dest = R_BlendModeMix(GetTrueColor(idx), *(UINT32 *)dest, ds_alpha)
#define WriteTranslucentSpan32(idx) *dest = R_BlendModeMix(idx, *(UINT32 *)dest, ds_alpha)
#define WriteTranslucentSpanIdx(idx, destidx) dest[destidx] = R_BlendModeMix(GetTrueColor(idx), dest[destidx], ds_alpha)
#define WriteTranslucentSpanIdx32(idx, destidx) dest[destidx] = R_BlendModeMix(idx, dest[destidx], ds_alpha)

#define WriteTranslucentWaterSpan(idx) *dest = R_BlendModeMix(GetTrueColor(idx), *(UINT32 *)dsrc, ds_alpha); dsrc++
#define WriteTranslucentWaterSpan32(idx) *dest = R_BlendModeMix(idx, *(UINT32 *)dsrc, ds_alpha); dsrc++
#define WriteTranslucentWaterSpanIdx(idx, destidx) dest[destidx] = R_BlendModeMix(GetTrueColor(idx), *(UINT32 *)dsrc, ds_alpha); dsrc++
#define WriteTranslucentWaterSpanIdx32(idx, destidx) dest[destidx] = R_BlendModeMix(idx, *(UINT32 *)dsrc, ds_alpha); dsrc++

#include "r_draw32.c"
#include "r_draw32_npo2.c"
#endif
