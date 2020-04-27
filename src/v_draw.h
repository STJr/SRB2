// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  v_draw.h
/// \brief Functions to draw patches (by post) directly to screen.

#ifndef __V_DRAW__
#define __V_DRAW__

#include "doomdef.h"
#include "doomtype.h"

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
void V_DrawCroppedPatch(fixed_t x, fixed_t y, fixed_t pscale, INT32 scrn, patch_t *patch, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);

void V_DrawContinueIcon(INT32 x, INT32 y, INT32 flags, INT32 skinnum, UINT8 skincolor);

// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(INT32 x, INT32 y, INT32 scrn, INT32 width, INT32 height, const UINT8 *src);

// draw a pic_t, SCALED
void V_DrawScaledPic (INT32 px1, INT32 py1, INT32 scrn, INT32 lumpnum);

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

// draw a single character
void V_DrawCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed);
// draw a single character, but for the chat
void V_DrawChatCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed, UINT8 *colormap);

UINT8 *V_GetStringColormap(INT32 colorflags);

void V_DrawLevelTitle(INT32 x, INT32 y, INT32 option, const char *string);

// wordwrap a string using the hu_font
char *V_WordWrap(INT32 x, INT32 w, INT32 option, const char *string);
UINT8 *V_GetStringColormap(INT32 colorflags);

// draw a string using the hu_font
void V_DrawString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawCenteredString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedString(INT32 x, INT32 y, INT32 option, const char *string);

// draw a string using the hu_font, 0.5x scale
void V_DrawSmallString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawCenteredSmallString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedSmallString(INT32 x, INT32 y, INT32 option, const char *string);

// draw a string using the tny_font
void V_DrawThinString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawCenteredThinString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedThinString(INT32 x, INT32 y, INT32 option, const char *string);

// draw a string using the tny_font, 0.5x scale
void V_DrawSmallThinString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawCenteredSmallThinString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedSmallThinString(INT32 x, INT32 y, INT32 option, const char *string);

// draw a string using the hu_font at fixed_t coordinates
void V_DrawStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawCenteredStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawRightAlignedStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);

// draw a string using the hu_font at fixed_t coordinates, 0.5x scale
void V_DrawSmallStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawCenteredSmallStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawRightAlignedSmallStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);

// draw a string using the tny_font at fixed_t coordinates
void V_DrawThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawCenteredThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawRightAlignedThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);

// draw a string using the tny_font at fixed_t coordinates, 0.5x scale
void V_DrawSmallThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawCenteredSmallThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawRightAlignedSmallThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);

// Draw tall nums, used for menu, HUD, intermission
void V_DrawTallNum(INT32 x, INT32 y, INT32 flags, INT32 num);
void V_DrawPaddedTallNum(INT32 x, INT32 y, INT32 flags, INT32 num, INT32 digits);
void V_DrawLevelActNum(INT32 x, INT32 y, INT32 flags, INT32 num);

// Find string width from lt_font chars
INT32 V_LevelNameWidth(const char *string);
INT32 V_LevelNameHeight(const char *string);
INT32 V_LevelActNumWidth(INT32 num); // act number width

void V_DrawCreditString(fixed_t x, fixed_t y, INT32 option, const char *string);
INT32 V_CreditStringWidth(const char *string);

// Draw a string using the nt_font
void V_DrawNameTag(INT32 x, INT32 y, INT32 option, fixed_t scale, UINT8 *basecolormap, UINT8 *outlinecolormap, const char *string);
INT32 V_CountNameTagLines(const char *string);
INT32 V_NameTagWidth(const char *string);

// Find string width from hu_font chars
INT32 V_StringWidth(const char *string, INT32 option);
// Find string width from hu_font chars, 0.5x scale
INT32 V_SmallStringWidth(const char *string, INT32 option);
// Find string width from tny_font chars
INT32 V_ThinStringWidth(const char *string, INT32 option);
// Find string width from tny_font chars, 0.5x scale
INT32 V_SmallThinStringWidth(const char *string, INT32 option);

void V_DrawPatchFill(patch_t *pat);
void V_BlitScaledPic(INT32 px1, INT32 py1, INT32 scrn, pic_t *pic);

#endif
