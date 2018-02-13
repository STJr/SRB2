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
/// \file  w_wad.c
/// \brief Handles WAD file header, directory, lump I/O

#ifdef __GNUC__
#include <unistd.h>
#endif

#define ZWAD

#ifdef ZWAD
#include <errno.h>
#include "lzf.h"
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

//#ifdef HAVE_ZLIB
#include "zlib.h"
//#endif // HAVE_ZLIB

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "w_wad.h"
#include "z_zone.h"
#include "fastcmp.h"

#include "filesrch.h"

#include "i_video.h" // rendermode
#include "d_netfil.h"
#include "dehacked.h"
#include "d_clisrv.h"
#include "r_defs.h"
#include "i_system.h"
#include "md5.h"
#include "lua_script.h"
#ifdef SCANTHINGS
#include "p_setup.h" // P_ScanThings
#endif
#include "m_misc.h" // M_MapNumber

#ifdef HWRENDER
#include "r_data.h"
#include "hardware/hw_main.h"
#include "hardware/hw_glob.h"
#endif

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined(_MSC_VER)
#pragma pack(1)
#endif

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	const char *name;
	size_t len;
} lumpchecklist_t;

// Must be a power of two
#define LUMPNUMCACHESIZE 64

typedef struct lumpnum_cache_s
{
	char lumpname[8];
	lumpnum_t lumpnum;
} lumpnum_cache_t;

static lumpnum_cache_t lumpnumcache[LUMPNUMCACHESIZE];
static UINT16 lumpnumcacheindex = 0;

//===========================================================================
//                                                                    GLOBALS
//===========================================================================
UINT16 numwadfiles; // number of active wadfiles
wadfile_t *wadfiles[MAX_WADFILES]; // 0 to numwadfiles-1 are valid

// W_Shutdown
// Closes all of the WAD files before quitting
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected
void W_Shutdown(void)
{
	while (numwadfiles--)
	{
		fclose(wadfiles[numwadfiles]->handle);
		Z_Free(wadfiles[numwadfiles]->lumpinfo);
		Z_Free(wadfiles[numwadfiles]->filename);
		while (wadfiles[numwadfiles]->numlumps--)
			Z_Free(wadfiles[numwadfiles]->lumpinfo[wadfiles[numwadfiles]->numlumps].name2);
		Z_Free(wadfiles[numwadfiles]);
	}
}

//===========================================================================
//                                                        LUMP BASED ROUTINES
//===========================================================================

// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

static char filenamebuf[MAX_WADPATH];

// W_OpenWadFile
// Helper function for opening the WAD file.
// Returns the FILE * handle for the file, or NULL if not found or could not be opened
// If "useerrors" is true then print errors in the console, else just don't bother
// "filename" may be modified to have the correct path the actual file is located in, if necessary
FILE *W_OpenWadFile(const char **filename, boolean useerrors)
{
	FILE *handle;

	strncpy(filenamebuf, *filename, MAX_WADPATH);
	filenamebuf[MAX_WADPATH - 1] = '\0';
	*filename = filenamebuf;

	// open wad file
	if ((handle = fopen(*filename, "rb")) == NULL)
	{
		// If we failed to load the file with the path as specified by
		// the user, strip the directories and search for the file.
		nameonly(filenamebuf);

		// If findfile finds the file, the full path will be returned
		// in filenamebuf == *filename.
		if (findfile(filenamebuf, NULL, true))
		{
			if ((handle = fopen(*filename, "rb")) == NULL)
			{
				if (useerrors)
					CONS_Alert(CONS_ERROR, M_GetText("Can't open %s\n"), *filename);
				return NULL;
			}
		}
		else
		{
			if (useerrors)
				CONS_Alert(CONS_ERROR, M_GetText("File %s not found.\n"), *filename);
			return NULL;
		}
	}
	return handle;
}

// Look for all DEHACKED and Lua scripts inside a PK3 archive.
static inline void W_LoadDehackedLumpsPK3(UINT16 wadnum)
{
	UINT16 posStart, posEnd;
	posStart = W_CheckNumForFolderStartPK3("Lua/", wadnum, 0);
	if (posStart != INT16_MAX)
	{
		posEnd = W_CheckNumForFolderEndPK3("Lua/", wadnum, posStart);
		posStart++; // first "lump" will be "Lua/" folder itself, so ignore it
		for (; posStart < posEnd; posStart++)
			LUA_LoadLump(wadnum, posStart);
	}
	posStart = W_CheckNumForFolderStartPK3("SOCs/", wadnum, 0);
	if (posStart != INT16_MAX)
	{
		posEnd = W_CheckNumForFolderEndPK3("SOCs/", wadnum, posStart);
		posStart++; // first "lump" will be "SOCs/" folder itself, so ignore it
		for(; posStart < posEnd; posStart++)
		{
			lumpinfo_t *lump_p = &wadfiles[wadnum]->lumpinfo[posStart];
			size_t length = strlen(wadfiles[wadnum]->filename) + 1 + strlen(lump_p->name2); // length of file name, '|', and lump name
			char *name = malloc(length + 1);
			sprintf(name, "%s|%s", wadfiles[wadnum]->filename, lump_p->name2);
			name[length] = '\0';

			CONS_Printf(M_GetText("Loading SOC from %s\n"), name);
			DEH_LoadDehackedLumpPwad(wadnum, posStart);
			free(name);
		}
	}
}

// search for all DEHACKED lump in all wads and load it
static inline void W_LoadDehackedLumps(UINT16 wadnum)
{
	UINT16 lump;

#ifdef HAVE_BLUA
	// Find Lua scripts before SOCs to allow new A_Actions in SOC editing.
	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
			if (memcmp(lump_p->name,"LUA_",4)==0)
				LUA_LoadLump(wadnum, lump);
	}
