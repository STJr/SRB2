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

// ----------------------
// COMMON STUFF FOR 32bpp
// ----------------------
extern boolean tc_colormaps;
extern boolean tc_spritecolormaps;

extern UINT32 *topleft_u32;

extern UINT8 dp_lighting;
extern extracolormap_t *dp_extracolormap;
extern extracolormap_t *defaultextracolormap;

// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

extern lighttable_t *dc_colormap;
extern INT32 dc_x, dc_yl, dc_yh;
extern fixed_t dc_iscale, dc_texturemid;
extern UINT8 dc_hires;
extern UINT8 dc_picfmt;
extern UINT8 dc_colmapstyle;

extern UINT8 *dc_source; // first pixel in a column

// translucency stuff here
extern UINT8 *dc_transmap;
extern UINT8 dc_alpha;

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
extern lighttable_t *ds_translation;

extern fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;
extern INT32 ds_waterofs, ds_bgofs;

extern UINT16 ds_flatwidth, ds_flatheight;
extern boolean ds_powersoftwo, ds_solidcolor;

extern UINT8 *ds_source;
extern UINT8 *ds_transmap;
extern UINT8 ds_alpha;
extern UINT8 ds_picfmt;
extern UINT8 ds_colmapstyle;

typedef struct {
	float x, y, z;
} floatv3_t;

// Vectors for Software's tilted slope drawers
extern floatv3_t *ds_su, *ds_sv, *ds_sz;
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

// ------------------------
// r_draw.c COMMON ROUTINES
// ------------------------

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
extern boolean usetranstables;

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
UINT8 R_BlendModeTransnumToAlpha(int style, INT32 num);

INT32 R_AlphaToTransnum(UINT8 alpha);
UINT8 R_TransnumToAlpha(INT32 num);

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

#define COLFUNCLIST8(X) \
	X(TRANSLUCENT, Translucent); \
	X(MAPPED, Translated); \
	X(SHADE, Shade); \
	X(LIGHTLIST, Shadowed); \
	X(MAPPED_TRANSLUCENT, TranslatedTranslucent); \
	X(MULTIPATCH, 2sMultiPatch); \
	X(MULTIPATCH_TRANSLUCENT, 2sMultiPatchTranslucent)

#define COLFUNCLIST8_NOTEXTURE(X) \
	X(FOG, Fog); \
	X(DROP_SHADOW, DropShadow)

#define SPANFUNCLIST8(X) \
	X(TRANSLUCENT, TranslucentSpan); \
	X(TILTED, TiltedSpan); \
	X(TILTED_TRANSLUCENT, TiltedTranslucentSpan); \
	X(SPLAT, Splat); \
	X(SPLAT_TRANSLUCENT, TranslucentSplat); \
	X(SPLAT_TILTED, TiltedSplat); \
	X(SPRITE, FloorSprite); \
	X(SPRITE_TRANSLUCENT, TranslucentFloorSprite); \
	X(SPRITE_TILTED, TiltedFloorSprite); \
	X(SPRITE_TILTED_TRANSLUCENT, TiltedTranslucentFloorSprite); \
	X(WATER, WaterSpan); \
	X(WATER_TILTED, TiltedWaterSpan); \

#define SPANFUNCLIST8_NOTEXTURE(X) \
	X(FOG, FogSpan); \
	X(FOG_TILTED, TiltedFogSpan); \
	X(SOLIDCOLOR, SolidColorSpan); \
	X(SOLIDCOLOR_TILTED, TiltedSolidColorSpan); \
	X(SOLIDCOLOR_TRANSLUCENT, TranslucentSolidColorSpan); \
	X(SOLIDCOLOR_TILTED_TRANSLUCENT, TiltedTranslucentSolidColorSpan); \
	X(SOLIDCOLOR_WATER, WaterSolidColorSpan); \
	X(SOLIDCOLOR_TILTED_WATER, TiltedWaterSolidColorSpan)

#define COLFUNCLIST32(X) COLFUNCLIST8(X)

#define COLFUNCLIST32_NOTEXTURE(X) COLFUNCLIST8_NOTEXTURE(X)

#define SPANFUNCLIST32(X) SPANFUNCLIST8(X)

#define SPANFUNCLIST32_NOTEXTURE(X) SPANFUNCLIST8_NOTEXTURE(X)

