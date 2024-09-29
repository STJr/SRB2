// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_script.c
/// \brief Lua scripting basics

#include "doomdef.h"
#include "fastcmp.h"
#include "dehacked.h"
#include "deh_lua.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_setup.h"
#include "r_state.h"
#include "r_sky.h"
#include "g_game.h"
#include "g_input.h"
#include "f_finale.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "p_local.h"
#include "p_slopes.h" // for P_SlopeById and slopelist
#include "p_polyobj.h" // polyobj_t, PolyObjects
#ifdef LUA_ALLOW_BYTECODE
#include "netcode/d_netfil.h" // for LUA_DumpFile
#endif

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hook.h"

#include "doomstat.h"
#include "g_state.h"

#include "hu_stuff.h"

lua_State *gL = NULL;

// List of internal libraries to load from SRB2
static lua_CFunction liblist[] = {
	LUA_EnumLib, // global metatable for enums
	LUA_SOCLib, // A_Action functions, freeslot
	LUA_BaseLib, // string concatination by +, CONS_Printf, p_local.h stuff (P_InstaThrust, P_Move), etc.
	LUA_MathLib, // fixed_t and angle_t math functions
	LUA_HookLib, // hookAdd and hook-calling functions
	LUA_ConsoleLib, // console command/variable functions and structs
	LUA_InfoLib, // info.h stuff: mobjinfo_t, mobjinfo[], state_t, states[]
	LUA_MobjLib, // mobj_t, mapthing_t
	LUA_PlayerLib, // player_t
	LUA_SkinLib, // skin_t, skins[]
	LUA_ThinkerLib, // thinker_t
	LUA_MapLib, // line_t, side_t, sector_t, subsector_t
	LUA_TagLib, // tags
	LUA_PolyObjLib, // polyobj_t
	LUA_BlockmapLib, // blockmap stuff
	LUA_HudLib, // HUD stuff
	LUA_ColorLib, // general color functions
	LUA_InputLib, // inputs
	NULL
};

// Lua asks for memory using this.
static void *LUA_Alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	(void)ud;
	if (nsize == 0) {
		if (osize != 0)
			Z_Free(ptr);
		return NULL;
	} else
		return Z_Realloc(ptr, nsize, PU_LUA, NULL);
}

// Panic function Lua calls when there's an unprotected error.
// This function cannot return. Lua would kill the application anyway if it did.
FUNCNORETURN static int LUA_Panic(lua_State *L)
{
	CONS_Alert(CONS_ERROR,"LUA PANIC! %s\n",lua_tostring(L,-1));
	I_Error("An unfortunate Lua processing error occurred in the exe itself. This is not a scripting error on your part.");
#ifndef __GNUC__
	return -1;
#endif
}

#define LEVELS1 12 // size of the first part of the stack
#define LEVELS2 10 // size of the second part of the stack

// Error handler used with pcall() when loading scripts or calling hooks
// Takes a string with the original error message,
// appends the traceback to it, and return the result
int LUA_GetErrorMessage(lua_State *L)
{
	int level = 1;
	int firstpart = 1; // still before eventual `...'
	lua_Debug ar;

	lua_pushliteral(L, "\nstack traceback:");
	while (lua_getstack(L, level++, &ar))
	{
		if (level > LEVELS1 && firstpart)
		{
			// no more than `LEVELS2' more levels?
			if (!lua_getstack(L, level + LEVELS2, &ar))
				level--; // keep going
			else
			{
				lua_pushliteral(L, "\n    ..."); // too many levels
				while (lua_getstack(L, level + LEVELS2, &ar)) // find last levels
					level++;
			}
			firstpart = 0;
			continue;
		}
		lua_pushliteral(L, "\n    ");
		lua_getinfo(L, "Snl", &ar);
		lua_pushfstring(L, "%s:", ar.short_src);
		if (ar.currentline > 0)
			lua_pushfstring(L, "%d:", ar.currentline);
		if (*ar.namewhat != '\0') // is there a name?
			lua_pushfstring(L, " in function " LUA_QS, ar.name);
		else
		{
			if (*ar.what == 'm') // main?
				lua_pushfstring(L, " in main chunk");
			else if (*ar.what == 'C' || *ar.what == 't')
				lua_pushliteral(L, " ?"); // C function or tail call
			else
				lua_pushfstring(L, " in function <%s:%d>",
					ar.short_src, ar.linedefined);
		}
		lua_concat(L, lua_gettop(L));
	}
	lua_concat(L, lua_gettop(L));
	return 1;
}

