// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_clisrv.c
/// \brief SRB2 Network game communication and protocol, all OS independent parts.

#if !defined (UNDER_CE)
#include <time.h>
#endif
#ifdef __GNUC__
#include <unistd.h> //for unlink
#endif

#include "i_system.h"
#include "i_video.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "keys.h"
#include "m_menu.h"
#include "console.h"
#include "d_netfil.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "z_zone.h"
#include "p_local.h"
#include "m_misc.h"
#include "am_map.h"
#include "m_random.h"
#include "mserv.h"
#include "y_inter.h"
#include "r_local.h"
#include "m_argv.h"
#include "p_setup.h"
#include "lzf.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "d_enet.h"

#ifdef CLIENT_LOADINGSCREEN
// cl loading screen
#include "v_video.h"
#include "f_finale.h"
#endif

#ifdef _XBOX
#include "sdl/SRB2XBOX/xboxhelp.h"
#endif

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run

#define PREDICTIONQUEUE BACKUPTICS
#define PREDICTIONMASK (PREDICTIONQUEUE-1)
#define MAX_REASONLENGTH 30

boolean server = true; // true or false but !server == client
boolean nodownload = false;
static boolean serverrunning = false;
INT32 serverplayer = 0;
char motd[254], server_context[8]; // Message of the Day, Unique Context (even without Mumble support)

static tic_t nettics[MAXNETNODES]; // what tic the client have received
static tic_t supposedtics[MAXNETNODES]; // nettics prevision for smaller packet
static UINT8 nodewaiting[MAXNETNODES];
static tic_t firstticstosend; // min of the nettics
static tic_t tictoclear = 0; // optimize d_clearticcmd

// client specific
static ticcmd_t localcmds;
static ticcmd_t localcmds2;
static boolean cl_packetmissed;
// here it is for the secondary local player (splitscreen)
static UINT8 mynode; // my address pointofview server

SINT8 servernode = 0; // the number of the server node
/// \brief do we accept new players?
/// \todo WORK!
boolean acceptnewnode = true;

// engine

// Must be a power of two
#define TEXTCMD_HASH_SIZE 4

typedef struct textcmdplayer_s
{
	INT32 playernum;
	UINT8 cmd[MAXTEXTCMD];
	struct textcmdplayer_s *next;
} textcmdplayer_t;

typedef struct textcmdtic_s
{
	tic_t tic;
	textcmdplayer_t *playercmds[TEXTCMD_HASH_SIZE];
	struct textcmdtic_s *next;
} textcmdtic_t;

ticcmd_t netcmds[MAXPLAYERS];
static textcmdtic_t *textcmds[TEXTCMD_HASH_SIZE] = {NULL};


static consvar_t cv_showjoinaddress = {"showjoinaddress", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t playbackspeed_cons_t[] = {{1, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_playbackspeed = {"playbackspeed", "1", 0, playbackspeed_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

void D_ResetTiccmds(void)
{
	memset(&localcmds, 0, sizeof(ticcmd_t));
	memset(&localcmds2, 0, sizeof(ticcmd_t));
}

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



// some software don't support largest packet
// (original sersetup, not exactely, but the probabylity of sending a packet
// of 512 octet is like 0.1)
UINT16 software_MAXPACKETLENGTH;

tic_t ExpandTics(INT32 low)
{
	INT32 delta;

	delta = low - (gametic & UINT8_MAX);

	if (delta >= -64 && delta <= 64)
		return (gametic & ~UINT8_MAX) + low;
	else if (delta > 64)
		return (gametic & ~UINT8_MAX) - 256 + low;
	else //if (delta < -64)
		return (gametic & ~UINT8_MAX) + 256 + low;
}

// -----------------------------------------------------------------
// Some extra data function for handle textcmd buffer
// -----------------------------------------------------------------

static void (*listnetxcmd[MAXNETXCMD])(UINT8 **p, INT32 playernum);

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum))
{
#ifdef PARANOIA
	if (id >= MAXNETXCMD)
		I_Error("command id %d too big", id);
	if (listnetxcmd[id] != 0)
		I_Error("Command id %d already used", id);
#endif
	listnetxcmd[id] = cmd_f;
}

// Frees all textcmd memory for the specified tic
static void D_FreeTextcmd(void)
{
	// NET TODO
}

// Gets the buffer for the specified ticcmd, or NULL if there isn't one
static UINT8* D_GetExistingTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	while (textcmdtic && textcmdtic->tic != tic) textcmdtic = textcmdtic->next;

	// Do we have an entry for the tic? If so, look for player.
	if (textcmdtic)
	{
		textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
		while (textcmdplayer && textcmdplayer->playernum != playernum) textcmdplayer = textcmdplayer->next;

		if (textcmdplayer) return textcmdplayer->cmd;
	}

	return NULL;
}

// Gets the buffer for the specified ticcmd, creating one if necessary
static UINT8* D_GetTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer_t *textcmdplayer, **tcpprev;

	// Look for the tic.
	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}

	// If we don't have an entry for the tic, make it.
	if (!textcmdtic)
	{
		textcmdtic = *tctprev = Z_Calloc(sizeof (textcmdtic_t), PU_STATIC, NULL);
		textcmdtic->tic = tic;
	}

	tcpprev = &textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer = *tcpprev;

	// Look for the player.
	while (textcmdplayer && textcmdplayer->playernum != playernum)
	{
		tcpprev = &textcmdplayer->next;
		textcmdplayer = textcmdplayer->next;
	}

	// If we don't have an entry for the player, make it.
	if (!textcmdplayer)
	{
		textcmdplayer = *tcpprev = Z_Calloc(sizeof (textcmdplayer_t), PU_STATIC, NULL);
		textcmdplayer->playernum = playernum;
	}

	return textcmdplayer->cmd;
}

static void ExtraDataTicker(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] || i == 0)
		{
			UINT8 *bufferstart = D_GetExistingTextcmd(gametic, i);

			if (bufferstart)
			{
				UINT8 *curpos = bufferstart;
				UINT8 *bufferend = &curpos[curpos[0]+1];

				curpos++;
				while (curpos < bufferend)
				{
					if (*curpos < MAXNETXCMD && listnetxcmd[*curpos])
					{
						const UINT8 id = *curpos;
						curpos++;
						DEBFILE(va("executing x_cmd %u ply %u ", id, i));
						(listnetxcmd[id])(&curpos, i);
						DEBFILE("done\n");
					}
					else
					{
						if (server)
						{
							XBOXSTATIC UINT8 buf[3];

							buf[0] = (UINT8)i;
							buf[1] = KICK_MSG_XD_FAIL;
							SendNetXCmd(XD_KICK, &buf, 2);
							DEBFILE(va("player %d kicked [gametic=%u] reason as follows:\n", i, gametic));
						}
						CONS_Alert(CONS_WARNING, M_GetText("Got unknown net command [%s]=%d (max %d)\n"), sizeu1(curpos - bufferstart), *curpos, bufferstart[0]);
						D_FreeTextcmd();
						return;
					}
				}
			}
		}

	D_FreeTextcmd();
}

