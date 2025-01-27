// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by Graue.
// Copyright (C) 2006-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  z_zone.c
/// \brief Zone memory allocation.
///        This file does zone memory allocation. Each allocation is done with a
///        tag, and this file keeps track of all the allocations made. Later, you
///        can purge everything with a given tag.
///
///        Some tags (PU_CACHE, for example) may be automatically purged whenever
///        the space is needed, so memory allocated with these tags is no longer
///        guaranteed to be valid after another call to Z_Malloc().
///
///        The original implementation allocated a large block (48 MB, as of the
///        last version of SRB2 that did this) upfront, and Z_Malloc() carved
///        pieces out of that. Unfortunately, this had the effect of masking a
///        lot of read/write past end of buffer type bugs which we have since
///        caught with this direct-malloc version. We also suspected that SRB2's
///        allocator was fragmenting badly. Finally, this version is a bit
///        simpler (about half the lines of code).

#include "doomdef.h"
#include "doomstat.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "i_system.h" // I_GetFreeMem
#include "i_video.h" // rendermode
#include "z_zone.h"
#include "m_misc.h" // M_Memcpy
#include "lua_script.h"

#ifdef HWRENDER
#include "hardware/hw_main.h" // For hardware memory info
#endif

#ifdef HAVE_VALGRIND
#include "valgrind.h"
static boolean Z_calloc = false;
#include "memcheck.h"
#endif

#define ZONEID 0xa441d13d

#ifdef ZDEBUG
//#define ZDEBUG2
#endif

typedef struct memblock_s
{
	void **user;
	INT32 tag; // purgelevel
	UINT32 id; // Should be ZONEID

	size_t size; // including the header and blocks
	size_t realsize; // size of real data only

#ifdef ZDEBUG
	const char *ownerfile;
	INT32 ownerline;
#endif

	struct memblock_s *next, *prev;
} memblock_t;

#define MEMORY(x) (void *)((uintptr_t)(x) + sizeof(memblock_t))
#define MEMBLOCK(x) (memblock_t *)((uintptr_t)(x) - sizeof(memblock_t))

// both the head and tail of the zone memory block list
static memblock_t head;

//
// Function prototypes
//
static void Command_Memfree_f(void);
#ifdef ZDEBUG
static void Command_Memdump_f(void);
#endif

// --------------------------
// Zone memory initialisation
// --------------------------

/** Initialises zone memory.
  * Used at game startup.
  *
  * \sa I_GetFreeMem, Command_Memfree_f, Command_Memdump_f
  */
void Z_Init(void)
{
	size_t total, memfree;

	memset(&head, 0x00, sizeof(head));

	head.next = head.prev = &head;

	memfree = I_GetFreeMem(&total)>>20;
	CONS_Printf("System memory: %sMB - Free: %sMB\n", sizeu1(total>>20), sizeu2(memfree));

	// Note: This allocates memory. Watch out.
	COM_AddCommand("memfree", Command_Memfree_f, COM_LUA);

#ifdef ZDEBUG
	COM_AddCommand("memdump", Command_Memdump_f, COM_LUA);
#endif
}


// ----------------------
// Zone memory allocation
// ----------------------

/** Frees allocated memory.
  *
  * \param ptr A pointer to allocated memory,
  *             assumed to have been allocated with Z_Malloc/Z_Calloc.
  * \sa Z_FreeTags
  */
#ifdef ZDEBUG
void Z_Free2(void *ptr, const char *file, INT32 line)
#else
void Z_Free(void *ptr)
#endif
{
	memblock_t *block;

	if (ptr == NULL)
		return;

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Free %s:%d\n", file, line);
#endif

	block = MEMBLOCK(ptr);
#ifdef PARANOIA
	if (block->id != ZONEID)
#ifdef ZDEBUG
		I_Error("Z_Free at %s:%d: wrong id", file, line);
#else
		I_Error("Z_Free: wrong id");
#endif
#endif

#ifdef ZDEBUG
	// Write every Z_Free call to a debug file.
	CONS_Debug(DBG_MEMORY, "Z_Free at %s:%d\n", file, line);
#endif

	// anything that isn't by lua gets passed to lua just in case.
	if (block->tag != PU_LUA)
		LUA_InvalidateUserdata(ptr);

	// TODO: if zdebugging, make sure no other block has a user
	// that is about to be freed.

	// Clear the user's mark.
	if (block->user != NULL)
		*block->user = NULL;

#ifdef VALGRIND_DESTROY_MEMPOOL
	VALGRIND_DESTROY_MEMPOOL(block);
#endif
	block->prev->next = block->next;
	block->next->prev = block->prev;
	free(block);
}

