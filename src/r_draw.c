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

#ifdef ESLOPE
pslope_t *ds_slope; // Current slope being used
floatv3_t ds_su[MAXVIDHEIGHT], ds_sv[MAXVIDHEIGHT], ds_sz[MAXVIDHEIGHT]; // Vectors for... stuff?
floatv3_t *ds_sup, *ds_svp, *ds_szp;
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
#define RAINBOW_TT_CACHE_INDEX (MAXSKINS + 4)
#define BLINK_TT_CACHE_INDEX (MAXSKINS + 5)
#define DASHMODE_TT_CACHE_INDEX (MAXSKINS + 6)
#define DEFAULT_STARTTRANSCOLOR 96
#define NUM_PALETTE_ENTRIES 256

static UINT8** translationtablecache[MAXSKINS + 7] = {NULL};

const UINT8 Color_Index[MAXTRANSLATIONS-1][16] = {
	// {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // SKINCOLOR_NONE

	// Greyscale ranges
	{0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x02, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x11}, // SKINCOLOR_WHITE
	{0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x11, 0x12}, // SKINCOLOR_BONE
	{0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14}, // SKINCOLOR_CLOUDY
	{0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18}, // SKINCOLOR_GREY
	{0x02, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f}, // SKINCOLOR_SILVER
	{0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x16, 0x17, 0x17, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1d}, // SKINCOLOR_CARBON
	{0x00, 0x05, 0x0a, 0x0f, 0x14, 0x19, 0x1a, 0x1b, 0x1c, 0x1e, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f}, // SKINCOLOR_JET
	{0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1b, 0x1b, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f}, // SKINCOLOR_BLACK

	// Desaturated
	{0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x91, 0x91, 0x91, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xaf}, // SKINCOLOR_AETHER
	{0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0xaa, 0xaa, 0xaa, 0xab, 0xac, 0xac, 0xad, 0xad, 0xae, 0xaf}, // SKINCOLOR_SLATE
	{0x90, 0x91, 0x92, 0x93, 0x94, 0x94, 0x95, 0xac, 0xac, 0xad, 0xad, 0xa8, 0xa8, 0xa9, 0xfd, 0xfe}, // SKINCOLOR_BLUEBELL
	{0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0xd3, 0xd3, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0x2b, 0x2c, 0x2e}, // SKINCOLOR_PINK
	{0xd0, 0x30, 0xd8, 0xd9, 0xda, 0xdb, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe3, 0xe6, 0xe8, 0xe9}, // SKINCOLOR_YOGURT
	{0xdf, 0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef}, // SKINCOLOR_BROWN
	{0xde, 0xe0, 0xe1, 0xe4, 0xe7, 0xe9, 0xeb, 0xec, 0xed, 0xed, 0xed, 0x19, 0x19, 0x1b, 0x1d, 0x1e}, // SKINCOLOR_BRONZE
	{0x51, 0x51, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x56, 0x57, 0xf5, 0xf5, 0xf9, 0xf9, 0xed, 0xed}, // SKINCOLOR_TAN
	{0x54, 0x55, 0x56, 0x56, 0xf2, 0xf3, 0xf3, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfb, 0xed, 0xed}, // SKINCOLOR_BEIGE
	{0x58, 0x58, 0x59, 0x59, 0x5a, 0x5a, 0x5b, 0x5b, 0x5b, 0x5c, 0x5d, 0x5d, 0x5e, 0x5e, 0x5f, 0x5f}, // SKINCOLOR_MOSS
	{0x90, 0x90, 0x91, 0x91, 0xaa, 0xaa, 0xab, 0xab, 0xab, 0xac, 0xad, 0xad, 0xae, 0xae, 0xaf, 0xaf}, // SKINCOLOR_AZURE
	{0xc0, 0xc0, 0xc1, 0xc1, 0xc2, 0xc2, 0xc3, 0xc3, 0xc3, 0xc4, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc7}, // SKINCOLOR_LAVENDER

	// Viv's vivid colours (toast 21/07/17)
	{0xb0, 0xb0, 0xc9, 0xca, 0xcc, 0x26, 0x27, 0x28, 0x29, 0x2a, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xfd}, // SKINCOLOR_RUBY
	{0xd0, 0xd0, 0xd1, 0xd2, 0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e}, // SKINCOLOR_SALMON
	{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x47, 0x2e, 0x2f}, // SKINCOLOR_RED
	{0x27, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2e, 0x2e, 0x2f, 0x2f, 0x1f}, // SKINCOLOR_CRIMSON
	{0x31, 0x32, 0x33, 0x36, 0x22, 0x22, 0x25, 0x25, 0x25, 0xcd, 0xcf, 0xcf, 0xc5, 0xc5, 0xc7, 0xc7}, // SKINCOLOR_FLAME
	{0x48, 0x49, 0x40, 0x33, 0x34, 0x36, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2b, 0x2c, 0x47, 0x2e, 0x2f}, // SKINCOLOR_KETCHUP
	{0xd0, 0x30, 0x31, 0x31, 0x32, 0x32, 0xdc, 0xdc, 0xdc, 0xd3, 0xd4, 0xd4, 0xcc, 0xcd, 0xce, 0xcf}, // SKINCOLOR_PEACHY
	{0xd8, 0xd9, 0xdb, 0xdc, 0xde, 0xdf, 0xd5, 0xd5, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0x1d, 0x1f}, // SKINCOLOR_QUAIL
	{0x51, 0x52, 0x40, 0x40, 0x34, 0x36, 0xd5, 0xd5, 0xd6, 0xd7, 0xcf, 0xcf, 0xc6, 0xc6, 0xc7, 0xfe}, // SKINCOLOR_SUNSET
	{0x58, 0x54, 0x40, 0x34, 0x35, 0x38, 0x3a, 0x3c, 0x3d, 0x2a, 0x2b, 0x2c, 0x2c, 0xba, 0xba, 0xbb}, // SKINCOLOR_COPPER
	{0x00, 0xd8, 0xd9, 0xda, 0xdb, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e}, // SKINCOLOR_APRICOT
	{0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x2c}, // SKINCOLOR_ORANGE
	{0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3c, 0x3d, 0x3d, 0x3d, 0x3f, 0x2c, 0x2d, 0x47, 0x2e, 0x2f, 0x2f}, // SKINCOLOR_RUST
	{0x51, 0x51, 0x54, 0x54, 0x41, 0x42, 0x43, 0x43, 0x44, 0x45, 0x46, 0x3f, 0x2d, 0x2e, 0x2f, 0x2f}, // SKINCOLOR_GOLD
	{0x53, 0x40, 0x41, 0x42, 0x43, 0xe6, 0xe9, 0xe9, 0xea, 0xec, 0xec, 0xc6, 0xc6, 0xc7, 0xc7, 0xfe}, // SKINCOLOR_SANDY
	{0x52, 0x53, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4b, 0x4b, 0x4c, 0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0xed}, // SKINCOLOR_YELLOW
	{0x4b, 0x4b, 0x4c, 0x4c, 0x4d, 0x4e, 0xe7, 0xe7, 0xe9, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc7, 0xfd}, // SKINCOLOR_OLIVE
	{0x50, 0x51, 0x52, 0x53, 0x48, 0xbc, 0xbd, 0xbe, 0xbe, 0xbf, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f}, // SKINCOLOR_LIME
	{0x58, 0x58, 0xbc, 0xbc, 0xbd, 0xbd, 0xbe, 0xbe, 0xbe, 0xbf, 0x5e, 0x5e, 0x5f, 0x5f, 0x77, 0x77}, // SKINCOLOR_PERIDOT
	{0x49, 0x49, 0xbc, 0xbd, 0xbe, 0xbe, 0xbe, 0x67, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6d, 0x6d}, // SKINCOLOR_APPLE
	{0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f}, // SKINCOLOR_GREEN
	{0x65, 0x66, 0x67, 0x68, 0x69, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6d, 0x6e, 0x6e, 0x6e, 0x6f}, // SKINCOLOR_FOREST
	{0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x73, 0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77}, // SKINCOLOR_EMERALD
	{0x00, 0x00, 0x58, 0x58, 0x59, 0x62, 0x62, 0x62, 0x64, 0x67, 0x7e, 0x7e, 0x8f, 0x8f, 0x8a, 0x8a}, // SKINCOLOR_MINT
	{0x01, 0x58, 0x59, 0x5a, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x8f, 0x8f, 0x8a, 0x8a, 0x8a, 0xfd, 0xfd}, // SKINCOLOR_SEAFOAM
	{0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7b, 0x7c, 0x7c, 0x7c, 0x7d, 0x7e, 0x7e, 0x7f, 0x7f, 0x76, 0x77}, // SKINCOLOR_AQUA
	{0x78, 0x78, 0x8c, 0x8c, 0x8d, 0x8d, 0x8d, 0x8e, 0x8e, 0x8f, 0x8f, 0x8f, 0x8a, 0x8a, 0x8a, 0x8a}, // SKINCOLOR_TEAL
	{0x00, 0x78, 0x78, 0x79, 0x8d, 0x87, 0x88, 0x89, 0x89, 0xae, 0xa8, 0xa8, 0xa9, 0xa9, 0xfd, 0xfd}, // SKINCOLOR_WAVE
	{0x80, 0x81, 0xff, 0xff, 0x83, 0x83, 0x8d, 0x8d, 0x8d, 0x8e, 0x7e, 0x7f, 0x76, 0x76, 0x77, 0x6e}, // SKINCOLOR_CYAN
	{0x80, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x85, 0x85, 0x86, 0x87, 0x88, 0x89, 0x89, 0x8a, 0x8b}, // SKINCOLOR_SKY
	{0x85, 0x86, 0x87, 0x88, 0x88, 0x89, 0x89, 0x89, 0x8a, 0x8a, 0xfd, 0xfd, 0xfd, 0x1f, 0x1f, 0x1f}, // SKINCOLOR_CERULEAN
	{0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x83, 0x83, 0x86, 0x87, 0x95, 0x95, 0xad, 0xad, 0xae, 0xaf}, // SKINCOLOR_ICY
	{0x80, 0x83, 0x86, 0x87, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfe}, // SKINCOLOR_SAPPHIRE
	{0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x9a, 0x9c, 0x9d, 0x9d, 0x9e, 0x9e, 0x9e}, // SKINCOLOR_CORNFLOWER
	{0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfe}, // SKINCOLOR_BLUE
	{0x93, 0x94, 0x95, 0x96, 0x98, 0x9a, 0x9b, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfd, 0xfe, 0xfe}, // SKINCOLOR_COBALT
	{0x80, 0x81, 0x83, 0x86, 0x94, 0x94, 0xa3, 0xa3, 0xa4, 0xa6, 0xa6, 0xa6, 0xa8, 0xa8, 0xa9, 0xa9}, // SKINCOLOR_VAPOR
	{0x92, 0x93, 0x94, 0x94, 0xac, 0xad, 0xad, 0xad, 0xae, 0xae, 0xaf, 0xaf, 0xa9, 0xa9, 0xfd, 0xfd}, // SKINCOLOR_DUSK
	{0x90, 0x90, 0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa2, 0xa3, 0xa4, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8}, // SKINCOLOR_PASTEL
	{0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa5, 0xa6, 0xa7, 0xa7, 0xa8, 0xa8, 0xa9, 0xa9}, // SKINCOLOR_PURPLE
	{0x00, 0xd0, 0xd0, 0xc8, 0xc8, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8}, // SKINCOLOR_BUBBLEGUM
	{0xb3, 0xb3, 0xb4, 0xb5, 0xb6, 0xb6, 0xb7, 0xb7, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xbb}, // SKINCOLOR_MAGENTA
	{0xb3, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xbb, 0xc7, 0xc7, 0x1d, 0x1d, 0x1e}, // SKINCOLOR_NEON
	{0xd0, 0xd1, 0xd2, 0xca, 0xcc, 0xb8, 0xb9, 0xb9, 0xba, 0xa8, 0xa8, 0xa9, 0xa9, 0xfd, 0xfe, 0xfe}, // SKINCOLOR_VIOLET
	{0x00, 0xd0, 0xd1, 0xd2, 0xd3, 0xc1, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc5, 0xc6, 0xc6, 0xfe, 0x1f}, // SKINCOLOR_LILAC
	{0xc8, 0xd3, 0xd5, 0xd6, 0xd7, 0xce, 0xcf, 0xb9, 0xb9, 0xba, 0xba, 0xa9, 0xa9, 0xa9, 0xfd, 0xfe}, // SKINCOLOR_PLUM
	{0xc8, 0xc9, 0xca, 0xcb, 0xcb, 0xcc, 0xcd, 0xcd, 0xce, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xfe, 0xfe}, // SKINCOLOR_RASPBERRY
	{0xfc, 0xc8, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 0xcb, 0xcc, 0xcc, 0xcd, 0xcd, 0xce, 0xce, 0xcf}, // SKINCOLOR_ROSY

	// {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // SKINCOLOR_?

	// super
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03}, // SKINCOLOR_SUPERSILVER1
	{0x00, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07}, // SKINCOLOR_SUPERSILVER2
	{0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b}, // SKINCOLOR_SUPERSILVER3
	{0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11}, // SKINCOLOR_SUPERSILVER4
	{0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13}, // SKINCOLOR_SUPERSILVER5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2}, // SKINCOLOR_SUPERRED1
	{0x00, 0x00, 0x00, 0xd0, 0xd0, 0xd0, 0xd1, 0xd1, 0xd1, 0xd2, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21}, // SKINCOLOR_SUPERRED2
	{0x00, 0x00, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23}, // SKINCOLOR_SUPERRED3
	{0x00, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24}, // SKINCOLOR_SUPERRED4
	{0xd0, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25}, // SKINCOLOR_SUPERRED5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x30, 0x31, 0x32, 0x33, 0x34}, // SKINCOLOR_SUPERORANGE1
	{0x00, 0x00, 0x00, 0x00, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34}, // SKINCOLOR_SUPERORANGE2
	{0x00, 0x00, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35}, // SKINCOLOR_SUPERORANGE3
	{0x00, 0xd0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x44, 0x45, 0x46}, // SKINCOLOR_SUPERORANGE4
	{0xd0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x44, 0x45, 0x46, 0x47}, // SKINCOLOR_SUPERORANGE5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x51, 0x52, 0x53, 0x48}, // SKINCOLOR_SUPERGOLD1
	{0x00, 0x50, 0x51, 0x52, 0x53, 0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41}, // SKINCOLOR_SUPERGOLD2
	{0x51, 0x52, 0x53, 0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43}, // SKINCOLOR_SUPERGOLD3
	{0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46}, // SKINCOLOR_SUPERGOLD4
	{0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47}, // SKINCOLOR_SUPERGOLD5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x58, 0x58, 0xbc, 0xbc, 0xbc}, // SKINCOLOR_SUPERPERIDOT1
	{0x00, 0x58, 0x58, 0x58, 0xbc, 0xbc, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe}, // SKINCOLOR_SUPERPERIDOT2
	{0x58, 0x58, 0xbc, 0xbc, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf}, // SKINCOLOR_SUPERPERIDOT3
	{0x58, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf, 0x5e, 0x5e, 0x5f}, // SKINCOLOR_SUPERPERIDOT4
	{0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf, 0x5e, 0x5e, 0x5f, 0x77}, // SKINCOLOR_SUPERPERIDOT5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x81, 0x82, 0x83, 0x84}, // SKINCOLOR_SUPERSKY1
	{0x00, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86}, // SKINCOLOR_SUPERSKY2
	{0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87}, // SKINCOLOR_SUPERSKY3
	{0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a}, // SKINCOLOR_SUPERSKY4
	{0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a, 0x8b}, // SKINCOLOR_SUPERSKY5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0xa0, 0xa0, 0xa1, 0xa2}, // SKINCOLOR_SUPERPURPLE1
	{0x00, 0x90, 0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5}, // SKINCOLOR_SUPERPURPLE2
	{0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6}, // SKINCOLOR_SUPERPURPLE3
	{0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6, 0xa7, 0xa8, 0xa9}, // SKINCOLOR_SUPERPURPLE4
	{0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6, 0xa7, 0xa8, 0xa9, 0xfd}, // SKINCOLOR_SUPERPURPLE5

	{0x00, 0xd0, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x32, 0x33, 0x37, 0x3a, 0x44, 0x45, 0x46, 0x47, 0x2e}, // SKINCOLOR_SUPERRUST1
	{0x30, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x38, 0x3a, 0x44, 0x45, 0x46, 0x47, 0x47, 0x2e}, // SKINCOLOR_SUPERRUST2
	{0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x3a, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x2e, 0x2e}, // SKINCOLOR_SUPERRUST3
	{0x48, 0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x2e, 0x2e, 0x2e}, // SKINCOLOR_SUPERRUST4
	{0x41, 0x42, 0x43, 0x43, 0x44, 0x44, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xed, 0xee, 0xee, 0xef, 0xef}, // SKINCOLOR_SUPERRUST5

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52}, // SKINCOLOR_SUPERTAN1
	{0x00, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5}, // SKINCOLOR_SUPERTAN2
	{0x50, 0x51, 0x51, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9}, // SKINCOLOR_SUPERTAN3
	{0x51, 0x52, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9, 0xfb, 0xed}, // SKINCOLOR_SUPERTAN4
	{0x52, 0x52, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9, 0xfb, 0xed, 0xee, 0xef, 0xef}  // SKINCOLOR_SUPERTAN5
};

