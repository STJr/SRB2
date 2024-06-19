// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  client_connection.h
/// \brief Client connection handling

#include "client_connection.h"
#include "gamestate.h"
#include "d_clisrv.h"
#include "d_netfil.h"
#include "../d_main.h"
#include "../f_finale.h"
#include "../g_game.h"
#include "../g_input.h"
#include "i_net.h"
#include "../i_system.h"
#include "../i_time.h"
#include "../i_video.h"
#include "../keys.h"
#include "../m_menu.h"
#include "../m_misc.h"
#include "../snake.h"
#include "../s_sound.h"
#include "../v_video.h"
#include "../y_inter.h"
#include "../z_zone.h"
#include "../doomtype.h"
#include "../doomstat.h"
#if defined (__GNUC__) || defined (__unix__)
#include <unistd.h>
#endif

cl_mode_t cl_mode = CL_SEARCHING;
static UINT16 cl_lastcheckedfilecount = 0;	// used for full file list
boolean serverisfull = false; // lets us be aware if the server was full after we check files, but before downloading, so we can ask if the user still wants to download or not
tic_t firstconnectattempttime = 0;
UINT8 mynode;
static void *snake = NULL;

static boolean IsDownloadingFile(void)
{
	if (cl_mode == CL_DOWNLOADFILES || cl_mode == CL_DOWNLOADHTTPFILES)
		return filedownload.current != -1;

	return false;
}

static void DrawConnectionStatusBox(void)
{
	M_DrawTextBox(BASEVIDWIDTH/2-128-8, BASEVIDHEIGHT-16-8, 32, 1);

	if (cl_mode == CL_CONFIRMCONNECT || IsDownloadingFile())
		return;

	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-16, V_YELLOWMAP, "Press ESC to abort");
}

static void DrawFileProgress(fileneeded_t *file, int y)
{
	Net_GetNetStat();

	INT32 dldlength = (INT32)((file->currentsize/(double)file->totalsize) * 256);
	if (dldlength > 256)
		dldlength = 256;
	V_DrawFill(BASEVIDWIDTH/2-128, y, 256, 8, 111);
	V_DrawFill(BASEVIDWIDTH/2-128, y, dldlength, 8, 96);

	const char *progress_str;
	if (file->totalsize >= 1024*1024)
		progress_str = va(" %.2fMiB/%.2fMiB", (double)file->currentsize / (1024*1024), (double)file->totalsize / (1024*1024));
	else if (file->totalsize < 1024)
		progress_str = va(" %4uB/%4uB", file->currentsize, file->totalsize);
	else
		progress_str = va(" %.2fKiB/%.2fKiB", (double)file->currentsize / 1024, (double)file->totalsize / 1024);

	V_DrawString(BASEVIDWIDTH/2-128, y, V_20TRANS|V_ALLOWLOWERCASE, progress_str);

	V_DrawRightAlignedString(BASEVIDWIDTH/2+128, y, V_20TRANS|V_MONOSPACE, va("%3.1fK/s ", ((double)getbps)/1024));
}

