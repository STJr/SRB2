// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by Graue.
// Copyright (C) 2006-2014 by Sonic Team Junior.
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

struct memblock_s;

typedef struct
{
	struct memblock_s *block; // Describing this memory
	UINT32 id; // Should be ZONEID
} ATTRPACK memhdr_t;

// Some code might want aligned memory. Assume it wants memory n bytes
// aligned -- then we allocate n-1 extra bytes and return a pointer to
// the first byte aligned as requested.
// Thus, "real" is the pointer we get from malloc() and will free()
// later, but "hdr" is where the memhdr_t starts.
// For non-aligned allocations they will be the same.
typedef struct memblock_s
{
	void *real;
	memhdr_t *hdr;

	void **user;
	INT32 tag; // purgelevel

	size_t size; // including the header and blocks
	size_t realsize; // size of real data only

#ifdef ZDEBUG
	const char *ownerfile;
	INT32 ownerline;
#endif

	struct memblock_s *next, *prev;
} ATTRPACK memblock_t;

#ifdef ZDEBUG
#define Ptr2Memblock(s, f) Ptr2Memblock2(s, f, __FILE__, __LINE__)
static memblock_t *Ptr2Memblock2(void *ptr, const char* func, const char *file, INT32 line)
#else
static memblock_t *Ptr2Memblock(void *ptr, const char* func)
#endif
{
	memhdr_t *hdr;
	memblock_t *block;

	if (ptr == NULL)
		return NULL;

#ifdef ZDEBUG2
	CONS_Printf("%s %s:%d\n", func, file, line);
#endif

	hdr = (memhdr_t *)((UINT8 *)ptr - sizeof *hdr);

#ifdef VALGRIND_MAKE_MEM_DEFINED
	VALGRIND_MAKE_MEM_DEFINED(hdr, sizeof *hdr);
#endif

#ifdef VALGRIND_MEMPOOL_EXISTS
	if (!VALGRIND_MEMPOOL_EXISTS(hdr->block))
	{
#ifdef ZDEBUG
		I_Error("%s: bad memblock from %s:%d", func, file, line);
#else
		I_Error("%s: bad memblock", func);
#endif
	}
#endif
	if (hdr->id != ZONEID)
	{
#ifdef ZDEBUG
		I_Error("%s: wrong id from %s:%d", func, file, line);
#else
		I_Error("%s: wrong id", func);
#endif
	}
	block = hdr->block;
#ifdef VALGRIND_MAKE_MEM_NOACCESS
	VALGRIND_MAKE_MEM_NOACCESS(hdr, sizeof *hdr);
#endif
	return block;

}

static memblock_t head;

static void Command_Memfree_f(void);
#ifdef ZDEBUG
static void Command_Memdump_f(void);
#endif

void Z_Init(void)
{
	UINT32 total, memfree;

	memset(&head, 0x00, sizeof(head));

	head.next = head.prev = &head;

	memfree = I_GetFreeMem(&total)>>20;
	CONS_Printf("System memory: %uMB - Free: %uMB\n", total>>20, memfree);

	// Note: This allocates memory. Watch out.
	COM_AddCommand("memfree", Command_Memfree_f);

#ifdef ZDEBUG
	COM_AddCommand("memdump", Command_Memdump_f);
#endif
}

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

#ifdef ZDEBUG
	block = Ptr2Memblock2(ptr, "Z_Free", file, line);
#else
	block = Ptr2Memblock(ptr, "Z_Free");
#endif

#ifdef ZDEBUG
	// Write every Z_Free call to a debug file.
	CONS_Debug(DBG_MEMORY, "Z_Free at %s:%d\n", file, line);
#endif

#ifdef HAVE_BLUA
	// anything that isn't by lua gets passed to lua just in case.
	if (block->tag != PU_LUA)
		LUA_InvalidateUserdata(ptr);
