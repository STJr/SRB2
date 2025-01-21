// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  v_video.h
/// \brief Gamma correction LUT

#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomdef.h"
#include "doomtype.h"
#include "r_defs.h"
#include "hu_stuff.h" //font arrays

//
// VIDEO
//

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

extern UINT8 *screens[5];

extern consvar_t cv_ticrate, cv_constextsize,
cv_globalgamma, cv_globalsaturation,
cv_rhue, cv_yhue, cv_ghue, cv_chue, cv_bhue, cv_mhue,
cv_rgamma, cv_ygamma, cv_ggamma, cv_cgamma, cv_bgamma, cv_mgamma,
cv_rsaturation, cv_ysaturation, cv_gsaturation, cv_csaturation, cv_bsaturation, cv_msaturation;

// Allocates buffer screens, call before R_Init.
void V_Init(void);

// Recalculates the viddef (dup, fdup, etc.) according to the current screen resolution.
void V_Recalc(void);

// Color look-up table
#define CLUTINDEX(r, g, b) (((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)

typedef struct
{
	boolean init;
	RGBA_t palette[256];
	UINT16 table[0xFFFF];
} colorlookup_t;

void InitColorLUT(colorlookup_t *lut, RGBA_t *palette, boolean makecolors);
UINT8 GetColorLUT(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b);
UINT8 GetColorLUTDirect(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b);

// Set the current RGB palette lookup to use for palettized graphics
void V_SetPalette(INT32 palettenum);

void V_SetPaletteLump(const char *pal);

const char *R_GetPalname(UINT16 num);
const char *GetPalette(void);

extern RGBA_t *pLocalPalette;
extern RGBA_t *pMasterPalette;

void V_CubeApply(UINT8 *red, UINT8 *green, UINT8 *blue);

// Retrieve the ARGB value from a palette color index
#define V_GetColor(color) (pLocalPalette[color&0xFF])
#define V_GetMasterColor(color) (pMasterPalette[color&0xFF])

// Bottom 8 bits are used for parameter (screen or character)
#define V_PARAMMASK          0x000000FF

// flags hacked in scrn (not supported by all functions (see src))
// patch scaling uses bits 9 and 10
#define V_SCALEPATCHSHIFT    8
#define V_SCALEPATCHMASK     0x00000300
#define V_NOSCALEPATCH       0x00000100
#define V_SMALLSCALEPATCH    0x00000200
#define V_MEDSCALEPATCH      0x00000300

// string spacing uses bits 11 and 12
#define V_SPACINGMASK        0x00000C00
#define V_6WIDTHSPACE        0x00000400 // early 2.1 style spacing, variable widths, 6 character space
#define V_OLDSPACING         0x00000800 // Old style spacing, 8 per character 4 per space
#define V_MONOSPACE          0x00000C00 // Don't do width checks on characters, all characters 8 width

// use bits 13-16 for colors
// though we only have 7 colors now, perhaps we can introduce
// more as needed later
#define V_CHARCOLORSHIFT     12
#define V_CHARCOLORMASK      0x0000F000
// for simplicity's sake, shortcuts to specific colors
#define V_MAGENTAMAP         0x00001000
#define V_YELLOWMAP          0x00002000
#define V_GREENMAP           0x00003000
#define V_BLUEMAP            0x00004000
#define V_REDMAP             0x00005000
#define V_GRAYMAP            0x00006000
#define V_ORANGEMAP          0x00007000
#define V_SKYMAP             0x00008000
#define V_PURPLEMAP          0x00009000
#define V_AQUAMAP            0x0000A000
#define V_PERIDOTMAP         0x0000B000
#define V_AZUREMAP           0x0000C000
#define V_BROWNMAP           0x0000D000
#define V_ROSYMAP            0x0000E000
#define V_INVERTMAP          0x0000F000

