// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2016-2020 by Iestyn "Monster Iestyn" Jealous.
// Copyright (C) 2016-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_blockmaplib.c
/// \brief blockmap library for Lua scripting

#include "doomdef.h"
#include "p_local.h"
#include "r_main.h" // validcount
#include "p_polyobj.h"
#include "lua_script.h"
#include "lua_libs.h"
//#include "lua_hud.h" // hud_running errors

static const char *const search_opt[] = {
	"objects",
	"lines",
	"polyobjs",
	NULL};

// a quickly-made function pointer typedef used by lib_searchBlockmap...
// return values:
// 0 - normal, no interruptions
// 1 - stop search through current block
// 2 - stop search completely
typedef UINT8 (*blockmap_func)(lua_State *, INT32, INT32, mobj_t *);

static boolean blockfuncerror = false; // errors should only print once per search blockmap call

// Helper function for "objects" search
static UINT8 lib_searchBlockmap_Objects(lua_State *L, INT32 x, INT32 y, mobj_t *thing)
{
	mobj_t *mobj, *bnext = NULL;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return 0;

	// Check interaction with the objects in the blockmap.
	for (mobj = blocklinks[y*bmapwidth + x]; mobj; mobj = bnext)
	{
		P_SetTarget(&bnext, mobj->bnext); // We want to note our reference to bnext here incase it is MF_NOTHINK and gets removed!
		if (mobj == thing)
			continue; // our thing just found itself, so move on
		lua_pushvalue(L, 1); // push function
		LUA_PushUserdata(L, thing, META_MOBJ);
		LUA_PushUserdata(L, mobj, META_MOBJ);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!blockfuncerror || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			blockfuncerror = true;
			P_SetTarget(&bnext, NULL);
			return 0; // *shrugs*
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, continue
			P_SetTarget(&bnext, NULL);
			if (lua_toboolean(gL, -1))
				return 2; // stop whole search
			else
				return 1; // stop block search
		}
		lua_pop(gL, 1);
		if (P_MobjWasRemoved(thing) // func just popped our thing, cannot continue.
		|| (bnext && P_MobjWasRemoved(bnext))) // func just broke blockmap chain, cannot continue.
		{
			P_SetTarget(&bnext, NULL);
			return (P_MobjWasRemoved(thing)) ? 2 : 1;
		}
	}
	return 0;
}

// Helper function for "lines" search
static UINT8 lib_searchBlockmap_Lines(lua_State *L, INT32 x, INT32 y, mobj_t *thing)
{
	INT32 offset;
	const INT32 *list; // Big blockmap
	polymaplink_t *plink; // haleyjd 02/22/06
	line_t *ld;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return 0;

	offset = y*bmapwidth + x;

	// haleyjd 02/22/06: consider polyobject lines
	plink = polyblocklinks[offset];

	while (plink)
	{
		polyobj_t *po = plink->po;

		if (po->validcount != validcount) // if polyobj hasn't been checked
		{
			size_t i;
			po->validcount = validcount;

			for (i = 0; i < po->numLines; ++i)
			{
				if (po->lines[i]->validcount == validcount) // line has been checked
					continue;
				po->lines[i]->validcount = validcount;

				lua_pushvalue(L, 1);
				LUA_PushUserdata(L, thing, META_MOBJ);
				LUA_PushUserdata(L, po->lines[i], META_LINE);
				if (lua_pcall(gL, 2, 1, 0)) {
					if (!blockfuncerror || cv_debug & DBG_LUA)
						CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
					lua_pop(gL, 1);
					blockfuncerror = true;
					return 0; // *shrugs*
				}
				if (!lua_isnil(gL, -1))
				{ // if nil, continue
					if (lua_toboolean(gL, -1))
						return 2; // stop whole search
					else
						return 1; // stop block search
				}
				lua_pop(gL, 1);
				if (P_MobjWasRemoved(thing))
					return 2;
			}
		}
		plink = (polymaplink_t *)(plink->link.next);
	}

	offset = *(blockmap + offset); // offset = blockmap[y*bmapwidth+x];

	// First index is really empty, so +1 it.
	for (list = blockmaplump + offset + 1; *list != -1; list++)
	{
		ld = &lines[*list];

		if (ld->validcount == validcount)
			continue; // Line has already been checked.

		ld->validcount = validcount;

		lua_pushvalue(L, 1);
		LUA_PushUserdata(L, thing, META_MOBJ);
		LUA_PushUserdata(L, ld, META_LINE);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!blockfuncerror || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			blockfuncerror = true;
			return 0; // *shrugs*
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, continue
			if (lua_toboolean(gL, -1))
				return 2; // stop whole search
			else
				return 1; // stop block search
		}
		lua_pop(gL, 1);
		if (P_MobjWasRemoved(thing))
			return 2;
	}
	return 0; // Everything was checked.
}

