// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_main.c
/// \brief Rendering main loop and setup functions,
///        utility functions (BSP, geometry, trigonometry).
///        See tables.c, too.

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h" // faB(21jan): testing
#include "r_sky.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "am_map.h"
#include "d_main.h"
#include "v_video.h"
#include "p_spec.h" // skyboxmo
#include "p_setup.h"
#include "z_zone.h"
#include "m_random.h" // quake camera shake
#include "r_portal.h"
#include "r_main.h"
#include "i_system.h" // I_GetTimeMicros

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
INT64 mycount;
INT64 mytotal = 0;
//unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// increment every time a check is made
size_t validcount = 1;

INT32 centerx, centery;

fixed_t centerxfrac, centeryfrac;
fixed_t projection;
fixed_t projectiony; // aspect ratio
fixed_t fovtan; // field of view

// just for profiling purposes
size_t framecount;

size_t loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle;
fixed_t viewcos, viewsin;
sector_t *viewsector;
player_t *viewplayer;
mobj_t *r_viewmobj;

//
// precalculated math tables
//
angle_t clipangle;
angle_t doubleclipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
INT32 viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[MAXVIDWIDTH+1];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// Hack to support extra boom colormaps.
extracolormap_t *extra_colormaps = NULL;

// Render stats
int rs_prevframetime = 0;
int rs_rendercalltime = 0;
int rs_uitime = 0;
int rs_swaptime = 0;
int rs_tictime = 0;

int rs_bsptime = 0;

int rs_sw_portaltime = 0;
int rs_sw_planetime = 0;
int rs_sw_maskedtime = 0;

int rs_numbspcalls = 0;
int rs_numsprites = 0;
int rs_numdrawnodes = 0;
int rs_numpolyobjects = 0;

static CV_PossibleValue_t drawdist_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{3072, "3072"},	{4096, "4096"},	{6144, "6144"},
	{8192, "8192"},	{0, "Infinite"},	{0, NULL}};

//static CV_PossibleValue_t precipdensity_cons_t[] = {{0, "None"}, {1, "Light"}, {2, "Moderate"}, {4, "Heavy"}, {6, "Thick"}, {8, "V.Thick"}, {0, NULL}};

static CV_PossibleValue_t drawdist_precip_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{0, "None"},	{0, NULL}};

