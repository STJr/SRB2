// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  byteptr.h
/// \brief Macros to read/write from/to a UINT8 *,
///        used for packet creation and such

#if defined (__alpha__) || defined (__arm__) || defined (__mips__) || defined (__ia64__) || defined (__clang__)
#define DEALIGNED
#endif

#include "endian.h"

#ifndef SRB2_BIG_ENDIAN
//
// Little-endian machines
//
#ifdef DEALIGNED
#define WRITEUINT8(p,b)     do {   UINT8 *p_tmp = (void *)p; const   UINT8 tv = (  UINT8)(b); memcpy(p, &tv, sizeof(  UINT8)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITESINT8(p,b)     do {   SINT8 *p_tmp = (void *)p; const   SINT8 tv = (  UINT8)(b); memcpy(p, &tv, sizeof(  UINT8)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEINT16(p,b)     do {   INT16 *p_tmp = (void *)p; const   INT16 tv = (  INT16)(b); memcpy(p, &tv, sizeof(  INT16)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEUINT16(p,b)    do {  UINT16 *p_tmp = (void *)p; const  UINT16 tv = ( UINT16)(b); memcpy(p, &tv, sizeof( UINT16)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEINT32(p,b)     do {   INT32 *p_tmp = (void *)p; const   INT32 tv = (  INT32)(b); memcpy(p, &tv, sizeof(  INT32)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEUINT32(p,b)    do {  UINT32 *p_tmp = (void *)p; const  UINT32 tv = ( UINT32)(b); memcpy(p, &tv, sizeof( UINT32)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITECHAR(p,b)      do {    char *p_tmp = (void *)p; const    char tv = (   char)(b); memcpy(p, &tv, sizeof(   char)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEFIXED(p,b)     do { fixed_t *p_tmp = (void *)p; const fixed_t tv = (fixed_t)(b); memcpy(p, &tv, sizeof(fixed_t)); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEANGLE(p,b)     do { angle_t *p_tmp = (void *)p; const angle_t tv = (angle_t)(b); memcpy(p, &tv, sizeof(angle_t)); p_tmp++; p = (void *)p_tmp; } while (0)
#else
#define WRITEUINT8(p,b)     do {   UINT8 *p_tmp = (  UINT8 *)p; *p_tmp = (  UINT8)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITESINT8(p,b)     do {   SINT8 *p_tmp = (  SINT8 *)p; *p_tmp = (  SINT8)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEINT16(p,b)     do {   INT16 *p_tmp = (  INT16 *)p; *p_tmp = (  INT16)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEUINT16(p,b)    do {  UINT16 *p_tmp = ( UINT16 *)p; *p_tmp = ( UINT16)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEINT32(p,b)     do {   INT32 *p_tmp = (  INT32 *)p; *p_tmp = (  INT32)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEUINT32(p,b)    do {  UINT32 *p_tmp = ( UINT32 *)p; *p_tmp = ( UINT32)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITECHAR(p,b)      do {    char *p_tmp = (   char *)p; *p_tmp = (   char)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEFIXED(p,b)     do { fixed_t *p_tmp = (fixed_t *)p; *p_tmp = (fixed_t)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#define WRITEANGLE(p,b)     do { angle_t *p_tmp = (angle_t *)p; *p_tmp = (angle_t)(b); p_tmp++; p = (void *)p_tmp; } while (0)
#endif

