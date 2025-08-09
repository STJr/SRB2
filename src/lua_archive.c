// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_archive.c
/// \brief Lua gamestate archival

#include "lua_archive.h"
#include "doomtype.h"
#include "fastcmp.h"
#include "g_game.h"
#include "g_input.h"
#include "lua_script.h"
#include "lua_libs.h"
#include "matrix.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "p_slopes.h"
#include "p_spec.h"
#include "quaternion.h"
#include "r_skins.h"
#include "r_state.h"

enum
{
	ARCH_NULL=0,
	ARCH_TRUE,
	ARCH_FALSE,
	ARCH_INT8,
	ARCH_INT16,
	ARCH_INT32,
	ARCH_SMALLSTRING,
	ARCH_LARGESTRING,
	ARCH_TABLE,

	ARCH_MOBJINFO,
	ARCH_STATE,
	ARCH_MOBJ,
	ARCH_PLAYER,
	ARCH_MAPTHING,
	ARCH_VERTEX,
	ARCH_LINE,
	ARCH_SIDE,
	ARCH_SUBSECTOR,
	ARCH_SECTOR,
	ARCH_FFLOOR,
	ARCH_POLYOBJ,
	ARCH_SLOPE,
	ARCH_MAPHEADER,
	ARCH_SKINCOLOR,
	ARCH_MOUSE,
	ARCH_SKIN,

	ARCH_VECTOR2,
	ARCH_VECTOR3,
	ARCH_MATRIX,
	ARCH_QUATERNION,

	ARCH_TEND=0xFF,
};

static const struct {
	const char *meta;
	UINT8 arch;
} meta2arch[] = {
	{META_MOBJINFO, ARCH_MOBJINFO},
	{META_STATE,    ARCH_STATE},
	{META_MOBJ,     ARCH_MOBJ},
	{META_PLAYER,   ARCH_PLAYER},
	{META_MAPTHING, ARCH_MAPTHING},
	{META_VERTEX,   ARCH_VERTEX},
	{META_LINE,     ARCH_LINE},
	{META_SIDE,     ARCH_SIDE},
	{META_SUBSECTOR,ARCH_SUBSECTOR},
	{META_SECTOR,   ARCH_SECTOR},
	{META_FFLOOR,	ARCH_FFLOOR},
	{META_POLYOBJ,  ARCH_POLYOBJ},
	{META_SLOPE,    ARCH_SLOPE},
	{META_MAPHEADER,   ARCH_MAPHEADER},
	{META_SKINCOLOR,   ARCH_SKINCOLOR},
	{META_MOUSE,    ARCH_MOUSE},
	{META_SKIN,     ARCH_SKIN},
	{META_VECTOR2,    ARCH_VECTOR2},
	{META_VECTOR3,    ARCH_VECTOR3},
	{META_MATRIX,     ARCH_MATRIX},
	{META_QUATERNION, ARCH_QUATERNION},
	{NULL,          ARCH_NULL}
};

static UINT8 GetUserdataArchType(int index)
{
	UINT8 i;
	lua_getmetatable(gL, index);

	for (i = 0; meta2arch[i].meta; i++)
	{
		luaL_getmetatable(gL, meta2arch[i].meta);
		if (lua_rawequal(gL, -1, -2))
		{
			lua_pop(gL, 2);
			return meta2arch[i].arch;
		}
		lua_pop(gL, 1);
	}

	lua_pop(gL, 1);
	return ARCH_NULL;
}

static void *PrepareArchiveLuaUserdata(lua_State *L, save_t *save_p, int myindex, int archtype, int USERDATAINDEX)
{
	boolean found = false;
	INT32 i;
	UINT16 t = (UINT16)lua_objlen(gL, USERDATAINDEX);

	for (i = 1; i <= t && !found; i++)
	{
		lua_rawgeti(gL, USERDATAINDEX, i);
		if (lua_rawequal(gL, myindex, -1))
		{
			t = i;
			found = true;
		}
		lua_pop(gL, 1);
	}
	if (!found)
	{
		t++;

		if (t == 0)
		{
			CONS_Alert(CONS_ERROR, "Too much userdata to archive!\n");
			P_WriteUINT8(save_p, ARCH_NULL);
			return NULL;
		}
	}

	P_WriteUINT8(save_p, archtype);
	P_WriteUINT16(save_p, t);

	if (found)
	{
		return NULL;
	}
	else
	{
		lua_pushvalue(gL, myindex);
		lua_rawseti(gL, USERDATAINDEX, t);

		return lua_touserdata(L, myindex);
	}
}

