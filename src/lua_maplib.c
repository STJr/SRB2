// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_maplib.c
/// \brief game map library for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "r_state.h"
#include "p_local.h"
#include "p_setup.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

static const char *const sector_opt[] = {
	"valid",
	"floorheight",
	"ceilingheight",
	"floorpic",
	"ceilingpic",
	"lightlevel",
	"special",
	"tag",
	NULL};

static const char *const subsector_opt[] = {
	"valid",
	"sector",
	"numlines",
	"firstline",
	"validcount",
	NULL};

static const char *const line_opt[] = {
	"valid",
	"v1",
	"v2",
	"dx",
	"dy",
	"flags",
	"special",
	"tag",
	"sidenum",
	"frontside",
	"backside",
	"slopetype",
	"frontsector",
	"backsector",
	"validcount",
	"firsttag",
	"nexttag",
	"text",
	NULL};

static const char *const side_opt[] = {
	"valid",
	"textureoffset",
	"rowoffset",
	"toptexture",
	"bottomtexture",
	"midtexture",
	"sector",
	"special",
	"repeatcnt",
	"text",
	NULL};

static const char *const vertex_opt[] = {
	"valid",
	"x",
	"y",
	"z",
	NULL};

static const char *const array_opt[] ={"iterate",NULL};
static const char *const valid_opt[] ={"valid",NULL};

static int sector_get(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	int field = luaL_checkoption(L, 2, sector_opt[0], sector_opt);

	if (!sector)
	{
		if (field == 0) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case 0: // valid
		lua_pushboolean(L, 1);
		return 1;
	case 1:
		lua_pushinteger(L, sector->floorheight);
		return 1;
	case 2:
		lua_pushinteger(L, sector->ceilingheight);
		return 1;
	case 3: { // floorpic
		levelflat_t *levelflat;
		INT16 i;
		for (i = 0, levelflat = levelflats; i != sector->floorpic; i++, levelflat++)
			;
		lua_pushlstring(L, levelflat->name, 8);
		return 1;
	}
	case 4: { // ceilingpic
		levelflat_t *levelflat;
		INT16 i;
		for (i = 0, levelflat = levelflats; i != sector->ceilingpic; i++, levelflat++)
			;
		lua_pushlstring(L, levelflat->name, 8);
		return 1;
	}
	case 5:
		lua_pushinteger(L, sector->lightlevel);
		return 1;
	case 6:
		lua_pushinteger(L, sector->special);
		return 1;
	case 7:
		lua_pushinteger(L, sector->tag);
		return 1;
	}
	return 0;
}

// help function for P_LoadSectors, find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
//
static INT32 P_AddLevelFlatRuntime(const char *flatname)
{
	size_t i;
	levelflat_t *levelflat = levelflats;

	//
	//  first scan through the already found flats
	//
	for (i = 0; i < numlevelflats; i++, levelflat++)
		if (strnicmp(levelflat->name,flatname,8)==0)
			break;

	// that flat was already found in the level, return the id
	if (i == numlevelflats)
	{
		// allocate new flat memory
		levelflats = Z_Realloc(levelflats, (numlevelflats + 1) * sizeof(*levelflats), PU_LEVEL, NULL);
		levelflat = levelflats+i;

		// store the name
		strlcpy(levelflat->name, flatname, sizeof (levelflat->name));
		strupr(levelflat->name);

		// store the flat lump number
		levelflat->lumpnum = R_GetFlatNumForName(flatname);

#ifndef ZDEBUG
		CONS_Debug(DBG_SETUP, "flat #%03d: %s\n", atoi(sizeu1(numlevelflats)), levelflat->name);
#endif

		numlevelflats++;
	}

	// level flat id
	return (INT32)i;
}

static int sector_set(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	int field = luaL_checkoption(L, 2, sector_opt[0], sector_opt);

	if (!sector)
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter sector_t in HUD rendering code!");

	switch(field)
	{
	case 0: // valid
	default:
		return luaL_error(L, "sector_t field " LUA_QS " cannot be set.", sector_opt[field]);
	case 1: { // floorheight
		boolean flag;
		fixed_t lastpos = sector->floorheight;
		sector->floorheight = (fixed_t)luaL_checkinteger(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->floorheight = lastpos;
			P_CheckSector(sector, true);
		}
		break;
	}
	case 2: { // ceilingheight
		boolean flag;
		fixed_t lastpos = sector->ceilingheight;
		sector->ceilingheight = (fixed_t)luaL_checkinteger(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->ceilingheight = lastpos;
			P_CheckSector(sector, true);
		}
		break;
	}
	case 3:
		sector->floorpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case 4:
		sector->ceilingpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case 5:
		sector->lightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case 6:
		sector->special = (INT16)luaL_checkinteger(L, 3);
		break;
	case 7:
		P_ChangeSectorTag((UINT32)(sector - sectors), (INT16)luaL_checkinteger(L, 3));
		break;
	}
	return 0;
}

