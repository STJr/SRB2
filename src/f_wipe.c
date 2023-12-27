// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2013-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_wipe.c
/// \brief SRB2 2.1 custom fade mask "wipe" behavior.

#include "f_wipe.h"
#include "i_video.h"
#include "v_video.h"

#include "r_main.h" // framecount
#include "r_state.h" // fadecolormap
#include "r_draw.h" // transtable
#include "p_local.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_time.h"
#include "i_system.h"
#include "m_menu.h"
#include "console.h"
#include "d_main.h"
#include "g_game.h"
#include "y_inter.h" // intertype
#include "m_misc.h" // movie mode

#include "doomstat.h"

#include "lua_hud.h" // level title

#ifdef HWRENDER
#include "hardware/hw_main.h"
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
	0,  // wipe_intermission_toblack
	0,  // wipe_continuing_toblack
	0,  // wipe_titlescreen_toblack
	0,  // wipe_timeattack_toblack
	99, // wipe_credits_toblack
	0,  // wipe_evaluation_toblack
	0,  // wipe_gameend_toblack
	99, // wipe_intro_toblack (hardcoded)
	0,  // wipe_ending_toblack
	99, // wipe_cutscene_toblack (hardcoded)

	0,  // wipe_specinter_toblack
	0,  // wipe_multinter_toblack
	0,  // wipe_speclevel_towhite

	0,  // wipe_level_final
	0,  // wipe_intermission_final
	0,  // wipe_continuing_final
	0,  // wipe_titlescreen_final
	0,  // wipe_timeattack_final
	99, // wipe_credits_final
	0,  // wipe_evaluation_final
	0,  // wipe_gameend_final
	99, // wipe_intro_final (hardcoded)
	0,  // wipe_ending_final
	99, // wipe_cutscene_final (hardcoded)

	0,  // wipe_specinter_final
	0   // wipe_multinter_final
};

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------

#define WIPEQUEUESIZE 8

static wipe_t wipe_queue[WIPEQUEUESIZE];
static unsigned wipe_numqueued = 0;

static UINT8 wipe_type;
static UINT8 wipe_frame;
static boolean wipe_stopped = false;
static tic_t wipe_holdframes = 0;

static wipestyle_t wipe_style = WIPESTYLE_NORMAL;
static wipeflags_t wipe_flags = 0;

specialwipe_t ranspecialwipe = SPECIALWIPE_NONE;

boolean wipe_running = false;
boolean wipe_drawmenuontop = false;

#ifndef NOWIPE
static UINT8 *wipe_scr_start; //screen 3
static UINT8 *wipe_scr_end; //screen 4
static UINT8 *wipe_scr; //screen 0 (main drawing)
static fixed_t paldiv = 0;

/** Create fademask_t from lump
  *
  * \param	lump	Lump name to get data from
  * \return	fademask_t for lump
  */