static void D_Clearticcmd(void)
{
	INT32 i;

	D_FreeTextcmd();

	for (i = 0; i < MAXPLAYERS; i++)
		netcmds[i].angleturn = 0;

	DEBFILE(va("clear tic\n"));
}

// -----------------------------------------------------------------
// end of extra data function
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// extra data function for lmps
// -----------------------------------------------------------------

// if extradatabit is set, after the ziped tic you find this:
//
//   type   |  description
// ---------+--------------
//   byte   | size of the extradata
//   byte   | the extradata (xd) bits: see XD_...
//            with this byte you know what parameter folow
// if (xd & XDNAMEANDCOLOR)
//   byte   | color
//   char[MAXPLAYERNAME] | name of the player
// endif
// if (xd & XD_WEAPON_PREF)
//   byte   | original weapon switch: boolean, true if use the old
//          | weapon switch methode
//   char[NUMWEAPONS] | the weapon switch priority
//   byte   | autoaim: true if use the old autoaim system
// endif
/*boolean AddLmpExtradata(UINT8 **demo_point, INT32 playernum)
{
	UINT8 *textcmd = D_GetExistingTextcmd(gametic, playernum);

	if (!textcmd)
		return false;

	M_Memcpy(*demo_point, textcmd, textcmd[0]+1);
	*demo_point += textcmd[0]+1;
	return true;
}

void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum)
{
	UINT8 nextra;
	UINT8 *textcmd;

	if (!demo_pointer)
		return;

	textcmd = D_GetTextcmd(gametic, playernum);
	nextra = **demo_pointer;
	M_Memcpy(textcmd, *demo_pointer, nextra + 1);
	// increment demo pointer
	*demo_pointer += nextra + 1;
}*/

// -----------------------------------------------------------------
// end extra data function for lmps
// -----------------------------------------------------------------

#ifndef NONET
#define JOININGAME
#endif

typedef enum
{
	cl_searching,
	cl_downloadfiles,
	cl_askjoin,
	cl_waitjoinresponse,
#ifdef JOININGAME
	cl_downloadsavegame,
#endif
	cl_connected,
	cl_aborted
} cl_mode_t;

static cl_mode_t cl_mode = cl_searching;

// Player name send/load

static void CV_SavePlayerNames(UINT8 **p)
{
	INT32 i = 0;
	// Players in game only.
	for (; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
		{
			WRITEUINT8(*p, 0);
			continue;
		}
		WRITESTRING(*p, player_names[i]);
	}
}

static void CV_LoadPlayerNames(UINT8 **p)
{
	INT32 i = 0;
	char tmp_name[MAXPLAYERNAME+1];
	tmp_name[MAXPLAYERNAME] = 0;

	for (; i < MAXPLAYERS; ++i)
	{
		READSTRING(*p, tmp_name);
		if (tmp_name[0] == 0)
			continue;
		if (tmp_name[MAXPLAYERNAME]) // overflow detected
			I_Error("Received bad server config packet when trying to join");
		memcpy(player_names[i], tmp_name, MAXPLAYERNAME+1);
	}
}

#ifdef CLIENT_LOADINGSCREEN
//
// CL_DrawConnectionStatus
//
// Keep the local client informed of our status.
//
static inline void CL_DrawConnectionStatus(void)
{
	INT32 ccstime = I_GetTime();

	// Draw background fade
	V_DrawFadeScreen();

	// Draw the bottom box.
	M_DrawTextBox(BASEVIDWIDTH/2-128-8, BASEVIDHEIGHT-24-8, 32, 1);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-24-24, V_YELLOWMAP, "Press ESC to abort");

	if (cl_mode != cl_downloadfiles)
	{
		INT32 i, animtime = ((ccstime / 4) & 15) + 16;
		UINT8 palstart = (cl_mode == cl_searching) ? 32 : 96;
		// 15 pal entries total.
		const char *cltext;

		for (i = 0; i < 16; ++i)
			V_DrawFill((BASEVIDWIDTH/2-128) + (i * 16), BASEVIDHEIGHT-24, 16, 8, palstart + ((animtime - i) & 15));

		switch (cl_mode)
		{
#ifdef JOININGAME
			case cl_downloadsavegame:
				cltext = M_GetText("Downloading game state...");
				Net_GetNetStat();
				V_DrawString(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-24, V_20TRANS|V_MONOSPACE,
					va(" %4uK",fileneeded[lastfilenum].currentsize>>10));
				// NET TODO
				//V_DrawRightAlignedString(BASEVIDWIDTH/2+128, BASEVIDHEIGHT-24, V_20TRANS|V_MONOSPACE,
				//	va("%3.1fK/s ", ((double)getbps)/1024));
				break;
#endif
			case cl_askjoin:
			case cl_waitjoinresponse:
				cltext = M_GetText("Requesting to join...");
				break;
			default:
				cltext = M_GetText("Connecting to server...");
				break;
		}
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-24-32, V_YELLOWMAP, cltext);
	}
	else
	{
		INT32 dldlength;
		static char tempname[32];

		Net_GetNetStat();
		dldlength = (INT32)((fileneeded[lastfilenum].currentsize/(double)fileneeded[lastfilenum].totalsize) * 256);
		if (dldlength > 256)
			dldlength = 256;
		V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-24, 256, 8, 111);
		V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-24, dldlength, 8, 96);

		memset(tempname, 0, sizeof(tempname));
		nameonly(strncpy(tempname, fileneeded[lastfilenum].filename, 31));

		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-24-32, V_YELLOWMAP,
			va(M_GetText("Downloading \"%s\""), tempname));
		V_DrawString(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-24, V_20TRANS|V_MONOSPACE,
			va(" %4uK/%4uK",fileneeded[lastfilenum].currentsize>>10,fileneeded[lastfilenum].totalsize>>10));
		// NET TODO
		//V_DrawRightAlignedString(BASEVIDWIDTH/2+128, BASEVIDHEIGHT-24, V_20TRANS|V_MONOSPACE,
		//	va("%3.1fK/s ", ((double)getbps)/1024));
	}
}
#endif

//
// CL_SendJoin
//
// send a special packet for declare how many player in local
// used only in arbitratrenetstart()
static boolean CL_SendJoin(void)
{
	// NET TODO
	return true;
}

static void SV_SendServerInfo(INT32 node, tic_t servertime)
{
	// NET TODO
}

static void SV_SendPlayerInfo(INT32 node)
{
	// NET TODO
}

