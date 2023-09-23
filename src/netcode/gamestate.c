// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  gamestate.c
/// \brief Gamestate (re)sending

#include "d_clisrv.h"
#include "d_netfil.h"
#include "gamestate.h"
#include "i_net.h"
#include "protocol.h"
#include "server_connection.h"
#include "../am_map.h"
#include "../byteptr.h"
#include "../console.h"
#include "../d_main.h"
#include "../doomstat.h"
#include "../doomtype.h"
#include "../f_finale.h"
#include "../g_demo.h"
#include "../g_game.h"
#include "../i_time.h"
#include "../lua_script.h"
#include "../lzf.h"
#include "../m_misc.h"
#include "../p_local.h"
#include "../p_saveg.h"
#include "../r_main.h"
#include "../tables.h"
#include "../z_zone.h"
#if defined (__GNUC__) || defined (__unix__)
#include <unistd.h>
#endif

#define SAVEGAMESIZE (768*1024)

UINT8 hu_redownloadinggamestate = 0;
boolean cl_redownloadinggamestate = false;

boolean SV_ResendingSavegameToAnyone(void)
{
	for (INT32 i = 0; i < MAXNETNODES; i++)
		if (netnodes[i].resendingsavegame)
			return true;
	return false;
}

void SV_SendSaveGame(INT32 node, boolean resending)
{
	size_t length, compressedlen;
	UINT8 *savebuffer;
	UINT8 *compressedsave;
	UINT8 *buffertosend;

	// first save it in a malloced buffer
	savebuffer = (UINT8 *)malloc(SAVEGAMESIZE);
	if (!savebuffer)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for savegame\n"));
		return;
	}

	// Leave room for the uncompressed length.
	save_p = savebuffer + sizeof(UINT32);

	P_SaveNetGame(resending);

	length = save_p - savebuffer;
	if (length > SAVEGAMESIZE)
	{
		free(savebuffer);
		save_p = NULL;
		I_Error("Savegame buffer overrun");
	}

	// Allocate space for compressed save: one byte fewer than for the
	// uncompressed data to ensure that the compression is worthwhile.
	compressedsave = malloc(length - 1);
	if (!compressedsave)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for savegame\n"));
		return;
	}

	// Attempt to compress it.
	if((compressedlen = lzf_compress(savebuffer + sizeof(UINT32), length - sizeof(UINT32), compressedsave + sizeof(UINT32), length - sizeof(UINT32) - 1)))
	{
		// Compressing succeeded; send compressed data

		free(savebuffer);

		// State that we're compressed.
		buffertosend = compressedsave;
		WRITEUINT32(compressedsave, length - sizeof(UINT32));
		length = compressedlen + sizeof(UINT32);
	}
	else
	{
		// Compression failed to make it smaller; send original

		free(compressedsave);

		// State that we're not compressed
		buffertosend = savebuffer;
		WRITEUINT32(savebuffer, 0);
	}

	AddRamToSendQueue(node, buffertosend, length, SF_RAM, 0);
	save_p = NULL;

	// Remember when we started sending the savegame so we can handle timeouts
	netnodes[node].sendingsavegame = true;
	netnodes[node].freezetimeout = I_GetTime() + jointimeout + length / 1024; // 1 extra tic for each kilobyte
}

#ifdef DUMPCONSISTENCY
#define TMPSAVENAME "badmath.sav"
static consvar_t cv_dumpconsistency = CVAR_INIT ("dumpconsistency", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);

void SV_SavedGame(void)
{
	size_t length;
	UINT8 *savebuffer;
	char tmpsave[256];

	if (!cv_dumpconsistency.value)
		return;

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	// first save it in a malloced buffer
	save_p = savebuffer = (UINT8 *)malloc(SAVEGAMESIZE);
	if (!save_p)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for savegame\n"));
		return;
	}

	P_SaveNetGame(false);

	length = save_p - savebuffer;
	if (length > SAVEGAMESIZE)
	{
		free(savebuffer);
		save_p = NULL;
		I_Error("Savegame buffer overrun");
	}

	// then save it!
	if (!FIL_WriteFile(tmpsave, savebuffer, length))
		CONS_Printf(M_GetText("Didn't save %s for netgame"), tmpsave);

	free(savebuffer);
	save_p = NULL;
}

#undef  TMPSAVENAME
#endif
#define TMPSAVENAME "$$$.sav"