//
// CL_DrawConnectionStatus
//
// Keep the local client informed of our status.
//
static void CL_DrawConnectionStatus(void)
{
	INT32 ccstime = I_GetTime();

	// Draw background fade
	V_DrawFadeScreen(0xFF00, 16); // force default

	if (cl_mode != CL_DOWNLOADFILES && cl_mode != CL_DOWNLOADHTTPFILES && cl_mode != CL_LOADFILES)
	{
		INT32 animtime = ((ccstime / 4) & 15) + 16;
		UINT8 palstart;
		const char *cltext;

		// Draw the bottom box.
		DrawConnectionStatusBox();

		if (cl_mode == CL_SEARCHING)
			palstart = 32; // Red
		else if (cl_mode == CL_CONFIRMCONNECT)
			palstart = 48; // Orange
		else
			palstart = 96; // Green

		if (!(cl_mode == CL_DOWNLOADSAVEGAME && filedownload.current != -1))
			for (INT32 i = 0; i < 16; ++i) // 15 pal entries total.
				V_DrawFill((BASEVIDWIDTH/2-128) + (i * 16), BASEVIDHEIGHT-16, 16, 8, palstart + ((animtime - i) & 15));

		switch (cl_mode)
		{
			case CL_DOWNLOADSAVEGAME:
				if (fileneeded && filedownload.current != -1)
				{
					fileneeded_t *file = &fileneeded[filedownload.current];

					cltext = M_GetText("Downloading game state...");

					DrawFileProgress(file, BASEVIDHEIGHT-16);
				}
				else
					cltext = M_GetText("Waiting to download game state...");
				break;
			case CL_ASKFULLFILELIST:
			case CL_CHECKFILES:
				cltext = M_GetText("Checking server addon list...");
				break;
			case CL_CONFIRMCONNECT:
				cltext = "";
				break;
			case CL_LOADFILES:
				cltext = M_GetText("Loading server addons...");
				break;
			case CL_ASKJOIN:
			case CL_WAITJOINRESPONSE:
				if (serverisfull)
					cltext = M_GetText("Server full, waiting for a slot...");
				else
					cltext = M_GetText("Requesting to join...");
				break;
			default:
				cltext = M_GetText("Connecting to server...");
				break;
		}
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_YELLOWMAP, cltext);
	}
	else
	{
		if (cl_mode == CL_LOADFILES)
		{
			INT32 totalfileslength;
			INT32 loadcompletednum = 0;

			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-16, V_YELLOWMAP, "Press ESC to abort");

			// ima just count files here
			if (fileneeded)
			{
				for (INT32 i = 0; i < fileneedednum; i++)
					if (fileneeded[i].status == FS_OPEN)
						loadcompletednum++;
			}

			// Loading progress
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_YELLOWMAP, "Loading server addons...");
			totalfileslength = (INT32)((loadcompletednum/(double)(fileneedednum)) * 256);
			M_DrawTextBox(BASEVIDWIDTH/2-128-8, BASEVIDHEIGHT-16-8, 32, 1);
			V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, 256, 8, 111);
			V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, totalfileslength, 8, 96);
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16, V_20TRANS|V_MONOSPACE,
				va(" %2u/%2u files",loadcompletednum,fileneedednum));
		}
		else if (filedownload.current != -1)
		{
			char tempname[28];
			fileneeded_t *file;
			char *filename;

			if (snake)
				Snake_Draw(snake);

			// Draw the bottom box.
			DrawConnectionStatusBox();

			if (fileneeded)
			{
				file = &fileneeded[filedownload.current];
				filename = file->filename;
			}
			else
				return;

			// offset filename to just the name only part
			filename += strlen(filename) - nameonlylength(filename);

			if (strlen(filename) > sizeof(tempname)-1) // too long to display fully
			{
				size_t endhalfpos = strlen(filename)-10;
				// display as first 14 chars + ... + last 10 chars
				// which should add up to 27 if our math(s) is correct
				snprintf(tempname, sizeof(tempname), "%.14s...%.10s", filename, filename+endhalfpos);
			}
			else // we can copy the whole thing in safely
			{
				strlcpy(tempname, filename, sizeof(tempname));
			}

			// Lactozilla: Disabled, see below change
			// (also it doesn't really fit on a typical SRB2 screen)
#if 0
			const char *download_str = cl_mode == CL_DOWNLOADHTTPFILES
				? M_GetText("HTTP downloading \"%s\"")
				: M_GetText("Downloading \"%s\"");
#else
			const char *download_str = M_GetText("Downloading \"%s\"");
#endif

			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_ALLOWLOWERCASE|V_YELLOWMAP,
				va(download_str, tempname));

			// Rusty: actually lets do this instead
			if (cl_mode == CL_DOWNLOADHTTPFILES)
			{
				const char *http_source = filedownload.http_source;

				if (strlen(http_source) > sizeof(tempname)-1) // too long to display fully
				{
					size_t endhalfpos = strlen(http_source)-10;
					// display as first 14 chars + ... + last 10 chars
					// which should add up to 27 if our math(s) is correct
					snprintf(tempname, sizeof(tempname), "%.14s...%.10s", http_source, http_source+endhalfpos);
				}
				else // we can copy the whole thing in safely
				{
					strlcpy(tempname, http_source, sizeof(tempname));
				}

				V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-16, V_ALLOWLOWERCASE|V_YELLOWMAP,
					va(M_GetText("from %s"), tempname));
			}
			else
			{
				V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-16, V_ALLOWLOWERCASE|V_YELLOWMAP,
					M_GetText("from the server"));
			}

			DrawFileProgress(file, BASEVIDHEIGHT-16);
		}
		else
		{
			if (snake)
				Snake_Draw(snake);

			DrawConnectionStatusBox();
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_YELLOWMAP,
				M_GetText("Waiting to download files..."));
		}
	}
}

static boolean CL_AskFileList(INT32 firstfile)
{
	netbuffer->packettype = PT_TELLFILESNEEDED;
	netbuffer->u.filesneedednum = firstfile;

	return HSendPacket(servernode, false, 0, sizeof (INT32));
}

/** Sends a PT_CLIENTJOIN packet to the server
  *
  * \return True if the packet was successfully sent
  *
  */
boolean CL_SendJoin(void)
{
	UINT8 localplayers = 1;
	char const *player2name;
	if (netgame)
		CONS_Printf(M_GetText("Sending join request...\n"));
	netbuffer->packettype = PT_CLIENTJOIN;

	netbuffer->u.clientcfg.modversion = MODVERSION;
	strncpy(netbuffer->u.clientcfg.application,
			SRB2APPLICATION,
			sizeof netbuffer->u.clientcfg.application);

	if (splitscreen || botingame)
		localplayers++;
	netbuffer->u.clientcfg.localplayers = localplayers;

	CleanupPlayerName(consoleplayer, cv_playername.zstring);
	if (splitscreen)
		CleanupPlayerName(1, cv_playername2.zstring); // 1 is a HACK? oh no

	// Avoid empty string on bots to avoid softlocking in singleplayer
	if (botingame)
		player2name = strcmp(cv_playername.zstring, "Tails") == 0 ? "Tail" : "Tails";
	else
		player2name = cv_playername2.zstring;

	strncpy(netbuffer->u.clientcfg.names[0], cv_playername.zstring, sizeof(netbuffer->u.clientcfg.names[0])-1);
	strncpy(netbuffer->u.clientcfg.names[1], player2name, MAXPLAYERNAME);

	return HSendPacket(servernode, true, 0, sizeof (clientconfig_pak));
}

