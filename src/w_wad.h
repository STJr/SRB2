// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
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

// a raw entry of the wad directory
// NOTE: This sits here and not in w_wad.c because p_setup.c makes use of it to load map WADs inside PK3s.
#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	UINT32 filepos; // file offset of the resource
	UINT32 size; // size of the resource
	char name[8]; // name of the resource
} ATTRPACK filelump_t;
#if defined(_MSC_VER)
#pragma pack()
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

// Available compression methods for lumps.
typedef enum
{
	CM_NOCOMPRESSION,
#ifdef HAVE_ZLIB
	CM_DEFLATE,
#endif
	CM_LZF,
	CM_UNSUPPORTED
} compmethod;

//  a memory entry of the wad directory
typedef struct
{
	unsigned long position; // filelump_t filepos
	unsigned long disksize; // filelump_t size
	char name[9];           // filelump_t name[] e.g. "LongEntr"
	UINT32 hash;
	char *longname;         //                   e.g. "LongEntryName"
	char *fullname;         //                   e.g. "Folder/Subfolder/LongEntryName.extension"
	char *diskpath;         // path to the file  e.g. "/usr/games/srb2/Addon/Folder/Subfolder/LongEntryName.extension"
	size_t size;            // real (uncompressed) size
	compmethod compression; // lump compression method
} lumpinfo_t;

// =========================================================================
//                         'VIRTUAL' RESOURCES
// =========================================================================

typedef struct {
	char name[9];
	UINT8* data;
	size_t size;
} virtlump_t;

typedef struct {
	size_t numlumps;
	virtlump_t* vlumps;
} virtres_t;

virtres_t* vres_GetMap(lumpnum_t);
void vres_Free(virtres_t*);
virtlump_t* vres_Find(const virtres_t*, const char*);

// =========================================================================
//                         DYNAMIC WAD LOADING
// =========================================================================

// Maximum of files that can be loaded
// (there is a max of simultaneous open files anyway)
#ifdef ENFORCE_WAD_LIMIT
#define MAX_WADFILES 2048 // This cannot be any higher than UINT16_MAX.
#else
#define MAX_WADFILES UINT16_MAX
#endif

#define MAX_WADPATH 512

#define lumpcache_t void *

// Resource type of the WAD. Yeah, I know this sounds dumb, but I'll leave it like this until I clean up the code further.
typedef enum restype
{
	RET_WAD,
	RET_SOC,
	RET_LUA,
	RET_PK3,
	RET_FOLDER,
	RET_UNKNOWN,
} restype_t;

typedef struct wadfile_s
{
	char *filename, *path;
	restype_t type;
	lumpinfo_t *lumpinfo;
	lumpcache_t *lumpcache;
	lumpcache_t *patchcache;
	UINT16 numlumps; // this wad's number of resources
	UINT16 foldercount; // folder count
	FILE *handle;
	UINT32 filesize; // for network
	UINT8 md5sum[16];

	boolean important; // also network - !W_VerifyNMUSlumps
} wadfile_t;

#define WADFILENUM(lumpnum) (UINT16)((lumpnum)>>16) // wad file number in upper word
#define LUMPNUM(lumpnum) (UINT16)((lumpnum)&0xFFFF) // lump number for this pwad

extern UINT16 numwadfiles;
extern wadfile_t **wadfiles;

typedef struct
{
	char **files;
	size_t numfiles;
} addfilelist_t;

// =========================================================================

void W_Shutdown(void);

// Opens a WAD file. Returns the FILE * handle for the file, or NULL if not found or could not be opened
FILE *W_OpenWadFile(const char **filename, boolean useerrors);
// Load and add a wadfile to the active wad files, returns numbers of lumps, INT16_MAX on error
UINT16 W_InitFile(const char *filename, boolean mainfile, boolean startup);
// Adds a folder as a file
UINT16 W_InitFolder(const char *path, boolean mainfile, boolean startup);

// W_InitMultipleFiles exits if a file was not found, but not if all is okay.
void W_InitMultipleFiles(addfilelist_t *list);

#define W_FileHasFolders(wadfile) ((wadfile)->type == RET_PK3 || (wadfile)->type == RET_FOLDER)

INT32 W_IsPathToFolderValid(const char *path);
char *W_GetFullFolderPath(const char *path);

const char *W_CheckNameForNumPwad(UINT16 wad, UINT16 lump);
const char *W_CheckNameForNum(lumpnum_t lumpnum);

