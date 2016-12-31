// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netfil.c
/// \brief Transfer a file using HSendPacket.

#include <stdio.h>
#ifndef _WIN32_WCE
#ifdef __OS2__
#include <sys/types.h>
#endif // __OS2__
#include <sys/stat.h>
#endif

#if !defined (UNDER_CE)
#include <time.h>
#endif

#if ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (__DJGPP__)) && !defined (_XBOX)
#include <io.h>
#include <direct.h>
#elif !defined (_WIN32_WCE) && !(defined (_XBOX) && !defined (__GNUC__))
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#include <limits.h>
#elif defined (_WIN32) && !defined (_WIN32_WCE)
#include <sys/utime.h>
#endif
#ifdef __DJGPP__
#include <dir.h>
#include <utime.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "i_net.h"
#include "i_system.h"
#include "m_argv.h"
#include "d_net.h"
#include "w_wad.h"
#include "d_netfil.h"
#include "z_zone.h"
#include "byteptr.h"
#include "p_setup.h"
#include "m_misc.h"
#include "m_menu.h"
#include "md5.h"
#include "filesrch.h"

#include <errno.h>

static void SV_SendFile(INT32 node, const char *filename, UINT8 fileid);

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
	UINT32 position; // The current position in the file
	FILE *currentfile; // The file currently being sent/received
} filetran_t;
static filetran_t transfer[MAXNETNODES];

// Read time of file: stat _stmtime
// Write time of file: utime

// Receiver structure
INT32 fileneedednum; // Number of files needed to join the server
fileneeded_t fileneeded[MAX_WADFILES]; // List of needed files
char downloaddir[256] = "DOWNLOAD";

#ifdef CLIENT_LOADINGSCREEN
// for cl loading screen
INT32 lastfilenum = 0;
#endif

/** Fills a serverinfo packet with information about wad files loaded.
  *
  * \todo Give this function a better name since it is in global scope.
  *
  */
UINT8 *PutFileNeeded(void)
{
	size_t i, count = 0;
	UINT8 *p = netbuffer->u.serverinfo.fileneeded;
	char wadfilename[MAX_WADPATH] = "";
	UINT8 filestatus;
	size_t bytesused = 0;

	for (i = 0; i < numwadfiles; i++)
	{
		// If it has only music/sound lumps, mark it as unimportant
		if (W_VerifyNMUSlumps(wadfiles[i]->filename))
			filestatus = 0;
		else
			filestatus = 1; // Important

		// Store in the upper four bits
		if (!cv_downloading.value)
			filestatus += (2 << 4); // Won't send
		else if ((wadfiles[i]->filesize > (UINT32)cv_maxsend.value * 1024))
			filestatus += (0 << 4); // Won't send
		else
			filestatus += (1 << 4); // Will send if requested

		bytesused += (nameonlylength(wadfilename) + 22);

		// Don't write too far...
		if (bytesused > sizeof(netbuffer->u.serverinfo.fileneeded))
			I_Error("Too many wad files added to host a game. (%s, stopped on %s)\n", sizeu1(bytesused), wadfilename);

		WRITEUINT8(p, filestatus);

		count++;
		WRITEUINT32(p, wadfiles[i]->filesize);
		nameonly(strcpy(wadfilename, wadfiles[i]->filename));
		WRITESTRINGN(p, wadfilename, MAX_WADPATH);
		WRITEMEM(p, wadfiles[i]->md5sum, 16);
	}
	netbuffer->u.serverinfo.fileneedednum = (UINT8)count;

	return p;
}

/** Parses the serverinfo packet and fills the fileneeded table on client
  *
  * \param fileneedednum_parm The number of files needed to join the server
  * \param fileneededstr The memory block containing the list of needed files
  *
  */
