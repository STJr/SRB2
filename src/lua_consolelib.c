// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_consolelib.c
/// \brief console modifying/etc library for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "fastcmp.h"
#include "p_local.h"
#include "g_game.h"
#include "byteptr.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

#define NOHUD if (hud_running) return luaL_error(L, "HUD rendering code should not call this function!");

static const char *cvname = NULL;

void Got_Luacmd(UINT8 **cp, INT32 playernum)
{
	UINT8 i, argc, flags;
	char buf[256];

	// don't use I_Assert here, goto the deny code below
	// to clean up and kick people who try nefarious exploits
	// like sending random junk lua commands to crash the server

	if (!gL) goto deny;
	lua_getfield(gL, LUA_REGISTRYINDEX, "COM_Command"); // push COM_Command
	if (!lua_istable(gL, -1)) goto deny;

	argc = READUINT8(*cp);
	READSTRINGN(*cp, buf, 255);
	strlwr(buf); // must lowercase buffer
	lua_getfield(gL, -1, buf); // push command info table
	if (!lua_istable(gL, -1)) goto deny;

	lua_remove(gL, -2); // pop COM_Command

	lua_rawgeti(gL, -1, 2); // push flags from command info table
	if (lua_isboolean(gL, -1))
		flags = (lua_toboolean(gL, -1) ? 1 : 0);
	else
		flags = (UINT8)lua_tointeger(gL, -1);
	lua_pop(gL, 1); // pop flags

	// requires server/admin and the player is not one of them
	if ((flags & 1) && playernum != serverplayer && playernum != adminplayer)
		goto deny;

	lua_rawgeti(gL, -1, 1); // push function from command info table

	// although honestly this should be true anyway
	// BUT GODDAMNIT I SAID NO I_ASSERTS SO NO I_ASSERTS IT IS
	if (!lua_isfunction(gL, -1)) goto deny;

	lua_remove(gL, -2); // pop command info table

	LUA_PushUserdata(gL, &players[playernum], META_PLAYER);
	for (i = 1; i < argc; i++)
	{
		READSTRINGN(*cp, buf, 255);
		lua_pushstring(gL, buf);
	}
	LUA_Call(gL, (int)argc); // argc is 1-based, so this will cover the player we passed too.
	return;

deny:
	//must be hacked/buggy client
	if (gL) // check if Lua is actually turned on first, you dummmy -- Monster Iestyn 04/07/18
		lua_settop(gL, 0); // clear stack

	CONS_Alert(CONS_WARNING, M_GetText("Illegal lua command received from %s\n"), player_names[playernum]);
	if (server)
	{
		XBOXSTATIC UINT8 bufn[2];

		bufn[0] = (UINT8)playernum;
		bufn[1] = KICK_MSG_CON_FAIL;
		SendNetXCmd(XD_KICK, &bufn, 2);
	}
}

// Wrapper for COM_AddCommand commands
void COM_Lua_f(void)
{
	char *buf, *p;
	UINT8 i, flags;
	UINT16 len;
	INT32 playernum = consoleplayer;

	I_Assert(gL != NULL);
	lua_getfield(gL, LUA_REGISTRYINDEX, "COM_Command"); // push COM_Command
	I_Assert(lua_istable(gL, -1));

	// use buf temporarily -- must use lowercased string
	buf = Z_StrDup(COM_Argv(0));
	strlwr(buf);
	lua_getfield(gL, -1, buf); // push command info table
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -2); // pop COM_Command
	Z_Free(buf);

	lua_rawgeti(gL, -1, 2); // push flags from command info table
	if (lua_isboolean(gL, -1))
		flags = (lua_toboolean(gL, -1) ? 1 : 0);
	else
		flags = (UINT8)lua_tointeger(gL, -1);
	lua_pop(gL, 1); // pop flags

	if (flags & 2) // flag 2: splitscreen player command.
	{
		if (!splitscreen)
		{
			lua_pop(gL, 1); // pop command info table
			return; // can't execute splitscreen command without player 2!
		}
		playernum = secondarydisplayplayer;
	}

	if (netgame)
	{ // Send the command through the network
		UINT8 argc;
		lua_pop(gL, 1); // pop command info table

		if (flags & 1 && !server && adminplayer != playernum) // flag 1: only server/admin can use this command.
		{
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
			return;
		}

		if (COM_Argc() > UINT8_MAX)
			argc = UINT8_MAX;
		else
			argc = (UINT8)COM_Argc();
		if (argc == UINT8_MAX)
			len = UINT16_MAX;
		else
			len = (argc+1)*256;

		buf = malloc(len);
		p = buf;
		WRITEUINT8(p, argc);
		for (i = 0; i < argc; i++)
			WRITESTRINGN(p, COM_Argv(i), 255);
		if (flags & 2)
			SendNetXCmd2(XD_LUACMD, buf, p-buf);
		else
			SendNetXCmd(XD_LUACMD, buf, p-buf);
		free(buf);
		return;
	}

	// Do the command locally, NetXCmds don't go through outside of GS_LEVEL || GS_INTERMISSION
	lua_rawgeti(gL, -1, 1); // push function from command info table
	I_Assert(lua_isfunction(gL, -1));
	lua_remove(gL, -2); // pop command info table

	LUA_PushUserdata(gL, &players[playernum], META_PLAYER);
	for (i = 1; i < COM_Argc(); i++)
		lua_pushstring(gL, COM_Argv(i));
	LUA_Call(gL, (int)COM_Argc()); // COM_Argc is 1-based, so this will cover the player we passed too.
}

