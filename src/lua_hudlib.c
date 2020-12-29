// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2020 by Sonic Team Junior.
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
#include "st_stuff.h" // hudinfo[]
#include "g_game.h"
#include "i_video.h" // rendermode
#include "p_local.h" // camera_t
#include "screen.h" // screen width/height
#include "m_random.h" // m_random
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"

#define HUDONLY if (!hud_running) return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

boolean hud_running = false;
static UINT8 hud_enabled[(hud_MAX/8)+1];

static UINT8 hudAvailable; // hud hooks field

// must match enum hud in lua_hud.h
static const char *const hud_disable_options[] = {
	"stagetitle",
	"textspectator",
	"crosshair",

	"score",
	"time",
	"rings",
	"lives",

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
	"intermissionmessages",
	NULL};

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

enum hudhook {
	hudhook_game = 0,
	hudhook_scores,
	hudhook_intermission,
	hudhook_title,
	hudhook_titlecard
};
static const char *const hudhook_opt[] = {
	"game",
	"scores",
	"intermission",
	"title",
	"titlecard",
	NULL};

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
	enum patch field = luaL_checkoption(L, 2, NULL, patch_opt);

	// patches are invalidated when switching renderers
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
	enum cameraf field = luaL_checkoption(L, 2, NULL, camera_opt);

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
		for (i = 0; i < NUMSPRITES; i++)
			if (fastcmp(name, sprnames[i]))
				break;
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
			patch_t *rotsprite = Patch_GetRotatedSprite(sprframe, frame, angle, sprframe->flip & (1<<angle), true, &spriteinfo[i], rot);
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
	boolean super = false; // add FF_SPR2SUPER to sprite2 if true
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
			if (fastcmp(skins[i].name, name))
				break;
		if (i >= numskins)
			return 0;
	}

	lua_remove(L, 1); // remove skin now

	if (lua_isnumber(L, 1)) // sprite number given, e.g. SPR2_STND
	{
		j = lua_tonumber(L, 1);
		if (j & FF_SPR2SUPER) // e.g. SPR2_STND|FF_SPR2SUPER
		{
			super = true;
			j &= ~FF_SPR2SUPER; // remove flag so the next check doesn't fail
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
		super = lua_toboolean(L, 2); // note: this can override FF_SPR2SUPER from sprite number
		lua_remove(L, 2); // remove
	}
	// if it's not boolean then just assume it's the frame number

	if (super)
		j |= FF_SPR2SUPER;

	j = P_GetSkinSprite2(&skins[i], j, NULL); // feed skin and current sprite2 through to change sprite2 used if necessary

	sprdef = &skins[i].sprites[j];

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
			patch_t *rotsprite = Patch_GetRotatedSprite(sprframe, frame, angle, sprframe->flip & (1<<angle), true, &skins[i].sprinfo[j], rot);
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
	const UINT8 *colormap = NULL;

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

	V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, flags, patch, colormap);
	return 0;
}

static int libd_drawScaled(lua_State *L)
{
	fixed_t x, y, scale;
	INT32 flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

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

	V_DrawFixedPatch(x, y, scale, flags, patch, colormap);
	return 0;
}

static int libd_drawStretched(lua_State *L)
{
	fixed_t x, y, hscale, vscale;
	INT32 flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

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

	V_DrawStretchyFixedPatch(x, y, hscale, vscale, flags, patch, colormap);
	return 0;
}

