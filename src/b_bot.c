// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2011-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  b_bot.c
/// \brief Basic bot handling

#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "r_main.h"
#include "p_local.h"
#include "b_bot.h"
#include "lua_hook.h"

// If you want multiple bots, variables like this will
// have to be stuffed in something accessible through player_t.
static boolean lastForward = false;
static boolean lastBlocked = false;
static boolean blocked = false;

static inline void B_BuildTailsTiccmd(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd)
{
	boolean forward=false, backward=false, left=false, right=false, jump=false, spin=false;
	angle_t angle;
	INT16 rangle;
	fixed_t dist;

	// We can't follow Sonic if he's not around!
	if (!sonic || sonic->health <= 0)
		return;

#ifdef HAVE_BLUA
	// Lua can handle it!
	if (LUAh_BotAI(sonic, tails, cmd))
		return;
#endif

	if (tails->player->pflags & (PF_MACESPIN|PF_ITEMHANG))
	{
		dist = P_AproxDistance(tails->x-sonic->x, tails->y-sonic->y);
		if (sonic->player->cmd.buttons & BT_JUMP && sonic->player->pflags & (PF_JUMPED|PF_MACESPIN|PF_ITEMHANG))
			cmd->buttons |= BT_JUMP;
		if (sonic->player->pflags & (PF_MACESPIN|PF_ITEMHANG))
		{
			cmd->forwardmove = sonic->player->cmd.forwardmove;
			cmd->angleturn = abs((signed)(tails->angle - sonic->angle))>>16;
			if (sonic->angle < tails->angle)
				cmd->angleturn = -cmd->angleturn;
		} else if (dist > FixedMul(512*FRACUNIT, tails->scale))
			cmd->buttons |= BT_JUMP;
		return;
	}

	// Gather data about the environment
	dist = P_AproxDistance(tails->x-sonic->x, tails->y-sonic->y);
	if (tails->player->pflags & PF_STARTDASH)
		angle = sonic->angle;
	else
		angle = R_PointToAngle2(tails->x, tails->y, sonic->x, sonic->y);

	// Decide which direction to turn
	angle = (tails->angle - angle);
	if (angle < ANGLE_180) {
		right = true; // We need to turn right
		rangle = AngleFixed(angle)>>FRACBITS;
	} else {
		left = true; // We need to turn left
		rangle = 360-(AngleFixed(angle)>>FRACBITS);
	}

	// Decide to move forward if you're finished turning
	if (abs(rangle) < 10) { // We're facing the right way?
		left = right = false; // Stop turning
		forward = true; // and walk forward instead.
	}
	if (dist < (sonic->radius+tails->radius)*3) // We're close enough?
		forward = false; // Stop walking.

	// Decide when to jump
	if (!(tails->player->pflags & (PF_JUMPED|PF_JUMPDOWN))) { // We're not jumping yet...
		if (forward && lastForward && blocked && lastBlocked) // We've been stopped by a wall or something
			jump = true; // Try to jump up
	} else if ((tails->player->pflags & (PF_JUMPDOWN|PF_JUMPED)) == (PF_JUMPDOWN|PF_JUMPED)) { // When we're already jumping...
		if (lastForward && blocked) // We're still stuck on something?
			jump = true;
		if (sonic->floorz > tails->floorz) // He's still above us? Jump HIGHER, then!
			jump = true;
	}

	// Decide when to spin
	if (sonic->player->pflags & PF_STARTDASH
	&& (tails->player->pflags & PF_STARTDASH || (P_AproxDistance(tails->momx, tails->momy) < 2*FRACUNIT && !forward)))
		spin = true;

	// Turn the virtual keypresses into ticcmd_t.
	B_KeysToTiccmd(tails, cmd, forward, backward, left, right, false, false, jump, spin);

	// Update our status
	lastForward = forward;
	lastBlocked = blocked;
	blocked = false;
}

void B_BuildTiccmd(player_t *player, ticcmd_t *cmd)
{
	// Can't build a ticcmd if we aren't spawned...
	if (!player->mo)
		return;

	if (player->playerstate == PST_DEAD)
	{
		if (B_CheckRespawn(player))
			cmd->buttons |= BT_JUMP;
		return;
	}

	// Bot AI isn't programmed in analog.
	CV_SetValue(&cv_analog2, false);

#ifdef HAVE_BLUA
	// Let Lua scripts build ticcmds
	if (LUAh_BotTiccmd(player, cmd))
		return;
#endif

	// We don't have any main character AI, sorry. D:
	if (player-players == consoleplayer)
		return;

	// Basic Tails AI
	B_BuildTailsTiccmd(players[consoleplayer].mo, player->mo, cmd);
}