int LUA_Call(lua_State *L, int nargs, int nresults, int errorhandlerindex)
{
	int err = lua_pcall(L, nargs, nresults, errorhandlerindex);

	if (err)
	{
		CONS_Alert(CONS_WARNING, "%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	return err;
}

// Moved here from lib_getenum.
int LUA_PushGlobals(lua_State *L, const char *word)
{
	if (fastcmp(word,"gamemap")) {
		lua_pushinteger(L, gamemap);
		return 1;
	} else if (fastcmp(word,"udmf")) {
		lua_pushboolean(L, udmf);
		return 1;
	} else if (fastcmp(word,"maptol")) {
		lua_pushinteger(L, maptol);
		return 1;
	} else if (fastcmp(word,"ultimatemode")) {
		lua_pushboolean(L, ultimatemode != 0);
		return 1;
	} else if (fastcmp(word,"mariomode")) {
		lua_pushboolean(L, mariomode != 0);
		return 1;
	} else if (fastcmp(word,"twodlevel")) {
		lua_pushboolean(L, twodlevel != 0);
		return 1;
	} else if (fastcmp(word,"circuitmap")) {
		lua_pushboolean(L, circuitmap);
		return 1;
	} else if (fastcmp(word,"stoppedclock")) {
		lua_pushboolean(L, stoppedclock);
		return 1;
	} else if (fastcmp(word,"netgame")) {
		lua_pushboolean(L, netgame);
		return 1;
	} else if (fastcmp(word,"multiplayer")) {
		lua_pushboolean(L, multiplayer);
		return 1;
	} else if (fastcmp(word,"modeattacking")) {
		lua_pushboolean(L, modeattacking);
		return 1;
	} else if (fastcmp(word,"metalrecording")) {
		lua_pushboolean(L, metalrecording);
		return 1;
	} else if (fastcmp(word,"splitscreen")) {
		lua_pushboolean(L, splitscreen);
		return 1;
	} else if (fastcmp(word,"gamecomplete")) {
		lua_pushboolean(L, (gamecomplete != 0));
		return 1;
	} else if (fastcmp(word,"marathonmode")) {
		lua_pushinteger(L, marathonmode);
		return 1;
	} else if (fastcmp(word,"devparm")) {
		lua_pushboolean(L, devparm);
		return 1;
	} else if (fastcmp(word,"modifiedgame")) {
		lua_pushboolean(L, modifiedgame && !savemoddata);
		return 1;
	} else if (fastcmp(word,"usedCheats")) {
		lua_pushboolean(L, usedCheats);
		return 1;
	} else if (fastcmp(word,"menuactive")) {
		lua_pushboolean(L, menuactive);
		return 1;
	} else if (fastcmp(word,"paused")) {
		lua_pushboolean(L, paused);
		return 1;
	} else if (fastcmp(word,"bluescore")) {
		lua_pushinteger(L, bluescore);
		return 1;
	} else if (fastcmp(word,"redscore")) {
		lua_pushinteger(L, redscore);
		return 1;
	} else if (fastcmp(word,"timelimit")) {
		lua_pushinteger(L, cv_timelimit.value);
		return 1;
	} else if (fastcmp(word,"pointlimit")) {
		lua_pushinteger(L, cv_pointlimit.value);
		return 1;
	} else if (fastcmp(word, "redflag")) {
		LUA_PushUserdata(L, redflag, META_MOBJ);
		return 1;
	} else if (fastcmp(word, "blueflag")) {
		LUA_PushUserdata(L, blueflag, META_MOBJ);
		return 1;
	} else if (fastcmp(word, "rflagpoint")) {
		LUA_PushUserdata(L, rflagpoint, META_MAPTHING);
		return 1;
	} else if (fastcmp(word, "bflagpoint")) {
		LUA_PushUserdata(L, bflagpoint, META_MAPTHING);
		return 1;
	// begin map vars
	} else if (fastcmp(word,"spstage_start")) {
		lua_pushinteger(L, spstage_start);
		return 1;
	} else if (fastcmp(word,"spmarathon_start")) {
		lua_pushinteger(L, spmarathon_start);
		return 1;
	} else if (fastcmp(word,"sstage_start")) {
		lua_pushinteger(L, sstage_start);
		return 1;
	} else if (fastcmp(word,"sstage_end")) {
		lua_pushinteger(L, sstage_end);
		return 1;
	} else if (fastcmp(word,"smpstage_start")) {
		lua_pushinteger(L, smpstage_start);
		return 1;
	} else if (fastcmp(word,"smpstage_end")) {
		lua_pushinteger(L, smpstage_end);
		return 1;
	} else if (fastcmp(word,"titlemap")) {
		lua_pushinteger(L, titlemap);
		return 1;
	} else if (fastcmp(word,"titlemapinaction")) {
		lua_pushboolean(L, (titlemapinaction != TITLEMAP_OFF));
		return 1;
	} else if (fastcmp(word,"bootmap")) {
		lua_pushinteger(L, bootmap);
		return 1;
	} else if (fastcmp(word,"tutorialmap")) {
		lua_pushinteger(L, tutorialmap);
		return 1;
	} else if (fastcmp(word,"tutorialmode")) {
		lua_pushboolean(L, tutorialmode);
		return 1;
	// end map vars
	// begin CTF colors
	} else if (fastcmp(word,"skincolor_redteam")) {
		lua_pushinteger(L, skincolor_redteam);
		return 1;
	} else if (fastcmp(word,"skincolor_blueteam")) {
		lua_pushinteger(L, skincolor_blueteam);
		return 1;
	} else if (fastcmp(word,"skincolor_redring")) {
		lua_pushinteger(L, skincolor_redring);
		return 1;
	} else if (fastcmp(word,"skincolor_bluering")) {
		lua_pushinteger(L, skincolor_bluering);
		return 1;
	// end CTF colors
	// begin timers
	} else if (fastcmp(word,"invulntics")) {
		lua_pushinteger(L, invulntics);
		return 1;
	} else if (fastcmp(word,"sneakertics")) {
		lua_pushinteger(L, sneakertics);
		return 1;
	} else if (fastcmp(word,"flashingtics")) {
		lua_pushinteger(L, flashingtics);
		return 1;
	} else if (fastcmp(word,"tailsflytics")) {
		lua_pushinteger(L, tailsflytics);
		return 1;
	} else if (fastcmp(word,"underwatertics")) {
		lua_pushinteger(L, underwatertics);
		return 1;
	} else if (fastcmp(word,"spacetimetics")) {
		lua_pushinteger(L, spacetimetics);
		return 1;
	} else if (fastcmp(word,"extralifetics")) {
		lua_pushinteger(L, extralifetics);
		return 1;
	} else if (fastcmp(word,"nightslinktics")) {
		lua_pushinteger(L, nightslinktics);
		return 1;
	} else if (fastcmp(word,"gameovertics")) {
		lua_pushinteger(L, gameovertics);
		return 1;
	} else if (fastcmp(word,"ammoremovaltics")) {
		lua_pushinteger(L, ammoremovaltics);
		return 1;
	// end timers
	} else if (fastcmp(word,"use1upSound")) {
		lua_pushinteger(L, use1upSound);
		return 1;
	} else if (fastcmp(word,"maxXtraLife")) {
		lua_pushinteger(L, maxXtraLife);
		return 1;
	} else if (fastcmp(word,"useContinues")) {
		lua_pushinteger(L, useContinues);
		return 1;
	} else if (fastcmp(word,"shareEmblems")) {
		lua_pushinteger(L, shareEmblems);
		return 1;
	} else if (fastcmp(word,"gametype")) {
		lua_pushinteger(L, gametype);
		return 1;
	} else if (fastcmp(word,"gametyperules")) {
		lua_pushinteger(L, gametyperules);
		return 1;
	} else if (fastcmp(word,"leveltime")) {
		lua_pushinteger(L, leveltime);
		return 1;
	} else if (fastcmp(word,"sstimer")) {
		lua_pushinteger(L, sstimer);
		return 1;
	} else if (fastcmp(word,"curWeather")) {
		lua_pushinteger(L, curWeather);
		return 1;
	} else if (fastcmp(word,"globalweather")) {
		lua_pushinteger(L, globalweather);
		return 1;
	} else if (fastcmp(word,"levelskynum")) {
		lua_pushinteger(L, levelskynum);
		return 1;
	} else if (fastcmp(word,"globallevelskynum")) {
		lua_pushinteger(L, globallevelskynum);
		return 1;
	} else if (fastcmp(word,"mapmusname")) {
		lua_pushstring(L, mapmusname);
		return 1;
	} else if (fastcmp(word,"mapmusflags")) {
		lua_pushinteger(L, mapmusflags);
		return 1;
	} else if (fastcmp(word,"mapmusposition")) {
		lua_pushinteger(L, mapmusposition);
		return 1;
	// local player variables, by popular request
	} else if (fastcmp(word,"consoleplayer")) { // player controlling console (aka local player 1)
		if (!addedtogame || consoleplayer < 0 || !playeringame[consoleplayer])
			return 0;
		LUA_PushUserdata(L, &players[consoleplayer], META_PLAYER);
		return 1;
	} else if (fastcmp(word,"displayplayer")) { // player visible on screen (aka display player 1)
		if (displayplayer < 0 || !playeringame[displayplayer])
			return 0;
		LUA_PushUserdata(L, &players[displayplayer], META_PLAYER);
		return 1;
	} else if (fastcmp(word,"secondarydisplayplayer")) { // local/display player 2, for splitscreen
		if (!splitscreen || secondarydisplayplayer < 0 || !playeringame[secondarydisplayplayer])
			return 0;
		LUA_PushUserdata(L, &players[secondarydisplayplayer], META_PLAYER);
		return 1;
	} else if (fastcmp(word,"isserver")) {
		lua_pushboolean(L, server);
		return 1;
	} else if (fastcmp(word,"isdedicatedserver")) {
		lua_pushboolean(L, dedicated);
		return 1;
	// end local player variables
	} else if (fastcmp(word,"server")) {
		if ((!multiplayer || !netgame) && !playeringame[serverplayer])
			return 0;
		LUA_PushUserdata(L, &players[serverplayer], META_PLAYER);
		return 1;
	} else if (fastcmp(word,"emeralds")) {
		lua_pushinteger(L, emeralds);
		return 1;
	} else if (fastcmp(word,"gravity")) {
		lua_pushinteger(L, gravity);
		return 1;
	} else if (fastcmp(word,"VERSION")) {
		lua_pushinteger(L, VERSION);
		return 1;
	} else if (fastcmp(word,"SUBVERSION")) {
		lua_pushinteger(L, SUBVERSION);
		return 1;
	} else if (fastcmp(word,"VERSIONSTRING")) {
		lua_pushstring(L, VERSIONSTRING);
		return 1;
	} else if (fastcmp(word, "token")) {
		lua_pushinteger(L, token);
		return 1;
	} else if (fastcmp(word, "gamestate")) {
		lua_pushinteger(L, gamestate);
		return 1;
	} else if (fastcmp(word, "stagefailed")) {
		lua_pushboolean(L, stagefailed);
		return 1;
	// TODO: 2.3: Deprecated (moved to the input library)
	} else if (fastcmp(word, "mouse")) {
		LUA_PushUserdata(L, &mouse, META_MOUSE);
		return 1;
	// TODO: 2.3: Deprecated (moved to the input library)
	} else if (fastcmp(word, "mouse2")) {
		LUA_PushUserdata(L, &mouse2, META_MOUSE);
		return 1;
	} else if (fastcmp(word, "camera")) {
		LUA_PushUserdata(L, &camera, META_CAMERA);
		return 1;
	} else if (fastcmp(word, "camera2")) {
		if (!splitscreen)
			return 0;
		LUA_PushUserdata(L, &camera2, META_CAMERA);
		return 1;
	} else if (fastcmp(word, "chatactive")) {
		lua_pushboolean(L, chat_on);
		return 1;
	}
	return 0;
}

// See the above.
int LUA_CheckGlobals(lua_State *L, const char *word)
{
	if (fastcmp(word, "redscore"))
		redscore = (UINT32)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "bluescore"))
		bluescore = (UINT32)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "skincolor_redteam"))
		skincolor_redteam = (UINT16)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "skincolor_blueteam"))
		skincolor_blueteam = (UINT16)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "skincolor_redring"))
		skincolor_redring = (UINT16)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "skincolor_bluering"))
		skincolor_bluering = (UINT16)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "emeralds"))
		emeralds = (UINT16)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "token"))
		token = (UINT32)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "gravity"))
		gravity = (fixed_t)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "stoppedclock"))
		stoppedclock = luaL_checkboolean(L, 2);
	else if (fastcmp(word, "displayplayer"))
	{
		player_t *player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));

		if (player)
			displayplayer = player - players;
	}
	else if (fastcmp(word, "mapmusname"))
	{
		size_t strlength;
		const char *str = luaL_checklstring(L, 2, &strlength);

		if (strlength > 6)
			return luaL_error(L, "string length out of range (maximum 6 characters)");

		if (strlen(str) < strlength)
			return luaL_error(L, "string must not contain embedded zeros!");

		strlcpy(mapmusname, str, sizeof mapmusname);
	}
	else if (fastcmp(word, "mapmusflags"))
		mapmusflags = (UINT16)luaL_checkinteger(L, 2);
	else if (fastcmp(word, "stagefailed"))
		stagefailed = luaL_checkboolean(L, 2);
	else
		return 0;

	// Global variable set, so return and don't error.
	return 1;
}