static CV_PossibleValue_t fov_cons_t[] = {{60*FRACUNIT, "MIN"}, {179*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t translucenthud_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
static CV_PossibleValue_t maxportals_cons_t[] = {{0, "MIN"}, {12, "MAX"}, {0, NULL}}; // lmao rendering 32 portals, you're a card
static CV_PossibleValue_t homremoval_cons_t[] = {{0, "No"}, {1, "Yes"}, {2, "Flash"}, {0, NULL}};

static void Fov_OnChange(void);
static void ChaseCam_OnChange(void);
static void ChaseCam2_OnChange(void);
static void FlipCam_OnChange(void);
static void FlipCam2_OnChange(void);
void SendWeaponPref(void);
void SendWeaponPref2(void);

consvar_t cv_tailspickup = {"tailspickup", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasecam = {"chasecam", "On", CV_CALL, CV_OnOff, ChaseCam_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasecam2 = {"chasecam2", "On", CV_CALL, CV_OnOff, ChaseCam2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_flipcam = {"flipcam", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_flipcam2 = {"flipcam2", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam2_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_shadow = {"shadow", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_skybox = {"skybox", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowmlook = {"allowmlook", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_showhud = {"showhud", "Yes", CV_CALL,  CV_YesNo, R_SetViewSize, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_translucenthud = {"translucenthud", "10", CV_SAVE, translucenthud_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_translucency = {"translucency", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_drawdist = {"drawdist", "Infinite", CV_SAVE, drawdist_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_drawdist_nights = {"drawdist_nights", "2048", CV_SAVE, drawdist_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_drawdist_precip = {"drawdist_precip", "1024", CV_SAVE, drawdist_precip_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
//consvar_t cv_precipdensity = {"precipdensity", "Moderate", CV_SAVE, precipdensity_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_fov = {"fov", "90", CV_FLOAT|CV_CALL, fov_cons_t, Fov_OnChange, 0, NULL, NULL, 0, 0, NULL};

// Okay, whoever said homremoval causes a performance hit should be shot.
consvar_t cv_homremoval = {"homremoval", "No", CV_SAVE, homremoval_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_maxportals = {"maxportals", "2", CV_SAVE, maxportals_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_renderstats = {"renderstats", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

void SplitScreen_OnChange(void)
{
	if (!cv_debug && netgame)
	{
		if (splitscreen)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Splitscreen not supported in netplay, sorry!\n"));
			splitscreen = false;
		}
		return;
	}

	// recompute screen size
	R_ExecuteSetViewSize();

	if (!demoplayback && !botingame)
	{
		if (splitscreen)
			CL_AddSplitscreenPlayer();
		else
			CL_RemoveSplitscreenPlayer();

		if (server && !netgame)
			multiplayer = splitscreen;
	}
	else
	{
		INT32 i;
		secondarydisplayplayer = consoleplayer;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && i != consoleplayer)
			{
				secondarydisplayplayer = i;
				break;
			}
	}
}
static void Fov_OnChange(void)
{
	// Shouldn't be needed with render parity?
	//if ((netgame || multiplayer) && !cv_debug && cv_fov.value != 90*FRACUNIT)
	//	CV_Set(&cv_fov, cv_fov.defaultvalue);

	R_SetViewSize();
}

static void ChaseCam_OnChange(void)
{
	if (!cv_chasecam.value || !cv_useranalog[0].value)
		CV_SetValue(&cv_analog[0], 0);
	else
		CV_SetValue(&cv_analog[0], 1);
}

static void ChaseCam2_OnChange(void)
{
	if (botingame)
		return;
	if (!cv_chasecam2.value || !cv_useranalog[1].value)
		CV_SetValue(&cv_analog[1], 0);
	else
		CV_SetValue(&cv_analog[1], 1);
}

static void FlipCam_OnChange(void)
{
	SendWeaponPref();
}

static void FlipCam2_OnChange(void)
{
	SendWeaponPref2();
}

//
// R_PointOnSide
// Traverse BSP (sub) tree,
// check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//
INT32 R_PointOnSide(fixed_t x, fixed_t y, node_t *node)
{
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;

	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;

	x -= node->x;
	y -= node->y;

	// Try to quickly decide by looking at sign bits.
	if ((node->dy ^ node->dx ^ x ^ y) < 0)
		return (node->dy ^ x) < 0;  // (left is negative)
	return FixedMul(y, node->dx>>FRACBITS) >= FixedMul(node->dy>>FRACBITS, x);
}

// killough 5/2/98: reformatted
INT32 R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
	fixed_t lx = line->v1->x;
	fixed_t ly = line->v1->y;
	fixed_t ldx = line->v2->x - lx;
	fixed_t ldy = line->v2->y - ly;

	if (!ldx)
		return x <= lx ? ldy > 0 : ldy < 0;

	if (!ldy)
		return y <= ly ? ldx < 0 : ldx > 0;

	x -= lx;
	y -= ly;

	// Try to quickly decide by looking at sign bits.
	if ((ldy ^ ldx ^ x ^ y) < 0)
		return (ldy ^ x) < 0;          // (left is negative)
	return FixedMul(y, ldx>>FRACBITS) >= FixedMul(ldy>>FRACBITS, x);
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

// This version uses 64-bit variables to avoid overflows with large values.
// Currently used only by OpenGL rendering.
angle_t R_PointToAngle64(INT64 x, INT64 y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDivEx(y,x)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x,y)] :                               // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDivEx(y,x)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x,y)] :                              // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDivEx(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x,y)] :                             // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDivEx(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x,y)] :                              // octant 5
		0;
}

angle_t R_PointToAngle2(fixed_t pviewx, fixed_t pviewy, fixed_t x, fixed_t y)
{
	return (y -= pviewy, (x -= pviewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1)
{
	angle_t angle;
	fixed_t dx, dy, dist;

	dx = abs(px1 - px2);
	dy = abs(py1 - py2);

	if (dy > dx)
	{
		fixed_t temp;

		temp = dx;
		dx = dy;
		dy = temp;
	}
	if (!dy)
		return dx;

	angle = (tantoangle[FixedDiv(dy, dx)>>DBITS] + ANGLE_90) >> ANGLETOFINESHIFT;

	// use as cosine
	dist = FixedDiv(dx, FINESINE(angle));

	return dist;
}

// Little extra utility. Works in the same way as R_PointToAngle2
fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	return R_PointToDist2(viewx, viewy, x, y);
}

angle_t R_PointToAngleEx(INT32 x2, INT32 y2, INT32 x1, INT32 y1)
{
	INT64 dx = x1-x2;
	INT64 dy = y1-y2;
	if (dx < INT32_MIN || dx > INT32_MAX || dy < INT32_MIN || dy > INT32_MAX)
	{
		x1 = (int)(dx / 2 + x2);
		y1 = (int)(dy / 2 + y2);
	}
	return (y1 -= y2, (x1 -= x2) || y1) ?
	x1 >= 0 ?
	y1 >= 0 ?
		(x1 > y1) ? tantoangle[SlopeDivEx(y1,x1)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x1,y1)] :                               // octant 1
		x1 > (y1 = -y1) ? 0-tantoangle[SlopeDivEx(y1,x1)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x1,y1)] :                              // octant 7
		y1 >= 0 ? (x1 = -x1) > y1 ? ANGLE_180-tantoangle[SlopeDivEx(y1,x1)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x1,y1)] :                             // octant 2
		(x1 = -x1) > (y1 = -y1) ? ANGLE_180+tantoangle[SlopeDivEx(y1,x1)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x1,y1)] :                              // octant 5
		0;
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
// killough 5/2/98: reformatted, cleaned up
//
// note: THIS IS USED ONLY FOR WALLS!
fixed_t R_ScaleFromGlobalAngle(angle_t visangle)
{
	angle_t anglea = ANGLE_90 + (visangle-viewangle);
	angle_t angleb = ANGLE_90 + (visangle-rw_normalangle);
	fixed_t den = FixedMul(rw_distance, FINESINE(anglea>>ANGLETOFINESHIFT));
	// proff 11/06/98: Changed for high-res
	fixed_t num = FixedMul(projectiony, FINESINE(angleb>>ANGLETOFINESHIFT));

	if (den > num>>16)
	{
		num = FixedDiv(num, den);
		if (num > 64*FRACUNIT)
			return 64*FRACUNIT;
		if (num < 256)
			return 256;
		return num;
	}
	return 64*FRACUNIT;
}

//
// R_DoCulling
// Checks viewz and top/bottom heights of an item against culling planes
// Returns true if the item is to be culled, i.e it shouldn't be drawn!
// if ML_NOCLIMB is set, the camera view is required to be in the same area for culling to occur
boolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph)
{
	fixed_t cullplane;

	if (!cullheight)
		return false;

	cullplane = cullheight->frontsector->floorheight;
	if (cullheight->flags & ML_NOCLIMB) // Group culling
	{
		if (!viewcullheight)
			return false;

		// Make sure this is part of the same group
		if (viewcullheight->frontsector == cullheight->frontsector)
		{
			// OK, we can cull
			if (vz > cullplane && toph < cullplane) // Cull if below plane
				return true;

			if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
				return true;
		}
	}
	else // Quick culling
	{
		if (vz > cullplane && toph < cullplane) // Cull if below plane
			return true;

		if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
			return true;
	}

	return false;
}

//
// R_InitTextureMapping
//
static void R_InitTextureMapping(void)
{
	INT32 i;
	INT32 x;
	INT32 t;
	fixed_t focallength;

	// Use tangent table to generate viewangletox:
	//  viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv(projection,
		FINETANGENT(FINEANGLES/4+FIELDOFVIEW/2));

	focallengthf = FIXED_TO_FLOAT(focallength);

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > fovtan*2)
			t = -1;
		else if (FINETANGENT(i) < -fovtan*2)
			t = viewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > viewwidth+1)
				t = viewwidth+1;
		}
		viewangletox[i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//  xtoviewangle will give the smallest view angle
	//  that maps to x.
	for (x = 0; x <= viewwidth;x++)
	{
		i = 0;
		while (viewangletox[i] > x)
			i++;
		xtoviewangle[x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (viewangletox[i] == -1)
			viewangletox[i] = 0;
		else if (viewangletox[i] == viewwidth+1)
			viewangletox[i]  = viewwidth;
	}

	clipangle = xtoviewangle[0];
	doubleclipangle = clipangle*2;
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP 2

static inline void R_InitLightTables(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	INT32 scale;

	// Calculate the light levels to use
	//  for each level / distance combination.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTZ; j++)
		{
			//added : 02-02-98 : use BASEVIDWIDTH, vid.width is not set already,
			// and it seems it needs to be calculated only once.
			scale = FixedDiv((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = startmapl - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			zlight[i][j] = colormaps + level*256;
		}
	}
}

//#define WOUGHMP_WOUGHMP // I got a fish-eye lens - I'll make a rap video with a couple of friends
// it's kinda laggy sometimes

static struct {
	angle_t rollangle; // pre-shifted by fineshift
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye;
#endif

	fixed_t zoomneeded;
	INT32 *scrmap;
	INT32 scrmapsize;

	INT32 x1; // clip rendering horizontally for efficiency
	INT16 ceilingclip[MAXVIDWIDTH], floorclip[MAXVIDWIDTH];

	boolean use;
} viewmorph = {
	0,
#ifdef WOUGHMP_WOUGHMP
	0,
#endif

	FRACUNIT,
	NULL,
	0,

	0,
	{}, {},

	false
};

void R_CheckViewMorph(void)
{
	float zoomfactor, rollcos, rollsin;
	float x1, y1, x2, y2;
	fixed_t temp;
	INT32 end, vx, vy, pos, usedpos;
	INT32 usedx, usedy, halfwidth = vid.width/2, halfheight = vid.height/2;
#ifdef WOUGHMP_WOUGHMP
	float fisheyemap[MAXVIDWIDTH/2 + 1];
#endif

	angle_t rollangle = players[displayplayer].viewrollangle;
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye = cv_cam2_turnmultiplier.value; // temporary test value
#endif

	rollangle >>= ANGLETOFINESHIFT;
	rollangle = ((rollangle+2) & ~3) & FINEMASK; // Limit the distinct number of angles to reduce recalcs from angles changing a lot.

#ifdef WOUGHMP_WOUGHMP
	fisheye &= ~0x7FF; // Same
#endif

	if (rollangle == viewmorph.rollangle &&
#ifdef WOUGHMP_WOUGHMP
		fisheye == viewmorph.fisheye &&
#endif
		viewmorph.scrmapsize == vid.width*vid.height)
		return; // No change

	viewmorph.rollangle = rollangle;
#ifdef WOUGHMP_WOUGHMP
	viewmorph.fisheye = fisheye;
#endif

	if (viewmorph.rollangle == 0
#ifdef WOUGHMP_WOUGHMP
		 && viewmorph.fisheye == 0
#endif
	 )
	{
		viewmorph.use = false;
		viewmorph.x1 = 0;
		if (viewmorph.zoomneeded != FRACUNIT)
			R_SetViewSize();
		viewmorph.zoomneeded = FRACUNIT;

		return;
	}

	if (viewmorph.scrmapsize != vid.width*vid.height)
	{
		if (viewmorph.scrmap)
			free(viewmorph.scrmap);
		viewmorph.scrmap = malloc(vid.width*vid.height * sizeof(INT32));
		viewmorph.scrmapsize = vid.width*vid.height;
	}

	temp = FINECOSINE(rollangle);
	rollcos = FIXED_TO_FLOAT(temp);
	temp = FINESINE(rollangle);
	rollsin = FIXED_TO_FLOAT(temp);

	// Calculate maximum zoom needed
	x1 = (vid.width*fabsf(rollcos) + vid.height*fabsf(rollsin)) / vid.width;
	y1 = (vid.height*fabsf(rollcos) + vid.width*fabsf(rollsin)) / vid.height;

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		float f = FIXED_TO_FLOAT(fisheye);
		for (vx = 0; vx <= halfwidth; vx++)
			fisheyemap[vx] = 1.0f / cos(atan(vx * f / halfwidth));

		f = cos(atan(f));
		if (f < 1.0f)
		{
			x1 /= f;
			y1 /= f;
		}
	}
#endif

	temp = max(x1, y1)*FRACUNIT;
	if (temp < FRACUNIT)
		temp = FRACUNIT;
	else
		temp |= 0x3FFF; // Limit how many times the viewport needs to be recalculated

	//CONS_Printf("Setting zoom to %f\n", FIXED_TO_FLOAT(temp));

	if (temp != viewmorph.zoomneeded)
	{
		viewmorph.zoomneeded = temp;
		R_SetViewSize();
	}

	zoomfactor = FIXED_TO_FLOAT(viewmorph.zoomneeded);

	end = vid.width * vid.height - 1;

	pos = 0;

	// Pre-multiply rollcos and rollsin to use for positional stuff
	rollcos /= zoomfactor;
	rollsin /= zoomfactor;

	x1 = -(halfwidth * rollcos - halfheight * rollsin);
	y1 = -(halfheight * rollcos + halfwidth * rollsin);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
		viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)) * fisheyemap[halfwidth]);
	else
#endif
	viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)));
	//CONS_Printf("saving %d cols\n", viewmorph.x1);

	// Set ceilingclip and floorclip
	for (vx = 0; vx < vid.width; vx++)
	{
		viewmorph.ceilingclip[vx] = vid.height;
		viewmorph.floorclip[vx] = -1;
	}
	x2 = x1;
	y2 = y1;
	for (vx = 0; vx < vid.width; vx++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight-1;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 += rollcos;
		y2 += rollsin;
	}
	x2 = x1;
	y2 = y1;
	for (vy = 0; vy < vid.height; vy++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 -= rollsin;
		y2 += rollcos;
	}

	//CONS_Printf("Top left corner is %f %f\n", x1, y1);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		for (vy = 0; vy < halfheight; vy++)
		{
			x2 = x1;
			y2 = y1;
			x1 -= rollsin;
			y1 += rollcos;

			for (vx = 0; vx < vid.width; vx++)
			{
				usedx = halfwidth + x2*fisheyemap[(int) floorf(fabsf(y2*zoomfactor))];
				usedy = halfheight + y2*fisheyemap[(int) floorf(fabsf(x2*zoomfactor))];

				usedpos = usedx + usedy*vid.width;

				viewmorph.scrmap[pos] = usedpos;
				viewmorph.scrmap[end-pos] = end-usedpos;

				x2 += rollcos;
				y2 += rollsin;
				pos++;
			}
		}
	}
	else
	{
#endif
	x1 += halfwidth;
	y1 += halfheight;

	for (vy = 0; vy < halfheight; vy++)
	{
		x2 = x1;
		y2 = y1;
		x1 -= rollsin;
		y1 += rollcos;

		for (vx = 0; vx < vid.width; vx++)
		{
			usedx = x2;
			usedy = y2;

			usedpos = usedx + usedy*vid.width;

			viewmorph.scrmap[pos] = usedpos;
			viewmorph.scrmap[end-pos] = end-usedpos;

			x2 += rollcos;
			y2 += rollsin;
			pos++;
		}
	}
#ifdef WOUGHMP_WOUGHMP
	}
#endif

	viewmorph.use = true;
}

void R_ApplyViewMorph(void)
{
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	INT32 p, end = vid.width * vid.height;

	if (!viewmorph.use)
		return;

	if (cv_debug & DBG_VIEWMORPH)
	{
		UINT8 border = 32;
		UINT8 grid = 160;
		INT32 ws = vid.width / 4;
		INT32 hs = vid.width * (vid.height / 4);

		memcpy(tmpscr, srcscr, vid.width*vid.height);
		for (p = 0; p < vid.width; p++)
		{
			tmpscr[viewmorph.scrmap[p]] = border;
			tmpscr[viewmorph.scrmap[p + hs]] = grid;
			tmpscr[viewmorph.scrmap[p + hs*2]] = grid;
			tmpscr[viewmorph.scrmap[p + hs*3]] = grid;
			tmpscr[viewmorph.scrmap[end - 1 - p]] = border;
		}
		for (p = vid.width; p < end; p += vid.width)
		{
			tmpscr[viewmorph.scrmap[p]] = border;
			tmpscr[viewmorph.scrmap[p + ws]] = grid;
			tmpscr[viewmorph.scrmap[p + ws*2]] = grid;
			tmpscr[viewmorph.scrmap[p + ws*3]] = grid;
			tmpscr[viewmorph.scrmap[end - 1 - p]] = border;
		}
	}
	else
		for (p = 0; p < end; p++)
			tmpscr[p] = srcscr[viewmorph.scrmap[p]];

	VID_BlitLinearScreen(tmpscr, screens[0],
			vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.width);
}


//
// R_SetViewSize
// Do not really change anything here,
// because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
boolean setsizeneeded;

void R_SetViewSize(void)
{
	setsizeneeded = true;
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
	fixed_t dy;
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	angle_t fov;

	setsizeneeded = false;

	if (rendermode == render_none)
		return;

	// status bar overlay
	st_overlay = cv_showhud.value;

	scaledviewwidth = vid.width;
	viewheight = vid.height;

	if (splitscreen)
		viewheight >>= 1;

	viewwidth = scaledviewwidth;

	centerx = viewwidth/2;
	centery = viewheight/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	fov = FixedAngle(cv_fov.value/2) + ANGLE_90;
	fovtan = FixedMul(FINETANGENT(fov >> ANGLETOFINESHIFT), viewmorph.zoomneeded);
	if (splitscreen == 1) // Splitscreen FOV should be adjusted to maintain expected vertical view
		fovtan = 17*fovtan/10;

	projection = projectiony = FixedDiv(centerxfrac, fovtan);

	R_InitViewBuffer(scaledviewwidth, viewheight);

	R_InitTextureMapping();

	// thing clipping
	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (INT16)viewheight;

	// setup sky scaling
	R_SetSkyScale();

	// planes
	if (rendermode == render_soft)
	{
		// this is only used for planes rendering in software mode
		j = viewheight*16;
		for (i = 0; i < j; i++)
		{
			dy = ((i - viewheight*8)<<FRACBITS) + FRACUNIT/2;
			dy = FixedMul(abs(dy), fovtan);
			yslopetab[i] = FixedDiv(centerx*FRACUNIT, dy);
		}
	}

	memset(scalelight, 0xFF, sizeof(scalelight));

	// Calculate the light levels to use for each level/scale combination.
	for (i = 0; i< LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS - 1 - i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmapl - j*vid.width/(viewwidth)/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS - 1;

			scalelight[i][j] = colormaps + level*256;
		}
	}

	// continue to do the software setviewsize as long as we use the reference software view
#ifdef HWRENDER
	if (rendermode != render_soft)
		HWR_SetViewSize();
#endif

	am_recalc = true;
}

//
// R_Init
//

void R_Init(void)
{
	// screensize independent
	//I_OutputMsg("\nR_InitData");
	R_InitData();

	//I_OutputMsg("\nR_InitViewBorder");
	R_InitViewBorder();
	R_SetViewSize(); // setsizeneeded is set true

	//I_OutputMsg("\nR_InitPlanes");
	R_InitPlanes();

	// this is now done by SCR_Recalc() at the first mode set
	//I_OutputMsg("\nR_InitLightTables");
	R_InitLightTables();

	//I_OutputMsg("\nR_InitTranslationTables\n");
	R_InitTranslationTables();

	R_InitDrawNodes();

	framecount = 0;
}

//
// R_PointInSubsector
//
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
	size_t nodenum = numnodes-1;

	while (!(nodenum & NF_SUBSECTOR))
		nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_PointInSubsectorOrNull, same as above but returns 0 if not in subsector
//
subsector_t *R_PointInSubsectorOrNull(fixed_t x, fixed_t y)
{
	node_t *node;
	INT32 side, i;
	size_t nodenum;
	subsector_t *ret;
	seg_t *seg;

	// single subsector is a special case
	if (numnodes == 0)
		return subsectors;

	nodenum = numnodes - 1;

	while (!(nodenum & NF_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = R_PointOnSide(x, y, node);
		nodenum = node->children[side];
	}

	ret = &subsectors[nodenum & ~NF_SUBSECTOR];
	for (i = 0, seg = &segs[ret->firstline]; i < ret->numlines; i++, seg++)
	{
		if (seg->glseg)
			continue;

		//if (R_PointOnSegSide(x, y, seg)) -- breaks in ogl because polyvertex_t cast over vertex pointers
		if (P_PointOnLineSide(x, y, seg->linedef) != seg->side)
			return 0;
	}

	return ret;
}

//
// R_SetupFrame
//

// recalc necessary stuff for mouseaiming
// slopes are already calculated for the full possible view (which is 4*viewheight).
// 18/08/18: (No it's actually 16*viewheight, thanks Jimita for finding this out)
static void R_SetupFreelook(player_t *player, boolean skybox)
{
	INT32 dy = 0;

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

	if (rendermode == render_soft)
	{
		dy = (AIMINGTODY(aimingangle)>>FRACBITS) * viewwidth/BASEVIDWIDTH;
		yslope = &yslopetab[viewheight*8 - (viewheight/2 + dy)];
	}

	centery = (viewheight/2) + dy;
	centeryfrac = centery<<FRACBITS;
}

void R_SetupFrame(player_t *player)
{
	camera_t *thiscam;
	boolean chasecam = false;

	if (splitscreen && player == &players[secondarydisplayplayer]
		&& player != &players[consoleplayer])
	{
		thiscam = &camera2;
		chasecam = (cv_chasecam2.value != 0);
	}
	else
	{
		thiscam = &camera;
		chasecam = (cv_chasecam.value != 0);
	}

	if (player->climbing || (player->powers[pw_carry] == CR_NIGHTSMODE) || player->playerstate == PST_DEAD || gamestate == GS_TITLESCREEN || tutorialmode)
		chasecam = true; // force chasecam on
	else if (player->spectator) // no spectator chasecam
		chasecam = false; // force chasecam off

	if (chasecam && !thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = true;
	}
	else if (!chasecam)
		thiscam->chase = false;

	if (player->awayviewtics)
	{
		// cut-away view stuff
		r_viewmobj = player->awayviewmobj; // should be a MT_ALTVIEWMAN
		I_Assert(r_viewmobj != NULL);
		viewz = r_viewmobj->z + 20*FRACUNIT;
		aimingangle = player->awayviewaiming;
		viewangle = r_viewmobj->angle;
	}
	else if (!player->spectator && chasecam)
	// use outside cam view
	{
		r_viewmobj = NULL;
		viewz = thiscam->z + (thiscam->height>>1);
		aimingangle = thiscam->aiming;
		viewangle = thiscam->angle;
	}
	else
	// use the player's eyes view
	{
		viewz = player->viewz;

		r_viewmobj = player->mo;
		I_Assert(r_viewmobj != NULL);

		aimingangle = player->aiming;
		viewangle = r_viewmobj->angle;

		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				viewangle = localangle; // WARNING: camera uses this
				aimingangle = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				viewangle = localangle2;
				aimingangle = localaiming2;
			}
		}
	}
	viewz += quake.z;

	viewplayer = player;

	if (chasecam && !player->awayviewtics && !player->spectator)
	{
		viewx = thiscam->x;
		viewy = thiscam->y;
		viewx += quake.x;
		viewy += quake.y;

		if (thiscam->subsector)
			viewsector = thiscam->subsector->sector;
		else
			viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}
	else
	{
		viewx = r_viewmobj->x;
		viewy = r_viewmobj->y;
		viewx += quake.x;
		viewy += quake.y;

		if (r_viewmobj->subsector)
			viewsector = r_viewmobj->subsector->sector;
		else
			viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	R_SetupFreelook(player, false);
}

void R_SkyboxFrame(player_t *player)
{
	camera_t *thiscam;

	if (splitscreen && player == &players[secondarydisplayplayer]
	&& player != &players[consoleplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	// cut-away view stuff
	r_viewmobj = skyboxmo[0];
#ifdef PARANOIA
	if (!r_viewmobj)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("R_SkyboxFrame: r_viewmobj null (player %s)", sizeu1(playeri));
	}
#endif
	if (player->awayviewtics)
	{
		aimingangle = player->awayviewaiming;
		viewangle = player->awayviewmobj->angle;
	}
	else if (thiscam->chase)
	{
		aimingangle = thiscam->aiming;
		viewangle = thiscam->angle;
	}
	else
	{
		aimingangle = player->aiming;
		viewangle = player->mo->angle;
		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				viewangle = localangle; // WARNING: camera uses this
				aimingangle = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				viewangle = localangle2;
				aimingangle = localaiming2;
			}
		}
	}
	viewangle += r_viewmobj->angle;

	viewplayer = player;

	viewx = r_viewmobj->x;
	viewy = r_viewmobj->y;
	viewz = r_viewmobj->z; // 26/04/17: use actual Z position instead of spawnpoint angle!

	if (mapheaderinfo[gamemap-1])
	{
		mapheader_t *mh = mapheaderinfo[gamemap-1];
		vector3_t campos = {0,0,0}; // Position of player's actual view point

		if (player->awayviewtics) {
			campos.x = player->awayviewmobj->x;
			campos.y = player->awayviewmobj->y;
			campos.z = player->awayviewmobj->z + 20*FRACUNIT;
		} else if (thiscam->chase) {
			campos.x = thiscam->x;
			campos.y = thiscam->y;
			campos.z = thiscam->z + (thiscam->height>>1);
		} else {
			campos.x = player->mo->x;
			campos.y = player->mo->y;
			campos.z = player->viewz;
		}

		// Earthquake effects should be scaled in the skybox
		// (if an axis isn't used, the skybox won't shake in that direction)
		campos.x += quake.x;
		campos.y += quake.y;
		campos.z += quake.z;

		if (skyboxmo[1]) // Is there a viewpoint?
		{
			fixed_t x = 0, y = 0;
			if (mh->skybox_scalex > 0)
				x = (campos.x - skyboxmo[1]->x) / mh->skybox_scalex;
			else if (mh->skybox_scalex < 0)
				x = (campos.x - skyboxmo[1]->x) * -mh->skybox_scalex;

			if (mh->skybox_scaley > 0)
				y = (campos.y - skyboxmo[1]->y) / mh->skybox_scaley;
			else if (mh->skybox_scaley < 0)
				y = (campos.y - skyboxmo[1]->y) * -mh->skybox_scaley;

			if (r_viewmobj->angle == 0)
			{
				viewx += x;
				viewy += y;
			}
			else if (r_viewmobj->angle == ANGLE_90)
			{
				viewx -= y;
				viewy += x;
			}
			else if (r_viewmobj->angle == ANGLE_180)
			{
				viewx -= x;
				viewy -= y;
			}
			else if (r_viewmobj->angle == ANGLE_270)
			{
				viewx += y;
				viewy -= x;
			}
			else
			{
				angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
				viewx += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
				viewy += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
			}
		}
		if (mh->skybox_scalez > 0)
			viewz += campos.z / mh->skybox_scalez;
		else if (mh->skybox_scalez < 0)
			viewz += campos.z * -mh->skybox_scalez;
	}

	if (r_viewmobj->subsector)
		viewsector = r_viewmobj->subsector->sector;
	else
		viewsector = R_PointInSubsector(viewx, viewy)->sector;

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	R_SetupFreelook(player, true);
}

boolean R_ViewpointHasChasecam(player_t *player)
{
	boolean chasecam = false;

	if (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer])
		chasecam = (cv_chasecam2.value != 0);
	else
		chasecam = (cv_chasecam.value != 0);

	if (player->climbing || (player->powers[pw_carry] == CR_NIGHTSMODE) || player->playerstate == PST_DEAD || gamestate == GS_TITLESCREEN || tutorialmode)
		chasecam = true; // force chasecam on
	else if (player->spectator) // no spectator chasecam
		chasecam = false; // force chasecam off

	return chasecam;
}

boolean R_IsViewpointThirdPerson(player_t *player, boolean skybox)
{
	boolean chasecam = R_ViewpointHasChasecam(player);

	// cut-away view stuff
	if (player->awayviewtics || skybox)
		return chasecam;
	// use outside cam view
	else if (!player->spectator && chasecam)
		return true;

	// use the player's eyes view
	return false;
}

static void R_PortalFrame(portal_t *portal)
{
	viewx = portal->viewx;
	viewy = portal->viewy;
	viewz = portal->viewz;

	viewangle = portal->viewangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	portalclipstart = portal->start;
	portalclipend = portal->end;

	if (portal->clipline != -1)
	{
		portalclipline = &lines[portal->clipline];
		portalcullsector = portalclipline->frontsector;
		viewsector = portalclipline->frontsector;
	}
	else
	{
		portalclipline = NULL;
		portalcullsector = NULL;
		viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}
}

static void Mask_Pre (maskcount_t* m)
{
	m->drawsegs[0] = ds_p - drawsegs;
	m->vissprites[0] = visspritecount;
	m->viewx = viewx;
	m->viewy = viewy;
	m->viewz = viewz;
	m->viewsector = viewsector;
}

static void Mask_Post (maskcount_t* m)
{
	m->drawsegs[1] = ds_p - drawsegs;
	m->vissprites[1] = visspritecount;
}

// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void R_RenderPlayerView(player_t *player)
{
	UINT8			nummasks	= 1;
	maskcount_t*	masks		= malloc(sizeof(maskcount_t));

	if (cv_homremoval.value && player == &players[displayplayer]) // if this is display player 1
	{
		if (cv_homremoval.value == 1)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31); // No HOM effect!
		else //'development' HOM removal -- makes it blindingly obvious if HOM is spotted.
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 32+(timeinmap&15));
	}

	R_SetupFrame(player);
	framecount++;
	validcount++;

	// Clear buffers.
	R_ClearPlanes();
	if (viewmorph.use)
	{
		portalclipstart = viewmorph.x1;
		portalclipend = viewwidth-viewmorph.x1-1;
		R_PortalClearClipSegs(portalclipstart, portalclipend);
		memcpy(ceilingclip, viewmorph.ceilingclip, sizeof(INT16)*vid.width);
		memcpy(floorclip, viewmorph.floorclip, sizeof(INT16)*vid.width);
	}
	else
	{
		portalclipstart = 0;
		portalclipend = viewwidth;
		R_ClearClipSegs();
	}
	R_ClearDrawSegs();
	R_ClearSprites();