static void SendAskInfo(INT32 node)
{
	const tic_t asktime = I_GetTime();
	netbuffer->packettype = PT_ASKINFO;
	netbuffer->u.askinfo.version = VERSION;
	netbuffer->u.askinfo.time = (tic_t)LONG(asktime);

	// Even if this never arrives due to the host being firewalled, we've
	// now allowed traffic from the host to us in, so once the MS relays
	// our address to the host, it'll be able to speak to us.
	HSendPacket(node, false, 0, sizeof (askinfo_pak));
}

serverelem_t serverlist[MAXSERVERLIST];
UINT32 serverlistcount = 0;

#define FORCECLOSE 0x8000

static void SL_ClearServerList(INT32 connectedserver)
{
	for (UINT32 i = 0; i < serverlistcount; i++)
		if (connectedserver != serverlist[i].node)
		{
			Net_CloseConnection(serverlist[i].node|FORCECLOSE);
			serverlist[i].node = 0;
		}
	serverlistcount = 0;
}

static UINT32 SL_SearchServer(INT32 node)
{
	for (UINT32 i = 0; i < serverlistcount; i++)
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

		// check it later if connecting to this one
		if (node != servernode)
		{
			if (info->_255 != 255)
				return; // Old packet format

			if (info->packetversion != PACKETVERSION)
				return; // Old new packet format

			if (info->version != VERSION)
				return; // Not same version.

			if (info->subversion != SUBVERSION)
				return; // Close, but no cigar.

			if (strcmp(info->application, SRB2APPLICATION))
				return; // That's a different mod
		}

		i = serverlistcount++;
	}

	serverlist[i].info = *info;
	serverlist[i].node = node;

	// resort server list
	M_SortServerList();
}

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
struct Fetch_servers_ctx
{
	int room;
	int id;
};

static void
Fetch_servers_thread (struct Fetch_servers_ctx *ctx)
{
	msg_server_t *server_list;

	server_list = GetShortServersList(ctx->room, ctx->id);

	if (server_list)
	{
		I_lock_mutex(&ms_QueryId_mutex);
		{
			if (ctx->id != ms_QueryId)
			{
				free(server_list);
				server_list = NULL;
			}
		}
		I_unlock_mutex(ms_QueryId_mutex);

		if (server_list)
		{
			I_lock_mutex(&m_menu_mutex);
			{
				if (m_waiting_mode == M_WAITING_SERVERS)
					m_waiting_mode = M_NOT_WAITING;
			}
			I_unlock_mutex(m_menu_mutex);

			I_lock_mutex(&ms_ServerList_mutex);
			{
				ms_ServerList = server_list;
			}
			I_unlock_mutex(ms_ServerList_mutex);
		}
	}

	free(ctx);
}
#endif // defined (MASTERSERVER) && defined (HAVE_THREADS)

void CL_QueryServerList (msg_server_t *server_list)
{
	for (INT32 i = 0; server_list[i].header.buffer[0]; i++)
	{
		// Make sure MS version matches our own, to
		// thwart nefarious servers who lie to the MS.

		// lol bruh, that version COMES from the servers
		//if (strcmp(version, server_list[i].version) == 0)
		{
			INT32 node = I_NetMakeNodewPort(server_list[i].ip, server_list[i].port);
			if (node == -1)
				break; // no more node free
			SendAskInfo(node);
			// Force close the connection so that servers can't eat
			// up nodes forever if we never get a reply back from them
			// (usually when they've not forwarded their ports).
			//
			// Don't worry, we'll get in contact with the working
			// servers again when they send SERVERINFO to us later!
			//
			// (Note: as a side effect this probably means every
			// server in the list will probably be using the same node (e.g. node 1),
			// not that it matters which nodes they use when
			// the connections are closed afterwards anyway)
			// -- Monster Iestyn 12/11/18
			Net_CloseConnection(node|FORCECLOSE);
		}
	}
}

void CL_UpdateServerList(boolean internetsearch, INT32 room)
{
	(void)internetsearch;
	(void)room;

	SL_ClearServerList(0);

	if (!netgame && I_NetOpenSocket)
	{
		if (I_NetOpenSocket())
		{
			netgame = true;
			multiplayer = true;
		}
	}

	// search for local servers
	if (netgame)
		SendAskInfo(BROADCASTADDR);

#ifdef MASTERSERVER
	if (internetsearch)
	{
#ifdef HAVE_THREADS
		struct Fetch_servers_ctx *ctx;

		ctx = malloc(sizeof *ctx);

		// This called from M_Refresh so I don't use a mutex
		m_waiting_mode = M_WAITING_SERVERS;

		I_lock_mutex(&ms_QueryId_mutex);
		{
			ctx->id = ms_QueryId;
		}
		I_unlock_mutex(ms_QueryId_mutex);

		ctx->room = room;

		I_spawn_thread("fetch-servers", (I_thread_fn)Fetch_servers_thread, ctx);
#else
		msg_server_t *server_list;

		server_list = GetShortServersList(room, 0);

		if (server_list)
		{
			CL_QueryServerList(server_list);
			free(server_list);
		}
#endif
	}
#endif // MASTERSERVER
}

static boolean IsFileDownloadable(fileneeded_t *file)
{
	return file->status == FS_NOTFOUND || file->status == FS_MD5SUMBAD;
}

static boolean UseDirectDownloader(void)
{
	return filedownload.http_source[0] == '\0' || filedownload.http_failed;
}

