// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  tic_command.c
/// \brief Tic command handling

#include "tic_command.h"
#include "d_clisrv.h"
#include "net_command.h"
#include "client_connection.h"
#include "gamestate.h"
#include "i_net.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../i_system.h"
#include "../i_time.h"
#include "../byteptr.h"
#include "../doomstat.h"
#include "../doomtype.h"

tic_t firstticstosend; // Smallest netnode.tic
tic_t tictoclear = 0; // Optimize D_ClearTiccmd
ticcmd_t localcmds;
ticcmd_t localcmds2;
boolean cl_packetmissed;
ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];

static inline void *G_DcpyTiccmd(void* dest, const ticcmd_t* src, const size_t n)
{
	const size_t d = n / sizeof(ticcmd_t);
	const size_t r = n % sizeof(ticcmd_t);
	UINT8 *ret = dest;

	if (r)
		M_Memcpy(dest, src, n);
	else if (d)
		G_MoveTiccmd(dest, src, d);
	return ret+n;
}

static inline void *G_ScpyTiccmd(ticcmd_t* dest, void* src, const size_t n)
{
	const size_t d = n / sizeof(ticcmd_t);
	const size_t r = n % sizeof(ticcmd_t);
	UINT8 *ret = src;

	if (r)
		M_Memcpy(dest, src, n);
	else if (d)
		G_MoveTiccmd(dest, src, d);
	return ret+n;
}

/** Guesses the full value of a tic from its lowest byte, for a specific node
  *
  * \param low The lowest byte of the tic value
  * \param node The node to deduce the tic for
  * \return The full tic value
  *
  */
tic_t ExpandTics(INT32 low, INT32 node)
{
	INT32 delta;

	delta = low - (netnodes[node].tic & UINT8_MAX);

	if (delta >= -64 && delta <= 64)
		return (netnodes[node].tic & ~UINT8_MAX) + low;
	else if (delta > 64)
		return (netnodes[node].tic & ~UINT8_MAX) - 256 + low;
	else //if (delta < -64)
		return (netnodes[node].tic & ~UINT8_MAX) + 256 + low;
}

void D_Clearticcmd(tic_t tic)
{
	D_FreeTextcmd(tic);

	for (INT32 i = 0; i < MAXPLAYERS; i++)
		netcmds[tic%BACKUPTICS][i].angleturn = 0;

	DEBFILE(va("clear tic %5u (%2u)\n", tic, tic%BACKUPTICS));
}

void D_ResetTiccmds(void)
{
	memset(&localcmds, 0, sizeof(ticcmd_t));
	memset(&localcmds2, 0, sizeof(ticcmd_t));

	// Reset the net command list
	for (INT32 i = 0; i < TEXTCMD_HASH_SIZE; i++)
		while (textcmds[i])
			D_Clearticcmd(textcmds[i]->tic);
}

