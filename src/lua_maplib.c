// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_maplib.c
/// \brief game map library for Lua scripting

#include "doomdef.h"
#include "r_state.h"
#include "p_local.h"
#include "p_setup.h"
#include "z_zone.h"
#include "p_slopes.h"
#include "p_polyobj.h"
#include "r_main.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "lua_hook.h" // hook_cmd_running errors

#include "dehacked.h"
#include "fastcmp.h"
#include "doomstat.h"

enum sector_e {
	sector_valid = 0,
	sector_floorheight,
	sector_ceilingheight,
	sector_floorpic,
	sector_floorxoffset,
	sector_flooryoffset,
	sector_floorxscale,
	sector_flooryscale,
	sector_floorangle,
	sector_ceilingpic,
	sector_ceilingxoffset,
	sector_ceilingyoffset,
	sector_ceilingxscale,
	sector_ceilingyscale,
	sector_ceilingangle,
	sector_lightlevel,
	sector_floorlightlevel,
	sector_floorlightabsolute,
	sector_floorlightsec,
	sector_ceilinglightlevel,
	sector_ceilinglightabsolute,
	sector_ceilinglightsec,
	sector_special,
	sector_tag,
	sector_taglist,
	sector_thinglist,
	sector_heightsec,
	sector_camsec,
	sector_lines,
	sector_ffloors,
	sector_fslope,
	sector_cslope,
	sector_colormap,
	sector_flags,
	sector_specialflags,
	sector_damagetype,
	sector_triggertag,
	sector_triggerer,
	sector_friction,
	sector_gravity,
};

static const char *const sector_opt[] = {
	"valid",
	"floorheight",
	"ceilingheight",
	"floorpic",
	"floorxoffset",
	"flooryoffset",
	"floorxscale",
	"flooryscale",
	"floorangle",
	"ceilingpic",
	"ceilingxoffset",
	"ceilingyoffset",
	"ceilingxscale",
	"ceilingyscale",
	"ceilingangle",
	"lightlevel",
	"floorlightlevel",
	"floorlightabsolute",
	"floorlightsec",
	"ceilinglightlevel",
	"ceilinglightabsolute",
	"ceilinglightsec",
	"special",
	"tag",
	"taglist",
	"thinglist",
	"heightsec",
	"camsec",
	"lines",
	"ffloors",
	"f_slope",
	"c_slope",
	"colormap",
	"flags",
	"specialflags",
	"damagetype",
	"triggertag",
	"triggerer",
	"friction",
	"gravity",
	NULL};

static int sector_fields_ref = LUA_NOREF;

enum subsector_e {
	subsector_valid = 0,
	subsector_sector,
	subsector_numlines,
	subsector_firstline,
	subsector_polyList
};

static const char *const subsector_opt[] = {
	"valid",
	"sector",
	"numlines",
	"firstline",
	"polyList",
	NULL};

static int subsector_fields_ref = LUA_NOREF;

enum line_e {
	line_valid = 0,
	line_v1,
	line_v2,
	line_dx,
	line_dy,
	line_angle,
	line_flags,
	line_special,
	line_tag,
	line_taglist,
	line_args,
	line_stringargs,
	line_sidenum,
	line_frontside,
	line_backside,
	line_alpha,
	line_executordelay,
	line_slopetype,
	line_frontsector,
	line_backsector,
	line_polyobj,
	line_text,
	line_callcount
};

static const char *const line_opt[] = {
	"valid",
	"v1",
	"v2",
	"dx",
	"dy",
	"angle",
	"flags",
	"special",
	"tag",
	"taglist",
	"args",
	"stringargs",
	"sidenum",
	"frontside",
	"backside",
	"alpha",
	"executordelay",
	"slopetype",
	"frontsector",
	"backsector",
	"polyobj",
	"text",
	"callcount",
	NULL};

static int line_fields_ref = LUA_NOREF;

enum side_e {
	side_valid = 0,
	side_textureoffset,
	side_rowoffset,
	side_offsetx_top,
	side_offsety_top,
	side_offsetx_mid,
	side_offsety_mid,
	side_offsetx_bottom,
	side_offsetx_bot,
	side_offsety_bottom,
	side_offsety_bot,
	side_scalex_top,
	side_scaley_top,
	side_scalex_mid,
	side_scaley_mid,
	side_scalex_bottom,
	side_scaley_bottom,
	side_toptexture,
	side_bottomtexture,
	side_midtexture,
	side_line,
	side_sector,
	side_special,
	side_repeatcnt,
	side_light,
	side_light_top,
	side_light_mid,
	side_light_bottom,
	side_lightabsolute,
	side_lightabsolute_top,
	side_lightabsolute_mid,
	side_lightabsolute_bottom,
	side_text
};

static const char *const side_opt[] = {
	"valid",
	"textureoffset",
	"rowoffset",
	"offsetx_top",
	"offsety_top",
	"offsetx_mid",
	"offsety_mid",
	"offsetx_bottom",
	"offsetx_bot",
	"offsety_bottom",
	"offsety_bot",
	"scalex_top",
	"scaley_top",
	"scalex_mid",
	"scaley_mid",
	"scalex_bottom",
	"scaley_bottom",
	"toptexture",
	"bottomtexture",
	"midtexture",
	"line",
	"sector",
	"special",
	"repeatcnt",
	"light",
	"light_top",
	"light_mid",
	"light_bottom",
	"lightabsolute",
	"lightabsolute_top",
	"lightabsolute_mid",
	"lightabsolute_bottom",
	"text",
	NULL};

static int side_fields_ref = LUA_NOREF;

enum vertex_e {
	vertex_valid = 0,
	vertex_x,
	vertex_y,
	vertex_floorz,
	vertex_floorzset,
	vertex_ceilingz,
	vertex_ceilingzset
};

static const char *const vertex_opt[] = {
	"valid",
	"x",
	"y",
	"floorz",
	"floorzset",
	"ceilingz",
	"ceilingzset",
	NULL};

static int vertex_fields_ref = LUA_NOREF;

enum ffloor_e {
	ffloor_valid = 0,
	ffloor_topheight,
	ffloor_toppic,
	ffloor_toplightlevel,
	ffloor_topxoffs,
	ffloor_topyoffs,
	ffloor_topxscale,
	ffloor_topyscale,
	ffloor_bottomheight,
	ffloor_bottompic,
	ffloor_bottomxoffs,
	ffloor_bottomyoffs,
	ffloor_bottomxscale,
	ffloor_bottomyscale,
	ffloor_tslope,
	ffloor_bslope,
	ffloor_sector,
	ffloor_fofflags,
	ffloor_flags,
	ffloor_master,
	ffloor_target,
	ffloor_next,
	ffloor_prev,
	ffloor_alpha,
	ffloor_blend,
	ffloor_bustflags,
	ffloor_busttype,
	ffloor_busttag,
	ffloor_sinkspeed,
	ffloor_friction,
	ffloor_bouncestrength,
};

static const char *const ffloor_opt[] = {
	"valid",
	"topheight",
	"toppic",
	"toplightlevel",
	"topxoffs",
	"topyoffs",
	"topxscale",
	"topyscale",
	"bottomheight",
	"bottompic",
	"bottomxoffs",
	"bottomyoffs",
	"bottomxscale",
	"bottomyscale",
	"t_slope",
	"b_slope",
	"sector", // secnum pushed as control sector userdata
	"fofflags",
	"flags",
	"master", // control linedef
	"target", // target sector
	"next",
	"prev",
	"alpha",
	"blend",
	"bustflags",
	"busttype",
	"busttag",
	"sinkspeed",
	"friction",
	"bouncestrength",
	NULL};

static int ffloor_fields_ref = LUA_NOREF;

#ifdef HAVE_LUA_SEGS
enum seg_e {
	seg_valid = 0,
	seg_v1,
	seg_v2,
	seg_side,
	seg_offset,
	seg_angle,
	seg_sidedef,
	seg_linedef,
	seg_frontsector,
	seg_backsector,
	seg_polyseg
};

static const char *const seg_opt[] = {
	"valid",
	"v1",
	"v2",
	"side",
	"offset",
	"angle",
	"sidedef",
	"linedef",
	"frontsector",
	"backsector",
	"polyseg",
	NULL};

static int seg_fields_ref = LUA_NOREF;

enum node_e {
	node_valid = 0,
	node_x,
	node_y,
	node_dx,
	node_dy,
	node_bbox,
	node_children,
};

static const char *const node_opt[] = {
	"valid",
	"x",
	"y",
	"dx",
	"dy",
	"bbox",
	"children",
	NULL};

static int node_fields_ref = LUA_NOREF;

enum nodechild_e {
	nodechild_valid = 0,
	nodechild_right,
	nodechild_left,
};

static const char *const nodechild_opt[] = {
	"valid",
	"right",
	"left",
	NULL};
#endif

enum bbox_e {
	bbox_valid = 0,
	bbox_top,
	bbox_bottom,
	bbox_left,
	bbox_right,
};

static const char *const bbox_opt[] = {
	"valid",
	"top",
	"bottom",
	"left",
	"right",
	NULL};

enum slope_e {
	slope_valid = 0,
	slope_o,
	slope_d,
	slope_zdelta,
	slope_normal,
	slope_zangle,
	slope_xydirection,
	slope_flags
};

static const char *const slope_opt[] = {
	"valid",
	"o",
	"d",
	"zdelta",
	"normal",
	"zangle",
	"xydirection",
	"flags",
	NULL};

static int slope_fields_ref = LUA_NOREF;

// shared by both vector2_t and vector3_t
enum vector_e {
	vector_x = 0,
	vector_y,
	vector_z
};

static const char *const vector_opt[] = {
	"x",
	"y",
	"z",
	NULL};

static const char *const array_opt[] ={"iterate",NULL};
static const char *const valid_opt[] ={"valid",NULL};

/////////////////////////////////////////////
// sector/subsector list iterate functions //
/////////////////////////////////////////////

