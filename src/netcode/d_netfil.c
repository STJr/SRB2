// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netfil.c
/// \brief Transfer a file using HSendPacket.

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include <stdio.h>
#include <sys/stat.h>

#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#include <limits.h>
#elif defined (_WIN32)
#include <sys/utime.h>
#endif

#include "../doomdef.h"
#include "../doomstat.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../i_time.h"
#include "i_net.h"
#include "../i_system.h"
#include "../m_argv.h"
#include "d_net.h"
#include "../w_wad.h"
#include "d_netfil.h"
#include "net_command.h"
#include "../z_zone.h"
#include "../byteptr.h"
#include "../p_setup.h"
#include "../m_misc.h"
#include "../m_menu.h"
#include "../md5.h"
#include "../filesrch.h"

#include <errno.h>

// Prototypes
static boolean AddFileToSendQueue(INT32 node, UINT8 fileid);

// Sender structure
typedef struct filetx_s
{
	INT32 ram;
	union {
		char *filename; // Name of the file
		char *ram; // Pointer to the data in RAM
	} id;
	UINT32 size; // Size of the file
	UINT8 fileid;
	INT32 node; // Destination
	struct filetx_s *next; // Next file in the list
} filetx_t;

// Current transfers (one for each node)
typedef struct filetran_s
{
	filetx_t *txlist; // Linked list of all files for the node
	UINT8 iteration;
	UINT8 ackediteration;
	UINT32 position; // The current position in the file
	boolean *ackedfragments;
	UINT32 ackedsize;
	FILE *currentfile; // The file currently being sent/received
	tic_t dontsenduntil;
} filetran_t;
static filetran_t transfer[MAXNETNODES];

// Read time of file: stat _stmtime
// Write time of file: utime

// Receiver structure
INT32 fileneedednum; // Number of files needed to join the server
fileneeded_t *fileneeded; // List of needed files
static tic_t lasttimeackpacketsent = 0;
char downloaddir[512] = "DOWNLOAD";

// For resuming failed downloads
typedef struct
{
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	boolean *receivedfragments;
	UINT32 fragmentsize;
	UINT32 currentsize;
} pauseddownload_t;
static pauseddownload_t *pauseddownload = NULL;

file_download_t filedownload;

static CURL *http_handle;
static CURLM *multi_handle;
static UINT32 curl_dlnow;
static UINT32 curl_dltotal;
static time_t curl_starttime;
static int curl_runninghandles = 0;
static UINT32 curl_origfilesize;
static UINT32 curl_origtotalfilesize;
static char *curl_realname = NULL;
static fileneeded_t *curl_curfile = NULL;
HTTP_login *curl_logins;

luafiletransfer_t *luafiletransfers = NULL;
boolean waitingforluafiletransfer = false;
boolean waitingforluafilecommand = false;
char luafiledir[256 + 16] = "luafiles";

// max file size to send to a player (in kilobytes)
static CV_PossibleValue_t maxsend_cons_t[] = {{0, "MIN"}, {204800, "MAX"}, {0, NULL}};
consvar_t cv_maxsend = CVAR_INIT ("maxsend", "4096", CV_SAVE|CV_NETVAR, maxsend_cons_t, NULL);

consvar_t cv_noticedownload = CVAR_INIT ("noticedownload", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);

// Speed of file downloading (in packets per tic)
static CV_PossibleValue_t downloadspeed_cons_t[] = {{1, "MIN"}, {300, "MAX"}, {0, NULL}};
consvar_t cv_downloadspeed = CVAR_INIT ("downloadspeed", "16", CV_SAVE|CV_NETVAR, downloadspeed_cons_t, NULL);

static UINT16 GetWadNumFromFileNeededId(UINT8 id)
{
	for (UINT16 wadnum = mainwads; wadnum < numwadfiles; wadnum++)
	{
		if (!wadfiles[wadnum]->important)
			continue;
		if (id == 0)
			return wadnum;
		id--;
	}

	return UINT16_MAX;
}

enum
{
	WILLSEND_YES,
	WILLSEND_NO,
	WILLSEND_TOOLARGE
};

/** Fills a serverinfo packet with information about wad files loaded.
  *
  * \todo Give this function a better name since it is in global scope.
  * Used to have size limiting built in - now handled via W_InitFile in w_wad.c
  *
  */
UINT8 *PutFileNeeded(UINT16 firstfile)
{
	UINT8 count = 0;
	UINT8 *p_start = netbuffer->packettype == PT_MOREFILESNEEDED ? netbuffer->u.filesneededcfg.files : netbuffer->u.serverinfo.fileneeded;
	UINT8 *p = p_start;
	char wadfilename[MAX_WADPATH] = "";
	UINT8 filestatus, folder;

	for (size_t i = mainwads; i < numwadfiles; i++) //mainwads, otherwise we start on the first mainwad
	{
		// If it has only music/sound lumps, don't put it in the list
		if (!wadfiles[i]->important)
			continue;

		if (firstfile)
		{ // Skip files until we reach the first file.
			firstfile--;
			continue;
		}

		nameonly(strcpy(wadfilename, wadfiles[i]->filename));

		// Look below at the WRITE macros to understand what these numbers mean.
		if (p + 1 + 4 + min(strlen(wadfilename) + 1, MAX_WADPATH) + 16 > p_start + MAXFILENEEDED)
		{
			// Too many files to send all at once
			if (netbuffer->packettype == PT_MOREFILESNEEDED)
				netbuffer->u.filesneededcfg.more = 1;
			else
				netbuffer->u.serverinfo.flags |= SV_LOTSOFADDONS;
			break;
		}

		filestatus = 1; // Importance - not really used any more, holds 1 by default for backwards compat with MS
		folder = wadfiles[i]->type == RET_FOLDER;

		if (!folder)
		{
			// Store in the upper four bits
			if (!cv_downloading.value)
				filestatus += (WILLSEND_NO << 4); // Won't send
			else if (wadfiles[i]->filesize <= (UINT32)cv_maxsend.value * 1024)
				filestatus += (WILLSEND_YES << 4); // Will send if requested
			// else
				// filestatus += (0 << 4); -- Won't send, too big
		}

		WRITEUINT8(p, filestatus);
		WRITEUINT8(p, folder);

		count++;
		WRITEUINT32(p, wadfiles[i]->filesize);
		WRITESTRINGN(p, wadfilename, MAX_WADPATH);
		WRITEMEM(p, wadfiles[i]->md5sum, 16);
	}

	if (netbuffer->packettype == PT_MOREFILESNEEDED)
		netbuffer->u.filesneededcfg.num = count;
	else
		netbuffer->u.serverinfo.fileneedednum = count;

	return p;
}

void AllocFileNeeded(INT32 size)
{
	if (fileneeded == NULL)
		fileneeded = Z_Calloc(sizeof(fileneeded_t) * size, PU_STATIC, NULL);
	else
		fileneeded = Z_Realloc(fileneeded, sizeof(fileneeded_t) * size, PU_STATIC, NULL);
}

void FreeFileNeeded(void)
{
	Z_Free(fileneeded);
	fileneeded = NULL;
}

/** Parses the serverinfo packet and fills the fileneeded table on client
  *
  * \param fileneedednum_parm The number of files needed to join the server
  * \param fileneededstr The memory block containing the list of needed files
  *
  */
