// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
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

tic_t firstticstosend; // min of the nettics
tic_t tictoclear = 0; // optimize d_clearticcmd
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
	INT32 i;

	D_FreeTextcmd(tic);

	for (i = 0; i < MAXPLAYERS; i++)
		netcmds[tic%BACKUPTICS][i].angleturn = 0;

	DEBFILE(va("clear tic %5u (%2u)\n", tic, tic%BACKUPTICS));
}

void D_ResetTiccmds(void)
{
	INT32 i;

	memset(&localcmds, 0, sizeof(ticcmd_t));
	memset(&localcmds2, 0, sizeof(ticcmd_t));

	// Reset the net command list
	for (i = 0; i < TEXTCMD_HASH_SIZE; i++)
		while (textcmds[i])
			D_Clearticcmd(textcmds[i]->tic);
}

// Check ticcmd for "speed hacks"
static void CheckTiccmdHacks(INT32 playernum)
{
	ticcmd_t *cmd = &netcmds[maketic%BACKUPTICS][playernum];
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

void PT_ClientCmd(SINT8 node, INT32 netconsole)
{
	tic_t realend, realstart;

	if (client)
		return;

	// To save bytes, only the low byte of tic numbers are sent
	// Use ExpandTics to figure out what the rest of the bytes are
	realstart = ExpandTics(netbuffer->u.clientpak.client_tic, node);
	realend = ExpandTics(netbuffer->u.clientpak.resendfrom, node);

	if (netbuffer->packettype == PT_CLIENTMIS || netbuffer->packettype == PT_CLIENT2MIS
		|| netbuffer->packettype == PT_NODEKEEPALIVEMIS
		|| netnodes[node].supposedtic < realend)
	{
		netnodes[node].supposedtic = realend;
	}
	// Discard out of order packet
	if (netnodes[node].tic > realend)
	{
		DEBFILE(va("out of order ticcmd discarded nettics = %u\n", netnodes[node].tic));
		return;
	}

	// Update the nettics
	netnodes[node].tic = realend;

	// Don't do anything for packets of type NODEKEEPALIVE?
	if (netconsole == -1 || netbuffer->packettype == PT_NODEKEEPALIVE
		|| netbuffer->packettype == PT_NODEKEEPALIVEMIS)
		return;

	// As long as clients send valid ticcmds, the server can keep running, so reset the timeout
	/// \todo Use a separate cvar for that kind of timeout?
	netnodes[node].freezetimeout = I_GetTime() + connectiontimeout;

	// Copy ticcmd
	G_MoveTiccmd(&netcmds[maketic%BACKUPTICS][netconsole], &netbuffer->u.clientpak.cmd, 1);

	// Splitscreen cmd
	if ((netbuffer->packettype == PT_CLIENT2CMD || netbuffer->packettype == PT_CLIENT2MIS)
		&& netnodes[node].player2 >= 0)
		G_MoveTiccmd(&netcmds[maketic%BACKUPTICS][(UINT8)netnodes[node].player2],
			&netbuffer->u.client2pak.cmd2, 1);

	CheckTiccmdHacks(netconsole);
	CheckConsistancy(node, realstart);
}

void PT_ServerTics(SINT8 node, INT32 netconsole)
{
	tic_t realend, realstart;

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

	realstart = netbuffer->u.serverpak.starttic;
	realend = realstart + netbuffer->u.serverpak.numtics;

	if (realend > gametic + CLIENTBACKUPTICS)
		realend = gametic + CLIENTBACKUPTICS;
	cl_packetmissed = realstart > neededtic;

	if (realstart <= neededtic && realend > neededtic)
	{
		UINT8 *pak = (UINT8 *)&netbuffer->u.serverpak.cmds;

		for (tic_t i = realstart; i < realend; i++)
		{
			// clear first
			D_Clearticcmd(i);

			// copy the tics
			pak = G_ScpyTiccmd(netcmds[i%BACKUPTICS], pak,
				netbuffer->u.serverpak.numslots*sizeof (ticcmd_t));

			CL_CopyNetCommandsFromServerPacket(i);
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

	netbuffer->packettype = PT_CLIENTCMD;

	if (cl_packetmissed)
		netbuffer->packettype++;
	netbuffer->u.clientpak.resendfrom = (UINT8)(neededtic & UINT8_MAX);
	netbuffer->u.clientpak.client_tic = (UINT8)(gametic & UINT8_MAX);

	if (gamestate == GS_WAITINGPLAYERS)
	{
		// Send PT_NODEKEEPALIVE packet
		netbuffer->packettype += 4;
		packetsize = sizeof (clientcmd_pak) - sizeof (ticcmd_t) - sizeof (INT16);
		HSendPacket(servernode, false, 0, packetsize);
	}
	else if (gamestate != GS_NULL && (addedtogame || dedicated))
	{
		G_MoveTiccmd(&netbuffer->u.clientpak.cmd, &localcmds, 1);
		netbuffer->u.clientpak.consistancy = SHORT(consistancy[gametic%BACKUPTICS]);

		// Send a special packet with 2 cmd for splitscreen
		if (splitscreen || botingame)
		{
			netbuffer->packettype += 2;
			G_MoveTiccmd(&netbuffer->u.client2pak.cmd2, &localcmds2, 1);
			packetsize = sizeof (client2cmd_pak);
		}
		else
			packetsize = sizeof (clientcmd_pak);

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
	tic_t tic;

	for (tic = firsttic; tic < lasttic; tic++)
	{
		size += sizeof (ticcmd_t) * doomcom->numslots;
		size += TotalTextCmdPerTic(tic);

		if (size > software_MAXPACKETLENGTH)
		{
			DEBFILE(va("packet too large (%s) at tic %d (should be from %d to %d)\n",
				sizeu1(size), tic, firsttic, lasttic));
			lasttic = tic;

			// too bad: too much player have send extradata and there is too
			//          much data in one tic.
			// To avoid it put the data on the next tic. (see getpacket
			// textcmd case) but when numplayer changes the computation can be different
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

// send the server packet
// send tic from firstticstosend to maketic-1
void SV_SendTics(void)
{
	tic_t realfirsttic, lasttictosend, i;
	UINT32 n;
	size_t packsize;
	UINT8 *bufpos;

	// send to all client but not to me
	// for each node create a packet with x tics and send it
	// x is computed using netnodes[n].supposedtic, max packet size and maketic
	for (n = 1; n < MAXNETNODES; n++)
		if (netnodes[n].ingame)
		{
			// assert netnodes[n].supposedtic>=netnodes[n].tic
			realfirsttic = netnodes[n].supposedtic;
			lasttictosend = min(maketic, netnodes[n].tic + CLIENTBACKUPTICS);

			if (realfirsttic >= lasttictosend)
			{
				// well we have sent all tics we will so use extrabandwidth
				// to resent packet that are supposed lost (this is necessary since lost
				// packet detection work when we have received packet with firsttic > neededtic
				// (getpacket servertics case)
				DEBFILE(va("Nothing to send node %u mak=%u sup=%u net=%u \n",
					n, maketic, netnodes[n].supposedtic, netnodes[n].tic));
				realfirsttic = netnodes[n].tic;
				if (realfirsttic >= lasttictosend || (I_GetTime() + n)&3)
					// all tic are ok
					continue;
				DEBFILE(va("Sent %d anyway\n", realfirsttic));
			}
			if (realfirsttic < firstticstosend)
				realfirsttic = firstticstosend;

			lasttictosend = realfirsttic + SV_CalculateNumTicsForPacket(n, realfirsttic, lasttictosend);

			// Send the tics
			netbuffer->packettype = PT_SERVERTICS;
			netbuffer->u.serverpak.starttic = realfirsttic;
			netbuffer->u.serverpak.numtics = (UINT8)(lasttictosend - realfirsttic);
			netbuffer->u.serverpak.numslots = (UINT8)SHORT(doomcom->numslots);
			bufpos = (UINT8 *)&netbuffer->u.serverpak.cmds;

			for (i = realfirsttic; i < lasttictosend; i++)
			{
				bufpos = G_DcpyTiccmd(bufpos, netcmds[i%BACKUPTICS], doomcom->numslots * sizeof (ticcmd_t));
			}

			// add textcmds
			for (i = realfirsttic; i < lasttictosend; i++)
				SV_WriteNetCommandsForTic();
			packsize = bufpos - (UINT8 *)&(netbuffer->u);

			HSendPacket(n, false, 0, packsize);
			// when tic are too large, only one tic is sent so don't go backward!
			if (lasttictosend-doomcom->extratics > realfirsttic)
				netnodes[n].supposedtic = lasttictosend-doomcom->extratics;
			else
				netnodes[n].supposedtic = lasttictosend;
			if (netnodes[n].supposedtic < netnodes[n].tic) netnodes[n].supposedtic = netnodes[n].tic;
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
	if (!dedicated) rendergametic = gametic;
	// translate inputs (keyboard/mouse/gamepad) into game controls
	G_BuildTiccmd(&localcmds, realtics, 1);
	if (splitscreen || botingame)
		G_BuildTiccmd(&localcmds2, realtics, 2);

	localcmds.angleturn |= TICCMD_RECEIVED;
	localcmds2.angleturn |= TICCMD_RECEIVED;
}

// create missed tic
void SV_Maketic(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
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

	// all tic are now proceed make the next
	maketic++;
}
