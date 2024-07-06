// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hudlib.c
/// \brief custom HUD rendering library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "r_defs.h"
#include "r_local.h"
#include "r_translation.h"
#include "st_stuff.h" // hudinfo[]
#include "g_game.h"
#include "i_video.h" // rendermode
#include "p_local.h" // camera_t
#include "screen.h" // screen width/height
#include "m_random.h" // m_random
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "y_inter.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"
#include "lua_hook.h"

#define HUDONLY if (!hud_running) return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

boolean hud_running = false;
static UINT8 hud_enabled[(hud_MAX/8)+1];

// must match enum hud in lua_hud.h
static const char *const hud_disable_options[] = {
	"stagetitle",
	"textspectator",
	"crosshair",
	"powerups",

	"score",
	"time",
	"rings",
	"lives",
	"input",

	"weaponrings",
	"powerstones",
	"teamscores",

	"nightslink",
	"nightsdrill",
	"nightsrings",
	"nightsscore",
	"nightstime",
	"nightsrecords",

	"rankings",
	"coopemeralds",
	"tokens",
	"tabemblems",

	"intermissiontally",
	"intermissiontitletext",
	"intermissionmessages",
	"intermissionemeralds",
	NULL};

// you know, let's actually make sure that the table is synced.
// because fuck knows how many times this has happened at this point. :v
I_StaticAssert(sizeof(hud_disable_options) / sizeof(*hud_disable_options) == hud_MAX+1);

enum hudinfo {
	hudinfo_x = 0,
	hudinfo_y,
	hudinfo_f
};

static const char *const hudinfo_opt[] = {
	"x",
	"y",
	"f",
	NULL};

enum patch {
	patch_valid = 0,
	patch_width,
	patch_height,
	patch_leftoffset,
	patch_topoffset
};
static const char *const patch_opt[] = {
	"valid",
	"width",
	"height",
	"leftoffset",
	"topoffset",
	NULL};

static int patch_fields_ref = LUA_NOREF;

// alignment types for v.drawString
enum align {
	align_left = 0,
	align_center,
	align_right,
	align_fixed,
	align_fixedcenter,
	align_fixedright,
	align_small,
	align_smallfixed,
	align_smallfixedcenter,
	align_smallfixedright,
	align_smallcenter,
	align_smallright,
	align_smallthin,
	align_smallthincenter,
	align_smallthinright,
	align_smallthinfixed,
	align_smallthinfixedcenter,
	align_smallthinfixedright,
	align_thin,
	align_thinfixed,
	align_thinfixedcenter,
	align_thinfixedright,
	align_thincenter,
	align_thinright
};
static const char *const align_opt[] = {
	"left",
	"center",
	"right",
	"fixed",
	"fixed-center",
	"fixed-right",
	"small",
	"small-fixed",
	"small-fixed-center",
	"small-fixed-right",
	"small-center",
	"small-right",
	"small-thin",
	"small-thin-center",
	"small-thin-right",
	"small-thin-fixed",
	"small-thin-fixed-center",
	"small-thin-fixed-right",
	"thin",
	"thin-fixed",
	"thin-fixed-center",
	"thin-fixed-right",
	"thin-center",
	"thin-right",
	NULL};

// width types for v.stringWidth
enum widtht {
	widtht_normal = 0,
	widtht_small,
	widtht_thin
};
static const char *const widtht_opt[] = {
	"normal",
	"small",
	"thin",
	NULL};

enum cameraf {
	camera_chase = 0,
	camera_aiming,
	camera_x,
	camera_y,
	camera_z,
	camera_reset,
	camera_angle,
	camera_subsector,
	camera_floorz,
	camera_ceilingz,
	camera_radius,
	camera_height,
	camera_momx,
	camera_momy,
	camera_momz
};


static const char *const camera_opt[] = {
	"chase",
	"aiming",
	"x",
	"y",
	"z",
	"reset",
	"angle",
	"subsector",
	"floorz",
	"ceilingz",
	"radius",
	"height",
	"momx",
	"momy",
	"momz",
	NULL};

static int camera_fields_ref = LUA_NOREF;

static int lib_getHudInfo(lua_State *L)
{
	UINT32 i;
	lua_remove(L, 1);

	i = luaL_checkinteger(L, 1);
	if (i >= NUMHUDITEMS)
		return luaL_error(L, "hudinfo[] index %d out of range (0 - %d)", i, NUMHUDITEMS-1);
	LUA_PushUserdata(L, &hudinfo[i], META_HUDINFO);
	return 1;
}

static int lib_hudinfolen(lua_State *L)
{
	lua_pushinteger(L, NUMHUDITEMS);
	return 1;
}

static int hudinfo_get(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	enum hudinfo field = luaL_checkoption(L, 2, hudinfo_opt[0], hudinfo_opt);
	I_Assert(info != NULL); // huditems are always valid

	switch(field)
	{
	case hudinfo_x:
		lua_pushinteger(L, info->x);
		break;
	case hudinfo_y:
		lua_pushinteger(L, info->y);
		break;
	case hudinfo_f:
		lua_pushinteger(L, info->f);
		break;
	}
	return 1;
}

