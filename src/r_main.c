// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
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
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "am_map.h"
#include "d_main.h"
#include "v_video.h"
#include "p_spec.h" // skyboxmo
#include "z_zone.h"
#include "m_random.h" // quake camera shake

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

// just for profiling purposes
size_t framecount;

size_t sscount;
size_t loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle;
fixed_t viewcos, viewsin;
boolean viewsky, skyVisible;
sector_t *viewsector;
player_t *viewplayer;

// PORTALS!
// You can thank and/or curse JTE for these.
UINT8 portalrender;
typedef struct portal_pair
{
	INT32 line1;
	INT32 line2;
	UINT8 pass;
	struct portal_pair *next;
} portal_pair;
portal_pair *portal_base, *portal_cap;

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

// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from INT32_MIN to 0 to INT32_MAX.

#if !(defined _NDS) || !(defined NONET)
fixed_t *finecosine = &finesine[FINEANGLES/4];
#endif

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// Hack to support extra boom colormaps.
size_t num_extra_colormaps;
extracolormap_t extra_colormaps[MAXCOLORMAPS];

static CV_PossibleValue_t drawdist_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{3072, "3072"},	{4096, "4096"},	{6144, "6144"},
	{8192, "8192"},	{0, "Infinite"},	{0, NULL}};
static CV_PossibleValue_t precipdensity_cons_t[] = {{0, "None"}, {1, "Light"}, {2, "Moderate"}, {4, "Heavy"}, {6, "Thick"}, {8, "V.Thick"}, {0, NULL}};
static CV_PossibleValue_t translucenthud_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
static CV_PossibleValue_t homremoval_cons_t[] = {{0, "No"}, {1, "Yes"}, {2, "Flash"}, {0, NULL}};

static void ChaseCam_OnChange(void);
static void ChaseCam2_OnChange(void);
void SendWeaponPref(void);