static fademask_t *F_GetFadeMask(UINT8 masknum, UINT8 scrnnum) {
	static char lumpname[11] = "FADEmmss";
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
		pcolor = &pMasterPalette[*lump++];
		if (wipe_style == WIPESTYLE_COLORMAP)
			*mask++ = pcolor->s.red / FADECOLORMAPDIV;
		else
			*mask++ = FixedDiv((pcolor->s.red+1)<<FRACBITS, paldiv)>>FRACBITS;
	}

	fm.xscale = FixedDiv(vid.width<<FRACBITS, fm.width<<FRACBITS);
	fm.yscale = FixedDiv(vid.height<<FRACBITS, fm.height<<FRACBITS);
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
	// Software mask wipe -- optimized; though it might not look like it!
	// Okay, to save you wondering *how* this is more optimized than the simpler
	// version that came before it...
	// ---
	// The previous code did two FixedMul calls for every single pixel on the
	// screen, of which there are hundreds of thousands -- if not millions -- of.
	// This worked fine for smaller screen sizes, but with excessively large
	// (1920x1200) screens that meant 4 million+ calls out to FixedMul, and that
	// would take /just/ long enough that fades would start to noticably lag.
	// ---
	// This code iterates over the fade mask's pixels instead of the screen's,
	// and deals with drawing over each rectangular area before it moves on to
	// the next pixel in the fade mask.  As a result, it's more complex (and might
	// look a little messy; sorry!) but it simultaneously runs at twice the speed.
	// In addition, we precalculate all the X and Y positions that we need to draw
	// from and to, so it uses a little extra memory, but again, helps it run faster.
	{
		// wipe screen, start, end
		UINT8       *w = wipe_scr;
		const UINT8 *s = wipe_scr_start;
		const UINT8 *e = wipe_scr_end;

		// first pixel for each screen
		UINT8       *w_base = w;
		const UINT8 *s_base = s;
		const UINT8 *e_base = e;

		// mask data, end
		UINT8       *transtbl;
		const UINT8 *mask    = fademask->mask;
		const UINT8 *maskend = mask + fademask->size;

		// rectangle draw hints
		UINT32 draw_linestart, draw_rowstart;
		UINT32 draw_lineend,   draw_rowend;
		UINT32 draw_linestogo, draw_rowstogo;

		// rectangle coordinates, etc.
		UINT16* scrxpos = (UINT16*)malloc((fademask->width + 1)  * sizeof(UINT16));
		UINT16* scrypos = (UINT16*)malloc((fademask->height + 1) * sizeof(UINT16));
		UINT16 maskx, masky;
		UINT32 relativepos;

		// ---
		// Screw it, we do the fixed point math ourselves up front.
		scrxpos[0] = 0;
		for (relativepos = 0, maskx = 1; maskx < fademask->width; ++maskx)
			scrxpos[maskx] = (relativepos += fademask->xscale)>>FRACBITS;
		scrxpos[fademask->width] = vid.width;

		scrypos[0] = 0;
		for (relativepos = 0, masky = 1; masky < fademask->height; ++masky)
			scrypos[masky] = (relativepos += fademask->yscale)>>FRACBITS;
		scrypos[fademask->height] = vid.height;
		// ---

		maskx = masky = 0;
		do
		{
			draw_rowstart = scrxpos[maskx];
			draw_rowend   = scrxpos[maskx + 1];
			draw_linestart = scrypos[masky];
			draw_lineend   = scrypos[masky + 1];

			relativepos = (draw_linestart * vid.width) + draw_rowstart;
			draw_linestogo = draw_lineend - draw_linestart;

			if (*mask == 0)
			{
				// shortcut - memcpy source to work
				while (draw_linestogo--)
				{
					M_Memcpy(w_base+relativepos, s_base+relativepos, draw_rowend-draw_rowstart);
					relativepos += vid.width;
				}
			}
			else if (*mask >= 10)
			{
				// shortcut - memcpy target to work
				while (draw_linestogo--)
				{
					M_Memcpy(w_base+relativepos, e_base+relativepos, draw_rowend-draw_rowstart);
					relativepos += vid.width;
				}
			}
			else
			{
				// pointer to transtable that this mask would use
				transtbl = R_GetTranslucencyTable((9 - *mask) + 1);

				// DRAWING LOOP
				while (draw_linestogo--)
				{
					w = w_base + relativepos;
					s = s_base + relativepos;
					e = e_base + relativepos;
					draw_rowstogo = draw_rowend - draw_rowstart;

					while (draw_rowstogo--)
						*w++ = transtbl[ ( *e++ << 8 ) + *s++ ];

					relativepos += vid.width;
				}
				// END DRAWING LOOP
			}

			if (++maskx >= fademask->width)
				++masky, maskx = 0;
		} while (++mask < maskend);

		free(scrxpos);
		free(scrypos);
	}
}