#endif

	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
			if (memcmp(lump_p->name,"SOC_",4)==0) // Check for generic SOC lump
			{	// shameless copy+paste of code from LUA_LoadLump
				size_t length = strlen(wadfiles[wadnum]->filename) + 1 + strlen(lump_p->name2); // length of file name, '|', and lump name
				char *name = malloc(length + 1);
				sprintf(name, "%s|%s", wadfiles[wadnum]->filename, lump_p->name2);
				name[length] = '\0';

				CONS_Printf(M_GetText("Loading SOC from %s\n"), name);
				DEH_LoadDehackedLumpPwad(wadnum, lump);
				free(name);
			}
			else if (memcmp(lump_p->name,"MAINCFG",8)==0) // Check for MAINCFG
			{
				CONS_Printf(M_GetText("Loading main config from %s\n"), wadfiles[wadnum]->filename);
				DEH_LoadDehackedLumpPwad(wadnum, lump);
			}
			else if (memcmp(lump_p->name,"OBJCTCFG",8)==0) // Check for OBJCTCFG
			{
				CONS_Printf(M_GetText("Loading object config from %s\n"), wadfiles[wadnum]->filename);
				DEH_LoadDehackedLumpPwad(wadnum, lump);
			}
	}

#ifdef SCANTHINGS
	// Scan maps for emblems 'n shit
	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
		{
			const char *name = lump_p->name;
			if (name[0] == 'M' && name[1] == 'A' && name[2] == 'P' && name[5]=='\0')
			{
				INT16 mapnum = (INT16)M_MapNumber(name[3], name[4]);
				P_ScanThings(mapnum, wadnum, lump + ML_THINGS);
			}
		}
	}
#endif
}

/** Compute MD5 message digest for bytes read from STREAM of this filname.
  *
  * The resulting message digest number will be written into the 16 bytes
  * beginning at RESBLOCK.
  *
  * \param filename path of file
  * \param resblock resulting MD5 checksum
  * \return 0 if MD5 checksum was made, and is at resblock, 1 if error was found
  */
static inline INT32 W_MakeFileMD5(const char *filename, void *resblock)
{
#ifdef NOMD5
	(void)filename;
	memset(resblock, 0x00, 16);
#else
	FILE *fhandle;

	if ((fhandle = fopen(filename, "rb")) != NULL)
	{
		tic_t t = I_GetTime();
		CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",filename);
		if (md5_stream(fhandle, resblock) == 1)
		{
			fclose(fhandle);
			return 1;
		}
		CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f seconds\n",
			filename, (float)(I_GetTime() - t)/NEWTICRATE);
		fclose(fhandle);
		return 0;
	}
#endif
	return 1;
}

// Invalidates the cache of lump numbers. Call this whenever a wad is added.
static void W_InvalidateLumpnumCache(void)
{
	memset(lumpnumcache, 0, sizeof (lumpnumcache));
}

//  Allocate a wadfile, setup the lumpinfo (directory) and
//  lumpcache, add the wadfile to the current active wadfiles
//
//  now returns index into wadfiles[], you can get wadfile_t *
//  with:
//       wadfiles[<return value>]
//
//  return -1 in case of problem
//
// Can now load dehacked files (.soc)
//
UINT16 W_InitFile(const char *filename)
{
	FILE *handle;
	lumpinfo_t *lumpinfo;
	wadfile_t *wadfile;
	restype_t type;
	UINT16 numlumps;
	size_t i;
	INT32 compressed = 0;
	size_t packetsize;
	UINT8 md5sum[16];
	boolean important;

	if (!(refreshdirmenu & REFRESHDIR_ADDFILE))
		refreshdirmenu = REFRESHDIR_NORMAL|REFRESHDIR_ADDFILE; // clean out cons_alerts that happened earlier

	//CONS_Debug(DBG_SETUP, "Loading %s\n", filename);
	//
	// check if limit of active wadfiles
	//
	if (numwadfiles >= MAX_WADFILES)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Maximum wad files reached\n"));
		refreshdirmenu |= REFRESHDIR_MAX;
		return INT16_MAX;
	}

	// open wad file
	if ((handle = W_OpenWadFile(&filename, true)) == NULL)
		return INT16_MAX;

	// Check if wad files will overflow fileneededbuffer. Only the filename part
	// is send in the packet; cf.
	// see PutFileNeeded in d_netfil.c
	if ((important = !W_VerifyNMUSlumps(filename)))
	{
		packetsize = packetsizetally;

		packetsize += nameonlylength(filename) + 22;

		if (packetsize > MAXFILENEEDED*sizeof(UINT8))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Maximum wad files reached\n"));
			refreshdirmenu |= REFRESHDIR_MAX;
			if (handle)
				fclose(handle);
			return INT16_MAX;
		}

		packetsizetally = packetsize;
	}

#ifndef NOMD5
	//
	// w-waiiiit!
	// Let's not add a wad file if the MD5 matches
	// an MD5 of an already added WAD file!
	//
	W_MakeFileMD5(filename, md5sum);

	for (i = 0; i < numwadfiles; i++)
	{
		if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
		{
			CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), filename);
			return INT16_MAX;
		}
	}
#endif

	// detect dehacked file with the "soc" extension
	if (!stricmp(&filename[strlen(filename) - 4], ".soc"))
	{
		// This code emulates a wadfile with one lump name "OBJCTCFG"
		// at position 0 and size of the whole file.
		// This allows soc files to be like all wads, copied by network and loaded at the console.
		type = RET_SOC;

		numlumps = 1;
		lumpinfo = Z_Calloc(sizeof (*lumpinfo), PU_STATIC, NULL);
		lumpinfo->position = 0;
		fseek(handle, 0, SEEK_END);
		lumpinfo->size = ftell(handle);
		fseek(handle, 0, SEEK_SET);
		strcpy(lumpinfo->name, "OBJCTCFG");
		// Allocate the lump's full name.
		lumpinfo->name2 = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
		strcpy(lumpinfo->name2, "OBJCTCFG");
		lumpinfo->name2[8] = '\0';
	}
#ifdef HAVE_BLUA
	// detect lua script with the "lua" extension
	else if (!stricmp(&filename[strlen(filename) - 4], ".lua"))
	{
		// This code emulates a wadfile with one lump name "LUA_INIT"
		// at position 0 and size of the whole file.
		// This allows soc files to be like all wads, copied by network and loaded at the console.
		type = RET_LUA;

		numlumps = 1;
		lumpinfo = Z_Calloc(sizeof (*lumpinfo), PU_STATIC, NULL);
		lumpinfo->position = 0;
		fseek(handle, 0, SEEK_END);
		lumpinfo->size = ftell(handle);
		fseek(handle, 0, SEEK_SET);
		strcpy(lumpinfo->name, "LUA_INIT");
		// Allocate the lump's full name.
		lumpinfo->name2 = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
		strcpy(lumpinfo->name2, "LUA_INIT");
		lumpinfo->name2[8] = '\0';
	}