static int sector_num(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	lua_pushinteger(L, sector-sectors);
	return 1;
}

static int subsector_get(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	int field = luaL_checkoption(L, 2, subsector_opt[0], subsector_opt);

	if (!subsector)
	{
		if (field == 0) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed subsector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case 0: // valid
		lua_pushboolean(L, 1);
		return 1;
	case 1:
		LUA_PushUserdata(L, subsector->sector, META_SECTOR);
		return 1;
	case 2:
		lua_pushinteger(L, subsector->numlines);
		return 1;
	case 3:
		lua_pushinteger(L, subsector->firstline);
		return 1;
	case 4:
		lua_pushinteger(L, subsector->validcount);
		return 1;
	}
	return 0;
}

static int subsector_num(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	lua_pushinteger(L, subsector-subsectors);
	return 1;
}

static int line_get(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	int field = luaL_checkoption(L, 2, line_opt[0], line_opt);

	if (!line)
	{
		if (field == 0) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed line_t doesn't exist anymore.");
	}

	switch(field)
	{
	case 0: // valid
		lua_pushboolean(L, 1);
		return 1;
	case 1:
		LUA_PushUserdata(L, line->v1, META_VERTEX);
		return 1;
	case 2:
		LUA_PushUserdata(L, line->v2, META_VERTEX);
		return 1;
	case 3:
		lua_pushinteger(L, line->dx);
		return 1;
	case 4:
		lua_pushinteger(L, line->dy);
		return 1;
	case 5:
		lua_pushinteger(L, line->flags);
		return 1;
	case 6:
		lua_pushinteger(L, line->special);
		return 1;
	case 7:
		lua_pushinteger(L, line->tag);
		return 1;
	case 8:
		LUA_PushUserdata(L, line->sidenum, META_SIDENUM);
		return 1;
	case 9: // frontside
		LUA_PushUserdata(L, &sides[line->sidenum[0]], META_SIDE);
		return 1;
	case 10: // backside
		if (line->sidenum[1] == 0xffff)
			return 0;
		LUA_PushUserdata(L, &sides[line->sidenum[1]], META_SIDE);
		return 1;
	case 11:
		switch(lines->slopetype)
		{
		case ST_HORIZONTAL:
			lua_pushliteral(L, "horizontal");
			break;
		case ST_VERTICAL:
			lua_pushliteral(L, "vertical");
			break;
		case ST_POSITIVE:
			lua_pushliteral(L, "positive");
			break;
		case ST_NEGATIVE:
			lua_pushliteral(L, "negative");
			break;
		}
		return 1;
	case 12:
		LUA_PushUserdata(L, line->frontsector, META_SECTOR);
		return 1;
	case 13:
		LUA_PushUserdata(L, line->backsector, META_SECTOR);
		return 1;
	case 14:
		lua_pushinteger(L, line->validcount);
		return 1;
	case 15:
		lua_pushinteger(L, line->firsttag);
		return 1;
	case 16:
		lua_pushinteger(L, line->nexttag);
		return 1;
	case 17:
		lua_pushstring(L, line->text);
		return 1;
	}
	return 0;
}

static int line_num(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	lua_pushinteger(L, line-lines);
	return 1;
}

static int sidenum_get(lua_State *L)
{
	UINT16 *sidenum = *((UINT16 **)luaL_checkudata(L, 1, META_SIDENUM));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		int field = luaL_checkoption(L, 2, NULL, valid_opt);
		if (!sidenum)
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed line_t doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	i = lua_tointeger(L, 2);
	if (i < 0 || i > 1)
		return 0;
	lua_pushinteger(L, sidenum[i]);
	return 1;
}

static int side_get(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	int field = luaL_checkoption(L, 2, side_opt[0], side_opt);

	if (!side)
	{
		if (field == 0) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case 0: // valid
		lua_pushboolean(L, 1);
		return 1;
	case 1:
		lua_pushinteger(L, side->textureoffset);
		return 1;
	case 2:
		lua_pushinteger(L, side->rowoffset);
		return 1;
	case 3:
		lua_pushinteger(L, side->toptexture);
		return 1;
	case 4:
		lua_pushinteger(L, side->bottomtexture);
		return 1;
	case 5:
		lua_pushinteger(L, side->midtexture);
		return 1;
	case 6:
		LUA_PushUserdata(L, side->sector, META_SECTOR);
		return 1;
	case 7:
		lua_pushinteger(L, side->special);
		return 1;
	case 8:
		lua_pushinteger(L, side->repeatcnt);
		return 1;
	case 9:
		lua_pushstring(L, side->text);
		return 1;
	}
	return 0;
}