#ifdef FLOORSPLATS
	R_ClearVisibleFloorSplats();
#endif
	Portal_InitList();

	// check for new console commands.
	NetUpdate();

	// The head node is the last node output.

	Mask_Pre(&masks[nummasks - 1]);
	curdrawsegs = ds_p;
//profile stuff ---------------------------------------------------------
#ifdef TIMING
	mytotal = 0;
	ProfZeroTimer();
#endif
	rs_numbspcalls = rs_numpolyobjects = rs_numdrawnodes = 0;
	rs_bsptime = I_GetTimeMicros();
	R_RenderBSPNode((INT32)numnodes - 1);
	rs_bsptime = I_GetTimeMicros() - rs_bsptime;
	rs_numsprites = visspritecount;
#ifdef TIMING
	RDMSR(0x10, &mycount);
	mytotal += mycount; // 64bit add

	CONS_Debug(DBG_RENDER, "RenderBSPNode: 0x%d %d\n", *((INT32 *)&mytotal + 1), (INT32)mytotal);
#endif
//profile stuff ---------------------------------------------------------
	Mask_Post(&masks[nummasks - 1]);

	R_ClipSprites(drawsegs, NULL);


	// Add skybox portals caused by sky visplanes.
	if (cv_skybox.value && skyboxmo[0])
		Portal_AddSkyboxPortals();

	// Portal rendering. Hijacks the BSP traversal.
	rs_sw_portaltime = I_GetTimeMicros();
	if (portal_base)
	{
		portal_t *portal;

		for(portal = portal_base; portal; portal = portal_base)
		{
			portalrender = portal->pass; // Recursiveness depth.

			R_ClearFFloorClips();

			// Apply the viewpoint stored for the portal.
			R_PortalFrame(portal);

			// Hack in the clipsegs to delimit the starting
			// clipping for sprites and possibly other similar
			// future items.
			R_PortalClearClipSegs(portal->start, portal->end);

			// Hack in the top/bottom clip values for the window
			// that were previously stored.
			Portal_ClipApply(portal);

			validcount++;

			masks = realloc(masks, (++nummasks)*sizeof(maskcount_t));

			Mask_Pre(&masks[nummasks - 1]);
			curdrawsegs = ds_p;

			// Render the BSP from the new viewpoint, and clip
			// any sprites with the new clipsegs and window.
			R_RenderBSPNode((INT32)numnodes - 1);
			Mask_Post(&masks[nummasks - 1]);

			R_ClipSprites(ds_p - (masks[nummasks - 1].drawsegs[1] - masks[nummasks - 1].drawsegs[0]), portal);

			Portal_Remove(portal);
		}
	}
	rs_sw_portaltime = I_GetTimeMicros() - rs_sw_portaltime;

	rs_sw_planetime = I_GetTimeMicros();
	R_DrawPlanes();
