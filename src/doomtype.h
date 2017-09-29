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
/// \file  doomtype.h
/// \brief SRB2 standard types
///
///        Simple basic typedefs, isolated here to make it easier
///        separating modules.

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#if defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__))
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif

#ifdef _NDS
#include <nds.h>
#endif

/* 7.18.1.1  Exact-width integer types */
#ifdef _MSC_VER
#define UINT8 unsigned __int8
#define SINT8 signed __int8

#define UINT16 unsigned __int16
#define INT16 __int16

#define INT32 __int32
#define UINT32 unsigned __int32

#define INT64  __int64
#define UINT64 unsigned __int64

typedef long ssize_t;

/* Older Visual C++ headers don't have the Win64-compatible typedefs... */
#if ((_MSC_VER <= 1200) && (!defined(DWORD_PTR)))
#define DWORD_PTR DWORD
#endif

#if ((_MSC_VER <= 1200) && (!defined(PDWORD_PTR)))
#define PDWORD_PTR PDWORD
#endif
#elif defined (__DJGPP__)
#define UINT8 unsigned char
#define SINT8 signed char

#define UINT16 unsigned short int
#define INT16 signed short int

#define INT32 signed long
#define UINT32 unsigned long
#define INT64  signed long long
#define UINT64 unsigned long long
#else
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#define UINT8 uint8_t
#define SINT8 int8_t

#define UINT16 uint16_t
#define INT16 int16_t

#define INT32 int32_t
#define UINT32 uint32_t
#define INT64  int64_t
#define UINT64 uint64_t
#endif

#ifdef __APPLE_CC__
#define DIRECTFULLSCREEN 1
#define DEBUG_LOG
#define NOIPX
#endif

#if defined (_MSC_VER) || defined (__OS2__)
	// Microsoft VisualC++
#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // MSVC 2013 and back
	#define snprintf                _snprintf
#if (_MSC_VER <= 1200) // MSVC 2012 and back
	#define vsnprintf               _vsnprintf
#endif
#endif
#endif
	#define strncasecmp             strnicmp
	#define strcasecmp              stricmp
	#define inline                  __inline
#elif defined (__WATCOMC__)
	#include <dos.h>
	#include <sys\types.h>
	#include <direct.h>
	#include <malloc.h>
	#define strncasecmp             strnicmp
	#define strcasecmp              strcmpi
#endif
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
	#undef stricmp
	#define stricmp(x,y) strcasecmp(x,y)
	#undef strnicmp
	#define strnicmp(x,y,n) strncasecmp(x,y,n)
#endif
#ifdef _WIN32_WCE
#ifndef __GNUC__
	#define stricmp(x,y)            _stricmp(x,y)
	#define strnicmp                _strnicmp
#endif
	#define strdup                  _strdup
	#define strupr                  _strupr
	#define strlwr                  _strlwr
#endif

#if defined (macintosh) //|| defined (__APPLE__) //skip all boolean/Boolean crap
	#define true 1
	#define false 0
	#define min(x,y) (((x)<(y)) ? (x) : (y))
	#define max(x,y) (((x)>(y)) ? (x) : (y))

#ifdef macintosh
	#define stricmp strcmp
	#define strnicmp strncmp
#endif

	#define boolean INT32

	#ifndef O_BINARY
	#define O_BINARY 0
	#endif
#endif //macintosh

#if defined (PC_DOS) || defined (_WIN32) || defined (__HAIKU__) || defined(_NDS)
#define HAVE_DOSSTR_FUNCS
#endif

#ifndef HAVE_DOSSTR_FUNCS
int strupr(char *n); // from dosstr.c
int strlwr(char *n); // from dosstr.c
#endif

#include <stddef.h> // for size_t

#ifndef __APPLE__
size_t strlcat(char *dst, const char *src, size_t siz);
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

// Macro for use with char foo[FOOSIZE+1] type buffers.
// Never use this with a buffer that is a "char *" or passed
// into the function as an argument.
//
// In those cases sizeof will return the size of the pointer,
// not the number of bytes in the buffer.
#define STRBUFCPY(dst,src) strlcpy(dst, src, sizeof dst)

// \note __BYTEBOOL__ used to be set above if "macintosh" was defined,
// if macintosh's version of boolean type isn't needed anymore, then isn't this macro pointless now?
#ifndef __BYTEBOOL__
	#define __BYTEBOOL__

	//faB: clean that up !!
	#if defined( _MSC_VER)  && (_MSC_VER >= 1800) // MSVC 2013 and forward
	#include "stdbool.h"
	#elif defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__))
		#define false   FALSE           // use windows types
		#define true    TRUE
		#define boolean BOOL
	#elif defined(_NDS)
		#define boolean bool
	#else
		typedef enum {false, true} boolean;
	#endif
