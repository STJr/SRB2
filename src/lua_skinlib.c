// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_skinlib.c
/// \brief player skin structure library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "r_skins.h"
#include "sounds.h"

#include "lua_script.h"
#include "lua_libs.h"

enum skin {
	skin_valid = 0,
	skin_name,
	skin_wadnum,
	skin_flags,
	skin_realname,
	skin_hudname,
	skin_ability,
	skin_ability2,
	skin_thokitem,
	skin_spinitem,
	skin_revitem,
	skin_followitem,
	skin_actionspd,
	skin_mindash,
	skin_maxdash,
	skin_normalspeed,
	skin_runspeed,
	skin_thrustfactor,
	skin_accelstart,
	skin_acceleration,
	skin_jumpfactor,
	skin_radius,
	skin_height,
	skin_spinheight,
	skin_shieldscale,
	skin_camerascale,
	skin_starttranscolor,
	skin_prefcolor,
	skin_supercolor,
	skin_prefoppositecolor,
	skin_highresscale,
	skin_contspeed,
	skin_contangle,
	skin_soundsid,
	skin_availability,
	skin_sprites
};
static const char *const skin_opt[] = {
	"valid",
	"name",
	"wadnum",
	"flags",
	"realname",
	"hudname",
	"ability",
	"ability2",
	"thokitem",
	"spinitem",
	"revitem",
	"followitem",
	"actionspd",
	"mindash",
	"maxdash",
	"normalspeed",
	"runspeed",
	"thrustfactor",
	"accelstart",
	"acceleration",
	"jumpfactor",
	"radius",
	"height",
	"spinheight",
	"shieldscale",
	"camerascale",
	"starttranscolor",
	"prefcolor",
	"supercolor",
	"prefoppositecolor",
	"highresscale",
	"contspeed",
	"contangle",
	"soundsid",
	"availability",
	"sprites",
	NULL};

#define UNIMPLEMENTED luaL_error(L, LUA_QL("skin_t") " field " LUA_QS " is not implemented for Lua and cannot be accessed.", skin_opt[field])

static int skin_get(lua_State *L)
{
	skin_t *skin = *((skin_t **)luaL_checkudata(L, 1, META_SKIN));
	enum skin field = luaL_checkoption(L, 2, NULL, skin_opt);

	// skins are always valid, only added, never removed
	I_Assert(skin != NULL);

	switch (field)
	{
	case skin_valid:
		lua_pushboolean(L, skin != NULL);
		break;
	case skin_name:
		lua_pushstring(L, skin->name);
		break;
	case skin_wadnum:
		// !!WARNING!! May differ between clients due to music wads, therefore NOT NETWORK SAFE
		return UNIMPLEMENTED;
	case skin_flags:
		lua_pushinteger(L, skin->flags);
		break;
	case skin_realname:
		lua_pushstring(L, skin->realname);
		break;
	case skin_hudname:
		lua_pushstring(L, skin->hudname);
		break;
	case skin_ability:
		lua_pushinteger(L, skin->ability);
		break;
	case skin_ability2:
		lua_pushinteger(L, skin->ability2);
		break;
	case skin_thokitem:
		lua_pushinteger(L, skin->thokitem);
		break;
	case skin_spinitem:
		lua_pushinteger(L, skin->spinitem);
		break;
	case skin_revitem:
		lua_pushinteger(L, skin->revitem);
		break;
	case skin_followitem:
		lua_pushinteger(L, skin->followitem);
		break;
	case skin_actionspd:
		lua_pushfixed(L, skin->actionspd);
		break;
	case skin_mindash:
		lua_pushfixed(L, skin->mindash);
		break;
	case skin_maxdash:
		lua_pushfixed(L, skin->maxdash);
		break;
	case skin_normalspeed:
		lua_pushfixed(L, skin->normalspeed);
		break;
	case skin_runspeed:
		lua_pushfixed(L, skin->runspeed);
		break;
	case skin_thrustfactor:
		lua_pushinteger(L, skin->thrustfactor);
		break;
	case skin_accelstart:
		lua_pushinteger(L, skin->accelstart);
		break;
	case skin_acceleration:
		lua_pushinteger(L, skin->acceleration);
		break;
	case skin_jumpfactor:
		lua_pushfixed(L, skin->jumpfactor);
		break;
	case skin_radius:
		lua_pushfixed(L, skin->radius);
		break;
	case skin_height:
		lua_pushfixed(L, skin->height);
		break;
	case skin_spinheight:
		lua_pushfixed(L, skin->spinheight);
		break;
	case skin_shieldscale:
		lua_pushfixed(L, skin->shieldscale);
		break;
	case skin_camerascale:
		lua_pushfixed(L, skin->camerascale);
		break;
	case skin_starttranscolor:
		lua_pushinteger(L, skin->starttranscolor);
		break;
	case skin_prefcolor:
		lua_pushinteger(L, skin->prefcolor);
		break;
	case skin_supercolor:
		lua_pushinteger(L, skin->supercolor);
		break;
	case skin_prefoppositecolor:
		lua_pushinteger(L, skin->prefoppositecolor);
		break;
	case skin_highresscale:
		lua_pushinteger(L, skin->highresscale);
		break;
	case skin_contspeed:
		lua_pushinteger(L, skin->contspeed);
		break;
	case skin_contangle:
		lua_pushinteger(L, skin->contangle);
		break;
	case skin_soundsid:
		LUA_PushUserdata(L, skin->soundsid, META_SOUNDSID);
		break;
	case skin_availability:
		lua_pushinteger(L, skin->availability);
		break;
	case skin_sprites:
		LUA_PushUserdata(L, skin->sprites, META_SKINSPRITES);
		break;
	}
	return 1;
}