static void DoLoadFiles(void)
{
	Snake_Free(&snake);

	cl_mode = CL_LOADFILES;
}

static void AbortConnection(void)
{
	Snake_Free(&snake);

	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();

	// Will be reset by caller. Signals refusal.
	cl_mode = CL_ABORTED;
}

static void BeginDownload(boolean direct)
{
	filedownload.current = 0;
	filedownload.remaining = 0;

	for (int i = 0; i < fileneedednum; i++)
	{
		// Lactozilla: Rusty had fixed this SLIGHTLY incorrectly.
		// Since [redacted] doesn't HAVE its own file transmission code - it spins an HTTP server - it wasn't
		// instantly obvious to us where to do this, and we didn't want to check the original implementation.
		if (fileneeded[i].status == FS_FALLBACK)
			fileneeded[i].status = FS_NOTFOUND;

		if (IsFileDownloadable(&fileneeded[i]))
			filedownload.remaining++;
	}

	if (!filedownload.remaining)
	{
		DoLoadFiles();
		return;
	}

	if (!direct)
	{
		cl_mode = CL_DOWNLOADHTTPFILES;
		Snake_Allocate(&snake);

		// Discard any paused downloads
		CL_AbortDownloadResume();
	}
	else
	{
		// do old LEGACY request
		if (CL_SendFileRequest())
		{
			cl_mode = CL_DOWNLOADFILES;

			// don't alloc snake if already alloced
			if (!snake)
				Snake_Allocate(&snake);
		}
		else
		{
			AbortConnection();

			// why was this its own cl_mode_t?
			M_StartMessage(M_GetText(
				"The direct downloader encountered an error.\n"
				"See the logfile for more info.\n\n"
				"Press ESC\n"
			), NULL, MM_NOTHING);
		}
	}
}

static void M_ConfirmConnect(event_t *ev)
{
	if (ev->type == ev_keydown)
	{
		if (ev->key == ' ' || ev->key == 'y' || ev->key == KEY_ENTER || ev->key == KEY_JOY1)
		{
			BeginDownload(UseDirectDownloader());
			M_ClearMenus(true);
		}
		else if (ev->key == 'n' || ev->key == KEY_ESCAPE || ev->key == KEY_JOY1 + 3)
		{
			cl_mode = CL_ABORTED;
			M_ClearMenus(true);
		}
	}
}

static const char *GetPrintableFileSize(UINT64 filesize)
{
	static char downloadsize[32];

	if (filesize >= 1024*1024)
		snprintf(downloadsize, sizeof(downloadsize), "%.2fMiB", (double)filesize / (1024*1024));
	else if (filesize < 1024)
		snprintf(downloadsize, sizeof(downloadsize), "%sB", sizeu1(filesize));
	else
		snprintf(downloadsize, sizeof(downloadsize), "%.2fKiB", (double)filesize / 1024);

	return downloadsize;
}

static void ShowDownloadConsentMessage(void)
{
	UINT64 totalsize = 0;

	filedownload.completednum = 0;
	filedownload.completedsize = 0;

	if (fileneeded == NULL)
		I_Error("CL_FinishedFileList: fileneeded == NULL");

	for (int i = 0; i < fileneedednum; i++)
	{
		if (IsFileDownloadable(&fileneeded[i]))
			totalsize += fileneeded[i].totalsize;
	}

	const char *downloadsize = GetPrintableFileSize(totalsize);

	if (serverisfull)
		M_StartMessage(va(M_GetText(
			"This server is full!\n"
			"Download of %s of additional\ncontent is required to join.\n"
			"\n"
			"You may download server addons,\nand wait for a slot.\n"
			"\n"
			"Press ENTER to continue\nor ESC to cancel.\n"
		), downloadsize), M_ConfirmConnect, MM_EVENTHANDLER);
	else
		M_StartMessage(va(M_GetText(
			"Download of %s of additional\ncontent is required to join.\n"
			"\n"
			"Press ENTER to continue\nor ESC to cancel.\n"
		), downloadsize), M_ConfirmConnect, MM_EVENTHANDLER);

	cl_mode = CL_CONFIRMCONNECT;
	curfadevalue = 0;
}

static const char *GetDirectDownloadFailReason(UINT8 dlstatus)
{
	switch (dlstatus)
	{
		case DLSTATUS_TOOLARGE:
			return M_GetText("Some addons are larger than the server is willing to send.");
		case DLSTATUS_WONTSEND:
			return M_GetText("The server is not allowing download requests.");
		case DLSTATUS_NODOWNLOAD:
			return M_GetText("All addons downloadable, but you have chosen to disable addon downloading.");
		case DLSTATUS_FOLDER:
			return M_GetText("One or more addons were added as a folder, which the server cannot send.");
	}

	return "Unknown reason";
}