void D_ParseFileneeded(INT32 fileneedednum_parm, UINT8 *fileneededstr)
{
	INT32 i;
	UINT8 *p;
	UINT8 filestatus;

	fileneedednum = fileneedednum_parm;
	p = (UINT8 *)fileneededstr;
	for (i = 0; i < fileneedednum; i++)
	{
		fileneeded[i].status = FS_NOTFOUND; // We haven't even started looking for the file yet
		filestatus = READUINT8(p); // The first byte is the file status
		fileneeded[i].important = (UINT8)(filestatus & 3);
		fileneeded[i].willsend = (UINT8)(filestatus >> 4);
		fileneeded[i].totalsize = READUINT32(p); // The four next bytes are the file size
		fileneeded[i].file = NULL; // The file isn't open yet
		READSTRINGN(p, fileneeded[i].filename, MAX_WADPATH); // The next bytes are the file name
		READMEM(p, fileneeded[i].md5sum, 16); // The last 16 bytes are the file checksum
	}
}

void CL_PrepareDownloadSaveGame(const char *tmpsave)
{
	fileneedednum = 1;
	fileneeded[0].status = FS_REQUESTED;
	fileneeded[0].totalsize = UINT32_MAX;
	fileneeded[0].file = NULL;
	memset(fileneeded[0].md5sum, 0, 16);
	strcpy(fileneeded[0].filename, tmpsave);
}

/** Checks the server to see if we CAN download all the files,
  * before starting to create them and requesting.
  *
  * \return True if we can download all the files
  *
  */
boolean CL_CheckDownloadable(void)
{
	UINT8 i,dlstatus = 0;

	for (i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN && fileneeded[i].important)
		{
			if (fileneeded[i].willsend == 1)
				continue;

			if (fileneeded[i].willsend == 0)
				dlstatus = 1;
			else //if (fileneeded[i].willsend == 2)
				dlstatus = 2;
		}

	// Downloading locally disabled
	if (!dlstatus && M_CheckParm("-nodownload"))
		dlstatus = 3;

	if (!dlstatus)
		return true;

	// not downloadable, put reason in console
	CONS_Alert(CONS_NOTICE, M_GetText("You need additional files to connect to this server:\n"));
	for (i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN && fileneeded[i].important)
		{
			CONS_Printf(" * \"%s\" (%dK)", fileneeded[i].filename, fileneeded[i].totalsize >> 10);

				if (fileneeded[i].status == FS_NOTFOUND)
					CONS_Printf(M_GetText(" not found, md5: "));
				else if (fileneeded[i].status == FS_MD5SUMBAD)
					CONS_Printf(M_GetText(" wrong version, md5: "));

			{
				INT32 j;
				char md5tmp[33];
				for (j = 0; j < 16; j++)
					sprintf(&md5tmp[j*2], "%02x", fileneeded[i].md5sum[j]);
				CONS_Printf("%s", md5tmp);
			}
			CONS_Printf("\n");
		}

	switch (dlstatus)
	{
		case 1:
			CONS_Printf(M_GetText("Some files are larger than the server is willing to send.\n"));
			break;
		case 2:
			CONS_Printf(M_GetText("The server is not allowing download requests.\n"));
			break;
		case 3:
			CONS_Printf(M_GetText("All files downloadable, but you have chosen to disable downloading locally.\n"));
			break;
	}
	return false;
}

/** Sends requests for files in the ::fileneeded table with a status of
  * ::FS_NOTFOUND.
  *
  * \return True if the packet was successfully sent
  * \note Sends a PT_REQUESTFILE packet
  *
  */
