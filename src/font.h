// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1999-2018 by Sonic Team Junior.
// Copyright (C)      2019 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  font.h
/// \brief Font setup

#ifndef __FONT_H__
#define __FONT_H__

#define MAX_FONTS 32

typedef struct font font_t;

struct font
{
	patch_t **font;

	UINT8     start;
	UINT8     size;

	char      prefix[8];/* 7 used at most */
	unsigned  digits : 2;
};

extern font_t fontv[MAX_FONTS];
extern int    fontc;

/*
Reloads already registered fonts.
*/
void Font_Load (void);

/*
Registers and loads a new font.
*/
int Font_Register     (const font_t *);

/*
Register a new font, but do not load it yet.
*/
int Font_DumbRegister (const font_t *);

#endif