static void HandleDirectDownloadFail(UINT8 dlstatus)
{
	// not downloadable, put reason in console
	CONS_Alert(CONS_NOTICE, M_GetText("You need additional addons to connect to this server:\n"));

	for (UINT8 i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].folder || (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN))
		{
			CONS_Printf(" * \"%s\" ", fileneeded[i].filename);

			if (fileneeded[i].folder)
			{
				CONS_Printf("(folder)");
			}
			else
			{
				CONS_Printf("(%s)", GetPrintableFileSize(fileneeded[i].totalsize));

				if (fileneeded[i].status == FS_NOTFOUND)
					CONS_Printf(M_GetText(" not found, md5: "));
				else if (fileneeded[i].status == FS_MD5SUMBAD)
					CONS_Printf(M_GetText(" wrong version, md5: "));

				char md5tmp[33];
				for (INT32 j = 0; j < 16; j++)
					sprintf(&md5tmp[j*2], "%02x", fileneeded[i].md5sum[j]);
				CONS_Printf("%s", md5tmp);
			}

			CONS_Printf("\n");
		}
	}

	CONS_Printf("%s\n", GetDirectDownloadFailReason(dlstatus));
}

static boolean CL_FinishedFileList(void)
{
	INT32 i = CL_CheckFiles();
	if (i == 4) // still checking ...
	{
		return true;
	}
	else if (i == 3) // too many files
	{
		AbortConnection();
		M_StartMessage(M_GetText(
			"You have too many WAD files loaded\n"
			"to add ones the server is using.\n"
			"Please restart SRB2 before connecting.\n\n"
			"Press ESC\n"
		), NULL, MM_NOTHING);
		return false;
	}
	else if (i == 2) // cannot join for some reason
	{
		AbortConnection();
		M_StartMessage(M_GetText(
			"You have the wrong addons loaded.\n"
			"\n"
			"To play on this server, restart\n"
			"the game and don't load any addons.\n"
			"SRB2 will automatically add\n"
			"everything you need when you join.\n"
			"\n"
			"Press ESC\n"
		), NULL, MM_NOTHING);
		return false;
	}
	else if (i == 1)
	{
		if (serverisfull)
		{
			M_StartMessage(M_GetText(
				"This server is full!\n"
				"\n"
				"You may load server addons (if any), and wait for a slot.\n"
				"\n"
				"Press ENTER to continue\nor ESC to cancel.\n\n"
			), M_ConfirmConnect, MM_EVENTHANDLER);
			cl_mode = CL_CONFIRMCONNECT;
			curfadevalue = 0;
		}
		else
			cl_mode = CL_LOADFILES;
	}
	else
	{
		// must download something
		// can we, though?
		// Rusty: always check downloadable
		UINT8 status = CL_CheckDownloadable(UseDirectDownloader());
		if (status != DLSTATUS_OK)
		{
			HandleDirectDownloadFail(status);
			AbortConnection();
			M_StartMessage(M_GetText(
				"An error occurred when trying to\n"
				"download missing addons.\n"
				"(This is almost always a problem\n"
				"with the server, not your game.)\n"
				"\n"
				"See the console or log file\n"
				"for additional details.\n"
				"\n"
				"Press ESC\n"
			), NULL, MM_NOTHING);
			return false;
		}

		if (!filedownload.http_failed)
		{
			// show download consent modal ONCE!
			ShowDownloadConsentMessage();
		}
		else
		{
			// do a direct download
			BeginDownload(true);
		}
	}

	return true;
}

static const char * InvalidServerReason (serverinfo_pak *info)
{
#define EOT "\nPress ESC\n"

	// Magic number for new packet format
	if (info->_255 != 255)
	{
		return
			"Outdated server (version unknown).\n" EOT;
	}

	if (strncmp(info->application, SRB2APPLICATION, sizeof
				info->application))
	{
		return va(
				"%s cannot connect\n"
				"to %s servers.\n" EOT,
				SRB2APPLICATION,
				info->application);
	}

	if (
			info->packetversion != PACKETVERSION ||
			info->version != VERSION ||
			info->subversion != SUBVERSION
	){
		return va(
				"Incompatible %s versions.\n"
				"(server version %d.%d.%d)\n" EOT,
				SRB2APPLICATION,
				info->version / 100,
				info->version % 100,
				info->subversion);
	}

	switch (info->refusereason)
	{
		case REFUSE_BANNED:
			return
				"You have been banned\n"
				"from the server.\n" EOT;
		case REFUSE_JOINS_DISABLED:
			return
				"The server is not accepting\n"
				"joins for the moment.\n" EOT;
		case REFUSE_SLOTS_FULL:
			return va(
					"Maximum players reached: %d\n" EOT,
					info->maxplayer - D_NumBots());
		default:
			if (info->refusereason)
			{
				return
					"You can't join.\n"
					"I don't know why,\n"
					"but you can't join.\n" EOT;
			}
	}

	return NULL;

#undef EOT
}

/** Called by CL_ServerConnectionTicker
  *
  * \param asksent The last time we asked the server to join. We re-ask every second in case our request got lost in transmit.
  * \return False if the connection was aborted
  * \sa CL_ServerConnectionTicker
  * \sa CL_ConnectToServer
  *
  */
