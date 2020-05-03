// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  z_zone.h
/// \brief Zone Memory Allocation, perhaps NeXT ObjectiveC inspired

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>
#include "doomtype.h"

#ifdef __GNUC__ // __attribute__ ((X))
#if (__GNUC__ > 4) || (__GNUC__ == 4 && (__GNUC_MINOR__ >= 3 || (__GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ >= 5)))
#define FUNCALLOC(X) __attribute__((alloc_size(X)))
#endif // odd, it is documented in GCC 4.3.0 but it exists in 4.2.4, at least
#endif

#ifndef FUNCALLOC
#define FUNCALLOC(x)
#endif

//#define ZDEBUG

//
// Purge tags
//
// Now they are an enum! -- Monster Iestyn 15/02/18
//
enum
{
	// Tags < PU_LEVEL are not purged until freed explicitly.
	PU_STATIC                = 1, // static entire execution time
	PU_LUA                   = 2, // static entire execution time -- used by lua so it doesn't get caught in loops forever

	PU_SOUND                 = 11, // static while playing
	PU_MUSIC                 = 12, // static while playing
	PU_HUDGFX                = 13, // static until WAD added
	PU_PATCH                 = 14, // static until renderer change

	PU_HWRPATCHINFO          = 21, // Hardware GLPatch_t struct for OpenGL texture cache
	PU_HWRPATCHCOLMIPMAP     = 22, // Hardware GLMipmap_t struct colormap variation of patch
	PU_HWRMODELTEXTURE       = 23, // Hardware model texture

	PU_HWRCACHE              = 48, // static until unlocked
	PU_CACHE                 = 49, // static until unlocked

	// Tags s.t. PU_LEVEL <= tag < PU_PURGELEVEL are purged at level start
	PU_LEVEL                 = 50, // static until level exited
	PU_LEVSPEC               = 51, // a special thinker in a level
	PU_HWRPLANE              = 52, // if ZPLANALLOC is enabled in hw_bsp.c, this is used to alloc polygons for OpenGL

	// Tags >= PU_PURGELEVEL are purgable whenever needed
	PU_PURGELEVEL            = 100, // Note: this is never actually used as a tag
	PU_CACHE_UNLOCKED        = 101, // Note: unused
	PU_HWRCACHE_UNLOCKED     = 102, // 'unlocked' PU_HWRCACHE memory:
									// 'second-level' cache for graphics
                                    // stored in hardware format and downloaded as needed
	PU_HWRPATCHINFO_UNLOCKED = 103, // 'unlocked' PU_HWRPATCHINFO memory
	PU_HWRMODELTEXTURE_UNLOCKED = 104, // 'unlocked' PU_HWRMODELTEXTURE memory
};

//
// Zone memory initialisation
//
void Z_Init(void);

//
// Zone memory allocation
//
// enable ZDEBUG to get the file + line the functions were called from
// for ZZ_Alloc, see doomdef.h
//

// Z_Free and alloc with alignment
#ifdef ZDEBUG
#define Z_Free(p)                 Z_Free2(p, __FILE__, __LINE__)
#define Z_MallocAlign(s,t,u,a)    Z_Malloc2(s, t, u, a, __FILE__, __LINE__)
#define Z_CallocAlign(s,t,u,a)    Z_Calloc2(s, t, u, a, __FILE__, __LINE__)
#define Z_ReallocAlign(p,s,t,u,a) Z_Realloc2(p,s, t, u, a, __FILE__, __LINE__)
void Z_Free2(void *ptr, const char *file, INT32 line);
void *Z_Malloc2(size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line) FUNCALLOC(1);
void *Z_Calloc2(size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line) FUNCALLOC(1);
void *Z_Realloc2(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line) FUNCALLOC(2);
#else
void Z_Free(void *ptr);
void *Z_MallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits) FUNCALLOC(1);
void *Z_CallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits) FUNCALLOC(1);
void *Z_ReallocAlign(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits) FUNCALLOC(2);
#endif

// Alloc with no alignment
#define Z_Malloc(s,t,u)    Z_MallocAlign(s, t, u, 0)
#define Z_Calloc(s,t,u)    Z_CallocAlign(s, t, u, 0)
#define Z_Realloc(p,s,t,u) Z_ReallocAlign(p, s, t, u, 0)

// Free all memory by tag
// these don't give line numbers for ZDEBUG currently though
// (perhaps this should be changed in future?)
#define Z_FreeTag(tagnum) Z_FreeTags(tagnum, tagnum)
void Z_FreeTags(INT32 lowtag, INT32 hightag);

//
// Utility functions
//
void Z_CheckMemCleanup(void);
void Z_CheckHeap(INT32 i);

//
// Zone memory modification
//
// enable PARANOIA to get the file + line the functions were called from
//
#ifdef PARANOIA
#define Z_ChangeTag(p,t) Z_ChangeTag2(p, t, __FILE__, __LINE__)
#define Z_SetUser(p,u)   Z_SetUser2(p, u, __FILE__, __LINE__)
void Z_ChangeTag2(void *ptr, INT32 tag, const char *file, INT32 line);
void Z_SetUser2(void *ptr, void **newuser, const char *file, INT32 line);
#else
void Z_ChangeTag(void *ptr, INT32 tag);
void Z_SetUser(void *ptr, void **newuser);
#endif

//
// Zone memory usage
//
// Note: These give the memory used in bytes,
// shift down by 10 to convert to KB
//
#define Z_TagUsage(tagnum) Z_TagsUsage(tagnum, tagnum)
size_t Z_TagsUsage(INT32 lowtag, INT32 hightag);
#define Z_TotalUsage() Z_TagsUsage(0, INT32_MAX)

//
// Miscellaneous functions
//
char *Z_StrDup(const char *in);
#define Z_Unlock(p) (void)p // TODO: remove this now that NDS code has been removed

// For renderer switching
extern boolean needpatchflush;
extern boolean needpatchrecache;
void Z_FlushCachedPatches(void);
void Z_PreparePatchFlush(void);

#endif