void B_KeysToTiccmd(mobj_t *mo, ticcmd_t *cmd, boolean forward, boolean backward, boolean left, boolean right, boolean strafeleft, boolean straferight, boolean jump, boolean spin)
{
	// Turn the virtual keypresses into ticcmd_t.
	if (twodlevel || mo->flags2 & MF2_TWOD) {
		if (players[consoleplayer].climbing
		|| mo->player->pflags & PF_GLIDING) {
			// Don't mess with bot inputs during these unhandled movement conditions.
			// The normal AI doesn't use abilities, so custom AI should be sending us exactly what it wants anyway.
			if (forward)
				cmd->forwardmove += MAXPLMOVE<<FRACBITS>>16;
			if (backward)
				cmd->forwardmove -= MAXPLMOVE<<FRACBITS>>16;
			if (left || strafeleft)
				cmd->sidemove -= MAXPLMOVE<<FRACBITS>>16;
			if (right || straferight)
				cmd->sidemove += MAXPLMOVE<<FRACBITS>>16;
		} else {
			// In standard 2D mode, interpret "forward" as "the way you're facing" and everything else as "the way you're not facing"
			if (left || right)
				backward = true;
			left = right = false;
			if (forward) {
				if (mo->angle < ANGLE_90 || mo->angle > ANGLE_270)
					right = true;
				else
					left = true;
			} else if (backward) {
				if (mo->angle < ANGLE_90 || mo->angle > ANGLE_270)
					left = true;
				else
					right = true;
			}
			if (left || strafeleft)
				cmd->sidemove -= MAXPLMOVE<<FRACBITS>>16;
			if (right || straferight)
				cmd->sidemove += MAXPLMOVE<<FRACBITS>>16;
		}
	} else {
		if (forward)
			cmd->forwardmove += MAXPLMOVE<<FRACBITS>>16;
		if (backward)
			cmd->forwardmove -= MAXPLMOVE<<FRACBITS>>16;
		if (left)
			cmd->angleturn += 1280;
		if (right)
			cmd->angleturn -= 1280;
		if (strafeleft)
			cmd->sidemove -= MAXPLMOVE<<FRACBITS>>16;
		if (straferight)
			cmd->sidemove += MAXPLMOVE<<FRACBITS>>16;
	}
	if (jump)
		cmd->buttons |= BT_JUMP;
	if (spin)
		cmd->buttons |= BT_USE;
}

void B_MoveBlocked(player_t *player)
{
	(void)player;
	blocked = true;
}

boolean B_CheckRespawn(player_t *player)
{
	mobj_t *sonic = players[consoleplayer].mo;
	mobj_t *tails = player->mo;

	// We can't follow Sonic if he's not around!
	if (!sonic || sonic->health <= 0)
		return false;

	// Check if Sonic is busy first.
	// If he's doing any of these things, he probably doesn't want to see us.
	if (sonic->player->pflags & (PF_ROPEHANG|PF_GLIDING|PF_CARRIED|PF_SLIDING|PF_ITEMHANG|PF_MACESPIN|PF_NIGHTSMODE)
	|| (sonic->player->panim != PA_IDLE && sonic->player->panim != PA_WALK))
		return false;

	// Low ceiling, do not want!
	if (sonic->ceilingz - sonic->z < 2*sonic->height)
		return false;

	// If you're dead, wait a few seconds to respawn.
	if (player->playerstate == PST_DEAD) {
		if (player->deadtimer > 4*TICRATE)
			return true;
		return false;
	}

	// If you can't see Sonic, I guess we should?
	if (!P_CheckSight(sonic, tails) && P_AproxDistance(P_AproxDistance(tails->x-sonic->x, tails->y-sonic->y), tails->z-sonic->z) > FixedMul(1024*FRACUNIT, tails->scale))
		return true;
	return false;
}

void B_RespawnBot(INT32 playernum)
{
	player_t *player = &players[playernum];
	fixed_t x,y,z;
	mobj_t *sonic = players[consoleplayer].mo;
	mobj_t *tails;

	if (!sonic || sonic->health <= 0)
		return;

	player->bot = 1;
	P_SpawnPlayer(playernum);
	tails = player->mo;

	x = sonic->x;
	y = sonic->y;
	if (sonic->eflags & MFE_VERTICALFLIP) {
		tails->eflags |= MFE_VERTICALFLIP;
		z = sonic->z - FixedMul(512*FRACUNIT,sonic->scale);
		if (z < sonic->floorz)
			z = sonic->floorz;
	} else {
		z = sonic->z + sonic->height + FixedMul(512*FRACUNIT,sonic->scale);
		if (z > sonic->ceilingz - sonic->height)
			z = sonic->ceilingz - sonic->height;
	}

	if (sonic->flags2 & MF2_OBJECTFLIP)
		tails->flags2 |= MF2_OBJECTFLIP;
	if (sonic->flags2 & MF2_TWOD)
		tails->flags2 |= MF2_TWOD;
	if (sonic->eflags & MFE_UNDERWATER)
		tails->eflags |= MFE_UNDERWATER;
	player->powers[pw_underwater] = sonic->player->powers[pw_underwater];
	player->powers[pw_spacetime] = sonic->player->powers[pw_spacetime];
	player->powers[pw_gravityboots] = sonic->player->powers[pw_gravityboots];
	player->powers[pw_nocontrol] = sonic->player->powers[pw_nocontrol];

	P_TeleportMove(tails, x, y, z);
	if (player->charability == CA_FLY)
	{
		P_SetPlayerMobjState(tails, S_PLAY_ABL1);
		tails->player->powers[pw_tailsfly] = (UINT16)-1;
	}
	else
		P_SetPlayerMobjState(tails, S_PLAY_FALL1);
	P_SetScale(tails, sonic->scale);
	tails->destscale = sonic->destscale;
}