boolean CL_SendRequestFile(void)
{
	char *p;
	INT32 i;
	INT64 totalfreespaceneeded = 0, availablefreespace;

#ifdef PARANOIA
	if (M_CheckParm("-nodownload"))
		I_Error("Attempted to download files in -nodownload mode");

	for (i = 0; i < fileneedednum; i++)
		if (fileneeded[i].status != FS_FOUND && fileneeded[i].status != FS_OPEN
			&& fileneeded[i].important && (fileneeded[i].willsend == 0 || fileneeded[i].willsend == 2))
		{
			I_Error("Attempted to download files that were not sendable");
		}
#endif

	netbuffer->packettype = PT_REQUESTFILE;
	p = (char *)netbuffer->u.textcmd;
	for (i = 0; i < fileneedednum; i++)
		if ((fileneeded[i].status == FS_NOTFOUND || fileneeded[i].status == FS_MD5SUMBAD)
			&& fileneeded[i].important)
		{
			totalfreespaceneeded += fileneeded[i].totalsize;
			nameonly(fileneeded[i].filename);
			WRITEUINT8(p, i); // fileid
			WRITESTRINGN(p, fileneeded[i].filename, MAX_WADPATH);
			// put it in download dir
			strcatbf(fileneeded[i].filename, downloaddir, "/");
			fileneeded[i].status = FS_REQUESTED;
		}
	WRITEUINT8(p, 0xFF);
	I_GetDiskFreeSpace(&availablefreespace);
	if (totalfreespaceneeded > availablefreespace)
		I_Error("To play on this server you must download %s KB,\n"
			"but you have only %s KB free space on this drive\n",
			sizeu1((size_t)(totalfreespaceneeded>>10)), sizeu2((size_t)(availablefreespace>>10)));

	// prepare to download
	I_mkdir(downloaddir, 0755);
	return HSendPacket(servernode, true, 0, p - (char *)netbuffer->u.textcmd);
}

// get request filepak and put it on the send queue
void Got_RequestFilePak(INT32 node)
{
	char wad[MAX_WADPATH+1];
	UINT8 *p = netbuffer->u.textcmd;
	UINT8 id;
	while (p < netbuffer->u.textcmd + MAXTEXTCMD-1) // Don't allow hacked client to overflow
	{
		id = READUINT8(p);
		if (id == 0xFF)
			break;
		READSTRINGN(p, wad, MAX_WADPATH);
		SV_SendFile(node, wad, id);
	}
}

/** Checks if the files needed aren't already loaded or on the disk
  *
  * \return 0 if some files are missing
  *         1 if all files exist
  *         2 if some already loaded files are not requested or are in a different order
  *
  */