#endif // __BYTEBOOL__

/* 7.18.2.1  Limits of exact-width integer types */
#ifndef INT8_MIN
#define INT8_MIN (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif
#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif
#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif
#ifndef INT16_MAX
#define INT16_MAX 32767
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 0xff /* 255U */
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xffff /* 65535U */
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff  /* 4294967295U */
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */
#endif

union FColorRGBA
{
	UINT32 rgba;
	struct
	{
		UINT8 red;
		UINT8 green;
		UINT8 blue;
		UINT8 alpha;
	} s;
} ATTRPACK;
typedef union FColorRGBA RGBA_t;

typedef enum
{
	postimg_none,
	postimg_water,
	postimg_motion,
	postimg_flip,
	postimg_heat
} postimg_t;

typedef UINT32 lumpnum_t; // 16 : 16 unsigned long (wad num: lump num)
#define LUMPERROR UINT32_MAX

typedef UINT32 tic_t;
#define INFTICS UINT32_MAX

#ifdef _BIG_ENDIAN
#define UINT2RGBA(a) a
#else
#define UINT2RGBA(a) (UINT32)((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|(((UINT32)a&0xff000000)>>24)
#endif

#ifdef __GNUC__ // __attribute__ ((X))
#define FUNCNORETURN __attribute__ ((noreturn))
#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) && defined (__MINGW32__)
#include "inttypes.h"
#if 0 //defined  (__USE_MINGW_ANSI_STDIO) && __USE_MINGW_ANSI_STDIO > 0
#define FUNCPRINTF __attribute__ ((format(gnu_printf, 1, 2)))
#define FUNCDEBUG  __attribute__ ((format(gnu_printf, 2, 3)))
#define FUNCIERROR __attribute__ ((format(gnu_printf, 1, 2),noreturn))
#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
#define FUNCPRINTF __attribute__ ((format(ms_printf, 1, 2)))
#define FUNCDEBUG  __attribute__ ((format(ms_printf, 2, 3)))
#define FUNCIERROR __attribute__ ((format(ms_printf, 1, 2),noreturn))
#else
#define FUNCPRINTF __attribute__ ((format(printf, 1, 2)))
#define FUNCDEBUG  __attribute__ ((format(printf, 2, 3)))
#define FUNCIERROR __attribute__ ((format(printf, 1, 2),noreturn))
#endif
#else
#define FUNCPRINTF __attribute__ ((format(printf, 1, 2)))
#define FUNCDEBUG  __attribute__ ((format(printf, 2, 3)))
#define FUNCIERROR __attribute__ ((format(printf, 1, 2),noreturn))
#endif
#ifndef FUNCIERROR
#define FUNCIERROR __attribute__ ((noreturn))
#endif
#define FUNCMATH __attribute__((const))
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define FUNCDEAD __attribute__ ((deprecated))
#define FUNCINLINE __attribute__((always_inline))
#define FUNCNONNULL __attribute__((nonnull))
#endif
#define FUNCNOINLINE __attribute__((noinline))
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
#ifdef __i386__ // i386 only
#define FUNCTARGET(X)  __attribute__ ((__target__ (X)))
#endif
#endif
#if defined (__MINGW32__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#define ATTRPACK __attribute__((packed, gcc_struct))
#else
#define ATTRPACK __attribute__((packed))
#endif
#define ATTRUNUSED __attribute__((unused))
#elif defined (_MSC_VER)
#define ATTRNORETURN __declspec(noreturn)
#define ATTRINLINE __forceinline
#if _MSC_VER > 1200
#define ATTRNOINLINE __declspec(noinline)
#endif
#endif

#ifndef FUNCPRINTF
#define FUNCPRINTF
#endif
#ifndef FUNCDEBUG
#define FUNCDEBUG
#endif
#ifndef FUNCNORETURN
#define FUNCNORETURN
#endif
#ifndef FUNCIERROR
#define FUNCIERROR
#endif
#ifndef FUNCMATH
#define FUNCMATH
#endif
#ifndef FUNCDEAD
#define FUNCDEAD
#endif
#ifndef FUNCINLINE
#define FUNCINLINE
#endif
#ifndef FUNCNONNULL
#define FUNCNONNULL
#endif
#ifndef FUNCNOINLINE
#define FUNCNOINLINE
#endif
#ifndef FUNCTARGET
#define FUNCTARGET(x)
#endif
#ifndef ATTRPACK
#define ATTRPACK
#endif
#ifndef ATTRUNUSED
#define ATTRUNUSED
#endif
#ifndef ATTRNORETURN
#define ATTRNORETURN
#endif
#ifndef ATTRINLINE
#define ATTRINLINE inline
#endif
#ifndef ATTRNOINLINE
#define ATTRNOINLINE
#endif
#endif //__DOOMTYPE__