/** malloc() that doesn't accept failure.
  *
  * \param size Amount of memory to be allocated, in bytes.
  * \return A pointer to the allocated memory.
  */
static void *xm(size_t size)
{
	const size_t padedsize = size+sizeof (size_t);
	void *p;

	if (padedsize < size)/* overflow check */
		I_Error("You are allocating memory too large!");
	p = malloc(padedsize);

	if (p == NULL)
	{
		// Oh crumbs: we're out of heap. Try purging the cache and reallocating.
		Z_FreeTags(PU_PURGELEVEL, INT32_MAX);
		p = malloc(padedsize);

		if (p == NULL)
		{
			I_Error("Out of memory allocating %s bytes", sizeu1(size));
		}
	}

	return p;
}

/** The Z_MallocAlign function.
  * Allocates a block of memory, adds it to a linked list so we can keep track of it.
  *
  * \param size Amount of memory to be allocated, in bytes.
  * \param tag Purge tag.
  * \param user The address of a pointer to the memory to be allocated.
  *             When the memory is freed by Z_Free later,
  *             the pointer at this address will then be automatically set to NULL.
  * \param alignbits The alignment of the memory to be allocated, in bits. Can be 0.
  * \note You can pass Z_Malloc() a NULL user if the tag is less than PU_PURGELEVEL.
  * \sa Z_CallocAlign, Z_ReallocAlign
  */
#ifdef ZDEBUG
void *Z_Malloc2(size_t size, INT32 tag, void *user, INT32 alignbits,
	const char *file, INT32 line)
#else
void *Z_MallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
	memblock_t *block;
	void *ptr;
	(void)(alignbits); // no longer used, so silence warnings.

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Malloc %s:%d\n", file, line);
#endif

	block = xm(sizeof (memblock_t) + size);
	ptr = MEMORY(block);
	I_Assert((intptr_t)ptr % sizeof (void *) == 0);

#ifdef HAVE_VALGRIND
	Z_calloc = false;
#endif

	block->next = head.next;
	block->prev = &head;
	head.next = block;
	block->next->prev = block;

	block->tag = tag;
	block->user = NULL;
#ifdef ZDEBUG
	block->ownerline = line;
	block->ownerfile = file;
#endif
	block->size = sizeof (memblock_t) + size;
	block->realsize = size;

#ifdef VALGRIND_CREATE_MEMPOOL
	VALGRIND_CREATE_MEMPOOL(block, size, Z_calloc);
#endif

	block->id = ZONEID;

	if (user != NULL)
	{
		block->user = user;
		*(void **)user = ptr;
	}
	else if (tag >= PU_PURGELEVEL)
		I_Error("Z_Malloc: attempted to allocate purgable block "
			"(size %s) with no user", sizeu1(size));

	return ptr;
}

/** The Z_CallocAlign function.
  * Allocates a block of memory, adds it to a linked list so we can keep track of it.
  * Unlike Z_MallocAlign, this also initialises the bytes to zero.
  *
  * \param size Amount of memory to be allocated, in bytes.
  * \param tag Purge tag.
  * \param user The address of a pointer to the memory to be allocated.
  *             When the memory is freed by Z_Free later,
  *             the pointer at this address will then be automatically set to NULL.
  * \param alignbits The alignment of the memory to be allocated, in bits. Can be 0.
  * \note You can pass Z_Calloc() a NULL user if the tag is less than PU_PURGELEVEL.
  * \sa Z_MallocAlign, Z_ReallocAlign
  */
#ifdef ZDEBUG
void *Z_Calloc2(size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line)
#else
void *Z_CallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
#ifdef VALGRIND_MEMPOOL_ALLOC
	Z_calloc = true;
#endif
#ifdef ZDEBUG
	return memset(Z_Malloc2    (size, tag, user, alignbits, file, line), 0, size);
#else
	return memset(Z_MallocAlign(size, tag, user, alignbits            ), 0, size);
#endif
}