static UINT8 ArchiveValue(save_t *save_p, int TABLESINDEX, int USERDATAINDEX, int myindex)
{
	if (myindex < 0)
		myindex = lua_gettop(gL)+1+myindex;
	switch (lua_type(gL, myindex))
	{
	case LUA_TNONE:
	case LUA_TNIL:
		P_WriteUINT8(save_p, ARCH_NULL);
		break;
	// This might be a problem. D:
	case LUA_TLIGHTUSERDATA:
	case LUA_TTHREAD:
	case LUA_TFUNCTION:
		P_WriteUINT8(save_p, ARCH_NULL);
		return 2;
	case LUA_TBOOLEAN:
		P_WriteUINT8(save_p, lua_toboolean(gL, myindex) ? ARCH_TRUE : ARCH_FALSE);
		break;
	case LUA_TNUMBER:
	{
		lua_Integer number = lua_tointeger(gL, myindex);
		if (number >= INT8_MIN && number <= INT8_MAX)
		{
			P_WriteUINT8(save_p, ARCH_INT8);
			P_WriteSINT8(save_p, number);
		}
		else if (number >= INT16_MIN && number <= INT16_MAX)
		{
			P_WriteUINT8(save_p, ARCH_INT16);
			P_WriteINT16(save_p, number);
		}
		else
		{
			P_WriteUINT8(save_p, ARCH_INT32);
			P_WriteFixed(save_p, number);
		}
		break;
	}
	case LUA_TSTRING:
	{
		UINT32 len = (UINT32)lua_objlen(gL, myindex); // get length of string, including embedded zeros
		const char *s = lua_tostring(gL, myindex);
		UINT32 i = 0;
		// if you're wondering why we're writing a string to save_p this way,
		// it turns out that Lua can have embedded zeros ('\0') in the strings,
		// so we can't use P_WriteString as that cuts off when it finds a '\0'.
		// Saving the size of the string also allows us to get the size of the string on the other end,
		// fixing the awful crashes previously encountered for reading strings longer than 1024
		// (yes I know that's kind of a stupid thing to care about, but it'd be evil to trim or ignore them?)
		// -- Monster Iestyn 05/08/18
		if (len < 255)
		{
			P_WriteUINT8(save_p, ARCH_SMALLSTRING);
			P_WriteUINT8(save_p, len); // save size of string
		}
		else
		{
			P_WriteUINT8(save_p, ARCH_LARGESTRING);
			P_WriteUINT32(save_p, len); // save size of string
		}
		while (i < len)
			P_WriteChar(save_p, s[i++]); // write chars individually, including the embedded zeros
		break;
	}
	case LUA_TTABLE:
	{
		boolean found = false;
		INT32 i;
		UINT16 t = (UINT16)lua_objlen(gL, TABLESINDEX);

		for (i = 1; i <= t && !found; i++)
		{
			lua_rawgeti(gL, TABLESINDEX, i);
			if (lua_rawequal(gL, myindex, -1))
			{
				t = i;
				found = true;
			}
			lua_pop(gL, 1);
		}
		if (!found)
		{
			t++;

			if (t == 0)
			{
				CONS_Alert(CONS_ERROR, "Too many tables to archive!\n");
				P_WriteUINT8(save_p, ARCH_NULL);
				return 0;
			}
		}

		P_WriteUINT8(save_p, ARCH_TABLE);
		P_WriteUINT16(save_p, t);

		if (!found)
		{
			lua_pushvalue(gL, myindex);
			lua_rawseti(gL, TABLESINDEX, t);
			return 1;
		}
		break;
	}
	case LUA_TUSERDATA:
		switch (GetUserdataArchType(myindex))
		{
		case ARCH_MOBJINFO:
		{
			mobjinfo_t *info = *((mobjinfo_t **)lua_touserdata(gL, myindex));
			P_WriteUINT8(save_p, ARCH_MOBJINFO);
			P_WriteUINT16(save_p, info - mobjinfo);
			break;
		}
		case ARCH_STATE:
		{
			state_t *state = *((state_t **)lua_touserdata(gL, myindex));
			P_WriteUINT8(save_p, ARCH_STATE);
			P_WriteUINT16(save_p, state - states);
			break;
		}
		case ARCH_MOBJ:
		{
			mobj_t *mobj = *((mobj_t **)lua_touserdata(gL, myindex));
			if (!mobj)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_MOBJ);
				P_WriteUINT32(save_p, mobj->mobjnum);
			}
			break;
		}
		case ARCH_PLAYER:
		{
			player_t *player = *((player_t **)lua_touserdata(gL, myindex));
			if (!player)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_PLAYER);
				P_WriteUINT8(save_p, player - players);
			}
			break;
		}
		case ARCH_MAPTHING:
		{
			mapthing_t *mapthing = *((mapthing_t **)lua_touserdata(gL, myindex));
			if (!mapthing)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_MAPTHING);
				P_WriteUINT16(save_p, mapthing - mapthings);
			}
			break;
		}
		case ARCH_VERTEX:
		{
			vertex_t *vertex = *((vertex_t **)lua_touserdata(gL, myindex));
			if (!vertex)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_VERTEX);
				P_WriteUINT16(save_p, vertex - vertexes);
			}
			break;
		}
		case ARCH_LINE:
		{
			line_t *line = *((line_t **)lua_touserdata(gL, myindex));
			if (!line)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_LINE);
				P_WriteUINT16(save_p, line - lines);
			}
			break;
		}
		case ARCH_SIDE:
		{
			side_t *side = *((side_t **)lua_touserdata(gL, myindex));
			if (!side)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_SIDE);
				P_WriteUINT16(save_p, side - sides);
			}
			break;
		}
		case ARCH_SUBSECTOR:
		{
			subsector_t *subsector = *((subsector_t **)lua_touserdata(gL, myindex));
			if (!subsector)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_SUBSECTOR);
				P_WriteUINT16(save_p, subsector - subsectors);
			}
			break;
		}
		case ARCH_SECTOR:
		{
			sector_t *sector = *((sector_t **)lua_touserdata(gL, myindex));
			if (!sector)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_SECTOR);
				P_WriteUINT16(save_p, sector - sectors);
			}
			break;
		}
		case ARCH_FFLOOR:
		{
			ffloor_t *rover = *((ffloor_t **)lua_touserdata(gL, myindex));
			if (!rover)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				UINT16 i = P_GetFFloorID(rover);
				if (i == UINT16_MAX) // invalid ID
					P_WriteUINT8(save_p, ARCH_NULL);
				else
				{
					P_WriteUINT8(save_p, ARCH_FFLOOR);
					P_WriteUINT16(save_p, rover->target - sectors);
					P_WriteUINT16(save_p, i);
				}
			}
			break;
		}
		case ARCH_POLYOBJ:
		{
			polyobj_t *polyobj = *((polyobj_t **)lua_touserdata(gL, myindex));
			if (!polyobj)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_POLYOBJ);
				P_WriteUINT16(save_p, polyobj-PolyObjects);
			}
			break;
		}
		case ARCH_SLOPE:
		{
			pslope_t *slope = *((pslope_t **)lua_touserdata(gL, myindex));
			if (!slope)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_SLOPE);
				P_WriteUINT16(save_p, slope->id);
			}
			break;
		}
		case ARCH_MAPHEADER:
		{
			mapheader_t *header = *((mapheader_t **)lua_touserdata(gL, myindex));
			if (!header)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_MAPHEADER);
				P_WriteUINT16(save_p, header - *mapheaderinfo);
			}
			break;
		}
		case ARCH_SKINCOLOR:
		{
			skincolor_t *info = *((skincolor_t **)lua_touserdata(gL, myindex));
			P_WriteUINT8(save_p, ARCH_SKINCOLOR);
			P_WriteUINT16(save_p, info - skincolors);
			break;
		}
		case ARCH_MOUSE:
		{
			mouse_t *m = *((mouse_t **)lua_touserdata(gL, myindex));
			P_WriteUINT8(save_p, ARCH_MOUSE);
			P_WriteUINT8(save_p, m == &mouse ? 1 : 2);
			break;
		}
		case ARCH_SKIN:
		{
			skin_t *skin = *((skin_t **)lua_touserdata(gL, myindex));
			P_WriteUINT8(save_p, ARCH_SKIN);
			P_WriteUINT8(save_p, skin->skinnum); // UINT8 because MAXSKINS must be <= 256
			break;
		}
		case ARCH_VECTOR2:
		{
			vector2_t *vector = PrepareArchiveLuaUserdata(gL, save_p, myindex, ARCH_VECTOR2, USERDATAINDEX);
			if (vector)
			{
				P_WriteFixed(save_p, vector->x);
				P_WriteFixed(save_p, vector->y);
			}
			break;
		}
		case ARCH_VECTOR3:
		{
			vector3_t *vector = PrepareArchiveLuaUserdata(gL, save_p, myindex, ARCH_VECTOR3, USERDATAINDEX);
			if (vector)
			{
				P_WriteFixed(save_p, vector->x);
				P_WriteFixed(save_p, vector->y);
				P_WriteFixed(save_p, vector->z);
			}
			break;
		}
		case ARCH_MATRIX:
		{
			matrix_t *matrix = PrepareArchiveLuaUserdata(gL, save_p, myindex, ARCH_MATRIX, USERDATAINDEX);
			if (matrix)
			{
				for (size_t r = 0; r < 4; r++)
					for (size_t c = 0; c < 4; c++)
						P_WriteFixed(save_p, matrix->matrix[r][c]);
			}
			break;
		}
		case ARCH_QUATERNION:
		{
			quaternion_t *quat = PrepareArchiveLuaUserdata(gL, save_p, myindex, ARCH_QUATERNION, USERDATAINDEX);
			if (quat)
			{
				P_WriteFixed(save_p, quat->x);
				P_WriteFixed(save_p, quat->y);
				P_WriteFixed(save_p, quat->z);
				P_WriteFixed(save_p, quat->w);
			}
			break;
		}
		default:
			P_WriteUINT8(save_p, ARCH_NULL);
			return 2;
		}
		break;
	}
	return 0;
}