#endif

	// TODO: if zdebugging, make sure no other block has a user
	// that is about to be freed.

	// Clear the user's mark.
	if (block->user != NULL)
		*block->user = NULL;

	// Free the memory and get rid of the block.
	free(block->real);
	block->prev->next = block->next;
	block->next->prev = block->prev;
	free(block);
#ifdef VALGRIND_DESTROY_MEMPOOL
	VALGRIND_DESTROY_MEMPOOL(block);
#endif
}

// malloc() that doesn't accept failure.
static void *xm(size_t size)
{
	const size_t padedsize = size+sizeof (size_t);
	void *p = malloc(padedsize);

	if (p == NULL)
	{
		// Oh crumbs: we're out of heap. Try purging the cache and reallocating.
		Z_FreeTags(PU_PURGELEVEL, INT32_MAX);
		p = malloc(padedsize);

		if (p == NULL)
		{
#if defined (_NDS) | defined (_PSP)
			// Temporary-ish debugging measure
			Command_Memfree_f();
#endif
			I_Error("Out of memory allocating %s bytes", sizeu1(size));
		}
	}

	return p;
}

// Z_Malloc
// You can pass Z_Malloc() a NULL user if the tag is less than
// PU_PURGELEVEL.

#ifdef ZDEBUG
void *Z_Malloc2(size_t size, INT32 tag, void *user, INT32 alignbits,
	const char *file, INT32 line)
#else
void *Z_MallocAlign(size_t size, INT32 tag, void *user, INT32 alignbits)
#endif
{
	size_t extrabytes = (1<<alignbits) - 1;
	size_t padsize = 0;
	memblock_t *block;
	void *ptr;
	memhdr_t *hdr;
	void *given;
	size_t blocksize = extrabytes + sizeof *hdr + size;

#ifdef ZDEBUG2
	CONS_Debug(DBG_MEMORY, "Z_Malloc %s:%d\n", file, line);
#endif

	block = xm(sizeof *block);
#ifdef HAVE_VALGRIND
	padsize += (1<<sizeof(size_t))*2;
#endif
	ptr = xm(blocksize + padsize*2);

	// This horrible calculation makes sure that "given" is aligned
	// properly.
	given = (void *)((size_t)((UINT8 *)ptr + extrabytes + sizeof *hdr + padsize/2)
		& ~extrabytes);

	// The mem header lives 'sizeof (memhdr_t)' bytes before given.
	hdr = (memhdr_t *)((UINT8 *)given - sizeof *hdr);

#ifdef VALGRIND_CREATE_MEMPOOL
	VALGRIND_CREATE_MEMPOOL(block, padsize, Z_calloc);
	Z_calloc = false;
#endif
#ifdef VALGRIND_MEMPOOL_ALLOC
	VALGRIND_MEMPOOL_ALLOC(block, hdr, size + sizeof *hdr);
#endif

	block->next = head.next;
	block->prev = &head;
	head.next = block;
	block->next->prev = block;

	block->real = ptr;
	block->hdr = hdr;
	block->tag = tag;
	block->user = NULL;
#ifdef ZDEBUG
	block->ownerline = line;
	block->ownerfile = file;
#endif
	block->size = blocksize;
	block->realsize = size;

	hdr->id = ZONEID;
	hdr->block = block;

#ifdef VALGRIND_MAKE_MEM_NOACCESS
	VALGRIND_MAKE_MEM_NOACCESS(hdr, sizeof *hdr);
#endif

	if (user != NULL)
	{
		block->user = user;
		*(void **)user = given;
	}
	else if (tag >= PU_PURGELEVEL)
		I_Error("Z_Malloc: attempted to allocate purgable block "
			"(size %s) with no user", sizeu1(size));

	return given;
}

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

#ifdef ZDEBUG
void *Z_Realloc2(void *ptr, size_t size, INT32 tag, void *user, INT32 alignbits, const char *file, INT32 line)
#else
void *Z_ReallocAlign(void *ptr, size_t size,INT32 tag, void *user,  INT32 alignbits)
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