static int hudinfo_set(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	enum hudinfo field = luaL_checkoption(L, 2, hudinfo_opt[0], hudinfo_opt);
	I_Assert(info != NULL);

	switch(field)
	{
	case hudinfo_x:
		info->x = (INT32)luaL_checkinteger(L, 3);
		break;
	case hudinfo_y:
		info->y = (INT32)luaL_checkinteger(L, 3);
		break;
	case hudinfo_f:
		info->f = (INT32)luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

static int hudinfo_num(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	lua_pushinteger(L, info-hudinfo);
	return 1;
}

static int colormap_get(lua_State *L)
{
	const UINT8 *colormap = *((UINT8 **)luaL_checkudata(L, 1, META_COLORMAP));
	UINT32 i = luaL_checkinteger(L, 2);
	if (i >= 256)
		return luaL_error(L, "colormap index %d out of range (0 - %d)", i, 255);
	lua_pushinteger(L, colormap[i]);
	return 1;
}

static int patch_get(lua_State *L)
{
	patch_t *patch = *((patch_t **)luaL_checkudata(L, 1, META_PATCH));
	enum patch field = Lua_optoption(L, 2, -1, patch_fields_ref);

	if (!patch) {
		if (field == patch_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return LUA_ErrInvalid(L, "patch_t");
	}

	switch (field)
	{
	case patch_valid:
		lua_pushboolean(L, patch != NULL);
		break;
	case patch_width:
		lua_pushinteger(L, patch->width);
		break;
	case patch_height:
		lua_pushinteger(L, patch->height);
		break;
	case patch_leftoffset:
		lua_pushinteger(L, patch->leftoffset);
		break;
	case patch_topoffset:
		lua_pushinteger(L, patch->topoffset);
		break;
	}
	return 1;
}

static int patch_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("patch_t") " struct cannot be edited by Lua.");
}

static int camera_get(lua_State *L)
{
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 1, META_CAMERA));
	enum cameraf field = Lua_optoption(L, 2, -1, camera_fields_ref);

	// cameras should always be valid unless I'm a nutter
	I_Assert(cam != NULL);

	switch (field)
	{
	case camera_chase:
		lua_pushboolean(L, cam->chase);
		break;
	case camera_aiming:
		lua_pushinteger(L, cam->aiming);
		break;
	case camera_x:
		lua_pushinteger(L, cam->x);
		break;
	case camera_y:
		lua_pushinteger(L, cam->y);
		break;
	case camera_z:
		lua_pushinteger(L, cam->z);
		break;
	case camera_reset:
		lua_pushboolean(L, cam->reset);
		break;
	case camera_angle:
		lua_pushinteger(L, cam->angle);
		break;
	case camera_subsector:
		LUA_PushUserdata(L, cam->subsector, META_SUBSECTOR);
		break;
	case camera_floorz:
		lua_pushinteger(L, cam->floorz);
		break;
	case camera_ceilingz:
		lua_pushinteger(L, cam->ceilingz);
		break;
	case camera_radius:
		lua_pushinteger(L, cam->radius);
		break;
	case camera_height:
		lua_pushinteger(L, cam->height);
		break;
	case camera_momx:
		lua_pushinteger(L, cam->momx);
		break;
	case camera_momy:
		lua_pushinteger(L, cam->momy);
		break;
	case camera_momz:
		lua_pushinteger(L, cam->momz);
		break;
	}
	return 1;
}

static int camera_set(lua_State *L)
{
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 1, META_CAMERA));
	enum cameraf field = Lua_optoption(L, 2, -1, camera_fields_ref);

	I_Assert(cam != NULL);

	switch(field)
	{
	case camera_subsector:
	case camera_floorz:
	case camera_ceilingz:
	case camera_x:
	case camera_y:
		return luaL_error(L, LUA_QL("camera_t") " field " LUA_QS " should not be set directly. Use " LUA_QL("P_TryCameraMove") " or " LUA_QL("P_TeleportCameraMove") " instead.", camera_opt[field]);
	case camera_reset:
		cam->reset = luaL_checkboolean(L, 3);
		break;
	case camera_chase: {
		INT32 chase = luaL_checkboolean(L, 3);
		if (cam == &camera)
			CV_SetValue(&cv_chasecam, chase);
		else if (cam == &camera2)
			CV_SetValue(&cv_chasecam2, chase);
		else // ??? this should never happen, but ok
			cam->chase = chase;
		break;
	}
	case camera_aiming:
		cam->aiming = luaL_checkangle(L, 3);
		break;
	case camera_z:
		cam->z = luaL_checkfixed(L, 3);
		P_CheckCameraPosition(cam->x, cam->y, cam);
		cam->floorz = tmfloorz;
		cam->ceilingz = tmceilingz;
		break;
	case camera_angle:
		cam->angle = luaL_checkangle(L, 3);
		break;
	case camera_radius:
		cam->radius = luaL_checkfixed(L, 3);
		if (cam->radius < 0)
			cam->radius = 0;
		P_CheckCameraPosition(cam->x, cam->y, cam);
		cam->floorz = tmfloorz;
		cam->ceilingz = tmceilingz;
		break;
	case camera_height:
		cam->height = luaL_checkfixed(L, 3);
		if (cam->height < 0)
			cam->height = 0;
		P_CheckCameraPosition(cam->x, cam->y, cam);
		cam->floorz = tmfloorz;
		cam->ceilingz = tmceilingz;
		break;
	case camera_momx:
		cam->momx = luaL_checkfixed(L, 3);
		break;
	case camera_momy:
		cam->momy = luaL_checkfixed(L, 3);
		break;
	case camera_momz:
		cam->momz = luaL_checkfixed(L, 3);
		break;
	default:
		return luaL_error(L, LUA_QL("camera_t") " has no field named " LUA_QS ".", lua_tostring(L, 2));
	}
	return 0;
}