static void F_DoColormapWipe(fademask_t *fademask, UINT8 *colormap)
{
	// Lactozilla: F_DoWipe for WIPESTYLE_COLORMAP
	{
		// wipe screen, start, end
		UINT8       *w = wipe_scr;
		const UINT8 *e = wipe_scr_end;

		// first pixel for each screen
		UINT8       *w_base = w;
		const UINT8 *e_base = e;

		// mask data, end
		UINT8       *transtbl;
		const UINT8 *mask    = fademask->mask;
		const UINT8 *maskend = mask + fademask->size;

		// rectangle draw hints
		UINT32 draw_linestart, draw_rowstart;
		UINT32 draw_lineend,   draw_rowend;
		UINT32 draw_linestogo, draw_rowstogo;

		// rectangle coordinates, etc.
		UINT16* scrxpos = (UINT16*)malloc((fademask->width + 1)  * sizeof(UINT16));
		UINT16* scrypos = (UINT16*)malloc((fademask->height + 1) * sizeof(UINT16));
		UINT16 maskx, masky;
		UINT32 relativepos;

		// ---
		// Screw it, we do the fixed point math ourselves up front.
		scrxpos[0] = 0;
		for (relativepos = 0, maskx = 1; maskx < fademask->width; ++maskx)
			scrxpos[maskx] = (relativepos += fademask->xscale)>>FRACBITS;
		scrxpos[fademask->width] = vid.width;

		scrypos[0] = 0;
		for (relativepos = 0, masky = 1; masky < fademask->height; ++masky)
			scrypos[masky] = (relativepos += fademask->yscale)>>FRACBITS;
		scrypos[fademask->height] = vid.height;
		// ---

		maskx = masky = 0;
		do
		{
			draw_rowstart = scrxpos[maskx];
			draw_rowend   = scrxpos[maskx + 1];
			draw_linestart = scrypos[masky];
			draw_lineend   = scrypos[masky + 1];

			relativepos = (draw_linestart * vid.width) + draw_rowstart;
			draw_linestogo = draw_lineend - draw_linestart;

			int nmask = *mask;
			if (wipe_flags & WIPEFLAGS_FADEIN)
				nmask = (FADECOLORMAPROWS-1) - nmask;

			transtbl = colormap + (nmask * 256);

			// DRAWING LOOP
			while (draw_linestogo--)
			{
				w = w_base + relativepos;
				e = e_base + relativepos;
				draw_rowstogo = draw_rowend - draw_rowstart;

				while (draw_rowstogo--)
					*w++ = transtbl[*e++];

				relativepos += vid.width;
			}
			// END DRAWING LOOP

			if (++maskx >= fademask->width)
				++masky, maskx = 0;
		} while (++mask < maskend);

		free(scrxpos);
		free(scrypos);
	}
}
#endif

/** Saves the "before" screen of a wipe.
  */
void ScreenWipe_StartScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_StartScreenWipe();
		return;
	}
#endif

	wipe_scr_start = screens[3];
	I_ReadScreen(wipe_scr_start);
#endif
}

/** Saves the "after" screen of a wipe.
  */
void ScreenWipe_EndScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_EndScreenWipe();
		return;
	}
#endif

	wipe_scr_end = screens[4];
	I_ReadScreen(wipe_scr_end);
#endif
}

static boolean ScreenWipe_CanTint(wipeflags_t flags)
{
	if (flags & WIPEFLAGS_CROSSFADE)
		return false;

	return true;
}

/** Decides what wipe style to use.
  */
wipestyle_t ScreenWipe_GetStyle(wipeflags_t flags)
{
	if (ScreenWipe_CanTint(flags))
		return WIPESTYLE_COLORMAP;
	else
		return WIPESTYLE_NORMAL;
}

static void F_RestartWipe(void)
{
	if (!wipe_numqueued)
		return;

	wipe_t *curwipe = &wipe_queue[0];

	wipe_running = true;
	wipe_stopped = false;
	wipe_type = curwipe->type;
	wipe_style = curwipe->style;
	wipe_flags = curwipe->flags;
	wipe_holdframes = curwipe->holdframes;
	wipe_drawmenuontop = curwipe->drawmenuontop;
	wipe_frame = 0;
}

void ScreenWipe_StartPending(void)
{
	if (wipe_numqueued)
		F_RestartWipe();
}

/** After setting up the screens you want to wipe,
  * calling this will do a 'typical' wipe.
  */
void ScreenWipe_Start(UINT8 type, wipeflags_t flags)
{
	wipe_t wipe = {0};
	wipe.style = ScreenWipe_GetStyle(flags);
	wipe.flags = flags;
	wipe.type = type;
	wipe.drawmenuontop = true;
	ScreenWipe_StartParametrized(&wipe);
}

