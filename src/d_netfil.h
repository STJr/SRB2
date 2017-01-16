// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netfil.h
/// \brief File transferring related structs and functions.

#ifndef __D_NETFIL__
#define __D_NETFIL__

#include "w_wad.h"

typedef enum
{
	SF_FILE,
	SF_Z_RAM,
	SF_RAM,
	SF_NOFREERAM
} freemethod_t;

typedef enum
{
	FS_NOTFOUND,
	FS_FOUND,
	FS_REQUESTED,
	FS_DOWNLOADING,
	FS_OPEN, // Is opened and used in w_wad
	FS_MD5SUMBAD
} filestatus_t;

typedef struct
{
	UINT8 important;
	UINT8 willsend; // Is the server willing to send it?
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	// Used only for download
	FILE *file;
	UINT32 currentsize;
	UINT32 totalsize;
	filestatus_t status; // The value returned by recsearch
} fileneeded_t;

extern INT32 fileneedednum;
extern fileneeded_t fileneeded[MAX_WADFILES];
extern char downloaddir[256];

#ifdef CLIENT_LOADINGSCREEN
extern INT32 lastfilenum;
#endif

UINT8 *PutFileNeeded(void);
void D_ParseFileneeded(INT32 fileneedednum_parm, UINT8 *fileneededstr);
void CL_PrepareDownloadSaveGame(const char *tmpsave);

INT32 CL_CheckFiles(void);
void CL_LoadServerFiles(void);
void SV_SendRam(INT32 node, void *data, size_t size, freemethod_t freemethod,
	UINT8 fileid);

void SV_FileSendTicker(void);
void Got_Filetxpak(void);

boolean CL_CheckDownloadable(void);
boolean CL_SendRequestFile(void);
void Got_RequestFilePak(INT32 node);

void SV_AbortSendFiles(INT32 node);
void CloseNetFile(void);

boolean fileexist(char *filename, time_t ptime);

// Search a file in the wadpath, return FS_FOUND when found
filestatus_t findfile(char *filename, const UINT8 *wantedmd5sum,
	boolean completepath);
filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum);

void nameonly(char *s);
size_t nameonlylength(const char *s);

#endif // __D_NETFIL__