//
// lib_draw
//

static int libd_patchExists(lua_State *L)
{
	HUDONLY
	lua_pushboolean(L, W_LumpExists(luaL_checkstring(L, 1)));
	return 1;
}

static int libd_cachePatch(lua_State *L)
{
	HUDONLY
	LUA_PushUserdata(L, W_CachePatchLongName(luaL_checkstring(L, 1), PU_PATCH), META_PATCH);
	return 1;
}

// v.getSpritePatch(sprite, [frame, [angle, [rollangle]]])
static int libd_getSpritePatch(lua_State *L)
{
	UINT32 i; // sprite prefix
	UINT32 frame = 0; // 'A'
	UINT8 angle = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	HUDONLY

	if (lua_isnumber(L, 1)) // sprite number given, e.g. SPR_THOK
	{
		i = lua_tonumber(L, 1);
		if (i >= NUMSPRITES)
			return 0;
	}
	else if (lua_isstring(L, 1)) // sprite prefix name given, e.g. "THOK"
	{
		const char *name = lua_tostring(L, 1);
		i = R_GetSpriteNumByName(name);
		if (i >= NUMSPRITES)
			return 0;
	}
	else
		return 0;

	if (i == SPR_PLAY) // Use getSprite2Patch instead!
		return 0;

	sprdef = &sprites[i];

	// set frame number
	frame = luaL_optinteger(L, 2, 0);
	frame &= FF_FRAMEMASK; // ignore any bits that are not the actual frame, just in case
	if (frame >= sprdef->numframes)
		return 0;
	// set angle number
	sprframe = &sprdef->spriteframes[frame];
	angle = luaL_optinteger(L, 3, 1);

	// convert WAD editor angle numbers (1-8) to internal angle numbers (0-7)
	// keep 0 the same since we'll make it default to angle 1 (which is internally 0)
	// in case somebody didn't know that angle 0 really just maps all 8/16 angles to the same patch
	if (angle != 0)
		angle--;

	if (angle >= ((sprframe->rotate & SRF_3DGE) ? 16 : 8)) // out of range?
		return 0;

#ifdef ROTSPRITE
	if (lua_isnumber(L, 4))
	{
		// rotsprite?????
		angle_t rollangle = luaL_checkangle(L, 4);
		INT32 rot = R_GetRollAngle(rollangle);

		if (rot) {
			patch_t *rotsprite = Patch_GetRotatedSprite(sprframe, frame, angle, sprframe->flip & (1<<angle), &spriteinfo[i], rot);
			LUA_PushUserdata(L, rotsprite, META_PATCH);
			lua_pushboolean(L, false);
			lua_pushboolean(L, true);
			return 3;
		}
	}
#endif

	// push both the patch and it's "flip" value
	LUA_PushUserdata(L, W_CachePatchNum(sprframe->lumppat[angle], PU_SPRITE), META_PATCH);
	lua_pushboolean(L, (sprframe->flip & (1<<angle)) != 0);
	return 2;
}

