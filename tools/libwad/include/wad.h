// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// libwad: Doom WAD format interface library.
// Copyright (C) 2011 by Callum Dickinson.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file wad.h
/// \brief WAD file manipulation library.

#ifndef __WAD_H__
#define __WAD_H__

#ifdef __cplusplus
extern "C"
{
#endif

//
// Exact-width integer types.
//
#ifdef _MSC_VER
typedef signed __int8 wad_int8_t;
typedef unsigned __int8 wad_uint8_t;

typedef __int16 wad_int16_t;
typedef unsigned __int16 wad_uint16_t;

typedef __int32 wad_int32_t;
typedef unsigned __int32 wad_uint32_t;

typedef __int64 wad_int64_t;
typedef unsigned __int64 wad_uint64_t;

// Older Visual C++ headers don't have the Win64-compatible typedefs...
#if ((_MSC_VER <= 1200) && (!defined(DWORD_PTR)))
#define DWORD_PTR DWORD
#endif

#if ((_MSC_VER <= 1200) && (!defined(PDWORD_PTR)))
#define PDWORD_PTR PDWORD
#endif

#elif defined (_arch_dreamcast)
#include <arch/types.h>

typedef signed char wad_int8_t;
typedef unsigned char wad_uint8_t;

typedef int16 wad_int16_t;
typedef uint16 wad_uint16_t;

typedef int wad_int32_t;
typedef unsigned int wad_uint32_t;

typedef int64 wad_int64_t;
typedef uint64 wad_uint64_t;
#elif defined (__DJGPP__)
typedef signed char wad_int8_t;
typedef unsigned char wad_uint8_t;

typedef signed short int wad_int16_t;
typedef unsigned short int wad_uint16_t;

typedef signed long wad_int32_t;
typedef unsigned long wad_uint32_t;

typedef signed long long wad_int64_t;
typedef unsigned long long wad_uint64_t;
#else
#define __STDC_LIMIT_MACROS
#include <stdint.h>

typedef int8_t wad_int8_t;
typedef uint8_t wad_uint8_t;

typedef int16_t wad_int16_t;
typedef uint16_t wad_uint16_t;

typedef int32_t wad_int32_t;
typedef uint32_t wad_uint32_t;

typedef int64_t wad_int64_t;
typedef uint64_t wad_uint64_t;
#endif

typedef enum
{
	false,
	true
} wad_boolean_t;

typedef struct
{
	char id[4];		// The WAD header's 4-character ID.
	wad_uint32_t numlumps;	// The number of lumps the WAD has.
	wad_uint32_t lumpdir;	// The pointer to the lump directory.
} wadheader_t;

typedef struct wad_s
{
	char *filename;		// The filename of the open WAD.

	wadheader_t *header;	// The WAD's header.
	wad_boolean_t compressed;

	struct lump_s *lumps;	// The WAD's lumps buffer.
} wad_t;

typedef struct lump_s
{
	wad_t *wad;		// The WAD the lump belongs to.

	char name[9];		// The name of the lump.

	wad_uint64_t disksize;	// The size of the lump on disk.
	size_t size;		// The real (uncompressed) size of the lump.

	wad_boolean_t compressed;

	void *data;		// The actual lump data.

	wad_uint32_t num;	// The lump number in the WAD.
	struct lump_s *prev; 	// The previous node in the tree.
	struct lump_s *next;	// The next node in the tree.
} lump_t;

// a raw entry of the wad directory
typedef struct
{
	wad_uint32_t position;		// The position of the lump in the WAD file.
	wad_uint32_t size;		// Size of the lump.
	char name[8];			// The lump's name.
} lumpdir_t;

//
// wad_static.c
// Static file for internal libwad operations.
//
extern char *WAD_Error(void);
extern char *WAD_BaseName(char *path);
extern char *WAD_VA(const char *format, ...);

extern inline wad_int16_t WAD_INT16_Endianness(wad_int16_t x);
extern inline wad_int32_t WAD_INT32_Endianness(wad_int32_t x);

//
// wad.c
// WAD interface functionality.
//
extern wad_uint32_t wad_numwads;

extern wad_uint32_t WAD_WADLoopAdvance(wad_uint32_t i);

extern wad_t *WAD_WADByNum(wad_uint16_t num);
extern wad_t *WAD_WADByFileName(const char *filename);

extern wad_t *WAD_OpenWAD(const char *filename);

extern wad_int32_t WAD_SaveWAD(wad_t *wad);
extern inline wad_int32_t WAD_SaveWADByNum(wad_uint16_t num);
extern inline wad_int32_t WAD_SaveWADByFileName(const char *filename);

extern inline wad_int32_t WAD_SaveCloseWAD(wad_t *wad);
extern inline wad_int32_t WAD_SaveCloseWADByNum(wad_uint16_t num);
extern inline wad_int32_t WAD_SaveCloseWADByFileName(const char *filename);

extern void WAD_CloseWAD(wad_t *wad);
extern inline void WAD_CloseWADByNum(wad_uint16_t num);
extern inline void WAD_CloseWADByFileName(const char *filename);

//
// lump.c
// Lump interface functionality.
//
extern char *WAD_LumpNameFromFileName(char *path);
extern lump_t *WAD_LumpInWADByNum(wad_t *wad, wad_uint32_t num);
extern lump_t *WAD_LumpInWADByName(wad_t *wad, const char *name);
extern lump_t *WAD_LumpByName(const char *name);

extern inline wad_uint32_t WAD_LumpNum(lump_t *lump);
extern inline wad_uint32_t WAD_LumpNumInWADByName(wad_t *wad, const char *name);
extern inline wad_uint32_t WAD_LumpNumByName(const char *name);

extern inline size_t WAD_LumpSize(lump_t *lump);
extern inline size_t WAD_LumpSizeInWADByName(wad_t *wad, const char *name);
extern inline size_t WAD_LumpSizeByName(const char *name);

extern lump_t *WAD_AddLump(wad_t *wad, lump_t *destlump, const char *name, const char *filename);
extern inline lump_t *WAD_AddLumpInWADByNum(wad_t *wad, wad_uint32_t num, const char *name, const char *filename);
extern inline lump_t *WAD_AddLumpInWADByName(wad_t *wad, const char *destname, const char *name, const char *filename);
extern inline lump_t *WAD_AddLumpByName(const char *destname, const char *name, const char *filename);

extern lump_t *WAD_CacheLump(lump_t *lump);
extern inline lump_t *WAD_CacheLumpInWADByNum(wad_t *wad, wad_uint32_t num);
extern inline lump_t *WAD_CacheLumpInWADByName(wad_t *wad, const char *name);
extern inline lump_t *WAD_CacheLumpByName(const char *name);

extern lump_t *WAD_MoveLump(lump_t *lump, lump_t *destlump);
extern inline lump_t *WAD_MoveLumpInWADByNum(wad_t *wad, wad_uint32_t num, wad_t *destwad, wad_uint32_t destnum);
extern inline lump_t *WAD_MoveLumpInWADByName(wad_t *wad, const char *name, wad_t *destwad, const char *destname);
extern inline lump_t *WAD_MoveLumpByName(const char *name, const char *destname);

extern void WAD_UncacheLump(lump_t *lump);
extern inline void WAD_UncacheLumpInWADByNum(wad_t *wad, wad_uint32_t num);
extern inline void WAD_UncacheLumpInWADByName(wad_t *wad, const char *name);
extern inline void WAD_UncacheLumpByName(const char *name);

extern void WAD_RemoveLump(lump_t *lump);
extern inline void WAD_RemoveLumpInWADByNum(wad_t *wad, wad_uint32_t num);
extern inline void WAD_RemoveLumpInWADByName(wad_t *wad, const char *name);
extern inline void WAD_RemoveLumpByName(const char *name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __WAD_H__