void D_ParseFileneeded(INT32 fileneedednum_parm, UINT8 *fileneededstr, UINT16 firstfile)
{
	UINT8 *p;
	UINT8 filestatus;

	fileneedednum = firstfile + fileneedednum_parm;
	p = (UINT8 *)fileneededstr;

	AllocFileNeeded(fileneedednum);

	for (INT32 i = firstfile; i < fileneedednum; i++)
	{
		fileneeded[i].type = FILENEEDED_WAD;
		fileneeded[i].status = FS_NOTCHECKED; // We haven't even started looking for the file yet
		fileneeded[i].justdownloaded = false;
		filestatus = READUINT8(p); // The first byte is the file status
		fileneeded[i].folder = READUINT8(p); // The second byte is the folder flag
		fileneeded[i].willsend = (UINT8)(filestatus >> 4);
		fileneeded[i].totalsize = READUINT32(p); // The four next bytes are the file size
		fileneeded[i].file = NULL; // The file isn't open yet
		fileneeded[i].failed = FDOWNLOAD_FAIL_NONE;
		READSTRINGN(p, fileneeded[i].filename, MAX_WADPATH); // The next bytes are the file name
		READMEM(p, fileneeded[i].md5sum, 16); // The last 16 bytes are the file checksum
	}
}

void CL_PrepareDownloadSaveGame(const char *tmpsave)
{
	filedownload.current = -1;

	FreeFileNeeded();
	AllocFileNeeded(1);

	fileneedednum = 1;
	fileneeded[0].type = FILENEEDED_SAVEGAME;
	fileneeded[0].status = FS_REQUESTED;
	fileneeded[0].justdownloaded = false;
	fileneeded[0].totalsize = UINT32_MAX;
	fileneeded[0].file = NULL;
	memset(fileneeded[0].md5sum, 0, 16);
	strcpy(fileneeded[0].filename, tmpsave);
}

/** Checks the server to see if we CAN download all the files,
  * before starting to create them and requesting.
  *
  * \param direct Game will do a direct download
  * \return True if we can download all the files
  *
  */
UINT8 CL_CheckDownloadable(boolean direct)
{
	UINT8 dlstatus = DLSTATUS_OK;

	for (UINT8 i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN)
		{
			if (fileneeded[i].folder)
			{
				dlstatus = DLSTATUS_FOLDER;
				break;
			}

			if (!direct || fileneeded[i].willsend == WILLSEND_YES)
				continue;

			if (fileneeded[i].willsend == WILLSEND_TOOLARGE)
				dlstatus = DLSTATUS_TOOLARGE;
			else //if (fileneeded[i].willsend == WILLSEND_NO)
				dlstatus = DLSTATUS_WONTSEND;
		}

	// Downloading locally disabled
	if (direct && !dlstatus && M_CheckParm("-nodownload"))
		dlstatus = DLSTATUS_NODOWNLOAD;

	return dlstatus;
}

/** Returns true if a needed file transfer can be resumed
  *
  * \param file The needed file to resume the transfer for
  * \return True if the transfer can be resumed
  *
  */
static boolean CL_CanResumeDownload(fileneeded_t *file)
{
	return pauseddownload
		&& !strcmp(pauseddownload->filename, file->filename) // Same name
		&& !memcmp(pauseddownload->md5sum, file->md5sum, 16) // Same checksum
		&& pauseddownload->fragmentsize == file->fragmentsize; // Same fragment size
}

void CL_AbortDownloadResume(void)
{
	if (!pauseddownload)
		return;

	free(pauseddownload->receivedfragments);
	remove(pauseddownload->filename);
	free(pauseddownload);
	pauseddownload = NULL;
}

/** Sends requests for files in the ::fileneeded table with a status of
  * ::FS_NOTFOUND.
  *
  * \return True if the packet was successfully sent
  * \note Sends a PT_REQUESTFILE packet
  *
  */
boolean CL_SendFileRequest(void)
{
	char *p;
	INT64 totalfreespaceneeded = 0, availablefreespace;

#ifdef PARANOIA
	if (M_CheckParm("-nodownload"))
	{
		CONS_Printf("Attempted to download files in -nodownload mode");
		return false;
	}
#endif

	for (INT32 i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN
			&& (fileneeded[i].willsend == WILLSEND_TOOLARGE || fileneeded[i].willsend == WILLSEND_NO || fileneeded[i].folder))
		{
			CONS_Printf("Attempted to download files that were not sendable");
			return false;
		}

		if (fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD)
		{
			// Error check for the first time around.
			totalfreespaceneeded += fileneeded[i].totalsize;
		}
	}

	I_GetDiskFreeSpace(&availablefreespace);
	if (totalfreespaceneeded > availablefreespace)
	{
		CONS_Printf("To play on this server you must download %s KB,\n"
			"but you have only %s KB free space on your device\n",
			sizeu1((size_t)(totalfreespaceneeded>>10)), sizeu2((size_t)(availablefreespace>>10)));
		return false;
	}

	netbuffer->packettype = PT_REQUESTFILE;
	p = (char *)netbuffer->u.textcmd;
	for (INT32 i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD)
		{
			WRITEUINT8(p, i); // fileid

			// put it in download dir
			nameonly(fileneeded[i].filename);
			strcatbf(fileneeded[i].filename, downloaddir, "/");

			fileneeded[i].status = FS_REQUESTED;
		}

	WRITEUINT8(p, 0xFF);

	if (!HSendPacket(servernode, true, 0, p - (char *)netbuffer->u.textcmd))
	{
		CONS_Printf("Could not send download request packet to server\n");
		return false;
	}

	// prepare to download
	I_mkdir(downloaddir, 0755);

	return true;
}

// get request filepak and put it on the send queue
void PT_RequestFile(SINT8 node)
{
	UINT8 *p = netbuffer->u.textcmd;

	if (client || !cv_downloading.value)
	{
		Net_CloseConnection(node); // close connection if you are not the server or disabled downloading
		return;
	}

	while (p < netbuffer->u.textcmd + MAXTEXTCMD-1) // Don't allow hacked client to overflow
	{
		UINT8 id = READUINT8(p);
		if (id == 0xFF)
			break;

		if (!AddFileToSendQueue(node, id))
		{
			SV_AbortSendFiles(node);
			Net_CloseConnection(node); // close connection if one of the requested files could not be sent
			return; // don't read the rest of the files
		}
	}

	return; // no problems with any files
}

/** Checks if the files needed aren't already loaded or on the disk
  *
  * \return 0 if some files are missing
  *         1 if all files exist
  *         2 if some already loaded files are not requested or are in a different order
  *         3 too many files, over WADLIMIT
  *         4 still checking, continuing next tic
  *
  */