// v.getSprite2Patch(skin, sprite, [super?,] [frame, [angle, [rollangle]]])
static int libd_getSprite2Patch(lua_State *L)
{
	INT32 i; // skin number
	playersprite_t j; // sprite2 prefix
	UINT32 frame = 0; // 'A'
	UINT8 angle = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	boolean super = false; // add SPR2F_SUPER to sprite2 if true
	HUDONLY

	// get skin first!
	if (lua_isnumber(L, 1)) // find skin by number
	{
		i = lua_tonumber(L, 1);
		if (i < 0 || i >= MAXSKINS)
			return luaL_error(L, "skin number %d out of range (0 - %d)", i, MAXSKINS-1);
		if (i >= numskins)
			return 0;
	}
	else // find skin by name
	{
		const char *name = luaL_checkstring(L, 1);
		for (i = 0; i < numskins; i++)
			if (fastcmp(skins[i]->name, name))
				break;
		if (i >= numskins)
			return 0;
	}

	lua_remove(L, 1); // remove skin now

	if (lua_isnumber(L, 1)) // sprite number given, e.g. SPR2_STND
	{
		j = lua_tonumber(L, 1);
		if (j & SPR2F_SUPER) // e.g. SPR2_STND|SPR2F_SUPER
		{
			super = true;
			j &= ~SPR2F_SUPER; // remove flag so the next check doesn't fail
		}

		if (j >= free_spr2)
			return 0;
	}
	else if (lua_isstring(L, 1)) // sprite prefix name given, e.g. "STND"
	{
		const char *name = lua_tostring(L, 1);
		for (j = 0; j < free_spr2; j++)
			if (fastcmp(name, spr2names[j]))
				break;
		// if you want super flags you'll have to use the optional boolean following this
		if (j >= free_spr2)
			return 0;
	}
	else
		return 0;

	if (lua_isboolean(L, 2)) // optional boolean for superness
	{
		super = lua_toboolean(L, 2); // note: this can override SPR2F_SUPER from sprite number
		lua_remove(L, 2); // remove
	}
	// if it's not boolean then just assume it's the frame number

	if (super)
		j |= SPR2F_SUPER;

	sprdef = P_GetSkinSpritedef(skins[i], j);

	// set frame number
	frame = luaL_optinteger(L, 2, 0);
	frame &= FF_FRAMEMASK; // ignore any bits that are not the actual frame, just in case
	if (frame >= sprdef->numframes)
		return 0;
	// set angle number
	sprframe = &sprdef->spriteframes[frame];
	angle = luaL_optinteger(L, 3, 1);

	// convert WAD editor angle numbers (1-8) to internal angle numbers (0-7)
	// keep 0 the same since we'll make it default to angle 1 (which is internally 0)
	// in case somebody didn't know that angle 0 really just maps all 8/16 angles to the same patch
	if (angle != 0)
		angle--;

	if (angle >= ((sprframe->rotate & SRF_3DGE) ? 16 : 8)) // out of range?
		return 0;

#ifdef ROTSPRITE
	if (lua_isnumber(L, 4))
	{
		// rotsprite?????
		angle_t rollangle = luaL_checkangle(L, 4);
		INT32 rot = R_GetRollAngle(rollangle);

		if (rot) {
			patch_t *rotsprite = Patch_GetRotatedSprite(sprframe, frame, angle, sprframe->flip & (1<<angle), P_GetSkinSpriteInfo(skins[i], j), rot);
			LUA_PushUserdata(L, rotsprite, META_PATCH);
			lua_pushboolean(L, false);
			lua_pushboolean(L, true);
			return 3;
		}
	}
#endif

	// push both the patch and it's "flip" value
	LUA_PushUserdata(L, W_CachePatchNum(sprframe->lumppat[angle], PU_SPRITE), META_PATCH);
	lua_pushboolean(L, (sprframe->flip & (1<<angle)) != 0);
	return 2;
}