static void ArchiveExtVars(save_t *save_p, void *pointer, const char *ptype)
{
	int TABLESINDEX;
	int USERDATAINDEX;
	UINT16 i;

	if (!gL) {
		if (fastcmp(ptype,"player")) // players must always be included, even if no vars
			P_WriteUINT16(save_p, 0);
		return;
	}

	TABLESINDEX = lua_gettop(gL) - 1;
	USERDATAINDEX = lua_gettop(gL);

	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_EXTVARS);
	I_Assert(lua_istable(gL, -1));
	lua_pushlightuserdata(gL, pointer);
	lua_rawget(gL, -2);
	lua_remove(gL, -2); // pop LREG_EXTVARS

	if (!lua_istable(gL, -1))
	{ // no extra values table
		lua_pop(gL, 1);
		if (fastcmp(ptype,"player")) // players must always be included, even if no vars
			P_WriteUINT16(save_p, 0);
		return;
	}

	lua_pushnil(gL);
	for (i = 0; lua_next(gL, -2); i++)
		lua_pop(gL, 1);

	// skip anything that has an empty table and isn't a player.
	if (i == 0)
	{
		if (fastcmp(ptype,"player")) // always include players even if they have no extra variables
			P_WriteUINT16(save_p, 0);
		lua_pop(gL, 1);
		return;
	}

	if (fastcmp(ptype,"mobj")) // mobjs must write their mobjnum as a header
		P_WriteUINT32(save_p, ((mobj_t *)pointer)->mobjnum);
	P_WriteUINT16(save_p, i);
	lua_pushnil(gL);
	while (lua_next(gL, -2))
	{
		I_Assert(lua_type(gL, -2) == LUA_TSTRING);
		P_WriteString(save_p, lua_tostring(gL, -2));
		if (ArchiveValue(save_p, TABLESINDEX, USERDATAINDEX, -1) == 2)
			CONS_Alert(CONS_ERROR, "Type of value for %s entry '%s' (%s) could not be archived!\n", ptype, lua_tostring(gL, -2), luaL_typename(gL, -1));
		lua_pop(gL, 1);
	}

	lua_pop(gL, 1);
}