static int skin_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("skin_t") " struct cannot be edited by Lua.");
}

static int skin_num(lua_State *L)
{
	skin_t *skin = *((skin_t **)luaL_checkudata(L, 1, META_SKIN));

	// skins are always valid, only added, never removed
	I_Assert(skin != NULL);

	lua_pushinteger(L, skin-skins);
	return 1;
}

static int lib_iterateSkins(lua_State *L)
{
	INT32 i;

	if (lua_gettop(L) < 2)
	{
		//return luaL_error(L, "Don't call skins.iterate() directly, use it as 'for skin in skins.iterate do <block> end'.");
		lua_pushcfunction(L, lib_iterateSkins);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = (INT32)(*((skin_t **)luaL_checkudata(L, 1, META_SKIN)) - skins) + 1;
	else
		i = 0;

	// skins are always valid, only added, never removed
	if (i < numskins)
	{
		LUA_PushUserdata(L, &skins[i], META_SKIN);
		return 1;
	}

	return 0;
}

static int lib_getSkin(lua_State *L)
{
	const char *field;
	INT32 i;

	// find skin by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= MAXSKINS)
			return luaL_error(L, "skins[] index %d out of range (0 - %d)", i, MAXSKINS-1);
		if (i >= numskins)
			return 0;
		LUA_PushUserdata(L, &skins[i], META_SKIN);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateSkins);
		return 1;
	}

	// find skin by name
	for (i = 0; i < numskins; i++)
		if (fastcmp(skins[i].name, field))
		{
			LUA_PushUserdata(L, &skins[i], META_SKIN);
			return 1;
		}

	return 0;
}

static int lib_numSkins(lua_State *L)
{
	lua_pushinteger(L, numskins);
	return 1;
}

// soundsid, i -> soundsid[i]
static int soundsid_get(lua_State *L)
{
	sfxenum_t *soundsid = *((sfxenum_t **)luaL_checkudata(L, 1, META_SOUNDSID));
	skinsound_t i = luaL_checkinteger(L, 2);
	if (i >= NUMSKINSOUNDS)
		return luaL_error(L, LUA_QL("skinsound_t") " cannot be %u", i);
	lua_pushinteger(L, soundsid[i]);
	return 1;
}

// #soundsid -> NUMSKINSOUNDS
static int soundsid_num(lua_State *L)
{
	lua_pushinteger(L, NUMSKINSOUNDS);
	return 1;
}

enum spritesopt {
	numframes = 0
};

static const char *const sprites_opt[] = {
	"numframes",
	NULL};

// skin.sprites[i] -> sprites[i]
static int lib_getSkinSprite(lua_State *L)
{
	spritedef_t *sprites = *(spritedef_t **)luaL_checkudata(L, 1, META_SKINSPRITES);
	playersprite_t i = luaL_checkinteger(L, 2);

	if (i < 0 || i >= NUMPLAYERSPRITES*2)
		return luaL_error(L, LUA_QL("skin_t") " field 'sprites' index %d out of range (0 - %d)", i, (NUMPLAYERSPRITES*2)-1);

	LUA_PushUserdata(L, &sprites[i], META_SKINSPRITESLIST);
	return 1;
}

// #skin.sprites -> NUMPLAYERSPRITES*2
static int lib_numSkinsSprites(lua_State *L)
{
	lua_pushinteger(L, NUMPLAYERSPRITES*2);
	return 1;
}

static int sprite_get(lua_State *L)
{
	spritedef_t *sprite = *(spritedef_t **)luaL_checkudata(L, 1, META_SKINSPRITESLIST);
	enum spritesopt field = luaL_checkoption(L, 2, NULL, sprites_opt);

	switch (field)
	{
	case numframes:
		lua_pushinteger(L, sprite->numframes);
		break;
	}
	return 1;
}


int LUA_SkinLib(lua_State *L)
{
	luaL_newmetatable(L, META_SKIN);
		lua_pushcfunction(L, skin_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, skin_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, skin_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_SOUNDSID);
		lua_pushcfunction(L, soundsid_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, soundsid_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_SKINSPRITES);
		lua_pushcfunction(L, lib_getSkinSprite);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, lib_numSkinsSprites);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_SKINSPRITESLIST);
		lua_pushcfunction(L, sprite_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSkin);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numSkins);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "skins");

	return 0;
}