/** The Z_ReallocAlign function.
  * Reallocates a block of memory with a new size.
  *
  * \param ptr A pointer to allocated memory,
  *             assumed to have been allocated with Z_Malloc/Z_Calloc.
  *             If NULL, this function instead acts as a wrapper for Z_CallocAlign.
  * \param size New size of memory block, in bytes.
  *             If zero, then the memory is freed and NULL is returned.
  * \param tag New purge tag.
  * \param user The address of a pointer to the memory to be reallocated.
  *             This can be a different user to the one originally assigned to the memory block.
  * \param alignbits The alignment of the memory to be allocated, in bits. Can be 0.
  * \return A pointer to the reallocated memory. Can be NULL if memory was freed.
  * \note You can pass Z_Realloc() a NULL user if the tag is less than PU_PURGELEVEL.
  * \sa Z_MallocAlign, Z_CallocAlign
  */
#ifdef ZDEBUG
void *Z_Realloc2(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line)
#else
void *Z_ReallocAlign(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
	void *rez;
	memblock_t *block;
	size_t copysize;

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Realloc %s:%d\n", file, line);
#endif

	if (!size)
	{
		Z_Free(ptr);
		return NULL;
	}

	if (!ptr)
	{
#ifdef ZDEBUG
		return Z_Calloc2(size, tag, user, alignbits, file , line);
#else
		return Z_CallocAlign(size, tag, user, alignbits);
#endif
	}

	block = MEMBLOCK(ptr);
#ifdef PARANOIA
	if (block->id != ZONEID)
#ifdef ZDEBUG
		I_Error("Z_ReallocAlign at %s:%d: wrong id", file, line);
#else
		I_Error("Z_ReallocAlign: wrong id");
#endif
#endif

	if (block == NULL)
		return NULL;

#ifdef ZDEBUG
	// Write every Z_Realloc call to a debug file.
	DEBFILE(va("Z_Realloc at %s:%d\n", file, line));
	rez = Z_Malloc2(size, tag, user, alignbits, file, line);
#else
	rez = Z_MallocAlign(size, tag, user, alignbits);
#endif

	if (size < block->realsize)
		copysize = size;
	else
		copysize = block->realsize;

	M_Memcpy(rez, ptr, copysize);

#ifdef ZDEBUG
	Z_Free2(ptr, file, line);
#else
	Z_Free(ptr);
#endif

	// Need to set the user in case the old block had the same one, in
	// which case the Z_Free will just have NULLed it out.
	if (user)
		*((void**)user) = rez;

	if (size > copysize)
		memset((char*)rez+copysize, 0x00, size-copysize);

	return rez;
}

/** Frees all memory for a given set of tags.
  *
  * \param lowtag The lowest tag to consider.
  * \param hightag The highest tag to consider.
  */
void Z_FreeTags(INT32 lowtag, INT32 hightag)
{
	memblock_t *block, *next;

	Z_CheckHeap(420);
	for (block = head.next; block != &head; block = next)
	{
		next = block->next; // get link before freeing
		if (block->tag >= lowtag && block->tag <= hightag)
			Z_Free(MEMORY(block));
	}
}

/** Iterates through all memory for a given set of tags.
  *
  * \param lowtag The lowest tag to consider.
  * \param hightag The highest tag to consider.
  * \param iterfunc The iterator function.
  */
void Z_IterateTags(INT32 lowtag, INT32 hightag, boolean (*iterfunc)(void *))
{
	memblock_t *block, *next;

	if (!iterfunc)
		I_Error("Z_IterateTags: no iterator function was given");

	for (block = head.next; block != &head; block = next)
	{
		next = block->next; // get link before possibly freeing

		if (block->tag >= lowtag && block->tag <= hightag)
		{
			void *mem = MEMORY(block);
			boolean free = iterfunc(mem);
			if (free)
				Z_Free(mem);
		}
	}
}

// -----------------
// Utility functions
// -----------------

// starting value of nextcleanup
#define CLEANUPCOUNT 2000

// number of function calls left before next cleanup
static INT32 nextcleanup = CLEANUPCOUNT;

/** This was in Z_Malloc, but was freeing data at
  * unsafe times. Now it is only called when it is safe
  * to cleanup memory.
  *
  * \todo Currently blocks >= PU_PURGELEVEL are freed every
  *       CLEANUPCOUNT. It might be better to keep track of
  *       the total size of all purgable memory and free it when the
  *       size exceeds some value.
  */