// See also the enum skincolors_t
// TODO Callum: Can this be translated?
const char *Color_Names[MAXSKINCOLORS + NUMSUPERCOLORS] =
{
	"None",     	// SKINCOLOR_NONE,

	// Greyscale ranges
	"White",     	// SKINCOLOR_WHITE,
	"Bone",     	// SKINCOLOR_BONE,
	"Cloudy",     	// SKINCOLOR_CLOUDY,
	"Grey",     	// SKINCOLOR_GREY,
	"Silver",     	// SKINCOLOR_SILVER,
	"Carbon",     	// SKINCOLOR_CARBON,
	"Jet",     		// SKINCOLOR_JET,
	"Black",     	// SKINCOLOR_BLACK,

	// Desaturated
	"Aether",     	// SKINCOLOR_AETHER,
	"Slate",     	// SKINCOLOR_SLATE,
	"Bluebell",    	// SKINCOLOR_BLUEBELL,
	"Pink",     	// SKINCOLOR_PINK,
	"Yogurt",     	// SKINCOLOR_YOGURT,
	"Brown",      	// SKINCOLOR_BROWN,
	"Bronze",     	// SKINCOLOR_BRONZE,
	"Tan",     		// SKINCOLOR_TAN,
	"Beige",     	// SKINCOLOR_BEIGE,
	"Moss",     	// SKINCOLOR_MOSS,
	"Azure",     	// SKINCOLOR_AZURE,
	"Lavender",     // SKINCOLOR_LAVENDER,

	// Viv's vivid colours (toast 21/07/17)
	"Ruby",     	// SKINCOLOR_RUBY,
	"Salmon",     	// SKINCOLOR_SALMON,
	"Red",     		// SKINCOLOR_RED,
	"Crimson",     	// SKINCOLOR_CRIMSON,
	"Flame",     	// SKINCOLOR_FLAME,
	"Ketchup",    	// SKINCOLOR_KETCHUP,
	"Peachy",     	// SKINCOLOR_PEACHY,
	"Quail",     	// SKINCOLOR_QUAIL,
	"Sunset",     	// SKINCOLOR_SUNSET,
	"Copper",     	// SKINCOLOR_COPPER,
	"Apricot",     	// SKINCOLOR_APRICOT,
	"Orange",     	// SKINCOLOR_ORANGE,
	"Rust",     	// SKINCOLOR_RUST,
	"Gold",     	// SKINCOLOR_GOLD,
	"Sandy",     	// SKINCOLOR_SANDY,
	"Yellow",     	// SKINCOLOR_YELLOW,
	"Olive",     	// SKINCOLOR_OLIVE,
	"Lime",     	// SKINCOLOR_LIME,
	"Peridot",     	// SKINCOLOR_PERIDOT,
	"Apple",      	// SKINCOLOR_APPLE,
	"Green",     	// SKINCOLOR_GREEN,
	"Forest",     	// SKINCOLOR_FOREST,
	"Emerald",     	// SKINCOLOR_EMERALD,
	"Mint",     	// SKINCOLOR_MINT,
	"Seafoam",     	// SKINCOLOR_SEAFOAM,
	"Aqua",     	// SKINCOLOR_AQUA,
	"Teal",     	// SKINCOLOR_TEAL,
	"Wave",     	// SKINCOLOR_WAVE,
	"Cyan",     	// SKINCOLOR_CYAN,
	"Sky",     		// SKINCOLOR_SKY,
	"Cerulean",     // SKINCOLOR_CERULEAN,
	"Icy",     		// SKINCOLOR_ICY,
	"Sapphire",     // SKINCOLOR_SAPPHIRE,
	"Cornflower",   // SKINCOLOR_CORNFLOWER,
	"Blue",     	// SKINCOLOR_BLUE,
	"Cobalt",     	// SKINCOLOR_COBALT,
	"Vapor",     	// SKINCOLOR_VAPOR,
	"Dusk",     	// SKINCOLOR_DUSK,
	"Pastel",     	// SKINCOLOR_PASTEL,
	"Purple",     	// SKINCOLOR_PURPLE,
	"Bubblegum",    // SKINCOLOR_BUBBLEGUM,
	"Magenta",     	// SKINCOLOR_MAGENTA,
	"Neon",     	// SKINCOLOR_NEON,
	"Violet",     	// SKINCOLOR_VIOLET,
	"Lilac",     	// SKINCOLOR_LILAC,
	"Plum",     	// SKINCOLOR_PLUM,
	"Raspberry", 	// SKINCOLOR_RASPBERRY,
	"Rosy",     	// SKINCOLOR_ROSY,

	// Super behaves by different rules (one name per 5 colours), and will be accessed exclusively via R_GetSuperColorByName instead of R_GetColorByName.
	"Silver",		// SKINCOLOR_SUPERSILVER1,
	"Red",			// SKINCOLOR_SUPERRED1,
	"Orange",		// SKINCOLOR_SUPERORANGE1,
	"Gold",			// SKINCOLOR_SUPERGOLD1,
	"Peridot",		// SKINCOLOR_SUPERPERIDOT1,
	"Sky",			// SKINCOLOR_SUPERSKY1,
	"Purple",		// SKINCOLOR_SUPERPURPLE1,
	"Rust",			// SKINCOLOR_SUPERRUST1,
	"Tan"			// SKINCOLOR_SUPERTAN1,
};