void ScreenWipe_StartParametrized(wipe_t *wipe)
{
#ifdef NOWIPE
	(void)wipe;
#else
	if (!paldiv)
		paldiv = FixedDiv(257<<FRACBITS, 11<<FRACBITS);
#endif

	if (wipe_numqueued >= WIPEQUEUESIZE || wipe->type == UINT8_MAX)
	{
		// Can't queue it, but its callback has to run.
		if (wipe->callback)
			wipe->callback();
		return;
	}

	wipe_queue[wipe_numqueued] = *wipe;
	wipe_numqueued++;
}

/** Runs the current wipe.
  */
void ScreenWipe_Run(void)
{
#ifndef NOWIPE
	if (!wipe_numqueued)
		return;

	if (wipe_stopped)
	{
		wipe_holdframes--;
		if (wipe_holdframes <= 0)
			ScreenWipe_Stop();
		return;
	}

	fademask_t *fmask = F_GetFadeMask(wipe_type, wipe_frame);
	if (!fmask)
	{
		wipe_stopped = true;
		if (wipe_holdframes)
		{
			if (!(wipe_flags & WIPEFLAGS_FADEIN))
				wipe_frame--;
			return;
		}
		ScreenWipe_Stop();
		return;
	}

	wipe_frame++;
#else
	ScreenWipe_Stop();
#endif
}

/** Stops running the current wipe.
  */
void ScreenWipe_Stop(void)
{
	void (*callback)(void) = NULL;

	if (wipe_numqueued)
	{
		callback = wipe_queue[0].callback;

		wipe_numqueued--;

		if (wipe_numqueued)
			memmove(&wipe_queue[0], &wipe_queue[1], wipe_numqueued * sizeof(wipe_t));
	}

	wipe_stopped = false;
	wipe_running = false;
	wipe_drawmenuontop = false;
	wipe_holdframes = 0;

	if (titlecard.wipe)
		titlecard.wipe = 0;

	if (callback)
		callback();
}

void ScreenWipe_StopAll(void)
{
	wipe_numqueued = 0;

	ScreenWipe_Stop();
}

#ifndef NOWIPE
/** Renders the current wipe into wipe_scr.
  */
static void F_RenderWipe(UINT8 style, UINT8 flags, fademask_t *fmask)
{
	if (style == WIPESTYLE_COLORMAP)
	{
		UINT8 *colormap = fadecolormap;
		if (flags & WIPEFLAGS_TOWHITE)
			colormap += (FADECOLORMAPROWS * 256);
		F_DoColormapWipe(fmask, colormap);
	}
	else
	{
		F_DoWipe(fmask);
	}
}

#ifdef HWRENDER
static void F_RenderWipeHW(UINT8 style, UINT8 type, UINT8 frame)
{
	// send in the wipe type and wipe frame because we need to cache the graphic
	if (style == WIPESTYLE_COLORMAP)
		HWR_DoTintedWipe(type, frame);
	else
		HWR_DoWipe(type, frame);
}
#endif
#endif

/** Displays the current wipe.
  */
