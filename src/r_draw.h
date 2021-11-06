// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw.h
/// \brief Low-level span/column drawer functions

#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_defs.h"

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------
extern UINT8 *ylookup[MAXVIDHEIGHT*4];
extern UINT8 *ylookup1[MAXVIDHEIGHT*4];
extern UINT8 *ylookup2[MAXVIDHEIGHT*4];
extern INT32 columnofs[MAXVIDWIDTH*4];
extern UINT8 *topleft;

extern float focallengthf;

/// \brief Top border
#define BRDR_T 0
/// \brief Bottom border
#define BRDR_B 1
/// \brief Left border
#define BRDR_L 2
/// \brief Right border
#define BRDR_R 3
/// \brief Topleft border
#define BRDR_TL 4
/// \brief Topright border
#define BRDR_TR 5
/// \brief Bottomleft border
#define BRDR_BL 6
/// \brief Bottomright border
#define BRDR_BR 7

extern lumpnum_t viewborderlump[8];

// ------------------------------------------------
// r_draw.c COMMON ROUTINES FOR BOTH 8bpp and 16bpp
// ------------------------------------------------

#define GTC_CACHE 1

enum
{
	TC_BOSS       = INT8_MIN,
	TC_METALSONIC, // For Metal Sonic battle
	TC_ALLWHITE,   // For Cy-Brak-demon
	TC_RAINBOW,    // For single colour
	TC_BLINK,      // For item blinking, according to kart
	TC_DASHMODE,   // For Metal Sonic's dashmode

	TC_DEFAULT
};

// Custom player skin translation
// Initialize color translation tables, for player rendering etc.
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags);
void R_FlushTranslationColormapCache(void);
UINT16 R_GetColorByName(const char *name);
UINT16 R_GetSuperColorByName(const char *name);

extern UINT8 *transtables; // translucency tables, should be (*transtables)[5][256][256]

enum
{
	blendtab_add,
	blendtab_subtract,
	blendtab_reversesubtract,
	blendtab_modulate,
	NUMBLENDMAPS
};

extern UINT8 *blendtables[NUMBLENDMAPS];

void R_InitTranslucencyTables(void);
void R_GenerateBlendTables(void);

UINT8 *R_GetTranslucencyTable(INT32 alphalevel);
UINT8 *R_GetBlendTable(int style, INT32 alphalevel);

boolean R_BlendLevelVisible(INT32 blendmode, INT32 alphalevel);

// Color ramp modification should force a recache
extern UINT8 skincolor_modified[];

void R_InitViewBuffer(INT32 width, INT32 height);
void R_InitViewBorder(void);
void R_VideoErase(size_t ofs, INT32 count);