// FIXME: remove and pass as local variable
static save_t *lua_save_p;

static int NetArchive(lua_State *L)
{
	int TABLESINDEX = lua_upvalueindex(1);
	int USERDATAINDEX = lua_upvalueindex(2);
	int i, n = lua_gettop(L);
	for (i = 1; i <= n; i++)
		ArchiveValue(lua_save_p, TABLESINDEX, USERDATAINDEX, i);
	return n;
}

static void ArchiveTables(save_t *save_p)
{
	int TABLESINDEX;
	int USERDATAINDEX;
	UINT16 i, n;
	UINT8 e;

	if (!gL)
		return;

	TABLESINDEX = lua_gettop(gL) - 1;
	USERDATAINDEX = lua_gettop(gL);

	n = (UINT16)lua_objlen(gL, TABLESINDEX);
	for (i = 1; i <= n; i++)
	{
		lua_rawgeti(gL, TABLESINDEX, i);
		lua_pushnil(gL);
		while (lua_next(gL, -2))
		{
			// Write key
			e = ArchiveValue(save_p, TABLESINDEX, USERDATAINDEX, -2); // key should be either a number or a string, ArchiveValue can handle this.
			if (e == 1)
				n++; // the table contained a new table we'll have to archive. :(
			else if (e == 2) // invalid key type (function, thread, lightuserdata, or anything we don't recognise)
				CONS_Alert(CONS_ERROR, "Index '%s' (%s) of table %d could not be archived!\n", lua_tostring(gL, -2), luaL_typename(gL, -2), i);

			// Write value
			e = ArchiveValue(save_p, TABLESINDEX, USERDATAINDEX, -1);
			if (e == 1)
				n++; // the table contained a new table we'll have to archive. :(
			else if (e == 2) // invalid value type
				CONS_Alert(CONS_ERROR, "Type of value for table %d entry '%s' (%s) could not be archived!\n", i, lua_tostring(gL, -2), luaL_typename(gL, -1));

			lua_pop(gL, 1);
		}
		P_WriteUINT8(save_p, ARCH_TEND);

		// Write metatable ID
		if (lua_getmetatable(gL, -1))
		{
			// registry.metatables[metatable]
			lua_getfield(gL, LUA_REGISTRYINDEX, LREG_METATABLES);
			lua_pushvalue(gL, -2);
			lua_gettable(gL, -2);
			P_WriteUINT16(save_p, lua_isnil(gL, -1) ? 0 : lua_tointeger(gL, -1));
			lua_pop(gL, 3);
		}
		else
			P_WriteUINT16(save_p, 0);

		lua_pop(gL, 1);
	}
}