static int side_num(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	lua_pushinteger(L, side-sides);
	return 1;
}

static int vertex_get(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	int field = luaL_checkoption(L, 2, vertex_opt[0], vertex_opt);

	if (!vertex)
	{
		if (field == 0) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed vertex_t doesn't exist anymore.");
	}

	switch(field)
	{
	case 0: // valid
		lua_pushboolean(L, 1);
		return 1;
	case 1:
		lua_pushinteger(L, vertex->x);
		return 1;
	case 2:
		lua_pushinteger(L, vertex->y);
		return 1;
	case 3:
		lua_pushinteger(L, vertex->z);
		return 1;
	}
	return 0;
}

static int vertex_num(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	lua_pushinteger(L, vertex-vertexes);
	return 1;
}

static int lib_iterateSectors(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sectors.iterate() directly, use it as 'for sector in sectors.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((sector_t **)luaL_checkudata(L, 1, META_SECTOR)) - sectors)+1;
	if (i < numsectors)
	{
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	return 0;
}

static int lib_getSector(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsectors)
			return 0;
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSectors);
		return 1;
	}
	return 0;
}

static int lib_numsectors(lua_State *L)
{
	lua_pushinteger(L, numsectors);
	return 1;
}

static int lib_iterateSubsectors(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call subsectors.iterate() directly, use it as 'for subsector in subsectors.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR)) - subsectors)+1;
	if (i < numsubsectors)
	{
		LUA_PushUserdata(L, &subsectors[i], META_SUBSECTOR);
		return 1;
	}
	return 0;
}

static int lib_getSubsector(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsubsectors)
			return 0;
		LUA_PushUserdata(L, &subsectors[i], META_SUBSECTOR);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSubsectors);
		return 1;
	}
	return 0;
}

static int lib_numsubsectors(lua_State *L)
{
	lua_pushinteger(L, numsubsectors);
	return 1;
}

static int lib_iterateLines(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call lines.iterate() directly, use it as 'for line in lines.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((line_t **)luaL_checkudata(L, 1, META_LINE)) - lines)+1;
	if (i < numlines)
	{
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	return 0;
}

static int lib_getLine(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numlines)
			return 0;
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateLines);
		return 1;
	}
	return 0;
}

static int lib_numlines(lua_State *L)
{
	lua_pushinteger(L, numlines);
	return 1;
}

static int lib_iterateSides(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sides.iterate() directly, use it as 'for side in sides.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((side_t **)luaL_checkudata(L, 1, META_SIDE)) - sides)+1;
	if (i < numsides)
	{
		LUA_PushUserdata(L, &sides[i], META_SIDE);
		return 1;
	}
	return 0;
}

static int lib_getSide(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsides)
			return 0;
		LUA_PushUserdata(L, &sides[i], META_SIDE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSides);
		return 1;
	}
	return 0;
}

static int lib_numsides(lua_State *L)
{
	lua_pushinteger(L, numsides);
	return 1;
}

static int lib_iterateVertexes(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call vertexes.iterate() directly, use it as 'for vertex in vertexes.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((vertex_t **)luaL_checkudata(L, 1, META_VERTEX)) - vertexes)+1;
	if (i < numvertexes)
	{
		LUA_PushUserdata(L, &vertexes[i], META_VERTEX);
		return 1;
	}
	return 0;
}

static int lib_getVertex(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numvertexes)
			return 0;
		LUA_PushUserdata(L, &vertexes[i], META_VERTEX);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateVertexes);
		return 1;
	}
	return 0;
}

static int lib_numvertexes(lua_State *L)
{
	lua_pushinteger(L, numvertexes);
	return 1;
}

int LUA_MapLib(lua_State *L)
{
	luaL_newmetatable(L, META_SECTOR);
		lua_pushcfunction(L, sector_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sector_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, sector_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SUBSECTOR);
		lua_pushcfunction(L, subsector_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, subsector_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_LINE);
		lua_pushcfunction(L, line_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, line_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SIDENUM);
		lua_pushcfunction(L, sidenum_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SIDE);
		lua_pushcfunction(L, side_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, side_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_VERTEX);
		lua_pushcfunction(L, vertex_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, vertex_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSector);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsectors);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "sectors");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSubsector);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsubsectors);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "subsectors");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getLine);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numlines);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "lines");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSide);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsides);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "sides");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getVertex);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numvertexes);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "vertexes");
	return 0;
}

#endif