// This function decides which global variables you are allowed to set.
static int setglobals(lua_State *L)
{
	const char *csname;
	char *name;
	enum actionnum actionnum;

	lua_remove(L, 1); // we're not gonna be using _G
	csname = lua_tostring(L, 1);

	// make an uppercase copy of the name
	name = Z_StrDup(csname);
	strupr(name);

	if (fastncmp(name, "A_", 2) && lua_isfunction(L, 2))
	{
		// Accept new A_Action functions
		// Add the action to Lua actions refrence table
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_ACTIONS);
		lua_pushstring(L, name); // "A_ACTION"
		lua_pushvalue(L, 2); // function
		lua_rawset(L, -3); // rawset doesn't trigger this metatable again.
		// otherwise we would've used setfield, obviously.

		actionnum = LUA_GetActionNumByName(name);
		if (actionnum < NUMACTIONS)
		{
			int i;

			for (i = MAX_ACTION_RECURSION-1; i > 0; i--)
			{
				// Move other references deeper.
				actionsoverridden[actionnum][i] = actionsoverridden[actionnum][i - 1];
			}

			// Add the new reference.
			lua_pushvalue(L, 2);
			actionsoverridden[actionnum][0] = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		Z_Free(name);
		return 0;
	}

	if (LUA_CheckGlobals(L, csname))
		return 0;

	Z_Free(name);
	return luaL_error(L, "Implicit global " LUA_QS " prevented. Create a local variable instead.", csname);
}