// Wrapper for COM_AddCommand
static int lib_comAddCommand(lua_State *L)
{
	int com_return = -1;
	const char *luaname = luaL_checkstring(L, 1);

	// must store in all lowercase
	char *name = Z_StrDup(luaname);
	strlwr(name);

	luaL_checktype(L, 2, LUA_TFUNCTION);
	NOHUD
	if (lua_gettop(L) >= 3)
	{ // For the third argument, only take a boolean or a number.
		lua_settop(L, 3);
		if (lua_type(L, 3) != LUA_TBOOLEAN)
			luaL_checktype(L, 3, LUA_TNUMBER);
	}
	else
	{ // No third argument? Default to 0.
		lua_settop(L, 2);
		lua_pushinteger(L, 0);
	}

	lua_getfield(L, LUA_REGISTRYINDEX, "COM_Command");
	I_Assert(lua_istable(L, -1));

	lua_createtable(L, 2, 0);
		lua_pushvalue(L, 2);
		lua_rawseti(L, -2, 1);

		lua_pushvalue(L, 3);
		lua_rawseti(L, -2, 2);
	lua_setfield(L, -2, name);

	// Try to add the Lua command
	com_return = COM_AddLuaCommand(name);

	if (com_return < 0)
	{ // failed to add -- free the lowercased name and return error
		Z_Free(name);
		return luaL_error(L, "Couldn't add a new console command \"%s\"", luaname);
	}
	else if (com_return == 1)
	{ // command existed already -- free our name as the old string will continue to be used
		CONS_Printf("Replaced command \"%s\"\n", name);
		Z_Free(name);
	}
	else
	{ // new command was added -- do NOT free the string as it will forever be used by the console
		CONS_Printf("Added command \"%s\"\n", name);
	}
	return 0;
}

static int lib_comBufAddText(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	player_t *plr;
	if (n < 2)
		return luaL_error(L, "COM_BufAddText requires two arguments: player and text.");
	NOHUD
	lua_settop(L, 2);
	plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");
	if (plr != &players[consoleplayer])
		return 0;
	COM_BufAddText(va("%s\n", luaL_checkstring(L, 2)));
	return 0;
}

static int lib_comBufInsertText(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	player_t *plr;
	if (n < 2)
		return luaL_error(L, "COM_BufInsertText requires two arguments: player and text.");
	NOHUD
	lua_settop(L, 2);
	plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");
	if (plr != &players[consoleplayer])
		return 0;
	COM_BufInsertText(va("%s\n", luaL_checkstring(L, 2)));
	return 0;
}

void LUA_CVarChanged(const char *name)
{
	cvname = name;
}

static void Lua_OnChange(void)
{
	I_Assert(gL != NULL);
	I_Assert(cvname != NULL);

	/// \todo Network this! XD_LUAVAR

	// From CV_OnChange registry field, get the function for this cvar by name.
	lua_getfield(gL, LUA_REGISTRYINDEX, "CV_OnChange");
	I_Assert(lua_istable(gL, -1));
	lua_getfield(gL, -1, cvname); // get function

	// From the CV_Vars registry field, get the cvar's userdata by name.
	lua_getfield(gL, LUA_REGISTRYINDEX, "CV_Vars");
	I_Assert(lua_istable(gL, -1));
	lua_getfield(gL, -1, cvname); // get consvar_t* userdata.
	lua_remove(gL, -2); // pop the CV_Vars table.

	LUA_Call(gL, 1); // call function(cvar)
	lua_pop(gL, 1); // pop CV_OnChange table
}