void Z_CheckMemCleanup(void)
{
	if (nextcleanup-- == 0)
	{
		nextcleanup = CLEANUPCOUNT;
		Z_FreeTags(PU_PURGELEVEL, INT32_MAX);
	}
}


/** Checks the heap, as well as the memhdr_ts, for any corruption or
  * other problems.
  * \param i Identifies from where in the code Z_CheckHeap was called.
  * \author Graue <graue@oceanbase.org>
  */
void Z_CheckHeap(INT32 i)
{
	memblock_t *block;
	UINT32 blocknumon = 0;
	void *given;

	for (block = head.next; block != &head; block = block->next)
	{
		blocknumon++;
		given = MEMORY(block);
#ifdef ZDEBUG2
		CONS_Debug(DBG_MEMORY, "block %u owned by %s:%d\n",
			blocknumon, block->ownerfile, block->ownerline);
#endif
#ifdef VALGRIND_MEMPOOL_EXISTS
		if (!VALGRIND_MEMPOOL_EXISTS(block))
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" should not exist", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
#endif
		if (block->user != NULL && *(block->user) != given)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" doesn't have a proper user", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
		if (block->next->prev != block)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" lacks proper backlink", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
		if (block->prev->next != block)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" lacks proper forward link", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
		if (block->id != ZONEID)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" have the wrong ID", i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
				);
		}
	}
}

// ------------------------
// Zone memory modification
// ------------------------

/** Changes a memory block's purge tag.
  *
  * \param ptr A pointer to allocated memory,
  *             assumed to have been allocated with Z_Malloc/Z_Calloc.
  * \param tag The new tag.
  * \sa Z_SetUser
  */
#ifdef PARANOIA
void Z_ChangeTag2(void *ptr, INT32 tag, const char *file, INT32 line)
#else
void Z_ChangeTag(void *ptr, INT32 tag)
#endif
{
	memblock_t *block;

	if (ptr == NULL)
		return;

	block = MEMBLOCK(ptr);

#ifdef PARANOIA
	if (block->id != ZONEID) I_Error("Z_ChangeTag at %s:%d: wrong id", file, line);
#endif

	if (tag >= PU_PURGELEVEL && block->user == NULL)
		I_Error("Internal memory management error: "
			"tried to make block purgable but it has no owner");

	block->tag = tag;
}

/** Changes a memory block's user.
  *
  * \param ptr A pointer to allocated memory,
  *             assumed to have been allocated with Z_Malloc/Z_Calloc.
  * \param newuser The new user for the memory block.
  * \sa Z_ChangeTag
  */
#ifdef PARANOIA
void Z_SetUser2(void *ptr, void **newuser, const char *file, INT32 line)
#else
void Z_SetUser(void *ptr, void **newuser)
#endif
{
	memblock_t *block;

	if (ptr == NULL)
		return;

	block = MEMBLOCK(ptr);

#ifdef PARANOIA
	if (block->id != ZONEID) I_Error("Z_SetUser at %s:%d: wrong id", file, line);
#endif

	if (block->tag >= PU_PURGELEVEL && newuser == NULL)
		I_Error("Internal memory management error: "
			"tried to make block purgable but it has no owner");

	block->user = (void*)newuser;
	*newuser = ptr;
}

// -----------------
// Zone memory usage
// -----------------

/** Calculates memory usage for a given set of tags.
  *
  * \param lowtag The lowest tag to consider.
  * \param hightag The highest tag to consider.
  * \return Number of bytes currently allocated in the heap for the
  *         given tags.
  */
size_t Z_TagsUsage(INT32 lowtag, INT32 hightag)
{
	size_t cnt = 0;
	memblock_t *rover;

	for (rover = head.next; rover != &head; rover = rover->next)
	{
		if (rover->tag < lowtag || rover->tag > hightag)
			continue;
		cnt += rover->size + sizeof *rover;
	}

	return cnt;
}

// -----------------------
// Miscellaneous functions
// -----------------------

/** The function called by the "memfree" console command.
  * Prints the memory being used by each part of the game to the console.
  */
