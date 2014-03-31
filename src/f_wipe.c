// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2013-2014 by Matthew "Inuyasha" Walsh.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_wipe.c
/// \brief SRB2 2.1 custom fade mask "wipe" behavior.

#include "f_finale.h"
#include "i_video.h"
#include "v_video.h"

#include "r_draw.h" // transtable
#include "p_pspr.h" // tr_transxxx
#include "w_wad.h"
#include "z_zone.h"

#include "i_system.h"
#include "m_menu.h"
#include "console.h"
#include "d_main.h"
#include "m_misc.h" // movie mode

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#if NUMSCREENS < 5
#define NOWIPE // do not enable wipe image post processing for ARM, SH and MIPS CPUs
#endif

typedef struct fademask_s {
	UINT8* mask;
	UINT16 width, height;
	size_t size;
	fixed_t xscale, yscale;
} fademask_t;

UINT8 wipedefs[NUMWIPEDEFS] = {
	99, // wipe_credits_intermediate (0)

	0,  // wipe_level_toblack
	UINT8_MAX,  // wipe_intermission_toblack
	UINT8_MAX,  // wipe_continuing_toblack
	0,  // wipe_titlescreen_toblack
	0,  // wipe_timeattack_toblack
	99, // wipe_credits_toblack
	0,  // wipe_evaluation_toblack
	0,  // wipe_gameend_toblack
	99, // wipe_intro_toblack (hardcoded)
	99, // wipe_cutscene_toblack (hardcoded)

	0,  // wipe_specinter_toblack
	0,  // wipe_multinter_toblack

	0,  // wipe_level_final
	0,  // wipe_intermission_final
	0,  // wipe_continuing_final
	0,  // wipe_titlescreen_final
	0,  // wipe_timeattack_final
	99, // wipe_credits_final
	0,  // wipe_evaluation_final
	0,  // wipe_gameend_final
	99, // wipe_intro_final (hardcoded)
	99, // wipe_cutscene_final (hardcoded)

	0,  // wipe_specinter_final
	0   // wipe_multinter_final
};

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------

boolean WipeInAction = false;
INT32 lastwipetic = 0;

#ifndef NOWIPE
static UINT8 *wipe_scr_start; //screen 3
static UINT8 *wipe_scr_end; //screen 4
static UINT8 *wipe_scr; //screen 0 (main drawing)
static fixed_t paldiv;

/** Create fademask_t from lump
  *
  * \param	lump	Lump name to get data from
  * \return	fademask_t for lump
  */
static fademask_t *F_GetFadeMask(UINT8 masknum, UINT8 scrnnum) {
	static char lumpname[9] = "FADEmmss";
	static fademask_t fm = {NULL,0,0,0,0,0};
	lumpnum_t lumpnum;
	UINT8 *lump, *mask;
	size_t lsize;
	RGBA_t *pcolor;

	if (masknum > 99 || scrnnum > 99)
		goto freemask;

	sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)masknum, (UINT16)scrnnum);

	lumpnum = W_CheckNumForName(lumpname);
	if (lumpnum == LUMPERROR)
		goto freemask;

	lump = W_CacheLumpNum(lumpnum, PU_CACHE);
	lsize = W_LumpLength(lumpnum);
	switch (lsize)
	{
		case 256000: // 640x400
			fm.width = 640;
			fm.height = 400;
			break;
		case 64000: // 320x200
			fm.width = 320;
			fm.height = 200;
			break;
		case 16000: // 160x100
			fm.width = 160;
			fm.height = 100;
			break;
		case 4000: // 80x50 (minimum)
			fm.width = 80;
			fm.height = 50;
			break;

		default: // bad lump
			CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		case 0: // end marker (not bad!, but still need clearing)
			goto freemask;
	}
	if (lsize != fm.size)
		fm.mask = Z_Realloc(fm.mask, lsize, PU_STATIC, NULL);
	fm.size = lsize;

	mask = fm.mask;

	while (lsize--)
	{
		// Determine pixel to use from fademask
		pcolor = &pLocalPalette[*lump++];
		*mask++ = FixedDiv((pcolor->s.red+1)<<FRACBITS, paldiv)>>FRACBITS;
	}

	fm.xscale = FixedDiv(fm.width<<FRACBITS, vid.width<<FRACBITS);
	fm.yscale = FixedDiv(fm.height<<FRACBITS, vid.height<<FRACBITS);
	return &fm;

	// Landing point for freeing data -- do this instead of just returning NULL
	// this ensures the fade data isn't remaining in memory, unused
	// (could be up to 256,000 bytes if it's a HQ fade!)
	freemask:
	if (fm.mask)
	{
		Z_Free(fm.mask);
		fm.mask = NULL;
		fm.size = 0;
	}

	return NULL;
}