#endif
	else if (!stricmp(&filename[strlen(filename) - 4], ".pk3"))
	{
		char curHeader[4];
		unsigned long size;
		char seekPat[] = {0x50, 0x4b, 0x01, 0x02, 0x00};
		char endPat[] = {0x50, 0x4b, 0x05, 0x06, 0x00};
		char *s;
		int c;
		UINT32 position;
		boolean matched = false;
		lumpinfo_t *lump_p;

		type = RET_PK3;

		// Obtain the file's size.
		fseek(handle, 0, SEEK_END);
		size = ftell(handle);
		CONS_Debug(DBG_SETUP, "PK3 size is: %ld\n", size);

		// We must look for the central directory through the file. (Thanks to JTE for this algorithm.)
		// All of the central directory entry headers have a signature of 0x50 0x4b 0x01 0x02.
		// The first entry found means the beginning of the central directory.
		fseek(handle, 0-min(size, (22 + 65536)), SEEK_CUR);
		s = endPat;
		while((c = fgetc(handle)) != EOF)
		{
			if (*s != c && s > endPat) // No match?
				s = endPat; // We "reset" the counter by sending the s pointer back to the start of the array.
			if (*s == c)
			{
				s++;
				if (*s == 0x00) // The array pointer has reached the key char which marks the end. It means we have matched the signature.
				{
					matched = true;
					CONS_Debug(DBG_SETUP, "Found PK3 central directory at position %ld.\n", ftell(handle));
					break;
				}
			}
		}

		// Error if we couldn't find the central directory at all. It likely means this is not a ZIP/PK3 file.
		if (matched == false)
		{
			CONS_Alert(CONS_ERROR, "No central directory inside PK3! File may be corrupted or incomplete.\n");
			return INT16_MAX;
		}

		fseek(handle, 4, SEEK_CUR);
		fread(&numlumps, 1, 2, handle);
		fseek(handle, 6, SEEK_CUR);
		fread(&position, 1, 4, handle);
		lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL);
		fseek(handle, position, SEEK_SET);

		// Since we found the central directory, now we can map our lumpinfo table.
		// We will look for file headers inside it, until we reach the central directory end signature.
		// We exactly know what data to expect this time, so now we don't need to do a byte-by-byte search.
		CONS_Debug(DBG_SETUP, "Now finding central directory file headers...\n");
		for (i = 0; i < numlumps; i++, lump_p++)
		{
			fread(curHeader, 1, 4, handle);

			// We found a central directory entry signature?
			if (!strncmp(curHeader, seekPat, 3))
			{
				// Let's fill in the fields that we actually need.
				// (Declaring all those vars might not be the optimal way to do this, sorry.)
				char *eName;
				int namePos;
				int nameEnd;
				unsigned short int eNameLen = 8;
				unsigned short int eXFieldLen = 0;
				unsigned short int lNameLen = 0;
				unsigned short int lXFieldLen = 0;
				unsigned short int eCommentLen = 0;
				unsigned short int eCompression = 0;
				unsigned int eSize = 0;
				unsigned int eCompSize = 0;
				unsigned int eLocalHeaderOffset = 0;
				unsigned long int rememberPos = 0;

				// We get the compression type indicator value.
				fseek(handle, 6, SEEK_CUR);
				fread(&eCompression, 1, 2, handle);
				// Get the size
				fseek(handle, 8, SEEK_CUR);
				fread(&eCompSize, 1, 4, handle);
				fread(&eSize, 1, 4, handle);
				// We get the variable length fields.
				fread(&eNameLen, 1, 2, handle);
				fread(&eXFieldLen, 1, 2, handle);
				fread(&eCommentLen, 1, 2, handle);
				fseek(handle, 8, SEEK_CUR);
				fread(&eLocalHeaderOffset, 1, 4, handle); // Get the offset.

				eName = malloc(sizeof(char)*(eNameLen + 1));
				fgets(eName, eNameLen + 1, handle);

				// Don't load lump if folder.
//				if (*(eName + eNameLen - 1) == '/')
//					continue;

				// We must calculate the position for the actual data.
				// Why not eLocalHeaderOffset + 30 + eNameLen + eXFieldLen? That's because the extra field and name lengths MAY be different in the local headers.
				rememberPos = ftell(handle);
				fseek(handle, eLocalHeaderOffset + 26, SEEK_SET);
				fread(&lNameLen, 1, 2, handle);
				fread(&lXFieldLen, 1, 2, handle);
				lump_p->position = ftell(handle) + lNameLen + lXFieldLen;

				fseek(handle, rememberPos, SEEK_SET); // Let's go back to the central dir.
				lump_p->disksize = eCompSize;
				lump_p->size = eSize;

				// We will trim the file's full name so that only the filename is left.
				namePos = eNameLen - 1;
				while(namePos--)
					if(eName[namePos] == '/')
						break;
				namePos++;
				// We will remove the file extension too.
				nameEnd = 0;
				while(nameEnd++ < 8)
					if(eName[namePos + nameEnd] == '.')
						break;
				memset(lump_p->name, '\0', 9);
				strncpy(lump_p->name, eName + namePos, nameEnd);

				lump_p->name2 = Z_Malloc((eNameLen+1)*sizeof(char), PU_STATIC, NULL);
				strncpy(lump_p->name2, eName, eNameLen);
				lump_p->name2[eNameLen] = '\0';

				// We set the compression type from what we're supporting so far.
				switch(eCompression)
				{
				case 0:
					lump_p->compression = CM_NOCOMPRESSION;
					break;
				case 8:
					lump_p->compression = CM_DEFLATE;
					break;
				case 14:
					lump_p->compression = CM_LZF;
					break;
				default:
					CONS_Alert(CONS_WARNING, "Lump has an unsupported compression type!\n");
					lump_p->compression = CM_UNSUPPORTED;
					break;
				}
				CONS_Debug(DBG_SETUP, "File %s, data begins at: %ld\n", eName, lump_p->position);
				fseek(handle, eXFieldLen + eCommentLen, SEEK_CUR); // We skip to where we expect the next central directory entry or end marker to be.
				free(eName);
			}
			// We found the central directory end signature?
			else if (!strncmp(curHeader, endPat, 4))
			{
				CONS_Debug(DBG_SETUP, "Central directory end signature found at: %ld\n", ftell(handle));

				/*// We will create a "virtual" marker lump at the very end of lumpinfo for convenience.
				// This marker will be used by the different lump-seeking (eg. textures, sprites, etc.) in PK3-specific cases in an auxiliary way.
				lumpinfo = (lumpinfo_t*) Z_Realloc(lumpinfo, (numlumps + 1)*sizeof(*lumpinfo), PU_STATIC, NULL);
				strcpy(lumpinfo[numlumps].name, "PK3_ENDM\0");
				lumpinfo[numlumps].name2 = Z_Malloc(14 * sizeof(char), PU_STATIC, NULL);
				strcpy(lumpinfo[numlumps].name2, "PK3_ENDMARKER\0");
				lumpinfo[numlumps].position = 0;
				lumpinfo[numlumps].size = 0;
				lumpinfo[numlumps].disksize = 0;
				lumpinfo[numlumps].compression = CM_NOCOMPRESSION;
				numlumps++;*/
				break;
			}
			// ... None of them? We're only expecting either a central directory signature entry or the central directory end signature.
			// The file may be broken or incomplete...
			else
			{
				CONS_Alert(CONS_WARNING, "Expected central directory header signature, got something else!");
				return INT16_MAX;
			}
		}
		// If we've reached this far, then it means our dynamically stored lumpinfo has to be ready.
		// Now we finally build our... incorrectly called wadfile.
		// TODO: Maybe we should give them more generalized names, like resourcefile or resfile or something.
		// Mostly for clarity and better understanding when reading the code.
	}
	// assume wad file
	else
	{
		wadinfo_t header;
		lumpinfo_t *lump_p;
		filelump_t *fileinfo;
		void *fileinfov;

		type = RET_WAD;

		// read the header
		if (fread(&header, 1, sizeof header, handle) < sizeof header)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Can't read wad header from %s because %s\n"), filename, strerror(ferror(handle)));
			return INT16_MAX;
		}

		if (memcmp(header.identification, "ZWAD", 4) == 0)
			compressed = 1;
		else if (memcmp(header.identification, "IWAD", 4) != 0
			&& memcmp(header.identification, "PWAD", 4) != 0
			&& memcmp(header.identification, "SDLL", 4) != 0)
		{
			CONS_Alert(CONS_ERROR, M_GetText("%s does not have a valid WAD header\n"), filename);
			return INT16_MAX;
		}

		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		// read wad file directory
		i = header.numlumps * sizeof (*fileinfo);
		fileinfov = fileinfo = malloc(i);
		if (fseek(handle, header.infotableofs, SEEK_SET) == -1
			|| fread(fileinfo, 1, i, handle) < i)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Wadfile directory in %s is corrupted (%s)\n"), filename, strerror(ferror(handle)));
			free(fileinfov);
			return INT16_MAX;
		}

		numlumps = header.numlumps;

		// fill in lumpinfo for this wad
		lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL);
		for (i = 0; i < numlumps; i++, lump_p++, fileinfo++)
		{
			lump_p->position = LONG(fileinfo->filepos);
			lump_p->size = lump_p->disksize = LONG(fileinfo->size);
			if (compressed) // wad is compressed, lump might be
			{
				UINT32 realsize = 0;

				if (fseek(handle, lump_p->position, SEEK_SET)
					== -1 || fread(&realsize, 1, sizeof realsize,
					handle) < sizeof realsize)
				{
					I_Error("corrupt compressed file: %s; maybe %s",
						filename, strerror(ferror(handle)));
				}
				realsize = LONG(realsize);
				if (realsize != 0)
				{
					lump_p->size = realsize;
					lump_p->compression = CM_LZF;
				}
				else
				{
					lump_p->size -= 4;
					lump_p->compression = CM_NOCOMPRESSION;
				}

				lump_p->position += 4;
				lump_p->disksize -= 4;
			}
			else
			{
				lump_p->compression = CM_NOCOMPRESSION;
			}
			memset(lump_p->name, 0x00, 9);
			strncpy(lump_p->name, fileinfo->name, 8);
			// Allocate the lump's full name.
			lump_p->name2 = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
			strncpy(lump_p->name2, fileinfo->name, 8);
			lump_p->name2[8] = '\0';
		}
		free(fileinfov);
	}

	//
	// link wad file to search files
	//
	wadfile = Z_Malloc(sizeof (*wadfile), PU_STATIC, NULL);
	wadfile->filename = Z_StrDup(filename);
	wadfile->type = type;
	wadfile->handle = handle;
	wadfile->numlumps = (UINT16)numlumps;
	wadfile->lumpinfo = lumpinfo;
	wadfile->important = important;
	fseek(handle, 0, SEEK_END);
	wadfile->filesize = (unsigned)ftell(handle);

	// already generated, just copy it over
	M_Memcpy(&wadfile->md5sum, &md5sum, 16);

	//
	// set up caching
	//
	Z_Calloc(numlumps * sizeof (*wadfile->lumpcache), PU_STATIC, &wadfile->lumpcache);