INT32 CL_CheckFiles(void)
{
	INT32 i, j;
	char wadfilename[MAX_WADPATH];
	size_t filestoload = 0;
	boolean downloadrequired = false;

	// Modified game handling -- check for an identical file list
	// must be identical in files loaded AND in order
	// Return 2 on failure -- disconnect from server
	if (modifiedgame)
	{
		CONS_Debug(DBG_NETPLAY, "game is modified; only doing basic checks\n");
		for (i = 0, j = mainwads; i < fileneedednum || j < numwadfiles;)
		{
			if (j < numwadfiles && !wadfiles[j]->important)
			{
				// Unimportant on our side.
				++j;
				continue;
			}

			// If this test is true, we've reached the end of one file list
			// and the other still has a file that's important
			if (i >= fileneedednum || j >= numwadfiles)
				return 2;

			// For the sake of speed, only bother with a md5 check
			if (memcmp(wadfiles[j]->md5sum, fileneeded[i].md5sum, 16))
				return 2;

			// It's accounted for! let's keep going.
			CONS_Debug(DBG_NETPLAY, "'%s' accounted for\n", fileneeded[i].filename);
			fileneeded[i].status = FS_OPEN;
			++i;
			++j;
		}
		return 1;
	}

	for (i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD || fileneeded[i].status == FS_FALLBACK)
			downloadrequired = true;

		if (fileneeded[i].status != FS_OPEN)
			filestoload++;

		if (fileneeded[i].status != FS_NOTCHECKED) //since we're running this over multiple tics now, its possible for us to come across files checked in previous tics
			continue;

		CONS_Debug(DBG_NETPLAY, "searching for '%s' ", fileneeded[i].filename);

		if (fileneeded[i].folder)
		{
			fileneeded[i].status = FS_NOTFOUND;
		}
		else
		{
			// Check in already loaded files
			for (j = mainwads; j < numwadfiles; j++)
			{
				nameonly(strcpy(wadfilename, wadfiles[j]->filename));
				if (!stricmp(wadfilename, fileneeded[i].filename) &&
					!memcmp(wadfiles[j]->md5sum, fileneeded[i].md5sum, 16))
				{
					CONS_Debug(DBG_NETPLAY, "already loaded\n");
					fileneeded[i].status = FS_OPEN;
					return 4;
				}
			}

			fileneeded[i].status = findfile(fileneeded[i].filename, fileneeded[i].md5sum, true);
		}

		CONS_Debug(DBG_NETPLAY, "found %d\n", fileneeded[i].status);
		return 4;
	}

	//now making it here means we've checked the entire list and no FS_NOTCHECKED files remain
	if (numwadfiles+filestoload > MAX_WADFILES)
		return 3;
	else if (downloadrequired)
		return 0; //some stuff is FS_NOTFOUND, needs download
	else
		return 1; //everything is FS_OPEN or FS_FOUND, proceed to loading
}

// Load it now
boolean CL_LoadServerFiles(void)
{
	for (INT32 i = 0; i < fileneedednum; i++)
	{
		if (fileneeded[i].status == FS_OPEN)
			continue; // Already loaded
		else if (fileneeded[i].status == FS_FOUND)
		{
			if (fileneeded[i].folder)
				P_AddFolder(fileneeded[i].filename);
			else
				P_AddWadFile(fileneeded[i].filename);
			G_SetGameModified(true);
			fileneeded[i].status = FS_OPEN;
			return false;
		}
		else if (fileneeded[i].status == FS_MD5SUMBAD)
			I_Error("Wrong version of file %s", fileneeded[i].filename);
		else
		{
			const char *s;
			switch(fileneeded[i].status)
			{
			case FS_NOTFOUND:
				s = "FS_NOTFOUND";
				break;
			case FS_REQUESTED:
				s = "FS_REQUESTED";
				break;
			case FS_DOWNLOADING:
				s = "FS_DOWNLOADING";
				break;
			default:
				s = "unknown";
				break;
			}
			I_Error("Try to load file \"%s\" with status of %d (%s)\n", fileneeded[i].filename,
				fileneeded[i].status, s);
		}
	}
	return true;
}

void AddLuaFileTransfer(const char *filename, const char *mode)
{
	luafiletransfer_t **prevnext; // A pointer to the "next" field of the last transfer in the list
	luafiletransfer_t *filetransfer;
	static INT32 id;

	// Find the last transfer in the list and set a pointer to its "next" field
	prevnext = &luafiletransfers;
	while (*prevnext)
		prevnext = &((*prevnext)->next);

	// Allocate file transfer information and append it to the transfer list
	filetransfer = malloc(sizeof(luafiletransfer_t));
	if (!filetransfer)
		I_Error("AddLuaFileTransfer: Out of memory\n");
	*prevnext = filetransfer;
	filetransfer->next = NULL;

	// Allocate the file name
	filetransfer->filename = strdup(filename);
	if (!filetransfer->filename)
		I_Error("AddLuaFileTransfer: Out of memory\n");

	// Create and allocate the real file name
	if (server)
		filetransfer->realfilename = strdup(va("%s" PATHSEP "%s",
												luafiledir, filename));
	else
		filetransfer->realfilename = strdup(va("%s" PATHSEP "client" PATHSEP "$$$%d%d.tmp",
												luafiledir, rand(), rand()));
	if (!filetransfer->realfilename)
		I_Error("AddLuaFileTransfer: Out of memory\n");

	strlcpy(filetransfer->mode, mode, sizeof(filetransfer->mode));

	// Only if there is no transfer already going on
	if (server && filetransfer == luafiletransfers)
		SV_PrepareSendLuaFile();
	else
		filetransfer->ongoing = false;

	// Store the callback so it can be called once everyone has the file
	filetransfer->id = id;
	StoreLuaFileCallback(id);
	id++;

	if (waitingforluafiletransfer)
	{
		waitingforluafiletransfer = false;
		CL_PrepareDownloadLuaFile();
	}
}

static void SV_PrepareSendLuaFileToNextNode(void)
{
	UINT8 success = 1;

    // Find a client to send the file to
	for (INT32 i = 1; i < MAXNETNODES; i++)
		if (luafiletransfers->nodestatus[i] == LFTNS_WAITING) // Node waiting
		{
			// Tell the client we're about to send them the file
			netbuffer->packettype = PT_SENDINGLUAFILE;
			if (!HSendPacket(i, true, 0, 0))
				I_Error("Failed to send a PT_SENDINGLUAFILE packet\n"); // !!! Todo: Handle failure a bit better lol

			luafiletransfers->nodestatus[i] = LFTNS_ASKED;
			luafiletransfers->nodetimeouts[i] = I_GetTime() + 30 * TICRATE;

			return;
		}

	// No client found, everyone has the file
	// Send a net command with 1 as its first byte to indicate the file could be opened
	SendNetXCmd(XD_LUAFILE, &success, 1);
}

void SV_PrepareSendLuaFile(void)
{
	char *binfilename;

	luafiletransfers->ongoing = true;

	// Set status to "waiting" for everyone
	for (INT32 i = 0; i < MAXNETNODES; i++)
		luafiletransfers->nodestatus[i] = (netnodes[i].ingame ? LFTNS_WAITING : LFTNS_NONE);

	if (FIL_ReadFileOK(luafiletransfers->realfilename))
	{
		// If opening in text mode, convert all newlines to LF
		if (!strchr(luafiletransfers->mode, 'b'))
		{
			binfilename = strdup(va("%s" PATHSEP "$$$%d%d.tmp",
				luafiledir, rand(), rand()));
			if (!binfilename)
				I_Error("SV_PrepareSendLuaFile: Out of memory\n");

			if (!FIL_ConvertTextFileToBinary(luafiletransfers->realfilename, binfilename))
				I_Error("SV_PrepareSendLuaFile: Failed to convert file newlines\n");

			// Use the temporary file instead
			free(luafiletransfers->realfilename);
			luafiletransfers->realfilename = binfilename;
		}

		SV_PrepareSendLuaFileToNextNode();
	}
	else
	{
		// Send a net command with 0 as its first byte to indicate the file couldn't be opened
		UINT8 success = 0;
		SendNetXCmd(XD_LUAFILE, &success, 1);
	}
}

void SV_HandleLuaFileSent(UINT8 node)
{
	luafiletransfers->nodestatus[node] = LFTNS_SENT;
	SV_PrepareSendLuaFileToNextNode();
}

void RemoveLuaFileTransfer(void)
{
	luafiletransfer_t *filetransfer = luafiletransfers;

	// If it was a temporary file, delete it
	if (server && !strchr(filetransfer->mode, 'b'))
		remove(filetransfer->realfilename);

	RemoveLuaFileCallback(filetransfer->id);

	luafiletransfers = filetransfer->next;

	free(filetransfer->filename);
	free(filetransfer->realfilename);
	free(filetransfer);
}

