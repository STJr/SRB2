// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
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

UINT8 *ds_source; // start of a 64*64 tile image
UINT8 *ds_transmap; // one of the translucency tables

#ifdef ESLOPE
pslope_t *ds_slope; // Current slope being used
floatv3_t ds_su, ds_sv, ds_sz; // Vectors for... stuff?
float focallengthf, zeroheight;
#endif

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
#define SKIN_RAMP_LENGTH 16
#define DEFAULT_STARTTRANSCOLOR 96
#define NUM_PALETTE_ENTRIES 256

static UINT8** translationtablecache[MAXSKINS + 4] = {NULL};


// See also the enum skincolors_t
// TODO Callum: Can this be translated?
const char *Color_Names[MAXSKINCOLORS] =
{
	"None",     	// SKINCOLOR_NONE
	"White",    	// SKINCOLOR_WHITE
	"Silver",   	// SKINCOLOR_SILVER
	"Grey",	    	// SKINCOLOR_GREY
	"Black",    	// SKINCOLOR_BLACK
	"Beige",    	// SKINCOLOR_BEIGE
	"Peach",    	// SKINCOLOR_PEACH
	"Brown",    	// SKINCOLOR_BROWN
	"Red",      	// SKINCOLOR_RED
	"Crimson",     	// SKINCOLOR_CRIMSON
	"Orange",   	// SKINCOLOR_ORANGE
	"Rust",     	// SKINCOLOR_RUST
	"Gold",      	// SKINCOLOR_GOLD
	"Yellow",   	// SKINCOLOR_YELLOW
	"Tan",      	// SKINCOLOR_TAN
	"Moss",      	// SKINCOLOR_MOSS
	"Peridot",    	// SKINCOLOR_PERIDOT
	"Green",    	// SKINCOLOR_GREEN
	"Emerald",  	// SKINCOLOR_EMERALD
	"Aqua",     	// SKINCOLOR_AQUA
	"Teal",     	// SKINCOLOR_TEAL
	"Cyan",     	// SKINCOLOR_CYAN
	"Blue",     	// SKINCOLOR_BLUE
	"Azure",    	// SKINCOLOR_AZURE
	"Pastel",		// SKINCOLOR_PASTEL
	"Purple",   	// SKINCOLOR_PURPLE
	"Lavender", 	// SKINCOLOR_LAVENDER
	"Magenta",   	// SKINCOLOR_MAGENTA
	"Pink",     	// SKINCOLOR_PINK
	"Rosy"     	// SKINCOLOR_ROSY
};