// -----------------
// 8bpp DRAWING CODE
// -----------------

void R_DrawColumn_8(void);

#define COLFUNC8(type, func) void R_Draw##func##Column_8(void)
	COLFUNCLIST8(COLFUNC8);
	COLFUNCLIST8_NOTEXTURE(COLFUNC8);
#undef COLFUNC8

void R_DrawSpan_8(void);

#define SPANFUNC8(type, func) void R_Draw##func##_8(void)
	SPANFUNCLIST8(SPANFUNC8);
	SPANFUNCLIST8_NOTEXTURE(SPANFUNC8);
#undef SPANFUNC8

void R_DrawSpan_NPO2_8(void);

#define SPANFUNC8(type, func) void R_Draw##func##_NPO2_8(void)
	SPANFUNCLIST8(SPANFUNC8);
#undef SPANFUNC8

#define PLANELIGHTFLOAT (BASEVIDWIDTH * BASEVIDWIDTH / vid.width / zeroheight / 21.0f * FIXED_TO_FLOAT(fovtan))
#define SPANSIZE 16
#define INVSPAN 0.0625f // (1.0f / 16.0f)

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

// ------------------
// 32bpp DRAWING CODE
// ------------------

void R_DrawColumn_32(void);

#define COLFUNC32(type, func) void R_Draw##func##Column_32(void)
	COLFUNCLIST32(COLFUNC32);
	COLFUNCLIST32_NOTEXTURE(COLFUNC32);
#undef COLFUNC32

void R_DrawSpan_32(void);

#define SPANFUNC32(type, func) void R_Draw##func##_32(void)
	SPANFUNCLIST32(SPANFUNC32);
	SPANFUNCLIST32_NOTEXTURE(SPANFUNC32);
#undef SPANFUNC32

void R_DrawSpan_NPO2_32(void);

#define SPANFUNC32(type, func) void R_Draw##func##_NPO2_32(void)
	SPANFUNCLIST32(SPANFUNC32);
#undef SPANFUNC32

// ----------
// COLOR MATH
// ----------

#define MIX_ALPHA(a) (0xFF-(a))

#define R_TranslucentMix(bg, fg, alpha) \
	((alpha) == 0) ? (bg) : ( ((alpha)==0xFF) ? (fg) \
	:( ( (R_GetRgbaR(bg) * MIX_ALPHA(alpha)) + (R_GetRgbaR(fg) * (alpha)) ) >> 8) \
	|( ( (R_GetRgbaG(bg) * MIX_ALPHA(alpha)) + (R_GetRgbaG(fg) * (alpha)) ) >> 8) << 8 \
	|( ( (R_GetRgbaB(bg) * MIX_ALPHA(alpha)) + (R_GetRgbaB(fg) * (alpha)) ) >> 8) << 16 \
	|( ( (R_GetRgbaA(bg) * MIX_ALPHA(alpha)) + (R_GetRgbaA(fg) * (alpha)) ) >> 8) << 24)

extern UINT32 (*R_BlendModeMix)(UINT32, UINT32, UINT8);
extern UINT8 (*R_AlphaBlend)(UINT8, UINT8, UINT8 *);

UINT8 R_AlphaBlend_8(UINT8 src, UINT8 alpha, UINT8 *dest);
UINT8 R_TransTabBlend_8(UINT8 src, UINT8 alpha, UINT8 *dest);

void R_SetColumnBlendingFunction(INT32 blendmode);
void R_SetSpanBlendingFunction(INT32 blendmode);
void R_InitAlphaLUT(void);

FUNCMATH UINT32 TC_TintTrueColor(RGBA_t rgba, UINT32 blendcolor, UINT8 tintamt);
void TC_ClearMixCache(void);

#define TC_CalcScaleLight(light_p) (((scalelight_u32[0][0] - light_p) / 256) * 8)
#define TC_CalcScaleLightPaletted(light_p) (((scalelight[0][0] - light_p) / 256) * 8)

enum
{
	TC_COLORMAPSTYLE_8BPP,
	TC_COLORMAPSTYLE_32BPP
};

#define GetTrueColor(c) ((st_palette > 0) ? V_GetPalNumColor(c,st_palette) : V_GetColor(c)).rgba

// =========================================================================
#endif  // __R_DRAW__