// Clear and create a new Lua state, laddo!
// There's SCRIPTIN to be had!
static void LUA_ClearState(void)
{
	lua_State *L;
	int i;

	// close previous state
	if (gL)
		lua_close(gL);
	gL = NULL;

	CONS_Printf(M_GetText("Pardon me while I initialize the Lua scripting interface...\n"));

	// allocate state
	L = lua_newstate(LUA_Alloc, NULL);
	lua_atpanic(L, LUA_Panic);

	// open base libraries
	luaL_openlibs(L);
	lua_settop(L, 0);

	// make LREG_VALID table for all pushed userdata cache.
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_VALID);

	// make LREG_METATABLES table for all registered metatables
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_METATABLES);

	// open srb2 libraries
	for(i = 0; liblist[i]; i++) {
		lua_pushcfunction(L, liblist[i]);
		lua_call(L, 0, 0);
	}

	// lock the global namespace
	lua_getmetatable(L, LUA_GLOBALSINDEX);
		LUA_SetCFunctionField(L, "__newindex", setglobals);
		lua_newtable(L);
		lua_setfield(L, -2, "__metatable");
	lua_pop(L, 1);

	// lua state is ready!
	gL = L;
}

#ifdef _DEBUG
void LUA_ClearExtVars(void)
{
	if (!gL)
		return;
	lua_newtable(gL);
	lua_setfield(gL, LUA_REGISTRYINDEX, LREG_EXTVARS);
}
#endif

// Use this variable to prevent certain functions from running
// if they were not called on lump load
// (i.e. they were called in hooks or coroutines etc)
INT32 lua_lumploading = 0;

// Load a script from a MYFILE
static inline boolean LUA_LoadFile(MYFILE *f, char *name)
{
	int errorhandlerindex;
	boolean success;

	if (!name)
		name = wadfiles[f->wad]->filename;

	CONS_Printf("Loading Lua script from %s\n", name);

	if (!gL) // Lua needs to be initialized
		LUA_ClearState();

	lua_pushcfunction(gL, LUA_GetErrorMessage);
	errorhandlerindex = lua_gettop(gL);

	success = !luaL_loadbuffer(gL, f->data, f->size, va("@%s",name));

	if (!success) {
		CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
		lua_pop(gL,1);
	}

	lua_gc(gL, LUA_GCCOLLECT, 0);
	lua_remove(gL, errorhandlerindex);

	return success;
}