static boolean CL_ServerConnectionSearchTicker(tic_t *asksent)
{
	INT32 i;

	// serverlist is updated by GetPackets
	if (serverlistcount > 0)
	{
		// This can be a response to our broadcast request
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
				return true;
		}

		if (client)
		{
			serverinfo_pak *info = &serverlist[i].info;

			if (info->refusereason == REFUSE_SLOTS_FULL)
				serverisfull = true;
			else
			{
				const char *reason = InvalidServerReason(info);

				// Quit here rather than downloading files
				// and being refused later.
				if (reason)
				{
					char *message = Z_StrDup(reason);
					AbortConnection();
					M_StartMessage(message, NULL, MM_NOTHING);
					Z_Free(message);
					return false;
				}
			}

			if (serverlist[i].info.httpsource[0])
				strlcpy(filedownload.http_source, serverlist[i].info.httpsource, MAX_MIRROR_LENGTH);
			else
				filedownload.http_source[0] = '\0';

			D_ParseFileneeded(info->fileneedednum, info->fileneeded, 0);

			if (info->flags & SV_LOTSOFADDONS)
			{
				cl_mode = CL_ASKFULLFILELIST;
				cl_lastcheckedfilecount = 0;
				return true;
			}

			cl_mode = CL_CHECKFILES;
		}
		else
		{
			cl_mode = CL_ASKJOIN; // files need not be checked for the server.
			*asksent = 0;
		}

		return true;
	}

	// Ask the info to the server (askinfo packet)
	if (*asksent + NEWTICRATE < I_GetTime())
	{
		SendAskInfo(servernode);
		*asksent = I_GetTime();
	}

	return true;
}

static void HandleHTTPDownloadFail(void)
{
	char filename[MAX_WADPATH];

	CONS_Alert(CONS_WARNING, M_GetText("One or more addons failed to download:\n"));

	for (int i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].failed == FDOWNLOAD_FAIL_NONE)
			continue;

		strlcpy(filename, fileneeded[i].filename, sizeof filename);
		nameonly(filename);

		CONS_Printf(" * \"%s\" (%s)", filename, GetPrintableFileSize(fileneeded[i].totalsize));

		if (fileneeded[i].failed == FDOWNLOAD_FAIL_NOTFOUND)
			CONS_Printf(M_GetText(" not found, md5: "));
		else if (fileneeded[i].failed == FDOWNLOAD_FAIL_MD5SUMBAD)
			CONS_Printf(M_GetText(" wrong version, md5: "));
		else
			CONS_Printf(M_GetText(" other error, md5: "));

		char md5tmp[33];
		for (INT32 j = 0; j < 16; j++)
			snprintf(&md5tmp[j*2], sizeof(md5tmp), "%02x", fileneeded[i].md5sum[j]);
		CONS_Printf("%s\n", md5tmp);

		fileneeded[i].failed = FDOWNLOAD_FAIL_NONE;
	}

	CONS_Printf(M_GetText("Falling back to direct downloader.\n"));

	cl_mode = CL_CHECKFILES;
}

/** Called by CL_ConnectToServer
  *
  * \param tmpsave The name of the gamestate file???
  * \param oldtic Used for knowing when to poll events and redraw
  * \param asksent ???
  * \return False if the connection was aborted
  * \sa CL_ServerConnectionSearchTicker
  * \sa CL_ConnectToServer
  *
  */