#ifdef HWRENDER
	// allocates GLPatch info structures and store them in a tree
	wadfile->hwrcache = M_AATreeAlloc(AATREE_ZUSER);
#endif

	//
	// add the wadfile
	//
	CONS_Printf(M_GetText("Added file %s (%u lumps)\n"), filename, numlumps);
	wadfiles[numwadfiles] = wadfile;
	numwadfiles++; // must come BEFORE W_LoadDehackedLumps, so any addfile called by COM_BufInsertText called by Lua doesn't overwrite what we just loaded

	// TODO: HACK ALERT - Load Lua & SOC stuff right here. I feel like this should be out of this place, but... Let's stick with this for now.
	switch (wadfile->type)
	{
		case RET_WAD:
			W_LoadDehackedLumps(numwadfiles - 1);
			break;
		case RET_PK3:
			W_LoadDehackedLumpsPK3(numwadfiles - 1);
			break;
		case RET_SOC:
			CONS_Printf(M_GetText("Loading SOC from %s\n"), wadfile->filename);
			DEH_LoadDehackedLumpPwad(numwadfiles - 1, 0);
			break;
		case RET_LUA:
			LUA_LoadLump(numwadfiles - 1, 0);
			break;
		default:
			break;
	}

	W_InvalidateLumpnumCache();

	return wadfile->numlumps;
}