void ScreenWipe_Display(void)
{
#ifndef NOWIPE
	wipe_scr = screens[0];

	// get fademask first so we can tell if it exists or not
	fademask_t *fmask = F_GetFadeMask(wipe_type, wipe_frame);
	if (!fmask)
	{
		// Save screen for post-wipe
		fmask = F_GetFadeMask(wipe_type, wipe_frame-1);
		if (fmask)
		{
#ifdef HWRENDER
			if (rendermode == render_opengl)
				F_RenderWipeHW(wipe_style, wipe_type, wipe_frame-1);
			else
#endif
				F_RenderWipe(wipe_style, wipe_flags, fmask);
			ScreenWipe_StartScreen();
		}
		return;
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
		F_RenderWipeHW(wipe_style, wipe_type, wipe_frame);
	else
#endif
		F_RenderWipe(wipe_style, wipe_flags, fmask);
#endif
}

static int F_GetWipedefIndex(void)
{
	int index = gamestate; // wipe_xxx_toblack

	if (gamestate == GS_INTERMISSION)
	{
		if (intertype == int_spec) // Special Stage
			index = wipe_specinter_toblack;
		else if (intertype != int_coop) // Multiplayer
			index = wipe_multinter_toblack;
	}

	return index;
}

wipe_t *ScreenWipe_GetQueued(void)
{
	if (wipe_numqueued)
		return &wipe_queue[0];

	return NULL;
}

void ScreenWipe_SetupFadeOut(wipeflags_t flags)
{
#ifndef NOWIPE
	ScreenWipe_StartScreen();

	UINT8 wipecolor = (flags & WIPEFLAGS_TOWHITE) ? 0 : 31;

	if (ScreenWipe_CanTint(flags))
	{
#ifdef HWRENDER
		if (rendermode == render_opengl)
			ScreenWipe_DoColorFill(wipecolor);
#endif
	}
	else
	{
		ScreenWipe_DoColorFill(wipecolor);
	}

	ScreenWipe_EndScreen();
#endif
}

void ScreenWipe_DoFadeOutIn(void)
{
	ScreenWipe_DoFadeOut(DEFAULTWIPE, 0, NULL);
	ScreenWipe_DoFadeIn(DEFAULTWIPE, WIPEFLAGS_FADEIN, NULL);
}

/** Starts the "pre" type of a wipe.
  */
void ScreenWipe_DoFadeOut(INT16 type, wipeflags_t flags, wipe_callback_t callback)
{
	if (type == DEFAULTWIPE || !ScreenWipe_Exists(type))
		type = wipedefs[F_GetWipedefIndex()];

	wipe_t wipe = {0};
	wipe.flags = flags;
	wipe.style = ScreenWipe_GetStyle(flags);
	wipe.type = type;
	wipe.drawmenuontop = gamestate != GS_TIMEATTACK && gamestate != GS_TITLESCREEN;
	wipe.callback = callback;
	ScreenWipe_StartParametrized(&wipe);
}

/** Starts the "post" type of a wipe.
  */
void ScreenWipe_DoFadeIn(INT16 type, wipeflags_t flags, wipe_callback_t callback)
{
	if (type == DEFAULTWIPE || !ScreenWipe_Exists(type))
		type = wipedefs[F_GetWipedefIndex() + WIPEFINALSHIFT];

	wipe_t wipe = {0};
	wipe.flags = flags;
	wipe.style = ScreenWipe_GetStyle(flags);
	wipe.type = type;
	wipe.drawmenuontop = gamestate != GS_TIMEATTACK && gamestate != GS_TITLESCREEN;
	wipe.callback = callback;
	ScreenWipe_StartParametrized(&wipe);
}

/** Does a crossfade.
  */
void ScreenWipe_DoCrossfade(INT16 type)
{
	wipe_t wipe = {0};
	wipe.flags = WIPEFLAGS_CROSSFADE;
	wipe.style = ScreenWipe_GetStyle(wipe.flags);
	wipe.type = type == DEFAULTWIPE ? wipedefs[F_GetWipedefIndex()] : type;
	wipe.drawmenuontop = false;
	ScreenWipe_StartParametrized(&wipe);
}

/** Returns tic length of wipe
  * One lump equals one tic
  */
tic_t ScreenWipe_GetLength(UINT8 type)
{
#ifdef NOWIPE
	(void)type;
	return 0;
#else
	static char lumpname[11] = "FADEmmss";
	lumpnum_t lumpnum;
	UINT8 frame;

	if (type > 99)
		return 0;

	for (frame = 0; frame < 100; frame++)
	{
		sprintf(&lumpname[4], "%.2hu%.2hu", (UINT8)type, (UINT8)frame);

		lumpnum = W_CheckNumForName(lumpname);
		if (lumpnum == LUMPERROR)
			return --frame;
	}
	return --frame;
#endif
}

/** Does the specified wipe exist?
  */
boolean ScreenWipe_Exists(UINT8 type)
{
#ifdef NOWIPE
	(void)type;
	return false;
#else
	static char lumpname[11] = "FADEmm00";
	lumpnum_t lumpnum;

	if (type > 99)
		return false;

	sprintf(&lumpname[4], "%.2hu00", (UINT16)type);

	lumpnum = W_CheckNumForName(lumpname);
	return !(lumpnum == LUMPERROR);
#endif
}