void RemoveAllLuaFileTransfers(void)
{
	while (luafiletransfers)
		RemoveLuaFileTransfer();
}

void SV_AbortLuaFileTransfer(INT32 node)
{
	if (luafiletransfers)
	{
		if (luafiletransfers->nodestatus[node] == LFTNS_ASKED
			|| luafiletransfers->nodestatus[node] == LFTNS_SENDING)
		{
			SV_PrepareSendLuaFileToNextNode();
		}
		luafiletransfers->nodestatus[node] = LFTNS_NONE;
	}
}

void CL_PrepareDownloadLuaFile(void)
{
	// If there is no transfer in the list, this normally means the server
	// called io.open before us, so we have to wait until we call it too
	if (!luafiletransfers)
	{
		waitingforluafiletransfer = true;
		return;
	}

	if (luafiletransfers->ongoing)
	{
		waitingforluafilecommand = true;
		return;
	}

	// Tell the server we are ready to receive the file
	netbuffer->packettype = PT_ASKLUAFILE;
	HSendPacket(servernode, true, 0, 0);

	FreeFileNeeded();
	AllocFileNeeded(1);

	fileneedednum = 1;
	fileneeded[0].type = FILENEEDED_LUAFILE;
	fileneeded[0].status = FS_REQUESTED;
	fileneeded[0].justdownloaded = false;
	fileneeded[0].totalsize = UINT32_MAX;
	fileneeded[0].file = NULL;
	memset(fileneeded[0].md5sum, 0, 16);
	strcpy(fileneeded[0].filename, luafiletransfers->realfilename);

	// Make sure all directories in the file path exist
	MakePathDirs(fileneeded[0].filename);

	luafiletransfers->ongoing = true;
}

// Number of files to send
// Little optimization to quickly test if there is a file in the queue
static INT32 filestosend = 0;

/** Adds a file to the file list for a node
  *
  * \param node The node to send the file to
  * \param filename The file to send
  * \param fileid The index of the file in the list of added files
  * \sa AddRamToSendQueue
  * \sa AddLuaFileToSendQueue
  *
  */
static boolean AddFileToSendQueue(INT32 node, UINT8 fileid)
{
	filetx_t **q; // A pointer to the "next" field of the last file in the list
	filetx_t *p; // The new file request
	UINT16 wadnum;

	// Find the last file in the list and set a pointer to its "next" field
	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);

	// Allocate a file request and append it to the file list
	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("AddFileToSendQueue: No more memory\n");

	// Initialise with zeros
	memset(p, 0, sizeof (filetx_t));

	// Allocate the file name
	p->id.filename = (char *)malloc(MAX_WADPATH);
	if (!p->id.filename)
		I_Error("AddFileToSendQueue: No more memory\n");

	// Find the wad the ID refers to
	wadnum = GetWadNumFromFileNeededId(fileid);

	// Handle non-loaded file requests
	if (wadnum == UINT16_MAX)
	{
		DEBFILE(va("fileneeded %d not found in wadfiles\n", fileid));
		// This formerly checked if (!findfile(p->id.filename, NULL, true))

		// Not found
		// Don't inform client
		DEBFILE(va("Client %d request fileneeded %d: not found\n", node, fileid));
		free(p->id.filename);
		free(p);
		*q = NULL;
		return false; // cancel the rest of the requests
	}

	// Set the file name and get rid of the path
	strlcpy(p->id.filename, wadfiles[wadnum]->filename, MAX_WADPATH);

	// Handle huge file requests (i.e. bigger than cv_maxsend.value KB)
	if (wadfiles[wadnum]->filesize > (UINT32)cv_maxsend.value * 1024)
	{
		// Too big
		// Don't inform client (client sucks, man)
		DEBFILE(va("Client %d request %s: file too big, not sending\n", node, p->id.filename));
		free(p->id.filename);
		free(p);
		*q = NULL;
		return false; // cancel the rest of the requests
	}

	if (cv_noticedownload.value)
		CONS_Printf("Sending file \"%s\" to node %d (%s)\n", p->id.filename, node, I_GetNodeAddress(node));

	DEBFILE(va("Sending file %s (id=%d) to %d\n", p->id.filename, fileid, node));
	p->ram = SF_FILE; // It's a file, we need to close it and free its name once we're done sending it
	p->fileid = fileid;
	p->next = NULL; // End of list
	filestosend++;
	return true;
}

/** Adds a memory block to the file list for a node
  *
  * \param node The node to send the memory block to
  * \param data The memory block to send
  * \param size The size of the block in bytes
  * \param freemethod How to free the block after it has been sent
  * \param fileid The index of the file in the list of added files
  * \sa AddFileToSendQueue
  * \sa AddLuaFileToSendQueue
  *
  */
void AddRamToSendQueue(INT32 node, void *data, size_t size, freemethod_t freemethod, UINT8 fileid)
{
	filetx_t **q; // A pointer to the "next" field of the last file in the list
	filetx_t *p; // The new file request

	// Find the last file in the list and set a pointer to its "next" field
	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);

	// Allocate a file request and append it to the file list
	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("AddRamToSendQueue: No more memory\n");

	// Initialise with zeros
	memset(p, 0, sizeof (filetx_t));

	p->ram = freemethod; // Remember how to free the memory block for when we're done sending it
	p->id.ram = data;
	p->size = (UINT32)size;
	p->fileid = fileid;
	p->next = NULL; // End of list

	DEBFILE(va("Sending ram %p(size:%u) to %d (id=%u)\n",p->id.ram,p->size,node,fileid));

	filestosend++;
}

/** Adds a file requested by Lua to the file list for a node
  *
  * \param node The node to send the file to
  * \param filename The file to send
  * \sa AddFileToSendQueue
  * \sa AddRamToSendQueue
  *
  */
boolean AddLuaFileToSendQueue(INT32 node, const char *filename)
{
	filetx_t **q; // A pointer to the "next" field of the last file in the list
	filetx_t *p; // The new file request

	luafiletransfers->nodestatus[node] = LFTNS_SENDING;

	// Find the last file in the list and set a pointer to its "next" field
	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);

	// Allocate a file request and append it to the file list
	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("AddLuaFileToSendQueue: No more memory\n");

	// Initialise with zeros
	memset(p, 0, sizeof (filetx_t));

	// Allocate the file name
	p->id.filename = (char *)malloc(MAX_WADPATH); // !!!
	if (!p->id.filename)
		I_Error("AddLuaFileToSendQueue: No more memory\n");

	// Set the file name and get rid of the path
	strlcpy(p->id.filename, filename, MAX_WADPATH); // !!!
	//nameonly(p->id.filename);

	DEBFILE(va("Sending Lua file %s to %d\n", filename, node));
	p->ram = SF_FILE; // It's a file, we need to close it and free its name once we're done sending it
	p->next = NULL; // End of list
	filestosend++;
	return true;
}

/** Stops sending a file for a node, and removes the file request from the list,
  * either because the file has been fully sent or because the node was disconnected
  *
  * \param node The destination
  *
  */