/** Tries to load a series of files.
  * All files are wads unless they have an extension of ".soc" or ".lua".
  *
  * Each file is optional, but at least one file must be found or an error will
  * result. Lump names can appear multiple times. The name searcher looks
  * backwards, so a later file overrides all earlier ones.
  *
  * \param filenames A null-terminated list of files to use.
  * \return 1 if all files were loaded, 0 if at least one was missing or
  *           invalid.
  */
INT32 W_InitMultipleFiles(char **filenames)
{
	INT32 rc = 1;

	// open all the files, load headers, and count lumps
	numwadfiles = 0;

	// will be realloced as lumps are added
	for (; *filenames; filenames++)
	{
		//CONS_Debug(DBG_SETUP, "Loading %s\n", *filenames);
		rc &= (W_InitFile(*filenames) != INT16_MAX) ? 1 : 0;
	}

	if (!numwadfiles)
		I_Error("W_InitMultipleFiles: no files found");

	return rc;
}

/** Make sure a lump number is valid.
  * Compiles away to nothing if PARANOIA is not defined.
  */
static boolean TestValidLump(UINT16 wad, UINT16 lump)
{
	I_Assert(wad < MAX_WADFILES);
	if (!wadfiles[wad]) // make sure the wad file exists
		return false;

	I_Assert(lump < wadfiles[wad]->numlumps);
	if (lump >= wadfiles[wad]->numlumps) // make sure the lump exists
		return false;

	return true;
}


const char *W_CheckNameForNumPwad(UINT16 wad, UINT16 lump)
{
	if (lump >= wadfiles[wad]->numlumps || !TestValidLump(wad, 0))
		return NULL;

	return wadfiles[wad]->lumpinfo[lump].name;
}

const char *W_CheckNameForNum(lumpnum_t lumpnum)
{
	return W_CheckNameForNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

//
// Same as the original, but checks in one pwad only.
// wadid is a wad number
// (Used for sprites loading)
//
// 'startlump' is the lump number to start the search
//
UINT16 W_CheckNumForNamePwad(const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 i;
	static char uname[9];

	memset(uname, 0x00, sizeof uname);
	strncpy(uname, name, 8);
	uname[8] = 0;
	strupr(uname);

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	//
	// scan forward
	// start at 'startlump', useful parameter when there are multiple
	//                       resources with the same name
	//
	if (startlump < wadfiles[wad]->numlumps)
	{
		lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
			if (memcmp(lump_p->name,uname,8) == 0)
				return i;
	}

	// not found.
	return INT16_MAX;
}

// Look for the first lump from a folder.
UINT16 W_CheckNumForFolderStartPK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->name2, strlen(name)) == 0)
			break;
	}
	return i;
}

// In a PK3 type of resource file, it looks for the next lumpinfo entry that doesn't share the specified pathfile.
// Useful for finding folder ends.
// Returns the position of the lumpinfo entry.
UINT16 W_CheckNumForFolderEndPK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->name2, strlen(name)))
			break;
	}
	return i;
}

// In a PK3 type of resource file, it looks for an entry with the specified full name.
// Returns lump position in PK3's lumpinfo, or INT16_MAX if not found.
UINT16 W_CheckNumForFullNamePK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (!strnicmp(name, lump_p->name2, strlen(name)))
		{
			return i;
		}
	}
	// Not found at all?
	return INT16_MAX;
}

//
// W_CheckNumForName
// Returns LUMPERROR if name not found.
//
lumpnum_t W_CheckNumForName(const char *name)
{
	INT32 i;
	lumpnum_t check = INT16_MAX;

	// Check the lumpnumcache first. Loop backwards so that we check
	// most recent entries first
	for (i = lumpnumcacheindex + LUMPNUMCACHESIZE; i > lumpnumcacheindex; i--)
	{
		if (strncmp(lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumpname, name, 8) == 0)
		{
			lumpnumcacheindex = i & (LUMPNUMCACHESIZE - 1);
			return lumpnumcache[lumpnumcacheindex].lumpnum;
		}
	}

	// scan wad files backwards so patch lump files take precedence
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		check = W_CheckNumForNamePwad(name,(UINT16)i,0);
		if (check != INT16_MAX)
			break; //found it
	}

	if (check == INT16_MAX) return LUMPERROR;
	else
	{
		// Update the cache.
		lumpnumcacheindex = (lumpnumcacheindex + 1) & (LUMPNUMCACHESIZE - 1);
		strncpy(lumpnumcache[lumpnumcacheindex].lumpname, name, 8);
		lumpnumcache[lumpnumcacheindex].lumpnum = (i<<16)+check;

		return lumpnumcache[lumpnumcacheindex].lumpnum;
	}
}

// Look for valid map data through all added files in descendant order.
// Get a map marker for WADs, and a standalone WAD file lump inside PK3s.
// TODO: Make it search through cache first, maybe...?
lumpnum_t W_CheckNumForMap(const char *name)
{
	UINT16 lumpNum, end;
	UINT32 i;
	for (i = numwadfiles - 1; i < numwadfiles; i--)
	{
		if (wadfiles[i]->type == RET_WAD)
		{
			for (lumpNum = 0; lumpNum < wadfiles[i]->numlumps; lumpNum++)
				if (!strncmp(name, (wadfiles[i]->lumpinfo + lumpNum)->name, 8))
					return (i<<16) + lumpNum;
		}
		else if (wadfiles[i]->type == RET_PK3)
		{
			lumpNum = W_CheckNumForFolderStartPK3("maps/", i, 0);
			if (lumpNum != INT16_MAX)
				end = W_CheckNumForFolderEndPK3("maps/", i, lumpNum);
			else
				continue;
			// Now look for the specified map.
			for (++lumpNum; lumpNum < end; lumpNum++)
				if (!strnicmp(name, (wadfiles[i]->lumpinfo + lumpNum)->name, 8))
					return (i<<16) + lumpNum;
		}
	}
	return LUMPERROR;
}

//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
lumpnum_t W_GetNumForName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForName(name);

	if (i == LUMPERROR)
		I_Error("W_GetNumForName: %s not found!\n", name);

	return i;
}

