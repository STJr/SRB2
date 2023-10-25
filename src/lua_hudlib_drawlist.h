// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hudlib_drawlist.h
/// \brief a data structure for managing cached drawlists for the Lua hud lib

// The idea behinds this module is to cache drawcall information into an ordered
// list to repeat the same draw operations in later frames. It's used to ensure
// that the HUD hooks from Lua are called at precisely 35hz to avoid problems
// with variable framerates in existing Lua addons.

#ifndef __LUA_HUDLIB_DRAWLIST__
#define __LUA_HUDLIB_DRAWLIST__

#include "doomtype.h"
#include "r_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct huddrawlist_s *huddrawlist_h;

// Create a new drawlist. Returns a handle to it.
huddrawlist_h LUA_HUD_CreateDrawList(void);
// Clears the draw list.
void LUA_HUD_ClearDrawList(huddrawlist_h list);
// Destroys the drawlist, invalidating the given handle
void LUA_HUD_DestroyDrawList(huddrawlist_h list);
boolean LUA_HUD_IsDrawListValid(huddrawlist_h list);

void LUA_HUD_AddDraw(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap
);
void LUA_HUD_AddDrawScaled(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	fixed_t scale,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap
);
void LUA_HUD_AddDrawStretched(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	fixed_t hscale,
	fixed_t vscale,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap
);
void LUA_HUD_AddDrawCropped(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	fixed_t hscale,
	fixed_t vscale,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap,
	fixed_t sx,
	fixed_t sy,
	fixed_t w,
	fixed_t h
);
void LUA_HUD_AddDrawNum(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	INT32 num,
	INT32 flags
);
void LUA_HUD_AddDrawPaddedNum(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	INT32 num,
	INT32 digits,
	INT32 flags
);
void LUA_HUD_AddDrawFill(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	INT32 w,
	INT32 h,
	INT32 c
);
void LUA_HUD_AddDrawString(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	const char *str,
	INT32 flags,
	INT32 align
);
void LUA_HUD_AddDrawNameTag(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	const char *str,
	INT32 flags,
	UINT16 basecolor,
	UINT16 outlinecolor,
	UINT8 *basecolormap,
	UINT8 *outlinecolormap
);
void LUA_HUD_AddDrawScaledNameTag(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	const char *str,
	INT32 flags,
	fixed_t scale,
	UINT16 basecolor,
	UINT16 outlinecolor,
	UINT8 *basecolormap,
	UINT8 *outlinecolormap
);
void LUA_HUD_AddDrawLevelTitle(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	const char *str,
	INT32 flags
);
void LUA_HUD_AddFadeScreen(
	huddrawlist_h list,
	UINT16 color,
	UINT8 strength
);

// Draws the given draw list
void LUA_HUD_DrawList(huddrawlist_h list);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LUA_HUDLIB_DRAWLIST__
