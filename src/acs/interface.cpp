// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Russell's Smart Interfaces
// Copyright (C) 2025 by Sonic Team Junior.
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  interface.cpp
/// \brief Action Code Script: Interface for the rest of SRB2's game logic

#include <algorithm>
#include <cstddef>
#include <istream>
#include <ostream>
#include <vector>

#include "../tcb_span.hpp"

#include "acsvm.hpp"

#include "interface.h"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../r_defs.h"
#include "../g_game.h"
#include "../w_wad.h"
#include "../i_system.h"
#include "../p_saveg.h"

#include "environment.hpp"
#include "thread.hpp"
#include "stream.hpp"

#include "../cxxutil.hpp"

using namespace srb2::acs;

using std::size_t;

/*--------------------------------------------------
	void ACS_Init(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Init(void)
{
#if 0
	// Initialize ACS on engine start-up.
	ACSEnv = new Environment();
	I_AddExitFunc(ACS_Shutdown);
#endif
}

/*--------------------------------------------------
	void ACS_Shutdown(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Shutdown(void)
{
#if 0
	// Delete ACS environment.
	delete ACSEnv;
	ACSEnv = nullptr;
#endif
}

/*--------------------------------------------------
	void ACS_InvalidateMapScope(size_t mapID)

		See header file for description.
--------------------------------------------------*/
void ACS_InvalidateMapScope(void)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *hub = NULL;
	ACSVM::MapScope *map = NULL;

	// Conclude hub scope, even if we are not using it.
	hub = global->getHubScope(0);
	hub->reset();

	// Conclude current map scope.
	map = hub->getMapScope(0); // This is where you'd put in mapID if you add hub support.
	map->reset();
}

/*--------------------------------------------------
	void ACS_LoadLevelScripts(size_t mapID)

		See header file for description.
--------------------------------------------------*/
void ACS_LoadLevelScripts(size_t mapID)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *hub = nullptr;
	ACSVM::MapScope *map = nullptr;

	std::vector<ACSVM::Module *> modules;

	// Just some notes on how Hexen's scopes work, if anyone
	// intends to implement proper hub logic:

	// The integer is an ID for which hub / map it is,
	// and instead sets active according to which ones
	// should run, since you can go between them.

	// But I didn't intend on implementing these features,
	// since hubs aren't planned for Ring Racers (although
	// they might be useful for SRB2), and I intentionally
	// avoided implementing global ACS (since Lua would be
	// a better language to do that kind of code).

	// Since we literally only are using map scope, we can
	// just free everything between every level. But if
	// hubs are to be implemented, this logic would need
	// to be far more sophisticated.

	// Extra note regarding the commented out ->reset()'s:
	// This is too late! That needs to be done before
	// PU_LEVEL is purged. Call ACS_InvalidateMapScope
	// to take care of that. Those lines are left in
	// only as a warning to future code spelunkers.

	// Restart hub scope, even if we are not using it.
	hub = global->getHubScope(0);
	//hub->reset();
	hub->active = true;

	// Start up new map scope.
	map = hub->getMapScope(0); // This is where you'd put in mapID if you add hub support.
	//map->reset();
	map->active = true;

	// Insert BEHAVIOR lump into the list.
	{
		const char *maplumpname = G_BuildMapName(mapID);

		ACSVM::ModuleName name = ACSVM::ModuleName(
			env->getString( maplumpname ),
			nullptr,
			W_CheckNumForMap(maplumpname)
		);

		modules.push_back(env->getModule(name));
	}

	if (modules.empty() == false)
	{
		// Register the modules with map scope.
		map->addModules(modules.data(), modules.size());
	}
}

/*--------------------------------------------------
	void ACS_RunLevelStartScripts(void)

		See header file for description.
--------------------------------------------------*/
void ACS_RunLevelStartScripts(void)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	map->scriptStartType(ACS_ST_OPEN, {});
}