#ifdef __GNUC__
#ifdef DEALIGNED
#define READUINT8(p)        ({   UINT8 *p_tmp = (void *)p;   UINT8 b; memcpy(&b, p, sizeof(  UINT8)); p_tmp++; p = (void *)p_tmp; b; })
#define READSINT8(p)        ({   SINT8 *p_tmp = (void *)p;   SINT8 b; memcpy(&b, p, sizeof(  SINT8)); p_tmp++; p = (void *)p_tmp; b; })
#define READINT16(p)        ({   INT16 *p_tmp = (void *)p;   INT16 b; memcpy(&b, p, sizeof(  INT16)); p_tmp++; p = (void *)p_tmp; b; })
#define READUINT16(p)       ({  UINT16 *p_tmp = (void *)p;  UINT16 b; memcpy(&b, p, sizeof( UINT16)); p_tmp++; p = (void *)p_tmp; b; })
#define READINT32(p)        ({   INT32 *p_tmp = (void *)p;   INT32 b; memcpy(&b, p, sizeof(  INT32)); p_tmp++; p = (void *)p_tmp; b; })
#define READUINT32(p)       ({  UINT32 *p_tmp = (void *)p;  UINT32 b; memcpy(&b, p, sizeof( UINT32)); p_tmp++; p = (void *)p_tmp; b; })
#define READCHAR(p)         ({    char *p_tmp = (void *)p;    char b; memcpy(&b, p, sizeof(   char)); p_tmp++; p = (void *)p_tmp; b; })
#define READFIXED(p)        ({ fixed_t *p_tmp = (void *)p; fixed_t b; memcpy(&b, p, sizeof(fixed_t)); p_tmp++; p = (void *)p_tmp; b; })
#define READANGLE(p)        ({ angle_t *p_tmp = (void *)p; angle_t b; memcpy(&b, p, sizeof(angle_t)); p_tmp++; p = (void *)p_tmp; b; })
#else
#define READUINT8(p)        ({   UINT8 *p_tmp = (  UINT8 *)p;   UINT8 b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READSINT8(p)        ({   SINT8 *p_tmp = (  SINT8 *)p;   SINT8 b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READINT16(p)        ({   INT16 *p_tmp = (  INT16 *)p;   INT16 b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READUINT16(p)       ({  UINT16 *p_tmp = ( UINT16 *)p;  UINT16 b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READINT32(p)        ({   INT32 *p_tmp = (  INT32 *)p;   INT32 b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READUINT32(p)       ({  UINT32 *p_tmp = ( UINT32 *)p;  UINT32 b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READCHAR(p)         ({    char *p_tmp = (   char *)p;    char b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READFIXED(p)        ({ fixed_t *p_tmp = (fixed_t *)p; fixed_t b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READANGLE(p)        ({ angle_t *p_tmp = (angle_t *)p; angle_t b = *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#endif
#else
#define READUINT8(p)        *((  UINT8 *)p)++
#define READSINT8(p)        *((  SINT8 *)p)++
#define READINT16(p)        *((  INT16 *)p)++
#define READUINT16(p)       *(( UINT16 *)p)++
#define READINT32(p)        *((  INT32 *)p)++
#define READUINT32(p)       *(( UINT32 *)p)++
#define READCHAR(p)         *((   char *)p)++
#define READFIXED(p)        *((fixed_t *)p)++
#define READANGLE(p)        *((angle_t *)p)++
#endif

#else //SRB2_BIG_ENDIAN
//
// definitions for big-endian machines with alignment constraints.
//
// Write a value to a little-endian, unaligned destination.
//
FUNCINLINE static ATTRINLINE void writeshort(void *ptr, INT32 val)
{
	SINT8 *cp = ptr;
	cp[0] = val; val >>= 8;
	cp[1] = val;
}

FUNCINLINE static ATTRINLINE void writelong(void *ptr, INT32 val)
{
	SINT8 *cp = ptr;
	cp[0] = val; val >>= 8;
	cp[1] = val; val >>= 8;
	cp[2] = val; val >>= 8;
	cp[3] = val;
}

#define WRITEUINT8(p,b)     do {  UINT8 *p_tmp = (  UINT8 *)p; *p_tmp       = (  UINT8)(b) ; p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITESINT8(p,b)     do {  SINT8 *p_tmp = (  SINT8 *)p; *p_tmp       = (  SINT8)(b) ; p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEINT16(p,b)     do {  INT16 *p_tmp = (  INT16 *)p; writeshort (p, (  INT16)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEUINT16(p,b)    do { UINT16 *p_tmp = ( UINT16 *)p; writeshort (p, ( UINT16)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEINT32(p,b)     do {  INT32 *p_tmp = (  INT32 *)p; writelong  (p, (  INT32)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEUINT32(p,b)    do { UINT32 *p_tmp = ( UINT32 *)p; writelong  (p, ( UINT32)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITECHAR(p,b)      do {   char *p_tmp = (   char *)p; *p_tmp       = (   char)(b) ; p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEFIXED(p,b)     do {fixed_t *p_tmp = (fixed_t *)p; writelong  (p, (fixed_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEANGLE(p,b)     do {angle_t *p_tmp = (angle_t *)p; writelong  (p, (angle_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)