//
// W_CheckNumForNameInBlock
// Checks only in blocks from blockstart lump to blockend lump
//
lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend)
{
	INT32 i;
	lumpnum_t bsid, beid;
	lumpnum_t check = INT16_MAX;

	// scan wad files backwards so patch lump files take precedence
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		if (wadfiles[i]->type == RET_WAD)
		{
			bsid = W_CheckNumForNamePwad(blockstart, (UINT16)i, 0);
			if (bsid == INT16_MAX)
				continue; // Start block doesn't exist?
			beid = W_CheckNumForNamePwad(blockend, (UINT16)i, 0);
			if (beid == INT16_MAX)
				continue; // End block doesn't exist?

			check = W_CheckNumForNamePwad(name, (UINT16)i, bsid);
			if (check < beid)
				return (i<<16)+check; // found it, in our constraints
		}

	}
	return LUMPERROR;
}

// Used by Lua. Case sensitive lump checking, quickly...
#include "fastcmp.h"
UINT8 W_LumpExists(const char *name)
{
	INT32 i,j;
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		lumpinfo_t *lump_p = wadfiles[i]->lumpinfo;
		for (j = 0; j < wadfiles[i]->numlumps; ++j, ++lump_p)
			if (fastcmp(lump_p->name,name))
				return true;
	}
	return false;
}

size_t W_LumpLengthPwad(UINT16 wad, UINT16 lump)
{
	if (!TestValidLump(wad, lump))
		return 0;
	return wadfiles[wad]->lumpinfo[lump].size;
}

/** Returns the buffer size needed to load the given lump.
  *
  * \param lump Lump number to look at.
  * \return Buffer size needed, in bytes.
  */
size_t W_LumpLength(lumpnum_t lumpnum)
{
	return W_LumpLengthPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

//
// W_IsLumpWad
// Is the lump a WAD? (presumably in a PK3)
//
boolean W_IsLumpWad(lumpnum_t lumpnum)
{
	if (wadfiles[WADFILENUM(lumpnum)]->type == RET_PK3)
	{
		const char *lumpfullName = (wadfiles[WADFILENUM(lumpnum)]->lumpinfo + LUMPNUM(lumpnum))->name2;

		if (strlen(lumpfullName) < 4)
			return false; // can't possibly be a WAD can it?
		return !strnicmp(lumpfullName + strlen(lumpfullName) - 4, ".wad", 4);
	}

	return false; // WADs should never be inside non-PK3s as far as SRB2 is concerned
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    CONS_Printf("zpipe: ");
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            CONS_Printf("error reading stdin\n");
        if (ferror(stdout))
            CONS_Printf("error writing stdout\n");
        break;
    case Z_STREAM_ERROR:
        CONS_Printf("invalid compression level\n");
        break;
    case Z_DATA_ERROR:
        CONS_Printf("invalid or incomplete deflate data\n");
        break;
    case Z_MEM_ERROR:
        CONS_Printf("out of memory\n");
        break;
    case Z_VERSION_ERROR:
        CONS_Printf("zlib version mismatch!\n");
    }
}

/** Reads bytes from the head of a lump.
  * Note: If the lump is compressed, the whole thing has to be read anyway.
  *
  * \param wad Wad number to read from.
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination.
  * \param size Number of bytes to read.
  * \param offest Number of bytes to offset.
  * \return Number of bytes read (should equal size).
  * \sa W_ReadLump, W_RawReadLumpHeader
  */
size_t W_ReadLumpHeaderPwad(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset)
{
	size_t lumpsize;
	lumpinfo_t *l;
	FILE *handle;

	if (!TestValidLump(wad,lump))
		return 0;

	lumpsize = wadfiles[wad]->lumpinfo[lump].size;
	// empty resource (usually markers like S_START, F_END ..)
	if (!lumpsize || lumpsize<offset)
		return 0;

	// zero size means read all the lump
	if (!size || size+offset > lumpsize)
		size = lumpsize - offset;

	// Let's get the raw lump data.
	// We setup the desired file handle to read the lump data.
	l = wadfiles[wad]->lumpinfo + lump;
	handle = wadfiles[wad]->handle;
	fseek(handle, (long)(l->position + offset), SEEK_SET);

	// But let's not copy it yet. We support different compression formats on lumps, so we need to take that into account.
	switch(wadfiles[wad]->lumpinfo[lump].compression)
	{
	case CM_NOCOMPRESSION:		// If it's uncompressed, we directly write the data into our destination, and return the bytes read.
		return fread(dest, 1, size, handle);
	case CM_LZF:		// Is it LZF compressed? Used by ZWADs.
		{
#ifdef ZWAD
			char *rawData; // The lump's raw data.
			char *decData; // Lump's decompressed real data.
			size_t retval; // Helper var, lzf_decompress returns 0 when an error occurs.

			rawData = Z_Malloc(l->disksize, PU_STATIC, NULL);
			decData = Z_Malloc(l->size, PU_STATIC, NULL);

			if (fread(rawData, 1, l->disksize, handle) < l->disksize)
				I_Error("wad %d, lump %d: cannot read compressed data", wad, lump);
			retval = lzf_decompress(rawData, l->disksize, decData, l->size);
#ifndef AVOID_ERRNO
			if (retval == 0) // If this was returned, check if errno was set
			{
				// errno is a global var set by the lzf functions when something goes wrong.
				if (errno == E2BIG)
					I_Error("wad %d, lump %d: compressed data too big (bigger than %s)", wad, lump, sizeu1(l->size));
				else if (errno == EINVAL)
					I_Error("wad %d, lump %d: invalid compressed data", wad, lump);
			}
			// Otherwise, fall back on below error (if zero was actually the correct size then ???)
#endif
			if (retval != l->size)
			{
				I_Error("wad %d, lump %d: decompressed to wrong number of bytes (expected %s, got %s)", wad, lump, sizeu1(l->size), sizeu2(retval));
			}
			if (!decData) // Did we get no data at all?
				return 0;
			M_Memcpy(dest, decData + offset, size);
			Z_Free(rawData);
			Z_Free(decData);
			return size;
#else
			//I_Error("ZWAD files not supported on this platform.");
			return 0;
#endif
		}
	case CM_DEFLATE: // Is it compressed via DEFLATE? Very common in ZIPs/PK3s, also what most doom-related editors support.
		{
			z_const Bytef *rawData; // The lump's raw data.
			Bytef *decData; // Lump's decompressed real data.

			int zErr; // Helper var.
			z_stream strm;
			unsigned long rawSize = l->disksize;
			unsigned long decSize = l->size;

			rawData = Z_Malloc(rawSize, PU_STATIC, NULL);
			decData = Z_Malloc(decSize, PU_STATIC, NULL);

			if (fread(rawData, 1, rawSize, handle) < rawSize)
				I_Error("wad %d, lump %d: cannot read compressed data", wad, lump);

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;

			strm.total_in = strm.avail_in = rawSize;
			strm.total_out = strm.avail_out = decSize;

			strm.next_in = rawData;
			strm.next_out = decData;

			zErr = inflateInit2(&strm, -15);
			if (zErr == Z_OK)
			{
				zErr = inflate(&strm, Z_FINISH);
				if (zErr == Z_STREAM_END)
				{
					M_Memcpy(dest, decData, size);
				}
				else
				{
					size = 0;
					zerr(zErr);
					(void)inflateEnd(&strm);
				}
			}
			else
			{
				CONS_Printf("whopet\n");
				size = 0;
				zerr(zErr);
			}

			Z_Free(rawData);
			Z_Free(decData);

			return size;
		}
	default:
		I_Error("wad %d, lump %d: unsupported compression type!", wad, lump);
	}
	return 0;
}