// Check ticcmd for "speed hacks"
static void CheckTiccmdHacks(INT32 playernum, tic_t tic)
{
	ticcmd_t *cmd = &netcmds[tic%BACKUPTICS][playernum];
	if (cmd->forwardmove > MAXPLMOVE || cmd->forwardmove < -MAXPLMOVE
		|| cmd->sidemove > MAXPLMOVE || cmd->sidemove < -MAXPLMOVE)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal movement value received from node %d\n"), playernum);
		SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

// Check player consistancy during the level
static void CheckConsistancy(SINT8 nodenum, tic_t tic)
{
	netnode_t *node = &netnodes[nodenum];
	INT16 neededconsistancy = consistancy[tic%BACKUPTICS];
	INT16 clientconsistancy = SHORT(netbuffer->u.clientpak.consistancy);

	if (tic > gametic || tic + BACKUPTICS - 1 <= gametic || gamestate != GS_LEVEL
		|| neededconsistancy == clientconsistancy || SV_ResendingSavegameToAnyone()
		|| node->resendingsavegame || node->savegameresendcooldown > I_GetTime())
		return;

	if (cv_resynchattempts.value)
	{
		// Tell the client we are about to resend them the gamestate
		netbuffer->packettype = PT_WILLRESENDGAMESTATE;
		HSendPacket(nodenum, true, 0, 0);

		node->resendingsavegame = true;

		if (cv_blamecfail.value)
			CONS_Printf(M_GetText("Synch failure for player %d (%s); expected %hd, got %hd\n"),
				node->player+1, player_names[node->player],
				neededconsistancy, clientconsistancy);

		DEBFILE(va("Restoring player %d (synch failure) [%update] %d!=%d\n",
			node->player, tic, neededconsistancy, clientconsistancy));
	}
	else
	{
		SendKick(node->player, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);

		DEBFILE(va("player %d kicked (synch failure) [%u] %d!=%d\n",
			node->player, tic, neededconsistancy, clientconsistancy));
	}
}

void PT_ClientCmd(SINT8 nodenum, INT32 netconsole)
{
	netnode_t *node = &netnodes[nodenum];
	tic_t realend, realstart;

	if (client)
		return;

	// To save bytes, only the low byte of tic numbers are sent
	// Use ExpandTics to figure out what the rest of the bytes are
	realstart = ExpandTics(netbuffer->u.clientpak.client_tic, nodenum);
	realend = ExpandTics(netbuffer->u.clientpak.resendfrom, nodenum);

	if (netbuffer->packettype == PT_CLIENTMIS || netbuffer->packettype == PT_CLIENT2MIS
		|| netbuffer->packettype == PT_NODEKEEPALIVEMIS
		|| node->supposedtic < realend)
	{
		node->supposedtic = realend;
	}
	// Discard out of order packet
	if (node->tic > realend)
	{
		DEBFILE(va("out of order ticcmd discarded nettics = %u\n", node->tic));
		return;
	}

	// Update the nettics
	node->tic = realend;

	// This should probably still timeout though, as the node should always have a player 1 number
	if (netconsole == -1)
		return;

	// As long as clients send valid ticcmds, the server can keep running, so reset the timeout
	/// \todo Use a separate cvar for that kind of timeout?
	node->freezetimeout = I_GetTime() + connectiontimeout;

	// Don't do anything for packets of type NODEKEEPALIVE?
	// Sryder 2018/07/01: Update the freezetimeout still!
	if (netbuffer->packettype == PT_NODEKEEPALIVE
		|| netbuffer->packettype == PT_NODEKEEPALIVEMIS)
		return;

	// If we've alredy received a ticcmd for this tic, just submit it for the next one.
	tic_t faketic = maketic;
	if ((!!(netcmds[maketic % BACKUPTICS][netconsole].angleturn & TICCMD_RECEIVED))
		&& (maketic - firstticstosend < BACKUPTICS - 1))
		faketic++;

	// Copy ticcmd
	G_MoveTiccmd(&netcmds[faketic%BACKUPTICS][netconsole], &netbuffer->u.clientpak.cmd, 1);

	// Splitscreen cmd
	if ((netbuffer->packettype == PT_CLIENT2CMD || netbuffer->packettype == PT_CLIENT2MIS)
		&& node->player2 >= 0)
		G_MoveTiccmd(&netcmds[faketic%BACKUPTICS][(UINT8)node->player2],
			&netbuffer->u.client2pak.cmd2, 1);

	CheckTiccmdHacks(netconsole, faketic);
	CheckConsistancy(nodenum, realstart);
}

void PT_ServerTics(SINT8 node, INT32 netconsole)
{
	tic_t realend, realstart;
	servertics_pak *packet = &netbuffer->u.serverpak;

	if (!netnodes[node].ingame)
	{
		// Do not remove my own server (we have just get a out of order packet)
		if (node != servernode)
		{
			DEBFILE(va("unknown packet received (%d) from unknown host\n",netbuffer->packettype));
			Net_CloseConnection(node);
		}
		return;
	}

	// Only accept PT_SERVERTICS from the server.
	if (node != servernode)
	{
		CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_SERVERTICS", node);
		if (server)
			SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	realstart = packet->starttic;
	realend = realstart + packet->numtics;

	realend = min(realend, gametic + CLIENTBACKUPTICS);
	cl_packetmissed = realstart > neededtic;

	if (realstart <= neededtic && realend > neededtic)
	{
		UINT8 *pak = (UINT8 *)&packet->cmds;
		UINT8 *txtpak = (UINT8 *)&packet->cmds[packet->numslots * packet->numtics];

		for (tic_t i = realstart; i < realend; i++)
		{
			// clear first
			D_Clearticcmd(i);

			// copy the tics
			pak = G_ScpyTiccmd(netcmds[i%BACKUPTICS], pak,
				packet->numslots*sizeof (ticcmd_t));

			CL_CopyNetCommandsFromServerPacket(i, &txtpak);
		}

		neededtic = realend;
	}
	else
	{
		DEBFILE(va("frame not in bound: %u\n", neededtic));
	}
}

// send the client packet to the server
void CL_SendClientCmd(void)
{
	size_t packetsize = 0;
	boolean mis = false;

	netbuffer->packettype = PT_CLIENTCMD;

	if (cl_packetmissed)
	{
		netbuffer->packettype = PT_CLIENTMIS;
		mis = true;
	}

	netbuffer->u.clientpak.resendfrom = (UINT8)(neededtic & UINT8_MAX);
	netbuffer->u.clientpak.client_tic = (UINT8)(gametic & UINT8_MAX);

	if (gamestate == GS_WAITINGPLAYERS)
	{
		// Send PT_NODEKEEPALIVE packet
		netbuffer->packettype = (mis ? PT_NODEKEEPALIVEMIS : PT_NODEKEEPALIVE);
		packetsize = sizeof (clientcmd_pak) - sizeof (ticcmd_t) - sizeof (INT16);
		HSendPacket(servernode, false, 0, packetsize);
	}
	else if (gamestate != GS_NULL && (addedtogame || dedicated))
	{
		packetsize = sizeof (clientcmd_pak);
		G_MoveTiccmd(&netbuffer->u.clientpak.cmd, &localcmds, 1);
		netbuffer->u.clientpak.consistancy = SHORT(consistancy[gametic%BACKUPTICS]);

		// Send a special packet with 2 cmd for splitscreen
		if (splitscreen || botingame)
		{
			netbuffer->packettype = (mis ? PT_CLIENT2MIS : PT_CLIENT2CMD);
			packetsize = sizeof (client2cmd_pak);
			G_MoveTiccmd(&netbuffer->u.client2pak.cmd2, &localcmds2, 1);
		}

		HSendPacket(servernode, false, 0, packetsize);
	}

	if (cl_mode == CL_CONNECTED || dedicated)
		CL_SendNetCommands();
}

// PT_SERVERTICS packets can grow too large for a single UDP packet,
// So this checks how many tics worth of data can be sent in one packet.
// The rest can be sent later, usually the next tic.
static tic_t SV_CalculateNumTicsForPacket(SINT8 nodenum, tic_t firsttic, tic_t lasttic)
{
	size_t size = BASESERVERTICSSIZE;

	for (tic_t tic = firsttic; tic < lasttic; tic++)
	{
		size += sizeof (ticcmd_t) * doomcom->numslots;
		size += TotalTextCmdPerTic(tic);

		if (size > software_MAXPACKETLENGTH)
		{
			DEBFILE(va("packet too large (%s) at tic %d (should be from %d to %d)\n",
				sizeu1(size), tic, firsttic, lasttic));
			lasttic = tic;

			// Too bad: too many players have sent extra data
			// and there is too much data for a single tic.
			// To avoid that, keep the data for the next tic (see PT_TEXTCMD).
			if (lasttic == firsttic)
			{
				if (size > MAXPACKETLENGTH)
					I_Error("Too many players: can't send %s data for %d players to node %d\n"
							"Well sorry nobody is perfect....\n",
							sizeu1(size), doomcom->numslots, nodenum);
				else
				{
					lasttic++; // send it anyway!
					DEBFILE("sending it anyway\n");
				}
			}
			break;
		}
	}

	return lasttic - firsttic;
}

// Sends the server packet
// Sends tic/net commands from firstticstosend to maketic-1
void SV_SendTics(void)
{
	tic_t realfirsttic, lasttictosend;

	// Send to all clients except yourself
	// For each node, create a packet with x tics and send it
	// x is computed using node.supposedtic, max packet size and maketic
	for (INT32 n = 1; n < MAXNETNODES; n++)
		if (netnodes[n].ingame)
		{
			netnode_t *node = &netnodes[n];

			// assert node->supposedtic>=node->tic
			realfirsttic = node->supposedtic;
			lasttictosend = min(maketic, node->tic + CLIENTBACKUPTICS);

			if (realfirsttic >= lasttictosend)
			{
				// Well, we have sent all the tics, so we will use extra bandwidth
				// to resend packets that are supposed lost.
				// This is necessary since lost packet detection
				// works when we receive a packet with firsttic > neededtic (PT_SERVERTICS)
				DEBFILE(va("Nothing to send node %u mak=%u sup=%u net=%u \n",
					n, maketic, node->supposedtic, node->tic));

				realfirsttic = node->tic;

				if (realfirsttic >= lasttictosend || (I_GetTime() + n)&3)
					// All tics are Ok
					continue;

				DEBFILE(va("Sent %d anyway\n", realfirsttic));
			}
			realfirsttic = max(realfirsttic, firstticstosend);

			lasttictosend = realfirsttic + SV_CalculateNumTicsForPacket(n, realfirsttic, lasttictosend);

			// Prepare the packet header
			netbuffer->packettype = PT_SERVERTICS;
			netbuffer->u.serverpak.starttic = realfirsttic;
			netbuffer->u.serverpak.numtics = (UINT8)(lasttictosend - realfirsttic);
			netbuffer->u.serverpak.numslots = (UINT8)SHORT(doomcom->numslots);

			// Fill and send the packet
			UINT8 *bufpos = (UINT8 *)&netbuffer->u.serverpak.cmds;
			for (tic_t i = realfirsttic; i < lasttictosend; i++)
				bufpos = G_DcpyTiccmd(bufpos, netcmds[i%BACKUPTICS], doomcom->numslots * sizeof (ticcmd_t));
			for (tic_t i = realfirsttic; i < lasttictosend; i++)
				SV_WriteNetCommandsForTic(i, &bufpos);
			size_t packsize = bufpos - (UINT8 *)&(netbuffer->u);
			HSendPacket(n, false, 0, packsize);

			// When tics are too large, only one tic is sent so don't go backwards!
			if (lasttictosend-doomcom->extratics > realfirsttic)
				node->supposedtic = lasttictosend-doomcom->extratics;
			else
				node->supposedtic = lasttictosend;
			node->supposedtic = max(node->supposedtic, node->tic);
		}

	// node 0 is me!
	netnodes[0].supposedtic = maketic;
}

void Local_Maketic(INT32 realtics)
{
	I_OsPolling(); // I_Getevent
	D_ProcessEvents(); // menu responder, cons responder,
	                   // game responder calls HU_Responder, AM_Responder,
	                   // and G_MapEventsToControls
	if (!dedicated)
		rendergametic = gametic;
	// translate inputs (keyboard/mouse/joystick) into game controls
	G_BuildTiccmd(&localcmds, realtics, 1);
	if (splitscreen || botingame)
		G_BuildTiccmd(&localcmds2, realtics, 2);

	localcmds.angleturn |= TICCMD_RECEIVED;
	localcmds2.angleturn |= TICCMD_RECEIVED;
}

// create missed tic
void SV_Maketic(void)
{
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		// We didn't receive this tic
		if ((netcmds[maketic % BACKUPTICS][i].angleturn & TICCMD_RECEIVED) == 0)
		{
			ticcmd_t *    ticcmd = &netcmds[(maketic    ) % BACKUPTICS][i];
			ticcmd_t *prevticcmd = &netcmds[(maketic - 1) % BACKUPTICS][i];

			if (players[i].quittime)
			{
				// Copy the angle/aiming from the previous tic
				// and empty the other inputs
				memset(ticcmd, 0, sizeof(netcmds[0][0]));
				ticcmd->angleturn = prevticcmd->angleturn | TICCMD_RECEIVED;
				ticcmd->aiming = prevticcmd->aiming;
			}
			else
			{
				DEBFILE(va("MISS tic%4d for player %d\n", maketic, i));
				// Copy the input from the previous tic
				*ticcmd = *prevticcmd;
				ticcmd->angleturn &= ~TICCMD_RECEIVED;
			}
		}
	}

	// All tics have been processed, make the next
	maketic++;
}