static boolean CL_ServerConnectionTicker(const char *tmpsave, tic_t *oldtic, tic_t *asksent)
{
	boolean waitmore;

	switch (cl_mode)
	{
		case CL_SEARCHING:
			if (!CL_ServerConnectionSearchTicker(asksent))
				return false;
			break;

		case CL_ASKFULLFILELIST:
			if (cl_lastcheckedfilecount == UINT16_MAX) // All files retrieved
				cl_mode = CL_CHECKFILES;
			else if (fileneedednum != cl_lastcheckedfilecount || I_GetTime() >= *asksent)
			{
				if (CL_AskFileList(fileneedednum))
				{
					cl_lastcheckedfilecount = fileneedednum;
					*asksent = I_GetTime() + NEWTICRATE;
				}
			}
			break;
		case CL_CHECKFILES:
			if (!CL_FinishedFileList())
				return false;
			break;

		case CL_DOWNLOADHTTPFILES:
			waitmore = false;
			for (int i = filedownload.current; i < fileneedednum; i++)
			{
				if (IsFileDownloadable(&fileneeded[i]))
				{
					if (!filedownload.http_running)
					{
						if (!CURLPrepareFile(filedownload.http_source, i))
							HandleHTTPDownloadFail();
					}
					waitmore = true;
					break;
				}
			}

			// Rusty TODO: multithread
			if (filedownload.http_running)
				CURLGetFile();

			if (waitmore)
				break; // exit the case

			// Done downloading files
			if (!filedownload.remaining)
			{
				if (filedownload.http_failed)
					HandleHTTPDownloadFail();
				else
					DoLoadFiles();
			}
			break;
		case CL_DOWNLOADFILES:
			// Done downloading files
			if (!filedownload.remaining)
				DoLoadFiles();
			break;
		case CL_LOADFILES:
			if (CL_LoadServerFiles())
			{
				FreeFileNeeded();
				*asksent = 0; // This ensures the first join request is right away
				firstconnectattempttime = I_GetTime();
				cl_mode = CL_ASKJOIN;
			}
			break;
		case CL_ASKJOIN:
			if (firstconnectattempttime + NEWTICRATE*300 < I_GetTime() && !server)
			{
				CONS_Printf(M_GetText("5 minute wait time exceeded.\n"));
				AbortConnection();
				M_StartMessage(M_GetText(
					"5 minute wait time exceeded.\n"
					"You may retry connection.\n"
					"\n"
					"Press ESC\n"
				), NULL, MM_NOTHING);
				return false;
			}

			// Prepare structures to save the file
			CL_PrepareDownloadSaveGame(tmpsave);

			if (I_GetTime() >= *asksent && CL_SendJoin())
			{
				*asksent = I_GetTime() + NEWTICRATE*3;
				cl_mode = CL_WAITJOINRESPONSE;
			}
			break;
		case CL_WAITJOINRESPONSE:
			if (I_GetTime() >= *asksent)
			{
				cl_mode = CL_ASKJOIN;
			}
			break;
		case CL_DOWNLOADSAVEGAME:
			// At this state, the first (and only) needed file is the gamestate
			if (fileneeded[0].status == FS_FOUND)
			{
				// Gamestate is now handled within CL_LoadReceivedSavegame()
				CL_LoadReceivedSavegame(false);
				cl_mode = CL_CONNECTED;
			} // don't break case continue to CL_CONNECTED
			else
				break;

		case CL_CONNECTED:
		case CL_CONFIRMCONNECT: //logic is handled by M_ConfirmConnect
		default:
			break;

		// Connection closed by cancel, timeout or refusal.
		case CL_ABORTED:
			cl_mode = CL_SEARCHING;
			return false;
	}

	GetPackets();
	Net_AckTicker();

	// Call it only once by tic
	if (*oldtic != I_GetTime())
	{
		I_OsPolling();

		if (cl_mode == CL_CONFIRMCONNECT)
			D_ProcessEvents(); //needed for menu system to receive inputs
		else
		{
			// my hand has been forced and I am dearly sorry for this awful hack :vomit:
			for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
			{
				if (!Snake_JoyGrabber(snake, &events[eventtail]))
					G_MapEventsToControls(&events[eventtail]);
			}
		}

		if (gamekeydown[KEY_ESCAPE] || gamekeydown[KEY_JOY1+1] || cl_mode == CL_ABORTED)
		{
			CONS_Printf(M_GetText("Network game synchronization aborted.\n"));
			M_StartMessage(M_GetText("Network game synchronization aborted.\n\nPress ESC\n"), NULL, MM_NOTHING);

			AbortConnection();

			memset(gamekeydown, 0, NUMKEYS);
			return false;
		}
		else if ((cl_mode == CL_DOWNLOADFILES || cl_mode == CL_DOWNLOADHTTPFILES) && snake)
			Snake_Update(snake);

		if (client && (cl_mode == CL_DOWNLOADFILES || cl_mode == CL_DOWNLOADSAVEGAME))
			FileReceiveTicker();

		*oldtic = I_GetTime();

		if (client && cl_mode != CL_CONNECTED && cl_mode != CL_ABORTED)
		{
			if (!snake)
			{
				F_MenuPresTicker(); // title sky
				F_TitleScreenTicker(true);
				F_TitleScreenDrawer();
			}
			CL_DrawConnectionStatus();
#ifdef HAVE_THREADS
			I_lock_mutex(&m_menu_mutex);
#endif
			M_Drawer(); //Needed for drawing messageboxes on the connection screen
#ifdef HAVE_THREADS
			I_unlock_mutex(m_menu_mutex);
#endif
			I_UpdateNoVsync(); // page flip or blit buffer
			if (moviemode)
				M_SaveFrame();
			S_UpdateSounds();
			S_UpdateClosedCaptions();
		}
	}
	else
	{
		I_Sleep(cv_sleep.value);
		I_UpdateTime(cv_timescale.value);
	}

	return true;
}

#define TMPSAVENAME "$$$.sav"

void CL_ConnectToServer(void)
{
	INT32 pnumnodes, nodewaited = doomcom->numnodes, i;
	tic_t oldtic;
	tic_t asksent;
	char tmpsave[256];

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	filedownload.current = -1;

	cl_mode = CL_SEARCHING;

	// Don't get a corrupt savegame error because tmpsave already exists
	if (FIL_FileExists(tmpsave) && unlink(tmpsave) == -1)
		I_Error("Can't delete %s\n", tmpsave);

	if (netgame)
	{
		if (servernode < 0 || servernode >= MAXNETNODES)
			CONS_Printf(M_GetText("Searching for a server...\n"));
		else
			CONS_Printf(M_GetText("Contacting the server...\n"));
	}

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // clean up intermission graphics etc

	DEBFILE(va("waiting %d nodes\n", doomcom->numnodes));
	G_SetGamestate(GS_WAITINGPLAYERS);
	wipegamestate = GS_WAITINGPLAYERS;

	ClearAdminPlayers();
	pnumnodes = 1;
	oldtic = I_GetTime() - 1;

	asksent = (tic_t) - TICRATE;
	firstconnectattempttime = I_GetTime();

	i = SL_SearchServer(servernode);

	if (i != -1)
	{
		char *gametypestr = serverlist[i].info.gametypename;
		CONS_Printf(M_GetText("Connecting to: %s\n"), serverlist[i].info.servername);
		gametypestr[sizeof serverlist[i].info.gametypename - 1] = '\0';
		CONS_Printf(M_GetText("Gametype: %s\n"), gametypestr);
		CONS_Printf(M_GetText("Version: %d.%d.%u\n"), serverlist[i].info.version/100,
		 serverlist[i].info.version%100, serverlist[i].info.subversion);
	}
	SL_ClearServerList(servernode);

	do
	{
		// If the connection was aborted for some reason, leave
		if (!CL_ServerConnectionTicker(tmpsave, &oldtic, &asksent))
			return;

		if (server)
		{
			pnumnodes = 0;
			for (i = 0; i < MAXNETNODES; i++)
				if (netnodes[i].ingame)
					pnumnodes++;
		}
	}
	while (!(cl_mode == CL_CONNECTED && (client || (server && nodewaited <= pnumnodes))));

	if (netgame)
		F_StartWaitingPlayers();

	DEBFILE(va("Synchronisation Finished\n"));

	displayplayer = consoleplayer;
}