const UINT8 Color_Opposite[MAXSKINCOLORS*2] =
{
	SKINCOLOR_NONE,8,   	// SKINCOLOR_NONE
	SKINCOLOR_BLACK,10, 	// SKINCOLOR_WHITE
	SKINCOLOR_GREY,4,   	// SKINCOLOR_SILVER
	SKINCOLOR_SILVER,12,	// SKINCOLOR_GREY
	SKINCOLOR_WHITE,8,  	// SKINCOLOR_BLACK
	SKINCOLOR_BEIGE,8,   	// SKINCOLOR_BEIGE - needs new offset
	SKINCOLOR_BROWN,8,   	// SKINCOLOR_PEACH - ditto
	SKINCOLOR_PEACH,8,   	// SKINCOLOR_BROWN - ditto
	SKINCOLOR_GREEN,5,  	// SKINCOLOR_RED
	SKINCOLOR_CYAN,8,   	// SKINCOLOR_CRIMSON - ditto
	SKINCOLOR_BLUE,12,  	// SKINCOLOR_ORANGE
	SKINCOLOR_TAN,8,   		// SKINCOLOR_RUST - ditto
	SKINCOLOR_LAVENDER,8,    // SKINCOLOR_GOLD - ditto
	SKINCOLOR_TEAL,8,   	// SKINCOLOR_YELLOW - ditto
	SKINCOLOR_RUST,8,   	// SKINCOLOR_TAN - ditto
	SKINCOLOR_MAGENTA,3, 	// SKINCOLOR_MOSS
	SKINCOLOR_PURPLE,8,   	// SKINCOLOR_PERIDOT - ditto
	SKINCOLOR_RED,11,   	// SKINCOLOR_GREEN
	SKINCOLOR_PASTEL,8,   	// SKINCOLOR_EMERALD - ditto
	SKINCOLOR_ROSY,8,   	// SKINCOLOR_AQUA - ditto
	SKINCOLOR_YELLOW,8,   	// SKINCOLOR_TEAL - ditto
	SKINCOLOR_CRIMSON,8,   	// SKINCOLOR_CYAN - ditto
	SKINCOLOR_ORANGE,9, 	// SKINCOLOR_BLUE
	SKINCOLOR_PINK,8,   	// SKINCOLOR_AZURE - ditto
	SKINCOLOR_EMERALD,8,   	// SKINCOLOR_PASTEL - ditto
	SKINCOLOR_PERIDOT,8,   	// SKINCOLOR_PURPLE - ditto
	SKINCOLOR_GOLD,8,   	// SKINCOLOR_LAVENDER - ditto
	SKINCOLOR_MOSS,8,   	// SKINCOLOR_MAGENTA - ditto
	SKINCOLOR_AZURE,8,   	// SKINCOLOR_PINK - ditto
	SKINCOLOR_AQUA,8   	// SKINCOLOR_ROSY - ditto
};

CV_PossibleValue_t Color_cons_t[MAXSKINCOLORS+1];

/**	\brief The R_InitTranslationTables

  load in color translation tables
*/
void R_InitTranslationTables(void)
{
#ifdef _NDS
	// Ugly temporary NDS hack.
	transtables = (UINT8*)0x2000000;
#else
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
#endif
}