#ifdef ZDEBUG
	block = Ptr2Memblock2(ptr, "Z_Realloc", file, line);
#else
	block = Ptr2Memblock(ptr, "Z_Realloc");
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

void Z_FreeTags(INT32 lowtag, INT32 hightag)
{
	memblock_t *block, *next;

	Z_CheckHeap(420);
	for (block = head.next; block != &head; block = next)
	{
		next = block->next; // get link before freeing

		if (block->tag >= lowtag && block->tag <= hightag)
			Z_Free((UINT8 *)block->hdr + sizeof *block->hdr);
	}
}

//
// Z_CheckMemCleanup
//
// TODO: Currently blocks >= PU_PURGELEVEL are freed every
// CLEANUPCOUNT. It might be better to keep track of
// the total size of all purgable memory and free it when the
// size exceeds some value.
//
// This was in Z_Malloc, but was freeing data at
// unsafe times. Now it is only called when it is safe
// to cleanup memory.

#define CLEANUPCOUNT 2000

static INT32 nextcleanup = CLEANUPCOUNT;

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
	memhdr_t *hdr;
	UINT32 blocknumon = 0;
	void *given;

	for (block = head.next; block != &head; block = block->next)
	{
		blocknumon++;
		hdr = block->hdr;
		given = (UINT8 *)hdr + sizeof *hdr;
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
#ifdef VALGRIND_MAKE_MEM_DEFINED
		VALGRIND_MAKE_MEM_DEFINED(hdr, sizeof *hdr);
#endif
		if (hdr->block != block)
		{
			I_Error("Z_CheckHeap %d: block %u"
#ifdef ZDEBUG
				"(owned by %s:%d)"
#endif
				" doesn't have linkback from allocated memory",
				i, blocknumon
#ifdef ZDEBUG
				, block->ownerfile, block->ownerline
#endif
					);
		}
		if (hdr->id != ZONEID)
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
#ifdef VALGRIND_MAKE_MEM_NOACCESS
	VALGRIND_MAKE_MEM_NOACCESS(hdr, sizeof *hdr);
#endif
	}
}

#ifdef PARANOIA
void Z_ChangeTag2(void *ptr, INT32 tag, const char *file, INT32 line)
#else
void Z_ChangeTag2(void *ptr, INT32 tag)
#endif
{
	memblock_t *block;
	memhdr_t *hdr;

	if (ptr == NULL)
		return;

	hdr = (memhdr_t *)((UINT8 *)ptr - sizeof *hdr);

#ifdef VALGRIND_MAKE_MEM_DEFINED
	VALGRIND_MAKE_MEM_DEFINED(hdr, sizeof *hdr);
#endif

#ifdef VALGRIND_MEMPOOL_EXISTS
	if (!VALGRIND_MEMPOOL_EXISTS(hdr->block))
	{
#ifdef PARANOIA
		I_Error("Z_CT at %s:%d: bad memblock", file, line);
#else
		I_Error("Z_CT: bad memblock");
#endif
	}
#endif
#ifdef PARANOIA
	if (hdr->id != ZONEID) I_Error("Z_CT at %s:%d: wrong id", file, line);
#endif

	block = hdr->block;

#ifdef VALGRIND_MAKE_MEM_NOACCESS
	VALGRIND_MAKE_MEM_NOACCESS(hdr, sizeof *hdr);
#endif

	if (tag >= PU_PURGELEVEL && block->user == NULL)
		I_Error("Internal memory management error: "
			"tried to make block purgable but it has no owner");

	block->tag = tag;
}