static void *PrepareUnarchiveLuaUserdata(lua_State *L, save_t *save_p, const char *meta, size_t size, int USERDATAINDEX)
{
	UINT16 tid = P_ReadUINT16(save_p);
	lua_rawgeti(L, USERDATAINDEX, tid);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);

		void *ud = lua_newuserdata(L, size);

		luaL_getmetatable(L, meta);
		lua_setmetatable(L, -2);

		lua_pushvalue(L, -1);
		lua_rawseti(L, USERDATAINDEX, tid);

		return ud;
	}
	else
	{
		return NULL;
	}
}

static UINT8 UnArchiveValue(save_t *save_p, int TABLESINDEX, int USERDATAINDEX)
{
	UINT8 type = P_ReadUINT8(save_p);
	switch (type)
	{
	case ARCH_NULL:
		lua_pushnil(gL);
		break;
	case ARCH_TRUE:
		lua_pushboolean(gL, true);
		break;
	case ARCH_FALSE:
		lua_pushboolean(gL, false);
		break;
	case ARCH_INT8:
		lua_pushinteger(gL, P_ReadSINT8(save_p));
		break;
	case ARCH_INT16:
		lua_pushinteger(gL, P_ReadINT16(save_p));
		break;
	case ARCH_INT32:
		lua_pushinteger(gL, P_ReadFixed(save_p));
		break;
	case ARCH_SMALLSTRING:
	case ARCH_LARGESTRING:
	{
		UINT32 len;
		char *value;
		UINT32 i = 0;

		// See my comments in the ArchiveValue function;
		// it's much the same for reading strings as writing them!
		// (i.e. we can't use P_ReadString either)
		// -- Monster Iestyn 05/08/18
		if (type == ARCH_SMALLSTRING)
			len = P_ReadUINT8(save_p); // length of string, including embedded zeros
		else
			len = P_ReadUINT32(save_p); // length of string, including embedded zeros
		value = malloc(len); // make temp buffer of size len
		// now read the actual string
		while (i < len)
			value[i++] = P_ReadChar(save_p); // read chars individually, including the embedded zeros
		lua_pushlstring(gL, value, len); // push the string (note: this function supports embedded zeros)
		free(value); // free the buffer
		break;
	}
	case ARCH_TABLE:
	{
		UINT16 tid = P_ReadUINT16(save_p);
		lua_rawgeti(gL, TABLESINDEX, tid);
		if (lua_isnil(gL, -1))
		{
			lua_pop(gL, 1);
			lua_newtable(gL);
			lua_pushvalue(gL, -1);
			lua_rawseti(gL, TABLESINDEX, tid);
			return 2;
		}
		break;
	}
	case ARCH_MOBJINFO:
		LUA_PushUserdata(gL, &mobjinfo[P_ReadUINT16(save_p)], META_MOBJINFO);
		break;
	case ARCH_STATE:
		LUA_PushUserdata(gL, &states[P_ReadUINT16(save_p)], META_STATE);
		break;
	case ARCH_MOBJ:
		LUA_PushUserdata(gL, P_FindNewPosition(P_ReadUINT32(save_p)), META_MOBJ);
		break;
	case ARCH_PLAYER:
		LUA_PushUserdata(gL, &players[P_ReadUINT8(save_p)], META_PLAYER);
		break;
	case ARCH_MAPTHING:
		LUA_PushUserdata(gL, &mapthings[P_ReadUINT16(save_p)], META_MAPTHING);
		break;
	case ARCH_VERTEX:
		LUA_PushUserdata(gL, &vertexes[P_ReadUINT16(save_p)], META_VERTEX);
		break;
	case ARCH_LINE:
		LUA_PushUserdata(gL, &lines[P_ReadUINT16(save_p)], META_LINE);
		break;
	case ARCH_SIDE:
		LUA_PushUserdata(gL, &sides[P_ReadUINT16(save_p)], META_SIDE);
		break;
	case ARCH_SUBSECTOR:
		LUA_PushUserdata(gL, &subsectors[P_ReadUINT16(save_p)], META_SUBSECTOR);
		break;
	case ARCH_SECTOR:
		LUA_PushUserdata(gL, &sectors[P_ReadUINT16(save_p)], META_SECTOR);
		break;
	case ARCH_FFLOOR:
	{
		sector_t *sector = &sectors[P_ReadUINT16(save_p)];
		UINT16 id = P_ReadUINT16(save_p);
		ffloor_t *rover = P_GetFFloorByID(sector, id);
		if (rover)
			LUA_PushUserdata(gL, rover, META_FFLOOR);
		break;
	}
	case ARCH_POLYOBJ:
		LUA_PushUserdata(gL, &PolyObjects[P_ReadUINT16(save_p)], META_POLYOBJ);
		break;
	case ARCH_SLOPE:
		LUA_PushUserdata(gL, P_SlopeById(P_ReadUINT16(save_p)), META_SLOPE);
		break;
	case ARCH_MAPHEADER:
		LUA_PushUserdata(gL, mapheaderinfo[P_ReadUINT16(save_p)], META_MAPHEADER);
		break;
	case ARCH_SKINCOLOR:
		LUA_PushUserdata(gL, &skincolors[P_ReadUINT16(save_p)], META_SKINCOLOR);
		break;
	case ARCH_MOUSE:
		LUA_PushUserdata(gL, P_ReadUINT16(save_p) == 1 ? &mouse : &mouse2, META_MOUSE);
		break;
	case ARCH_SKIN:
		LUA_PushUserdata(gL, skins[P_ReadUINT8(save_p)], META_SKIN);
		break;
	case ARCH_VECTOR2:
	{
		vector2_t *vector = PrepareUnarchiveLuaUserdata(gL, save_p, META_VECTOR2, sizeof(vector2_t), USERDATAINDEX);
		if (vector)
		{
			vector->x = P_ReadFixed(save_p);
			vector->y = P_ReadFixed(save_p);
		}
		break;
	}
	case ARCH_VECTOR3:
	{
		vector3_t *vector = PrepareUnarchiveLuaUserdata(gL, save_p, META_VECTOR3, sizeof(vector3_t), USERDATAINDEX);
		if (vector)
		{
			vector->x = P_ReadFixed(save_p);
			vector->y = P_ReadFixed(save_p);
			vector->z = P_ReadFixed(save_p);
		}
		break;
	}
	case ARCH_MATRIX:
	{
		matrix_t *matrix = PrepareUnarchiveLuaUserdata(gL, save_p, META_MATRIX, sizeof(matrix_t), USERDATAINDEX);
		if (matrix)
		{
			for (size_t r = 0; r < 4; r++)
				for (size_t c = 0; c < 4; c++)
					matrix->matrix[r][c] = P_ReadFixed(save_p);
		}
		break;
	}
	case ARCH_QUATERNION:
	{
		quaternion_t *quat = PrepareUnarchiveLuaUserdata(gL, save_p, META_QUATERNION, sizeof(quaternion_t), USERDATAINDEX);
		if (quat)
		{
			quat->x = P_ReadFixed(save_p);
			quat->y = P_ReadFixed(save_p);
			quat->z = P_ReadFixed(save_p);
			quat->w = P_ReadFixed(save_p);
		}
		break;
	}
	case ARCH_TEND:
		return 1;
	}
	return 0;
}