static boolean SV_SendServerConfig(INT32 node)
{
	// NET TODO
	return true;
}

#ifdef JOININGAME
#define SAVEGAMESIZE (768*1024)

static void SV_SendSaveGame(INT32 node)
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

	P_SaveNetGame();

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

	SendRam(node, buffertosend, length, SF_RAM, 0);
	save_p = NULL;
}

#define TMPSAVENAME "$$$.sav"


static void CL_LoadReceivedSavegame(void)
{
	UINT8 *savebuffer = NULL;
	size_t length, decompressedlen;
	XBOXSTATIC char tmpsave[256];

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
	titledemo = false;
	automapactive = false;

	// load a base level
	playerdeadview = false;

	if (P_LoadNetGame())
	{
		const INT32 actnum = mapheaderinfo[gamemap-1]->actnum;
		CONS_Printf(M_GetText("Map is now \"%s"), G_BuildMapName(gamemap));
		if (strcmp(mapheaderinfo[gamemap-1]->lvlttl, ""))
		{
			CONS_Printf(": %s", mapheaderinfo[gamemap-1]->lvlttl);
			if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
				CONS_Printf(M_GetText("ZONE"));
			if (actnum > 0)
				CONS_Printf(" %2d", actnum);
		}
		CONS_Printf("\"\n");
	}
	else
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't load the level!\n"));
		Z_Free(savebuffer);
		save_p = NULL;
		if (unlink(tmpsave) == -1)
			CONS_Alert(CONS_ERROR, M_GetText("Can't delete %s\n"), tmpsave);
		return;
	}

	// done
	Z_Free(savebuffer);
	save_p = NULL;
	if (unlink(tmpsave) == -1)
		CONS_Alert(CONS_ERROR, M_GetText("Can't delete %s\n"), tmpsave);
	CON_ToggleOff();
}
#endif

#ifndef NONET
static void SendAskInfo(INT32 node, boolean viams)
{
	// NET TODO
}

serverelem_t serverlist[MAXSERVERLIST];
UINT32 serverlistcount = 0;

static void SL_ClearServerList(INT32 connectedserver)
{
	UINT32 i;

	for (i = 0; i < serverlistcount; i++)
		if (connectedserver != serverlist[i].node)
		{
			Net_CloseConnection(serverlist[i].node);
			serverlist[i].node = 0;
		}
	serverlistcount = 0;
}

static UINT32 SL_SearchServer(INT32 node)
{
	UINT32 i;
	for (i = 0; i < serverlistcount; i++)
		if (serverlist[i].node == node)
			return i;

	return UINT32_MAX;
}

static void SL_InsertServer(serverinfo_pak* info, SINT8 node)
{
	UINT32 i;

	// search if not already on it
	i = SL_SearchServer(node);
	if (i == UINT32_MAX)
	{
		// not found add it
		if (serverlistcount >= MAXSERVERLIST)
			return; // list full

		if (info->version != VERSION)
			return; // Not same version.

		if (info->subversion != SUBVERSION)
			return; // Close, but no cigar.

		i = serverlistcount++;
	}

	serverlist[i].info = *info;
	serverlist[i].node = node;

	// resort server list
	M_SortServerList();
}

void CL_UpdateServerList(boolean internetsearch, INT32 room)
{
	// NET TODO
}

#endif // ifndef NONET

// use adaptive send using net_bandwidth and stat.sendbytes
static void CL_ConnectToServer(boolean viams)
{
	INT32 pnumnodes, nodewaited = net_nodecount, i;
	boolean waitmore;
	tic_t oldtic;
#ifndef NONET
	tic_t asksent;
#endif
#ifdef JOININGAME
	XBOXSTATIC char tmpsave[256];

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);
#endif

	cl_mode = cl_searching;

#ifdef CLIENT_LOADINGSCREEN
	lastfilenum = 0;
#endif

#ifdef JOININGAME
	// don't get a corrupt savegame error because tmpsave already exists
	if (FIL_FileExists(tmpsave) && unlink(tmpsave) == -1)
		I_Error("Can't delete %s\n", tmpsave);
#endif

	if (netgame)
	{
		if (servernode < 0 || servernode >= MAXNETNODES)
			CONS_Printf(M_GetText("Searching for a server...\n"));
		else
			CONS_Printf(M_GetText("Contacting the server...\n"));
	}

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // clean up intermission graphics etc

	DEBFILE(va("waiting %d nodes\n", net_nodecount));
	G_SetGamestate(GS_WAITINGPLAYERS);
	wipegamestate = GS_WAITINGPLAYERS;

	adminplayer = -1;
	pnumnodes = 1;
	oldtic = I_GetTime() - 1;
#ifndef NONET
	asksent = (tic_t)-TICRATE;

	i = SL_SearchServer(servernode);

	if (i != -1)
	{
		INT32 j;
		const char *gametypestr = NULL;
		CONS_Printf(M_GetText("Connecting to: %s\n"), serverlist[i].info.servername);
		for (j = 0; gametype_cons_t[j].strvalue; j++)
		{
			if (gametype_cons_t[j].value == serverlist[i].info.gametype)
			{
				gametypestr = gametype_cons_t[j].strvalue;
				break;
			}
		}
		if (gametypestr)
			CONS_Printf(M_GetText("Gametype: %s\n"), gametypestr);
		CONS_Printf(M_GetText("Version: %d.%d.%u\n"), serverlist[i].info.version/100,
		 serverlist[i].info.version%100, serverlist[i].info.subversion);
	}
	SL_ClearServerList(servernode);