static void SV_EndFileSend(INT32 node)
{
	filetx_t *p = transfer[node].txlist;

	// Free the file request according to the freemethod
	// parameter used with AddFileToSendQueue/AddRamToSendQueue
	switch (p->ram)
	{
		case SF_FILE: // It's a file, close it and free its filename
			if (cv_noticedownload.value)
				CONS_Printf("Ending file transfer for node %d\n", node);
			if (transfer[node].currentfile)
				fclose(transfer[node].currentfile);
			free(p->id.filename);
			break;
		case SF_Z_RAM: // It's a memory block allocated with Z_Alloc or the likes, use Z_Free
			Z_Free(p->id.ram);
			break;
		case SF_RAM: // It's a memory block allocated with malloc, use free
			free(p->id.ram);
		case SF_NOFREERAM: // Nothing to free
			break;
	}

	// Remove the file request from the list
	transfer[node].txlist = p->next;
	free(p);

	// Indicate that the transmission is over
	transfer[node].currentfile = NULL;
	if (transfer[node].ackedfragments)
		free(transfer[node].ackedfragments);
	transfer[node].ackedfragments = NULL;

	filestosend--;
}

#define FILEFRAGMENTSIZE (software_MAXPACKETLENGTH - (FILETXHEADER + BASEPACKETSIZE))

/** Handles file transmission
  *
  */
void FileSendTicker(void)
{
	static INT32 currentnode = 0;
	filetx_pak *p;
	size_t fragmentsize;
	filetx_t *f;
	INT32 packetsent, ram, i, j;

	// If someone is taking too long to download, kick them with a timeout
	// to prevent blocking the rest of the server...
	if (luafiletransfers)
	{
		for (i = 1; i < MAXNETNODES; i++)
		{
			luafiletransfernodestatus_t status = luafiletransfers->nodestatus[i];

			if (status != LFTNS_NONE && status != LFTNS_WAITING && status != LFTNS_SENT
				&& I_GetTime() > luafiletransfers->nodetimeouts[i])
			{
				Net_ConnectionTimeout(i);
			}
		}
	}

	if (!filestosend) // No file to send
		return;

	packetsent = cv_downloadspeed.value;

	netbuffer->packettype = PT_FILEFRAGMENT;

	// (((sendbytes-nowsentbyte)*TICRATE)/(I_GetTime()-starttime)<(UINT32)net_bandwidth)
	while (packetsent-- && filestosend != 0)
	{
		for (i = currentnode, j = 0; j < MAXNETNODES;
			i = (i+1) % MAXNETNODES, j++)
		{
			if (transfer[i].txlist)
				break;
		}
		// no transfer to do
		if (j >= MAXNETNODES)
			I_Error("filestosend=%d but no file to send found\n", filestosend);

		currentnode = (i+1) % MAXNETNODES;
		f = transfer[i].txlist;
		ram = f->ram;

		// Open the file if it isn't open yet, or
		if (!transfer[i].currentfile)
		{
			if (!ram) // Sending a file
			{
				long filesize;

				transfer[i].currentfile =
					fopen(f->id.filename, "rb");

				if (!transfer[i].currentfile)
					I_Error("File %s does not exist",
						f->id.filename);

				fseek(transfer[i].currentfile, 0, SEEK_END);
				filesize = ftell(transfer[i].currentfile);

				// Nobody wants to transfer a file bigger
				// than 4GB!
				if (filesize >= LONG_MAX)
					I_Error("filesize of %s is too large", f->id.filename);
				if (filesize == -1)
					I_Error("Error getting filesize of %s", f->id.filename);

				f->size = (UINT32)filesize;
				fseek(transfer[i].currentfile, 0, SEEK_SET);
			}
			else // Sending RAM
				transfer[i].currentfile = (FILE *)1; // Set currentfile to a non-null value to indicate that it is open

			transfer[i].iteration = 1;
			transfer[i].ackediteration = 0;
			transfer[i].position = 0;
			transfer[i].ackedsize = 0;

			transfer[i].ackedfragments = calloc(f->size / FILEFRAGMENTSIZE + 1, sizeof(*transfer[i].ackedfragments));
			if (!transfer[i].ackedfragments)
				I_Error("FileSendTicker: No more memory\n");

			transfer[i].dontsenduntil = 0;
		}

		// If the client hasn't acknowledged any fragment from the previous iteration,
		// it is most likely because their acks haven't had enough time to reach the server
		// yet, due to latency. In that case, we wait a little to avoid useless resend.
		if (I_GetTime() < transfer[i].dontsenduntil)
			continue;

		// Find the first non-acknowledged fragment
		while (transfer[i].ackedfragments[transfer[i].position / FILEFRAGMENTSIZE])
		{
			transfer[i].position += FILEFRAGMENTSIZE;
			if (transfer[i].position >= f->size)
			{
				if (transfer[i].ackediteration < transfer[i].iteration)
					transfer[i].dontsenduntil = I_GetTime() + TICRATE / 2;

				transfer[i].position = 0;
				transfer[i].iteration++;
			}
		}

		// Build a packet containing a file fragment
		p = &netbuffer->u.filetxpak;
		fragmentsize = FILEFRAGMENTSIZE;
		if (f->size-transfer[i].position < fragmentsize)
			fragmentsize = f->size-transfer[i].position;
		if (ram)
			M_Memcpy(p->data, &f->id.ram[transfer[i].position], fragmentsize);
		else
		{
			fseek(transfer[i].currentfile, transfer[i].position, SEEK_SET);

			if (fread(p->data, 1, fragmentsize, transfer[i].currentfile) != fragmentsize)
				I_Error("FileSendTicker: can't read %s byte on %s at %d because %s", sizeu1(fragmentsize), f->id.filename, transfer[i].position, M_FileError(transfer[i].currentfile));
		}
		p->iteration = transfer[i].iteration;
		p->position = LONG(transfer[i].position);
		p->fileid = f->fileid;
		p->filesize = LONG(f->size);
		p->size = SHORT((UINT16)FILEFRAGMENTSIZE);

		// Send the packet
		if (HSendPacket(i, false, 0, FILETXHEADER + fragmentsize)) // Don't use the default acknowledgement system
		{ // Success
			transfer[i].position = (UINT32)(transfer[i].position + fragmentsize);
			if (transfer[i].position >= f->size)
			{
				if (transfer[i].ackediteration < transfer[i].iteration)
					transfer[i].dontsenduntil = I_GetTime() + TICRATE / 2;

				transfer[i].position = 0;
				transfer[i].iteration++;
			}
		}
		else
		{ // Not sent for some odd reason, retry at next call
			// Exit the while (can't send this one so why should i send the next?)
			break;
		}
	}
}

void PT_FileAck(SINT8 node)
{
	fileack_pak *packet = &netbuffer->u.fileack;
	filetran_t *trans = &transfer[node];

	if (client)
		return;

	// Wrong file id? Ignore it, it's probably a late packet
	if (!(trans->txlist && packet->fileid == trans->txlist->fileid))
		return;

	if (packet->numsegments * sizeof(*packet->segments) != doomcom->datalength - BASEPACKETSIZE - sizeof(*packet))
	{
		Net_CloseConnection(node);
		return;
	}

	if (packet->iteration > trans->ackediteration)
	{
		trans->ackediteration = packet->iteration;
		if (trans->ackediteration >= trans->iteration - 1)
			trans->dontsenduntil = 0;
	}

	for (INT32 i = 0; i < packet->numsegments; i++)
	{
		fileacksegment_t *segment = &packet->segments[i];

		for (INT32 j = 0; j < 32; j++)
			if (LONG(segment->acks) & (1 << j))
			{
				if (LONG(segment->start) * FILEFRAGMENTSIZE >= trans->txlist->size)
				{
					Net_CloseConnection(node);
					return;
				}

				if (!trans->ackedfragments[LONG(segment->start) + j])
				{
					trans->ackedfragments[LONG(segment->start) + j] = true;
					trans->ackedsize += FILEFRAGMENTSIZE;

					// If the last missing fragment was acked, finish!
					if (trans->ackedsize == trans->txlist->size)
					{
						SV_EndFileSend(node);
						return;
					}
				}
			}
	}
}