static void UnArchiveExtVars(save_t *save_p, void *pointer)
{
	int TABLESINDEX;
	int USERDATAINDEX;
	UINT16 field_count = P_ReadUINT16(save_p);
	UINT16 i;
	char field[1024];

	if (field_count == 0)
		return;
	I_Assert(gL != NULL);

	TABLESINDEX = lua_gettop(gL) - 1;
	USERDATAINDEX = lua_gettop(gL);
	lua_createtable(gL, 0, field_count); // pointer's ext vars subtable

	for (i = 0; i < field_count; i++)
	{
		P_ReadString(save_p, field);
		UnArchiveValue(save_p, TABLESINDEX, USERDATAINDEX);
		lua_setfield(gL, -2, field);
	}

	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_EXTVARS);
	I_Assert(lua_istable(gL, -1));
	lua_pushlightuserdata(gL, pointer);
	lua_pushvalue(gL, -3); // pointer's ext vars subtable
	lua_rawset(gL, -3);
	lua_pop(gL, 2); // pop LREG_EXTVARS and pointer's subtable
}

static int NetUnArchive(lua_State *L)
{
	int TABLESINDEX = lua_upvalueindex(1);
	int USERDATAINDEX = lua_upvalueindex(2);
	int i, n = lua_gettop(L);
	for (i = 1; i <= n; i++)
		UnArchiveValue(lua_save_p, TABLESINDEX, USERDATAINDEX);
	return n;
}