// iterates through a sector's thinglist!
static int lib_iterateSectorThinglist(lua_State *L)
{
	mobj_t *state = NULL;
	mobj_t *thing = NULL;

	INLEVEL

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sector.thinglist() directly, use it as 'for rover in sector.thinglist do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	else
		return 0; // no thinglist to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (P_MobjWasRemoved(thing))
			return luaL_error(L, "current entry in thinglist was removed; avoid calling P_RemoveMobj on entries!");
		thing = thing->snext;
	}
	else
		thing = state; // state is used as the "start" of the thinglist

	if (thing)
	{
		LUA_PushUserdata(L, thing, META_MOBJ);
		return 1;
	}
	return 0;
}

// iterates through the ffloors list in a sector!
static int lib_iterateSectorFFloors(lua_State *L)
{
	ffloor_t *state = NULL;
	ffloor_t *ffloor = NULL;

	INLEVEL

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sector.ffloors() directly, use it as 'for rover in sector.ffloors do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	else
		return 0; // no ffloors to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
		ffloor = ffloor->next;
	}
	else
		ffloor = state; // state is used as the "start" of the ffloor list

	if (ffloor)
	{
		LUA_PushUserdata(L, ffloor, META_FFLOOR);
		return 1;
	}
	return 0;
}

// iterates through a subsector's polyList! (for polyobj_t)
static int lib_iterateSubSectorPolylist(lua_State *L)
{
	polyobj_t *state = NULL;
	polyobj_t *po = NULL;

	INLEVEL

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call subsector.polyList() directly, use it as 'for polyobj in subsector.polyList do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((polyobj_t **)luaL_checkudata(L, 1, META_POLYOBJ));
	else
		return 0; // no polylist to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		po = *((polyobj_t **)luaL_checkudata(L, 1, META_POLYOBJ));
		po = (polyobj_t *)(po->link.next);
	}
	else
		po = state; // state is used as the "start" of the polylist

	if (po)
	{
		LUA_PushUserdata(L, po, META_POLYOBJ);
		return 1;
	}
	return 0;
}

static int sector_iterate(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1)); // iterator function, or the "generator"
	lua_pushvalue(L, lua_upvalueindex(2)); // state (used as the "start" of the list for our purposes
	lua_pushnil(L); // initial value (unused)
	return 3;
}

////////////////////
// sector.lines[] //
////////////////////

// sector.lines, i -> sector.lines[i]
// sector.lines.valid, for validity checking
//
// 25/9/19 Monster Iestyn
// Modified this and _num to use triple pointers, to allow for a new hack of mine involving offsetof
// this way we don't need to check frontsector or backsector of line #0 in the array
//
static int sectorlines_get(lua_State *L)
{
	line_t ***seclines = *((line_t ****)luaL_checkudata(L, 1, META_SECTORLINES));
	size_t i;
	size_t numoflines = 0;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		int field = luaL_checkoption(L, 2, NULL, valid_opt);
		if (!seclines || !(*seclines))
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed sector_t.lines doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

/* a snip from sector_t struct in r_defs.h, for reference
	size_t linecount;
	struct line_s **lines; // [linecount] size
*/
	// get the "linecount" by shifting our retrieved memory address of "lines" to where "linecount" is in the sector_t, then dereferencing the result
	// we need this to determine the array's actual size, and therefore also the maximum value allowed as an index
	// this only works if seclines is actually a pointer to a sector's lines member in memory, oh boy
	numoflines = *(size_t *)FIELDFROM (sector_t, seclines, lines,/* -> */linecount);

/* OLD HACK
	// check first linedef to figure which of its sectors owns this sector->lines pointer
	// then check that sector's linecount to get a maximum index
	//if (!(*seclines)[0])
		//return luaL_error(L, "no lines found!"); // no first linedef?????
	if ((*seclines)[0]->frontsector->lines == *seclines)
		numoflines = (*seclines)[0]->frontsector->linecount;
	else if ((*seclines)[0]->backsector && *seclines[0]->backsector->lines == *seclines) // check backsector exists first
		numoflines = (*seclines)[0]->backsector->linecount;
	//if neither sector has it then ???
*/

	if (!numoflines)
		return luaL_error(L, "no lines found!");

	i = (size_t)lua_tointeger(L, 2);
	if (i >= numoflines)
		return 0;
	LUA_PushUserdata(L, (*seclines)[i], META_LINE);
	return 1;
}

// #(sector.lines) -> sector.linecount
static int sectorlines_num(lua_State *L)
{
	line_t ***seclines = *((line_t ****)luaL_checkudata(L, 1, META_SECTORLINES));
	size_t numoflines = 0;

	if (!seclines || !(*seclines))
		return luaL_error(L, "accessed sector_t.lines doesn't exist anymore.");

	// see comments in the _get function above
	numoflines = *(size_t *)FIELDFROM (sector_t, seclines, lines,/* -> */linecount);
	lua_pushinteger(L, numoflines);
	return 1;
}

//////////////
// sector_t //
//////////////