static int lib_cvRegisterVar(lua_State *L)
{
	const char *k;
	lua_Integer i;
	consvar_t *cvar;
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1); // Clear out all other possible arguments, leaving only the first one.
	NOHUD
	cvar = lua_newuserdata(L, sizeof(consvar_t));
	luaL_getmetatable(L, META_CVAR);
	lua_setmetatable(L, -2);

#define FIELDERROR(f, e) luaL_error(L, "bad value for " LUA_QL(f) " in table passed to " LUA_QL("CV_RegisterVar") " (%s)", e);
#define TYPEERROR(f, t) FIELDERROR(f, va("%s expected, got %s", lua_typename(L, t), luaL_typename(L, -1)))

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		// stack: cvar table, cvar userdata, key/index, value
		//            1             2            3        4
		i = 0;
		k = NULL;
		if (lua_isnumber(L, 3))
			i = lua_tointeger(L, 3);
		else if (lua_isstring(L, 3))
			k = lua_tostring(L, 3);

		if (i == 1 || (k && fasticmp(k, "name"))) {
			if (!lua_isstring(L, 4))
				TYPEERROR("name", LUA_TSTRING)
			cvar->name = Z_StrDup(lua_tostring(L, 4));
		} else if (i == 2 || (k && fasticmp(k, "defaultvalue"))) {
			if (!lua_isstring(L, 4))
				TYPEERROR("defaultvalue", LUA_TSTRING)
			cvar->defaultvalue = Z_StrDup(lua_tostring(L, 4));
		} else if (i == 3 || (k && fasticmp(k, "flags"))) {
			if (!lua_isnumber(L, 4))
				TYPEERROR("flags", LUA_TNUMBER)
			cvar->flags = (INT32)lua_tointeger(L, 4);
		} else if (i == 4 || (k && fasticmp(k, "PossibleValue"))) {
			if (lua_islightuserdata(L, 4)) {
				CV_PossibleValue_t *pv = lua_touserdata(L, 4);
				if (pv == CV_OnOff || pv == CV_YesNo || pv == CV_Unsigned || pv == CV_Natural)
					cvar->PossibleValue = pv;
				else
					FIELDERROR("PossibleValue", "CV_PossibleValue_t expected, got unrecognised pointer")
			} else if (lua_istable(L, 4)) {
				// Accepts tables in the form of {MIN=0, MAX=9999} or {Red=0, Green=1, Blue=2}
				// and converts them to CV_PossibleValue_t {{0,"MIN"},{9999,"MAX"}} or {{0,"Red"},{1,"Green"},{2,"Blue"}}
				//
				// I don't really like the way this does it because a single PossibleValue table
				// being used for multiple cvars will be converted and stored multiple times.
				// So maybe instead it should be a seperate function which must be run beforehand or something.
				size_t count = 0;
				CV_PossibleValue_t *cvpv;

				lua_pushnil(L);
				while (lua_next(L, 4)) {
					count++;
					lua_pop(L, 1);
				}

				lua_getfield(L, LUA_REGISTRYINDEX, "CV_PossibleValue");
				I_Assert(lua_istable(L, 5));
				lua_pushvalue(L, 2); // cvar userdata
				cvpv = lua_newuserdata(L, sizeof(CV_PossibleValue_t) * (count+1));
				lua_rawset(L, 5);
				lua_pop(L, 1); // pop CV_PossibleValue registry table

				i = 0;
				lua_pushnil(L);
				while (lua_next(L, 4)) {
					// stack: [...] PossibleValue table, index, value
					//                       4             5      6
					if (lua_type(L, 5) != LUA_TSTRING
					|| lua_type(L, 6) != LUA_TNUMBER)
						FIELDERROR("PossibleValue", "custom PossibleValue table requires a format of string=integer, i.e. {MIN=0, MAX=9999}");
					cvpv[i].strvalue = Z_StrDup(lua_tostring(L, 5));
					cvpv[i].value = (INT32)lua_tonumber(L, 6);
					i++;
					lua_pop(L, 1);
				}
				cvpv[i].value = 0;
				cvpv[i].strvalue = NULL;
				cvar->PossibleValue = cvpv;
			} else
				FIELDERROR("PossibleValue", va("%s or CV_PossibleValue_t expected, got %s", lua_typename(L, LUA_TTABLE), luaL_typename(L, -1)))
		} else if (cvar->flags & CV_CALL && (i == 5 || (k && fasticmp(k, "func")))) {
			if (!lua_isfunction(L, 4))
				TYPEERROR("func", LUA_TFUNCTION)
			lua_getfield(L, LUA_REGISTRYINDEX, "CV_OnChange");
			I_Assert(lua_istable(L, 5));
			lua_pushvalue(L, 4);
			lua_setfield(L, 5, cvar->name);
			lua_pop(L, 1);
			cvar->func = Lua_OnChange;
		}
		lua_pop(L, 1);
	}