size_t W_ReadLumpHeader(lumpnum_t lumpnum, void *dest, size_t size, size_t offset)
{
	return W_ReadLumpHeaderPwad(WADFILENUM(lumpnum), LUMPNUM(lumpnum), dest, size, offset);
}

/** Reads a lump into memory.
  *
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination. Size must be >=
  *             W_LumpLength().
  * \sa W_ReadLumpHeader
  */
void W_ReadLump(lumpnum_t lumpnum, void *dest)
{
	W_ReadLumpHeaderPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),dest,0,0);
}

void W_ReadLumpPwad(UINT16 wad, UINT16 lump, void *dest)
{
	W_ReadLumpHeaderPwad(wad, lump, dest, 0, 0);
}

// ==========================================================================
// W_CacheLumpNum
// ==========================================================================
void *W_CacheLumpNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	lumpcache_t *lumpcache;

	if (!TestValidLump(wad,lump))
		return NULL;

	lumpcache = wadfiles[wad]->lumpcache;
	if (!lumpcache[lump])
	{
		void *ptr = Z_Malloc(W_LumpLengthPwad(wad, lump), tag, &lumpcache[lump]);
		W_ReadLumpHeaderPwad(wad, lump, ptr, 0, 0);  // read the lump in full
	}
	else
		Z_ChangeTag(lumpcache[lump], tag);

	return lumpcache[lump];
}

void *W_CacheLumpNum(lumpnum_t lumpnum, INT32 tag)
{

	return W_CacheLumpNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}

//
// W_CacheLumpNumForce
//
// Forces the lump to be loaded, even if it already is!
//
void *W_CacheLumpNumForce(lumpnum_t lumpnum, INT32 tag)
{
	UINT16 wad, lump;
	void *ptr;

	wad = WADFILENUM(lumpnum);
	lump = LUMPNUM(lumpnum);

	if (!TestValidLump(wad,lump))
		return NULL;

	ptr = Z_Malloc(W_LumpLengthPwad(wad, lump), tag, NULL);
	W_ReadLumpHeaderPwad(wad, lump, ptr, 0, 0);  // read the lump in full

	return ptr;
}

//
// W_IsLumpCached
//
// If a lump is already cached return true, otherwise
// return false.
//
// no outside code uses the PWAD form, for now
static inline boolean W_IsLumpCachedPWAD(UINT16 wad, UINT16 lump, void *ptr)
{
	void *lcache;

	if (!TestValidLump(wad, lump))
		return false;

	lcache = wadfiles[wad]->lumpcache[lump];

	if (ptr)
	{
		if (ptr == lcache)
			return true;
	}
	else if (lcache)
		return true;

	return false;
}

boolean W_IsLumpCached(lumpnum_t lumpnum, void *ptr)
{
	return W_IsLumpCachedPWAD(WADFILENUM(lumpnum),LUMPNUM(lumpnum), ptr);
}

// ==========================================================================
// W_CacheLumpName
// ==========================================================================
void *W_CacheLumpName(const char *name, INT32 tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current rendermode. For software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, and
// the heap memory cache then acts as a 'level 2' cache just after the
// graphics card memory.

//
// Cache a patch into heap memory, convert the patch format as necessary
//

// Software-only compile cache the data without conversion
#ifdef HWRENDER
static inline void *W_CachePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	GLPatch_t *grPatch;

	if (rendermode == render_soft || rendermode == render_none)
		return W_CacheLumpNumPwad(wad, lump, tag);

	if (!TestValidLump(wad, lump))
		return NULL;

	grPatch = HWR_GetCachedGLPatchPwad(wad, lump);

	if (grPatch->mipmap.grInfo.data)
	{
		if (tag == PU_CACHE)
			tag = PU_HWRCACHE;
		Z_ChangeTag(grPatch->mipmap.grInfo.data, tag);
	}
	else
	{
		patch_t *ptr = NULL;

		// Only load the patch if we haven't initialised the grPatch yet
		if (grPatch->mipmap.width == 0)
			ptr = W_CacheLumpNumPwad(grPatch->wadnum, grPatch->lumpnum, PU_STATIC);

		// Run HWR_MakePatch in all cases, to recalculate some things
		HWR_MakePatch(ptr, grPatch, &grPatch->mipmap, false);
		Z_Free(ptr);
	}

	// return GLPatch_t, which can be casted to (patch_t) with valid patch header info
	return (void *)grPatch;
}

void *W_CachePatchNum(lumpnum_t lumpnum, INT32 tag)
{
	return W_CachePatchNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}

#endif // HWRENDER

void W_UnlockCachedPatch(void *patch)
{
	// The hardware code does its own memory management, as its patches
	// have different lifetimes from software's.
#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
		HWR_UnlockCachedPatch((GLPatch_t*)patch);
	else
#endif
		Z_Unlock(patch);
}

void *W_CachePatchName(const char *name, INT32 tag)
{
	lumpnum_t num;

	num = W_CheckNumForName(name);

	if (num == LUMPERROR)
		return W_CachePatchNum(W_GetNumForName("MISSING"), tag);
	return W_CachePatchNum(num, tag);
}
#ifndef NOMD5
#define MD5_LEN 16

/**
  * Prints an MD5 string into a human-readable textual format.
  *
  * \param md5 The md5 in binary form -- MD5_LEN (16) bytes.
  * \param buf Where to print the textual form. Needs 2*MD5_LEN+1 (33) bytes.
  * \author Graue <graue@oceanbase.org>
  */