/*
A word of warning: If the following array is non-symmetrical,
A_SignPlayer's prefoppositecolor behaviour will break.
*/
// [0] = opposite skin color,
// [1] = shade index used by signpost, 0-15 (actual sprite frame is 15 minus this value)
const UINT8 Color_Opposite[MAXSKINCOLORS - 1][2] =
{
	// {SKINCOLOR_NONE, 8}, // SKINCOLOR_NONE

	// Greyscale ranges
	{SKINCOLOR_BLACK,   5}, // SKINCOLOR_WHITE,
	{SKINCOLOR_JET,     7}, // SKINCOLOR_BONE,
	{SKINCOLOR_CARBON,  7}, // SKINCOLOR_CLOUDY,
	{SKINCOLOR_AETHER, 12}, // SKINCOLOR_GREY,
	{SKINCOLOR_SLATE,  12}, // SKINCOLOR_SILVER,
	{SKINCOLOR_CLOUDY,  7}, // SKINCOLOR_CARBON,
	{SKINCOLOR_BONE,    7}, // SKINCOLOR_JET,
	{SKINCOLOR_WHITE,   7}, // SKINCOLOR_BLACK,

	// Desaturated
	{SKINCOLOR_GREY,   15}, // SKINCOLOR_AETHER,
	{SKINCOLOR_SILVER, 12}, // SKINCOLOR_SLATE,
	{SKINCOLOR_COPPER,  4}, // SKINCOLOR_BLUEBELL,
	{SKINCOLOR_AZURE,   9}, // SKINCOLOR_PINK,
	{SKINCOLOR_RUST,    7}, // SKINCOLOR_YOGURT,
	{SKINCOLOR_TAN,     2}, // SKINCOLOR_BROWN,
	{SKINCOLOR_KETCHUP, 0}, // SKINCOLOR_BRONZE,
	{SKINCOLOR_BROWN,  12}, // SKINCOLOR_TAN,
	{SKINCOLOR_MOSS,    5}, // SKINCOLOR_BEIGE,
	{SKINCOLOR_BEIGE,  13}, // SKINCOLOR_MOSS,
	{SKINCOLOR_PINK,    5}, // SKINCOLOR_AZURE,
	{SKINCOLOR_GOLD,    4}, // SKINCOLOR_LAVENDER,

	// Viv's vivid colours (toast 21/07/17)
	{SKINCOLOR_EMERALD,   10}, // SKINCOLOR_RUBY,
	{SKINCOLOR_FOREST,     6}, // SKINCOLOR_SALMON,
	{SKINCOLOR_GREEN,     10}, // SKINCOLOR_RED,
	{SKINCOLOR_ICY,       10}, // SKINCOLOR_CRIMSON,
	{SKINCOLOR_PURPLE,     8}, // SKINCOLOR_FLAME,
	{SKINCOLOR_BRONZE,     8}, // SKINCOLOR_KETCHUP,
	{SKINCOLOR_TEAL,       7}, // SKINCOLOR_PEACHY,
	{SKINCOLOR_WAVE,       5}, // SKINCOLOR_QUAIL,
	{SKINCOLOR_SAPPHIRE,   5}, // SKINCOLOR_SUNSET,
	{SKINCOLOR_BLUEBELL,   5}, // SKINCOLOR_COPPER
	{SKINCOLOR_CYAN,       4}, // SKINCOLOR_APRICOT,
	{SKINCOLOR_BLUE,       4}, // SKINCOLOR_ORANGE,
	{SKINCOLOR_YOGURT,     8}, // SKINCOLOR_RUST,
	{SKINCOLOR_LAVENDER,  10}, // SKINCOLOR_GOLD,
	{SKINCOLOR_SKY,        8}, // SKINCOLOR_SANDY,
	{SKINCOLOR_CORNFLOWER, 8}, // SKINCOLOR_YELLOW,
	{SKINCOLOR_DUSK,       3}, // SKINCOLOR_OLIVE,
	{SKINCOLOR_MAGENTA,    9}, // SKINCOLOR_LIME,
	{SKINCOLOR_COBALT,     2}, // SKINCOLOR_PERIDOT,
	{SKINCOLOR_RASPBERRY, 13}, // SKINCOLOR_APPLE,
	{SKINCOLOR_RED,        6}, // SKINCOLOR_GREEN,
	{SKINCOLOR_SALMON,     9}, // SKINCOLOR_FOREST,
	{SKINCOLOR_RUBY,       4}, // SKINCOLOR_EMERALD,
	{SKINCOLOR_VIOLET,     5}, // SKINCOLOR_MINT,
	{SKINCOLOR_PLUM,       6}, // SKINCOLOR_SEAFOAM,
	{SKINCOLOR_ROSY,       7}, // SKINCOLOR_AQUA,
	{SKINCOLOR_PEACHY,     7}, // SKINCOLOR_TEAL,
	{SKINCOLOR_QUAIL,      5}, // SKINCOLOR_WAVE,
	{SKINCOLOR_APRICOT,    6}, // SKINCOLOR_CYAN,
	{SKINCOLOR_SANDY,      1}, // SKINCOLOR_SKY,
	{SKINCOLOR_NEON,       4}, // SKINCOLOR_CERULEAN,
	{SKINCOLOR_CRIMSON,    0}, // SKINCOLOR_ICY,
	{SKINCOLOR_SUNSET,     5}, // SKINCOLOR_SAPPHIRE,
	{SKINCOLOR_YELLOW,     4}, // SKINCOLOR_CORNFLOWER,
	{SKINCOLOR_ORANGE,     5}, // SKINCOLOR_BLUE,
	{SKINCOLOR_PERIDOT,    5}, // SKINCOLOR_COBALT,
	{SKINCOLOR_LILAC,      4}, // SKINCOLOR_VAPOR,
	{SKINCOLOR_OLIVE,      0}, // SKINCOLOR_DUSK,
	{SKINCOLOR_BUBBLEGUM,  9}, // SKINCOLOR_PASTEL,
	{SKINCOLOR_FLAME,      7}, // SKINCOLOR_PURPLE,
	{SKINCOLOR_PASTEL,     8}, // SKINCOLOR_BUBBLEGUM,
	{SKINCOLOR_LIME,       6}, // SKINCOLOR_MAGENTA,
	{SKINCOLOR_CERULEAN,   2}, // SKINCOLOR_NEON,
	{SKINCOLOR_MINT,       6}, // SKINCOLOR_VIOLET,
	{SKINCOLOR_VAPOR,      4}, // SKINCOLOR_LILAC,
	{SKINCOLOR_MINT,       7}, // SKINCOLOR_PLUM,
	{SKINCOLOR_APPLE,     13}, // SKINCOLOR_RASPBERRY
	{SKINCOLOR_AQUA,       1}  // SKINCOLOR_ROSY,
};

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
static void R_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor)
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
		color = V_GetColor(Color_Index[skincolor-1][i]);
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
				dest_colormap[i] = Color_Index[skincolor-1][j];
			}
		}
	}
}

