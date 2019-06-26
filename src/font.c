// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1999-2018 by Sonic Team Junior.
// Copyright (C)      2019 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  font.c
/// \brief Font setup

#include "doomdef.h"
#include "hu_stuff.h"
#include "font.h"
#include "z_zone.h"

font_t       fontv[MAX_FONTS];
int          fontc;

static void
FontCache (font_t *fnt)
{
	int i;
	int c;

	c = fnt->start;
	for (i = 0; i < fnt->size; ++i, ++c)
	{
		fnt->font[i] = HU_CachePatch(
				"%s%.*d",
				fnt->prefix,
				fnt->digits,
				c);
	}
}

void
Font_Load (void)
{
	int i;
	for (i = 0; i < fontc; ++i)
	{
		FontCache(&fontv[i]);
	}
}

int
Font_DumbRegister (const font_t *sfnt)
{
	font_t *fnt;

	if (fontc == MAX_FONTS)
		return -1;

	fnt = &fontv[fontc];

	memcpy(fnt, sfnt, sizeof (font_t));

	if (!( fnt->font = ZZ_Alloc(sfnt->size * sizeof (patch_t *)) ))
		return -1;

	return fontc++;
}

int
Font_Register (const font_t *sfnt)
{
	int d;

	d = Font_DumbRegister(sfnt);

	if (d >= 0)
		FontCache(&fontv[d]);

	return d;
}