static int libd_draw(lua_State *L)
{
	INT32 x, y, flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	patch = *((patch_t **)luaL_checkudata(L, 3, META_PATCH));
	if (!patch)
		return LUA_ErrInvalid(L, "patch_t");
	flags = luaL_optinteger(L, 4, 0);
	if (!lua_isnoneornil(L, 5))
		colormap = *((UINT8 **)luaL_checkudata(L, 5, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDraw(list, x, y, patch, flags, colormap);
	else
		V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, flags, patch, colormap);
	return 0;
}

static int libd_drawScaled(lua_State *L)
{
	fixed_t x, y, scale;
	INT32 flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	scale = luaL_checkinteger(L, 3);
	if (scale < 0)
		return luaL_error(L, "negative scale");
	patch = *((patch_t **)luaL_checkudata(L, 4, META_PATCH));
	if (!patch)
		return LUA_ErrInvalid(L, "patch_t");
	flags = luaL_optinteger(L, 5, 0);
	if (!lua_isnoneornil(L, 6))
		colormap = *((UINT8 **)luaL_checkudata(L, 6, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawScaled(list, x, y, scale, patch, flags, colormap);
	else
		V_DrawFixedPatch(x, y, scale, flags, patch, colormap);
	return 0;
}

static int libd_drawStretched(lua_State *L)
{
	fixed_t x, y, hscale, vscale;
	INT32 flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	hscale = luaL_checkinteger(L, 3);
	if (hscale < 0)
		return luaL_error(L, "negative horizontal scale");
	vscale = luaL_checkinteger(L, 4);
	if (vscale < 0)
		return luaL_error(L, "negative vertical scale");
	patch = *((patch_t **)luaL_checkudata(L, 5, META_PATCH));
	flags = luaL_optinteger(L, 6, 0);
	if (!lua_isnoneornil(L, 7))
		colormap = *((UINT8 **)luaL_checkudata(L, 7, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawStretched(list, x, y, hscale, vscale, patch, flags, colormap);
	else
		V_DrawStretchyFixedPatch(x, y, hscale, vscale, flags, patch, colormap);
	return 0;
}

static int libd_drawCropped(lua_State *L)
{
	fixed_t x, y, hscale, vscale, sx, sy, w, h;
	INT32 flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	hscale = luaL_checkinteger(L, 3);
	if (hscale < 0)
		return luaL_error(L, "negative horizontal scale");
	vscale = luaL_checkinteger(L, 4);
	if (vscale < 0)
		return luaL_error(L, "negative vertical scale");
	patch = *((patch_t **)luaL_checkudata(L, 5, META_PATCH));
	flags = luaL_checkinteger(L, 6);
	if (!lua_isnoneornil(L, 7))
		colormap = *((UINT8 **)luaL_checkudata(L, 7, META_COLORMAP));
	sx = luaL_checkinteger(L, 8);
	if (sx < 0) // Don't crash. Now, we could do "x-=sx*FRACUNIT; sx=0;" here...
		return luaL_error(L, "negative crop sx");
	sy = luaL_checkinteger(L, 9);
	if (sy < 0) // ...but it's more truthful to just deny it, as negative values would crash
		return luaL_error(L, "negative crop sy");
	w = luaL_checkinteger(L, 10);
	if (w < 0) // Again, don't crash
		return luaL_error(L, "negative crop w");
	h = luaL_checkinteger(L, 11);
	if (h < 0)
		return luaL_error(L, "negative crop h");

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawCropped(list, x, y, hscale, vscale, patch, flags, colormap, sx, sy, w, h);
	else
		V_DrawCroppedPatch(x, y, hscale, vscale, flags, patch, colormap, sx, sy, w, h);
	return 0;
}

static int libd_drawNum(lua_State *L)
{
	INT32 x, y, flags, num;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = luaL_checkinteger(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawNum(list, x, y, num, flags);
	else
		V_DrawTallNum(x, y, flags, num);
	return 0;
}

static int libd_drawPaddedNum(lua_State *L)
{
	INT32 x, y, flags, num, digits;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = labs(luaL_checkinteger(L, 3));
	digits = luaL_optinteger(L, 4, 2);
	flags = luaL_optinteger(L, 5, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawPaddedNum(list, x, y, num, digits, flags);
	else
		V_DrawPaddedTallNum(x, y, flags, num, digits);
	return 0;
}

static int libd_drawFill(lua_State *L)
{
	huddrawlist_h list;
	INT32 x = luaL_optinteger(L, 1, 0);
	INT32 y = luaL_optinteger(L, 2, 0);
	INT32 w = luaL_optinteger(L, 3, BASEVIDWIDTH);
	INT32 h = luaL_optinteger(L, 4, BASEVIDHEIGHT);
	INT32 c = luaL_optinteger(L, 5, 31);

	HUDONLY

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawFill(list, x, y, w, h, c);
	else
		V_DrawFill(x, y, w, h, c);
	return 0;
}

static int libd_drawString(lua_State *L)
{
	huddrawlist_h list;
	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, V_ALLOWLOWERCASE);
	enum align align = luaL_checkoption(L, 5, "left", align_opt);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	HUDONLY

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	// okay, sorry, this is kind of ugly
	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawString(list, x, y, str, flags, align);
	else
	switch(align)
	{
	// hu_font
	case align_left:
		V_DrawString(x, y, flags, str);
		break;
	case align_center:
		V_DrawCenteredString(x, y, flags, str);
		break;
	case align_right:
		V_DrawRightAlignedString(x, y, flags, str);
		break;
	case align_fixed:
		V_DrawStringAtFixed(x, y, flags, str);
		break;
	case align_fixedcenter:
		V_DrawCenteredStringAtFixed(x, y, flags, str);
		break;
	case align_fixedright:
		V_DrawRightAlignedStringAtFixed(x, y, flags, str);
		break;
	// hu_font, 0.5x scale
	case align_small:
		V_DrawSmallString(x, y, flags, str);
		break;
	case align_smallfixed:
		V_DrawSmallStringAtFixed(x, y, flags, str);
		break;
	case align_smallfixedcenter:
		V_DrawCenteredSmallStringAtFixed(x, y, flags, str);
		break;
	case align_smallfixedright:
		V_DrawRightAlignedSmallStringAtFixed(x, y, flags, str);
		break;
	case align_smallcenter:
		V_DrawCenteredSmallString(x, y, flags, str);
		break;
	case align_smallright:
		V_DrawRightAlignedSmallString(x, y, flags, str);
		break;
	case align_smallthin:
		V_DrawSmallThinString(x, y, flags, str);
		break;
	case align_smallthincenter:
		V_DrawCenteredSmallThinString(x, y, flags, str);
		break;
	case align_smallthinright:
		V_DrawRightAlignedSmallThinString(x, y, flags, str);
		break;
	case align_smallthinfixed:
		V_DrawSmallThinStringAtFixed(x, y, flags, str);
		break;
	case align_smallthinfixedcenter:
		V_DrawCenteredSmallThinStringAtFixed(x, y, flags, str);
		break;
	case align_smallthinfixedright:
		V_DrawRightAlignedSmallThinStringAtFixed(x, y, flags, str);
		break;
	// tny_font
	case align_thin:
		V_DrawThinString(x, y, flags, str);
		break;
	case align_thincenter:
		V_DrawCenteredThinString(x, y, flags, str);
		break;
	case align_thinright:
		V_DrawRightAlignedThinString(x, y, flags, str);
		break;
	case align_thinfixed:
		V_DrawThinStringAtFixed(x, y, flags, str);
		break;
	case align_thinfixedcenter:
		V_DrawCenteredThinStringAtFixed(x, y, flags, str);
		break;
	case align_thinfixedright:
		V_DrawRightAlignedThinStringAtFixed(x, y, flags, str);
		break;
	}
	return 0;
}

static int libd_drawNameTag(lua_State *L)
{
	INT32 x;
	INT32 y;
	const char *str;
	INT32 flags;
	UINT16 basecolor;
	UINT16 outlinecolor;
	UINT8 *basecolormap = NULL;
	UINT8 *outlinecolormap = NULL;
	huddrawlist_h list;

	HUDONLY

	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	str = luaL_checkstring(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	basecolor = luaL_optinteger(L, 5, SKINCOLOR_BLUE);
	outlinecolor = luaL_optinteger(L, 6, SKINCOLOR_ORANGE);
	if (basecolor != SKINCOLOR_NONE)
		basecolormap = R_GetTranslationColormap(TC_DEFAULT, basecolor, GTC_CACHE);
	if (outlinecolor != SKINCOLOR_NONE)
		outlinecolormap = R_GetTranslationColormap(TC_DEFAULT, outlinecolor, GTC_CACHE);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawNameTag(list, x, y, str, flags, basecolor, outlinecolor, basecolormap, outlinecolormap);
	else
		V_DrawNameTag(x, y, flags, FRACUNIT, basecolormap, outlinecolormap, str);
	return 0;
}

static int libd_drawScaledNameTag(lua_State *L)
{
	fixed_t x;
	fixed_t y;
	const char *str;
	INT32 flags;
	fixed_t scale;
	UINT16 basecolor;
	UINT16 outlinecolor;
	UINT8 *basecolormap = NULL;
	UINT8 *outlinecolormap = NULL;
	huddrawlist_h list;

	HUDONLY

	x = luaL_checkfixed(L, 1);
	y = luaL_checkfixed(L, 2);
	str = luaL_checkstring(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	scale = luaL_optinteger(L, 5, FRACUNIT);
	if (scale < 0)
		return luaL_error(L, "negative scale");
	basecolor = luaL_optinteger(L, 6, SKINCOLOR_BLUE);
	outlinecolor = luaL_optinteger(L, 7, SKINCOLOR_ORANGE);
	if (basecolor != SKINCOLOR_NONE)
		basecolormap = R_GetTranslationColormap(TC_DEFAULT, basecolor, GTC_CACHE);
	if (outlinecolor != SKINCOLOR_NONE)
		outlinecolormap = R_GetTranslationColormap(TC_DEFAULT, outlinecolor, GTC_CACHE);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawScaledNameTag(list, x, y, str, flags, scale, basecolor, outlinecolor, basecolormap, outlinecolormap);
	else
		V_DrawNameTag(FixedInt(x), FixedInt(y), flags, scale, basecolormap, outlinecolormap, str);
	return 0;
}

static int libd_drawLevelTitle(lua_State *L)
{
	INT32 x;
	INT32 y;
	const char *str;
	INT32 flags;
	huddrawlist_h list;

	HUDONLY

	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	str = luaL_checkstring(L, 3);
	flags = luaL_optinteger(L, 4, 0);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawLevelTitle(list, x, y, str, flags);
	else
		V_DrawLevelTitle(x, y, flags, str);
	return 0;
}

static int libd_stringWidth(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	INT32 flags = luaL_optinteger(L, 2, V_ALLOWLOWERCASE);
	enum widtht widtht = luaL_checkoption(L, 3, "normal", widtht_opt);

	HUDONLY
	switch(widtht)
	{
	case widtht_normal: // hu_font
		lua_pushinteger(L, V_StringWidth(str, flags));
		break;
	case widtht_small: // hu_font, 0.5x scale
		lua_pushinteger(L, V_SmallStringWidth(str, flags));
		break;
	case widtht_thin: // tny_font
		lua_pushinteger(L, V_ThinStringWidth(str, flags));
		break;
	}
	return 1;
}

static int libd_nameTagWidth(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, V_NameTagWidth(luaL_checkstring(L, 1)));
	return 1;
}

static int libd_levelTitleWidth(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, V_LevelNameWidth(luaL_checkstring(L, 1)));
	return 1;
}

static int libd_levelTitleHeight(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, V_LevelNameHeight(luaL_checkstring(L, 1)));
	return 1;
}

static int libd_getColormap(lua_State *L)
{
	INT32 skinnum = TC_DEFAULT;
	skincolornum_t color = luaL_optinteger(L, 2, 0);
	UINT8* colormap = NULL;
	int translation_id = -1;

	HUDONLY

	if (lua_isnoneornil(L, 1))
		; // defaults to TC_DEFAULT
	else if (lua_type(L, 1) == LUA_TNUMBER) // skin number
	{
		skinnum = (INT32)luaL_checkinteger(L, 1);
		if (skinnum >= MAXSKINS)
			return luaL_error(L, "skin number %d is out of range (>%d)", skinnum, MAXSKINS-1);
		else if (skinnum < 0 && skinnum > TC_DEFAULT)
			return luaL_error(L, "translation colormap index is out of range");
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 1);
		INT32 i = R_SkinAvailable(skinname);
		if (i != -1) // if -1, just default to TC_DEFAULT as above
			skinnum = i;
	}

	if (!lua_isnoneornil(L, 3))
	{
		const char *translation_name = luaL_checkstring(L, 3);
		translation_id = R_FindCustomTranslation(translation_name);
		if (translation_id == -1)
			return luaL_error(L, "invalid translation '%s'.", translation_name);
	}

	// all was successful above, now we generate the colormap at last!
	if (translation_id != -1)
		colormap = R_GetTranslationRemap(translation_id, color, skinnum);

	if (colormap == NULL)
		colormap = R_GetTranslationColormap(skinnum, color, GTC_CACHE);

	LUA_PushUserdata(L, colormap, META_COLORMAP); // push as META_COLORMAP userdata, specifically for patches to use!
	return 1;
}

static int libd_getStringColormap(lua_State *L)
{
	INT32 flags = luaL_checkinteger(L, 1);
	UINT8* colormap = NULL;
	HUDONLY
	colormap = V_GetStringColormap(flags & V_CHARCOLORMASK);
	if (colormap) {
		LUA_PushUserdata(L, colormap, META_COLORMAP); // push as META_COLORMAP userdata, specifically for patches to use!
		return 1;
	}
	return 0;
}

static int libd_getSectorColormap(lua_State *L)
{
	boolean has_sector = false;
	sector_t *sector = NULL;
	if (!lua_isnoneornil(L, 1))
	{
		has_sector = true;
		sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	}
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	int lightlevel = luaL_optinteger(L, 5, 255);
	UINT8 *colormap = NULL;
	extracolormap_t *exc = NULL;

	INLEVEL
	HUDONLY

	if (has_sector && !sector)
		return LUA_ErrInvalid(L, "sector_t");

	if (sector)
		exc = P_GetColormapFromSectorAt(sector, x, y, z);
	else
		exc = P_GetSectorColormapAt(x, y, z);

	if (exc)
		colormap = exc->colormap;
	else
		colormap = colormaps;

	lightlevel = 255 - min(max(lightlevel, 0), 255);
	lightlevel >>= 3;

	LUA_PushUserdata(L, colormap + (lightlevel * 256), META_COLORMAP);
	return 1;
}

static int libd_fadeScreen(lua_State *L)
{
	huddrawlist_h list;
	UINT16 color = luaL_checkinteger(L, 1);
	UINT8 strength = luaL_checkinteger(L, 2);
	const UINT8 maxstrength = ((color & 0xFF00) ? 32 : 10);

	HUDONLY

	if (!strength)
		return 0;

	if (strength > maxstrength)
		return luaL_error(L, "%s fade strength %d out of range (0 - %d)", ((color & 0xFF00) ? "COLORMAP" : "TRANSMAP"), strength, maxstrength);

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (strength == maxstrength) // Allow as a shortcut for drawfill...
	{
		if (LUA_HUD_IsDrawListValid(list))
			LUA_HUD_AddDrawFill(list, 0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, ((color & 0xFF00) ? 31 : color));
		else
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, ((color & 0xFF00) ? 31 : color));
		return 0;
	}

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddFadeScreen(list, color, strength);
	else
		V_DrawFadeScreen(color, strength);

	return 0;
}

static int libd_width(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.width); // push screen width
	return 1;
}

static int libd_height(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.height); // push screen height
	return 1;
}

static int libd_dup(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dup); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdup); // push fixed point scale (position scale)
	return 2;
}