void CL_LoadReceivedSavegame(boolean reloading)
{
	UINT8 *savebuffer = NULL;
	size_t length, decompressedlen;
	char tmpsave[256];

	FreeFileNeeded();

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	length = FIL_ReadFile(tmpsave, &savebuffer);

	CONS_Printf(M_GetText("Loading savegame length %s\n"), sizeu1(length));
	if (!length)
	{
		I_Error("Can't read savegame sent");
		return;
	}

	save_p = savebuffer;

	// Decompress saved game if necessary.
	decompressedlen = READUINT32(save_p);
	if(decompressedlen > 0)
	{
		UINT8 *decompressedbuffer = Z_Malloc(decompressedlen, PU_STATIC, NULL);
		lzf_decompress(save_p, length - sizeof(UINT32), decompressedbuffer, decompressedlen);
		Z_Free(savebuffer);
		save_p = savebuffer = decompressedbuffer;
	}

	paused = false;
	demoplayback = false;
	titlemapinaction = TITLEMAP_OFF;
	titledemo = false;
	automapactive = false;

	// load a base level
	if (P_LoadNetGame(reloading))
	{
		const UINT8 actnum = mapheaderinfo[gamemap-1]->actnum;
		CONS_Printf(M_GetText("Map is now \"%s"), G_BuildMapName(gamemap));
		if (strcmp(mapheaderinfo[gamemap-1]->lvlttl, ""))
		{
			CONS_Printf(": %s", mapheaderinfo[gamemap-1]->lvlttl);
			if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
				CONS_Printf(M_GetText(" Zone"));
			if (actnum > 0)
				CONS_Printf(" %2d", actnum);
		}
		CONS_Printf("\"\n");
	}

	// done
	Z_Free(savebuffer);
	save_p = NULL;
	if (unlink(tmpsave) == -1)
		CONS_Alert(CONS_ERROR, M_GetText("Can't delete %s\n"), tmpsave);
	consistancy[gametic%BACKUPTICS] = Consistancy();
	CON_ToggleOff();

	// Tell the server we have received and reloaded the gamestate
	// so they know they can resume the game
	netbuffer->packettype = PT_RECEIVEDGAMESTATE;
	HSendPacket(servernode, true, 0, 0);
}

void CL_ReloadReceivedSavegame(void)
{
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		LUA_InvalidatePlayer(&players[i]);
		sprintf(player_names[i], "Player %d", i + 1);
	}

	CL_LoadReceivedSavegame(true);

	neededtic = max(neededtic, gametic);
	maketic = neededtic;

	ticcmd_oldangleturn[0] = players[consoleplayer].oldrelangleturn;
	P_ForceLocalAngle(&players[consoleplayer], (angle_t)(players[consoleplayer].angleturn << 16));
	if (splitscreen)
	{
		ticcmd_oldangleturn[1] = players[secondarydisplayplayer].oldrelangleturn;
		P_ForceLocalAngle(&players[secondarydisplayplayer], (angle_t)(players[secondarydisplayplayer].angleturn << 16));
	}

	camera.subsector = R_PointInSubsector(camera.x, camera.y);
	camera2.subsector = R_PointInSubsector(camera2.x, camera2.y);

	cl_redownloadinggamestate = false;

	CONS_Printf(M_GetText("Game state reloaded\n"));
}

void Command_ResendGamestate(void)
{
	SINT8 playernum;

	if (COM_Argc() == 1)
	{
		CONS_Printf(M_GetText("resendgamestate <playername/playernum>: resend the game state to a player\n"));
		return;
	}
	else if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	playernum = nametonum(COM_Argv(1));
	if (playernum == -1 || playernum == 0)
		return;

	// Send a PT_WILLRESENDGAMESTATE packet to the client so they know what's going on
	netbuffer->packettype = PT_WILLRESENDGAMESTATE;
	if (!HSendPacket(playernode[playernum], true, 0, 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("A problem occurred, please try again.\n"));
		return;
	}
}

void PT_CanReceiveGamestate(SINT8 node)
{
	if (client || netnodes[node].sendingsavegame)
		return;

	CONS_Printf(M_GetText("Resending game state to %s...\n"), player_names[netnodes[node].player]);

	SV_SendSaveGame(node, true); // Resend a complete game state
	netnodes[node].resendingsavegame = true;
}

void PT_ReceivedGamestate(SINT8 node)
{
	netnodes[node].sendingsavegame = false;
	netnodes[node].resendingsavegame = false;
	netnodes[node].savegameresendcooldown = I_GetTime() + 5 * TICRATE;
}

void PT_WillResendGamestate(SINT8 node)
{
	(void)node;

	char tmpsave[256];

	if (server || cl_redownloadinggamestate)
		return;

	// Send back a PT_CANRECEIVEGAMESTATE packet to the server
	// so they know they can start sending the game state
	netbuffer->packettype = PT_CANRECEIVEGAMESTATE;
	if (!HSendPacket(servernode, true, 0, 0))
		return;

	CONS_Printf(M_GetText("Reloading game state...\n"));

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	// Don't get a corrupt savegame error because tmpsave already exists
	if (FIL_FileExists(tmpsave) && unlink(tmpsave) == -1)
		I_Error("Can't delete %s\n", tmpsave);

	CL_PrepareDownloadSaveGame(tmpsave);

	cl_redownloadinggamestate = true;
}