// Read a signed quantity from little-endian, unaligned data.
//
FUNCINLINE static ATTRINLINE INT16 readshort(void *ptr)
{
	SINT8 *cp  = ptr;
	UINT8 *ucp = ptr;
	return (cp[1] << 8) | ucp[0];
}

FUNCINLINE static ATTRINLINE UINT16 readushort(void *ptr)
{
	UINT8 *ucp = ptr;
	return (ucp[1] << 8) | ucp[0];
}

FUNCINLINE static ATTRINLINE INT32 readlong(void *ptr)
{
	SINT8 *cp = ptr;
	UINT8 *ucp = ptr;
	return (cp[3] << 24) | (ucp[2] << 16) | (ucp[1] << 8) | ucp[0];
}

FUNCINLINE static ATTRINLINE UINT32 readulong(void *ptr)
{
	UINT8 *ucp = ptr;
	return (ucp[3] << 24) | (ucp[2] << 16) | (ucp[1] << 8) | ucp[0];
}

#define READUINT8(p)        ({   UINT8 *p_tmp = (  UINT8 *)p;   UINT8 b =        *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READSINT8(p)        ({   SINT8 *p_tmp = (  SINT8 *)p;   SINT8 b =        *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READINT16(p)        ({   INT16 *p_tmp = (  INT16 *)p;   INT16 b =  readshort(p); p_tmp++; p = (void *)p_tmp; b; })
#define READUINT16(p)       ({  UINT16 *p_tmp = ( UINT16 *)p;  UINT16 b = readushort(p); p_tmp++; p = (void *)p_tmp; b; })
#define READINT32(p)        ({   INT32 *p_tmp = (  INT32 *)p;   INT32 b =   readlong(p); p_tmp++; p = (void *)p_tmp; b; })
#define READUINT32(p)       ({  UINT32 *p_tmp = ( UINT32 *)p;  UINT32 b =  readulong(p); p_tmp++; p = (void *)p_tmp; b; })
#define READCHAR(p)         ({    char *p_tmp = (   char *)p;    char b =        *p_tmp; p_tmp++; p = (void *)p_tmp; b; })
#define READFIXED(p)        ({ fixed_t *p_tmp = (fixed_t *)p; fixed_t b =   readlong(p); p_tmp++; p = (void *)p_tmp; b; })
#define READANGLE(p)        ({ angle_t *p_tmp = (angle_t *)p; angle_t b =  readulong(p); p_tmp++; p = (void *)p_tmp; b; })
#endif //SRB2_BIG_ENDIAN

#undef DEALIGNED

#define WRITESTRINGN(p,s,n) { size_t tmp_i = 0; for (; tmp_i < n && s[tmp_i] != '\0'; tmp_i++) WRITECHAR(p, s[tmp_i]); if (tmp_i < n) WRITECHAR(p, '\0');}
#define WRITESTRING(p,s)    { size_t tmp_i = 0; for (;              s[tmp_i] != '\0'; tmp_i++) WRITECHAR(p, s[tmp_i]); WRITECHAR(p, '\0');}
#define WRITEMEM(p,s,n)     { memcpy(p, s, n); p += n; }

#define SKIPSTRING(p)       while (READCHAR(p) != '\0')

#define READSTRINGN(p,s,n)  { size_t tmp_i = 0; for (; tmp_i < n && (s[tmp_i] = READCHAR(p)) != '\0'; tmp_i++); s[tmp_i] = '\0';}
#define READSTRING(p,s)     { size_t tmp_i = 0; for (;              (s[tmp_i] = READCHAR(p)) != '\0'; tmp_i++); s[tmp_i] = '\0';}
#define READMEM(p,s,n)      { memcpy(s, p, n); p += n; }

#if 0 // old names
#define WRITEBYTE(p,b)      WRITEUINT8(p,b)
#define WRITESHORT(p,b)     WRITEINT16(p,b)
#define WRITEUSHORT(p,b)    WRITEUINT16(p,b)
#define WRITELONG(p,b)      WRITEINT32(p,b)
#define WRITEULONG(p,b)     WRITEUINT32(p,b)

#define READBYTE(p)         READUINT8(p)
#define READSHORT(p)        READINT16(p)
#define READUSHORT(p)       READUINT16(p)
#define READLONG(p)         READINT32(p)
#define READULONG(p)        READUINT32(p)
#endif