#endif

	do
	{
		switch (cl_mode)
		{
			case cl_searching:
#ifndef NONET
				// serverlist is updated by GetPacket function
				if (serverlistcount > 0)
				{
					// this can be a responce to our broadcast request
					if (servernode == -1 || servernode >= MAXNETNODES)
					{
						i = 0;
						servernode = serverlist[i].node;
						CONS_Printf(M_GetText("Found, "));
					}
					else
					{
						i = SL_SearchServer(servernode);
						if (i < 0)
							break; // the case
					}

					// Quit here rather than downloading files and being refused later.
					if (serverlist[i].info.numberofplayer >= serverlist[i].info.maxplayer)
					{
						D_QuitNetGame();
						CL_Reset();
						D_StartTitle();
						M_StartMessage(va(M_GetText("Maximum players reached: %d\n\nPress ESC\n"), serverlist[i].info.maxplayer), NULL, MM_NOTHING);
						return;
					}

					if (!server)
					{
						D_ParseFileneeded(serverlist[i].info.fileneedednum,
							serverlist[i].info.fileneeded);
						CONS_Printf(M_GetText("Checking files...\n"));
						i = CL_CheckFiles();
						if (i == 2) // cannot join for some reason
						{
							D_QuitNetGame();
							CL_Reset();
							D_StartTitle();
							M_StartMessage(M_GetText(
								"You have WAD files loaded or have\n"
								"modified the game in some way, and\n"
								"your file list does not match\n"
								"the server's file list.\n"
								"Please restart SRB2 before connecting.\n\n"
								"Press ESC\n"
							), NULL, MM_NOTHING);
							return;
						}
						else if (i == 1)
							cl_mode = cl_askjoin;
						else
						{
							// must download something
							// can we, though?
							if (!CL_CheckDownloadable()) // nope!
							{
								D_QuitNetGame();
								CL_Reset();
								D_StartTitle();
								M_StartMessage(M_GetText(
									"You cannot conect to this server\n"
									"because you cannot download the files\n"
									"that you are missing from the server.\n\n"
									"See the console or log file for\n"
									"more details.\n\n"
									"Press ESC\n"
								), NULL, MM_NOTHING);
								return;
							}
							// no problem if can't send packet, we will retry later
							if (CL_SendRequestFile())
								cl_mode = cl_downloadfiles;
						}
					}
					else
						cl_mode = cl_askjoin; // files need not be checked for the server.
					break;
				}
				// ask the info to the server (askinfo packet)
				if (asksent + NEWTICRATE < I_GetTime())
				{
					SendAskInfo(servernode, viams);
					asksent = I_GetTime();
				}
#else
				(void)viams;
				// No netgames, so we skip this state.
				cl_mode = cl_askjoin;
#endif // ifndef NONET/else
				break;
			case cl_downloadfiles:
				waitmore = false;
				for (i = 0; i < fileneedednum; i++)
					if (fileneeded[i].status == FS_DOWNLOADING
						|| fileneeded[i].status == FS_REQUESTED)
					{
						waitmore = true;
						break;
					}
				if (waitmore)
					break; // exit the case

				cl_mode = cl_askjoin; // don't break case continue to cljoin request now
			case cl_askjoin:
				CL_LoadServerFiles();
#ifdef JOININGAME
				// prepare structures to save the file
				// WARNING: this can be useless in case of server not in GS_LEVEL
				// but since the network layer doesn't provide ordered packets...
				CL_PrepareDownloadSaveGame(tmpsave);
#endif
				if (CL_SendJoin())
					cl_mode = cl_waitjoinresponse;
				break;
#ifdef JOININGAME
			case cl_downloadsavegame:
				if (fileneeded[0].status == FS_FOUND)
				{
					// Gamestate is now handled within CL_LoadReceivedSavegame()
					CL_LoadReceivedSavegame();
					cl_mode = cl_connected;
				} // don't break case continue to cl_connected
				else
					break;
#endif
			case cl_waitjoinresponse:
			case cl_connected:
			default:
				break;

			// Connection closed by cancel, timeout or refusal.
			case cl_aborted:
				cl_mode = cl_searching;
				return;
		}

		Net_AckTicker();

		// call it only one by tic
		if (oldtic != I_GetTime())
		{
			INT32 key;

			I_OsPolling();
			key = I_GetKey();
			if (key == KEY_ESCAPE)
			{
				CONS_Printf(M_GetText("Network game synchronization aborted.\n"));
//				M_StartMessage(M_GetText("Network game synchronization aborted.\n\nPress ESC\n"), NULL, MM_NOTHING);
				D_QuitNetGame();
				CL_Reset();
				D_StartTitle();
				return;
			}

			// why are these here? this is for servers, we're a client
			//if (key == 's' && server)
			//	doomcom->numnodes = (INT16)pnumnodes;
			//FiletxTicker();
			oldtic = I_GetTime();

#ifdef CLIENT_LOADINGSCREEN
			if (!server && cl_mode != cl_connected && cl_mode != cl_aborted)
			{
				F_TitleScreenTicker(true);
				F_TitleScreenDrawer();
				CL_DrawConnectionStatus();
				I_UpdateNoVsync(); // page flip or blit buffer
				if (moviemode)
					M_SaveFrame();
			}
#else
			CON_Drawer();
			I_UpdateNoVsync();
#endif
		}
		else I_Sleep();

		if (server)
		{
			pnumnodes = 0;
			for (i = 0; i < MAXNETNODES; i++)
				if (nodeingame[i]) pnumnodes++;
		}
	}
	while (!(cl_mode == cl_connected && (!server || (server && nodewaited <= pnumnodes))));

	DEBFILE(va("Synchronisation Finished\n"));

	displayplayer = consoleplayer;
}

#ifndef NONET
typedef struct banreason_s
{
	char *reason;
	struct banreason_s *prev; //-1
	struct banreason_s *next; //+1
} banreason_t;

static banreason_t *reasontail = NULL; //last entry, use prev
static banreason_t *reasonhead = NULL; //1st entry, use next

static void Command_ShowBan(void) //Print out ban list
{
	// NET TODO
}

void D_SaveBan(void)
{
	// NET TODO
}

static void Ban_Add(const char *reason)
{
	// NET TODO
}

static void Command_ClearBans(void)
{
	// NET TODO
}

static void Ban_Load_File(boolean warning)
{
	// NET TODO
}

static void Command_ReloadBan(void)  //recheck ban.txt
{
	Ban_Load_File(true);
}

static void Command_connect(void)
{
	// Assume we connect directly.
	boolean viams = false;

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText(
			"Connect <serveraddress> (port): connect to a server\n"
			"Connect ANY: connect to the first lan server found\n"
			"Connect SELF: connect to your own server.\n"));
		return;
	}

	if (Playing() || titledemo)
	{
		CONS_Printf(M_GetText("You cannot connect while in a game. End this game first.\n"));
		return;
	}

	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	server = false;

	if (!stricmp(COM_Argv(1), "self"))
	{
		servernode = 0;
		server = true;
		/// \bug should be but...
		//SV_SpawnServer();
	}
	else
	{
		// used in menu to connect to a server in the list
		if (netgame && !stricmp(COM_Argv(1), "node"))
		{
			servernode = (SINT8)atoi(COM_Argv(2));

			// Use MS to traverse NAT firewalls.
			viams = true;
		}
		else if (netgame)
		{
			CONS_Printf(M_GetText("You cannot connect while in a game. End this game first.\n"));
			return;
		}
		// NET TODO
		/*else if (I_NetOpenSocket)
		{
			MSCloseUDPSocket(); // Tidy up before wiping the slate.
			I_NetOpenSocket();
			netgame = true;
			multiplayer = true;

			if (!stricmp(COM_Argv(1), "any"))
				servernode = BROADCASTADDR;
			else if (I_NetMakeNodewPort && COM_Argc() >= 3)
				servernode = I_NetMakeNodewPort(COM_Argv(1), COM_Argv(2));
			else if (I_NetMakeNodewPort)
				servernode = I_NetMakeNode(COM_Argv(1));
			else
			{
				CONS_Alert(CONS_ERROR, M_GetText("There is no server identification with this network driver\n"));
				D_CloseConnection();
				return;
			}
		}*/
		else
			CONS_Alert(CONS_ERROR, M_GetText("There is no network driver\n"));
	}

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	CL_ConnectToServer(viams);
}
#endif