static int libd_renderer(lua_State *L)
{
	HUDONLY
	switch (rendermode) {
		case render_opengl: lua_pushliteral(L, "opengl");   break; // OpenGL renderer
		case render_soft:   lua_pushliteral(L, "software"); break; // Software renderer
		default:            lua_pushliteral(L, "none");     break; // render_none (for dedicated), in case there's any reason this should be run
	}
	return 1;
}

// M_RANDOM
//////////////

static int libd_RandomFixed(lua_State *L)
{
	HUDONLY
	lua_pushfixed(L, M_RandomFixed());
	return 1;
}

static int libd_RandomByte(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, M_RandomByte());
	return 1;
}

static int libd_RandomKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);

	HUDONLY
	lua_pushinteger(L, M_RandomKey(a));
	return 1;
}

static int libd_RandomRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);

	HUDONLY
	lua_pushinteger(L, M_RandomRange(a, b));
	return 1;
}

// Macros.
static int libd_SignedRandom(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, M_SignedRandom());
	return 1;
}

static int libd_RandomChance(lua_State *L)
{
	fixed_t p = luaL_checkfixed(L, 1);
	HUDONLY
	lua_pushboolean(L, M_RandomChance(p));
	return 1;
}

// 30/10/18 Lat': Get st_translucency's value for HUD rendering as a normal V_xxTRANS int
// Could as well be thrown in global vars for ease of access but I guess it makes sense for it to be a HUD fn
static int libd_getlocaltransflag(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, (10-st_translucency)*V_10TRANS);
	return 1;
}

