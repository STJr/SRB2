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
/// \file  m_misc.h
/// \brief Commonly used routines
///        Default config file, PCX screenshots, file i/o

#ifndef __M_MISC__
#define __M_MISC__

#include "doomtype.h"
#include "tables.h"

#include "d_event.h" // Screenshot responder

typedef enum {
	MM_OFF = 0,
	MM_APNG,
	MM_GIF,
	MM_SCREENSHOT
} moviemode_t;
extern moviemode_t moviemode;

void M_StartMovie(void);
void M_SaveFrame(void);
void M_StopMovie(void);

// the file where game vars and settings are saved
#ifdef DC
#define CONFIGFILENAME "srb2dc.cfg"
#elif defined (PSP)
#define CONFIGFILENAME "srb2psp.cfg"
#else
#define CONFIGFILENAME "config.cfg"
#endif

INT32 M_MapNumber(char first, char second);

boolean FIL_WriteFile(char const *name, const void *source, size_t length);
size_t FIL_ReadFileTag(char const *name, UINT8 **buffer, INT32 tag);
#define FIL_ReadFile(n, b) FIL_ReadFileTag(n, b, PU_STATIC)

boolean FIL_FileExists(const char *name);
boolean FIL_WriteFileOK(char const *name);
boolean FIL_ReadFileOK(char const *name);
boolean FIL_FileOK(char const *name);

void FIL_DefaultExtension (char *path, const char *extension);
void FIL_ForceExtension(char *path, const char *extension);
boolean FIL_CheckExtension(const char *in);

#ifdef HAVE_PNG
boolean M_SavePNG(const char *filename, void *data, int width, int height, const UINT8 *palette);
#endif

extern boolean takescreenshot;
void M_ScreenShot(void);
void M_DoScreenShot(void);
boolean M_ScreenshotResponder(event_t *ev);

void Command_SaveConfig_f(void);
void Command_LoadConfig_f(void);
void Command_ChangeConfig_f(void);

void M_FirstLoadConfig(void);
// save game config: cvars, aliases..
void M_SaveConfig(const char *filename);

INT32 axtoi(const char *hexStg);

const char *GetRevisionString(void);

// Vector/matrix math
typedef fixed_t TVector[4];
typedef fixed_t TMatrix[4][4];

TVector *VectorMatrixMultiply(TVector v, TMatrix m);
TMatrix *RotateXMatrix(angle_t rad);
#if 0
TMatrix *RotateYMatrix(angle_t rad);
#endif
TMatrix *RotateZMatrix(angle_t rad);

// s1 = s2+s3+s1 (1024 lenghtmax)
void strcatbf(char *s1, const char *s2, const char *s3);

// The first character of the current line.
// Works even when n is the position of a linefeed character.
size_t M_StartOfLine (const char *s, size_t n);

void M_SetupMemcpy(void);

// counting bits, for weapon ammo code, usually
FUNCMATH UINT8 M_CountBits(UINT32 num, UINT8 size);

#include "w_wad.h"
extern char configfile[MAX_WADPATH];

#endif