void PT_FileReceived(SINT8 node)
{
	filetx_t *trans = transfer[node].txlist;

	if (server && trans && netbuffer->u.filereceived == trans->fileid)
		SV_EndFileSend(node);
}

static void SendAckPacket(fileack_pak *packet, UINT8 fileid)
{
	size_t packetsize;

	packetsize = sizeof(*packet) + packet->numsegments * sizeof(*packet->segments);

	// Finalise the packet
	packet->fileid = fileid;
	for (INT32 i = 0; i < packet->numsegments; i++)
	{
		packet->segments[i].start = LONG(packet->segments[i].start);
		packet->segments[i].acks = LONG(packet->segments[i].acks);
	}

	// Send the packet
	netbuffer->packettype = PT_FILEACK;
	M_Memcpy(&netbuffer->u.fileack, packet, packetsize);
	HSendPacket(servernode, false, 0, packetsize);

	// Clear the packet
	memset(packet, 0, sizeof(*packet) + 512);
}

static void AddFragmentToAckPacket(fileack_pak *packet, UINT8 iteration, UINT32 fragmentpos, UINT8 fileid)
{
	fileacksegment_t *segment = &packet->segments[packet->numsegments - 1];

	packet->iteration = max(packet->iteration, iteration);

    if (packet->numsegments == 0
		|| fragmentpos < segment->start
		|| fragmentpos - segment->start >= 32)
	{
		// If the packet becomes too big, send it
		if ((packet->numsegments + 1) * sizeof(*segment) > 512)
			SendAckPacket(packet, fileid);

		packet->numsegments++;
		segment = &packet->segments[packet->numsegments - 1];
		segment->start = fragmentpos;
	}

	// Set the bit that represents the fragment
	segment->acks |= 1 << (fragmentpos - segment->start);
}

void FileReceiveTicker(void)
{
	for (INT32 i = 0; i < fileneedednum; i++)
	{
		fileneeded_t *file = &fileneeded[i];

		if (file->status == FS_DOWNLOADING)
		{
			if (lasttimeackpacketsent - I_GetTime() > TICRATE / 2)
				SendAckPacket(file->ackpacket, i);

			// When resuming a tranfer, start with telling
			// the server what parts we already received
			if (file->ackresendposition != UINT32_MAX && file->status == FS_DOWNLOADING)
			{
				// Acknowledge ~70 MB/s, whichs means the client sends ~18 KB/s
				for (INT32 j = 0; j < 2048; j++)
				{
					if (file->receivedfragments[file->ackresendposition])
						AddFragmentToAckPacket(file->ackpacket, file->iteration, file->ackresendposition, i);

					file->ackresendposition++;
					if (file->ackresendposition * file->fragmentsize >= file->totalsize)
					{
						file->ackresendposition = UINT32_MAX;
						break;
					}
				}
			}
		}
	}
}

static void OpenNewFileForDownload(fileneeded_t *file, const char *filename)
{
	file->file = fopen(filename, "wb");
	if (!file->file)
		I_Error("Can't create file %s: %s", filename, strerror(errno));

	file->currentsize = 0;
	file->totalsize = LONG(netbuffer->u.filetxpak.filesize);
	file->ackresendposition = UINT32_MAX; // Only used for resumed downloads

	file->receivedfragments = calloc(file->totalsize / file->fragmentsize + 1, sizeof(*file->receivedfragments));
	if (!file->receivedfragments)
		I_Error("FileSendTicker: No more memory\n");
}

void PT_FileFragment(SINT8 node, INT32 netconsole)
{
	if (netnodes[node].ingame)
	{
		// Only accept PT_FILEFRAGMENT from the server.
		if (node != servernode)
		{
			CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_FILEFRAGMENT", node);
			if (server)
				SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
			return;
		}
		if (server)
			return;
	}
	else if (server || node != servernode)
	{
		Net_CloseConnection(node);
		return;
	}

	INT32 filenum = netbuffer->u.filetxpak.fileid;
	fileneeded_t *file = &fileneeded[filenum];
	UINT32 fragmentpos = LONG(netbuffer->u.filetxpak.position);
	UINT16 fragmentsize = SHORT(netbuffer->u.filetxpak.size);
	UINT16 boundedfragmentsize = doomcom->datalength - BASEPACKETSIZE - sizeof(netbuffer->u.filetxpak);
	char *filename;

	if (!file)
		return;

	filename = va("%s", file->filename);
	nameonly(filename);

	if (!(strcmp(filename, "srb2.pk3")
		&& strcmp(filename, "zones.pk3")
		&& strcmp(filename, "player.dta")
		&& strcmp(filename, "patch.pk3")
		&& strcmp(filename, "music.dta")
		))
		I_Error("Tried to download \"%s\"", filename);

	if (filenum >= fileneedednum)
	{
		DEBFILE(va("fileframent not needed %d>%d\n", filenum, fileneedednum));
		//I_Error("Received an unneeded file fragment (file id received: %d, file id needed: %d)\n", filenum, fileneedednum);
		return;
	}

	if (file->status == FS_REQUESTED)
	{
		if (file->file)
			I_Error("PT_FileFragment: already open file\n");

		file->status = FS_DOWNLOADING;
		file->fragmentsize = fragmentsize;
		file->iteration = 0;

		file->ackpacket = calloc(1, sizeof(*file->ackpacket) + 512);
		if (!file->ackpacket)
			I_Error("FileSendTicker: No more memory\n");

		if (CL_CanResumeDownload(file))
		{
			file->file = fopen(file->filename, "r+b");
			if (!file->file)
			{
				CONS_Alert(CONS_ERROR, "Couldn't reopen file %s: %s\n", file->filename, strerror(errno));

				free(pauseddownload->receivedfragments);

				CONS_Printf("Restarting download of addon \"%s\"...\n", filename);

				OpenNewFileForDownload(file, file->filename);
			}
			else
			{
				CONS_Printf("Resuming download of addon \"%s\"...\n", filename);
				file->currentsize = pauseddownload->currentsize;
				file->receivedfragments = pauseddownload->receivedfragments;
				file->ackresendposition = 0;
			}

			free(pauseddownload);
			pauseddownload = NULL;
		}
		else
		{
			CL_AbortDownloadResume();
			OpenNewFileForDownload(file, file->filename);
			CONS_Printf("Downloading addon \"%s\" from the server...\n", filename);
		}

		lasttimeackpacketsent = I_GetTime();
	}

	if (file->status == FS_DOWNLOADING)
	{
		if (fragmentpos >= file->totalsize)
			I_Error("Invalid file fragment\n");

		file->iteration = max(file->iteration, netbuffer->u.filetxpak.iteration);

		if (!file->receivedfragments[fragmentpos / fragmentsize]) // Not received yet
		{
			file->receivedfragments[fragmentpos / fragmentsize] = true;

			// We can receive packets in the wrong order, anyway all OSes support gaped files
			fseek(file->file, fragmentpos, SEEK_SET);
			if (fragmentsize && fwrite(netbuffer->u.filetxpak.data, boundedfragmentsize, 1, file->file) != 1)
				I_Error("Can't write to %s: %s\n",file->filename, M_FileError(file->file));
			file->currentsize += boundedfragmentsize;

			AddFragmentToAckPacket(file->ackpacket, file->iteration, fragmentpos / fragmentsize, filenum);

			// Finished?
			if (file->currentsize == file->totalsize)
			{
				fclose(file->file);
				file->file = NULL;
				free(file->receivedfragments);
				free(file->ackpacket);
				file->status = FS_FOUND;
				file->justdownloaded = true;

				// Tell the server we have received the file
				netbuffer->packettype = PT_FILERECEIVED;
				netbuffer->u.filereceived = filenum;
				HSendPacket(servernode, true, 0, 1);

				if (luafiletransfers)
				{
					// Tell the server we have received the file
					netbuffer->packettype = PT_HASLUAFILE;
					HSendPacket(servernode, true, 0, 0);
					FreeFileNeeded();
				}
				else
				{
					filedownload.completednum++;
					filedownload.completedsize += file->totalsize;
					filedownload.remaining--;
				}

				CONS_Printf(M_GetText("Finished download of \"%s\"\n"), filename);
			}
		}
		else // Already received
		{
			// If they are sending us the fragment again, it's probably because
			// they missed our previous ack, so we must re-acknowledge it
			AddFragmentToAckPacket(file->ackpacket, file->iteration, fragmentpos / fragmentsize, filenum);
		}
	}
	else if (!file->justdownloaded)
	{
		const char *s;
		switch(file->status)
		{
		case FS_NOTFOUND:
			s = "FS_NOTFOUND";
			break;
		case FS_FOUND:
			s = "FS_FOUND";
			break;
		case FS_OPEN:
			s = "FS_OPEN";
			break;
		case FS_MD5SUMBAD:
			s = "FS_MD5SUMBAD";
			break;
		default:
			s = "unknown";
			break;
		}
		I_Error("Received a file not requested (file id: %d, file status: %s)\n", filenum, s);
	}

	filedownload.current = filenum;
}

