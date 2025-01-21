// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_consolelib.c
/// \brief console modifying/etc library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_local.h"
#include "g_game.h"
#include "byteptr.h"
#include "z_zone.h"
#include "netcode/net_command.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

// for functions not allowed in hud.add hooks
#define NOHUD if (hud_running)\
return luaL_error(L, "HUD rendering code should not call this function!");
// for functions not allowed in hooks or coroutines (supercedes above)
#define NOHOOK if (!lua_lumploading)\
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

static consvar_t *this_cvar;

static void clear_lua_stack(void)
{
	if (gL) // check if Lua is actually turned on first, you dummmy -- Monster Iestyn 04/07/18
		lua_settop(gL, 0); // clear stack
}

void Got_Luacmd(UINT8 **cp, INT32 playernum)
{
	UINT8 i, argc, flags;
	const char *argv[256];
	char buf[256];

	argc = READUINT8(*cp);
	argv[0] = (const char*)*cp;
	SKIPSTRINGN(*cp, 255);
	for (i = 1; i < argc; i++)
	{
		argv[i] = (const char*)*cp;
		SKIPSTRINGN(*cp, 255);
	}

	// don't use I_Assert here, goto the deny code below
	// to clean up and kick people who try nefarious exploits
	// like sending random junk lua commands to crash the server

	if (!gL) goto deny;

	lua_settop(gL, 0); // Just in case...
	lua_pushcfunction(gL, LUA_GetErrorMessage);

	lua_getfield(gL, LUA_REGISTRYINDEX, "COM_Command"); // push COM_Command
	if (!lua_istable(gL, -1)) goto deny;

	strlcpy(buf, argv[0], 255);
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
	if ((flags & 1) && playernum != serverplayer && !IsPlayerAdmin(playernum))
		goto deny;

	lua_rawgeti(gL, -1, 1); // push function from command info table

	// although honestly this should be true anyway
	// BUT GODDAMNIT I SAID NO I_ASSERTS SO NO I_ASSERTS IT IS
	if (!lua_isfunction(gL, -1)) goto deny;

	lua_remove(gL, -2); // pop command info table

	if (!lua_checkstack(gL, argc)) // player + command arguments
	{
		clear_lua_stack();
		CONS_Alert(CONS_WARNING, "lua command stack overflow from %s (%d, need %d more)\n", player_names[playernum], lua_gettop(gL), argc);
		return;
	}

	LUA_PushUserdata(gL, &players[playernum], META_PLAYER);
	for (i = 1; i < argc; i++)
	{
		strlcpy(buf, argv[i], 255);
		lua_pushstring(gL, buf);
	}
	LUA_Call(gL, (int)argc, 0, 1); // argc is 1-based, so this will cover the player we passed too.
	return;

deny:
	//must be hacked/buggy client
	clear_lua_stack();

	CONS_Alert(CONS_WARNING, M_GetText("Illegal lua command received from %s\n"), player_names[playernum]);
	if (server)
		SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
}

