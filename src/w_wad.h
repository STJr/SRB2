// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  w_wad.h
/// \brief WAD I/O functions, wad resource definitions (some)

#ifndef __W_WAD__
#define __W_WAD__

#ifdef HWRENDER
#include "hardware/hw_data.h"
#endif

#ifdef __GNUG__
#pragma interface
#endif

// ==============================================================
//               WAD FILE STRUCTURE DEFINITIONS
// ==============================================================

// header of a wad file
typedef struct
{
	char identification[4]; // should be "IWAD" or "PWAD"
	UINT32 numlumps; // how many resources
	UINT32 infotableofs; // the 'directory' of resources
} wadinfo_t;

//  a memory entry of the wad directory
typedef struct
{
	unsigned long position; // filelump_t filepos
	unsigned long disksize; // filelump_t size
	char name[9]; // filelump_t name[]
	size_t size; // real (uncompressed) size
	INT32 compressed; // i
} lumpinfo_t;

// =========================================================================
//                         DYNAMIC WAD LOADING
// =========================================================================

#define MAX_WADPATH 128
#define MAX_WADFILES 48 // maximum of wad files used at the same time
// (there is a max of simultaneous open files anyway, and this should be plenty)

#define lumpcache_t void *

#ifdef HWRENDER
#include "m_aatree.h"
#endif

typedef struct wadfile_s
{
	char *filename;
	lumpinfo_t *lumpinfo;
	lumpcache_t *lumpcache;
#ifdef HWRENDER
	aatree_t *hwrcache; // patches are cached in renderer's native format
#endif
	UINT16 numlumps; // this wad's number of resources
	FILE *handle;
	UINT32 filesize; // for network
	UINT8 md5sum[16];
} wadfile_t;

#define WADFILENUM(lumpnum) (UINT16)((lumpnum)>>16) // wad flumpnum>>16) // wad file number in upper word
#define LUMPNUM(lumpnum) (UINT16)((lumpnum)&0xFFFF) // lump number for this pwad

extern UINT16 numwadfiles;
extern wadfile_t *wadfiles[MAX_WADFILES];

// =========================================================================

void W_Shutdown(void);

// Opens a WAD file. Returns the FILE * handle for the file, or NULL if not found or could not be opened
FILE *W_OpenWadFile(const char **filename, boolean useerrors);
// Load and add a wadfile to the active wad files, returns numbers of lumps, INT16_MAX on error
UINT16 W_LoadWadFile(const char *filename);
#ifdef DELFILE
void W_UnloadWadFile(UINT16 num);
#endif

// W_InitMultipleFiles returns 1 if all is okay, 0 otherwise,
// so that it stops with a message if a file was not found, but not if all is okay.
INT32 W_InitMultipleFiles(char **filenames);

const char *W_CheckNameForNumPwad(UINT16 wad, UINT16 lump);
const char *W_CheckNameForNum(lumpnum_t lumpnum);

UINT16 W_CheckNumForNamePwad(const char *name, UINT16 wad, UINT16 startlump); // checks only in one pwad
lumpnum_t W_CheckNumForName(const char *name);
lumpnum_t W_GetNumForName(const char *name); // like W_CheckNumForName but I_Error on LUMPERROR
lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend);
UINT8 W_LumpExists(const char *name); // Lua uses this.

size_t W_LumpLengthPwad(UINT16 wad, UINT16 lump);
size_t W_LumpLength(lumpnum_t lumpnum);

size_t W_ReadLumpHeaderPwad(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset);
size_t W_ReadLumpHeader(lumpnum_t lump, void *dest, size_t size, size_t offest); // read all or a part of a lump
void W_ReadLumpPwad(UINT16 wad, UINT16 lump, void *dest);
void W_ReadLump(lumpnum_t lump, void *dest);

void *W_CacheLumpNumPwad(UINT16 wad, UINT16 lump, INT32 tag);
void *W_CacheLumpNum(lumpnum_t lump, INT32 tag);
void *W_CacheLumpNumForce(lumpnum_t lumpnum, INT32 tag);

boolean W_IsLumpCached(lumpnum_t lump, void *ptr);

void *W_CacheLumpName(const char *name, INT32 tag);
void *W_CachePatchName(const char *name, INT32 tag);

#ifdef HWRENDER
//void *W_CachePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag); // return a patch_t
void *W_CachePatchNum(lumpnum_t lumpnum, INT32 tag); // return a patch_t
#else
//#define W_CachePatchNumPwad(wad, lump, tag) W_CacheLumpNumPwad(wad, lump, tag)
#define W_CachePatchNum(lumpnum, tag) W_CacheLumpNum(lumpnum, tag)
#endif

void W_UnlockCachedPatch(void *patch);

void W_VerifyFileMD5(UINT16 wadfilenum, const char *matchmd5);

int W_VerifyNMUSlumps(const char *filename);

#endif // __W_WAD__
