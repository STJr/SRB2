// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hook.h
/// \brief hooks for Lua scripting

#include "r_defs.h"
#include "d_player.h"

enum hook {
	hook_NetVars=0,
	hook_MapChange,
	hook_MapLoad,
	hook_PlayerJoin,
	hook_PreThinkFrame,
	hook_ThinkFrame,
	hook_PostThinkFrame,
	hook_MobjSpawn,
	hook_MobjCollide,
	hook_MobjLineCollide,
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
	hook_BotRespawn,
	hook_LinedefExecute,
	hook_PlayerMsg,
	hook_HurtMsg,
	hook_PlayerSpawn,
	hook_ShieldSpawn,
	hook_ShieldSpecial,
	hook_MobjMoveBlocked,
	hook_MapThingSpawn,
	hook_FollowMobj,
	hook_PlayerCanDamage,
	hook_PlayerQuit,
	hook_IntermissionThinker,
	hook_TeamSwitch,
	hook_ViewpointSwitch,
	hook_SeenPlayer,
	hook_PlayerThink,
	hook_ShouldJingleContinue,
	hook_GameQuit,

	hook_MAX // last hook
};
extern const char *const hookNames[];

void LUAh_MapChange(INT16 mapnumber); // Hook for map change (before load)
void LUAh_MapLoad(void); // Hook for map load
void LUAh_PlayerJoin(int playernum); // Hook for Got_AddPlayer
void LUAh_PreThinkFrame(void); // Hook for frame (before mobj and player thinkers)
void LUAh_ThinkFrame(void); // Hook for frame (after mobj and player thinkers)
void LUAh_PostThinkFrame(void); // Hook for frame (at end of tick, ie after overlays, precipitation, specials)
boolean LUAh_MobjHook(mobj_t *mo, enum hook which);
boolean LUAh_PlayerHook(player_t *plr, enum hook which);
#define LUAh_MobjSpawn(mo) LUAh_MobjHook(mo, hook_MobjSpawn) // Hook for P_SpawnMobj by mobj type
UINT8 LUAh_MobjCollideHook(mobj_t *thing1, mobj_t *thing2, enum hook which);
UINT8 LUAh_MobjLineCollideHook(mobj_t *thing, line_t *line, enum hook which);
#define LUAh_MobjCollide(thing1, thing2) LUAh_MobjCollideHook(thing1, thing2, hook_MobjCollide) // Hook for PIT_CheckThing by (thing) mobj type
#define LUAh_MobjLineCollide(thing, line) LUAh_MobjLineCollideHook(thing, line, hook_MobjLineCollide) // Hook for PIT_CheckThing by (thing) mobj type
#define LUAh_MobjMoveCollide(thing1, thing2) LUAh_MobjCollideHook(thing1, thing2, hook_MobjMoveCollide) // Hook for PIT_CheckThing by (tmthing) mobj type
boolean LUAh_TouchSpecial(mobj_t *special, mobj_t *toucher); // Hook for P_TouchSpecialThing by mobj type
#define LUAh_MobjFuse(mo) LUAh_MobjHook(mo, hook_MobjFuse) // Hook for mobj->fuse == 0 by mobj type
boolean LUAh_MobjThinker(mobj_t *mo); // Hook for P_MobjThinker or P_SceneryThinker by mobj type
#define LUAh_BossThinker(mo) LUAh_MobjHook(mo, hook_BossThinker) // Hook for P_GenericBossThinker by mobj type
UINT8 LUAh_ShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype); // Hook for P_DamageMobj by mobj type (Should mobj take damage?)
boolean LUAh_MobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype); // Hook for P_DamageMobj by mobj type (Mobj actually takes damage!)
boolean LUAh_MobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype); // Hook for P_KillMobj by mobj type
#define LUAh_BossDeath(mo) LUAh_MobjHook(mo, hook_BossDeath) // Hook for A_BossDeath by mobj type
#define LUAh_MobjRemoved(mo) LUAh_MobjHook(mo, hook_MobjRemoved) // Hook for P_RemoveMobj by mobj type
#define LUAh_JumpSpecial(player) LUAh_PlayerHook(player, hook_JumpSpecial) // Hook for P_DoJumpStuff (Any-jumping)
#define LUAh_AbilitySpecial(player) LUAh_PlayerHook(player, hook_AbilitySpecial) // Hook for P_DoJumpStuff (Double-jumping)
#define LUAh_SpinSpecial(player) LUAh_PlayerHook(player, hook_SpinSpecial) // Hook for P_DoSpinAbility (Spin button effect)
#define LUAh_JumpSpinSpecial(player) LUAh_PlayerHook(player, hook_JumpSpinSpecial) // Hook for P_DoJumpStuff (Spin button effect (mid-air))
boolean LUAh_BotTiccmd(player_t *bot, ticcmd_t *cmd); // Hook for B_BuildTiccmd
boolean LUAh_BotAI(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd); // Hook for B_BuildTailsTiccmd by skin name
boolean LUAh_BotRespawn(mobj_t *sonic, mobj_t *tails); // Hook for B_CheckRespawn
boolean LUAh_LinedefExecute(line_t *line, mobj_t *mo, sector_t *sector); // Hook for linedef executors
boolean LUAh_PlayerMsg(int source, int target, int flags, char *msg); // Hook for chat messages
boolean LUAh_HurtMsg(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 damagetype); // Hook for hurt messages
#define LUAh_PlayerSpawn(player) LUAh_PlayerHook(player, hook_PlayerSpawn) // Hook for G_SpawnPlayer
#define LUAh_ShieldSpawn(player) LUAh_PlayerHook(player, hook_ShieldSpawn) // Hook for P_SpawnShieldOrb
#define LUAh_ShieldSpecial(player) LUAh_PlayerHook(player, hook_ShieldSpecial) // Hook for shield abilities
#define LUAh_MobjMoveBlocked(mo) LUAh_MobjHook(mo, hook_MobjMoveBlocked) // Hook for P_XYMovement (when movement is blocked)
boolean LUAh_MapThingSpawn(mobj_t *mo, mapthing_t *mthing); // Hook for P_SpawnMapThing by mobj type
boolean LUAh_FollowMobj(player_t *player, mobj_t *mobj); // Hook for P_PlayerAfterThink Smiles mobj-following
UINT8 LUAh_PlayerCanDamage(player_t *player, mobj_t *mobj); // Hook for P_PlayerCanDamage
void LUAh_PlayerQuit(player_t *plr, kickreason_t reason); // Hook for player quitting
void LUAh_IntermissionThinker(void); // Hook for Y_Ticker
boolean LUAh_TeamSwitch(player_t *player, int newteam, boolean fromspectators, boolean tryingautobalance, boolean tryingscramble); // Hook for team switching in... uh....
UINT8 LUAh_ViewpointSwitch(player_t *player, player_t *newdisplayplayer, boolean forced); // Hook for spy mode
#ifdef SEENAMES
boolean LUAh_SeenPlayer(player_t *player, player_t *seenfriend); // Hook for MT_NAMECHECK
#endif
#define LUAh_PlayerThink(player) LUAh_PlayerHook(player, hook_PlayerThink) // Hook for P_PlayerThink
boolean LUAh_ShouldJingleContinue(player_t *player, const char *musname); // Hook for whether a jingle of the given music should continue playing
void LUAh_GameQuit(void); // Hook for game quitting