// Get cv_translucenthud's value for HUD rendering as a normal V_xxTRANS int
static int libd_getusertransflag(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, (10-cv_translucenthud.value)*V_10TRANS);	// A bit weird that it's called "translucenthud" yet 10 is fully opaque :V
	return 1;
}

static luaL_Reg lib_draw[] = {
	// cache
	{"patchExists", libd_patchExists},
	{"cachePatch", libd_cachePatch},
	{"getSpritePatch", libd_getSpritePatch},
	{"getSprite2Patch", libd_getSprite2Patch},
	{"getColormap", libd_getColormap},
	{"getStringColormap", libd_getStringColormap},
	{"getSectorColormap", libd_getSectorColormap},
	// drawing
	{"draw", libd_draw},
	{"drawScaled", libd_drawScaled},
	{"drawStretched", libd_drawStretched},
	{"drawCropped", libd_drawCropped},
	{"drawNum", libd_drawNum},
	{"drawPaddedNum", libd_drawPaddedNum},
	{"drawFill", libd_drawFill},
	{"drawString", libd_drawString},
	{"drawNameTag", libd_drawNameTag},
	{"drawScaledNameTag", libd_drawScaledNameTag},
	{"drawLevelTitle", libd_drawLevelTitle},
	{"fadeScreen", libd_fadeScreen},
	// misc
	{"stringWidth", libd_stringWidth},
	{"nameTagWidth", libd_nameTagWidth},
	{"levelTitleWidth", libd_levelTitleWidth},
	{"levelTitleHeight", libd_levelTitleHeight},
	// m_random
	{"RandomFixed",libd_RandomFixed},
	{"RandomByte",libd_RandomByte},
	{"RandomKey",libd_RandomKey},
	{"RandomRange",libd_RandomRange},
	{"SignedRandom",libd_SignedRandom}, // MACRO
	{"RandomChance",libd_RandomChance}, // MACRO
	// properties
	{"width", libd_width},
	{"height", libd_height},
	{"dupx", libd_dup},
	{"dupy", libd_dup},
	{"renderer", libd_renderer},
	{"localTransFlag", libd_getlocaltransflag},
	{"userTransFlag", libd_getusertransflag},
	{NULL, NULL}
};