// Runs a script loaded by LUA_LoadFile.
static inline void LUA_DoFile(boolean noresults)
{
	int errorhandlerindex;

	if (!gL) // LUA_LoadFile should've allocated gL for us!
		return;

	lua_lumploading++; // turn on loading flag

	lua_pushcfunction(gL, LUA_GetErrorMessage);
	lua_insert(gL, -2); // move the function we're calling to the top.
	errorhandlerindex = lua_gettop(gL) - 1;

	if (lua_pcall(gL, 0, noresults ? 0 : LUA_MULTRET, lua_gettop(gL) - 1)) {
		CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
		lua_pop(gL,1);
	}

	lua_gc(gL, LUA_GCCOLLECT, 0);
	lua_remove(gL, errorhandlerindex);

	lua_lumploading--; // turn off again
}

static inline MYFILE *LUA_GetFile(UINT16 wad, UINT16 lump, char **name)
{
	MYFILE *f = Z_Malloc(sizeof(MYFILE), PU_LUA, NULL);
	size_t len;

	f->wad = wad;
	f->size = W_LumpLengthPwad(wad, lump);
	f->data = Z_Malloc(f->size, PU_LUA, NULL);
	W_ReadLumpPwad(wad, lump, f->data);
	f->curpos = f->data;

	len = strlen(wadfiles[wad]->filename); // length of file name

	if (wadfiles[wad]->type == RET_LUA)
	{
		*name = malloc(len+1);
		strcpy(*name, wadfiles[wad]->filename);
	}
	else // If it's not a .lua file, copy the lump name in too.
	{
		lumpinfo_t *lump_p = &wadfiles[wad]->lumpinfo[lump];
		len += 1 + strlen(lump_p->fullname); // length of file name, '|', and lump name
		*name = malloc(len+1);
		sprintf(*name, "%s|%s", wadfiles[wad]->filename, lump_p->fullname);
		(*name)[len] = '\0'; // annoying that index takes priority over dereference, but w/e
	}

	return f;
}

// Load a script from a lump
boolean LUA_LoadLump(UINT16 wad, UINT16 lump)
{
	char *name = NULL;
	MYFILE *f = LUA_GetFile(wad, lump, &name);
	boolean success = LUA_LoadFile(f, name); // actually load file!

	free(name);

	Z_Free(f->data);
	Z_Free(f);

	return success;
}

void LUA_DoLump(UINT16 wad, UINT16 lump, boolean noresults)
{
	boolean success = LUA_LoadLump(wad, lump);

	if (success)
		LUA_DoFile(noresults); // run it
}

#ifdef LUA_ALLOW_BYTECODE
// must match lua_Writer
static int dumpWriter(lua_State *L, const void *p, size_t sz, void *ud)
{
	FILE *handle = (FILE*)ud;
	I_Assert(handle != NULL);
	(void)L;
	if (!sz) return 0; // nothing to write? can't fail that! :D
	return (fwrite(p, 1, sz, handle) != sz); // if fwrite != sz, we've failed.
}

// Compile a script by name and dump it back to disk.
void LUA_DumpFile(const char *filename)
{
	FILE *handle;
	char filenamebuf[MAX_WADPATH];

	if (!gL) // Lua needs to be initialized
		LUA_ClearState(false);

	// find the file the SRB2 way
	strncpy(filenamebuf, filename, MAX_WADPATH);
	filenamebuf[MAX_WADPATH - 1] = '\0';
	filename = filenamebuf;
	if ((handle = fopen(filename, "rb")) == NULL)
	{
		// If we failed to load the file with the path as specified by
		// the user, strip the directories and search for the file.
		nameonly(filenamebuf);

		// If findfile finds the file, the full path will be returned
		// in filenamebuf == filename.
		if (findfile(filenamebuf, NULL, true))
		{
			if ((handle = fopen(filename, "rb")) == NULL)
			{
				CONS_Alert(CONS_ERROR, M_GetText("Can't open %s\n"), filename);
				return;
			}
		}
		else
		{
			CONS_Alert(CONS_ERROR, M_GetText("File %s not found.\n"), filename);
			return;
		}
	}
	fclose(handle);

	// pass the path we found to Lua
	// luaL_loadfile will open and read the file in as a Lua function
	if (luaL_loadfile(gL, filename)) {
		CONS_Alert(CONS_ERROR,"%s\n",lua_tostring(gL,-1));
		lua_pop(gL, 1);
		return;
	}

	// dump it back to disk
	if ((handle = fopen(filename, "wb")) == NULL)
		CONS_Alert(CONS_ERROR, M_GetText("Can't write to %s\n"), filename);
	if (lua_dump(gL, dumpWriter, handle))
		CONS_Printf("Failed while writing %s to disk... Sorry!\n", filename);
	else
		CONS_Printf("Successfully compiled %s into bytecode.\n", filename);
	fclose(handle);
	lua_pop(gL, 1); // function is still on stack after lua_dump
	lua_gc(gL, LUA_GCCOLLECT, 0);
	return;
}
#endif

fixed_t LUA_EvalMath(const char *word)
{
	static lua_State *L = NULL;
	char buf[1024], *b;
	const char *p;
	fixed_t res = 0;

	if (!L)
	{
		// make a new state so SOC can't interefere with scripts
		// allocate state
		L = lua_newstate(LUA_Alloc, NULL);
		lua_atpanic(L, LUA_Panic);

		// open only enum lib
		lua_pushcfunction(L, LUA_EnumLib);
		lua_pushboolean(L, true);
		lua_call(L, 1, 0);
	}

	// change ^ into ^^ for Lua.
	strcpy(buf, "return ");
	b = buf+strlen(buf);
	for (p = word; *p && b < &buf[1022]; p++)
	{
		*b++ = *p;
		if (*p == '^')
			*b++ = '^';
	}
	*b = '\0';

	// eval string.
	lua_settop(L, 0);
	if (luaL_dostring(L, buf))
	{
		p = lua_tostring(L, -1);
		while (*p++ != ':' && *p) ;
		p += 3; // "1: "
		CONS_Alert(CONS_WARNING, "%s\n", p);
	}
	else
		res = lua_tointeger(L, -1);

	return res;
}

