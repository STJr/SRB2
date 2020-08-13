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

// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

extern lighttable_t *dc_colormap;
extern INT32 dc_x, dc_yl, dc_yh;
extern fixed_t dc_iscale, dc_texturemid;
extern UINT8 dc_hires;

extern UINT8 *dc_source; // first pixel in a column

// translucency stuff here
extern UINT8 *transtables; // translucency tables, should be (*transtables)[5][256][256]
extern UINT8 *dc_transmap;

// translation stuff here

extern UINT8 *dc_translation;

extern struct r_lightlist_s *dc_lightlist;
extern INT32 dc_numlights, dc_maxlights;

//Fix TUTIFRUTI
extern INT32 dc_texheight;

// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

extern INT32 ds_y, ds_x1, ds_x2;
extern lighttable_t *ds_colormap;
extern fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;
extern UINT16 ds_flatwidth, ds_flatheight;
extern boolean ds_powersoftwo;
extern UINT8 *ds_source;
extern UINT8 *ds_transmap;

typedef struct {
	float x, y, z;
} floatv3_t;

extern pslope_t *ds_slope; // Current slope being used
extern floatv3_t ds_su[MAXVIDHEIGHT], ds_sv[MAXVIDHEIGHT], ds_sz[MAXVIDHEIGHT]; // Vectors for... stuff?
extern floatv3_t *ds_sup, *ds_svp, *ds_szp;
extern float focallengthf, zeroheight;

// Variable flat sizes
extern UINT32 nflatxshift;
extern UINT32 nflatyshift;
extern UINT32 nflatshiftup;
extern UINT32 nflatmask;

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

#define TC_DEFAULT    -1
#define TC_BOSS       -2
#define TC_METALSONIC -3 // For Metal Sonic battle
#define TC_ALLWHITE   -4 // For Cy-Brak-demon
#define TC_RAINBOW    -5 // For single colour
#define TC_BLINK      -6 // For item blinking, according to kart
#define TC_DASHMODE   -7 // For Metal Sonic's dashmode

// Initialize color translation tables, for player rendering etc.
void R_InitTranslationTables(void);
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags);
void R_FlushTranslationColormapCache(void);
UINT16 R_GetColorByName(const char *name);
UINT16 R_GetSuperColorByName(const char *name);

// Color ramp modification should force a recache
extern UINT8 skincolor_modified[];

// Custom player skin translation
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

// -----------------
// 8bpp DRAWING CODE
// -----------------

void R_DrawColumn_8(void);
void R_DrawShadeColumn_8(void);
void R_DrawTranslucentColumn_8(void);
void R_DrawTranslatedColumn_8(void);
void R_DrawTranslatedTranslucentColumn_8(void);
void R_Draw2sMultiPatchColumn_8(void);
void R_Draw2sMultiPatchTranslucentColumn_8(void);
void R_DrawFogColumn_8(void);
void R_DrawColumnShadowed_8(void);

void R_DrawSpan_8(void);
void R_DrawSplat_8(void);
void R_DrawTranslucentSpan_8(void);
void R_DrawTranslucentSplat_8(void);
void R_DrawTiltedSpan_8(void);
void R_DrawTiltedTranslucentSpan_8(void);
#ifndef NOWATER
void R_DrawTiltedTranslucentWaterSpan_8(void);
#endif
void R_DrawTiltedSplat_8(void);
void R_CalcTiltedLighting(fixed_t start, fixed_t end);
extern INT32 tiltlighting[MAXVIDWIDTH];
#ifndef NOWATER
void R_DrawTranslucentWaterSpan_8(void);
extern INT32 ds_bgofs;
extern INT32 ds_waterofs;
#endif
void R_DrawFogSpan_8(void);

// Lactozilla: Non-powers-of-two
void R_DrawSpan_NPO2_8(void);
void R_DrawTranslucentSpan_NPO2_8(void);
void R_DrawSplat_NPO2_8(void);
void R_DrawTranslucentSplat_NPO2_8(void);
void R_DrawTiltedSpan_NPO2_8(void);
void R_DrawTiltedTranslucentSpan_NPO2_8(void);
#ifndef NOWATER
void R_DrawTiltedTranslucentWaterSpan_NPO2_8(void);
#endif
void R_DrawTiltedSplat_NPO2_8(void);
#ifndef NOWATER
void R_DrawTranslucentWaterSpan_NPO2_8(void);
#endif

#ifdef USEASM
void ASMCALL R_DrawColumn_8_ASM(void);
void ASMCALL R_DrawShadeColumn_8_ASM(void);
void ASMCALL R_DrawTranslucentColumn_8_ASM(void);
void ASMCALL R_Draw2sMultiPatchColumn_8_ASM(void);

void ASMCALL R_DrawColumn_8_MMX(void);

void ASMCALL R_Draw2sMultiPatchColumn_8_MMX(void);
void ASMCALL R_DrawSpan_8_MMX(void);
#endif

// ------------------
// 16bpp DRAWING CODE
// ------------------

#ifdef HIGHCOLOR
void R_DrawColumn_16(void);
void R_DrawWallColumn_16(void);
void R_DrawTranslucentColumn_16(void);
void R_DrawTranslatedColumn_16(void);
void R_DrawSpan_16(void);
#endif

// =========================================================================
#endif  // __R_DRAW__