static int lib_draw_ref;

//
// lib_hud
//

// enable vanilla HUD element
static int lib_hudenable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] |= 1<<(option%8);
	return 0;
}

// disable vanilla HUD element
static int lib_huddisable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] &= ~(1<<(option%8));
	return 0;
}

// 30/10/18: Lat': How come this wasn't here before?
static int lib_hudenabled(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	if (hud_enabled[option/8] & (1<<(option%8)))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}


// add a HUD element for rendering
extern int lib_hudadd(lua_State *L);

static luaL_Reg lib_hud[] = {
	{"enable", lib_hudenable},
	{"disable", lib_huddisable},
	{"enabled", lib_hudenabled},
	{"add", lib_hudadd},
	{NULL, NULL}
};

//
//
//

int LUA_HudLib(lua_State *L)
{
	memset(hud_enabled, 0xff, (hud_MAX/8)+1);

	lua_newtable(L);
	luaL_register(L, NULL, lib_draw);
	lib_draw_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	LUA_RegisterUserdataMetatable(L, META_HUDINFO, hudinfo_get, hudinfo_set, hudinfo_num);
	LUA_RegisterUserdataMetatable(L, META_COLORMAP, colormap_get, NULL, NULL);
	LUA_RegisterUserdataMetatable(L, META_PATCH, patch_get, patch_set, NULL);
	LUA_RegisterUserdataMetatable(L, META_CAMERA, camera_get, camera_set, NULL);

	patch_fields_ref = Lua_CreateFieldTable(L, patch_opt);
	camera_fields_ref = Lua_CreateFieldTable(L, camera_opt);

	LUA_RegisterGlobalUserdata(L, "hudinfo", lib_getHudInfo, NULL, lib_hudinfolen);

	luaL_register(L, "hud", lib_hud);
	return 0;
}

boolean LUA_HudEnabled(enum hud option)
{
	if (!gL || hud_enabled[option/8] & (1<<(option%8)))
		return true;
	return false;
}

void LUA_SetHudHook(int hook, huddrawlist_h list)
{
	lua_getref(gL, lib_draw_ref);

	lua_pushlightuserdata(gL, list);
	lua_setfield(gL, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");

	switch (hook)
	{
		case HUD_HOOK(game): {
			camera_t *cam = (splitscreen && stplyr ==
					&players[secondarydisplayplayer])
				? &camera2 : &camera;

			LUA_PushUserdata(gL, stplyr, META_PLAYER);
			LUA_PushUserdata(gL, cam, META_CAMERA);
		}	break;

		case HUD_HOOK(titlecard):
			LUA_PushUserdata(gL, stplyr, META_PLAYER);
			lua_pushinteger(gL, lt_ticker);
			lua_pushinteger(gL, (lt_endtime + TICRATE));
			break;

		case HUD_HOOK(intermission):
			lua_pushboolean(gL, stagefailed);
	}
}