/*--------------------------------------------------
	void ACS_RunPlayerRespawnScript(player_t *player)

		See header file for description.
--------------------------------------------------*/
void ACS_RunPlayerRespawnScript(player_t *player)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ACSVM::MapScope::ScriptStartInfo scriptInfo;
	ThreadInfo info;

	P_SetTarget(&info.mo, player->mo);

	scriptInfo.info = &info;

	map->scriptStartTypeForced(ACS_ST_RESPAWN, scriptInfo);
}

/*--------------------------------------------------
	void ACS_RunPlayerDeathScript(player_t *player)

		See header file for description.
--------------------------------------------------*/
void ACS_RunPlayerDeathScript(player_t *player)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ACSVM::MapScope::ScriptStartInfo scriptInfo;
	ThreadInfo info;

	P_SetTarget(&info.mo, player->mo);

	scriptInfo.info = &info;

	map->scriptStartTypeForced(ACS_ST_DEATH, scriptInfo);
}

/*--------------------------------------------------
	void ACS_RunPlayerEnterScript(player_t *player)

		See header file for description.
--------------------------------------------------*/
void ACS_RunPlayerEnterScript(player_t *player)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ACSVM::MapScope::ScriptStartInfo scriptInfo;
	ThreadInfo info;

	P_SetTarget(&info.mo, player->mo);

	scriptInfo.info = &info;

	map->scriptStartTypeForced(ACS_ST_ENTER, scriptInfo);
}

/*--------------------------------------------------
	void ACS_RunPlayerFinishScript(player_t *player)

		See header file for description.
--------------------------------------------------*/
void ACS_RunPlayerFinishScript(player_t *player)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ACSVM::MapScope::ScriptStartInfo scriptInfo;
	ThreadInfo info;

	P_SetTarget(&info.mo, player->mo);

	scriptInfo.info = &info;

	map->scriptStartTypeForced(ACS_ST_FINISH, scriptInfo);
}

/*--------------------------------------------------
	void ACS_RunGameOverScript(void)

		See header file for description.
--------------------------------------------------*/
void ACS_RunGameOverScript(void)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	map->scriptStartType(ACS_ST_GAMEOVER, {});
}

/*--------------------------------------------------
	void ACS_Tick(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Tick(void)
{
	Environment *env = &ACSEnv;

	if (env->hasActiveThread() == true)
	{
		env->exec();
	}
}

/*--------------------------------------------------
	static std::vector<ACSVM::Word> ACS_MixArgs(tcb::span<const INT32> args, tcb::span<const char* const> stringArgs)

		Convert strings to ACS arguments and position them
		correctly among integer arguments.

	Input Arguments:-
		args: Integer arguments.
		stringArgs: C string arguments.

	Return:-
		Final argument vector.
--------------------------------------------------*/
static std::vector<ACSVM::Word> ACS_MixArgs(tcb::span<const INT32> args, tcb::span<const char* const> stringArgs)
{
	std::vector<ACSVM::Word> argV;
	size_t first = std::min(args.size(), stringArgs.size());

	auto new_string = [env = &ACSEnv](const char* str) -> ACSVM::Word { return ~env->getString(str, strlen(str))->idx; };

	for (size_t i = 0; i < first; ++i)
	{
		// args[i] must be 0.
		//
		// If ACS_Execute is called from ACS, stringargs[i]
		// will always be set, because there is no
		// differentiation between integers and strings on
		// arguments passed to a function. In this case,
		// string arguments already exist in the ACS string
		// table beforehand (and set in args[i]), so no
		// conversion is required here.
		//
		// If ACS_Execute is called from a map line special,
		// args[i] may be left unset (0), while stringArgs[i]
		// is set. In this case, conversion to ACS string
		// table is necessary.
		argV.push_back(!args[i] && stringArgs[i] ? new_string(stringArgs[i]) : args[i]);
	}

	for (size_t i = first; i < args.size(); ++i)
	{
		argV.push_back(args[i]);
	}

	for (size_t i = first; i < stringArgs.size(); ++i)
	{
		argV.push_back(new_string(stringArgs[i] ? stringArgs[i] : ""));
	}

	return argV;
}