#undef SETBRIGHTNESS

static void R_GenerateTranslationColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color)
{
	INT32 i, starttranscolor, skinramplength;

	if (skinnum >= numskins || skinnum <= (TC_DEFAULT-7))
	{
		CONS_Alert(CONS_WARNING, "R_GenerateTranslationColormap called with invalid skinnum %d\n", skinnum);
		skinnum = TC_DEFAULT;
	}
		
	// Handle a couple of simple special cases
	if (skinnum < TC_DEFAULT)
	{
		switch (skinnum)
		{
			case TC_ALLWHITE:
				memset(dest_colormap, 0, NUM_PALETTE_ENTRIES * sizeof(UINT8));
				return;
			case TC_RAINBOW:
				if (color >= MAXTRANSLATIONS)
					I_Error("Invalid skin color #%hu.", (UINT16)color);
				if (color != SKINCOLOR_NONE)
				{
					R_RainbowColormap(dest_colormap, color);
					return;
				}
				break;
			case TC_BLINK:
				if (color >= MAXTRANSLATIONS)
					I_Error("Invalid skin color #%hu.", (UINT16)color);
				if (color != SKINCOLOR_NONE)
				{
					memset(dest_colormap, Color_Index[color-1][3], NUM_PALETTE_ENTRIES * sizeof(UINT8));
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
				dest_colormap[Color_Index[SKINCOLOR_BLUE-1][12-i]] = Color_Index[SKINCOLOR_BLUE-1][i];
			}
			dest_colormap[159] = dest_colormap[253] = dest_colormap[254] = 0;
			for (i = 0; i < 16; i++)
				dest_colormap[96+i] = dest_colormap[Color_Index[SKINCOLOR_COBALT-1][i]];
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

	if (color >= MAXTRANSLATIONS)
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
		dest_colormap[starttranscolor + i] = (UINT8)Color_Index[color-1][i];
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
	return SKINCOLOR_GREEN;
}

UINT8 R_GetSuperColorByName(const char *name)
{
	UINT8 color; /* = (UINT8)atoi(name); -- This isn't relevant to S_SKIN, which is the only way it's accessible right now. Let's simplify things.
	if (color > MAXSKINCOLORS && color < MAXTRANSLATIONS && !((color - MAXSKINCOLORS) % 5))
		return color;*/
	for (color = 0; color < NUMSUPERCOLORS; color++)
		if (!stricmp(Color_Names[color + MAXSKINCOLORS], name))
			return ((color*5) + MAXSKINCOLORS);
	return SKINCOLOR_SUPERGOLD1;
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