UINT16 W_CheckNumForNamePwad(const char *name, UINT16 wad, UINT16 startlump); // checks only in one pwad
UINT16 W_CheckNumForLongNamePwad(const char *name, UINT16 wad, UINT16 startlump);

/* Find the first lump after F_START for instance. */
UINT16 W_CheckNumForMarkerStartPwad(const char *name, UINT16 wad, UINT16 startlump);

UINT16 W_CheckNumForFullNamePK3(const char *name, UINT16 wad, UINT16 startlump);
UINT16 W_CheckNumForFolderStartPK3(const char *name, UINT16 wad, UINT16 startlump);
UINT16 W_CheckNumForFolderEndPK3(const char *name, UINT16 wad, UINT16 startlump);
char *W_GetLumpFolderPathPK3(UINT16 wad, UINT16 lump);
char *W_GetLumpFolderNamePK3(UINT16 wad, UINT16 lump);

void W_GetFolderLumpsPwad(const char *name, UINT16 wad, UINT32 **list, UINT16 *list_capacity, UINT16 *numlumps);
void W_GetFolderLumps(const char *name, UINT32 **list, UINT16 *list_capacity, UINT16 *numlumps);
UINT32 W_CountFolderLumpsPwad(const char *name, UINT16 wad);
UINT32 W_CountFolderLumps(const char *name);

lumpnum_t W_CheckNumForMap(const char *name);
lumpnum_t W_CheckNumForName(const char *name);
lumpnum_t W_CheckNumForLongName(const char *name);
lumpnum_t W_GetNumForName(const char *name); // like W_CheckNumForName but I_Error on LUMPERROR
lumpnum_t W_GetNumForLongName(const char *name);

lumpnum_t W_CheckNumForPatchName(const char *name);
lumpnum_t W_CheckNumForLongPatchName(const char *name);
lumpnum_t W_GetNumForPatchName(const char *name); // like W_CheckNumForPatchName but I_Error on LUMPERROR
lumpnum_t W_GetNumForLongPatchName(const char *name);

lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend);
UINT8 W_LumpExists(const char *name); // Lua uses this.

size_t W_LumpLengthPwad(UINT16 wad, UINT16 lump);
size_t W_LumpLength(lumpnum_t lumpnum);

boolean W_IsLumpWad(lumpnum_t lumpnum); // for loading maps from WADs in PK3s
boolean W_IsLumpFolder(UINT16 wad, UINT16 lump); // for detecting folder "lumps"

#ifdef HAVE_ZLIB
void zerr(int ret); // zlib error checking
#endif

size_t W_ReadLumpHeaderPwad(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset);
size_t W_ReadLumpHeader(lumpnum_t lump, void *dest, size_t size, size_t offest); // read all or a part of a lump
void W_ReadLumpPwad(UINT16 wad, UINT16 lump, void *dest);
void W_ReadLump(lumpnum_t lump, void *dest);

void *W_CacheLumpNumPwad(UINT16 wad, UINT16 lump, INT32 tag);
void *W_CacheLumpNum(lumpnum_t lump, INT32 tag);
void *W_CacheLumpNumForce(lumpnum_t lumpnum, INT32 tag);

boolean W_IsLumpCached(lumpnum_t lump, void *ptr);
boolean W_IsPatchCached(lumpnum_t lump, void *ptr);
boolean W_IsPatchCachedPwad(UINT16 wad, UINT16 lump, void *ptr);

void *W_CacheLumpName(const char *name, INT32 tag);
void *W_CachePatchName(const char *name, INT32 tag);
void *W_CachePatchLongName(const char *name, INT32 tag);

void *W_CachePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag);
void *W_CachePatchNum(lumpnum_t lumpnum, INT32 tag);
void *W_GetCachedPatchNumPwad(UINT16 wad, UINT16 lump);

boolean W_ReadPatchHeaderPwad(UINT16 wadnum, UINT16 lumpnum, INT16 *width, INT16 *height, INT16 *topoffset, INT16 *leftoffset);
boolean W_ReadPatchHeader(lumpnum_t lumpnum, INT16 *width, INT16 *height, INT16 *topoffset, INT16 *leftoffset);

void W_UnlockCachedPatch(void *patch);

void W_VerifyFileMD5(UINT16 wadfilenum, const char *matchmd5);

int W_VerifyNMUSlumps(const char *filename, boolean exit_on_error);

#endif // __W_WAD__
