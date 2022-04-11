// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2000 by Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko (prboom)
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_fps.h
/// \brief Uncapped framerate stuff.

#include "r_fps.h"

#include "r_main.h"
#include "g_game.h"
#include "i_video.h"
#include "r_plane.h"
#include "p_spec.h"
#include "r_state.h"
#ifdef HWRENDER
#include "hardware/hw_main.h" // for cv_glshearing
#endif

static viewvars_t p1view_old;
static viewvars_t p1view_new;
static viewvars_t p2view_old;
static viewvars_t p2view_new;
static viewvars_t sky1view_old;
static viewvars_t sky1view_new;
static viewvars_t sky2view_old;
static viewvars_t sky2view_new;

static viewvars_t *oldview = &p1view_old;
static BOOL oldview_valid = FALSE;
viewvars_t *newview = &p1view_new;


enum viewcontext_e viewcontext = VIEWCONTEXT_PLAYER1;

static fixed_t R_LerpFixed(fixed_t from, fixed_t to, fixed_t frac)
{
	return FixedMul(frac, to - from);
}

static angle_t R_LerpAngle(angle_t from, angle_t to, fixed_t frac)
{
	return FixedMul(frac, to - from);
}

// recalc necessary stuff for mouseaiming
// slopes are already calculated for the full possible view (which is 4*viewheight).
// 18/08/18: (No it's actually 16*viewheight, thanks Jimita for finding this out)
static void R_SetupFreelook(player_t *player, boolean skybox)
{
#ifndef HWRENDER
	(void)player;
	(void)skybox;
#endif

	// clip it in the case we are looking a hardware 90 degrees full aiming
	// (lmps, network and use F12...)
	if (rendermode == render_soft
#ifdef HWRENDER
		|| (rendermode == render_opengl
			&& (cv_glshearing.value == 1
			|| (cv_glshearing.value == 2 && R_IsViewpointThirdPerson(player, skybox))))
#endif
		)
	{
		G_SoftwareClipAimingPitch((INT32 *)&aimingangle);
	}

	centeryfrac = (viewheight/2)<<FRACBITS;

	if (rendermode == render_soft)
		centeryfrac += FixedMul(AIMINGTODY(aimingangle), FixedDiv(viewwidth<<FRACBITS, BASEVIDWIDTH<<FRACBITS));

	centery = FixedInt(FixedRound(centeryfrac));

	if (rendermode == render_soft)
		yslope = &yslopetab[viewheight*8 - centery];
}

#undef AIMINGTODY

void R_InterpolateView(fixed_t frac)
{
	viewvars_t* prevview = oldview;
	boolean skybox = 0;
	if (FIXED_TO_FLOAT(frac) < 0)
		frac = 0;
	if (frac > FRACUNIT)
		frac = FRACUNIT;

	if (oldview_valid == FALSE)
	{
		// interpolate from newview to newview
		prevview = newview;
	}

	viewx = prevview->x + R_LerpFixed(prevview->x, newview->x, frac);
	viewy = prevview->y + R_LerpFixed(prevview->y, newview->y, frac);
	viewz = prevview->z + R_LerpFixed(prevview->z, newview->z, frac);

	viewangle = prevview->angle + R_LerpAngle(prevview->angle, newview->angle, frac);
	aimingangle = prevview->aim + R_LerpAngle(prevview->aim, newview->aim, frac);

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	// this is gonna create some interesting visual errors for long distance teleports...
	// might want to recalculate the view sector every frame instead...
	viewplayer = newview->player;
	viewsector = R_PointInSubsector(viewx, viewy)->sector;

	// well, this ain't pretty
	if (newview == &sky1view_new || newview == &sky2view_new)
	{
		skybox = 1;
	}

	R_SetupFreelook(newview->player, skybox);
}

void R_UpdateViewInterpolation(void)
{
	p1view_old = p1view_new;
	p2view_old = p2view_new;
	sky1view_old = sky1view_new;
	sky2view_old = sky2view_new;
	oldview_valid = TRUE;
}

void R_ResetViewInterpolation(void)
{
	oldview_valid = FALSE;
}

void R_SetViewContext(enum viewcontext_e _viewcontext)
{
	I_Assert(_viewcontext == VIEWCONTEXT_PLAYER1
			|| _viewcontext == VIEWCONTEXT_PLAYER2
			|| _viewcontext == VIEWCONTEXT_SKY1
			|| _viewcontext == VIEWCONTEXT_SKY2);
	viewcontext = _viewcontext;

	switch (viewcontext)
	{
		case VIEWCONTEXT_PLAYER1:
			oldview = &p1view_old;
			newview = &p1view_new;
			break;
		case VIEWCONTEXT_PLAYER2:
			oldview = &p2view_old;
			newview = &p2view_new;
			break;
		case VIEWCONTEXT_SKY1:
			oldview = &sky1view_old;
			newview = &sky1view_new;
			break;
		case VIEWCONTEXT_SKY2:
			oldview = &sky2view_old;
			newview = &sky2view_new;
			break;
		default:
			I_Error("viewcontext value is invalid: we should never get here without an assert!!");
			break;
	}
}