#undef FIELDERROR
#undef TYPEERROR

	// stack: cvar table, cvar userdata
	lua_getfield(L, LUA_REGISTRYINDEX, "CV_Vars");
	I_Assert(lua_istable(L, 3));

	lua_getfield(L, 3, cvar->name);
	if (lua_type(L, -1) != LUA_TNIL)
		return luaL_error(L, M_GetText("Variable %s is already defined\n"), cvar->name);
	lua_pop(L, 1);

	lua_pushvalue(L, 2);
	lua_setfield(L, 3, cvar->name);
	lua_pop(L, 1);

	// actually time to register it to the console now! Finally!
	cvar->flags |= CV_MODIFIED;
	CV_RegisterVar(cvar);
	if (cvar->flags & CV_MODIFIED)
		return luaL_error(L, "failed to register cvar (probable conflict with internal variable/command names)");

	// return cvar userdata
	return 1;
}

// CONS_Printf for a single player
// Use 'print' in baselib for a global message.
static int lib_consPrintf(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	player_t *plr;
	if (n < 2)
		return luaL_error(L, "CONS_Printf requires at least two arguments: player and text.");
	//HUDSAFE
	plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");
	if (plr != &players[consoleplayer])
		return 0;
	lua_getglobal(L, "tostring");
	for (i=2; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to "
													 LUA_QL("CONS_Printf"));
		if (i>2) CONS_Printf("\n");
		CONS_Printf("%s", s);
		lua_pop(L, 1);  /* pop result */
	}
	CONS_Printf("\n");
	return 0;
}

static luaL_Reg lib[] = {
	{"COM_AddCommand", lib_comAddCommand},
	{"COM_BufAddText", lib_comBufAddText},
	{"COM_BufInsertText", lib_comBufInsertText},
	{"CV_RegisterVar", lib_cvRegisterVar},
	{"CONS_Printf", lib_consPrintf},
	{NULL, NULL}
};

static int cvar_get(lua_State *L)
{
	consvar_t *cvar = (consvar_t *)luaL_checkudata(L, 1, META_CVAR);
	const char *field = luaL_checkstring(L, 2);

	if(fastcmp(field,"name"))
		lua_pushstring(L, cvar->name);
	else if(fastcmp(field,"defaultvalue"))
		lua_pushstring(L, cvar->defaultvalue);
	else if(fastcmp(field,"flags"))
		lua_pushinteger(L, cvar->flags);
	else if(fastcmp(field,"value"))
		lua_pushinteger(L, cvar->value);
	else if(fastcmp(field,"string"))
		lua_pushstring(L, cvar->string);
	else if(fastcmp(field,"changed"))
		lua_pushboolean(L, cvar->changed);
	else if (devparm)
		return luaL_error(L, LUA_QL("consvar_t") " has no field named " LUA_QS, field);
	else
		return 0;
	return 1;
}

int LUA_ConsoleLib(lua_State *L)
{
	// Metatable for consvar_t
	luaL_newmetatable(L, META_CVAR);
		lua_pushcfunction(L, cvar_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	// Set empty registry tables
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "COM_Command");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_Vars");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_PossibleValue");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_OnChange");

	// Push opaque CV_PossibleValue pointers
	// Because I don't care enough to bother.
	lua_pushlightuserdata(L, CV_OnOff);
	lua_setglobal(L, "CV_OnOff");
	lua_pushlightuserdata(L, CV_YesNo);
	lua_setglobal(L, "CV_YesNo");
	lua_pushlightuserdata(L, CV_Unsigned);
	lua_setglobal(L, "CV_Unsigned");
	lua_pushlightuserdata(L, CV_Natural);
	lua_setglobal(L, "CV_Natural");

	// Set global functions
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}

#endif