// use bits 17-20 for alpha transparency
#define V_ALPHASHIFT         16
#define V_ALPHAMASK          0x000F0000
// define specific translucencies
#define V_10TRANS            0x00010000
#define V_20TRANS            0x00020000
#define V_30TRANS            0x00030000
#define V_40TRANS            0x00040000
#define V_TRANSLUCENT        0x00050000 // TRANS50
#define V_60TRANS            0x00060000
#define V_70TRANS            0x00070000
#define V_80TRANS            0x00080000 // used to be V_8020TRANS
#define V_90TRANS            0x00090000
#define V_HUDTRANSHALF       0x000A0000
#define V_HUDTRANS           0x000B0000 // draw the hud translucent
#define V_HUDTRANSDOUBLE     0x000C0000
// Macros follow
#define V_USERHUDTRANSHALF   ((10-(cv_translucenthud.value/2))<<V_ALPHASHIFT)
#define V_USERHUDTRANS       ((10-cv_translucenthud.value)<<V_ALPHASHIFT)
#define V_USERHUDTRANSDOUBLE ((10-min(cv_translucenthud.value*2, 10))<<V_ALPHASHIFT)

// use bits 21-23 for blendmodes
#define V_BLENDSHIFT         20
#define V_BLENDMASK          0x00700000
// preshifted blend flags minus 1 as effects don't distinguish between AST_COPY and AST_TRANSLUCENT
#define V_ADD                ((AST_ADD-1)<<V_BLENDSHIFT) // Additive
#define V_SUBTRACT           ((AST_SUBTRACT-1)<<V_BLENDSHIFT) // Subtractive
#define V_REVERSESUBTRACT    ((AST_REVERSESUBTRACT-1)<<V_BLENDSHIFT) // Reverse subtractive
#define V_MODULATE           ((AST_MODULATE-1)<<V_BLENDSHIFT) // Modulate

#define V_ALLOWLOWERCASE     0x00800000 // (strings only) allow fonts that have lowercase letters to use them
#define V_FLIP               0x00800000 // (patches only) Horizontal flip
#define V_CENTERNAMETAG      0x00800000 // (nametag only) center nametag lines

#define V_SNAPTOTOP          0x01000000 // for centering
#define V_SNAPTOBOTTOM       0x02000000 // for centering
#define V_SNAPTOLEFT         0x04000000 // for centering
#define V_SNAPTORIGHT        0x08000000 // for centering

#define V_AUTOFADEOUT        0x10000000 // used by CECHOs, automatic fade out when almost over
#define V_RETURN8            0x20000000 // 8 pixel return instead of 12

#define V_NOSCALESTART       0x40000000 // don't scale x, y, start coords
#define V_PERPLAYER          0x80000000 // automatically adjust coordinates/scaling for splitscreen mode

// defines for old functions
#define V_DrawPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s|V_NOSCALESTART|V_NOSCALEPATCH, p, NULL)
#define V_DrawTranslucentMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s, p, c)
#define V_DrawSmallTranslucentMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, c)
#define V_DrawTinyTranslucentMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, c)
#define V_DrawMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s, p, c)
#define V_DrawSmallMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, c)
#define V_DrawTinyMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, c)
#define V_DrawScaledPatch(x,y,s,p) V_DrawFixedPatch((x)*FRACUNIT, (y)<<FRACBITS, FRACUNIT, s, p, NULL)
#define V_DrawSmallScaledPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, NULL)
#define V_DrawTinyScaledPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, NULL)
#define V_DrawTranslucentPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s, p, NULL)
#define V_DrawSmallTranslucentPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, NULL)
#define V_DrawTinyTranslucentPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, NULL)
#define V_DrawSciencePatch(x,y,s,p,sc) V_DrawFixedPatch(x,y,sc,s,p,NULL)
#define V_DrawFixedPatch(x,y,sc,s,p,c) V_DrawStretchyFixedPatch(x,y,sc,sc,s,p,c)
void V_DrawStretchyFixedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap);
void V_DrawCroppedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);

void V_DrawContinueIcon(INT32 x, INT32 y, INT32 flags, INT32 skinnum, UINT16 skincolor);

// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(INT32 x, INT32 y, INT32 scrn, INT32 width, INT32 height, const UINT8 *src);

// fill a box with a single color
void V_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c);
void V_DrawFillConsoleMap(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c);
// fill a box with a flat as a pattern
void V_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatnum);

// fade down the screen buffer before drawing the menu over
void V_DrawFadeScreen(UINT16 color, UINT8 strength);
// available to lua over my dead body, which will probably happen in this heat
void V_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c, UINT16 color, UINT8 strength);

void V_DrawFadeConsBack(INT32 plines);
void V_DrawPromptBack(INT32 boxheight, INT32 color);
UINT8 *V_GetStringColormap(INT32 colorflags);

// Generalized character drawing function, combining console & chat functionality with a specified font.
void V_DrawFontCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed, fixed_t scale, UINT8 *colormap, fontdef_t font);
#define V_DrawCharacter(x,y,c,l) V_DrawFontCharacter(x,y,c,l,FRACUNIT,NULL,hu_font)
#define V_DrawChatCharacter(x,y,c,l,cm) V_DrawFontCharacter(x,y,c,l,FRACUNIT/2,cm,hu_font)

// Precompile a wordwrapped string to any given width, using a specified font.
char *V_FontWordWrap(INT32 x, INT32 w, INT32 option, fixed_t scale, const char *string, fontdef_t font);
#define V_WordWrap(x,w,o,str) V_FontWordWrap(x, w, o, FRACUNIT, str, hu_font)
#define V_ChatWordWrap(x,w,o,str) V_FontWordWrap(x, w, o, FRACUNIT/2, str, hu_font)

enum string_align {
	alignleft = 0,
	aligncenter,
	alignright
};

// Draw a string, using a supplied font and scale.
void V_DrawFontString(INT32 x, INT32 y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font);
void V_DrawAlignedFontString(INT32 x, INT32 y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font, enum string_align align);
// Draw a string, using a supplied font and scale, at fixed_t coordinates.
void V_DrawFontStringAtFixed(fixed_t x, fixed_t y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font);
void V_DrawAlignedFontStringAtFixed(fixed_t x, fixed_t y, INT32 option, fixed_t pscale, fixed_t vscale, const char *string, fontdef_t font, enum string_align align);