// Helper function for "polyobjs" search
static UINT8 lib_searchBlockmap_PolyObjs(lua_State *L, INT32 x, INT32 y, mobj_t *thing)
{
	INT32 offset;
	polymaplink_t *plink; // haleyjd 02/22/06

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return 0;

	offset = y*bmapwidth + x;

	// haleyjd 02/22/06: consider polyobject lines
	plink = polyblocklinks[offset];

	while (plink)
	{
		polyobj_t *po = plink->po;

		if (po->validcount != validcount) // if polyobj hasn't been checked
		{
			po->validcount = validcount;

			lua_pushvalue(L, 1);
			LUA_PushUserdata(L, thing, META_MOBJ);
			LUA_PushUserdata(L, po, META_POLYOBJ);
			if (lua_pcall(gL, 2, 1, 0)) {
				if (!blockfuncerror || cv_debug & DBG_LUA)
					CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
				lua_pop(gL, 1);
				blockfuncerror = true;
				return 0; // *shrugs*
			}
			if (!lua_isnil(gL, -1))
			{ // if nil, continue
				if (lua_toboolean(gL, -1))
					return 2; // stop whole search
				else
					return 1; // stop block search
			}
			lua_pop(gL, 1);
			if (P_MobjWasRemoved(thing))
				return 2;
		}
		plink = (polymaplink_t *)(plink->link.next);
	}

	return 0; // Everything was checked.
}

// The searchBlockmap function
// arguments: searchBlockmap(searchtype, function, mobj, [x1, x2, y1, y2])
// return value:
//   true = search completely uninteruppted,
//   false = searching of at least one block stopped mid-way (including if the whole search was stopped)
static int lib_searchBlockmap(lua_State *L)
{
	int searchtype = luaL_checkoption(L, 1, "objects", search_opt);
	int n;
	mobj_t *mobj;
	INT32 xl, xh, yl, yh, bx, by;
	fixed_t x1, x2, y1, y2;
	boolean retval = true;
	UINT8 funcret = 0;
	blockmap_func searchFunc;

	lua_remove(L, 1); // remove searchtype, stack is now function, mobj, [x1, x2, y1, y2]
	luaL_checktype(L, 1, LUA_TFUNCTION);

	switch (searchtype)
	{
		case 0: // "objects"
		default:
			searchFunc = lib_searchBlockmap_Objects;
			break;
		case 1: // "lines"
			searchFunc = lib_searchBlockmap_Lines;
			break;
		case 2: // "polyobjs"
			searchFunc = lib_searchBlockmap_PolyObjs;
			break;
	}

	// the mobj we are searching around, the "calling" mobj we could say
	mobj = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");

	n = lua_gettop(L);

	if (n > 2) // specific x/y ranges have been supplied
	{
		if (n < 6)
			return luaL_error(L, "arguments 4 to 6 not all given (expected 4 fixed-point integers)");

		x1 = luaL_checkfixed(L, 3);
		x2 = luaL_checkfixed(L, 4);
		y1 = luaL_checkfixed(L, 5);
		y2 = luaL_checkfixed(L, 6);
	}
	else // mobj and function only - search around mobj's radius by default
	{
		fixed_t radius = mobj->radius + MAXRADIUS;
		x1 = mobj->x - radius;
		x2 = mobj->x + radius;
		y1 = mobj->y - radius;
		y2 = mobj->y + radius;
	}
	lua_settop(L, 2); // pop everything except function, mobj

	xl = (unsigned)(x1 - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(x2 - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(y1 - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(y2 - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	blockfuncerror = false; // reset
	validcount++;
	for (bx = xl; bx <= xh; bx++)
		for (by = yl; by <= yh; by++)
		{
			funcret = searchFunc(L, bx, by, mobj);
			// return value of searchFunc determines searchFunc's return value and/or when to stop
			if (funcret == 2){ // stop whole search
				lua_pushboolean(L, false); // return false
				return 1;
			}
			else if (funcret == 1) // search was interrupted for this block
				retval = false; // this changes the return value, but doesn't stop the whole search
			// else don't do anything, continue as normal
			if (P_MobjWasRemoved(mobj)){ // ...unless the original object was removed
				lua_pushboolean(L, false); // in which case we have to stop now regardless
				return 1;
			}
		}
	lua_pushboolean(L, retval);
	return 1;
}

int LUA_BlockmapLib(lua_State *L)
{
	lua_register(L, "searchBlockmap", lib_searchBlockmap);
	return 0;
}