#define MD5_FORMAT \
	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
static void PrintMD5String(const UINT8 *md5, char *buf)
{
	snprintf(buf, 2*MD5_LEN+1, MD5_FORMAT,
		md5[0], md5[1], md5[2], md5[3],
		md5[4], md5[5], md5[6], md5[7],
		md5[8], md5[9], md5[10], md5[11],
		md5[12], md5[13], md5[14], md5[15]);
}
#endif
/** Verifies a file's MD5 is as it should be.
  * For releases, used as cheat prevention -- if the MD5 doesn't match, a
  * fatal error is thrown. In debug mode, an MD5 mismatch only triggers a
  * warning.
  *
  * \param wadfilenum Number of the loaded wad file to check.
  * \param matchmd5   The MD5 sum this wad should have, expressed as a
  *                   textual string.
  * \author Graue <graue@oceanbase.org>
  */
void W_VerifyFileMD5(UINT16 wadfilenum, const char *matchmd5)
{
#ifdef NOMD5
	(void)wadfilenum;
	(void)matchmd5;
#else
	UINT8 realmd5[MD5_LEN];
	INT32 ix;

	I_Assert(strlen(matchmd5) == 2*MD5_LEN);
	I_Assert(wadfilenum < numwadfiles);
	// Convert an md5 string like "7d355827fa8f981482246d6c95f9bd48"
	// into a real md5.
	for (ix = 0; ix < 2*MD5_LEN; ix++)
	{
		INT32 n, c = matchmd5[ix];
		if (isdigit(c))
			n = c - '0';
		else
		{
			I_Assert(isxdigit(c));
			if (isupper(c)) n = c - 'A' + 10;
			else n = c - 'a' + 10;
		}
		if (ix & 1) realmd5[ix>>1] = (UINT8)(realmd5[ix>>1]+n);
		else realmd5[ix>>1] = (UINT8)(n<<4);
	}

	if (memcmp(realmd5, wadfiles[wadfilenum]->md5sum, 16))
	{
		char actualmd5text[2*MD5_LEN+1];
		PrintMD5String(wadfiles[wadfilenum]->md5sum, actualmd5text);
#ifdef _DEBUG
		CONS_Printf
#else
		I_Error
#endif
			(M_GetText("File is corrupt or has been modified: %s (found md5: %s, wanted: %s)\n"), wadfiles[wadfilenum]->filename, actualmd5text, matchmd5);
	}
#endif
}

// Note: This never opens lumps themselves and therefore doesn't have to
// deal with compressed lumps.
static int W_VerifyFile(const char *filename, lumpchecklist_t *checklist,
	boolean status)
{
	FILE *handle;
	size_t i, j;
	int goodfile = false;

	if (!checklist)
		I_Error("No checklist for %s\n", filename);
	// open wad file
	if ((handle = W_OpenWadFile(&filename, false)) == NULL)
		return -1;

	// detect wad file by the absence of the other supported extensions
	if (stricmp(&filename[strlen(filename) - 4], ".soc")
#ifdef HAVE_BLUA
	&& stricmp(&filename[strlen(filename) - 4], ".lua")
#endif
	&& stricmp(&filename[strlen(filename) - 4], ".pk3"))
	{
		// assume wad file
		wadinfo_t header;
		filelump_t lumpinfo;

		// read the header
		if (fread(&header, 1, sizeof header, handle) == sizeof header
			&& header.numlumps < INT16_MAX
			&& strncmp(header.identification, "ZWAD", 4)
			&& strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
		{
			fclose(handle);
			return true;
		}

		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		// let seek to the lumpinfo list
		if (fseek(handle, header.infotableofs, SEEK_SET) == -1)
		{
			fclose(handle);
			return false;
		}

		goodfile = true;
		for (i = 0; i < header.numlumps; i++)
		{
			// fill in lumpinfo for this wad file directory
			if (fread(&lumpinfo, sizeof (lumpinfo), 1 , handle) != 1)
			{
				fclose(handle);
				return -1;
			}

			lumpinfo.filepos = LONG(lumpinfo.filepos);
			lumpinfo.size = LONG(lumpinfo.size);

			if (lumpinfo.size == 0)
				continue;

			for (j = 0; j < NUMSPRITES; j++)
				if (sprnames[j] && !strncmp(lumpinfo.name, sprnames[j], 4)) // Sprites
					continue;

			goodfile = false;
			for (j = 0; checklist[j].len && checklist[j].name && !goodfile; j++)
				if ((strncmp(lumpinfo.name, checklist[j].name, checklist[j].len) != false) == status)
					goodfile = true;

			if (!goodfile)
				break;
		}
	}
	fclose(handle);
	return goodfile;
}


/** Checks a wad for lumps other than music and sound.
  * Used during game load to verify music.dta is a good file and during a
  * netgame join (on the server side) to see if a wad is important enough to
  * be sent.
  *
  * \param filename Filename of the wad to check.
  * \return 1 if file contains only music/sound lumps, 0 if it contains other
  *         stuff (maps, sprites, dehacked lumps, and so on). -1 if there no
  *         file exists with that filename
  * \author Alam Arias
  */
int W_VerifyNMUSlumps(const char *filename)
{
	// MIDI, MOD/S3M/IT/XM/OGG/MP3/WAV, WAVE SFX
	// ENDOOM text and palette lumps
	lumpchecklist_t NMUSlist[] =
	{
		{"D_", 2}, // MIDI music
		{"O_", 2}, // Digital music
		{"DS", 2}, // Sound effects

		{"ENDOOM", 6}, // ENDOOM text lump

		{"PLAYPAL", 7}, // Palette changes
		{"PAL", 3}, // Palette changes
		{"COLORMAP", 8}, // Colormap changes
		{"CLM", 3}, // Colormap changes
		{"TRANS", 5}, // Translucency map changes

		{"LTFNT", 5}, // Level title font changes
		{"TTL", 3}, // Act number changes
		{"STCFN", 5}, // Console font changes
		{"TNYFN", 5}, // Tiny console font changes
		{"STT", 3}, // Acceptable HUD changes (Score Time Rings)
		{"YB_", 3}, // Intermission graphics, goes with the above
		{"M_", 2}, // As does menu stuff

		{NULL, 0},
	};
	return W_VerifyFile(filename, NMUSlist, false);
}
