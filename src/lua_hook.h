// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hook.h
/// \brief hooks for Lua scripting

#ifdef HAVE_BLUA

#include "r_defs.h"
#include "d_player.h"

enum hook {
	hook_NetVars=0,
	hook_MapChange,
	hook_MapLoad,
	hook_PlayerJoin,
	hook_ThinkFrame,
	hook_MobjSpawn,
	hook_MobjCollide,
	hook_MobjMoveCollide,
	hook_TouchSpecial,
	hook_MobjFuse,
	hook_MobjThinker,
	hook_BossThinker,
	hook_ShouldDamage,
	hook_MobjDamage,
	hook_MobjDeath,
	hook_BossDeath,
	hook_MobjRemoved,
	hook_JumpSpecial,
	hook_AbilitySpecial,
	hook_SpinSpecial,
	hook_JumpSpinSpecial,
	hook_BotTiccmd,
	hook_BotAI,
	hook_LinedefExecute,
	hook_PlayerMsg,
	hook_HurtMsg,
	hook_PlayerSpawn,
	hook_ShieldSpawn,
	hook_ShieldSpecial,

	hook_MAX // last hook
};
extern const char *const hookNames[];

void LUAh_MapChange(void); // Hook for map change (before load)
void LUAh_MapLoad(void); // Hook for map load
void LUAh_PlayerJoin(int playernum); // Hook for Got_AddPlayer
void LUAh_ThinkFrame(void); // Hook for frame (after mobj and player thinkers)
boolean LUAh_MobjHook(mobj_t *mo, enum hook which);
boolean LUAh_PlayerHook(player_t *plr, enum hook which);
#define LUAh_MobjSpawn(mo) LUAh_MobjHook(mo, hook_MobjSpawn) // Hook for P_SpawnMobj by mobj type
UINT8 LUAh_MobjCollideHook(mobj_t *thing1, mobj_t *thing2, enum hook which);
#define LUAh_MobjCollide(thing1, thing2) LUAh_MobjCollideHook(thing1, thing2, hook_MobjCollide) // Hook for PIT_CheckThing by (thing) mobj type
#define LUAh_MobjMoveCollide(thing1, thing2) LUAh_MobjCollideHook(thing1, thing2, hook_MobjMoveCollide) // Hook for PIT_CheckThing by (tmthing) mobj type
boolean LUAh_TouchSpecial(mobj_t *special, mobj_t *toucher); // Hook for P_TouchSpecialThing by mobj type
#define LUAh_MobjFuse(mo) LUAh_MobjHook(mo, hook_MobjFuse) // Hook for mobj->fuse == 0 by mobj type
#define LUAh_MobjThinker(mo) LUAh_MobjHook(mo, hook_MobjThinker) // Hook for P_MobjThinker or P_SceneryThinker by mobj type
#define LUAh_BossThinker(mo) LUAh_MobjHook(mo, hook_BossThinker) // Hook for P_GenericBossThinker by mobj type
UINT8 LUAh_ShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage); // Hook for P_DamageMobj by mobj type (Should mobj take damage?)
boolean LUAh_MobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage); // Hook for P_DamageMobj by mobj type (Mobj actually takes damage!)
boolean LUAh_MobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source); // Hook for P_KillMobj by mobj type
#define LUAh_BossDeath(mo) LUAh_MobjHook(mo, hook_BossDeath) // Hook for A_BossDeath by mobj type
#define LUAh_MobjRemoved(mo) LUAh_MobjHook(mo, hook_MobjRemoved) // Hook for P_RemoveMobj by mobj type
#define LUAh_JumpSpecial(player) LUAh_PlayerHook(player, hook_JumpSpecial) // Hook for P_DoJumpStuff (Any-jumping)
#define LUAh_AbilitySpecial(player) LUAh_PlayerHook(player, hook_AbilitySpecial) // Hook for P_DoJumpStuff (Double-jumping)
#define LUAh_SpinSpecial(player) LUAh_PlayerHook(player, hook_SpinSpecial) // Hook for P_DoSpinAbility (Spin button effect)
#define LUAh_JumpSpinSpecial(player) LUAh_PlayerHook(player, hook_JumpSpinSpecial) // Hook for P_DoJumpStuff (Spin button effect (mid-air))
boolean LUAh_BotTiccmd(player_t *bot, ticcmd_t *cmd); // Hook for B_BuildTiccmd
boolean LUAh_BotAI(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd); // Hook for B_BuildTailsTiccmd by skin name
boolean LUAh_LinedefExecute(line_t *line, mobj_t *mo, sector_t *sector); // Hook for linedef executors
boolean LUAh_PlayerMsg(int source, int target, int flags, char *msg); // Hook for chat messages
boolean LUAh_HurtMsg(player_t *player, mobj_t *inflictor, mobj_t *source); // Hook for hurt messages
#define LUAh_PlayerSpawn(player) LUAh_PlayerHook(player, hook_PlayerSpawn) // Hook for G_SpawnPlayer
#define LUAh_ShieldSpawn(player) LUAh_PlayerHook(player, hook_ShieldSpawn) // Hook for P_SpawnShieldOrb
#define LUAh_ShieldSpecial(player) LUAh_PlayerHook(player, hook_ShieldSpecial) // Hook for shield abilities

#endif