static void ResetNode(INT32 node);

//
// CL_ClearPlayer
//
// Clears the player data so that a future client can use this slot
//
void CL_ClearPlayer(INT32 playernum)
{
	if (players[playernum].mo)
	{
		// Don't leave a NiGHTS ghost!
		if ((players[playernum].pflags & PF_NIGHTSMODE) && players[playernum].mo->tracer)
			P_RemoveMobj(players[playernum].mo->tracer);
		P_RemoveMobj(players[playernum].mo);
	}
	players[playernum].mo = NULL;
	memset(&players[playernum], 0, sizeof (player_t));
	sprintf(player_names[playernum], "New Player %u", playernum+1);
}

//
// CL_RemovePlayer
//
// Removes a player from the current game
//
static void CL_RemovePlayer(INT32 playernum)
{
	// Sanity check: exceptional cases (i.e. c-fails) can cause multiple
	// kick commands to be issued for the same player.
	if (!playeringame[playernum])
		return;

	if (server && !demoplayback)
	{
		INT32 node = playernode[playernum];
		playerpernode[node]--;
		if (playerpernode[node] <= 0)
		{
			nodeingame[playernode[playernum]] = false;
			Net_CloseConnection(playernode[playernum]);
			ResetNode(node);
		}
	}

	if (gametype == GT_CTF)
		P_PlayerFlagBurst(&players[playernum], false); // Don't take the flag with you!

	// If in a special stage, redistribute the player's rings across
	// the remaining players.
	if (G_IsSpecialStage(gamemap))
	{
		INT32 i, count, increment, rings;

		for (i = 0, count = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				count++;
		}

		count--;
		rings = players[playernum].health - 1;
		increment = rings/count;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && i != playernum)
			{
				if (rings < increment)
					P_GivePlayerRings(&players[i], rings);
				else
					P_GivePlayerRings(&players[i], increment);

				rings -= increment;
			}
		}
	}

	// Reset player data
	CL_ClearPlayer(playernum);

	// remove avatar of player
	playeringame[playernum] = false;
	playernode[playernum] = UINT8_MAX;
	net_playercount--;

	if (playernum == adminplayer)
		adminplayer = -1; // don't stay admin after you're gone

	if (playernum == displayplayer)
		displayplayer = consoleplayer; // don't look through someone's view who isn't there

#ifdef HAVE_BLUA
	LUA_InvalidatePlayer(&players[playernum]);
#endif

	if (G_TagGametype()) //Check if you still have a game. Location flexible. =P
		P_CheckSurvivors();
	else if (gametype == GT_RACE || gametype == GT_COMPETITION)
		P_CheckRacers();
}

void CL_Reset(void)
{
	if (metalrecording)
		G_StopMetalRecording();
	if (metalplayback)
		G_StopMetalDemo();
	if (demorecording)
		G_CheckDemoStatus();

	// reset client/server code
	DEBFILE(va("\n-=-=-=-=-=-=-= Client reset =-=-=-=-=-=-=-\n\n"));

	if (servernode > 0 && servernode < MAXNETNODES)
	{
		nodeingame[(UINT8)servernode] = false;
		Net_CloseConnection(servernode);
	}
	D_CloseConnection(); // netgame = false
	multiplayer = false;
	servernode = 0;
	server = true;
	net_nodecount = 1;
	net_playercount = 1;
	SV_StopServer();
	SV_ResetServer();

	// make sure we don't leave any fileneeded gunk over from a failed join
	fileneedednum = 0;
	memset(fileneeded, 0, sizeof(fileneeded));

	// D_StartTitle should get done now, but the calling function will handle it
}

#ifndef NONET
static void Command_GetPlayerNum(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
		{
			if (serverplayer == i)
				CONS_Printf(M_GetText("num:%2d  node:%2d  %s\n"), i, playernode[i], player_names[i]);
			else
				CONS_Printf(M_GetText("\x82num:%2d  node:%2d  %s\n"), i, playernode[i], player_names[i]);
		}
}

SINT8 nametonum(const char *name)
{
	INT32 playernum, i;

	if (!strcmp(name, "0"))
		return 0;

	playernum = (SINT8)atoi(name);

	if (playernum < 0 || playernum >= MAXPLAYERS)
		return -1;

	if (playernum)
	{
		if (playeringame[playernum])
			return (SINT8)playernum;
		else
			return -1;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && !stricmp(player_names[i], name))
			return (SINT8)i;

	CONS_Printf(M_GetText("There is no player named \"%s\"\n"), name);

	return -1;
}

/** Lists all players and their player numbers.
  *
  * \sa Command_GetPlayerNum
  */
static void Command_Nodes(void)
{
	INT32 i;
	size_t maxlen = 0;
	const char *address;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		const size_t plen = strlen(player_names[i]);
		if (playeringame[i] && plen > maxlen)
			maxlen = plen;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			CONS_Printf("%.2u: %*s", i, (int)maxlen, player_names[i]);
			CONS_Printf(" - %.2d", playernode[i]);
			// NET TODO: show network addresses

			if (i == adminplayer)
				CONS_Printf(M_GetText(" (verified admin)"));

			if (players[i].spectator)
				CONS_Printf(M_GetText(" (spectator)"));

			CONS_Printf("\n");
		}
	}
}

static void Command_Ban(void)
{
	if (COM_Argc() == 1)
	{
		CONS_Printf(M_GetText("Ban <playername/playernum> <reason>: ban and kick a player\n"));
		return;
	}

	if (server || adminplayer == consoleplayer)
	{
		XBOXSTATIC UINT8 buf[3 + MAX_REASONLENGTH];
		UINT8 *p = buf;
		const SINT8 pn = nametonum(COM_Argv(1));
		const INT32 node = playernode[(INT32)pn];

		if (pn == -1 || pn == 0)
			return;
		else
			WRITEUINT8(p, pn);
		// NET TODO
		//if (I_Ban && !I_Ban(node))
		{
			//CONS_Alert(CONS_WARNING, M_GetText("Too many bans! Geez, that's a lot of people you're excluding...\n"));
			WRITEUINT8(p, KICK_MSG_GO_AWAY);
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		/*else
		{
			Ban_Add(COM_Argv(2));

			if (COM_Argc() == 2)
			{
				WRITEUINT8(p, KICK_MSG_BANNED);
				SendNetXCmd(XD_KICK, &buf, 2);
			}
			else
			{
				size_t i, j = COM_Argc();
				char message[MAX_REASONLENGTH];

				//Steal from the motd code so you don't have to put the reason in quotes.
				strlcpy(message, COM_Argv(2), sizeof message);
				for (i = 3; i < j; i++)
				{
					strlcat(message, " ", sizeof message);
					strlcat(message, COM_Argv(i), sizeof message);
				}

				WRITEUINT8(p, KICK_MSG_CUSTOM_BAN);
				WRITESTRINGN(p, message, MAX_REASONLENGTH);
				SendNetXCmd(XD_KICK, &buf, p - buf);
			}
		}*/
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));

}