consvar_t cv_tailspickup = {"tailspickup", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasecam = {"chasecam", "On", CV_CALL, CV_OnOff, ChaseCam_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasecam2 = {"chasecam2", "On", CV_CALL, CV_OnOff, ChaseCam2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_flipcam = {"flipcam", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, SendWeaponPref, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_flipcam2 = {"flipcam2", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, SendWeaponPref, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_shadow = {"shadow", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_shadowoffs = {"offsetshadows", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_skybox = {"skybox", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_soniccd = {"soniccd", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowmlook = {"allowmlook", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_showhud = {"showhud", "Yes", CV_CALL,  CV_YesNo, R_SetViewSize, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_translucenthud = {"translucenthud", "10", CV_SAVE, translucenthud_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_drawdist = {"drawdist", "Infinite", CV_SAVE, drawdist_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_drawdist_nights = {"drawdist_nights", "2048", CV_SAVE, drawdist_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_drawdist_precip = {"drawdist_precip", "1024", CV_SAVE, drawdist_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_precipdensity = {"precipdensity", "Moderate", CV_SAVE, precipdensity_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// Okay, whoever said homremoval causes a performance hit should be shot.
consvar_t cv_homremoval = {"homremoval", "No", CV_SAVE, homremoval_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

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

static void ChaseCam_OnChange(void)
{
	if (!cv_chasecam.value || !cv_useranalog.value)
		CV_SetValue(&cv_analog, 0);
	else
		CV_SetValue(&cv_analog, 1);
}

static void ChaseCam2_OnChange(void)
{
	if (botingame)
		return;
	if (!cv_chasecam2.value || !cv_useranalog2.value)
		CV_SetValue(&cv_analog2, 0);
	else
		CV_SetValue(&cv_analog2, 1);
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
		ANGLE_90-1-tantoangle[SlopeDiv(x,y)] :                         // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-1-tantoangle[SlopeDiv(y,x)] :// octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[ SlopeDiv(y,x)] :   // octant 4
		ANGLE_270-1-tantoangle[SlopeDiv(x,y)] :                        // octant 5
		0;
}

angle_t R_PointToAngle2(fixed_t pviewx, fixed_t pviewy, fixed_t x, fixed_t y)
{
	return (y -= pviewy, (x -= pviewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-1-tantoangle[SlopeDiv(x,y)] :                         // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-1-tantoangle[SlopeDiv(y,x)] :// octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[ SlopeDiv(y,x)] :   // octant 4
		ANGLE_270-1-tantoangle[SlopeDiv(x,y)] :                        // octant 5
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

/***************************************
*** Zdoom C++ to Legacy C conversion ***
****************************************/

// Utility to find the Z height at an XY location in a sector (for slopes)
fixed_t R_SecplaneZatPoint(secplane_t *secplane, fixed_t x, fixed_t y)
{
	return FixedMul(secplane->ic, -secplane->d - DMulScale16(secplane->a, x, secplane->b, y));
}

// Returns the value of z at (x,y) if d is equal to dist
fixed_t R_SecplaneZatPointDist (secplane_t *secplane, fixed_t x, fixed_t y, fixed_t dist)
{
	return FixedMul(secplane->ic, -dist - DMulScale16(secplane->a, x, secplane->b, y));
}

// Flips the plane's vertical orientiation, so that if it pointed up,
// it will point down, and vice versa.
void R_SecplaneFlipVert(secplane_t *secplane)
{
	secplane->a = -secplane->a;
	secplane->b = -secplane->b;
	secplane->c = -secplane->c;
	secplane->d = -secplane->d;
	secplane->ic = -secplane->ic;
}

// Returns true if 2 planes are the same
boolean R_ArePlanesSame(secplane_t *original, secplane_t *other)
{
	return original->a == other->a && original->b == other->b
		&& original->c == other->c && original->d == other->d;
}

// Returns true if 2 planes are different
boolean R_ArePlanesDifferent(secplane_t *original, secplane_t *other)
{
	return original->a != other->a || original->b != other->b
		|| original->c != other->c || original->d != other->d;
}

// Moves a plane up/down by hdiff units
void R_SecplaneChangeHeight(secplane_t *secplane, fixed_t hdiff)
{
	secplane->d = secplane->d - FixedMul(hdiff, secplane->c);
}

// Returns how much this plane's height would change if d were set to oldd
fixed_t R_SecplaneHeightDiff(secplane_t *secplane, fixed_t oldd)
{
	return FixedMul(oldd - secplane->d, secplane->ic);
}

fixed_t R_SecplanePointToDist(secplane_t *secplane, fixed_t x, fixed_t y, fixed_t z)
{
	return -TMulScale16(secplane->a, x, y, secplane->b, z, secplane->c);
}

fixed_t R_SecplanePointToDist2(secplane_t *secplane, fixed_t x, fixed_t y, fixed_t z)
{
	return -TMulScale16(secplane->a, x, secplane->b, y, z, secplane->c);
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
	focallength = FixedDiv(centerxfrac,
		FINETANGENT(FINEANGLES/4+/*cv_fov.value*/ FIELDOFVIEW/2));

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > FRACUNIT*2)
			t = -1;
		else if (FINETANGENT(i) < -FRACUNIT*2)
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
		t = FixedMul(FINETANGENT(i), focallength);
		t = centerx - t;

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
	fixed_t cosadj;
	fixed_t dy;
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	INT32 aspectx;  //added : 02-02-98 : for aspect ratio calc. below...

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

	projection = centerxfrac;
	//projectiony = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width)<<FRACBITS;
	projectiony = centerxfrac;

	R_InitViewBuffer(scaledviewwidth, viewheight);

	R_InitTextureMapping();

#ifdef HWRENDER
	if (rendermode != render_soft)
		HWR_InitTextureMapping();
#endif

	// thing clipping
	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (INT16)viewheight;

	// setup sky scaling (uses pspriteyscale)
	R_SetSkyScale();

	// planes
	//aspectx = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width);
	aspectx = centerx;

	if (rendermode == render_soft)
	{
		// this is only used for planes rendering in software mode
		j = viewheight*4;
		for (i = 0; i < j; i++)
		{
			dy = ((i - viewheight*2)<<FRACBITS) + FRACUNIT/2;
			dy = abs(dy);
			yslopetab[i] = FixedDiv(aspectx*FRACUNIT, dy);
		}
	}

	for (i = 0; i < viewwidth; i++)
	{
		cosadj = abs(FINECOSINE(xtoviewangle[i]>>ANGLETOFINESHIFT));
		distscale[i] = FixedDiv(FRACUNIT, cosadj);
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
// R_IsPointInSubsector, same as above but returns 0 if not in subsector
//
subsector_t *R_IsPointInSubsector(fixed_t x, fixed_t y)
{
	node_t *node;
	INT32 side, i;
	size_t nodenum;
	subsector_t *ret;

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
	for (i = 0; i < ret->numlines; i++)
		if (R_PointOnSegSide(x, y, &segs[ret->firstline + i]))
			return 0;

	return ret;
}

//
// R_SetupFrame
//

static mobj_t *viewmobj;

// WARNING: a should be unsigned but to add with 2048, it isn't!
#define AIMINGTODY(a) ((FINETANGENT((2048+(((INT32)a)>>ANGLETOFINESHIFT)) & FINEMASK)*160)>>FRACBITS)

void R_SkyboxFrame(player_t *player)
{
	INT32 dy = 0;
	camera_t *thiscam;

	if (splitscreen && player == &players[secondarydisplayplayer]
	&& player != &players[consoleplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	// cut-away view stuff
	viewsky = true;
	viewmobj = skyboxmo[0];
#ifdef PARANOIA
	if (!viewmobj)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("R_SkyboxFrame: viewmobj null (player %s)", sizeu1(playeri));
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
	viewangle += viewmobj->angle;

	viewplayer = player;

	viewx = viewmobj->x;
	viewy = viewmobj->y;
	viewz = 0;
	if (viewmobj->spawnpoint)
		viewz = ((fixed_t)viewmobj->spawnpoint->angle)<<FRACBITS;

	viewx += quake.x;
	viewy += quake.y;
	viewz += quake.z;

	if (mapheaderinfo[gamemap-1])
	{
		mapheader_t *mh = mapheaderinfo[gamemap-1];
		if (player->awayviewtics)
		{
			if (skyboxmo[1])
			{
				fixed_t x = 0, y = 0;
				if (mh->skybox_scalex > 0)
					x = (player->awayviewmobj->x - skyboxmo[1]->x) / mh->skybox_scalex;
				else if (mh->skybox_scalex < 0)
					x = (player->awayviewmobj->x - skyboxmo[1]->x) * -mh->skybox_scalex;

				if (mh->skybox_scaley > 0)
					y = (player->awayviewmobj->y - skyboxmo[1]->y) / mh->skybox_scaley;
				else if (mh->skybox_scaley < 0)
					y = (player->awayviewmobj->y - skyboxmo[1]->y) * -mh->skybox_scaley;

				if (viewmobj->angle == 0)
				{
					viewx += x;
					viewy += y;
				}
				else if (viewmobj->angle == ANGLE_90)
				{
					viewx -= y;
					viewy += x;
				}
				else if (viewmobj->angle == ANGLE_180)
				{
					viewx -= x;
					viewy -= y;
				}
				else if (viewmobj->angle == ANGLE_270)
				{
					viewx += y;
					viewy -= x;
				}
				else
				{
					angle_t ang = viewmobj->angle>>ANGLETOFINESHIFT;
					viewx += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
					viewy += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
				}
			}
			if (mh->skybox_scalez > 0)
				viewz += player->awayviewmobj->z / mh->skybox_scalez;
			else if (mh->skybox_scalez < 0)
				viewz += player->awayviewmobj->z * -mh->skybox_scalez;
		}
		else if (thiscam->chase)
		{
			if (skyboxmo[1])
			{
				fixed_t x = 0, y = 0;
				if (mh->skybox_scalex > 0)
					x = (thiscam->x - skyboxmo[1]->x) / mh->skybox_scalex;
				else if (mh->skybox_scalex < 0)
					x = (thiscam->x - skyboxmo[1]->x) * -mh->skybox_scalex;

				if (mh->skybox_scaley > 0)
					y = (thiscam->y - skyboxmo[1]->y) / mh->skybox_scaley;
				else if (mh->skybox_scaley < 0)
					y = (thiscam->y - skyboxmo[1]->y) * -mh->skybox_scaley;

				if (viewmobj->angle == 0)
				{
					viewx += x;
					viewy += y;
				}
				else if (viewmobj->angle == ANGLE_90)
				{
					viewx -= y;
					viewy += x;
				}
				else if (viewmobj->angle == ANGLE_180)
				{
					viewx -= x;
					viewy -= y;
				}
				else if (viewmobj->angle == ANGLE_270)
				{
					viewx += y;
					viewy -= x;
				}
				else
				{
					angle_t ang = viewmobj->angle>>ANGLETOFINESHIFT;
					viewx += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
					viewy += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
				}
			}
			if (mh->skybox_scalez > 0)
				viewz += thiscam->z / mh->skybox_scalez;
			else if (mh->skybox_scalez < 0)
				viewz += thiscam->z * -mh->skybox_scalez;
		}
		else
		{
			if (skyboxmo[1])
			{
				if (mh->skybox_scalex > 0)
					viewx += (player->mo->x - skyboxmo[1]->x) / mh->skybox_scalex;
				else if (mh->skybox_scalex < 0)
					viewx += (player->mo->x - skyboxmo[1]->x) * -mh->skybox_scalex;
				if (mh->skybox_scaley > 0)
					viewy += (player->mo->y - skyboxmo[1]->y) / mh->skybox_scaley;
				else if (mh->skybox_scaley < 0)
					viewy += (player->mo->y - skyboxmo[1]->y) * -mh->skybox_scaley;
			}
			if (mh->skybox_scalez > 0)
				viewz += player->viewz / mh->skybox_scalez;
			else if (mh->skybox_scalez < 0)
				viewz += player->viewz * -mh->skybox_scalez;
		}
	}

	if (viewmobj->subsector)
		viewsector = viewmobj->subsector->sector;
	else
		viewsector = R_PointInSubsector(viewx, viewy)->sector;

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	sscount = 0;

	// recalc necessary stuff for mouseaiming
	// slopes are already calculated for the full possible view (which is 4*viewheight).

	if (rendermode == render_soft)
	{
		// clip it in the case we are looking a hardware 90 degrees full aiming
		// (lmps, network and use F12...)
		G_SoftwareClipAimingPitch((INT32 *)&aimingangle);

		dy = AIMINGTODY(aimingangle) * viewwidth/BASEVIDWIDTH;

		yslope = &yslopetab[(3*viewheight/2) - dy];
	}
	centery = (viewheight/2) + dy;
	centeryfrac = centery<<FRACBITS;
}

void R_SetupFrame(player_t *player, boolean skybox)
{
	INT32 dy = 0;
	camera_t *thiscam;
	boolean forcechase = false;

	if (splitscreen && player == &players[secondarydisplayplayer]
		&& player != &players[consoleplayer])
	{
		thiscam = &camera2;
	}
	else
		thiscam = &camera;

	if (player->climbing || (player->pflags & PF_NIGHTSMODE) || player->playerstate == PST_DEAD)
		forcechase = true;

	if (!forcechase && player->spectator) // no spectator chasecam
		thiscam->chase = false;
	else if ((cv_chasecam.value || forcechase) && !player->spectator && thiscam == &camera && !thiscam->chase)
	{
		P_ResetCamera(player, &camera);
		thiscam->chase = true;
	}
	else if ((cv_chasecam2.value || forcechase) && !player->spectator && thiscam == &camera2 && !thiscam->chase)
	{
		P_ResetCamera(player, &camera2);
		thiscam->chase = true;
	}
	else if (!(cv_chasecam.value || forcechase) && thiscam == &camera)
		thiscam->chase = false;
	else if (!(cv_chasecam2.value || forcechase) && thiscam == &camera2)
		thiscam->chase = false;

	viewsky = !skybox;
	if (player->awayviewtics)
	{
		// cut-away view stuff
		viewmobj = player->awayviewmobj; // should be a MT_ALTVIEWMAN
		I_Assert(viewmobj != NULL);
		viewz = viewmobj->z + 20*FRACUNIT;
		aimingangle = player->awayviewaiming;
		viewangle = viewmobj->angle;
	}
	else if (!player->spectator && (forcechase
		|| (cv_chasecam.value && thiscam == &camera)
		|| (cv_chasecam2.value && thiscam == &camera2)))
	// use outside cam view
	{
		viewmobj = NULL;
		viewz = thiscam->z + (thiscam->height>>1);
		aimingangle = thiscam->aiming;
		viewangle = thiscam->angle;
	}
	else
	// use the player's eyes view
	{
		viewz = player->viewz;

		viewmobj = player->mo;
		I_Assert(viewmobj != NULL);

		aimingangle = player->aiming;
		viewangle = viewmobj->angle;

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

	if ((forcechase || (cv_chasecam.value && thiscam == &camera) || (cv_chasecam2.value && thiscam == &camera2))
		&& !player->awayviewtics && !player->spectator)
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
		viewx = viewmobj->x;
		viewy = viewmobj->y;
		viewx += quake.x;
		viewy += quake.y;

		if (viewmobj->subsector)
			viewsector = viewmobj->subsector->sector;
		else
			viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	sscount = 0;

	// recalc necessary stuff for mouseaiming
	// slopes are already calculated for the full possible view (which is 4*viewheight).

	if (rendermode == render_soft)
	{
		// clip it in the case we are looking a hardware 90 degrees full aiming
		// (lmps, network and use F12...)
		G_SoftwareClipAimingPitch((INT32 *)&aimingangle);

		dy = AIMINGTODY(aimingangle) * viewwidth/BASEVIDWIDTH;

		yslope = &yslopetab[(3*viewheight/2) - dy];
	}
	centery = (viewheight/2) + dy;
	centeryfrac = centery<<FRACBITS;
}

static void R_PortalFrame(player_t *player, line_t *start, line_t *dest)
{
	vertex_t dest_c, start_c;
#ifdef ANGLED_PORTALS
	// delta angle
	angle_t dangle = R_PointToAngle2(0,0,dest->dx,dest->dy) - R_PointToAngle2(start->dx,start->dy,0,0);
#endif

	R_SetupFrame(player, false);

	// Offset the portal view by the linedef centers

	// looking glass center
	start_c.x = (start->v1->x + start->v2->x) / 2;
	start_c.y = (start->v1->y + start->v2->y) / 2;

	// other side center
	dest_c.x = (dest->v1->x + dest->v2->x) / 2;
	dest_c.y = (dest->v1->y + dest->v2->y) / 2;

	// calculate the difference in position and rotation!
#ifdef ANGLED_PORTALS
	if (dangle == 0)
#endif
	{ // the entrance goes straight opposite the exit, so we just need to mess with the offset.
		viewx += dest_c.x - start_c.x;
		viewy += dest_c.y - start_c.y;
		return;
	}

#ifdef ANGLED_PORTALS
	viewangle += dangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);
	CONS_Printf("dangle == %u\n", AngleFixed(dangle)>>FRACBITS);

	// ????
	viewx = dest_c.x - viewx;
	viewy = dest_c.y - viewy;
#endif
}

void R_AddPortal(INT32 line1, INT32 line2)
{
	portal_pair *portal = Z_Malloc(sizeof(portal_pair), PU_LEVEL, NULL);
	portal->line1 = line1;
	portal->line2 = line2;
	portal->pass = portalrender+1;
	portal->next = NULL;

	if (!portal_base)
	{
		portal_base = portal;
		portal_cap = portal;
	}
	else
	{
		portal_cap->next = portal;
		portal_cap = portal;
	}
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
	portal_pair *portal;
	const boolean skybox = (skyboxmo[0] && cv_skybox.value);

	if (cv_homremoval.value && player == &players[displayplayer]) // if this is display player 1
	{
		if (cv_homremoval.value == 1)
			V_DrawFill(0, 0, vid.width, vid.height, 31); // No HOM effect!
		else //'development' HOM removal -- makes it blindingly obvious if HOM is spotted.
			V_DrawFill(0, 0, vid.width, vid.height, 128+(timeinmap&15));
	}

	portalrender = 0;
	portal_base = portal_cap = NULL;

	if (skybox && skyVisible)
	{
		R_SkyboxFrame(player);

		R_ClearClipSegs();
		R_ClearDrawSegs();
		R_ClearPlanes();
		R_ClearSprites();
#ifdef FLOORSPLATS
		R_ClearVisibleFloorSplats();
#endif

		R_RenderBSPNode((INT32)numnodes - 1);
		R_DrawPlanes();
#ifdef FLOORSPLATS
		R_DrawVisibleFloorSplats();
#endif
		R_DrawMasked();
	}

	R_SetupFrame(player, skybox);
	skyVisible = false;
	framecount++;
	validcount++;

	// Clear buffers.
	R_ClearClipSegs();
	R_ClearDrawSegs();
	R_ClearPlanes();
	R_ClearSprites();
#ifdef FLOORSPLATS
	R_ClearVisibleFloorSplats();
#endif

	// check for new console commands.
	NetUpdate();

	// The head node is the last node output.

//profile stuff ---------------------------------------------------------
#ifdef TIMING
	mytotal = 0;
	ProfZeroTimer();
#endif
	R_RenderBSPNode((INT32)numnodes - 1);
#ifdef TIMING
	RDMSR(0x10, &mycount);
	mytotal += mycount; // 64bit add

	CONS_Debug(DBG_RENDER, "RenderBSPNode: 0x%d %d\n", *((INT32 *)&mytotal + 1), (INT32)mytotal);
#endif
//profile stuff ---------------------------------------------------------

	R_DrawPlanes();
#ifdef FLOORSPLATS
	R_DrawVisibleFloorSplats();
#endif
	// draw mid texture and sprite
	// And now 3D floors/sides!
	R_DrawMasked();

	// PORTAL RENDERING
	for(portal = portal_base; portal; portal = portal_base)
	{
		// render the portal
		CONS_Debug(DBG_RENDER, "Rendering portal from line %d to %d\n", portal->line1, portal->line2);
		portalrender = portal->pass;
		R_PortalFrame(player, &lines[portal->line1], &lines[portal->line2]);
		validcount++;
		R_RenderBSPNode((INT32)numnodes - 1);
		R_DrawPlanes();
		R_DrawMasked();

		// okay done. free it.
		portal_base = portal->next;
		Z_Free(portal);
	}
	// END PORTAL RENDERING

	// Check for new console commands.
	NetUpdate();
}

// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

void R_RegisterEngineStuff(void)
{
	CV_RegisterVar(&cv_gravity);
	CV_RegisterVar(&cv_tailspickup);
	CV_RegisterVar(&cv_soniccd);
	CV_RegisterVar(&cv_allowmlook);
	CV_RegisterVar(&cv_homremoval);
	CV_RegisterVar(&cv_flipcam);

	// Enough for dedicated server
	if (dedicated)
		return;

	CV_RegisterVar(&cv_precipdensity);
	CV_RegisterVar(&cv_drawdist);
	CV_RegisterVar(&cv_drawdist_nights);
	CV_RegisterVar(&cv_drawdist_precip);

	CV_RegisterVar(&cv_chasecam);
	CV_RegisterVar(&cv_chasecam2);
	CV_RegisterVar(&cv_shadow);
	CV_RegisterVar(&cv_shadowoffs);
	CV_RegisterVar(&cv_skybox);

	CV_RegisterVar(&cv_cam_dist);
	CV_RegisterVar(&cv_cam_still);
	CV_RegisterVar(&cv_cam_height);
	CV_RegisterVar(&cv_cam_speed);
	CV_RegisterVar(&cv_cam_rotate);
	CV_RegisterVar(&cv_cam_rotspeed);

	CV_RegisterVar(&cv_cam2_dist);
	CV_RegisterVar(&cv_cam2_still);
	CV_RegisterVar(&cv_cam2_height);
	CV_RegisterVar(&cv_cam2_speed);
	CV_RegisterVar(&cv_cam2_rotate);
	CV_RegisterVar(&cv_cam2_rotspeed);

	CV_RegisterVar(&cv_showhud);
	CV_RegisterVar(&cv_translucenthud);

	// Default viewheight is changeable,
	// initialized to standard viewheight
	CV_RegisterVar(&cv_viewheight);

#ifdef HWRENDER
	// GL-specific Commands
	CV_RegisterVar(&cv_grgammablue);
	CV_RegisterVar(&cv_grgammagreen);
	CV_RegisterVar(&cv_grgammared);
	CV_RegisterVar(&cv_grfovchange);
	CV_RegisterVar(&cv_grfog);
	CV_RegisterVar(&cv_voodoocompatibility);
	CV_RegisterVar(&cv_grfogcolor);
	CV_RegisterVar(&cv_grsoftwarefog);
	CV_RegisterVar(&cv_grstaticlighting);
	CV_RegisterVar(&cv_grdynamiclighting);
	CV_RegisterVar(&cv_grcoronas);
	CV_RegisterVar(&cv_grcoronasize);
#endif

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
		HWR_AddCommands();
#endif
}