/** Called when a PT_SERVERINFO packet is received
  *
  * \param node The packet sender
  * \note What happens if the packet comes from a client or something like that?
  *
  */
void PT_ServerInfo(SINT8 node)
{
	// compute ping in ms
	const tic_t ticnow = I_GetTime();
	const tic_t ticthen = (tic_t)LONG(netbuffer->u.serverinfo.time);
	const tic_t ticdiff = (ticnow - ticthen)*1000/NEWTICRATE;
	netbuffer->u.serverinfo.time = (tic_t)LONG(ticdiff);
	netbuffer->u.serverinfo.servername[MAXSERVERNAME-1] = 0;
	netbuffer->u.serverinfo.application
		[sizeof netbuffer->u.serverinfo.application - 1] = '\0';
	netbuffer->u.serverinfo.gametypename
		[sizeof netbuffer->u.serverinfo.gametypename - 1] = '\0';

	SL_InsertServer(&netbuffer->u.serverinfo, node);
}

// Helper function for packets that should only be sent by the server
// If it is NOT from the server, bail out and close the connection!
static boolean ServerOnly(SINT8 node)
{
	if (node == servernode)
		return false;

	Net_CloseConnection(node);
	return true;
}

void PT_MoreFilesNeeded(SINT8 node)
{
	if (server && serverrunning)
	{ // But wait I thought I'm the server?
		Net_CloseConnection(node);
		return;
	}
	if (ServerOnly(node))
		return;
	if (cl_mode == CL_ASKFULLFILELIST && netbuffer->u.filesneededcfg.first == fileneedednum)
	{
		D_ParseFileneeded(netbuffer->u.filesneededcfg.num, netbuffer->u.filesneededcfg.files, netbuffer->u.filesneededcfg.first);
		if (!netbuffer->u.filesneededcfg.more)
			cl_lastcheckedfilecount = UINT16_MAX; // Got the whole file list
	}
}

// Negative response of client join request
void PT_ServerRefuse(SINT8 node)
{
	if (server && serverrunning)
	{ // But wait I thought I'm the server?
		Net_CloseConnection(node);
		return;
	}
	if (ServerOnly(node))
		return;
	if (cl_mode == CL_WAITJOINRESPONSE)
	{
		// Save the reason so it can be displayed after quitting the netgame
		char *reason = strdup(netbuffer->u.serverrefuse.reason);
		if (!reason)
			I_Error("Out of memory!\n");

		if (strstr(reason, "Maximum players reached"))
		{
			serverisfull = true;
			//Special timeout for when refusing due to player cap. The client will wait 3 seconds between join requests when waiting for a slot, so we need this to be much longer
			//We set it back to the value of cv_nettimeout.value in CL_Reset
			connectiontimeout = NEWTICRATE*7;
			cl_mode = CL_ASKJOIN;
			free(reason);
			return;
		}

		M_StartMessage(va(M_GetText("Server refuses connection\n\nReason:\n%s"),
			reason), NULL, MM_NOTHING);

		AbortConnection();

		free(reason);
	}
}

// Positive response of client join request
void PT_ServerCFG(SINT8 node)
{
	if (server && serverrunning && node != servernode)
	{ // but wait I thought I'm the server?
		Net_CloseConnection(node);
		return;
	}
	if (ServerOnly(node))
		return;
	/// \note how would this happen? and is it doing the right thing if it does?
	if (cl_mode != CL_WAITJOINRESPONSE)
		return;

	if (client)
	{
		maketic = gametic = neededtic = (tic_t)LONG(netbuffer->u.servercfg.gametic);
		G_SetGametype(netbuffer->u.servercfg.gametype);
		modifiedgame = netbuffer->u.servercfg.modifiedgame;
		if (netbuffer->u.servercfg.usedCheats)
			G_SetUsedCheats(true);
		memcpy(server_context, netbuffer->u.servercfg.server_context, 8);
	}

	netnodes[(UINT8)servernode].ingame = true;
	serverplayer = netbuffer->u.servercfg.serverplayer;
	doomcom->numslots = SHORT(netbuffer->u.servercfg.totalslotnum);
	mynode = netbuffer->u.servercfg.clientnode;
	if (serverplayer >= 0)
		playernode[(UINT8)serverplayer] = servernode;

	if (netgame)
		CONS_Printf(M_GetText("Join accepted, waiting for complete game state...\n"));
	DEBFILE(va("Server accept join gametic=%u mynode=%d\n", gametic, mynode));

	/// \note Wait. What if a Lua script uses some global custom variables synched with the NetVars hook?
	///       Shouldn't they be downloaded even at intermission time?
	///       Also, according to PT_ClientJoin, the server will send the savegame even during intermission...
	if (netbuffer->u.servercfg.gamestate == GS_LEVEL/* ||
		netbuffer->u.servercfg.gamestate == GS_INTERMISSION*/)
		cl_mode = CL_DOWNLOADSAVEGAME;
	else
		cl_mode = CL_CONNECTED;
}