// Defines for old string drawers.
// draw a string using the hu_font
#define V_DrawString(x,y,o,str) V_DrawFontString(x,y,o,FRACUNIT,FRACUNIT,str,hu_font)
#define V_DrawCenteredString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT,FRACUNIT,str,hu_font,aligncenter)
#define V_DrawRightAlignedString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT,FRACUNIT,str,hu_font,alignright)
// draw a string using the hu_font, 0.5x scale
#define V_DrawSmallString(x,y,o,str) V_DrawFontString(x,y,o,FRACUNIT/2,FRACUNIT/2,str,hu_font)
#define V_DrawCenteredSmallString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT/2,FRACUNIT/2,str,hu_font,aligncenter)
#define V_DrawRightAlignedSmallString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT/2,FRACUNIT/2,str,hu_font,alignright)
// Write a string using the tny_font
#define V_DrawThinString(x,y,o,str) V_DrawFontString(x,y,o,FRACUNIT,FRACUNIT,str,tny_font)
#define V_DrawCenteredThinString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT,FRACUNIT,str,tny_font,aligncenter)
#define V_DrawRightAlignedThinString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT,FRACUNIT,str,tny_font,alignright)
// draw a string using the tny_font, 0.5x scale
#define V_DrawSmallThinString(x,y,o,str) V_DrawFontString(x,y,o,FRACUNIT/2,FRACUNIT/2,str,tny_font)
#define V_DrawCenteredSmallThinString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT/2,FRACUNIT/2,str,tny_font,aligncenter)
#define V_DrawRightAlignedSmallThinString(x,y,o,str) V_DrawAlignedFontString(x,y,o,FRACUNIT/2,FRACUNIT/2,str,tny_font,alignright)
// draw a string using the hu_font at fixed_t coordinates
#define V_DrawStringAtFixed(x,y,o,str) V_DrawFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,hu_font)
#define V_DrawCenteredStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,hu_font,aligncenter)
#define V_DrawRightAlignedStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,hu_font,alignright)
// draw a string using the hu_font at fixed_t coordinates, 0.5x scale
#define V_DrawSmallStringAtFixed(x,y,o,str) V_DrawFontStringAtFixed(x,y,o,FRACUNIT/2,FRACUNIT/2,str,hu_font)
#define V_DrawCenteredSmallStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT/2,FRACUNIT/2,str,hu_font,aligncenter)
#define V_DrawRightAlignedSmallStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT/2,FRACUNIT/2,str,hu_font,alignright)
// draw a string using the tny_font at fixed_t coordinates
#define V_DrawThinStringAtFixed(x,y,o,str) V_DrawFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,tny_font)
#define V_DrawCenteredThinStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,tny_font,aligncenter)
#define V_DrawRightAlignedThinStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,tny_font,alignright)
// draw a string using the tny_font at fixed_t coordinates, 0.5x scale
#define V_DrawSmallThinStringAtFixed(x,y,o,str) V_DrawFontStringAtFixed(x,y,o,FRACUNIT/2,FRACUNIT/2,str,tny_font)
#define V_DrawCenteredSmallThinStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT/2,FRACUNIT/2,str,tny_font,aligncenter)
#define V_DrawRightAlignedSmallThinStringAtFixed(x,y,o,str) V_DrawAlignedFontStringAtFixed(x,y,o,FRACUNIT/2,FRACUNIT/2,str,tny_font,alignright)
// draw a string using the credit font
#define V_DrawCreditString(x,y,o,str) V_DrawFontStringAtFixed(x,y,o,FRACUNIT,FRACUNIT,str,cred_font)
// draw a string using the level title font
#define V_DrawLevelTitle(x,y,o,str) V_DrawFontString(x,y,o|V_ALLOWLOWERCASE,FRACUNIT,FRACUNIT,str,lt_font)

// Draw tall nums, used for menu, HUD, intermission
void V_DrawTallNum(INT32 x, INT32 y, INT32 flags, INT32 num);
void V_DrawPaddedTallNum(INT32 x, INT32 y, INT32 flags, INT32 num, INT32 digits);
void V_DrawLevelActNum(INT32 x, INT32 y, INT32 flags, UINT8 num);
INT16 V_LevelActNumWidth(UINT8 num); // act number width

// Draw a string using the nt_font
void V_DrawNameTag(INT32 x, INT32 y, INT32 option, fixed_t scale, UINT8 *basecolormap, UINT8 *outlinecolormap, const char *string);
INT32 V_CountNameTagLines(const char *string);

// Find string width or height from supplied font chars
INT32 V_FontStringWidth(const char *string, INT32 option, fontdef_t font);
INT32 V_FontStringHeight(const char *string, INT32 option, fontdef_t font);

// Defines for old string width functions.
#define V_StringWidth(str,o) V_FontStringWidth(str,o,hu_font)
#define V_SmallStringWidth(str,o) V_FontStringWidth(str,o,hu_font)/2
#define V_ThinStringWidth(str,o) V_FontStringWidth(str,o,tny_font)
#define V_SmallThinStringWidth(str,o) V_FontStringWidth(str,o,tny_font)/2
#define V_CreditStringWidth(str) V_FontStringWidth(str,0,cred_font)
#define V_NameTagWidth(str) V_FontStringWidth(str,0,ntb_font)
#define V_LevelNameWidth(str) V_FontStringWidth(str,V_ALLOWLOWERCASE,lt_font)
#define V_LevelNameHeight(str) V_FontStringHeight(str,0,lt_font)
#define V_StringHeight(str,o) V_FontStringHeight(str,o,hu_font)

void V_DoPostProcessor(INT32 view, postimg_t type, INT32 param);

void V_DrawPatchFill(patch_t *pat);

void VID_BlitLinearScreen(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes);

#endif