/**	\brief	Generates a translation colormap.

	\param	dest_colormap	colormap to populate
	\param	skinnum		number of skin, TC_DEFAULT or TC_BOSS
	\param	color		translation color

	\return	void
*/
static void R_GenerateTranslationColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color)
{
	// Table of indices into the palette of the first entries of each translated ramp
	const UINT8 skinbasecolors[] = {
		0x00, // SKINCOLOR_WHITE
		0x03, // SKINCOLOR_SILVER
		0x08, // SKINCOLOR_GREY
		0x18, // SKINCOLOR_BLACK
		0xf0, // SKINCOLOR_BEIGE
		0xd8, // SKINCOLOR_PEACH
		0xe0, // SKINCOLOR_BROWN
		0x21, // SKINCOLOR_RED
		0x28, // SKINCOLOR_CRIMSON
		0x31, // SKINCOLOR_ORANGE
		0x3a, // SKINCOLOR_RUST
		0x40, // SKINCOLOR_GOLD
		0x48, // SKINCOLOR_YELLOW
		0x54, // SKINCOLOR_TAN
		0x58, // SKINCOLOR_MOSS
		0xbc, // SKINCOLOR_PERIDOT
		0x60, // SKINCOLOR_GREEN
		0x70, // SKINCOLOR_EMERALD
		0x78, // SKINCOLOR_AQUA
		0x8c, // SKINCOLOR_TEAL
		0x80, // SKINCOLOR_CYAN
		0x92, // SKINCOLOR_BLUE
		0xaa, // SKINCOLOR_AZURE
		0xa0, // SKINCOLOR_PASTEL
		0xa0, // SKINCOLOR_PURPLE
		0xc0, // SKINCOLOR_LAVENDER
		0xb3, // SKINCOLOR_MAGENTA
		0xd0, // SKINCOLOR_PINK
		0xc8, // SKINCOLOR_ROSY
	};
	INT32 i;
	INT32 starttranscolor;

	// Handle a couple of simple special cases
	if (skinnum == TC_BOSS || skinnum == TC_ALLWHITE || skinnum == TC_METALSONIC || color == SKINCOLOR_NONE)
	{
		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
		{
			if (skinnum == TC_ALLWHITE) dest_colormap[i] = 0;
			else dest_colormap[i] = (UINT8)i;
		}

		// White!
		if (skinnum == TC_BOSS)
			dest_colormap[31] = 0;
		else if (skinnum == TC_METALSONIC)
			dest_colormap[159] = 0;

		return;
	}

	starttranscolor = (skinnum != TC_DEFAULT) ? skins[skinnum].starttranscolor : DEFAULT_STARTTRANSCOLOR;

	// Fill in the entries of the palette that are fixed
	for (i = 0; i < starttranscolor; i++)
		dest_colormap[i] = (UINT8)i;

	for (i = (UINT8)(starttranscolor + 16); i < NUM_PALETTE_ENTRIES; i++)
		dest_colormap[i] = (UINT8)i;

	// Build the translated ramp
	switch (color)
	{
	case SKINCOLOR_SILVER:
	case SKINCOLOR_GREY:
	case SKINCOLOR_BROWN:
	case SKINCOLOR_GREEN:
		// 16 color ramp
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
			dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i);
		break;

	case SKINCOLOR_WHITE:
	case SKINCOLOR_CYAN:
		// 12 color ramp
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
			dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (12*i/SKIN_RAMP_LENGTH));
		break;

	case SKINCOLOR_BLACK:
	case SKINCOLOR_MOSS:
	case SKINCOLOR_EMERALD:
	case SKINCOLOR_LAVENDER:
	case SKINCOLOR_PINK:
		// 8 color ramp
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
			dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1));
		break;

	case SKINCOLOR_BEIGE:
		// 13 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 15)
				dest_colormap[starttranscolor + i] = 0xed; // Darkest
			else if (i <= 6)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + ((i + 1) >> 1)); // Brightest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i - 3);
		}
		break;

	case SKINCOLOR_PEACH:
		// 11 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 0)
				dest_colormap[starttranscolor + i] = 0xD0; // Lightest 1
			else if (i == 1)
				dest_colormap[starttranscolor + i] = 0x30; // Lightest 2
			else if (i <= 11)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) - 1);
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i - 7); // Darkest
		}
		break;

	case SKINCOLOR_RED:
		// 16 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 13)
				dest_colormap[starttranscolor + i] = 0x47; // Semidark
			else if (i > 13)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i - 1); // Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i);
		}
		break;

	case SKINCOLOR_CRIMSON:
		// 9 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i/2 == 6)
				dest_colormap[starttranscolor + i] = 0x47; // Semidark
			else if (i/2 == 7)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i - 8); // Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1));
		}
		break;

	case SKINCOLOR_ORANGE:
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 15)
				dest_colormap[starttranscolor + i] = 0x2c; // Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i);
		}
		break;

	case SKINCOLOR_RUST:
		// 10 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i <= 11)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1));
			else if (i == 12)
				dest_colormap[starttranscolor + i] = 0x2c; // Darkest 4
			else if (i == 13)
				dest_colormap[starttranscolor + i] = 0xfe; // Darkest 3
			else
				dest_colormap[starttranscolor + i] = 0x2d + i - 14; // Darkest 2 and 1
		}
		break;

	case SKINCOLOR_GOLD:
		// 10 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 0)
				dest_colormap[starttranscolor + i] = 0x50; // Lightest 1
			else if (i == 1)
				dest_colormap[starttranscolor + i] = 0x53; // Lightest 2
			else if (i/2 == 7)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i - 8); //Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) - 1);
		}
		break;

	case SKINCOLOR_YELLOW:
		// 10 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 0)
				dest_colormap[starttranscolor + i] = 0x53; // Lightest
			else if (i == 15)
				dest_colormap[starttranscolor + i] = 0xed; // Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1));
		}
		break;

	case SKINCOLOR_TAN:
		// 8 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i/2 == 0)
				dest_colormap[starttranscolor + i] = 0x51; // Lightest
			else if (i/2 == 5)
				dest_colormap[starttranscolor + i] = 0xf5; // Darkest 1
			else if (i/2 == 6)
				dest_colormap[starttranscolor + i] = 0xf9; // Darkest 2
			else if (i/2 == 7)
				dest_colormap[starttranscolor + i] = 0xed; // Darkest 3
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) - 1);
		}
		break;

	case SKINCOLOR_PERIDOT:
		// 8 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i/2 == 0)
				dest_colormap[starttranscolor + i] = 0x58; // Lightest
			else if (i/2 == 7)
				dest_colormap[starttranscolor + i] = 0x77; // Darkest
			else if (i/2 >= 5)
				dest_colormap[starttranscolor + i] = (UINT8)(0x5e + (i >> 1) - 5); // Semidark
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) - 1);
		}
		break;

	case SKINCOLOR_AQUA:
		// 10 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 0)
				dest_colormap[starttranscolor + i] = 0x78; // Lightest
			else if (i >= 14)
				dest_colormap[starttranscolor + i] = (UINT8)(0x76 + i - 14); // Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) + 1);
		}
		break;

	case SKINCOLOR_TEAL:
		// 6 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i <= 1)
				dest_colormap[starttranscolor + i] = 0x78; // Lightest
			else if (i >= 13)
				dest_colormap[starttranscolor + i] = 0x8a; // Darkest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + ((i - 1)/3));
		}
		break;

	case SKINCOLOR_AZURE:
		// 8 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i <= 3)
				dest_colormap[starttranscolor + i] = (UINT8)(0x90 + i/2); // Lightest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) - 2);
		}
		break;

	case SKINCOLOR_BLUE:
		// 16 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 15)
				dest_colormap[starttranscolor + i] = 0x1F; //Darkest 1
			else if (i == 14)
				dest_colormap[starttranscolor + i] = 0xfd; //Darkest 2
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i);
		}
		break;

	case SKINCOLOR_PASTEL:
		// 10 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i >= 12)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i - 7); // Darkest
			else if (i <= 1)
				dest_colormap[starttranscolor + i] = 0x90; // Lightest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) - 1);
		}
		break;

	case SKINCOLOR_PURPLE:
		// 10 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i <= 3)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + i); // Lightest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) + 2);
		}
		break;

	case SKINCOLOR_MAGENTA:
		// 9 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
			if (i == 0)
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1]); // Lightest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + (i >> 1) + 1);
		break;

	case SKINCOLOR_ROSY:
		// 9 colors
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
		{
			if (i == 0)
				dest_colormap[starttranscolor + i] = 0xfc; // Lightest
			else
				dest_colormap[starttranscolor + i] = (UINT8)(skinbasecolors[color - 1] + ((i - 1) >> 1));
		}
		break;

	// Super colors, from lightest to darkest!
	case SKINCOLOR_SUPER1:
		// Super White
		for (i = 0; i < 10; i++)
			dest_colormap[starttranscolor + i] = (UINT8)0; // True white
		for (; i < 12; i++) // White-yellow fade
			dest_colormap[starttranscolor + i] = (UINT8)(80);
		for (; i < 15; i++) // White-yellow fade
			dest_colormap[starttranscolor + i] = (UINT8)(81 + (i-12));
		dest_colormap[starttranscolor + 15] = (UINT8)(72);
		break;

	case SKINCOLOR_SUPER2:
		// Super Bright
		dest_colormap[starttranscolor] = (UINT8)(0);
		for (i = 1; i < 4; i++) // White-yellow fade
			dest_colormap[starttranscolor + i] = (UINT8)(80 + (i-1));
		for (; i < 6; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(83);
		for (; i < 8; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(72);
		for (; i < 14; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(73);
		for (; i < 16; i++) // With a fine golden finish! :3
			dest_colormap[starttranscolor + i] = (UINT8)(64 + (i-14));
		break;

	case SKINCOLOR_SUPER3:
		// Super Yellow
		for (i = 0; i < 2; i++) // White-yellow fade
			dest_colormap[starttranscolor + i] = (UINT8)(81 + i);
		for (; i < 4; i++)
			dest_colormap[starttranscolor + i] = (UINT8)(83);
		for (; i < 6; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(72);
		for (; i < 12; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(73);
		for (; i < 16; i++) // With a fine golden finish! :3
			dest_colormap[starttranscolor + i] = (UINT8)(64 + (i-12));
		break;

	case SKINCOLOR_SUPER4:
		// "The SSNTails"
		dest_colormap[starttranscolor] = 83; // Golden shine
		for (i = 1; i < 3; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(72);
		for (; i < 9; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(73);
		for (; i < 16; i++) // With a fine golden finish! :3
			dest_colormap[starttranscolor + i] = (UINT8)(64 + (i-9));
		break;

	case SKINCOLOR_SUPER5:
		// Golden Delicious
		for (i = 0; i < 2; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(72);
		for (; i < 8; i++) // Yellow
			dest_colormap[starttranscolor + i] = (UINT8)(73);
		for (; i < 15; i++) // With a fine golden finish! :3
			dest_colormap[starttranscolor + i] = (UINT8)(64 + (i-8));
		dest_colormap[starttranscolor + 15] = (UINT8)63;
		break;

	// Super Tails and Knuckles, who really should be dummied out by now
	case SKINCOLOR_TSUPER1:
	case SKINCOLOR_TSUPER2:
	case SKINCOLOR_TSUPER3:
	case SKINCOLOR_TSUPER4:
	case SKINCOLOR_TSUPER5:
	case SKINCOLOR_KSUPER1:
	case SKINCOLOR_KSUPER2:
	case SKINCOLOR_KSUPER3:
	case SKINCOLOR_KSUPER4:
	case SKINCOLOR_KSUPER5:
		for (i = 0; i < SKIN_RAMP_LENGTH; i++)
			dest_colormap[starttranscolor + i] = 0xFF;
		break;

	default:
		I_Error("Invalid skin color #%hu.", (UINT16)color);
		break;
	}
}


/**	\brief	Retrieves a translation colormap from the cache.

	\param	skinnum	number of skin, TC_DEFAULT or TC_BOSS
	\param	color	translation color
	\param	flags	set GTC_CACHE to use the cache

	\return	Colormap. If not cached, caller should Z_Free.
*/
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolors_t color, UINT8 flags)
{
	UINT8* ret;
	INT32 skintableindex;

	// Adjust if we want the default colormap
	if (skinnum == TC_DEFAULT) skintableindex = DEFAULT_TT_CACHE_INDEX;
	else if (skinnum == TC_BOSS) skintableindex = BOSS_TT_CACHE_INDEX;
	else if (skinnum == TC_METALSONIC) skintableindex = METALSONIC_TT_CACHE_INDEX;
	else if (skinnum == TC_ALLWHITE) skintableindex = ALLWHITE_TT_CACHE_INDEX;
	else skintableindex = skinnum;

	if (flags & GTC_CACHE)
	{

		// Allocate table for skin if necessary
		if (!translationtablecache[skintableindex])
			translationtablecache[skintableindex] = Z_Calloc(MAXTRANSLATIONS * sizeof(UINT8**), PU_STATIC, NULL);

		// Get colormap
		ret = translationtablecache[skintableindex][color];
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
			memset(translationtablecache[i], 0, MAXTRANSLATIONS * sizeof(UINT8**));
}

UINT8 R_GetColorByName(const char *name)
{
	UINT8 color = (UINT8)atoi(name);
	if (color > 0 && color < MAXSKINCOLORS)
		return color;
	for (color = 1; color < MAXSKINCOLORS; color++)
		if (!stricmp(Color_Names[color], name))
			return color;
	return 0;
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

// ==========================================================================
//                   INCLUDE 16bpp DRAWING CODE HERE
// ==========================================================================

#ifdef HIGHCOLOR
#include "r_draw16.c"
#endif