// Takes a pointer, any pointer, and a metatable name
// Creates a userdata for that pointer with the given metatable
// Pushes it to the stack and stores it in the registry.
void LUA_PushUserdata(lua_State *L, void *data, const char *meta)
{
	if (LUA_RawPushUserdata(L, data) == LPUSHED_NEW)
	{
		luaL_getmetatable(L, meta);
		lua_setmetatable(L, -2);
	}
}

// Same as LUA_PushUserdata but don't set a metatable yet.
lpushed_t LUA_RawPushUserdata(lua_State *L, void *data)
{
	lpushed_t status = LPUSHED_NIL;

	void **userdata;

	if (!data) { // push a NULL
		lua_pushnil(L);
		return status;
	}

	lua_getfield(L, LUA_REGISTRYINDEX, LREG_VALID);
	I_Assert(lua_istable(L, -1));

	lua_pushlightuserdata(L, data);
	lua_rawget(L, -2);

	if (lua_isnil(L, -1)) { // no userdata? deary me, we'll have to make one.
		lua_pop(L, 1); // pop the nil

		// create the userdata
		userdata = lua_newuserdata(L, sizeof(void *));
		*userdata = data;

		// Set it in the registry so we can find it again
		lua_pushlightuserdata(L, data); // k (store the userdata via the data's pointer)
		lua_pushvalue(L, -2); // v (copy of the userdata)
		lua_rawset(L, -4);

		// stack is left with the userdata on top, as if getting it had originally succeeded.

		status = LPUSHED_NEW;
	}
	else
		status = LPUSHED_EXISTING;

	lua_remove(L, -2); // remove LREG_VALID

	return status;
}

// When userdata is freed, use this function to remove it from Lua.
void LUA_InvalidateUserdata(void *data)
{
	void **userdata;
	if (!gL)
		return;

	// fetch the userdata
	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_VALID);
	I_Assert(lua_istable(gL, -1));
		lua_pushlightuserdata(gL, data);
		lua_rawget(gL, -2);
			if (lua_isnil(gL, -1)) { // not found, not in lua
				lua_pop(gL, 2); // pop nil and LREG_VALID
				return;
			}

			// nullify any additional data
			lua_getfield(gL, LUA_REGISTRYINDEX, LREG_EXTVARS);
			I_Assert(lua_istable(gL, -1));
				lua_pushlightuserdata(gL, data);
				lua_pushnil(gL);
				lua_rawset(gL, -3);
			lua_pop(gL, 1);

			// invalidate the userdata
			userdata = lua_touserdata(gL, -1);
			*userdata = NULL;
		lua_pop(gL, 1);

		// remove it from the registry
		lua_pushlightuserdata(gL, data);
		lua_pushnil(gL);
		lua_rawset(gL, -3);
	lua_pop(gL, 1); // pop LREG_VALID
}

// Invalidate level data arrays
void LUA_InvalidateLevel(void)
{
	thinker_t *th;
	size_t i;
	ffloor_t *rover = NULL;
	if (!gL)
		return;
	for (i = 0; i < NUM_THINKERLISTS; i++)
		for (th = thlist[i].next; th && th != &thlist[i]; th = th->next)
			LUA_InvalidateUserdata(th);

	LUA_InvalidateMapthings();

	for (i = 0; i < numsubsectors; i++)
		LUA_InvalidateUserdata(&subsectors[i]);
	for (i = 0; i < numsectors; i++)
	{
		LUA_InvalidateUserdata(&sectors[i]);
		LUA_InvalidateUserdata(&sectors[i].lines);
		LUA_InvalidateUserdata(&sectors[i].tags);
		if (sectors[i].extra_colormap)
			LUA_InvalidateUserdata(sectors[i].extra_colormap);
		if (sectors[i].ffloors)
		{
			for (rover = sectors[i].ffloors; rover; rover = rover->next)
				LUA_InvalidateUserdata(rover);
		}
	}
	for (i = 0; i < numlines; i++)
	{
		LUA_InvalidateUserdata(&lines[i]);
		LUA_InvalidateUserdata(&lines[i].tags);
		LUA_InvalidateUserdata(lines[i].args);
		LUA_InvalidateUserdata(lines[i].stringargs);
		LUA_InvalidateUserdata(lines[i].sidenum);
	}
	for (i = 0; i < numsides; i++)
		LUA_InvalidateUserdata(&sides[i]);
	for (i = 0; i < numvertexes; i++)
		LUA_InvalidateUserdata(&vertexes[i]);
	for (i = 0; i < (size_t)numPolyObjects; i++)
	{
		LUA_InvalidateUserdata(&PolyObjects[i]);
		LUA_InvalidateUserdata(&PolyObjects[i].vertices);
		LUA_InvalidateUserdata(&PolyObjects[i].lines);
	}
	for (pslope_t *slope = slopelist; slope; slope = slope->next)
	{
		LUA_InvalidateUserdata(slope);
		LUA_InvalidateUserdata(&slope->normal);
		LUA_InvalidateUserdata(&slope->o);
		LUA_InvalidateUserdata(&slope->d);
	}
#ifdef HAVE_LUA_SEGS
	for (i = 0; i < numsegs; i++)
		LUA_InvalidateUserdata(&segs[i]);
	for (i = 0; i < numnodes; i++)
	{
		LUA_InvalidateUserdata(&nodes[i]);
		LUA_InvalidateUserdata(nodes[i].bbox);
		LUA_InvalidateUserdata(nodes[i].children);
	}
#endif
}