/**	Wipe ticker
  *
  * \param	fademask	pixels to change
  */
static void F_DoWipe(fademask_t *fademask)
{
	// wipe screen, start, end
	UINT8 *w = wipe_scr;
	const UINT8 *s = wipe_scr_start;
	const UINT8 *e = wipe_scr_end;
	UINT8 transval;
	INT32 x = 0, y = 0;

#ifdef HWRENDER
	/// \todo Mask wipes for OpenGL
	if(rendermode != render_soft)
	{
		HWR_DoScreenWipe();
		return;
	}
#endif
	// Software mask wipe
	do
	{
		if (*s != *e)
		{
			transval = fademask->mask[ // y*width + x
				(FixedMul(y<<FRACBITS,fademask->yscale)>>FRACBITS)*fademask->width
				+ (FixedMul(x<<FRACBITS,fademask->xscale)>>FRACBITS)
			];

			if (transval == 0)
				*w = *s;
			else if (transval == 10)
				*w = *e;
			else
				*w = transtables[(*e<<8) + *s + ((9 - transval)<<FF_TRANSSHIFT)];
		}
		if (++x >= vid.width)
		{
			x = 0;
			++y;
		}
	} while (++w && ++s && ++e && w < wipe_scr + vid.width*vid.height);
}
#endif

/** Save the "before" screen of a wipe.
  */
void F_WipeStartScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if(rendermode != render_soft)
	{
		HWR_StartScreenWipe();
		return;
	}
#endif
	wipe_scr_start = screens[3];
	I_ReadScreen(wipe_scr_start);
#endif
}

/** Save the "after" screen of a wipe.
  */
void F_WipeEndScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if(rendermode != render_soft)
	{
		HWR_EndScreenWipe();
		return;
	}
#endif
	wipe_scr_end = screens[4];
	I_ReadScreen(wipe_scr_end);
	V_DrawBlock(0, 0, 0, vid.width, vid.height, wipe_scr_start);
#endif
}

/** After setting up the screens you want to wipe,
  * calling this will do a 'typical' wipe.
  */
void F_RunWipe(UINT8 wipetype, boolean drawMenu)
{
#ifdef NOWIPE
	(void)wipetype;
	(void)drawMenu;
#else
	tic_t nowtime;
	UINT8 wipeframe = 0;
	fademask_t *fmask;

	paldiv = FixedDiv(257<<FRACBITS, 11<<FRACBITS);

	// Init the wipe
	WipeInAction = true;
	wipe_scr = screens[0];

	// lastwipetic should either be 0 or the tic we last wiped
	// on for fade-to-black
	for (;;)
	{
		// get fademask first so we can tell if it exists or not
		fmask = F_GetFadeMask(wipetype, wipeframe++);
		if (!fmask)
			break;

		// wait loop
		while (!((nowtime = I_GetTime()) - lastwipetic))
			I_Sleep();
		lastwipetic = nowtime;

		F_DoWipe(fmask);
		I_OsPolling();
		I_UpdateNoBlit();

		if (drawMenu)
			M_Drawer(); // menu is drawn even on top of wipes

		I_FinishUpdate(); // page flip or blit buffer

		if (moviemode)
			M_SaveFrame();
	}
	WipeInAction = false;
#endif
}