INT32 CL_CheckFiles(void)
{
	INT32 i, j;
	char wadfilename[MAX_WADPATH];
	INT32 ret = 1;

//	if (M_CheckParm("-nofiles"))
//		return 1;

	// the first is the iwad (the main wad file)
	// we don't care if it's called srb2.srb or srb2.wad.
	// Never download the IWAD, just assume it's there and identical
	fileneeded[0].status = FS_OPEN;

	// Modified game handling -- check for an identical file list
	// must be identical in files loaded AND in order
	// Return 2 on failure -- disconnect from server
	if (modifiedgame)
	{
		CONS_Debug(DBG_NETPLAY, "game is modified; only doing basic checks\n");
		for (i = 1, j = 1; i < fileneedednum || j < numwadfiles;)
		{
			if (i < fileneedednum && !fileneeded[i].important)
			{
				// Eh whatever, don't care
				++i;
				continue;
			}
			if (j < numwadfiles && W_VerifyNMUSlumps(wadfiles[j]->filename))
			{
				// Unimportant on our side. still don't care.
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

	for (i = 1; i < fileneedednum; i++)
	{
		CONS_Debug(DBG_NETPLAY, "searching for '%s' ", fileneeded[i].filename);

		// Check in already loaded files
		for (j = 1; wadfiles[j]; j++)
		{
			nameonly(strcpy(wadfilename, wadfiles[j]->filename));
			if (!stricmp(wadfilename, fileneeded[i].filename) &&
				!memcmp(wadfiles[j]->md5sum, fileneeded[i].md5sum, 16))
			{
				CONS_Debug(DBG_NETPLAY, "already loaded\n");
				fileneeded[i].status = FS_OPEN;
				break;
			}
		}
		if (fileneeded[i].status != FS_NOTFOUND || !fileneeded[i].important)
			continue;

		fileneeded[i].status = findfile(fileneeded[i].filename, fileneeded[i].md5sum, true);
		CONS_Debug(DBG_NETPLAY, "found %d\n", fileneeded[i].status);
		if (fileneeded[i].status != FS_FOUND)
			ret = 0;
	}
	return ret;
}

// Load it now
void CL_LoadServerFiles(void)
{
	INT32 i;

//	if (M_CheckParm("-nofiles"))
//		return;

	for (i = 1; i < fileneedednum; i++)
	{
		if (fileneeded[i].status == FS_OPEN)
			continue; // Already loaded
		else if (fileneeded[i].status == FS_FOUND)
		{
			P_AddWadFile(fileneeded[i].filename, NULL);
			G_SetGameModified(true);
			fileneeded[i].status = FS_OPEN;
		}
		else if (fileneeded[i].status == FS_MD5SUMBAD)
		{
			// If the file is marked important, don't even bother proceeding.
			if (fileneeded[i].important)
				I_Error("Wrong version of important file %s", fileneeded[i].filename);

			// If it isn't, no need to worry the user with a console message,
			// although it can't hurt to put something in the debug file.

			// ...but wait a second. What if the local version is "important"?
			if (!W_VerifyNMUSlumps(fileneeded[i].filename))
				I_Error("File %s should only contain music and sound effects!",
					fileneeded[i].filename);

			// Okay, NOW we know it's safe. Whew.
			P_AddWadFile(fileneeded[i].filename, NULL);
			if (fileneeded[i].important)
				G_SetGameModified(true);
			fileneeded[i].status = FS_OPEN;
			DEBFILE(va("File %s found but with different md5sum\n", fileneeded[i].filename));
		}
		else if (fileneeded[i].important)
		{
			char *s;
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
}

// Number of files to send
// Little optimization to quickly test if there is a file in the queue
static INT32 filestosend = 0;

/** Adds a file to the file list for a node
  *
  * \param node The node to send the file to
  * \param filename The file to send
  * \param fileid ???
  * \sa SV_SendRam
  *
  */
static void SV_SendFile(INT32 node, const char *filename, UINT8 fileid)
{
	filetx_t **q; // A pointer to the "next" field of the last file in the list
	filetx_t *p; // The new file request
	INT32 i;
	char wadfilename[MAX_WADPATH];

	// Find the last file in the list and set a pointer to its "next" field
	q = &transfer[node].txlist;
	while (*q)
		q = &((*q)->next);

	// Allocate a file request and append it to the file list
	p = *q = (filetx_t *)malloc(sizeof (filetx_t));
	if (!p)
		I_Error("SV_SendFile: No more memory\n");

	// Initialise with zeros
	memset(p, 0, sizeof (filetx_t));

	// Allocate the file name
	p->id.filename = (char *)malloc(MAX_WADPATH);
	if (!p->id.filename)
		I_Error("SV_SendFile: No more memory\n");

	// Set the file name and get rid of the path
	strlcpy(p->id.filename, filename, MAX_WADPATH);
	nameonly(p->id.filename);

	// Look for the requested file through all loaded files
	for (i = 0; wadfiles[i]; i++)
	{
		strlcpy(wadfilename, wadfiles[i]->filename, MAX_WADPATH);
		nameonly(wadfilename);
		if (!stricmp(wadfilename, p->id.filename))
		{
			// Copy file name with full path
			strlcpy(p->id.filename, wadfiles[i]->filename, MAX_WADPATH);
			break;
		}
	}

	// Handle non-loaded file requests
	if (!wadfiles[i])
	{
		DEBFILE(va("%s not found in wadfiles\n", filename));
		// This formerly checked if (!findfile(p->id.filename, NULL, true))

		// Not found
		// Don't inform client (probably someone who thought they could leak 2.2 ACZ)
		DEBFILE(va("Client %d request %s: not found\n", node, filename));
		free(p->id.filename);
		free(p);
		*q = NULL;
		return;
	}

	// Handle huge file requests (i.e. bigger than cv_maxsend.value KB)
	if (wadfiles[i]->filesize > (UINT32)cv_maxsend.value * 1024)
	{
		// Too big
		// Don't inform client (client sucks, man)
		DEBFILE(va("Client %d request %s: file too big, not sending\n", node, filename));
		free(p->id.filename);
		free(p);
		*q = NULL;
		return;
	}

	DEBFILE(va("Sending file %s (id=%d) to %d\n", filename, fileid, node));
	p->ram = SF_FILE; // It's a file, we need to close it and free its name once we're done sending it
	p->fileid = fileid;
	p->next = NULL; // End of list
	filestosend++;
}

/** Adds a memory block to the file list for a node
  *
  * \param node The node to send the memory block to
  * \param data The memory block to send
  * \param size The size of the block in bytes
  * \param freemethod How to free the block after it has been sent
  * \param fileid ???
  * \sa SV_SendFile
  *
  */
void SV_SendRam(INT32 node, void *data, size_t size, freemethod_t freemethod, UINT8 fileid)
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
		I_Error("SV_SendRam: No more memory\n");

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

/** Stops sending a file for a node, and removes the file request from the list,
  * either because the file has been fully sent or because the node was disconnected
  *
  * \param node The destination
  *
  */
static void SV_EndFileSend(INT32 node)
{
	filetx_t *p = transfer[node].txlist;

	// Free the file request according to the freemethod parameter used with SV_SendFile/Ram
	switch (p->ram)
	{
		case SF_FILE: // It's a file, close it and free its filename
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

	filestosend--;
}

#define PACKETPERTIC net_bandwidth/(TICRATE*software_MAXPACKETLENGTH)

/** Handles file transmission
  *
  *
  */
void SV_FileSendTicker(void)
{
	static INT32 currentnode = 0;
	filetx_pak *p;
	size_t size;
	filetx_t *f;
	INT32 packetsent = PACKETPERTIC, ram, i;

	if (!filestosend)
		return;
	if (!packetsent)
		packetsent++;
	// (((sendbytes-nowsentbyte)*TICRATE)/(I_GetTime()-starttime)<(UINT32)net_bandwidth)
	while (packetsent-- && filestosend != 0)
	{
		for (i = currentnode, ram = 0; ram < MAXNETNODES;
			i = (i+1) % MAXNETNODES, ram++)
		{
			if (transfer[i].txlist)
				goto found;
		}
		// no transfer to do
		I_Error("filestosend=%d but no file to send found\n", filestosend);
	found:
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
			transfer[i].position = 0;
		}

		// Build a packet containing a file fragment
		p = &netbuffer->u.filetxpak;
		size = software_MAXPACKETLENGTH - (FILETXHEADER + BASEPACKETSIZE);
		if (f->size-transfer[i].position < size)
			size = f->size-transfer[i].position;
		if (ram)
			M_Memcpy(p->data, &f->id.ram[transfer[i].position], size);
		else if (fread(p->data, 1, size, transfer[i].currentfile) != size)
			I_Error("SV_FileSendTicker: can't read %s byte on %s at %d because %s", sizeu1(size), f->id.filename, transfer[i].position, strerror(ferror(transfer[i].currentfile)));
		p->position = LONG(transfer[i].position);
		// Put flag so receiver knows the total size
		if (transfer[i].position + size == f->size)
			p->position |= LONG(0x80000000);
		p->fileid = f->fileid;
		p->size = SHORT((UINT16)size);
		netbuffer->packettype = PT_FILEFRAGMENT;

		// Send the packet
		if (HSendPacket(i, true, 0, FILETXHEADER + size)) // Reliable SEND
		{ // Success
			transfer[i].position = (UINT32)(transfer[i].position + size);
			if (transfer[i].position == f->size) // Finish?
				SV_EndFileSend(i);
		}
		else
		{ // Not sent for some odd reason, retry at next call
			if (!ram)
				fseek(transfer[i].currentfile,transfer[i].position, SEEK_SET);
			// Exit the while (can't send this one so why should i send the next?)
			break;
		}
	}
}

void Got_Filetxpak(void)
{
	INT32 filenum = netbuffer->u.filetxpak.fileid;
	static INT32 filetime = 0;

	if (filenum >= fileneedednum)
	{
		DEBFILE(va("fileframent not needed %d>%d\n", filenum, fileneedednum));
		return;
	}

	if (fileneeded[filenum].status == FS_REQUESTED)
	{
		if (fileneeded[filenum].file)
			I_Error("Got_Filetxpak: already open file\n");
		fileneeded[filenum].file = fopen(fileneeded[filenum].filename, "wb");
		if (!fileneeded[filenum].file)
			I_Error("Can't create file %s: %s", fileneeded[filenum].filename, strerror(errno));
		CONS_Printf("\r%s...\n",fileneeded[filenum].filename);
		fileneeded[filenum].currentsize = 0;
		fileneeded[filenum].status = FS_DOWNLOADING;
	}

	if (fileneeded[filenum].status == FS_DOWNLOADING)
	{
		UINT32 pos = LONG(netbuffer->u.filetxpak.position);
		UINT16 size = SHORT(netbuffer->u.filetxpak.size);
		// Use a special trick to know when the file is complete (not always used)
		// WARNING: file fragments can arrive out of order so don't stop yet!
		if (pos & 0x80000000)
		{
			pos &= ~0x80000000;
			fileneeded[filenum].totalsize = pos + size;
		}
		// We can receive packet in the wrong order, anyway all os support gaped file
		fseek(fileneeded[filenum].file, pos, SEEK_SET);
		if (fwrite(netbuffer->u.filetxpak.data,size,1,fileneeded[filenum].file) != 1)
			I_Error("Can't write to %s: %s\n",fileneeded[filenum].filename, strerror(ferror(fileneeded[filenum].file)));
		fileneeded[filenum].currentsize += size;

		// Finished?
		if (fileneeded[filenum].currentsize == fileneeded[filenum].totalsize)
		{
			fclose(fileneeded[filenum].file);
			fileneeded[filenum].file = NULL;
			fileneeded[filenum].status = FS_FOUND;
			CONS_Printf(M_GetText("Downloading %s...(done)\n"),
				fileneeded[filenum].filename);
		}
	}
	else
		I_Error("Received a file not requested\n");

	// Send ack back quickly
	if (++filetime == 3)
	{
		Net_SendAcks(servernode);
		filetime = 0;
	}

#ifdef CLIENT_LOADINGSCREEN
	lastfilenum = filenum;
#endif
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
	INT32 i;
	// Is sending?
	for (i = 0; i < MAXNETNODES; i++)
		SV_AbortSendFiles(i);

	// Receiving a file?
	for (i = 0; i < MAX_WADFILES; i++)
		if (fileneeded[i].status == FS_DOWNLOADING && fileneeded[i].file)
		{
			fclose(fileneeded[i].file);
			// File is not complete delete it
			remove(fileneeded[i].filename);
		}

	// Remove PT_FILEFRAGMENT from acknowledge list
	Net_AbortPacketType(PT_FILEFRAGMENT);
}

// Functions cut and pasted from Doomatic :)

void nameonly(char *s)
{
	size_t j, len;
	void *ns;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			ns = &(s[j+1]);
			len = strlen(ns);
			if (false)
				M_Memcpy(s, ns, len+1);
			else
				memmove(s, ns, len+1);
			return;
		}
}

// Returns the length in characters of the last element of a path.
size_t nameonlylength(const char *s)
{
	size_t j, len = strlen(s);

	for (j = len; j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
			return len - j - 1;

	return len;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum)
{
#if defined (NOMD5) || defined (_arch_dreamcast)
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

filestatus_t findfile(char *filename, const UINT8 *wantedmd5sum, boolean completepath)
{
	filestatus_t homecheck = filesearch(filename, srb2home, wantedmd5sum, false, 10);
	if (homecheck == FS_FOUND)
		return filesearch(filename, srb2home, wantedmd5sum, completepath, 10);

	homecheck = filesearch(filename, srb2path, wantedmd5sum, false, 10);
	if (homecheck == FS_FOUND)
		return filesearch(filename, srb2path, wantedmd5sum, completepath, 10);

#ifdef _arch_dreamcast
	return filesearch(filename, "/cd", wantedmd5sum, completepath, 10);
#else
	return filesearch(filename, ".", wantedmd5sum, completepath, 10);
#endif
}