/** \brief Checks if a node is downloading a file
 *
 * \param node The node to check for
 * \return True if the node is downloading a file
 *
 */
boolean SendingFile(INT32 node)
{
	return transfer[node].txlist != NULL;
}

/** Cancels all file requests for a node
  *
  * \param node The destination
  * \sa SV_EndFileSend
  *
  */
void SV_AbortSendFiles(INT32 node)
{
	while (transfer[node].txlist)
		SV_EndFileSend(node);
}

void CloseNetFile(void)
{
	// Is sending?
	for (INT32 i = 0; i < MAXNETNODES; i++)
		SV_AbortSendFiles(i);

	// Receiving a file?
	if (fileneeded)
	{
		for (INT32 i = 0; i < fileneedednum; i++)
			if (fileneeded[i].status == FS_DOWNLOADING && fileneeded[i].file)
			{
				fclose(fileneeded[i].file);
				free(fileneeded[i].ackpacket);

				if (!pauseddownload && (fileneeded[i].type == FILENEEDED_WAD || i != 0)) // 0 is the gamestate...
				{
					// Don't remove the file, save it for later in case we resume the download
					pauseddownload = malloc(sizeof(*pauseddownload));
					if (!pauseddownload)
						I_Error("CloseNetFile: No more memory\n");

					strcpy(pauseddownload->filename, fileneeded[i].filename);
					memcpy(pauseddownload->md5sum, fileneeded[i].md5sum, 16);
					pauseddownload->currentsize = fileneeded[i].currentsize;
					pauseddownload->receivedfragments = fileneeded[i].receivedfragments;
					pauseddownload->fragmentsize = fileneeded[i].fragmentsize;
				}
				else
				{
					// File is not complete, delete it.
					free(fileneeded[i].receivedfragments);
					remove(fileneeded[i].filename);
				}
			}
	}

	FreeFileNeeded();
}

void Command_Downloads_f(void)
{
	for (INT32 node = 0; node < MAXNETNODES; node++)
		if (transfer[node].txlist
		&& transfer[node].txlist->ram == SF_FILE) // Node is downloading a file?
		{
			const char *name = transfer[node].txlist->id.filename;
			UINT32 position = transfer[node].ackedsize;
			UINT32 size = transfer[node].txlist->size;
			char ratecolor;

			// Avoid division by zero errors
			if (!size)
				size = 1;

			name = &name[strlen(name) - nameonlylength(name)];
			switch (4 * (position - 1) / size)
			{
				case 0: ratecolor = '\x85'; break;
				case 1: ratecolor = '\x87'; break;
				case 2: ratecolor = '\x82'; break;
				case 3: ratecolor = '\x83'; break;
				default: ratecolor = '\x80';
			}

			CONS_Printf("%2d  %c%s  ", node, ratecolor, name); // Node and file name
			CONS_Printf("\x80%uK\x84/\x80%uK ", position / 1024, size / 1024); // Progress in kB
			CONS_Printf("\x80(%c%u%%\x80)  ", ratecolor, (UINT32)(100.0 * position / size)); // Progress in %
			CONS_Printf("%s\n", I_GetNodeAddress(node)); // Address and newline
		}
}

static size_t curlwrite_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

static int curlprogress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	(void)clientp;
	(void)ultotal;
	(void)ulnow; // Function prototype requires these but we won't use, so just discard
	curl_dlnow = (UINT32)dlnow;
	curl_dltotal = (UINT32)dltotal;
	getbytes = ((double)dlnow) / (time(NULL) - curl_starttime); // To-do: Make this more accurate???
	return 0;
}

boolean CURLPrepareFile(const char* url, int dfilenum)
{
	HTTP_login *login;

#ifdef PARANOIA
	if (M_CheckParm("-nodownload"))
		I_Error("Attempted to download files in -nodownload mode");
#endif

	curl_global_init(CURL_GLOBAL_ALL);

	http_handle = curl_easy_init();
	multi_handle = curl_multi_init();

	if (http_handle && multi_handle)
	{
		I_mkdir(downloaddir, 0755);

		curl_curfile = &fileneeded[dfilenum];
		curl_realname = curl_curfile->filename;
		nameonly(curl_realname);

		curl_origfilesize = curl_curfile->currentsize;
		curl_origtotalfilesize = curl_curfile->totalsize;

		char md5tmp[33];
		for (INT32 j = 0; j < 16; j++)
			sprintf(&md5tmp[j*2], "%02x", curl_curfile->md5sum[j]);

		curl_easy_setopt(http_handle, CURLOPT_URL, va("%s/%s?md5=%s", url, curl_realname, md5tmp));

		// Only allow HTTP and HTTPS
#if (LIBCURL_VERSION_MAJOR <= 7) && (LIBCURL_VERSION_MINOR < 85)
		curl_easy_setopt(http_handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP|CURLPROTO_HTTPS);
#else
		curl_easy_setopt(http_handle, CURLOPT_PROTOCOLS_STR, "http,https");
#endif

		// Set user agent, as some servers won't accept invalid user agents.
		curl_easy_setopt(http_handle, CURLOPT_USERAGENT, va("Sonic Robo Blast 2/%s", VERSIONSTRING));

		// Authenticate if the user so wishes
		login = CURLGetLogin(url, NULL);

		if (login)
		{
			curl_easy_setopt(http_handle, CURLOPT_USERPWD, login->auth);
		}

		// Follow a redirect request, if sent by the server.
		curl_easy_setopt(http_handle, CURLOPT_FOLLOWLOCATION, 1L);

		curl_easy_setopt(http_handle, CURLOPT_FAILONERROR, 1L);

		CONS_Printf("Downloading addon \"%s\" from %s\n", curl_realname, url);

		strcatbf(curl_curfile->filename, downloaddir, "/");
		curl_curfile->file = fopen(curl_curfile->filename, "wb");
		curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, curl_curfile->file);
		curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, curlwrite_data);
		curl_easy_setopt(http_handle, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(http_handle, CURLOPT_XFERINFOFUNCTION, curlprogress_callback);

		curl_curfile->status = FS_DOWNLOADING;
		curl_multi_add_handle(multi_handle, http_handle);

		curl_multi_perform(multi_handle, &curl_runninghandles);
		curl_starttime = time(NULL);

		filedownload.current = dfilenum;
		filedownload.http_running = true;

		return true;
	}

	filedownload.http_running = false;

	return false;
}