#ifdef FLOORSPLATS
	R_DrawVisibleFloorSplats();
#endif
	rs_sw_planetime = I_GetTimeMicros() - rs_sw_planetime;

	// draw mid texture and sprite
	// And now 3D floors/sides!
	rs_sw_maskedtime = I_GetTimeMicros();
	R_DrawMasked(masks, nummasks);
	rs_sw_maskedtime = I_GetTimeMicros() - rs_sw_maskedtime;

	free(masks);
}

#ifdef HWRENDER
void R_InitHardwareMode(void)
{
	HWR_AddSessionCommands();
	HWR_Switch();
	HWR_LoadTextures(numtextures);
	if (gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction))
		HWR_SetupLevel();
}
#endif

void R_ReloadHUDGraphics(void)
{
	ST_LoadGraphics();
	HU_LoadGraphics();
	ST_ReloadSkinFaceGraphics();
}

// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

void R_RegisterEngineStuff(void)
{
	CV_RegisterVar(&cv_gravity);
	CV_RegisterVar(&cv_tailspickup);
	CV_RegisterVar(&cv_allowmlook);
	CV_RegisterVar(&cv_homremoval);
	CV_RegisterVar(&cv_flipcam);
	CV_RegisterVar(&cv_flipcam2);

	// Enough for dedicated server
	if (dedicated)
		return;

	CV_RegisterVar(&cv_translucency);
	CV_RegisterVar(&cv_drawdist);
	CV_RegisterVar(&cv_drawdist_nights);
	CV_RegisterVar(&cv_drawdist_precip);
	CV_RegisterVar(&cv_fov);

	CV_RegisterVar(&cv_chasecam);
	CV_RegisterVar(&cv_chasecam2);

	CV_RegisterVar(&cv_shadow);
	CV_RegisterVar(&cv_skybox);

	CV_RegisterVar(&cv_cam_dist);
	CV_RegisterVar(&cv_cam_still);
	CV_RegisterVar(&cv_cam_height);
	CV_RegisterVar(&cv_cam_speed);
	CV_RegisterVar(&cv_cam_rotate);
	CV_RegisterVar(&cv_cam_rotspeed);
	CV_RegisterVar(&cv_cam_turnmultiplier);
	CV_RegisterVar(&cv_cam_orbit);
	CV_RegisterVar(&cv_cam_adjust);

	CV_RegisterVar(&cv_cam2_dist);
	CV_RegisterVar(&cv_cam2_still);
	CV_RegisterVar(&cv_cam2_height);
	CV_RegisterVar(&cv_cam2_speed);
	CV_RegisterVar(&cv_cam2_rotate);
	CV_RegisterVar(&cv_cam2_rotspeed);
	CV_RegisterVar(&cv_cam2_turnmultiplier);
	CV_RegisterVar(&cv_cam2_orbit);
	CV_RegisterVar(&cv_cam2_adjust);

	CV_RegisterVar(&cv_cam_savedist[0][0]);
	CV_RegisterVar(&cv_cam_savedist[0][1]);
	CV_RegisterVar(&cv_cam_savedist[1][0]);
	CV_RegisterVar(&cv_cam_savedist[1][1]);

	CV_RegisterVar(&cv_cam_saveheight[0][0]);
	CV_RegisterVar(&cv_cam_saveheight[0][1]);
	CV_RegisterVar(&cv_cam_saveheight[1][0]);
	CV_RegisterVar(&cv_cam_saveheight[1][1]);

	CV_RegisterVar(&cv_showhud);
	CV_RegisterVar(&cv_translucenthud);

	CV_RegisterVar(&cv_maxportals);

	CV_RegisterVar(&cv_movebob);
}