static void UnArchiveTables(save_t *save_p)
{
	int TABLESINDEX;
	int USERDATAINDEX;
	UINT16 i, n;
	UINT16 metatableid;

	if (!gL)
		return;

	TABLESINDEX = lua_gettop(gL) - 1;
	USERDATAINDEX = lua_gettop(gL);

	n = (UINT16)lua_objlen(gL, TABLESINDEX);
	for (i = 1; i <= n; i++)
	{
		lua_rawgeti(gL, TABLESINDEX, i);
		while (true)
		{
			UINT8 e = UnArchiveValue(save_p, TABLESINDEX, USERDATAINDEX); // read key
			if (e == 1) // End of table
				break;
			else if (e == 2) // Key contains a new table
				n++;

			if (UnArchiveValue(save_p, TABLESINDEX, USERDATAINDEX) == 2) // read value
				n++;

			if (lua_isnil(gL, -2)) // if key is nil (if a function etc was accidentally saved)
			{
				CONS_Alert(CONS_ERROR, "A nil key in table %d was found! (Invalid key type or corrupted save?)\n", i);
				lua_pop(gL, 2); // pop key and value instead of setting them in the table, to prevent Lua panic errors
			}
			else
				lua_rawset(gL, -3);
		}

		metatableid = P_ReadUINT16(save_p);
		if (metatableid)
		{
			// setmetatable(table, registry.metatables[metatableid])
			lua_getfield(gL, LUA_REGISTRYINDEX, LREG_METATABLES);
				lua_rawgeti(gL, -1, metatableid);
				if (lua_isnil(gL, -1))
					I_Error("Unknown metatable ID %d\n", metatableid);
				lua_setmetatable(gL, -3);
			lua_pop(gL, 1);
		}

		lua_pop(gL, 1);
	}
}