/** Calculates memory usage for a given set of tags.
  * \param lowtag The lowest tag to consider.
  * \param hightag The highest tag to consider.
  * \return Number of bytes currently allocated in the heap for the
  *         given tags.
  * \sa Z_TagUsage
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

size_t Z_TagUsage(INT32 tagnum)
{
	return Z_TagsUsage(tagnum, tagnum);
}

void Command_Memfree_f(void)
{
	UINT32 freebytes, totalbytes;

	Z_CheckHeap(-1);
	CONS_Printf("\x82%s", M_GetText("Memory Info\n"));
	CONS_Printf(M_GetText("Total heap used   : %7s KB\n"), sizeu1(Z_TagsUsage(0, INT32_MAX)>>10));
	CONS_Printf(M_GetText("Static            : %7s KB\n"), sizeu1(Z_TagUsage(PU_STATIC)>>10));
	CONS_Printf(M_GetText("Static (sound)    : %7s KB\n"), sizeu1(Z_TagUsage(PU_SOUND)>>10));
	CONS_Printf(M_GetText("Static (music)    : %7s KB\n"), sizeu1(Z_TagUsage(PU_MUSIC)>>10));
	CONS_Printf(M_GetText("Locked cache      : %7s KB\n"), sizeu1(Z_TagUsage(PU_CACHE)>>10));
	CONS_Printf(M_GetText("Level             : %7s KB\n"), sizeu1(Z_TagUsage(PU_LEVEL)>>10));
	CONS_Printf(M_GetText("Special thinker   : %7s KB\n"), sizeu1(Z_TagUsage(PU_LEVSPEC)>>10));
	CONS_Printf(M_GetText("All purgable      : %7s KB\n"),
		sizeu1(Z_TagsUsage(PU_PURGELEVEL, INT32_MAX)>>10));

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		CONS_Printf(M_GetText("Patch info headers: %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPATCHINFO)>>10));
		CONS_Printf(M_GetText("Mipmap patches    : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPATCHCOLMIPMAP)>>10));
		CONS_Printf(M_GetText("HW Texture cache  : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRCACHE)>>10));
		CONS_Printf(M_GetText("Plane polygons    : %7s KB\n"), sizeu1(Z_TagUsage(PU_HWRPLANE)>>10));
		CONS_Printf(M_GetText("HW Texture used   : %7d KB\n"), HWR_GetTextureUsed()>>10);
	}
#endif

	CONS_Printf("\x82%s", M_GetText("System Memory Info\n"));
	freebytes = I_GetFreeMem(&totalbytes);
	CONS_Printf(M_GetText("    Total physical memory: %7u KB\n"), totalbytes>>10);
	CONS_Printf(M_GetText("Available physical memory: %7u KB\n"), freebytes>>10);
}

#ifdef ZDEBUG
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

// Creates a copy of a string.
char *Z_StrDup(const char *s)
{
	return strcpy(ZZ_Alloc(strlen(s) + 1), s);
}


#ifdef PARANOIA
void Z_SetUser2(void *ptr, void **newuser, const char *file, INT32 line)
#else
void Z_SetUser2(void *ptr, void **newuser)
#endif
{
	memblock_t *block;
	memhdr_t *hdr;

	if (ptr == NULL)
		return;

	hdr = (memhdr_t *)((UINT8 *)ptr - sizeof *hdr);

#ifdef VALGRIND_MAKE_MEM_DEFINED
	VALGRIND_MAKE_MEM_DEFINED(hdr, sizeof *hdr);
#endif

#ifdef PARANOIA
	if (hdr->id != ZONEID) I_Error("Z_CT at %s:%d: wrong id", file, line);
#endif

	block = hdr->block;

#ifdef VALGRIND_MAKE_MEM_NOACCESS
	VALGRIND_MAKE_MEM_NOACCESS(hdr, sizeof *hdr);
#endif

	if (block->tag >= PU_PURGELEVEL && newuser == NULL)
		I_Error("Internal memory management error: "
			"tried to make block purgable but it has no owner");

	block->user = (void*)newuser;
	*newuser = ptr;
}