void CURLGetFile(void)
{
	CURLMcode mc; /* return code used by curl_multi_wait() */
	CURLcode easyres; /* Return from easy interface */
	int numfds;
	CURLMsg *m; /* for picking up messages with the transfer status */
	CURL *e;
	int msgs_left; /* how many messages are left */
	const char *easy_handle_error;

	if (curl_runninghandles)
	{
		curl_multi_perform(multi_handle, &curl_runninghandles);

		/* wait for activity, timeout or "nothing" */
		mc = curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);

		if (mc != CURLM_OK)
		{
			CONS_Alert(CONS_WARNING, "curl_multi_wait() failed, code %d.\n", mc);
			return;
		}
		curl_curfile->currentsize = curl_dlnow;
		curl_curfile->totalsize = curl_dltotal;
	}

	/* See how the transfers went */
	while ((m = curl_multi_info_read(multi_handle, &msgs_left)))
	{
		if (m && (m->msg == CURLMSG_DONE))
		{
			e = m->easy_handle;
			easyres = m->data.result;

			char *filename = Z_StrDup(curl_realname);
			nameonly(filename);

			if (easyres != CURLE_OK)
			{
				long response_code = 0;

				if (easyres == CURLE_HTTP_RETURNED_ERROR)
					curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &response_code);

				if (response_code == 404)
					curl_curfile->failed = FDOWNLOAD_FAIL_NOTFOUND;
				else
					curl_curfile->failed = FDOWNLOAD_FAIL_OTHER;

				easy_handle_error = (response_code) ? va("HTTP response code %ld", response_code) : curl_easy_strerror(easyres);
				curl_curfile->status = FS_FALLBACK;
				curl_curfile->currentsize = curl_origfilesize;
				curl_curfile->totalsize = curl_origtotalfilesize;
				filedownload.http_failed = true;
				fclose(curl_curfile->file);
				remove(curl_curfile->filename);
				CONS_Alert(CONS_ERROR, M_GetText("Failed to download addon \"%s\" (%s)\n"), filename, easy_handle_error);
			}
			else
			{
				fclose(curl_curfile->file);

				CONS_Printf(M_GetText("Finished download of \"%s\"\n"), filename);

				if (checkfilemd5(curl_curfile->filename, curl_curfile->md5sum) == FS_MD5SUMBAD)
				{
					CONS_Alert(CONS_WARNING, M_GetText("File \"%s\" does not match the version used by the server\n"), filename);
					curl_curfile->status = FS_FALLBACK;
					curl_curfile->failed = FDOWNLOAD_FAIL_MD5SUMBAD;
					filedownload.http_failed = true;
				}
				else
				{
					filedownload.completednum++;
					filedownload.completedsize += curl_curfile->totalsize;
					curl_curfile->status = FS_FOUND;
				}
			}

			Z_Free(filename);

			curl_curfile->file = NULL;
			filedownload.http_running = false;
			filedownload.remaining--;
			curl_multi_remove_handle(multi_handle, e);
			curl_easy_cleanup(e);

			if (!filedownload.remaining)
				break;
		}
	}

	if (!filedownload.remaining)
	{
		curl_multi_cleanup(multi_handle);
		curl_global_cleanup();
	}
}

HTTP_login *
CURLGetLogin (const char *url, HTTP_login ***return_prev_next)
{
	HTTP_login  * login;
	HTTP_login ** prev_next;

	for (
			prev_next = &curl_logins;
			( login = (*prev_next));
			prev_next = &login->next
	){
		if (strcmp(login->url, url) == 0)
		{
			if (return_prev_next)
				(*return_prev_next) = prev_next;

			return login;
		}
	}

	return NULL;
}

// Functions cut and pasted from Doomatic :)

void nameonly(char *s)
{
	for (size_t j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			void *ns = &(s[j+1]);
			size_t len = strlen(ns);
#if 0
			M_Memcpy(s, ns, len+1);
#else
			memmove(s, ns, len+1);
#endif
			return;
		}
}

// Returns the length in characters of the last element of a path.
size_t nameonlylength(const char *s)
{
	size_t len = strlen(s);

	for (size_t j = len; j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
			return len - j - 1;

	return len;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum)
{
#if defined (NOMD5)
	(void)wantedmd5sum;
	(void)filename;
#else
	FILE *fhandle;
	UINT8 md5sum[16];

	if (!wantedmd5sum)
		return FS_FOUND;

	fhandle = fopen(filename, "rb");
	if (fhandle)
	{
		md5_stream(fhandle,md5sum);
		fclose(fhandle);
		if (!memcmp(wantedmd5sum, md5sum, 16))
			return FS_FOUND;
		return FS_MD5SUMBAD;
	}

	I_Error("Couldn't open %s for md5 check", filename);
#endif
	return FS_FOUND; // will never happen, but makes the compiler shut up
}

// Rewritten by Monster Iestyn to be less stupid
// Note: if completepath is true, "filename" is modified, but only if FS_FOUND is going to be returned
// (Don't worry about WinCE's version of filesearch, nobody cares about that OS anymore)
filestatus_t findfile(char *filename, const UINT8 *wantedmd5sum, boolean completepath)
{
	filestatus_t homecheck; // store result of last file search
	boolean badmd5 = false; // store whether md5 was bad from either of the first two searches (if nothing was found in the third)

	// first, check SRB2's "home" directory
	homecheck = filesearch(filename, srb2home, wantedmd5sum, completepath, 10);

	if (homecheck == FS_FOUND) // we found the file, so return that we have :)
		return FS_FOUND;
	else if (homecheck == FS_MD5SUMBAD) // file has a bad md5; move on and look for a file with the right md5
		badmd5 = true;
	// if not found at all, just move on without doing anything

	// next, check SRB2's "path" directory
	homecheck = filesearch(filename, srb2path, wantedmd5sum, completepath, 10);

	if (homecheck == FS_FOUND) // we found the file, so return that we have :)
		return FS_FOUND;
	else if (homecheck == FS_MD5SUMBAD) // file has a bad md5; move on and look for a file with the right md5
		badmd5 = true;
	// if not found at all, just move on without doing anything

	// finally check "." directory
	homecheck = filesearch(filename, ".", wantedmd5sum, completepath, 10);

	if (homecheck != FS_NOTFOUND) // if not found this time, fall back on the below return statement
		return homecheck; // otherwise return the result we got

	return (badmd5 ? FS_MD5SUMBAD : FS_NOTFOUND); // md5 sum bad or file not found
}

// Searches for a folder.
// This can be used with a full path, or an incomplete path.
// In the latter case, the function will try to find folders in
// srb2home, srb2path, and the current directory.
filestatus_t findfolder(const char *path)
{
	// Check the path by itself first.
	if (concatpaths(path, NULL) == 1)
		return FS_FOUND;

#define checkpath(startpath) \
	if (concatpaths(path, startpath) == 1) \
		return FS_FOUND

	checkpath(srb2home); // Then, look in srb2home.
	checkpath(srb2path); // Now, look in srb2path.
	checkpath("."); // Finally, look in the current directory.

#undef checkpath

	return FS_NOTFOUND;
}