void LUA_Archive(save_t *save_p)
{
	INT32 i;
	thinker_t *th;

	if (gL)
	{
		lua_newtable(gL); // tables to be archived.
		lua_newtable(gL); // userdata to be archived.
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!players[i].ingame && i > 0) // dedicated servers...
			continue;
		// all players in game will be archived, even if they just add a 0.
		ArchiveExtVars(save_p, &players[i], "player");
	}

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		// archive function will determine when to skip mobjs,
		// and write mobjnum in otherwise.
		ArchiveExtVars(save_p, th, "mobj");
	}

	P_WriteUINT32(save_p, UINT32_MAX); // end of mobjs marker, replaces mobjnum.

	lua_save_p = save_p;
	LUA_HookNetArchive(NetArchive); // call the NetArchive hook in archive mode
	ArchiveTables(save_p);

	if (gL)
		lua_pop(gL, 2); // pop tables
}

void LUA_UnArchive(save_t *save_p)
{
	UINT32 mobjnum;
	INT32 i;
	thinker_t *th;

	if (gL)
	{
		lua_newtable(gL); // tables to be read
		lua_newtable(gL); // userdata to be read
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!players[i].ingame && i > 0) // dedicated servers...
			continue;
		UnArchiveExtVars(save_p, &players[i]);
	}

	do {
		mobjnum = P_ReadUINT32(save_p); // read a mobjnum
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->removing)
				continue;
			if (((mobj_t *)th)->mobjnum != mobjnum) // find matching mobj
				continue;
			UnArchiveExtVars(save_p, th); // apply variables
		}
	} while(mobjnum != UINT32_MAX); // repeat until end of mobjs marker.

	lua_save_p = save_p;
	LUA_HookNetArchive(NetUnArchive); // call the NetArchive hook in unarchive mode
	UnArchiveTables(save_p);

	if (gL)
		lua_pop(gL, 2); // pop tables
}