static void Command_Kick(void)
{
	XBOXSTATIC UINT8 buf[3 + MAX_REASONLENGTH];
	UINT8 *p = buf;

	if (COM_Argc() == 1)
	{
		CONS_Printf(M_GetText("kick <playername/playernum> <reason>: kick a player\n"));
		return;
	}

	if (server || adminplayer == consoleplayer)
	{
		const SINT8 pn = nametonum(COM_Argv(1));
		WRITESINT8(p, pn);
		if (pn == -1 || pn == 0)
			return;
		if (COM_Argc() == 2)
		{
			WRITEUINT8(p, KICK_MSG_GO_AWAY);
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		else
		{
			size_t i, j = COM_Argc();
			char message[MAX_REASONLENGTH];

			//Steal from the motd code so you don't have to put the reason in quotes.
			strlcpy(message, COM_Argv(2), sizeof message);
			for (i = 3; i < j; i++)
			{
				strlcat(message, " ", sizeof message);
				strlcat(message, COM_Argv(i), sizeof message);
			}

			WRITEUINT8(p, KICK_MSG_CUSTOM_KICK);
			WRITESTRINGN(p, message, MAX_REASONLENGTH);
			SendNetXCmd(XD_KICK, &buf, p - buf);
		}
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}
#endif

static void Got_KickCmd(UINT8 **p, INT32 playernum)
{
	INT32 pnum, msg;
	XBOXSTATIC char buf[3 + MAX_REASONLENGTH];
	char *reason = buf;

	pnum = READUINT8(*p);
	msg = READUINT8(*p);

	if (pnum == serverplayer && playernum == adminplayer)
	{
		CONS_Printf(M_GetText("Server is being shut down remotely. Goodbye!\n"));

		if (server)
			COM_BufAddText("quit\n");

		return;
	}

	// Is playernum authorized to make this kick?
	if (playernum != serverplayer && playernum != adminplayer
		&& !(playerpernode[playernode[playernum]] == 2
		&& nodetoplayer2[playernode[playernum]] == pnum))
	{
		// We received a kick command from someone who isn't the
		// server or admin, and who isn't in splitscreen removing
		// player 2. Thus, it must be someone with a modified
		// binary, trying to kick someone but without having
		// authorization.

		// We deal with this by changing the kick reason to
		// "consistency failure" and kicking the offending user
		// instead.

		// Note: Splitscreen in netgames is broken because of
		// this. Only the server has any idea of which players
		// are using splitscreen on the same computer, so
		// clients cannot always determine if a kick is
		// legitimate.

		CONS_Alert(CONS_WARNING, M_GetText("Illegal kick command received from %s for player %d\n"), player_names[playernum], pnum);

		// In debug, print a longer message with more details.
		// TODO Callum: Should we translate this?
/*
		CONS_Debug(DBG_NETPLAY,
			"So, you must be asking, why is this an illegal kick?\n"
			"Well, let's take a look at the facts, shall we?\n"
			"\n"
			"playernum (this is the guy who did it), he's %d.\n"
			"pnum (the guy he's trying to kick) is %d.\n"
			"playernum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"pnum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"\n"
			"If you think this is a bug, please report it, including all of the details above.\n",
				playernum, pnum,
				playernode[playernum], playerpernode[playernode[playernum]],
				nodetoplayer2[playernode[playernum]],
				playernode[pnum], playerpernode[playernode[pnum]],
				nodetoplayer2[playernode[pnum]]);
*/
		pnum = playernum;
		msg = KICK_MSG_STOP_HACKING;
	}

	CONS_Printf("\x82%s ", player_names[pnum]);

	// If a verified admin banned someone, the server needs to know about it.
	// If the playernum isn't zero (the server) then the server needs to record the ban.
	if (server && playernum && msg == KICK_MSG_BANNED)
	{
		// NET TODO
	}

	switch (msg)
	{
		case KICK_MSG_GO_AWAY:
			CONS_Printf(M_GetText("has been kicked (Go away)\n"));
			break;
		case KICK_MSG_PING_HIGH:
			CONS_Printf(M_GetText("left the game (Broke ping limit)\n"));
			break;
		case KICK_MSG_STOP_HACKING:
			CONS_Printf(M_GetText("left the game (Hack attempted)\n"));
			break;
		case KICK_MSG_TIMEOUT:
			CONS_Printf(M_GetText("left the game (Connection timeout)\n"));
			break;
		case KICK_MSG_XD_FAIL:
			CONS_Printf(M_GetText("left the game (Command buffer error)\n"));
			break;
		case KICK_MSG_PLAYER_QUIT:
			if (netgame) // not splitscreen/bots
				CONS_Printf(M_GetText("left the game\n"));
			break;
		case KICK_MSG_BANNED:
			CONS_Printf(M_GetText("has been banned (Don't come back)\n"));
			break;
		case KICK_MSG_CUSTOM_KICK:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			CONS_Printf(M_GetText("has been kicked (%s)\n"), reason);
			break;
		case KICK_MSG_CUSTOM_BAN:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			CONS_Printf(M_GetText("has been banned (%s)\n"), reason);
			break;
	}

	if (pnum == consoleplayer)
	{
		D_QuitNetGame();
		CL_Reset();
		D_StartTitle();
		if (msg == KICK_MSG_STOP_HACKING) // You shouldn't have done that.
			M_StartMessage(M_GetText("Server closed connection\n\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_PING_HIGH)
			M_StartMessage(M_GetText("Server closed connection\n(Broke ping limit)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_XD_FAIL)
			M_StartMessage(M_GetText("Server closed connection\n(Command buffer error)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_BANNED)
			M_StartMessage(M_GetText("You have been banned by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_KICK)
			M_StartMessage(va(M_GetText("You have been kicked\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_BAN)
			M_StartMessage(va(M_GetText("You have been banned\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else
			M_StartMessage(M_GetText("You have been kicked by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
	}
	else
		CL_RemovePlayer(pnum);
}

consvar_t cv_allownewplayer = {"allowjoin", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL	};
consvar_t cv_joinnextround = {"joinnextround", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; /// \todo not done
static CV_PossibleValue_t maxplayers_cons_t[] = {{2, "MIN"}, {MAXPLAYERS, "MAX"}, {0, NULL}};
consvar_t cv_maxplayers = {"maxplayers", "12", CV_SAVE, maxplayers_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// max file size to send to a player (in kilobytes)
static CV_PossibleValue_t maxsend_cons_t[] = {{0, "MIN"}, {51200, "MAX"}, {0, NULL}};
consvar_t cv_maxsend = {"maxsend", "1024", CV_SAVE, maxsend_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static void Got_AddPlayer(UINT8 **p, INT32 playernum);

// called one time at init
void D_ClientServerInit(void)
{
	DEBFILE(va("- - -== SRB2 v%d.%.2d.%d "VERSIONSTRING" debugfile ==- - -\n",
		VERSION/100, VERSION%100, SUBVERSION));

#ifndef NONET
	COM_AddCommand("getplayernum", Command_GetPlayerNum);
	COM_AddCommand("kick", Command_Kick);
	COM_AddCommand("ban", Command_Ban);
	COM_AddCommand("clearbans", Command_ClearBans);
	COM_AddCommand("showbanlist", Command_ShowBan);
	COM_AddCommand("reloadbans", Command_ReloadBan);
	COM_AddCommand("connect", Command_connect);
	COM_AddCommand("nodes", Command_Nodes);
#endif

	RegisterNetXCmd(XD_KICK, Got_KickCmd);
	RegisterNetXCmd(XD_ADDPLAYER, Got_AddPlayer);
#ifndef NONET
	CV_RegisterVar(&cv_allownewplayer);
	CV_RegisterVar(&cv_joinnextround);
	CV_RegisterVar(&cv_showjoinaddress);
	Ban_Load_File(false);
#endif

	gametic = 0;
	localgametic = 0;

	// do not send anything before the real begin
	SV_StopServer();
	SV_ResetServer();
	if (dedicated)
		SV_SpawnServer();
}

static void ResetNode(INT32 node)
{
	nodeingame[node] = false;
	nodetoplayer[node] = -1;
	nodetoplayer2[node] = -1;
	nettics[node] = gametic;
	supposedtics[node] = gametic;
	nodewaiting[node] = 0;
	playerpernode[node] = 0;
}

void SV_ResetServer(void)
{
	INT32 i;

	for (i = 0; i < MAXNETNODES; i++)
		ResetNode(i);

	for (i = 0; i < MAXPLAYERS; i++)
	{
#ifdef HAVE_BLUA
		LUA_InvalidatePlayer(&players[i]);
#endif
		playeringame[i] = false;
		playernode[i] = UINT8_MAX;
		sprintf(player_names[i], "Player %d", i + 1);
	}

	mynode = 0;
	cl_packetmissed = false;

	if (dedicated)
	{
		nodeingame[0] = true;
		serverplayer = 0;
	}
	else
		serverplayer = consoleplayer;

	if (server)
		servernode = 0;

	net_nodecount = 0;
	net_playercount = 0;

	// clear server_context
	memset(server_context, '-', 8);

	DEBFILE("\n-=-=-=-=-=-=-= Server Reset =-=-=-=-=-=-=-\n\n");
}

static inline void SV_GenContext(void)
{
	UINT8 i;
	// generate server_context, as exactly 8 bytes of randomly mixed A-Z and a-z
	// (hopefully M_Random is initialized!! if not this will be awfully silly!)
	for (i = 0; i < 8; i++)
	{
		const char a = M_RandomKey(26*2);
		if (a <= 26) // uppercase
			server_context[i] = 'A'+a;
		else // lowercase
			server_context[i] = 'a'+(a-26);
	}
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(void)
{
	if (!netgame)
		return;

	DEBFILE("===========================================================================\n"
	        "                  Quitting Game, closing connection\n"
	        "===========================================================================\n");

	// abort send/receive of files
	CloseNetFile();

	D_CloseConnection();
	adminplayer = -1;

	DEBFILE("===========================================================================\n"
	        "                         Log finish\n"
	        "===========================================================================\n");
#ifdef DEBUGFILE
	if (debugfile)
	{
		fclose(debugfile);
		debugfile = NULL;
	}
#endif
}

// add a node to the game (player will follow at map change or at savegame....)
static inline void SV_AddNode(INT32 node)
{
	nettics[node] = gametic;
	supposedtics[node] = gametic;
	// little hack because the server connect to itself and put
	// nodeingame when connected not here
	if (node)
		nodeingame[node] = true;
}

// Xcmd XD_ADDPLAYER
static void Got_AddPlayer(UINT8 **p, INT32 playernum)
{
	INT16 node, newplayernum;
	boolean splitscreenplayer;

	if (playernum != serverplayer)
	{
		// protect against hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal add player command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_STOP_HACKING;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	node = READUINT8(*p);
	newplayernum = READUINT8(*p);
	splitscreenplayer = newplayernum & 0x80;
	newplayernum &= ~0x80;

	// Clear player before joining, lest some things get set incorrectly
	// HACK: don't do this for splitscreen, it relies on preset values
	if (!splitscreen && !botingame)
		CL_ClearPlayer(newplayernum);
	playeringame[newplayernum] = true;
	G_AddPlayer(newplayernum);
	net_playercount++;

	if (netgame)
		CONS_Printf(M_GetText("Player %d has joined the game (node %d)\n"), newplayernum+1, node);

	// the server is creating my player
	if (node == mynode)
	{
		playernode[newplayernum] = 0; // for information only
		if (!splitscreenplayer)
		{
			consoleplayer = newplayernum;
			displayplayer = newplayernum;
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning me\n");
			// Apply player flags as soon as possible!
			players[newplayernum].pflags &= ~(PF_FLIPCAM|PF_ANALOGMODE);
			if (cv_flipcam.value)
				players[newplayernum].pflags |= PF_FLIPCAM;
			if (cv_analog.value)
				players[newplayernum].pflags |= PF_ANALOGMODE;
		}
		else
		{
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning my brother\n");
			if (botingame)
				players[newplayernum].bot = 1;
			// Same goes for player 2 when relevant
			players[newplayernum].pflags &= ~(/*PF_FLIPCAM|*/PF_ANALOGMODE);
			//if (cv_flipcam2.value)
				//players[newplayernum].pflags |= PF_FLIPCAM;
			if (cv_analog2.value)
				players[newplayernum].pflags |= PF_ANALOGMODE;
		}
		D_SendPlayerConfig();
		addedtogame = true;
	}
	else if (server && netgame && cv_showjoinaddress.value)
	{
		// NET TODO: Show player connection address.
	}

	if (server && multiplayer && motd[0] != '\0')
		COM_BufAddText(va("sayto %d %s\n", newplayernum, motd));

#ifdef HAVE_BLUA
	LUAh_PlayerJoin(newplayernum);
#endif
}

static boolean SV_AddWaitingPlayers(void)
{
	INT32 node, n, newplayer = false;
	XBOXSTATIC UINT8 buf[2];
	UINT8 newplayernum = 0;

	// What is the reason for this? Why can't newplayernum always be 0?
	if (dedicated)
		newplayernum = 1;

	for (node = 0; node < MAXNETNODES; node++)
	{
		// splitscreen can allow 2 player in one node
		for (; nodewaiting[node] > 0; nodewaiting[node]--)
		{
			newplayer = true;

			// search for a free playernum
			// we can't use playeringame since it is not updated here
			for (; newplayernum < MAXPLAYERS; newplayernum++)
			{
				for (n = 0; n < MAXNETNODES; n++)
					if (nodetoplayer[n] == newplayernum || nodetoplayer2[n] == newplayernum)
						break;
				if (n == MAXNETNODES)
					break;
			}

			// should never happen since we check the playernum
			// before accepting the join
			I_Assert(newplayernum < MAXPLAYERS);

			playernode[newplayernum] = (UINT8)node;

			buf[0] = (UINT8)node;
			buf[1] = newplayernum;
			if (playerpernode[node] < 1)
				nodetoplayer[node] = newplayernum;
			else
			{
				nodetoplayer2[node] = newplayernum;
				buf[1] |= 0x80;
			}
			playerpernode[node]++;

			SendNetXCmd(XD_ADDPLAYER, &buf, 2);

			DEBFILE(va("Server added player %d node %d\n", newplayernum, node));
			// use the next free slot (we can't put playeringame[newplayernum] = true here)
			newplayernum++;
		}
	}

	return newplayer;
}

void CL_AddSplitscreenPlayer(void)
{
	if (cl_mode == cl_connected)
		CL_SendJoin();
}

void CL_RemoveSplitscreenPlayer(void)
{
	XBOXSTATIC UINT8 buf[2];

	if (cl_mode != cl_connected)
		return;

	buf[0] = (UINT8)secondarydisplayplayer;
	buf[1] = KICK_MSG_PLAYER_QUIT;
	SendNetXCmd(XD_KICK, &buf, 2);
}

// is there a game running
boolean Playing(void)
{
	return (server && serverrunning) || (!server && cl_mode == cl_connected);
}

boolean SV_SpawnServer(void)
{
	if (demoplayback)
		G_StopDemo(); // reset engine parameter
	if (metalplayback)
		G_StopMetalDemo();

	if (!serverrunning)
	{
		CONS_Printf(M_GetText("Starting Server....\n"));
		serverrunning = true;
		SV_ResetServer();
		SV_GenContext();
		if (netgame)
		{
			// NET TODO: Open network socket, register to masterserver, etc.
		}

		// non dedicated server just connect to itself
		if (!dedicated)
			CL_ConnectToServer(false);
	}

	return SV_AddWaitingPlayers();
}

void SV_StopServer(void)
{
	tic_t i;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = wipegamestate = GS_NULL;


	D_Clearticcmd();

	consoleplayer = 0;
	cl_mode = cl_searching;
	serverrunning = false;
}

// called at singleplayer start and stopdemo
void SV_StartSinglePlayerServer(void)
{
	server = true;
	netgame = false;
	multiplayer = false;
	gametype = GT_COOP;

	// no more tic the game with this settings!
	SV_StopServer();

	if (splitscreen)
		multiplayer = true;
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//

//
// TryRunTics
//
static void Local_Maketic(INT32 realtics)
{
	I_OsPolling(); // I_Getevent
	D_ProcessEvents(); // menu responder, cons responder,
	                   // game responder calls HU_Responder, AM_Responder, F_Responder,
	                   // and G_MapEventsToControls
	if (!dedicated) rendergametic = gametic;
	// translate inputs (keyboard/mouse/joystick) into game controls
	G_BuildTiccmd(&localcmds, realtics);
	if (splitscreen || botingame)
		G_BuildTiccmd2(&localcmds2, realtics);

	localcmds.angleturn |= TICCMD_RECEIVED;
}

void SV_SpawnPlayer(INT32 playernum, INT32 x, INT32 y, angle_t angle)
{
	(void)x;
	(void)y;
	// TODO: Send everyone a player spawn message??
}

void TryRunTics(tic_t realtics)
{
	int i;

	// the machine has lagged but it is not so bad
	if (realtics > TICRATE/7)
		realtics = 1;

	if (singletics)
		realtics = 1;

	if (realtics >= 1)
	{
		COM_BufExecute();
		if (mapchangepending)
			D_MapChange(-1, 0, ultimatemode, false, 2, false, fromlevelselect); // finish the map change
	}

	NetUpdate();

#ifdef DEBUGFILE
	if (debugfile && realtics)
	{
		//SoM: 3/30/2000: Need long INT32 in the format string for args 4 & 5.
		//Shut up stupid warning!
		fprintf(debugfile, "------------ Tryruntic: REAL:%d GAME:%d LOAD: %d\n",
			realtics, gametic, debugload);
		debugload = 100000;
	}
#endif

	if (advancedemo)
		D_StartTitle();
	else
		// TODO: re-impliment cv_playbackspeed for demos
		for (i = 0; i < realtics; i++)
		{
			DEBFILE(va("============ Running tic %d (local %d)\n", gametic, localgametic));

			G_Ticker((gametic % NEWTICRATERATIO) == 0);
			ExtraDataTicker();
			gametic++;
		}
}

void NetUpdate(void)
{
	static tic_t gametime = 0;
	static tic_t resptime = 0;
	tic_t nowtime;
	INT32 i;
	INT32 realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	if (realtics <= 0) // nothing new to update
		return;
	if (realtics > 5)
	{
		if (server)
			realtics = 1;
		else
			realtics = 5;
	}

	gametime = nowtime;

	Local_Maketic(realtics); // make local tic, and call menu?

	// client send the command after a receive of the server
	// the server send before because in single player is beter

	MasterClient_Ticker(); // acking the master server

	Net_AckTicker();
	nowtime /= NEWTICRATERATIO;
	if (nowtime > resptime)
	{
		resptime = nowtime;
		M_Ticker();
		CON_Ticker();
	}
}

/** Returns the number of players playing.
  * \return Number of players. Can be zero if we're running a ::dedicated
  *         server.
  * \author Graue <graue@oceanbase.org>
  */
INT32 D_NumPlayers(void)
{
	INT32 num = 0, ix;
	for (ix = 0; ix < MAXPLAYERS; ix++)
		if (playeringame[ix])
			num++;
	return num;
}

tic_t GetLag(INT32 node)
{
	return gametic - nettics[node];
}