// Wrapper for COM_AddCommand commands
void COM_Lua_f(void)
{
	char *buf, *p;
	UINT8 i, flags;
	UINT16 len;
	INT32 playernum = consoleplayer;

	I_Assert(gL != NULL);

	lua_settop(gL, 0); // Just in case...
	lua_pushcfunction(gL, LUA_GetErrorMessage);

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
		flags = (lua_toboolean(gL, -1) ? COM_ADMIN : 0);
	else
		flags = (UINT8)lua_tointeger(gL, -1);
	lua_pop(gL, 1); // pop flags

	if (flags & COM_SPLITSCREEN) // flag 2: splitscreen player command.
	{
		if (!splitscreen)
		{
			lua_pop(gL, 1); // pop command info table
			return; // can't execute splitscreen command without player 2!
		}
		playernum = secondarydisplayplayer;
	}

	if (netgame && !( flags & COM_LOCAL ))/* don't send local commands */
	{ // Send the command through the network
		UINT8 argc;
		lua_pop(gL, 1); // pop command info table

		if (flags & COM_ADMIN && !server && !IsPlayerAdmin(playernum)) // flag 1: only server/admin can use this command.
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
		if (flags & COM_SPLITSCREEN)
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

	if (!lua_checkstack(gL, COM_Argc() + 1))
	{
		CONS_Alert(CONS_WARNING, "lua command stack overflow (%d, need %s more)\n", lua_gettop(gL), sizeu1(COM_Argc() + 1));
		return;
	}
	LUA_PushUserdata(gL, &players[playernum], META_PLAYER);
	for (i = 1; i < COM_Argc(); i++)
		lua_pushstring(gL, COM_Argv(i));
	LUA_Call(gL, (int)COM_Argc(), 0, 1); // COM_Argc is 1-based, so this will cover the player we passed too.
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
	NOHOOK
	if (lua_gettop(L) >= 3)
	{ // For the third argument, only take a boolean or a number.
		lua_settop(L, 3);
		// TODO: 2.3: Remove boolean option
		if (lua_type(L, 3) == LUA_TBOOLEAN)
		{
			CONS_Alert(CONS_WARNING,
					"Using a boolean for admin commands is "
					"deprecated and will be removed.\n"
					"Use \"COM_ADMIN\" instead.\n"
			);
		}
		else
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
	player_t *plr = NULL;
	if (n < 2)
		return luaL_error(L, "COM_BufAddText requires two arguments: player and text.");
	NOHUD
	lua_settop(L, 2);
	if (!lua_isnoneornil(L, 1))
		plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (plr && plr != &players[consoleplayer])
		return 0;
	COM_BufAddTextEx(va("%s\n", luaL_checkstring(L, 2)), COM_LUA);
	return 0;
}

static int lib_comBufInsertText(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	player_t *plr = NULL;
	if (n < 2)
		return luaL_error(L, "COM_BufInsertText requires two arguments: player and text.");
	NOHUD
	lua_settop(L, 2);
	if (!lua_isnoneornil(L, 1))
		plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (plr && plr != &players[consoleplayer])
		return 0;
	COM_BufInsertTextEx(va("%s\n", luaL_checkstring(L, 2)), COM_LUA);
	return 0;
}

void LUA_CVarChanged(void *cvar)
{
	this_cvar = cvar;
}

static void Lua_OnChange(void)
{
	/// \todo Network this! XD_LUAVAR

	lua_pushcfunction(gL, LUA_GetErrorMessage);
	lua_insert(gL, 1); // Because LUA_Call wants it at index 1.

	// From CV_OnChange registry field, get the function for this cvar by name.
	lua_getfield(gL, LUA_REGISTRYINDEX, "CV_OnChange");
	I_Assert(lua_istable(gL, -1));
	lua_pushlightuserdata(gL, this_cvar);
	lua_rawget(gL, -2); // get function

	LUA_RawPushUserdata(gL, this_cvar);

	LUA_Call(gL, 1, 0, 1); // call function(cvar)
	lua_pop(gL, 1); // pop CV_OnChange table
	lua_remove(gL, 1); // remove LUA_GetErrorMessage
}

static boolean Lua_CanChange(const char *valstr)
{
	lua_pushcfunction(gL, LUA_GetErrorMessage);
	lua_insert(gL, 1); // Because LUA_Call wants it at index 1.

	// From CV_CanChange registry field, get the function for this cvar by name.
	lua_getfield(gL, LUA_REGISTRYINDEX, "CV_CanChange");
	I_Assert(lua_istable(gL, -1));
	lua_pushlightuserdata(gL, this_cvar);
	lua_rawget(gL, -2); // get function

	LUA_RawPushUserdata(gL, this_cvar);
	lua_pushstring(gL, valstr);

	boolean result;

	LUA_Call(gL, 2, 1, 1); // call function(cvar, valstr)
	result = lua_toboolean(gL, -1);
	lua_pop(gL, 1); // pop CV_CanChange table
	lua_remove(gL, 1); // remove LUA_GetErrorMessage

	return result;
}

static int lib_cvRegisterVar(lua_State *L)
{
	const char *k;
	lua_Integer i;
	consvar_t *cvar;
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1); // Clear out all other possible arguments, leaving only the first one.
	NOHOOK
	cvar = ZZ_Calloc(sizeof(consvar_t));
	LUA_PushUserdata(L, cvar, META_CVAR);

#define FIELDERROR(f, e) luaL_error(L, "bad value for " LUA_QL(f) " in table passed to " LUA_QL("CV_RegisterVar") " (%s)", e);
#define TYPEERROR(f, t) FIELDERROR(f, va("%s expected, got %s", lua_typename(L, t), luaL_typename(L, -1)))

	lua_pushnil(L);
	while (lua_next(L, 1))
	{
		// stack: cvar table, cvar userdata, key/index, value
		//            1             2            3        4
		i = 0;
		k = NULL;
		if (lua_isnumber(L, 3))
		{
			i = lua_tointeger(L, 3);
		}
		else if (lua_isstring(L, 3))
		{
			k = lua_tostring(L, 3);
		}

		if (i == 1 || (k && fasticmp(k, "name")))
		{
			if (!lua_isstring(L, 4))
			{
				TYPEERROR("name", LUA_TSTRING)
			}
			cvar->name = Z_StrDup(lua_tostring(L, 4));
		}
		else if (i == 2 || (k && fasticmp(k, "defaultvalue")))
		{
			if (!lua_isstring(L, 4))
			{
				TYPEERROR("defaultvalue", LUA_TSTRING)
			}
			cvar->defaultvalue = Z_StrDup(lua_tostring(L, 4));
		}
		else if (i == 3 || (k && fasticmp(k, "flags")))
		{
			if (!lua_isnumber(L, 4))
			{
				TYPEERROR("flags", LUA_TNUMBER)
			}
			cvar->flags = (INT32)lua_tointeger(L, 4);
		}
		else if (i == 4 || (k && fasticmp(k, "PossibleValue")))
		{
			if (lua_islightuserdata(L, 4))
			{
				CV_PossibleValue_t *pv = lua_touserdata(L, 4);
				if (pv == CV_OnOff || pv == CV_YesNo || pv == CV_Unsigned || pv == CV_Natural || pv == CV_TrueFalse)
					cvar->PossibleValue = pv;
				else
					FIELDERROR("PossibleValue", "CV_PossibleValue_t expected, got unrecognised pointer")
			}
			else if (lua_istable(L, 4))
			{
				// Accepts tables in the form of {MIN=0, MAX=9999} or {Red=0, Green=1, Blue=2}
				// and converts them to CV_PossibleValue_t {{0,"MIN"},{9999,"MAX"}} or {{0,"Red"},{1,"Green"},{2,"Blue"}}
				//
				// I don't really like the way this does it because a single PossibleValue table
				// being used for multiple cvars will be converted and stored multiple times.
				// So maybe instead it should be a seperate function which must be run beforehand or something.
				size_t count = 0;
				CV_PossibleValue_t *cvpv;

				const char * const MINMAX[2] = {"MIN", "MAX"};
				int minmax_unset = 3;

				lua_pushnil(L);
				while (lua_next(L, 4))
				{
					count++;
					lua_pop(L, 1);
				}

				lua_getfield(L, LUA_REGISTRYINDEX, "CV_PossibleValue");
				I_Assert(lua_istable(L, 5));
				lua_pushlightuserdata(L, cvar);
				cvpv = lua_newuserdata(L, sizeof(CV_PossibleValue_t) * (count+1));
				lua_rawset(L, 5);
				lua_pop(L, 1); // pop CV_PossibleValue registry table

				i = 0;
				lua_pushnil(L);
				while (lua_next(L, 4))
				{
					INT32 n;
					const char * strval;

					// stack: [...] PossibleValue table, index, value
					//                       4             5      6
					if (lua_type(L, 5) != LUA_TSTRING
					|| lua_type(L, 6) != LUA_TNUMBER)
						FIELDERROR("PossibleValue", "custom PossibleValue table requires a format of string=integer, i.e. {MIN=0, MAX=9999}");

					strval = lua_tostring(L, 5);

					if (
							stricmp(strval, MINMAX[n=0]) == 0 ||
							stricmp(strval, MINMAX[n=1]) == 0
					){
						/* need to shift forward */
						if (minmax_unset == 3)
						{
							memmove(&cvpv[2], &cvpv[0],
									i * sizeof *cvpv);
							i += 2;
						}
						cvpv[n].strvalue = MINMAX[n];
						minmax_unset &= ~(1 << n);
					}
					else
					{
						n = i++;
						cvpv[n].strvalue = Z_StrDup(strval);
					}

					cvpv[n].value = (INT32)lua_tonumber(L, 6);

					lua_pop(L, 1);
				}

				if (minmax_unset && minmax_unset != 3)
					FIELDERROR("PossibleValue", "custom PossibleValue table requires requires both MIN and MAX keys if one is present");

				cvpv[i].value = 0;
				cvpv[i].strvalue = NULL;
				cvar->PossibleValue = cvpv;
			}
			else
			{
				FIELDERROR("PossibleValue", va("%s or CV_PossibleValue_t expected, got %s", lua_typename(L, LUA_TTABLE), luaL_typename(L, -1)))
			}
		}
		else if (cvar->flags & CV_CALL && (i == 5 || (k && fasticmp(k, "func"))))
		{
			if (!lua_isfunction(L, 4))
			{
				TYPEERROR("func", LUA_TFUNCTION)
			}
			lua_getfield(L, LUA_REGISTRYINDEX, "CV_OnChange");
			I_Assert(lua_istable(L, 5));
			lua_pushlightuserdata(L, cvar);
			lua_pushvalue(L, 4);
			lua_rawset(L, 5);
			lua_pop(L, 1);
			cvar->func = Lua_OnChange;
		}
		else if (cvar->flags & CV_CALL && (k && fasticmp(k, "can_change")))
		{
			if (!lua_isfunction(L, 4))
			{
				TYPEERROR("func", LUA_TFUNCTION)
			}
			lua_getfield(L, LUA_REGISTRYINDEX, "CV_CanChange");
			I_Assert(lua_istable(L, 5));
			lua_pushlightuserdata(L, cvar);
			lua_pushvalue(L, 4);
			lua_rawset(L, 5);
			lua_pop(L, 1);
			cvar->can_change = Lua_CanChange;
		}
		lua_pop(L, 1);
	}

#undef FIELDERROR
#undef TYPEERROR

	if (!cvar->name || cvar->name[0] == '\0')
	{
		return luaL_error(L, M_GetText("Variable has no name!"));
	}

	if (!cvar->defaultvalue)
	{
		return luaL_error(L, M_GetText("Variable has no defaultvalue!"));
	}

	if ((cvar->flags & CV_NOINIT) && !(cvar->flags & CV_CALL))
	{
		return luaL_error(L, M_GetText("Variable %s has CV_NOINIT without CV_CALL"), cvar->name);
	}

	if ((cvar->flags & CV_CALL) && !(cvar->func || cvar->can_change))
	{
		return luaL_error(L, M_GetText("Variable %s has CV_CALL without any callbacks"), cvar->name);
	}

	cvar->flags |= CV_ALLOWLUA;
	// actually time to register it to the console now! Finally!
	cvar->flags |= CV_MODIFIED;
	CV_RegisterVar(cvar);

	if (cvar->flags & CV_MODIFIED)
	{
		return luaL_error(L, "failed to register cvar (probable conflict with internal variable/command names)");
	}

	// return cvar userdata
	return 1;
}

static int lib_cvFindVar(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	LUA_PushUserdata(L, CV_FindVar(name), META_CVAR);
	return 1;
}

static int CVarSetFunction
(
		lua_State *L,
		void (*Set)(consvar_t *, const char *),
		void (*SetValue)(consvar_t *, INT32)
){
	consvar_t *cvar = *(consvar_t **)luaL_checkudata(L, 1, META_CVAR);

	if (!(cvar->flags & CV_ALLOWLUA))
		return luaL_error(L, "Variable '%s' cannot be set from Lua.", cvar->name);

	switch (lua_type(L, 2))
	{
		case LUA_TSTRING:
			(*Set)(cvar, lua_tostring(L, 2));
			break;
		case LUA_TNUMBER:
			(*SetValue)(cvar, (INT32)lua_tonumber(L, 2));
			break;
		default:
			return luaL_typerror(L, 1, "string or number");
	}

	return 0;
}

static int lib_cvSet(lua_State *L)
{
	return CVarSetFunction(L, CV_Set, CV_SetValue);
}

static int lib_cvStealthSet(lua_State *L)
{
	return CVarSetFunction(L, CV_StealthSet, CV_StealthSetValue);
}

static int lib_cvAddValue(lua_State *L)
{
	consvar_t *cvar = *(consvar_t **)luaL_checkudata(L, 1, META_CVAR);

	if (!(cvar->flags & CV_ALLOWLUA))
		return luaL_error(L, "Variable %s cannot be set from Lua.", cvar->name);

	CV_AddValue(cvar, (INT32)luaL_checknumber(L, 2));

	return 0;
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
	{"CV_FindVar", lib_cvFindVar},
	{"CV_Set", lib_cvSet},
	{"CV_StealthSet", lib_cvStealthSet},
	{"CV_AddValue", lib_cvAddValue},
	{"CONS_Printf", lib_consPrintf},
	{NULL, NULL}
};

enum cvar_e
{
	cvar_name,
	cvar_defaultvalue,
	cvar_flags,
	cvar_value,
	cvar_string,
	cvar_changed,
};

static const char *const cvar_opt[] = {
	"name",
	"defaultvalue",
	"flags",
	"value",
	"string",
	"changed",
	NULL,
};

static int cvar_fields_ref = LUA_NOREF;

static int cvar_get(lua_State *L)
{
	consvar_t *cvar = *(consvar_t **)luaL_checkudata(L, 1, META_CVAR);
	enum cvar_e field = Lua_optoption(L, 2, -1, cvar_fields_ref);

	switch (field)
	{
	case cvar_name:
		lua_pushstring(L, cvar->name);
		break;
	case cvar_defaultvalue:
		lua_pushstring(L, cvar->defaultvalue);
		break;
	case cvar_flags:
		lua_pushinteger(L, cvar->flags);
		break;
	case cvar_value:
		lua_pushinteger(L, cvar->value);
		break;
	case cvar_string:
		lua_pushstring(L, cvar->string);
		break;
	case cvar_changed:
		lua_pushboolean(L, cvar->changed);
		break;
	default:
		if (devparm)
			return luaL_error(L, LUA_QL("consvar_t") " has no field named " LUA_QS ".", lua_tostring(L, 2));
		else
			return 0;
	}
	return 1;
}

int LUA_ConsoleLib(lua_State *L)
{
	// Metatable for consvar_t
	LUA_RegisterUserdataMetatable(L, META_CVAR, cvar_get, NULL, NULL);

	cvar_fields_ref = Lua_CreateFieldTable(L, cvar_opt);

	// Set empty registry tables
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "COM_Command");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_Vars");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_PossibleValue");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_OnChange");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "CV_CanChange");

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
	lua_pushlightuserdata(L, CV_TrueFalse);
	lua_setglobal(L, "CV_TrueFalse");

	// Set global functions
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}