void LUA_InvalidateMapthings(void)
{
	size_t i;
	if (!gL)
		return;

	for (i = 0; i < nummapthings; i++)
	{
		LUA_InvalidateUserdata(&mapthings[i]);
		LUA_InvalidateUserdata(&mapthings[i].tags);
		LUA_InvalidateUserdata(mapthings[i].args);
		LUA_InvalidateUserdata(mapthings[i].stringargs);
	}
}

void LUA_InvalidatePlayer(player_t *player)
{
	if (!gL)
		return;
	LUA_InvalidateUserdata(player);
	LUA_InvalidateUserdata(player->powers);
	LUA_InvalidateUserdata(&player->cmd);
}

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
#ifdef HAVE_LUA_SEGS
	ARCH_SEG,
	ARCH_NODE,
#endif
	ARCH_FFLOOR,
	ARCH_POLYOBJ,
	ARCH_SLOPE,
	ARCH_MAPHEADER,
	ARCH_SKINCOLOR,
	ARCH_MOUSE,
	ARCH_SKIN,

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
#ifdef HAVE_LUA_SEGS
	{META_SEG,      ARCH_SEG},
	{META_NODE,     ARCH_NODE},
#endif
	{META_FFLOOR,	ARCH_FFLOOR},
	{META_POLYOBJ,  ARCH_POLYOBJ},
	{META_SLOPE,    ARCH_SLOPE},
	{META_MAPHEADER,   ARCH_MAPHEADER},
	{META_SKINCOLOR,   ARCH_SKINCOLOR},
	{META_MOUSE,    ARCH_MOUSE},
	{META_SKIN,     ARCH_SKIN},
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

static UINT8 ArchiveValue(save_t *save_p, int TABLESINDEX, int myindex)
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
#ifdef HAVE_LUA_SEGS
		case ARCH_SEG:
		{
			seg_t *seg = *((seg_t **)lua_touserdata(gL, myindex));
			if (!seg)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_SEG);
				P_WriteUINT16(save_p, seg - segs);
			}
			break;
		}
		case ARCH_NODE:
		{
			node_t *node = *((node_t **)lua_touserdata(gL, myindex));
			if (!node)
				P_WriteUINT8(save_p, ARCH_NULL);
			else {
				P_WriteUINT8(save_p, ARCH_NODE);
				P_WriteUINT16(save_p, node - nodes);
			}
			break;
		}
#endif
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
	UINT16 i;

	if (!gL) {
		if (fastcmp(ptype,"player")) // players must always be included, even if no vars
			P_WriteUINT16(save_p, 0);
		return;
	}

	TABLESINDEX = lua_gettop(gL);

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
		if (ArchiveValue(save_p, TABLESINDEX, -1) == 2)
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
	int i, n = lua_gettop(L);
	for (i = 1; i <= n; i++)
		ArchiveValue(lua_save_p, TABLESINDEX, i);
	return n;
}

static void ArchiveTables(save_t *save_p)
{
	int TABLESINDEX;
	UINT16 i, n;
	UINT8 e;

	if (!gL)
		return;

	TABLESINDEX = lua_gettop(gL);

	n = (UINT16)lua_objlen(gL, TABLESINDEX);
	for (i = 1; i <= n; i++)
	{
		lua_rawgeti(gL, TABLESINDEX, i);
		lua_pushnil(gL);
		while (lua_next(gL, -2))
		{
			// Write key
			e = ArchiveValue(save_p, TABLESINDEX, -2); // key should be either a number or a string, ArchiveValue can handle this.
			if (e == 1)
				n++; // the table contained a new table we'll have to archive. :(
			else if (e == 2) // invalid key type (function, thread, lightuserdata, or anything we don't recognise)
				CONS_Alert(CONS_ERROR, "Index '%s' (%s) of table %d could not be archived!\n", lua_tostring(gL, -2), luaL_typename(gL, -2), i);

			// Write value
			e = ArchiveValue(save_p, TABLESINDEX, -1);
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

static UINT8 UnArchiveValue(save_t *save_p, int TABLESINDEX)
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
#ifdef HAVE_LUA_SEGS
	case ARCH_SEG:
		LUA_PushUserdata(gL, &segs[P_ReadUINT16(save_p)], META_SEG);
		break;
	case ARCH_NODE:
		LUA_PushUserdata(gL, &nodes[P_ReadUINT16(save_p)], META_NODE);
		break;
#endif
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
	case ARCH_TEND:
		return 1;
	}
	return 0;
}

static void UnArchiveExtVars(save_t *save_p, void *pointer)
{
	int TABLESINDEX;
	UINT16 field_count = P_ReadUINT16(save_p);
	UINT16 i;
	char field[1024];

	if (field_count == 0)
		return;
	I_Assert(gL != NULL);

	TABLESINDEX = lua_gettop(gL);
	lua_createtable(gL, 0, field_count); // pointer's ext vars subtable

	for (i = 0; i < field_count; i++)
	{
		P_ReadString(save_p, field);
		UnArchiveValue(save_p, TABLESINDEX);
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
	int i, n = lua_gettop(L);
	for (i = 1; i <= n; i++)
		UnArchiveValue(lua_save_p, TABLESINDEX);
	return n;
}

static void UnArchiveTables(save_t *save_p)
{
	int TABLESINDEX;
	UINT16 i, n;
	UINT16 metatableid;

	if (!gL)
		return;

	TABLESINDEX = lua_gettop(gL);

	n = (UINT16)lua_objlen(gL, TABLESINDEX);
	for (i = 1; i <= n; i++)
	{
		lua_rawgeti(gL, TABLESINDEX, i);
		while (true)
		{
			UINT8 e = UnArchiveValue(save_p, TABLESINDEX); // read key
			if (e == 1) // End of table
				break;
			else if (e == 2) // Key contains a new table
				n++;

			if (UnArchiveValue(save_p, TABLESINDEX) == 2) // read value
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

void LUA_Step(void)
{
	if (!gL)
		return;
	lua_settop(gL, 0);
	lua_gc(gL, LUA_GCSTEP, 1);
}

void LUA_Archive(save_t *save_p)
{
	INT32 i;
	thinker_t *th;

	if (gL)
		lua_newtable(gL); // tables to be archived.

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] && i > 0) // dedicated servers...
			continue;
		// all players in game will be archived, even if they just add a 0.
		ArchiveExtVars(save_p, &players[i], "player");
	}

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
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
		lua_pop(gL, 1); // pop tables
}

void LUA_UnArchive(save_t *save_p)
{
	UINT32 mobjnum;
	INT32 i;
	thinker_t *th;

	if (gL)
		lua_newtable(gL); // tables to be read

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] && i > 0) // dedicated servers...
			continue;
		UnArchiveExtVars(save_p, &players[i]);
	}

	do {
		mobjnum = P_ReadUINT32(save_p); // read a mobjnum
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
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
		lua_pop(gL, 1); // pop tables
}