/*--------------------------------------------------
	boolean ACS_Execute(const char *name, const INT32 *args, size_t numArgs, const char *const *stringArgs, size_t numStringArgs, activator_t *activator)

		See header file for description.
--------------------------------------------------*/
boolean ACS_Execute(const char *name, const INT32 *args, size_t numArgs, const char *const *stringArgs, size_t numStringArgs, activator_t *activator)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);
	ACSVM::ScopeID scope{global->id, hub->id, map->id};

	ThreadInfo info{activator};

	ACSVM::String *script = env->getString(name, strlen(name));
	std::vector<ACSVM::Word> argV = ACS_MixArgs(tcb::span {args, numArgs}, tcb::span {stringArgs, numStringArgs});
	return map->scriptStart(script, scope, {argV.data(), argV.size(), &info});
}

/*--------------------------------------------------
	boolean ACS_ExecuteAlways(const char *name, const INT32 *args, size_t numArgs, const char *const *stringArgs, size_t numStringArgs, activator_t *activator)

		See header file for description.
--------------------------------------------------*/
boolean ACS_ExecuteAlways(const char *name, const INT32 *args, size_t numArgs, const char *const *stringArgs, size_t numStringArgs, activator_t *activator)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);
	ACSVM::ScopeID scope{global->id, hub->id, map->id};

	ThreadInfo info{activator};

	ACSVM::String *script = env->getString(name, strlen(name));
	std::vector<ACSVM::Word> argV = ACS_MixArgs(tcb::span {args, numArgs}, tcb::span {stringArgs, numStringArgs});
	return map->scriptStartForced(script, scope, {argV.data(), argV.size(), &info});
}

/*--------------------------------------------------
	boolean ACS_ExecuteResult(const char *name, const INT32 *args, size_t numArgs, activator_t *activator)

		See header file for description.
--------------------------------------------------*/
boolean ACS_ExecuteResult(const char *name, const INT32 *args, size_t numArgs, activator_t *activator)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ThreadInfo info{activator};

	ACSVM::String *script = env->getString(name, strlen(name));
	return map->scriptStartResult(script, {reinterpret_cast<const ACSVM::Word *>(args), numArgs, &info});
}

/*--------------------------------------------------
	boolean ACS_Suspend(const char *name)

		See header file for description.
--------------------------------------------------*/
boolean ACS_Suspend(const char *name)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);
	ACSVM::ScopeID scope{global->id, hub->id, map->id};

	ACSVM::String *script = env->getString(name, strlen(name));
	return map->scriptPause(script, scope);
}

/*--------------------------------------------------
	boolean ACS_Terminate(const char *name)

		See header file for description.
--------------------------------------------------*/
boolean ACS_Terminate(const char *name)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);
	ACSVM::ScopeID scope{global->id, hub->id, map->id};

	ACSVM::String *script = env->getString(name, strlen(name));
	return map->scriptStop(script, scope);
}

/*--------------------------------------------------
	void ACS_Archive(save_t *save)

		See header file for description.
--------------------------------------------------*/
void ACS_Archive(save_t *save)
{
	Environment *env = &ACSEnv;

	SaveBuffer buffer{save};
	std::ostream stream{&buffer};
	ACSVM::Serial serial{stream};

	// Enable debug signatures.
	serial.signs = true;

	try
	{
		serial.saveHead();
		env->saveState(serial);
		serial.saveTail();
	}
	catch (ACSVM::SerialError const &e)
	{
		I_Error("ACS_Archive: %s\n", e.what());
	}
}

/*--------------------------------------------------
	void ACS_UnArchive(save_t *save)

		See header file for description.
--------------------------------------------------*/
void ACS_UnArchive(save_t *save)
{
	Environment *env = &ACSEnv;

	SaveBuffer buffer{save};
	std::istream stream{&buffer};
	ACSVM::Serial serial{stream};

	try
	{
		serial.loadHead();
		env->loadState(serial);
		serial.loadTail();
	}
	catch (ACSVM::SerialError const &e)
	{
		I_Error("ACS_UnArchive: %s\n", e.what());
	}
}