static void Command_Memfree_f(void)
{
	size_t freebytes, totalbytes;

	Z_CheckHeap(-1);
	CONS_Printf("\x82%s", M_GetText("Memory Info\n"));
	CONS_Printf(M_GetText("Total heap used        : %7s KB\n"), sizeu1(Z_TotalUsage()>>10));
	CONS_Printf(M_GetText("Static                 : %7s KB\n"), sizeu1(Z_TagUsage(PU_STATIC)>>10));
	CONS_Printf(M_GetText("Static (sound)         : %7s KB\n"), sizeu1(Z_TagUsage(PU_SOUND)>>10));
	CONS_Printf(M_GetText("Static (music)         : %7s KB\n"), sizeu1(Z_TagUsage(PU_MUSIC)>>10));
	CONS_Printf(M_GetText("Patches                : %7s KB\n"), sizeu1(Z_TagUsage(PU_PATCH)>>10));
	CONS_Printf(M_GetText("Patches (low priority) : %7s KB\n"), sizeu1(Z_TagUsage(PU_PATCH_LOWPRIORITY)>>10));
	CONS_Printf(M_GetText("Patches (rotated)      : %7s KB\n"), sizeu1(Z_TagUsage(PU_PATCH_ROTATED)>>10));
	CONS_Printf(M_GetText("Sprites                : %7s KB\n"), sizeu1(Z_TagUsage(PU_SPRITE)>>10));
	CONS_Printf(M_GetText("HUD graphics           : %7s KB\n"), sizeu1(Z_TagUsage(PU_HUDGFX)>>10));
	CONS_Printf(M_GetText("Locked cache           : %7s KB\n"), sizeu1(Z_TagUsage(PU_CACHE)>>10));
	CONS_Printf(M_GetText("Level                  : %7s KB\n"), sizeu1(Z_TagUsage(PU_LEVEL)>>10));
	CONS_Printf(M_GetText("Special thinker        : %7s KB\n"), sizeu1(Z_TagUsage(PU_LEVSPEC)>>10));
	CONS_Printf(M_GetText("All purgable           : %7s KB\n"),
		sizeu1(Z_TagsUsage(PU_PURGELEVEL, INT32_MAX)>>10));

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		CONS_Printf(M_GetText("Patch info headers     : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPATCHINFO)>>10));
		CONS_Printf(M_GetText("Cached textures        : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRCACHE)>>10));
		CONS_Printf(M_GetText("Texture colormaps      : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPATCHCOLMIPMAP)>>10));
		CONS_Printf(M_GetText("Model textures         : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRMODELTEXTURE)>>10));
		CONS_Printf(M_GetText("Light table textures   : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRLIGHTTABLEDATA)>>10));
		CONS_Printf(M_GetText("Plane polygons         : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPLANE)>>10));
		CONS_Printf(M_GetText("All GPU textures       : %7d KB\n"), HWR_GetTextureUsed()>>10);
	}
#endif

	CONS_Printf("\x82%s", M_GetText("System Memory Info\n"));
	freebytes = I_GetFreeMem(&totalbytes);
	CONS_Printf(M_GetText("    Total physical memory: %s KB\n"), sizeu1(totalbytes>>10));
	CONS_Printf(M_GetText("Available physical memory: %s KB\n"), sizeu1(freebytes>>10));
}

#ifdef ZDEBUG
/** The function called by the "memdump" console command.
  * Prints zone memory debugging information (i.e. tag, size, location in code allocated).
  * Can be all memory allocated in game, or between a set of tags (if -min/-max args used).
  * This command is available only if ZDEBUG is enabled.
  */
static void Command_Memdump_f(void)
{
	memblock_t *block;
	INT32 mintag = 0, maxtag = INT32_MAX;
	INT32 i;

	if ((i = COM_CheckParm("-min")))
		mintag = atoi(COM_Argv(i + 1));

	if ((i = COM_CheckParm("-max")))
		maxtag = atoi(COM_Argv(i + 1));

	for (block = head.next; block != &head; block = block->next)
		if (block->tag >= mintag && block->tag <= maxtag)
		{
			char *filename = strrchr(block->ownerfile, PATHSEP[0]);
			CONS_Printf("[%3d] %s (%s) bytes @ %s:%d\n", block->tag, sizeu1(block->size), sizeu2(block->realsize), filename ? filename + 1 : block->ownerfile, block->ownerline);
		}
}
#endif

/** Creates a copy of a string.
  *
  * \param s The string to be copied.
  * \return A copy of the string, allocated in zone memory.
  */
char *Z_StrDup(const char *s)
{
	return strcpy(ZZ_Alloc(strlen(s) + 1), s);
}