// For mobj_t, player_t, etc. to take custom variables.
int Lua_optoption(lua_State *L, int narg, int def, int list_ref)
{
	if (lua_isnoneornil(L, narg))
		return def;

	I_Assert(lua_checkstack(L, 2));
	luaL_checkstring(L, narg);

	lua_rawgeti(L, LUA_REGISTRYINDEX, list_ref);
	I_Assert(lua_istable(L, -1));
	lua_pushvalue(L, narg);
	lua_rawget(L, -2);

	if (lua_isnumber(L, -1))
		return lua_tointeger(L, -1);
	return -1;
}

int Lua_CreateFieldTable(lua_State *L, const char *const lst[])
{
	int i;

	lua_newtable(L);
	for (i = 0; lst[i] != NULL; i++)
	{
		lua_pushstring(L, lst[i]);
		lua_pushinteger(L, i);
		lua_settable(L, -3);
	}

	return luaL_ref(L, LUA_REGISTRYINDEX);
}

void LUA_PushTaggableObjectArray
(		lua_State *L,
		const char *field,
		lua_CFunction iterator,
		lua_CFunction indexer,
		lua_CFunction counter,
		taggroup_t *garray[],
		size_t * max_elements,
		void * element_array,
		size_t sizeof_element,
		const char *meta)
{
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_createtable(L, 0, 2);
				LUA_SetCFunctionField(L, "iterate", iterator);

				LUA_InsertTaggroupIterator(L, garray,
						max_elements, element_array, sizeof_element, meta);

				lua_createtable(L, 0, 1);
					LUA_SetCFunctionField(L, "__index", indexer);
				lua_setmetatable(L, -2);
			lua_setfield(L, -2, "__index");

			LUA_SetCFunctionField(L, "__len", counter);
		lua_setmetatable(L, -2);
	lua_setglobal(L, field);
}

static void SetBasicMetamethods(
	lua_State *L,
	lua_CFunction get,
	lua_CFunction set,
	lua_CFunction len
)
{
	if (get)
		LUA_SetCFunctionField(L, "__index", get);
	if (set)
		LUA_SetCFunctionField(L, "__newindex", set);
	if (len)
		LUA_SetCFunctionField(L, "__len", len);
}

void LUA_SetCFunctionField(lua_State *L, const char *name, lua_CFunction value)
{
	lua_pushcfunction(L, value);
	lua_setfield(L, -2, name);
}

void LUA_RegisterUserdataMetatable(
	lua_State *L,
	const char *name,
	lua_CFunction get,
	lua_CFunction set,
	lua_CFunction len
)
{
	luaL_newmetatable(L, name);
	SetBasicMetamethods(L, get, set, len);
	lua_pop(L, 1);
}

// If keep is true, leaves the metatable on the stack.
// Otherwise, the stack size remains unchanged.
void LUA_CreateAndSetMetatable(
	lua_State *L,
	lua_CFunction get,
	lua_CFunction set,
	lua_CFunction len,
	boolean keep
)
{
	lua_newtable(L);
	SetBasicMetamethods(L, get, set, len);

	lua_pushvalue(L, -1);
	lua_setmetatable(L, -3);

	if (!keep)
		lua_pop(L, 1);
}

// If keep is true, leaves the userdata and metatable on the stack.
// Otherwise, the stack size remains unchanged.
void LUA_CreateAndSetUserdataField(
	lua_State *L,
	int index,
	const char *name,
	lua_CFunction get,
	lua_CFunction set,
	lua_CFunction len,
	boolean keep
)
{
	if (index < 0 && index > LUA_REGISTRYINDEX)
		index -= 3;

	lua_newuserdata(L, 0);
	LUA_CreateAndSetMetatable(L, get, set, len, true);

	lua_pushvalue(L, -2);
	lua_setfield(L, index, name);

	if (!keep)
		lua_pop(L, 2);
}

void LUA_RegisterGlobalUserdata(
	lua_State *L,
	const char *name,
	lua_CFunction get,
	lua_CFunction set,
	lua_CFunction len
)
{
	LUA_CreateAndSetUserdataField(L, LUA_GLOBALSINDEX, name, get, set, len, false);
}