static int sector_get(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	enum sector_e field = Lua_optoption(L, 2, sector_valid, sector_fields_ref);
	INT16 i;

	if (!sector)
	{
		if (field == sector_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case sector_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case sector_floorheight:
		lua_pushfixed(L, sector->floorheight);
		return 1;
	case sector_ceilingheight:
		lua_pushfixed(L, sector->ceilingheight);
		return 1;
	case sector_floorpic: // floorpic
	{
		levelflat_t *levelflat = &levelflats[sector->floorpic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case sector_floorxoffset:
		lua_pushfixed(L, sector->floorxoffset);
		return 1;
	case sector_flooryoffset:
		lua_pushfixed(L, sector->flooryoffset);
		return 1;
	case sector_floorxscale:
		lua_pushfixed(L, sector->floorxscale);
		return 1;
	case sector_flooryscale:
		lua_pushfixed(L, sector->flooryscale);
		return 1;
	case sector_floorangle:
		lua_pushangle(L, sector->floorangle);
		return 1;
	case sector_ceilingpic: // ceilingpic
	{
		levelflat_t *levelflat = &levelflats[sector->ceilingpic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case sector_ceilingxoffset:
		lua_pushfixed(L, sector->ceilingxoffset);
		return 1;
	case sector_ceilingyoffset:
		lua_pushfixed(L, sector->ceilingyoffset);
		return 1;
	case sector_ceilingxscale:
		lua_pushfixed(L, sector->ceilingxscale);
		return 1;
	case sector_ceilingyscale:
		lua_pushfixed(L, sector->ceilingyscale);
		return 1;
	case sector_ceilingangle:
		lua_pushangle(L, sector->ceilingangle);
		return 1;
	case sector_lightlevel:
		lua_pushinteger(L, sector->lightlevel);
		return 1;
	case sector_floorlightlevel:
		lua_pushinteger(L, sector->floorlightlevel);
		return 1;
	case sector_floorlightabsolute:
		lua_pushboolean(L, sector->floorlightabsolute);
		return 1;
	case sector_floorlightsec:
		lua_pushinteger(L, sector->floorlightsec);
		return 1;
	case sector_ceilinglightlevel:
		lua_pushinteger(L, sector->ceilinglightlevel);
		return 1;
	case sector_ceilinglightabsolute:
		lua_pushboolean(L, sector->ceilinglightabsolute);
		return 1;
	case sector_ceilinglightsec:
		lua_pushinteger(L, sector->ceilinglightsec);
		return 1;
	case sector_special:
		lua_pushinteger(L, sector->special);
		return 1;
	case sector_tag:
		lua_pushinteger(L, (UINT16)Tag_FGet(&sector->tags));
		return 1;
	case sector_taglist:
		LUA_PushUserdata(L, &sector->tags, META_SECTORTAGLIST);
		return 1;
	case sector_thinglist: // thinglist
		lua_pushcfunction(L, lib_iterateSectorThinglist);
		LUA_PushUserdata(L, sector->thinglist, META_MOBJ);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateSectorThinglist and sector->thinglist as upvalues for the function
		return 1;
	case sector_heightsec: // heightsec - fake floor heights
		if (sector->heightsec < 0)
			return 0;
		LUA_PushUserdata(L, &sectors[sector->heightsec], META_SECTOR);
		return 1;
	case sector_camsec: // camsec - camera clipping heights
		if (sector->camsec < 0)
			return 0;
		LUA_PushUserdata(L, &sectors[sector->camsec], META_SECTOR);
		return 1;
	case sector_lines: // lines
		LUA_PushUserdata(L, &sector->lines, META_SECTORLINES); // push the address of the "lines" member in the struct, to allow our hacks in sectorlines_get/_num to work
		return 1;
	case sector_ffloors: // ffloors
		lua_pushcfunction(L, lib_iterateSectorFFloors);
		LUA_PushUserdata(L, sector->ffloors, META_FFLOOR);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateFFloors and sector->ffloors as upvalues for the function
		return 1;
	case sector_fslope: // f_slope
		LUA_PushUserdata(L, sector->f_slope, META_SLOPE);
		return 1;
	case sector_cslope: // c_slope
		LUA_PushUserdata(L, sector->c_slope, META_SLOPE);
		return 1;
	case sector_colormap: // extra_colormap
		LUA_PushUserdata(L, sector->extra_colormap, META_EXTRACOLORMAP);
		return 1;
	case sector_flags: // flags
		lua_pushinteger(L, sector->flags);
		return 1;
	case sector_specialflags: // specialflags
		lua_pushinteger(L, sector->specialflags);
		return 1;
	case sector_damagetype: // damagetype
		lua_pushinteger(L, (UINT8)sector->damagetype);
		return 1;
	case sector_triggertag: // triggertag
		lua_pushinteger(L, (INT16)sector->triggertag);
		return 1;
	case sector_triggerer: // triggerer
		lua_pushinteger(L, (UINT8)sector->triggerer);
		return 1;
	case sector_friction: // friction
		lua_pushfixed(L, sector->friction);
		return 1;
	case sector_gravity: // gravity
		lua_pushfixed(L, sector->gravity);
		return 1;
	}
	return 0;
}

static int sector_set(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	enum sector_e field = Lua_optoption(L, 2, sector_valid, sector_fields_ref);

	if (!sector)
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter sector_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter sector_t in CMD building code!");

	switch(field)
	{
	case sector_valid: // valid
	case sector_thinglist: // thinglist
	case sector_heightsec: // heightsec
	case sector_camsec: // camsec
	case sector_lines: // lines
	case sector_ffloors: // ffloors
	case sector_fslope: // f_slope
	case sector_cslope: // c_slope
	case sector_friction: // friction
		return luaL_error(L, "sector_t field " LUA_QS " cannot be set.", sector_opt[field]);
	default:
		return luaL_error(L, "sector_t has no field named " LUA_QS ".", lua_tostring(L, 2));
	case sector_floorheight: { // floorheight
		boolean flag;
		mobj_t *ptmthing = tmthing;
		fixed_t lastpos = sector->floorheight;
		sector->floorheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->floorheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case sector_ceilingheight: { // ceilingheight
		boolean flag;
		mobj_t *ptmthing = tmthing;
		fixed_t lastpos = sector->ceilingheight;
		sector->ceilingheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->ceilingheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case sector_floorpic:
		sector->floorpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case sector_floorxoffset:
		sector->floorxoffset = luaL_checkfixed(L, 3);
		break;
	case sector_flooryoffset:
		sector->flooryoffset = luaL_checkfixed(L, 3);
		break;
	case sector_floorxscale:
		sector->floorxscale = luaL_checkfixed(L, 3);
		break;
	case sector_flooryscale:
		sector->flooryscale = luaL_checkfixed(L, 3);
		break;
	case sector_floorangle:
		sector->floorangle = luaL_checkangle(L, 3);
		break;
	case sector_ceilingpic:
		sector->ceilingpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case sector_ceilingxoffset:
		sector->ceilingxoffset = luaL_checkfixed(L, 3);
		break;
	case sector_ceilingyoffset:
		sector->ceilingyoffset = luaL_checkfixed(L, 3);
		break;
	case sector_ceilingxscale:
		sector->ceilingxscale = luaL_checkfixed(L, 3);
		break;
	case sector_ceilingyscale:
		sector->ceilingyscale = luaL_checkfixed(L, 3);
		break;
	case sector_ceilingangle:
		sector->ceilingangle = luaL_checkangle(L, 3);
		break;
	case sector_lightlevel:
		sector->lightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_floorlightlevel:
		sector->floorlightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_floorlightabsolute:
		sector->floorlightabsolute = luaL_checkboolean(L, 3);
		break;
	case sector_floorlightsec:
		sector->floorlightsec = (INT32)luaL_checkinteger(L, 3);
		break;
	case sector_ceilinglightlevel:
		sector->ceilinglightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_ceilinglightabsolute:
		sector->ceilinglightabsolute = luaL_checkboolean(L, 3);
		break;
	case sector_ceilinglightsec:
		sector->ceilinglightsec = (INT32)luaL_checkinteger(L, 3);
		break;
	case sector_special:
		sector->special = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_tag:
		Tag_SectorFSet((UINT32)(sector - sectors), (INT16)luaL_checkinteger(L, 3));
		break;
	case sector_taglist:
		return LUA_ErrSetDirectly(L, "sector_t", "taglist");
	case sector_flags:
		sector->flags = luaL_checkinteger(L, 3);
		CheckForReverseGravity |= (sector->flags & MSF_GRAVITYFLIP);
		break;
	case sector_specialflags:
		sector->specialflags = luaL_checkinteger(L, 3);
		break;
	case sector_damagetype:
		sector->damagetype = (UINT8)luaL_checkinteger(L, 3);
		break;
	case sector_triggertag:
		sector->triggertag = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_triggerer:
		sector->triggerer = (UINT8)luaL_checkinteger(L, 3);
		break;
	case sector_gravity:
		sector->gravity = luaL_checkfixed(L, 3);
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

/////////////////
// subsector_t //
/////////////////

static int subsector_get(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	enum subsector_e field = Lua_optoption(L, 2, subsector_valid, subsector_fields_ref);

	if (!subsector)
	{
		if (field == subsector_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed subsector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case subsector_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case subsector_sector:
		LUA_PushUserdata(L, subsector->sector, META_SECTOR);
		return 1;
	case subsector_numlines:
		lua_pushinteger(L, subsector->numlines);
		return 1;
	case subsector_firstline:
		lua_pushinteger(L, subsector->firstline);
		return 1;
	case subsector_polyList: // polyList
		lua_pushcfunction(L, lib_iterateSubSectorPolylist);
		LUA_PushUserdata(L, subsector->polyList, META_POLYOBJ);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateSubSectorPolylist and subsector->polyList as upvalues for the function
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

////////////
// line_t //
////////////

// args, i -> args[i]
static int lineargs_get(lua_State *L)
{
	INT32 *args = *((INT32**)luaL_checkudata(L, 1, META_LINEARGS));
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUMLINEARGS)
		return luaL_error(L, LUA_QL("line_t.args") " index cannot be %d", i);
	lua_pushinteger(L, args[i]);
	return 1;
}

// #args -> NUMLINEARGS
static int lineargs_len(lua_State* L)
{
	lua_pushinteger(L, NUMLINEARGS);
	return 1;
}

// stringargs, i -> stringargs[i]
static int linestringargs_get(lua_State *L)
{
	char **stringargs = *((char***)luaL_checkudata(L, 1, META_LINESTRINGARGS));
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUMLINESTRINGARGS)
		return luaL_error(L, LUA_QL("line_t.stringargs") " index cannot be %d", i);
	lua_pushstring(L, stringargs[i]);
	return 1;
}

// #stringargs -> NUMLINESTRINGARGS
static int linestringargs_len(lua_State *L)
{
	lua_pushinteger(L, NUMLINESTRINGARGS);
	return 1;
}

static int line_get(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	enum line_e field = Lua_optoption(L, 2, line_valid, line_fields_ref);

	if (!line)
	{
		if (field == line_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed line_t doesn't exist anymore.");
	}

	switch(field)
	{
	case line_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case line_v1:
		LUA_PushUserdata(L, line->v1, META_VERTEX);
		return 1;
	case line_v2:
		LUA_PushUserdata(L, line->v2, META_VERTEX);
		return 1;
	case line_dx:
		lua_pushfixed(L, line->dx);
		return 1;
	case line_dy:
		lua_pushfixed(L, line->dy);
		return 1;
	case line_angle:
		lua_pushangle(L, line->angle);
		return 1;
	case line_flags:
		lua_pushinteger(L, line->flags);
		return 1;
	case line_special:
		lua_pushinteger(L, line->special);
		return 1;
	case line_tag:
		// TODO: 2.3: Always return a unsigned value
		lua_pushinteger(L, Tag_FGet(&line->tags));
		return 1;
	case line_taglist:
		LUA_PushUserdata(L, &line->tags, META_TAGLIST);
		return 1;
	case line_args:
		LUA_PushUserdata(L, line->args, META_LINEARGS);
		return 1;
	case line_stringargs:
		LUA_PushUserdata(L, line->stringargs, META_LINESTRINGARGS);
		return 1;
	case line_sidenum:
		LUA_PushUserdata(L, line->sidenum, META_SIDENUM);
		return 1;
	case line_frontside: // frontside
		LUA_PushUserdata(L, &sides[line->sidenum[0]], META_SIDE);
		return 1;
	case line_backside: // backside
		if (line->sidenum[1] == NO_SIDEDEF)
			return 0;
		LUA_PushUserdata(L, &sides[line->sidenum[1]], META_SIDE);
		return 1;
	case line_alpha:
		lua_pushfixed(L, line->alpha);
		return 1;
	case line_executordelay:
		lua_pushinteger(L, line->executordelay);
		return 1;
	case line_slopetype:
		switch(line->slopetype)
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
	case line_frontsector:
		LUA_PushUserdata(L, line->frontsector, META_SECTOR);
		return 1;
	case line_backsector:
		LUA_PushUserdata(L, line->backsector, META_SECTOR);
		return 1;
	case line_polyobj:
		LUA_PushUserdata(L, line->polyobj, META_POLYOBJ);
		return 1;
	// TODO: 2.3: Delete
	case line_text:
		{
			if (udmf)
			{
				LUA_Deprecated(L, "(linedef_t).text", "(linedef_t).stringargs");
				lua_pushnil(L);
				return 1;
			}

			if (line->special == 331 || line->special == 443)
			{
				// See P_ProcessLinedefsAfterSidedefs, P_ConvertBinaryLinedefTypes
				lua_pushstring(L, line->stringargs[0]);
			}
			else
				lua_pushnil(L);

			return 1;
		}
	case line_callcount:
		lua_pushinteger(L, line->callcount);
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

////////////////////
// line.sidenum[] //
////////////////////

static int sidenum_get(lua_State *L)
{
	UINT32 *sidenum = *((UINT32 **)luaL_checkudata(L, 1, META_SIDENUM));
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

////////////
// side_t //
////////////

static int side_get(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	enum side_e field = Lua_optoption(L, 2, side_valid, side_fields_ref);

	if (!side)
	{
		if (field == side_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case side_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case side_textureoffset:
		lua_pushfixed(L, side->textureoffset);
		return 1;
	case side_rowoffset:
		lua_pushfixed(L, side->rowoffset);
		return 1;
	case side_offsetx_top:
		lua_pushfixed(L, side->offsetx_top);
		return 1;
	case side_offsety_top:
		lua_pushfixed(L, side->offsety_top);
		return 1;
	case side_offsetx_mid:
		lua_pushfixed(L, side->offsetx_mid);
		return 1;
	case side_offsety_mid:
		lua_pushfixed(L, side->offsety_mid);
		return 1;
	case side_offsetx_bottom:
	case side_offsetx_bot:
		lua_pushfixed(L, side->offsetx_bottom);
		return 1;
	case side_offsety_bottom:
	case side_offsety_bot:
		lua_pushfixed(L, side->offsety_bottom);
		return 1;
	case side_scalex_top:
		lua_pushfixed(L, side->scalex_top);
		return 1;
	case side_scaley_top:
		lua_pushfixed(L, side->scaley_top);
		return 1;
	case side_scalex_mid:
		lua_pushfixed(L, side->scalex_mid);
		return 1;
	case side_scaley_mid:
		lua_pushfixed(L, side->scaley_mid);
		return 1;
	case side_scalex_bottom:
		lua_pushfixed(L, side->scalex_bottom);
		return 1;
	case side_scaley_bottom:
		lua_pushfixed(L, side->scaley_bottom);
		return 1;
	case side_toptexture:
		lua_pushinteger(L, side->toptexture);
		return 1;
	case side_bottomtexture:
		lua_pushinteger(L, side->bottomtexture);
		return 1;
	case side_midtexture:
		lua_pushinteger(L, side->midtexture);
		return 1;
	case side_line:
		LUA_PushUserdata(L, side->line, META_LINE);
		return 1;
	case side_sector:
		LUA_PushUserdata(L, side->sector, META_SECTOR);
		return 1;
	case side_special:
		lua_pushinteger(L, side->special);
		return 1;
	case side_repeatcnt:
		lua_pushinteger(L, side->repeatcnt);
		return 1;
	case side_light:
		lua_pushinteger(L, side->light);
		return 1;
	case side_light_top:
		lua_pushinteger(L, side->light_top);
		return 1;
	case side_light_mid:
		lua_pushinteger(L, side->light_mid);
		return 1;
	case side_light_bottom:
		lua_pushinteger(L, side->light_bottom);
		return 1;
	case side_lightabsolute:
		lua_pushboolean(L, side->lightabsolute);
		return 1;
	case side_lightabsolute_top:
		lua_pushboolean(L, side->lightabsolute_top);
		return 1;
	case side_lightabsolute_mid:
		lua_pushboolean(L, side->lightabsolute_mid);
		return 1;
	case side_lightabsolute_bottom:
		lua_pushboolean(L, side->lightabsolute_bottom);
		return 1;
	// TODO: 2.3: Delete
	case side_text:
		{
			boolean isfrontside;
			size_t sidei = side-sides;

			if (udmf)
			{
				LUA_Deprecated(L, "(sidedef_t).text", "(sidedef_t).line.stringargs");
				lua_pushnil(L);
				return 1;
			}

			isfrontside = side->line->sidenum[0] == sidei;

			lua_pushstring(L, side->line->stringargs[isfrontside ? 0 : 1]);
			return 1;
		}
	}
	return 0;
}

static int side_set(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	enum side_e field = Lua_optoption(L, 2, side_valid, side_fields_ref);

	if (!side)
	{
		if (field == side_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case side_valid: // valid
	case side_line:
	case side_sector:
	case side_special:
	case side_text:
		return luaL_error(L, "side_t field " LUA_QS " cannot be set.", side_opt[field]);
	default:
		return luaL_error(L, "side_t has no field named " LUA_QS ".", lua_tostring(L, 2));
	case side_textureoffset:
		side->textureoffset = luaL_checkfixed(L, 3);
		break;
	case side_rowoffset:
		side->rowoffset = luaL_checkfixed(L, 3);
		break;
	case side_offsetx_top:
		side->offsetx_top = luaL_checkfixed(L, 3);
		break;
	case side_offsety_top:
		side->offsety_top = luaL_checkfixed(L, 3);
		break;
	case side_offsetx_mid:
		side->offsetx_mid = luaL_checkfixed(L, 3);
		break;
	case side_offsety_mid:
		side->offsety_mid = luaL_checkfixed(L, 3);
		break;
	case side_offsetx_bot:
	case side_offsetx_bottom:
		side->offsetx_bottom = luaL_checkfixed(L, 3);
		break;
	case side_offsety_bot:
	case side_offsety_bottom:
		side->offsety_bottom = luaL_checkfixed(L, 3);
		break;
	case side_scalex_top:
		side->scalex_top = luaL_checkfixed(L, 3);
		break;
	case side_scaley_top:
		side->scaley_top = luaL_checkfixed(L, 3);
		break;
	case side_scalex_mid:
		side->scalex_mid = luaL_checkfixed(L, 3);
		break;
	case side_scaley_mid:
		side->scaley_mid = luaL_checkfixed(L, 3);
		break;
	case side_scalex_bottom:
		side->scalex_bottom = luaL_checkfixed(L, 3);
		break;
	case side_scaley_bottom:
		side->scaley_bottom = luaL_checkfixed(L, 3);
		break;
	case side_toptexture:
		side->toptexture = luaL_checkinteger(L, 3);
		break;
	case side_bottomtexture:
		side->bottomtexture = luaL_checkinteger(L, 3);
		break;
	case side_midtexture:
		side->midtexture = luaL_checkinteger(L, 3);
		break;
	case side_repeatcnt:
		side->repeatcnt = luaL_checkinteger(L, 3);
		break;
	case side_light:
		side->light = luaL_checkinteger(L, 3);
		break;
	case side_light_top:
		side->light_top = luaL_checkinteger(L, 3);
		break;
	case side_light_mid:
		side->light_mid = luaL_checkinteger(L, 3);
		break;
	case side_light_bottom:
		side->light_bottom = luaL_checkinteger(L, 3);
		break;
	case side_lightabsolute:
		side->lightabsolute = luaL_checkboolean(L, 3);
		break;
	case side_lightabsolute_top:
		side->lightabsolute_top = luaL_checkboolean(L, 3);
		break;
	case side_lightabsolute_mid:
		side->lightabsolute_mid = luaL_checkboolean(L, 3);
		break;
	case side_lightabsolute_bottom:
		side->lightabsolute_bottom = luaL_checkboolean(L, 3);
		break;
	}
	return 0;
}

static int side_num(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	lua_pushinteger(L, side-sides);
	return 1;
}

//////////////
// vertex_t //
//////////////

static int vertex_get(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	enum vertex_e field = Lua_optoption(L, 2, vertex_valid, vertex_fields_ref);

	if (!vertex)
	{
		if (field == vertex_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed vertex_t doesn't exist anymore.");
	}

	switch(field)
	{
	case vertex_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case vertex_x:
		lua_pushfixed(L, vertex->x);
		return 1;
	case vertex_y:
		lua_pushfixed(L, vertex->y);
		return 1;
	case vertex_floorzset:
		lua_pushboolean(L, vertex->floorzset);
		return 1;
	case vertex_ceilingzset:
		lua_pushboolean(L, vertex->ceilingzset);
		return 1;
	case vertex_floorz:
		lua_pushfixed(L, vertex->floorz);
		return 1;
	case vertex_ceilingz:
		lua_pushfixed(L, vertex->ceilingz);
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

#ifdef HAVE_LUA_SEGS

///////////
// seg_t //
///////////

static int seg_get(lua_State *L)
{
	seg_t *seg = *((seg_t **)luaL_checkudata(L, 1, META_SEG));
	enum seg_e field = Lua_optoption(L, 2, seg_valid, seg_fields_ref);

	if (!seg)
	{
		if (field == seg_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed seg_t doesn't exist anymore.");
	}

	switch(field)
	{
	case seg_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case seg_v1:
		LUA_PushUserdata(L, seg->v1, META_VERTEX);
		return 1;
	case seg_v2:
		LUA_PushUserdata(L, seg->v2, META_VERTEX);
		return 1;
	case seg_side:
		lua_pushinteger(L, seg->side);
		return 1;
	case seg_offset:
		lua_pushfixed(L, seg->offset);
		return 1;
	case seg_angle:
		lua_pushangle(L, seg->angle);
		return 1;
	case seg_sidedef:
		LUA_PushUserdata(L, seg->sidedef, META_SIDE);
		return 1;
	case seg_linedef:
		LUA_PushUserdata(L, seg->linedef, META_LINE);
		return 1;
	case seg_frontsector:
		LUA_PushUserdata(L, seg->frontsector, META_SECTOR);
		return 1;
	case seg_backsector:
		LUA_PushUserdata(L, seg->backsector, META_SECTOR);
		return 1;
	case seg_polyseg:
		LUA_PushUserdata(L, seg->polyseg, META_POLYOBJ);
		return 1;
	}
	return 0;
}

static int seg_num(lua_State *L)
{
	seg_t *seg = *((seg_t **)luaL_checkudata(L, 1, META_SEG));
	lua_pushinteger(L, seg-segs);
	return 1;
}

////////////
// node_t //
////////////

static int node_get(lua_State *L)
{
	node_t *node = *((node_t **)luaL_checkudata(L, 1, META_NODE));
	enum node_e field = Lua_optoption(L, 2, node_valid, node_fields_ref);

	if (!node)
	{
		if (field == node_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed node_t doesn't exist anymore.");
	}

	switch(field)
	{
	case node_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case node_x:
		lua_pushfixed(L, node->x);
		return 1;
	case node_y:
		lua_pushfixed(L, node->y);
		return 1;
	case node_dx:
		lua_pushfixed(L, node->x);
		return 1;
	case node_dy:
		lua_pushfixed(L, node->x);
		return 1;
	case node_bbox:
		LUA_PushUserdata(L, node->bbox, META_NODEBBOX);
		return 1;
	case node_children:
		LUA_PushUserdata(L, node->children, META_NODECHILDREN);
		return 1;
	}
	return 0;
}

static int node_num(lua_State *L)
{
	node_t *node = *((node_t **)luaL_checkudata(L, 1, META_NODE));
	lua_pushinteger(L, node-nodes);
	return 1;
}

///////////////
// node.bbox //
///////////////

/*
// node.bbox[i][j]: i = 0 or 1, j = 0 1 2 or 3
// NOTE: 2D arrays are NOT double pointers,
//       the second bbox will be directly after the first in memory (hence the way the bbox is pushed here)
// this function handles the [i] part, bbox_get handles the [j] part
static int nodebbox_get(lua_State *L)
{
	fixed_t *bbox = *((fixed_t **)luaL_checkudata(L, 1, META_NODEBBOX));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		int field = luaL_checkoption(L, 2, NULL, valid_opt);
		if (!bbox)
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed node_t doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	i = lua_tointeger(L, 2);
	if (i < 0 || i > 1)
		return 0;
	LUA_PushUserdata(L, bbox + i*4*sizeof(fixed_t), META_BBOX);
	return 1;
}
*/
static int nodebbox_call(lua_State *L)
{
	fixed_t *bbox = *((fixed_t **)luaL_checkudata(L, 1, META_NODEBBOX));
	int i, j;
	int n = lua_gettop(L);

	if (!bbox)
		return luaL_error(L, "accessed node bbox doesn't exist anymore.");
	if (n < 3)
		return luaL_error(L, "arguments 2 and/or 3 not given (expected node.bbox(child, coord))");
	// get child
	if (!lua_isnumber(L, 2)) {
		enum nodechild_e field = luaL_checkoption(L, 2, nodechild_opt[0], nodechild_opt);
		switch (field) {
			case nodechild_right: i = 0; break;
			case nodechild_left:  i = 1; break;
			default:
				return luaL_error(L, "invalid node child \"%s\".", lua_tostring(L, 2));
		}
	}
	else {
		i = lua_tointeger(L, 2);
		if (i < 0 || i > 1)
			return 0;
	}
	// get bbox coord
	if (!lua_isnumber(L, 3)) {
		enum bbox_e field = luaL_checkoption(L, 3, bbox_opt[0], bbox_opt);
		switch (field) {
			case bbox_top:    j = BOXTOP;    break;
			case bbox_bottom: j = BOXBOTTOM; break;
			case bbox_left:   j = BOXLEFT;   break;
			case bbox_right:  j = BOXRIGHT;  break;
			default:
				return luaL_error(L, "invalid bbox coordinate \"%s\".", lua_tostring(L, 3));
		}
	}
	else {
		j = lua_tointeger(L, 3);
		if (j < 0 || j > 3)
			return 0;
	}
	lua_pushinteger(L, bbox[i*4 + j]);
	return 1;
}

/////////////////////
// node.children[] //
/////////////////////

// node.children[i]: i = 0 or 1
static int nodechildren_get(lua_State *L)
{
	UINT16 *children = *((UINT16 **)luaL_checkudata(L, 1, META_NODECHILDREN));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		enum nodechild_e field = luaL_checkoption(L, 2, nodechild_opt[0], nodechild_opt);
		if (!children)
		{
			if (field == nodechild_valid) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed node_t doesn't exist anymore.");
		} else if (field == nodechild_valid) {
			lua_pushboolean(L, 1);
			return 1;
		} else switch (field) {
			case nodechild_right: i = 0; break;
			case nodechild_left:  i = 1; break;
			default:              return 0;
		}
	}
	else {
		i = lua_tointeger(L, 2);
		if (i < 0 || i > 1)
			return 0;
	}
	lua_pushinteger(L, children[i]);
	return 1;
}
#endif

//////////
// bbox //
//////////

// bounding box (aka fixed_t array with four elements)
// NOTE: may be useful for polyobjects or other things later
static int bbox_get(lua_State *L)
{
	fixed_t *bbox = *((fixed_t **)luaL_checkudata(L, 1, META_BBOX));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		enum bbox_e field = luaL_checkoption(L, 2, bbox_opt[0], bbox_opt);
		if (!bbox)
		{
			if (field == bbox_valid) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed bbox doesn't exist anymore.");
		} else if (field == bbox_valid) {
			lua_pushboolean(L, 1);
			return 1;
		} else switch (field) {
			case bbox_top:    i = BOXTOP;    break;
			case bbox_bottom: i = BOXBOTTOM; break;
			case bbox_left:   i = BOXLEFT;   break;
			case bbox_right:  i = BOXRIGHT;  break;
			default:          return 0;
		}
	}
	else {
		i = lua_tointeger(L, 2);
		if (i < 0 || i > 3)
			return 0;
	}
	lua_pushinteger(L, bbox[i]);
	return 1;
}

///////////////
// sectors[] //
///////////////

static int lib_iterateSectors(lua_State *L)
{
	size_t i = 0;
	INLEVEL
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
	INLEVEL
	if (lua_isnumber(L, 2))
	{
		size_t i = lua_tointeger(L, 2);
		if (i >= numsectors)
			return 0;
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	return 0;
}

static int lib_numsectors(lua_State *L)
{
	lua_pushinteger(L, numsectors);
	return 1;
}

//////////////////
// subsectors[] //
//////////////////

static int lib_iterateSubsectors(lua_State *L)
{
	size_t i = 0;
	INLEVEL
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
	INLEVEL
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

/////////////
// lines[] //
/////////////

static int lib_iterateLines(lua_State *L)
{
	size_t i = 0;
	INLEVEL
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
	INLEVEL
	if (lua_isnumber(L, 2))
	{
		size_t i = lua_tointeger(L, 2);
		if (i >= numlines)
			return 0;
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	return 0;
}

static int lib_numlines(lua_State *L)
{
	lua_pushinteger(L, numlines);
	return 1;
}

/////////////
// sides[] //
/////////////

static int lib_iterateSides(lua_State *L)
{
	size_t i = 0;
	INLEVEL
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
	INLEVEL
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

////////////////
// vertexes[] //
////////////////

static int lib_iterateVertexes(lua_State *L)
{
	size_t i = 0;
	INLEVEL
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
	INLEVEL
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

#ifdef HAVE_LUA_SEGS

////////////
// segs[] //
////////////

static int lib_iterateSegs(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call segs.iterate() directly, use it as 'for seg in segs.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((seg_t **)luaL_checkudata(L, 1, META_SEG)) - segs)+1;
	if (i < numsegs)
	{
		LUA_PushUserdata(L, &segs[i], META_SEG);
		return 1;
	}
	return 0;
}

static int lib_getSeg(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsegs)
			return 0;
		LUA_PushUserdata(L, &segs[i], META_SEG);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSegs);
		return 1;
	}
	return 0;
}

static int lib_numsegs(lua_State *L)
{
	lua_pushinteger(L, numsegs);
	return 1;
}

/////////////
// nodes[] //
/////////////

static int lib_iterateNodes(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call nodes.iterate() directly, use it as 'for node in nodes.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((node_t **)luaL_checkudata(L, 1, META_NODE)) - nodes)+1;
	if (i < numsegs)
	{
		LUA_PushUserdata(L, &nodes[i], META_NODE);
		return 1;
	}
	return 0;
}

static int lib_getNode(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numnodes)
			return 0;
		LUA_PushUserdata(L, &nodes[i], META_NODE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateNodes);
		return 1;
	}
	return 0;
}

static int lib_numnodes(lua_State *L)
{
	lua_pushinteger(L, numnodes);
	return 1;
}
#endif

//////////////
// ffloor_t //
//////////////

static INT32 P_GetOldFOFFlags(ffloor_t *fflr)
{
	INT32 result = 0;
	if (fflr->fofflags & FOF_EXISTS)
		result |= FF_OLD_EXISTS;
	if (fflr->fofflags & FOF_BLOCKPLAYER)
		result |= FF_OLD_BLOCKPLAYER;
	if (fflr->fofflags & FOF_BLOCKOTHERS)
		result |= FF_OLD_BLOCKOTHERS;
	if (fflr->fofflags & FOF_RENDERSIDES)
		result |= FF_OLD_RENDERSIDES;
	if (fflr->fofflags & FOF_RENDERPLANES)
		result |= FF_OLD_RENDERPLANES;
	if (fflr->fofflags & FOF_SWIMMABLE)
		result |= FF_OLD_SWIMMABLE;
	if (fflr->fofflags & FOF_NOSHADE)
		result |= FF_OLD_NOSHADE;
	if (fflr->fofflags & FOF_CUTSOLIDS)
		result |= FF_OLD_CUTSOLIDS;
	if (fflr->fofflags & FOF_CUTEXTRA)
		result |= FF_OLD_CUTEXTRA;
	if (fflr->fofflags & FOF_CUTSPRITES)
		result |= FF_OLD_CUTSPRITES;
	if (fflr->fofflags & FOF_BOTHPLANES)
		result |= FF_OLD_BOTHPLANES;
	if (fflr->fofflags & FOF_EXTRA)
		result |= FF_OLD_EXTRA;
	if (fflr->fofflags & FOF_TRANSLUCENT)
		result |= FF_OLD_TRANSLUCENT;
	if (fflr->fofflags & FOF_FOG)
		result |= FF_OLD_FOG;
	if (fflr->fofflags & FOF_INVERTPLANES)
		result |= FF_OLD_INVERTPLANES;
	if (fflr->fofflags & FOF_ALLSIDES)
		result |= FF_OLD_ALLSIDES;
	if (fflr->fofflags & FOF_INVERTSIDES)
		result |= FF_OLD_INVERTSIDES;
	if (fflr->fofflags & FOF_DOUBLESHADOW)
		result |= FF_OLD_DOUBLESHADOW;
	if (fflr->fofflags & FOF_FLOATBOB)
		result |= FF_OLD_FLOATBOB;
	if (fflr->fofflags & FOF_NORETURN)
		result |= FF_OLD_NORETURN;
	if (fflr->fofflags & FOF_CRUMBLE)
		result |= FF_OLD_CRUMBLE;
	if (fflr->bustflags & FB_ONLYBOTTOM)
		result |= FF_OLD_SHATTERBOTTOM;
	if (fflr->fofflags & FOF_GOOWATER)
		result |= FF_OLD_GOOWATER;
	if (fflr->fofflags & FOF_MARIO)
		result |= FF_OLD_MARIO;
	if (fflr->fofflags & FOF_BUSTUP)
		result |= FF_OLD_BUSTUP;
	if (fflr->fofflags & FOF_QUICKSAND)
		result |= FF_OLD_QUICKSAND;
	if (fflr->fofflags & FOF_PLATFORM)
		result |= FF_OLD_PLATFORM;
	if (fflr->fofflags & FOF_REVERSEPLATFORM)
		result |= FF_OLD_REVERSEPLATFORM;
	if (fflr->fofflags & FOF_INTANGIBLEFLATS)
		result |= FF_OLD_INTANGIBLEFLATS;
	if (fflr->busttype == BT_TOUCH)
		result |= FF_OLD_SHATTER;
	if (fflr->busttype == BT_SPINBUST)
		result |= FF_OLD_SPINBUST;
	if (fflr->busttype == BT_STRONG)
		result |= FF_OLD_STRONGBUST;
	if (fflr->fofflags & FOF_RIPPLE)
		result |= FF_OLD_RIPPLE;
	if (fflr->fofflags & FOF_COLORMAPONLY)
		result |= FF_OLD_COLORMAPONLY;
	return result;
}

static int ffloor_get(lua_State *L)
{
	ffloor_t *ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	enum ffloor_e field = Lua_optoption(L, 2, ffloor_valid, ffloor_fields_ref);
	INT16 i;

	if (!ffloor)
	{
		if (field == ffloor_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed ffloor_t doesn't exist anymore.");
	}

	switch(field)
	{
	case ffloor_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case ffloor_topheight:
		lua_pushfixed(L, *ffloor->topheight);
		return 1;
	case ffloor_toppic: { // toppic
		levelflat_t *levelflat = &levelflats[*ffloor->toppic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case ffloor_toplightlevel:
		lua_pushinteger(L, *ffloor->toplightlevel);
		return 1;
	case ffloor_topxoffs:
		lua_pushfixed(L, *ffloor->topxoffs);
		return 1;
	case ffloor_topyoffs:
		lua_pushfixed(L, *ffloor->topyoffs);
		return 1;
	case ffloor_topxscale:
		lua_pushfixed(L, *ffloor->topxscale);
		return 1;
	case ffloor_topyscale:
		lua_pushfixed(L, *ffloor->topyscale);
		return 1;
	case ffloor_bottomheight:
		lua_pushfixed(L, *ffloor->bottomheight);
		return 1;
	case ffloor_bottompic: { // bottompic
		levelflat_t *levelflat = &levelflats[*ffloor->bottompic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case ffloor_bottomxoffs:
		lua_pushfixed(L, *ffloor->bottomxoffs);
		return 1;
	case ffloor_bottomyoffs:
		lua_pushfixed(L, *ffloor->bottomyoffs);
		return 1;
	case ffloor_bottomxscale:
		lua_pushfixed(L, *ffloor->bottomxscale);
		return 1;
	case ffloor_bottomyscale:
		lua_pushfixed(L, *ffloor->bottomyscale);
		return 1;
	case ffloor_tslope:
		LUA_PushUserdata(L, *ffloor->t_slope, META_SLOPE);
		return 1;
	case ffloor_bslope:
		LUA_PushUserdata(L, *ffloor->b_slope, META_SLOPE);
		return 1;
	case ffloor_sector:
		LUA_PushUserdata(L, &sectors[ffloor->secnum], META_SECTOR);
		return 1;
	case ffloor_fofflags:
		lua_pushinteger(L, ffloor->fofflags);
		return 1;
	case ffloor_flags:
		lua_pushinteger(L, P_GetOldFOFFlags(ffloor));
		return 1;
	case ffloor_master:
		LUA_PushUserdata(L, ffloor->master, META_LINE);
		return 1;
	case ffloor_target:
		LUA_PushUserdata(L, ffloor->target, META_SECTOR);
		return 1;
	case ffloor_next:
		LUA_PushUserdata(L, ffloor->next, META_FFLOOR);
		return 1;
	case ffloor_prev:
		LUA_PushUserdata(L, ffloor->prev, META_FFLOOR);
		return 1;
	case ffloor_alpha:
		lua_pushinteger(L, ffloor->alpha);
		return 1;
	case ffloor_blend:
		lua_pushinteger(L, ffloor->blend);
		return 1;
	case ffloor_bustflags:
		lua_pushinteger(L, ffloor->bustflags);
		return 1;
	case ffloor_busttype:
		lua_pushinteger(L, ffloor->busttype);
		return 1;
	case ffloor_busttag:
		lua_pushinteger(L, ffloor->busttag);
		return 1;
	case ffloor_sinkspeed:
		lua_pushfixed(L, ffloor->sinkspeed);
		return 1;
	case ffloor_friction:
		lua_pushfixed(L, ffloor->friction);
		return 1;
	case ffloor_bouncestrength:
		lua_pushfixed(L, ffloor->bouncestrength);
		return 1;
	}
	return 0;
}

static void P_SetOldFOFFlags(ffloor_t *fflr, oldffloortype_e oldflags)
{
	ffloortype_e originalflags = fflr->fofflags;
	fflr->fofflags = 0;
	if (oldflags & FF_OLD_EXISTS)
		fflr->fofflags |= FOF_EXISTS;
	if (oldflags & FF_OLD_BLOCKPLAYER)
		fflr->fofflags |= FOF_BLOCKPLAYER;
	if (oldflags & FF_OLD_BLOCKOTHERS)
		fflr->fofflags |= FOF_BLOCKOTHERS;
	if (oldflags & FF_OLD_RENDERSIDES)
		fflr->fofflags |= FOF_RENDERSIDES;
	if (oldflags & FF_OLD_RENDERPLANES)
		fflr->fofflags |= FOF_RENDERPLANES;
	if (oldflags & FF_OLD_SWIMMABLE)
		fflr->fofflags |= FOF_SWIMMABLE;
	if (oldflags & FF_OLD_NOSHADE)
		fflr->fofflags |= FOF_NOSHADE;
	if (oldflags & FF_OLD_CUTSOLIDS)
		fflr->fofflags |= FOF_CUTSOLIDS;
	if (oldflags & FF_OLD_CUTEXTRA)
		fflr->fofflags |= FOF_CUTEXTRA;
	if (oldflags & FF_OLD_CUTSPRITES)
		fflr->fofflags |= FOF_CUTSPRITES;
	if (oldflags & FF_OLD_BOTHPLANES)
		fflr->fofflags |= FOF_BOTHPLANES;
	if (oldflags & FF_OLD_EXTRA)
		fflr->fofflags |= FOF_EXTRA;
	if (oldflags & FF_OLD_TRANSLUCENT)
		fflr->fofflags |= FOF_TRANSLUCENT;
	if (oldflags & FF_OLD_FOG)
		fflr->fofflags |= FOF_FOG;
	if (oldflags & FF_OLD_INVERTPLANES)
		fflr->fofflags |= FOF_INVERTPLANES;
	if (oldflags & FF_OLD_ALLSIDES)
		fflr->fofflags |= FOF_ALLSIDES;
	if (oldflags & FF_OLD_INVERTSIDES)
		fflr->fofflags |= FOF_INVERTSIDES;
	if (oldflags & FF_OLD_DOUBLESHADOW)
		fflr->fofflags |= FOF_DOUBLESHADOW;
	if (oldflags & FF_OLD_FLOATBOB)
		fflr->fofflags |= FOF_FLOATBOB;
	if (oldflags & FF_OLD_NORETURN)
		fflr->fofflags |= FOF_NORETURN;
	if (oldflags & FF_OLD_CRUMBLE)
		fflr->fofflags |= FOF_CRUMBLE;
	if (oldflags & FF_OLD_GOOWATER)
		fflr->fofflags |= FOF_GOOWATER;
	if (oldflags & FF_OLD_MARIO)
		fflr->fofflags |= FOF_MARIO;
	if (oldflags & FF_OLD_BUSTUP)
		fflr->fofflags |= FOF_BUSTUP;
	if (oldflags & FF_OLD_QUICKSAND)
		fflr->fofflags |= FOF_QUICKSAND;
	if (oldflags & FF_OLD_PLATFORM)
		fflr->fofflags |= FOF_PLATFORM;
	if (oldflags & FF_OLD_REVERSEPLATFORM)
		fflr->fofflags |= FOF_REVERSEPLATFORM;
	if (oldflags & FF_OLD_RIPPLE)
		fflr->fofflags |= FOF_RIPPLE;
	if (oldflags & FF_OLD_COLORMAPONLY)
		fflr->fofflags |= FOF_COLORMAPONLY;
	if (originalflags & FOF_BOUNCY)
		fflr->fofflags |= FOF_BOUNCY;
	if (originalflags & FOF_SPLAT)
		fflr->fofflags |= FOF_SPLAT;

	if (oldflags & FF_OLD_SHATTER)
		fflr->busttype = BT_TOUCH;
	else if (oldflags & FF_OLD_SPINBUST)
		fflr->busttype = BT_SPINBUST;
	else if (oldflags & FF_OLD_STRONGBUST)
		fflr->busttype = BT_STRONG;
	else
		fflr->busttype = BT_REGULAR;

	if (oldflags & FF_OLD_SHATTERBOTTOM)
		fflr->bustflags |= FB_ONLYBOTTOM;
	else
		fflr->bustflags &= ~FB_ONLYBOTTOM;
}

static int ffloor_set(lua_State *L)
{
	ffloor_t *ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	enum ffloor_e field = Lua_optoption(L, 2, ffloor_valid, ffloor_fields_ref);

	if (!ffloor)
		return luaL_error(L, "accessed ffloor_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter ffloor_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter ffloor_t in CMD building code!");

	switch(field)
	{
	case ffloor_valid: // valid
	case ffloor_tslope: // t_slope
	case ffloor_bslope: // b_slope
	case ffloor_sector: // sector
	case ffloor_master: // master
	case ffloor_target: // target
	case ffloor_next: // next
	case ffloor_prev: // prev
		return luaL_error(L, "ffloor_t field " LUA_QS " cannot be set.", ffloor_opt[field]);
	default:
		return luaL_error(L, "ffloor_t has no field named " LUA_QS ".", lua_tostring(L, 2));
	case ffloor_topheight: { // topheight
		boolean flag;
		fixed_t lastpos = *ffloor->topheight;
		mobj_t *ptmthing = tmthing;
		sector_t *sector = &sectors[ffloor->secnum];
		sector->ceilingheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			*ffloor->topheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case ffloor_toppic:
		*ffloor->toppic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case ffloor_toplightlevel:
		*ffloor->toplightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case ffloor_topxoffs:
		*ffloor->topxoffs = luaL_checkfixed(L, 3);
		break;
	case ffloor_topyoffs:
		*ffloor->topyoffs = luaL_checkfixed(L, 3);
		break;
	case ffloor_topxscale:
		*ffloor->topxscale = luaL_checkfixed(L, 3);
		break;
	case ffloor_topyscale:
		*ffloor->topyscale = luaL_checkfixed(L, 3);
		break;
	case ffloor_bottomheight: { // bottomheight
		boolean flag;
		fixed_t lastpos = *ffloor->bottomheight;
		mobj_t *ptmthing = tmthing;
		sector_t *sector = &sectors[ffloor->secnum];
		sector->floorheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			*ffloor->bottomheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case ffloor_bottompic:
		*ffloor->bottompic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case ffloor_bottomxoffs:
		*ffloor->bottomxoffs = luaL_checkfixed(L, 3);
		break;
	case ffloor_bottomyoffs:
		*ffloor->bottomyoffs = luaL_checkfixed(L, 3);
		break;
	case ffloor_bottomxscale:
		*ffloor->bottomxscale = luaL_checkfixed(L, 3);
		break;
	case ffloor_bottomyscale:
		*ffloor->bottomyscale = luaL_checkfixed(L, 3);
		break;
	case ffloor_fofflags: {
		ffloortype_e oldflags = ffloor->fofflags; // store FOF's old flags
		ffloor->fofflags = luaL_checkinteger(L, 3);
		if (ffloor->fofflags != oldflags)
			ffloor->target->moved = true; // reset target sector's lightlist
		break;
	}
	case ffloor_flags: {
		ffloortype_e oldflags = ffloor->fofflags; // store FOF's old flags
		busttype_e oldbusttype = ffloor->busttype;
		ffloorbustflags_e oldbustflags = ffloor->bustflags;
		oldffloortype_e newflags = luaL_checkinteger(L, 3);
		P_SetOldFOFFlags(ffloor, newflags);
		if (ffloor->fofflags != oldflags || ffloor->busttype != oldbusttype || ffloor->bustflags != oldbustflags)
			ffloor->target->moved = true; // reset target sector's lightlist
		break;
	}
	case ffloor_alpha:
		ffloor->alpha = (INT32)luaL_checkinteger(L, 3);
		break;
	case ffloor_blend:
		ffloor->blend = (INT32)luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

//////////////
// pslope_t //
//////////////

static int slope_get(lua_State *L)
{
	pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
	enum slope_e field = Lua_optoption(L, 2, slope_valid, slope_fields_ref);

	if (!slope)
	{
		if (field == slope_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed pslope_t doesn't exist anymore.");
	}

	switch(field)
	{
	case slope_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case slope_o: // o
		LUA_PushUserdata(L, &slope->o, META_VECTOR3);
		return 1;
	case slope_d: // d
		LUA_PushUserdata(L, &slope->d, META_VECTOR2);
		return 1;
	case slope_zdelta: // zdelta
		lua_pushfixed(L, slope->zdelta);
		return 1;
	case slope_normal: // normal
		LUA_PushUserdata(L, &slope->normal, META_VECTOR3);
		return 1;
	case slope_zangle: // zangle
		lua_pushangle(L, slope->zangle);
		return 1;
	case slope_xydirection: // xydirection
		lua_pushangle(L, slope->xydirection);
		return 1;
	case slope_flags: // flags
		lua_pushinteger(L, slope->flags);
		return 1;
	}
	return 0;
}

static int slope_set(lua_State *L)
{
	pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
	enum slope_e field = Lua_optoption(L, 2, slope_valid, slope_fields_ref);

	if (!slope)
		return luaL_error(L, "accessed pslope_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter pslope_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter pslope_t in CMD building code!");

	switch(field) // todo: reorganize this shit
	{
	case slope_valid: // valid
	case slope_d: // d
	case slope_flags: // flags
	case slope_normal: // normal
		return luaL_error(L, "pslope_t field " LUA_QS " cannot be set.", slope_opt[field]);
	default:
		return luaL_error(L, "pslope_t has no field named " LUA_QS ".", lua_tostring(L, 2));
	case slope_o: { // o
		luaL_checktype(L, 3, LUA_TTABLE);

		lua_getfield(L, 3, "x");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 1);
		}
		if (!lua_isnil(L, -1))
			slope->o.x = luaL_checkfixed(L, -1);
		else
			slope->o.x = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "y");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 2);
		}
		if (!lua_isnil(L, -1))
			slope->o.y = luaL_checkfixed(L, -1);
		else
			slope->o.y = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "z");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 3);
		}
		if (!lua_isnil(L, -1))
			slope->o.z = luaL_checkfixed(L, -1);
		else
			slope->o.z = 0;
		DVector3_Load(&slope->dorigin,
			FixedToDouble(slope->o.x),
			FixedToDouble(slope->o.y),
			FixedToDouble(slope->o.z)
		);
		lua_pop(L, 1);
		break;
	}
	case slope_zdelta: { // zdelta
		slope->zdelta = luaL_checkfixed(L, 3);
		slope->zangle = R_PointToAngle2(0, 0, FRACUNIT, -slope->zdelta);
		slope->dzdelta = FixedToDouble(slope->zdelta);
		P_CalculateSlopeNormal(slope);
		break;
	}
	case slope_zangle: { // zangle
		angle_t zangle = luaL_checkangle(L, 3);
		if (zangle == ANGLE_90 || zangle == ANGLE_270)
			return luaL_error(L, "invalid zangle for slope!");
		slope->zangle = zangle;
		slope->zdelta = -FINETANGENT(((slope->zangle+ANGLE_90)>>ANGLETOFINESHIFT) & 4095);
		slope->dzdelta = FixedToDouble(slope->zdelta);
		P_CalculateSlopeNormal(slope);
		break;
	}
	case slope_xydirection: // xydirection
		slope->xydirection = luaL_checkangle(L, 3);
		slope->d.x = -FINECOSINE((slope->xydirection>>ANGLETOFINESHIFT) & FINEMASK);
		slope->d.y = -FINESINE((slope->xydirection>>ANGLETOFINESHIFT) & FINEMASK);
		slope->dnormdir.x = FixedToDouble(slope->d.x);
		slope->dnormdir.y = FixedToDouble(slope->d.y);
		P_CalculateSlopeNormal(slope);
		break;
	}
	return 0;
}

///////////////
// vector*_t //
///////////////

static int vector2_get(lua_State *L)
{
	vector2_t *vec = *((vector2_t **)luaL_checkudata(L, 1, META_VECTOR2));
	enum vector_e field = luaL_checkoption(L, 2, vector_opt[0], vector_opt);

	if (!vec)
		return luaL_error(L, "accessed vector2_t doesn't exist anymore.");

	switch(field)
	{
		case vector_x: lua_pushfixed(L, vec->x); return 1;
		case vector_y: lua_pushfixed(L, vec->y); return 1;
		default: break;
	}

	return 0;
}

static int vector3_get(lua_State *L)
{
	vector3_t *vec = *((vector3_t **)luaL_checkudata(L, 1, META_VECTOR3));
	enum vector_e field = luaL_checkoption(L, 2, vector_opt[0], vector_opt);

	if (!vec)
		return luaL_error(L, "accessed vector3_t doesn't exist anymore.");

	switch(field)
	{
		case vector_x: lua_pushfixed(L, vec->x); return 1;
		case vector_y: lua_pushfixed(L, vec->y); return 1;
		case vector_z: lua_pushfixed(L, vec->z); return 1;
		default: break;
	}

	return 0;
}

/////////////////////
// mapheaderinfo[] //
/////////////////////

static int lib_getMapheaderinfo(lua_State *L)
{
	// i -> mapheaderinfo[i-1]

	//int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1)-1;
		if (i >= NUMMAPS)
			return 0;
		LUA_PushUserdata(L, mapheaderinfo[i], META_MAPHEADER);
		//CONS_Printf(mapheaderinfo[i]->lvlttl);
		return 1;
	}/*
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSubsectors);
		return 1;
	}*/
	return 0;
}

static int lib_nummapheaders(lua_State *L)
{
	lua_pushinteger(L, NUMMAPS);
	return 1;
}

/////////////////
// mapheader_t //
/////////////////

enum mapheaderinfo_e
{
	mapheaderinfo_lvlttl,
	mapheaderinfo_subttl,
	mapheaderinfo_actnum,
	mapheaderinfo_typeoflevel,
	mapheaderinfo_nextlevel,
	mapheaderinfo_marathonnext,
	mapheaderinfo_keywords,
	mapheaderinfo_musname,
	mapheaderinfo_mustrack,
	mapheaderinfo_muspos,
	mapheaderinfo_musinterfadeout,
	mapheaderinfo_musintername,
	mapheaderinfo_muspostbossname,
	mapheaderinfo_muspostbosstrack,
	mapheaderinfo_muspostbosspos,
	mapheaderinfo_muspostbossfadein,
	mapheaderinfo_musforcereset,
	mapheaderinfo_forcecharacter,
	mapheaderinfo_weather,
	mapheaderinfo_skynum,
	mapheaderinfo_skybox_scalex,
	mapheaderinfo_skybox_scaley,
	mapheaderinfo_skybox_scalez,
	mapheaderinfo_interscreen,
	mapheaderinfo_runsoc,
	mapheaderinfo_scriptname,
	mapheaderinfo_precutscenenum,
	mapheaderinfo_cutscenenum,
	mapheaderinfo_countdown,
	mapheaderinfo_palette,
	mapheaderinfo_numlaps,
	mapheaderinfo_unlockrequired,
	mapheaderinfo_levelselect,
	mapheaderinfo_bonustype,
	mapheaderinfo_ltzzpatch,
	mapheaderinfo_ltzztext,
	mapheaderinfo_ltactdiamond,
	mapheaderinfo_maxbonuslives,
	mapheaderinfo_levelflags,
	mapheaderinfo_menuflags,
	mapheaderinfo_selectheading,
	mapheaderinfo_startrings,
	mapheaderinfo_sstimer,
	mapheaderinfo_ssspheres,
	mapheaderinfo_gravity,
};

static const char *const mapheaderinfo_opt[] = {
	"lvlttl",
	"subttl",
	"actnum",
	"typeoflevel",
	"nextlevel",
	"marathonnext",
	"keywords",
	"musname",
	"mustrack",
	"muspos",
	"musinterfadeout",
	"musintername",
	"muspostbossname",
	"muspostbosstrack",
	"muspostbosspos",
	"muspostbossfadein",
	"musforcereset",
	"forcecharacter",
	"weather",
	"skynum",
	"skybox_scalex",
	"skybox_scaley",
	"skybox_scalez",
	"interscreen",
	"runsoc",
	"scriptname",
	"precutscenenum",
	"cutscenenum",
	"countdown",
	"palette",
	"numlaps",
	"unlockrequired",
	"levelselect",
	"bonustype",
	"ltzzpatch",
	"ltzztext",
	"ltactdiamond",
	"maxbonuslives",
	"levelflags",
	"menuflags",
	"selectheading",
	"startrings",
	"sstimer",
	"ssspheres",
	"gravity",
	NULL,
};

static int mapheaderinfo_fields_ref = LUA_NOREF;

static int mapheaderinfo_get(lua_State *L)
{
	mapheader_t *header = *((mapheader_t **)luaL_checkudata(L, 1, META_MAPHEADER));
	enum mapheaderinfo_e field = Lua_optoption(L, 2, -1, mapheaderinfo_fields_ref);
	INT16 i;

	switch (field)
	{
	case mapheaderinfo_lvlttl:
		lua_pushstring(L, header->lvlttl);
		break;
	case mapheaderinfo_subttl:
		lua_pushstring(L, header->subttl);
		break;
	case mapheaderinfo_actnum:
		lua_pushinteger(L, header->actnum);
		break;
	case mapheaderinfo_typeoflevel:
		lua_pushinteger(L, header->typeoflevel);
		break;
	case mapheaderinfo_nextlevel:
		lua_pushinteger(L, header->nextlevel);
		break;
	case mapheaderinfo_marathonnext:
		lua_pushinteger(L, header->marathonnext);
		break;
	case mapheaderinfo_keywords:
		lua_pushstring(L, header->keywords);
		break;
	case mapheaderinfo_musname:
		lua_pushstring(L, header->musname);
		break;
	case mapheaderinfo_mustrack:
		lua_pushinteger(L, header->mustrack);
		break;
	case mapheaderinfo_muspos:
		lua_pushinteger(L, header->muspos);
		break;
	case mapheaderinfo_musinterfadeout:
		lua_pushinteger(L, header->musinterfadeout);
		break;
	case mapheaderinfo_musintername:
		lua_pushstring(L, header->musintername);
		break;
	case mapheaderinfo_muspostbossname:
		lua_pushstring(L, header->muspostbossname);
		break;
	case mapheaderinfo_muspostbosstrack:
		lua_pushinteger(L, header->muspostbosstrack);
		break;
	case mapheaderinfo_muspostbosspos:
		lua_pushinteger(L, header->muspostbosspos);
		break;
	case mapheaderinfo_muspostbossfadein:
		lua_pushinteger(L, header->muspostbossfadein);
		break;
	case mapheaderinfo_musforcereset:
		lua_pushinteger(L, header->musforcereset);
		break;
	case mapheaderinfo_forcecharacter:
		lua_pushstring(L, header->forcecharacter);
		break;
	case mapheaderinfo_weather:
		lua_pushinteger(L, header->weather);
		break;
	case mapheaderinfo_skynum:
		lua_pushinteger(L, header->skynum);
		break;
	case mapheaderinfo_skybox_scalex:
		lua_pushinteger(L, header->skybox_scalex);
		break;
	case mapheaderinfo_skybox_scaley:
		lua_pushinteger(L, header->skybox_scaley);
		break;
	case mapheaderinfo_skybox_scalez:
		lua_pushinteger(L, header->skybox_scalez);
		break;
	case mapheaderinfo_interscreen:
		for (i = 0; i < 8; i++)
			if (!header->interscreen[i])
				break;
		lua_pushlstring(L, header->interscreen, i);
		break;
	case mapheaderinfo_runsoc:
		lua_pushstring(L, header->runsoc);
		break;
	case mapheaderinfo_scriptname:
		lua_pushstring(L, header->scriptname);
		break;
	case mapheaderinfo_precutscenenum:
		lua_pushinteger(L, header->precutscenenum);
		break;
	case mapheaderinfo_cutscenenum:
		lua_pushinteger(L, header->cutscenenum);
		break;
	case mapheaderinfo_countdown:
		lua_pushinteger(L, header->countdown);
		break;
	case mapheaderinfo_palette:
		lua_pushinteger(L, header->palette);
		break;
	case mapheaderinfo_numlaps:
		lua_pushinteger(L, header->numlaps);
		break;
	case mapheaderinfo_unlockrequired:
		lua_pushinteger(L, header->unlockrequired);
		break;
	case mapheaderinfo_levelselect:
		lua_pushinteger(L, header->levelselect);
		break;
	case mapheaderinfo_bonustype:
		lua_pushinteger(L, header->bonustype);
		break;
	case mapheaderinfo_ltzzpatch:
		lua_pushstring(L, header->ltzzpatch);
		break;
	case mapheaderinfo_ltzztext:
		lua_pushstring(L, header->ltzztext);
		break;
	case mapheaderinfo_ltactdiamond:
		lua_pushstring(L, header->ltactdiamond);
		break;
	case mapheaderinfo_maxbonuslives:
		lua_pushinteger(L, header->maxbonuslives);
		break;
	case mapheaderinfo_levelflags:
		lua_pushinteger(L, header->levelflags);
		break;
	case mapheaderinfo_menuflags:
		lua_pushinteger(L, header->menuflags);
		break;
	case mapheaderinfo_selectheading:
		lua_pushstring(L, header->selectheading);
		break;
	case mapheaderinfo_startrings:
		lua_pushinteger(L, header->startrings);
		break;
	case mapheaderinfo_sstimer:
		lua_pushinteger(L, header->sstimer);
		break;
	case mapheaderinfo_ssspheres:
		lua_pushinteger(L, header->ssspheres);
		break;
	case mapheaderinfo_gravity:
		lua_pushfixed(L, header->gravity);
		break;
	// TODO add support for reading numGradedMares and grades
	default:
	{
		// Read custom vars now
		// (note: don't include the "LUA." in your lua scripts!)
		UINT8 j = 0;
		for (;j < header->numCustomOptions && !fastcmp(lua_tostring(L, 2), header->customopts[j].option); ++j);

		if(j < header->numCustomOptions)
			lua_pushstring(L, header->customopts[j].value);
		else
			lua_pushnil(L);
	}
	}
	return 1;
}

int LUA_MapLib(lua_State *L)
{
	LUA_RegisterUserdataMetatable(L, META_SECTORLINES, sectorlines_get, NULL, sectorlines_num);
	LUA_RegisterUserdataMetatable(L, META_SECTOR, sector_get, sector_set, sector_num);
	LUA_RegisterUserdataMetatable(L, META_SUBSECTOR, subsector_get, NULL, subsector_num);
	LUA_RegisterUserdataMetatable(L, META_LINE, line_get, NULL, line_num);
	LUA_RegisterUserdataMetatable(L, META_LINEARGS, lineargs_get, NULL, lineargs_len);
	LUA_RegisterUserdataMetatable(L, META_LINESTRINGARGS, linestringargs_get, NULL, linestringargs_len);
	LUA_RegisterUserdataMetatable(L, META_SIDENUM, sidenum_get, NULL, NULL);
	LUA_RegisterUserdataMetatable(L, META_SIDE, side_get, side_set, side_num);
	LUA_RegisterUserdataMetatable(L, META_VERTEX, vertex_get, NULL, vertex_num);
	LUA_RegisterUserdataMetatable(L, META_FFLOOR, ffloor_get, ffloor_set, NULL);
	LUA_RegisterUserdataMetatable(L, META_BBOX, bbox_get, NULL, NULL);
	LUA_RegisterUserdataMetatable(L, META_SLOPE, slope_get, slope_set, NULL);
	LUA_RegisterUserdataMetatable(L, META_VECTOR2, vector2_get, NULL, NULL);
	LUA_RegisterUserdataMetatable(L, META_VECTOR3, vector3_get, NULL, NULL);
	LUA_RegisterUserdataMetatable(L, META_MAPHEADER, mapheaderinfo_get, NULL, NULL);

	sector_fields_ref = Lua_CreateFieldTable(L, sector_opt);
	subsector_fields_ref = Lua_CreateFieldTable(L, subsector_opt);
	line_fields_ref = Lua_CreateFieldTable(L, line_opt);
	side_fields_ref = Lua_CreateFieldTable(L, side_opt);
	vertex_fields_ref = Lua_CreateFieldTable(L, vertex_opt);
	ffloor_fields_ref = Lua_CreateFieldTable(L, ffloor_opt);
	slope_fields_ref = Lua_CreateFieldTable(L, slope_opt);
	mapheaderinfo_fields_ref = Lua_CreateFieldTable(L, mapheaderinfo_opt);

	LUA_RegisterGlobalUserdata(L, "subsectors", lib_getSubsector, NULL, lib_numsubsectors);
	LUA_RegisterGlobalUserdata(L, "sides", lib_getSide, NULL, lib_numsides);
	LUA_RegisterGlobalUserdata(L, "vertexes", lib_getVertex, NULL, lib_numvertexes);
	LUA_RegisterGlobalUserdata(L, "mapheaderinfo", lib_getMapheaderinfo, NULL, lib_nummapheaders);

	LUA_PushTaggableObjectArray(L, "sectors",
			lib_iterateSectors,
			lib_getSector,
			lib_numsectors,
			tags_sectors,
			&numsectors, &sectors,
			sizeof (sector_t), META_SECTOR);

	LUA_PushTaggableObjectArray(L, "lines",
			lib_iterateLines,
			lib_getLine,
			lib_numlines,
			tags_lines,
			&numlines, &lines,
			sizeof (line_t), META_LINE);

#ifdef HAVE_LUA_SEGS
	LUA_RegisterUserdataMetatable(L, META_SEG, seg_get, NULL, seg_num);
	LUA_RegisterUserdataMetatable(L, META_NODE, node_get, NULL, node_num);
	LUA_RegisterUserdataMetatable(L, META_NODECHILDREN, nodechildren_get, NULL, NULL);

	seg_fields_ref = Lua_CreateFieldTable(L, seg_opt);
	node_fields_ref = Lua_CreateFieldTable(L, node_opt);

	luaL_newmetatable(L, META_NODEBBOX);
		//LUA_SetCFunctionField(L, "__index", nodebbox_get);
		LUA_SetCFunctionField(L, "__call", nodebbox_call);
	lua_pop(L, 1);

	LUA_RegisterGlobalUserdata(L, "segs", lib_getSeg, NULL, lib_numsegs);
	LUA_RegisterGlobalUserdata(L, "nodes", lib_getNode, NULL, lib_numnodes);
#endif

	return 0;
}