// Rendering function.
#if 0
void R_FillBackScreen(void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder(void);
#endif

#define TRANSPARENTPIXEL 255

typedef struct colcontext_s
{
	void (*func) (struct colcontext_s *);
	vbuffer_t *dest;

	INT32 x, yl, yh;
	fixed_t iscale, texturemid;
	lighttable_t *colormap;

	UINT8 *source; // first pixel in a column

	// translucency stuff here
	UINT8 *transmap;

	// translation stuff here
	UINT8 *translation;

	struct r_lightlist_s *lightlist;
	INT32 numlights, maxlights;

	//Fix TUTIFRUTI
	INT32 texheight;

	// vars for R_DrawMaskedColumn
	// Rum and Raisin put these on the sprite context.
	// I put them on the column context because it felt
	// more appropriate (for [REDACTED] anyway)
	INT16 *mfloorclip;
	INT16 *mceilingclip;

	fixed_t spryscale, sprtopscreen, sprbotscreen;
	fixed_t windowtop, windowbottom;

	// column->length : for flipped column function pointers and multi-patch on 2sided wall = texture->height
	INT32 lengthcol;
} colcontext_t;

typedef struct spancontext_s
{
	void (*func) (struct spancontext_s *);
	vbuffer_t *dest;

	INT32 y, x1, x2;
	lighttable_t *colormap;
	lighttable_t *translation;

	fixed_t xfrac, yfrac, xstep, ystep;
	INT32 waterofs, bgofs;

	UINT16 flatwidth, flatheight;
	boolean powersoftwo;

	UINT8 *source;
	UINT8 *transmap;

	// Vectors for Software's tilted slope drawers
	floatv3_t *su, *sv, *sz;
	floatv3_t *sup, *svp, *szp;
	floatv3_t slope_origin, slope_u, slope_v;
	float zeroheight;

	// Variable flat sizes
	UINT32 nflatxshift;
	UINT32 nflatyshift;
	UINT32 nflatshiftup;
	UINT32 nflatmask;

	lighttable_t **zlight;
	INT32 tiltlighting[MAXVIDWIDTH];
} spancontext_t;

typedef void (*colfunc_t) (colcontext_t *);
typedef void (*spanfunc_t) (spancontext_t *);

// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

#define BASEDRAWFUNC 0

enum
{
	COLDRAWFUNC_BASE = BASEDRAWFUNC,
	COLDRAWFUNC_FUZZY,
	COLDRAWFUNC_TRANS,
	COLDRAWFUNC_SHADE,
	COLDRAWFUNC_SHADOWED,
	COLDRAWFUNC_TRANSTRANS,
	COLDRAWFUNC_TWOSMULTIPATCH,
	COLDRAWFUNC_TWOSMULTIPATCHTRANS,
	COLDRAWFUNC_FOG,

	COLDRAWFUNC_MAX
};

extern colfunc_t colfuncs[COLDRAWFUNC_MAX];

enum
{
	SPANDRAWFUNC_BASE = BASEDRAWFUNC,
	SPANDRAWFUNC_TRANS,
	SPANDRAWFUNC_TILTED,
	SPANDRAWFUNC_TILTEDTRANS,

	SPANDRAWFUNC_SPLAT,
	SPANDRAWFUNC_TRANSSPLAT,
	SPANDRAWFUNC_TILTEDSPLAT,

	SPANDRAWFUNC_SPRITE,
	SPANDRAWFUNC_TRANSSPRITE,
	SPANDRAWFUNC_TILTEDSPRITE,
	SPANDRAWFUNC_TILTEDTRANSSPRITE,

	SPANDRAWFUNC_WATER,
	SPANDRAWFUNC_TILTEDWATER,

	SPANDRAWFUNC_FOG,

	SPANDRAWFUNC_MAX
};

extern spanfunc_t spanfuncs[SPANDRAWFUNC_MAX];
extern spanfunc_t spanfuncs_npo2[SPANDRAWFUNC_MAX];

// -----------------
// 8bpp DRAWING CODE
// -----------------

void R_DrawColumn_8(colcontext_t *dc);
void R_DrawShadeColumn_8(colcontext_t *dc);
void R_DrawTranslucentColumn_8(colcontext_t *dc);
void R_DrawTranslatedColumn_8(colcontext_t *dc);
void R_DrawTranslatedTranslucentColumn_8(colcontext_t *dc);
void R_Draw2sMultiPatchColumn_8(colcontext_t *dc);
void R_Draw2sMultiPatchTranslucentColumn_8(colcontext_t *dc);
void R_DrawFogColumn_8(colcontext_t *dc);
void R_DrawColumnShadowed_8(colcontext_t *dc);

void R_DrawSpan_8(spancontext_t *ds);
void R_DrawTranslucentSpan_8(spancontext_t *ds);
void R_DrawTiltedSpan_8(spancontext_t *ds);
void R_DrawTiltedTranslucentSpan_8(spancontext_t *ds);

void R_DrawSplat_8(spancontext_t *ds);
void R_DrawTranslucentSplat_8(spancontext_t *ds);
void R_DrawTiltedSplat_8(spancontext_t *ds);

void R_DrawFloorSprite_8(spancontext_t *ds);
void R_DrawTranslucentFloorSprite_8(spancontext_t *ds);
void R_DrawTiltedFloorSprite_8(spancontext_t *ds);
void R_DrawTiltedTranslucentFloorSprite_8(spancontext_t *ds);

void R_DrawTranslucentWaterSpan_8(spancontext_t *ds);
void R_DrawTiltedTranslucentWaterSpan_8(spancontext_t *ds);

void R_DrawFogSpan_8(spancontext_t *ds);

// Lactozilla: Non-powers-of-two
void R_DrawSpan_NPO2_8(spancontext_t *ds);
void R_DrawTranslucentSpan_NPO2_8(spancontext_t *ds);
void R_DrawTiltedSpan_NPO2_8(spancontext_t *ds);
void R_DrawTiltedTranslucentSpan_NPO2_8(spancontext_t *ds);

void R_DrawSplat_NPO2_8(spancontext_t *ds);
void R_DrawTranslucentSplat_NPO2_8(spancontext_t *ds);
void R_DrawTiltedSplat_NPO2_8(spancontext_t *ds);

void R_DrawFloorSprite_NPO2_8(spancontext_t *ds);
void R_DrawTranslucentFloorSprite_NPO2_8(spancontext_t *ds);
void R_DrawTiltedFloorSprite_NPO2_8(spancontext_t *ds);
void R_DrawTiltedTranslucentFloorSprite_NPO2_8(spancontext_t *ds);

void R_DrawTranslucentWaterSpan_NPO2_8(spancontext_t *ds);
void R_DrawTiltedTranslucentWaterSpan_NPO2_8(spancontext_t *ds);

// =========================================================================
#endif  // __R_DRAW__