static int libd_drawNum(lua_State *L)
{
	INT32 x, y, flags, num;
	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = luaL_checkinteger(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawTallNum(x, y, flags, num);
	return 0;
}

static int libd_drawPaddedNum(lua_State *L)
{
	INT32 x, y, flags, num, digits;
	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = labs(luaL_checkinteger(L, 3));
	digits = luaL_optinteger(L, 4, 2);
	flags = luaL_optinteger(L, 5, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawPaddedTallNum(x, y, flags, num, digits);
	return 0;
}

static int libd_drawFill(lua_State *L)
{
	INT32 x = luaL_optinteger(L, 1, 0);
	INT32 y = luaL_optinteger(L, 2, 0);
	INT32 w = luaL_optinteger(L, 3, BASEVIDWIDTH);
	INT32 h = luaL_optinteger(L, 4, BASEVIDHEIGHT);
	INT32 c = luaL_optinteger(L, 5, 31);

	HUDONLY
	V_DrawFill(x, y, w, h, c);
	return 0;
}

static int libd_drawString(lua_State *L)
{
	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, V_ALLOWLOWERCASE);
	enum align align = luaL_checkoption(L, 5, "left", align_opt);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	HUDONLY
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
	V_DrawNameTag(FixedInt(x), FixedInt(y), flags, scale, basecolormap, outlinecolormap, str);
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

static int libd_getColormap(lua_State *L)
{
	INT32 skinnum = TC_DEFAULT;
	skincolornum_t color = luaL_optinteger(L, 2, 0);
	UINT8* colormap = NULL;
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

	// all was successful above, now we generate the colormap at last!

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

static int libd_fadeScreen(lua_State *L)
{
	UINT16 color = luaL_checkinteger(L, 1);
	UINT8 strength = luaL_checkinteger(L, 2);
	const UINT8 maxstrength = ((color & 0xFF00) ? 32 : 10);

	HUDONLY

	if (!strength)
		return 0;

	if (strength > maxstrength)
		return luaL_error(L, "%s fade strength %d out of range (0 - %d)", ((color & 0xFF00) ? "COLORMAP" : "TRANSMAP"), strength, maxstrength);

	if (strength == maxstrength) // Allow as a shortcut for drawfill...
	{
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, ((color & 0xFF00) ? 31 : color));
		return 0;
	}

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

static int libd_dupx(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dupx); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdupx); // push fixed point scale (position scale)
	return 2;
}

static int libd_dupy(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dupy); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdupy); // push fixed point scale (position scale)
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
	if (a > 65536)
		LUA_UsageWarning(L, "v.RandomKey: range > 65536 is undefined behavior");
	lua_pushinteger(L, M_RandomKey(a));
	return 1;
}

static int libd_RandomRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);

	HUDONLY
	if (b < a) {
		INT32 c = a;
		a = b;
		b = c;
	}
	if ((b-a+1) > 65536)
		LUA_UsageWarning(L, "v.RandomRange: range > 65536 is undefined behavior");
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
	// drawing
	{"draw", libd_draw},
	{"drawScaled", libd_drawScaled},
	{"drawStretched", libd_drawStretched},
	{"drawNum", libd_drawNum},
	{"drawPaddedNum", libd_drawPaddedNum},
	{"drawFill", libd_drawFill},
	{"drawString", libd_drawString},
	{"drawNameTag", libd_drawNameTag},
	{"drawScaledNameTag", libd_drawScaledNameTag},
	{"fadeScreen", libd_fadeScreen},
	// misc
	{"stringWidth", libd_stringWidth},
	{"nameTagWidth", libd_nameTagWidth},
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
	{"dupx", libd_dupx},
	{"dupy", libd_dupy},
	{"renderer", libd_renderer},
	{"localTransFlag", libd_getlocaltransflag},
	{"userTransFlag", libd_getusertransflag},
	{NULL, NULL}
};

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
static int lib_hudadd(lua_State *L)
{
	enum hudhook field;

	luaL_checktype(L, 1, LUA_TFUNCTION);
	field = luaL_checkoption(L, 2, "game", hudhook_opt);

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(L, -1));
	lua_rawgeti(L, -1, field+2); // HUD[2+]
	I_Assert(lua_istable(L, -1));
	lua_remove(L, -2);

	lua_pushvalue(L, 1);
	lua_rawseti(L, -2, (int)(lua_objlen(L, -2) + 1));

	hudAvailable |= 1<<field;
	return 0;
}

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

	lua_newtable(L); // HUD registry table
		lua_newtable(L);
		luaL_register(L, NULL, lib_draw);
		lua_rawseti(L, -2, 1); // HUD[1] = lib_draw

		lua_newtable(L);
		lua_rawseti(L, -2, 2); // HUD[2] = game rendering functions array

		lua_newtable(L);
		lua_rawseti(L, -2, 3); // HUD[3] = scores rendering functions array

		lua_newtable(L);
		lua_rawseti(L, -2, 4); // HUD[4] = intermission rendering functions array

		lua_newtable(L);
		lua_rawseti(L, -2, 5); // HUD[5] = title rendering functions array

		lua_newtable(L);
		lua_rawseti(L, -2, 6); // HUD[6] = title card rendering functions array
	lua_setfield(L, LUA_REGISTRYINDEX, "HUD");

	luaL_newmetatable(L, META_HUDINFO);
		lua_pushcfunction(L, hudinfo_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, hudinfo_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, hudinfo_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getHudInfo);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_hudinfolen);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "hudinfo");

	luaL_newmetatable(L, META_COLORMAP);
		lua_pushcfunction(L, colormap_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_PATCH);
		lua_pushcfunction(L, patch_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, patch_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	luaL_newmetatable(L, META_CAMERA);
		lua_pushcfunction(L, camera_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_register(L, "hud", lib_hud);
	return 0;
}

boolean LUA_HudEnabled(enum hud option)
{
	if (!gL || hud_enabled[option/8] & (1<<(option%8)))
		return true;
	return false;
}

// Hook for HUD rendering
void LUAh_GameHUD(player_t *stplayr)
{
	if (!gL || !(hudAvailable & (1<<hudhook_game)))
		return;

	hud_running = true;
	lua_settop(gL, 0);

	lua_pushcfunction(gL, LUA_GetErrorMessage);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2+hudhook_game); // HUD[2] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	LUA_PushUserdata(gL, stplayr, META_PLAYER);

	if (splitscreen && stplayr == &players[secondarydisplayplayer])
		LUA_PushUserdata(gL, &camera2, META_CAMERA);
	else
		LUA_PushUserdata(gL, &camera, META_CAMERA);

	lua_pushnil(gL);
	while (lua_next(gL, -5) != 0) {
		lua_pushvalue(gL, -5); // graphics library (HUD[1])
		lua_pushvalue(gL, -5); // stplayr
		lua_pushvalue(gL, -5); // camera
		LUA_Call(gL, 3, 0, 1);
	}
	lua_settop(gL, 0);
	hud_running = false;
}

void LUAh_ScoresHUD(void)
{
	if (!gL || !(hudAvailable & (1<<hudhook_scores)))
		return;

	hud_running = true;
	lua_settop(gL, 0);

	lua_pushcfunction(gL, LUA_GetErrorMessage);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2+hudhook_scores); // HUD[3] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // graphics library (HUD[1])
		LUA_Call(gL, 1, 0, 1);
	}
	lua_settop(gL, 0);
	hud_running = false;
}

void LUAh_TitleHUD(void)
{
	if (!gL || !(hudAvailable & (1<<hudhook_title)))
		return;

	hud_running = true;
	lua_settop(gL, 0);

	lua_pushcfunction(gL, LUA_GetErrorMessage);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2+hudhook_title); // HUD[5] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // graphics library (HUD[1])
		LUA_Call(gL, 1, 0, 1);
	}
	lua_settop(gL, 0);
	hud_running = false;
}

void LUAh_TitleCardHUD(player_t *stplayr)
{
	if (!gL || !(hudAvailable & (1<<hudhook_titlecard)))
		return;

	hud_running = true;
	lua_settop(gL, 0);

	lua_pushcfunction(gL, LUA_GetErrorMessage);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2+hudhook_titlecard); // HUD[6] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD

	LUA_PushUserdata(gL, stplayr, META_PLAYER);
	lua_pushinteger(gL, lt_ticker);
	lua_pushinteger(gL, (lt_endtime + TICRATE));
	lua_pushnil(gL);

	while (lua_next(gL, -6) != 0) {
		lua_pushvalue(gL, -6); // graphics library (HUD[1])
		lua_pushvalue(gL, -6); // stplayr
		lua_pushvalue(gL, -6); // lt_ticker
		lua_pushvalue(gL, -6); // lt_endtime
		LUA_Call(gL, 4, 0, 1);
	}

	lua_settop(gL, 0);
	hud_running = false;
}

void LUAh_IntermissionHUD(void)
{
	if (!gL || !(hudAvailable & (1<<hudhook_intermission)))
		return;

	hud_running = true;
	lua_settop(gL, 0);

	lua_pushcfunction(gL, LUA_GetErrorMessage);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2+hudhook_intermission); // HUD[4] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // graphics library (HUD[1])
		LUA_Call(gL, 1, 0, 1);
	}
	lua_settop(gL, 0);
	hud_running = false;
}
