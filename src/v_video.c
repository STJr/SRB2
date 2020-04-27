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
/// \file  v_video.c
/// \brief Gamma correction LUT stuff
///        Functions to blit a block to the screen.

#include "doomdef.h"
#include "p_local.h"
#include "r_local.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "console.h"

#include "i_video.h" // rendermode
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "doomstat.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

// Each screen is [vid.width*vid.height];
UINT8 *screens[5];
// screens[0] = main display window
// screens[1] = back screen, alternative blitting
// screens[2] = screenshot buffer, gif movie buffer
// screens[3] = fade screen start
// screens[4] = fade screen end, postimage tempoarary buffer

consvar_t cv_ticrate = {"showfps", "No", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

static void CV_palette_OnChange(void);

static CV_PossibleValue_t gamma_cons_t[] = {{-15, "MIN"}, {5, "MAX"}, {0, NULL}};
consvar_t cv_globalgamma = {"gamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t saturation_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_globalsaturation = {"saturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};

#define huecoloursteps 4

static CV_PossibleValue_t hue_cons_t[] = {{0, "MIN"}, {(huecoloursteps*6)-1, "MAX"}, {0, NULL}};
consvar_t cv_rhue = {"rhue",  "0", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_yhue = {"yhue",  "4", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghue = {"ghue",  "8", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chue = {"chue", "12", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_bhue = {"bhue", "16", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mhue = {"mhue", "20", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_rgamma = {"rgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ygamma = {"ygamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ggamma = {"ggamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cgamma = {"cgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_bgamma = {"bgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mgamma = {"mgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_rsaturation = {"rsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ysaturation = {"ysaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_gsaturation = {"gsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_csaturation = {"csaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_bsaturation = {"bsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_msaturation = {"msaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_allcaps = {"allcaps", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t constextsize_cons_t[] = {
	{V_NOSCALEPATCH, "Small"}, {V_SMALLSCALEPATCH, "Medium"}, {V_MEDSCALEPATCH, "Large"}, {0, "Huge"},
	{0, NULL}};
static void CV_constextsize_OnChange(void);
consvar_t cv_constextsize = {"con_textsize", "Medium", CV_SAVE|CV_CALL, constextsize_cons_t, CV_constextsize_OnChange, 0, NULL, NULL, 0, 0, NULL};

// local copy of the palette for V_GetColor()
RGBA_t *pLocalPalette = NULL;
RGBA_t *pMasterPalette = NULL;

/*
The following was an extremely helpful resource when developing my Colour Cube LUT.
http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter24.html
Please check it out if you're trying to maintain this.
toast 18/04/17
*/
float Cubepal[2][2][2][3];
boolean Cubeapply = false;

// returns whether to apply cube, selectively avoiding expensive operations
static boolean InitCube(void)
{
	boolean apply = false;
	UINT8 q;
	float working[2][2][2][3] = // the initial positions of the corners of the colour cube!
	{
		{
			{
				{0.0, 0.0, 0.0}, // black corner
				{0.0, 0.0, 1.0}  // blue corner
			},
			{
				{0.0, 1.0, 0.0}, // green corner
				{0.0, 1.0, 1.0}  // cyan corner
			}
		},
		{
			{
				{1.0, 0.0, 0.0}, // red corner
				{1.0, 0.0, 1.0}  // magenta corner
			},
			{
				{1.0, 1.0, 0.0}, // yellow corner
				{1.0, 1.0, 1.0}  // white corner
			}
		}
	};
	float desatur[3]; // grey
	float globalgammamul, globalgammaoffs;
	boolean doinggamma;

#define diffcons(cv) (cv.value != atoi(cv.defaultvalue))

	doinggamma = diffcons(cv_globalgamma);

#define gammascale 8
	globalgammamul = (cv_globalgamma.value ? ((255 - (gammascale*abs(cv_globalgamma.value)))/255.0) : 1.0);
	globalgammaoffs = ((cv_globalgamma.value > 0) ? ((gammascale*cv_globalgamma.value)/255.0) : 0.0);
	desatur[0] = desatur[1] = desatur[2] = globalgammaoffs + (0.33*globalgammamul);

	if (doinggamma
		|| diffcons(cv_rhue)
		|| diffcons(cv_yhue)
		|| diffcons(cv_ghue)
		|| diffcons(cv_chue)
		|| diffcons(cv_bhue)
		|| diffcons(cv_mhue)
		|| diffcons(cv_rgamma)
		|| diffcons(cv_ygamma)
		|| diffcons(cv_ggamma)
		|| diffcons(cv_cgamma)
		|| diffcons(cv_bgamma)
		|| diffcons(cv_mgamma)) // set the gamma'd/hued positions (saturation is done later)
	{
		float mod, tempgammamul, tempgammaoffs;

		apply = true;

		working[0][0][0][0] = working[0][0][0][1] = working[0][0][0][2] = globalgammaoffs;
		working[1][1][1][0] = working[1][1][1][1] = working[1][1][1][2] = globalgammaoffs+globalgammamul;

#define dohue(hue, gamma, loc) \
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		mod = ((hue % huecoloursteps)*(tempgammamul)/huecoloursteps);\
		switch (hue/huecoloursteps)\
		{\
			case 0:\
			default:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs+mod;\
				loc[2] = tempgammaoffs;\
				break;\
			case 1:\
				loc[0] = tempgammaoffs+tempgammamul-mod;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs;\
				break;\
			case 2:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs+mod;\
				break;\
			case 3:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul-mod;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 4:\
				loc[0] = tempgammaoffs+mod;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 5:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul-mod;\
				break;\
		}
		dohue(cv_rhue.value, cv_rgamma.value, working[1][0][0]);
		dohue(cv_yhue.value, cv_ygamma.value, working[1][1][0]);
		dohue(cv_ghue.value, cv_ggamma.value, working[0][1][0]);
		dohue(cv_chue.value, cv_cgamma.value, working[0][1][1]);
		dohue(cv_bhue.value, cv_bgamma.value, working[0][0][1]);
		dohue(cv_mhue.value, cv_mgamma.value, working[1][0][1]);
#undef dohue
	}

#define dosaturation(a, e) a = ((1 - work)*e + work*a)
#define docvsat(cv_sat, hue, gamma, r, g, b) \
	if diffcons(cv_sat)\
	{\
		float work, mod, tempgammamul, tempgammaoffs;\
		apply = true;\
		work = (cv_sat.value/10.0);\
		mod = ((hue % huecoloursteps)*(1.0)/huecoloursteps);\
		if (hue & huecoloursteps)\
			mod = 2-mod;\
		else\
			mod += 1;\
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		for (q = 0; q < 3; q++)\
			dosaturation(working[r][g][b][q], (tempgammaoffs+(desatur[q]*mod*tempgammamul)));\
	}

	docvsat(cv_rsaturation, cv_rhue.value, cv_rgamma.value, 1, 0, 0);
	docvsat(cv_ysaturation, cv_yhue.value, cv_ygamma.value, 1, 1, 0);
	docvsat(cv_gsaturation, cv_ghue.value, cv_ggamma.value, 0, 1, 0);
	docvsat(cv_csaturation, cv_chue.value, cv_cgamma.value, 0, 1, 1);
	docvsat(cv_bsaturation, cv_bhue.value, cv_bgamma.value, 0, 0, 1);
	docvsat(cv_msaturation, cv_mhue.value, cv_mgamma.value, 1, 0, 1);

#undef gammascale

	if diffcons(cv_globalsaturation)
	{
		float work = (cv_globalsaturation.value/10.0);

		apply = true;

		for (q = 0; q < 3; q++)
		{
			dosaturation(working[1][0][0][q], desatur[q]);
			dosaturation(working[0][1][0][q], desatur[q]);
			dosaturation(working[0][0][1][q], desatur[q]);

			dosaturation(working[1][1][0][q], 2*desatur[q]);
			dosaturation(working[0][1][1][q], 2*desatur[q]);
			dosaturation(working[1][0][1][q], 2*desatur[q]);
		}
	}

#undef dosaturation

#undef diffcons

	if (!apply)
		return false;

#define dowork(i, j, k, l) \
	if (working[i][j][k][l] > 1.0)\
		working[i][j][k][l] = 1.0;\
	else if (working[i][j][k][l] < 0.0)\
		working[i][j][k][l] = 0.0;\
	Cubepal[i][j][k][l] = working[i][j][k][l]
	for (q = 0; q < 3; q++)
	{
		dowork(0, 0, 0, q);
		dowork(1, 0, 0, q);
		dowork(0, 1, 0, q);
		dowork(1, 1, 0, q);
		dowork(0, 0, 1, q);
		dowork(1, 0, 1, q);
		dowork(0, 1, 1, q);
		dowork(1, 1, 1, q);
	}
#undef dowork

	return true;
}

#ifdef BACKWARDSCOMPATCORRECTION
/*
So it turns out that the way gamma was implemented previously, the default
colour profile of the game was messed up. Since this bad decision has been
around for a long time, and the intent is to keep the base game looking the
same, I'm not gonna be the one to remove this base modification.
toast 20/04/17
... welp yes i am (27/07/19, see the ifdef around it)
*/
const UINT8 correctiontable[256] =
	{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
	65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
	81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
	97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
	113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
	208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
#endif

// keep a copy of the palette so that we can get the RGB value for a color index at any time.
static void LoadPalette(const char *lumpname)
{
	lumpnum_t lumpnum = W_GetNumForName(lumpname);
	size_t i, palsize = W_LumpLength(lumpnum)/3;
	UINT8 *pal;

	Cubeapply = InitCube();

	Z_Free(pLocalPalette);
	Z_Free(pMasterPalette);

	pLocalPalette = Z_Malloc(sizeof (*pLocalPalette)*palsize, PU_STATIC, NULL);
	pMasterPalette = Z_Malloc(sizeof (*pMasterPalette)*palsize, PU_STATIC, NULL);

	pal = W_CacheLumpNum(lumpnum, PU_CACHE);
	for (i = 0; i < palsize; i++)
	{
#ifdef BACKWARDSCOMPATCORRECTION
		pMasterPalette[i].s.red = pLocalPalette[i].s.red = correctiontable[*pal++];
		pMasterPalette[i].s.green = pLocalPalette[i].s.green = correctiontable[*pal++];
		pMasterPalette[i].s.blue = pLocalPalette[i].s.blue = correctiontable[*pal++];
#else
		pMasterPalette[i].s.red = pLocalPalette[i].s.red = *pal++;
		pMasterPalette[i].s.green = pLocalPalette[i].s.green = *pal++;
		pMasterPalette[i].s.blue = pLocalPalette[i].s.blue = *pal++;
#endif
		pMasterPalette[i].s.alpha = pLocalPalette[i].s.alpha = 0xFF;

		// lerp of colour cubing! if you want, make it smoother yourself
		if (Cubeapply)
			V_CubeApply(&pLocalPalette[i].s.red, &pLocalPalette[i].s.green, &pLocalPalette[i].s.blue);
	}
}

void V_CubeApply(UINT8 *red, UINT8 *green, UINT8 *blue)
{
	float working[4][3];
	float linear;
	UINT8 q;

	if (!Cubeapply)
		return;

	linear = (*red/255.0);
#define dolerp(e1, e2) ((1 - linear)*e1 + linear*e2)
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(Cubepal[0][0][0][q], Cubepal[1][0][0][q]);
		working[1][q] = dolerp(Cubepal[0][1][0][q], Cubepal[1][1][0][q]);
		working[2][q] = dolerp(Cubepal[0][0][1][q], Cubepal[1][0][1][q]);
		working[3][q] = dolerp(Cubepal[0][1][1][q], Cubepal[1][1][1][q]);
	}
	linear = (*green/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(working[0][q], working[1][q]);
		working[1][q] = dolerp(working[2][q], working[3][q]);
	}
	linear = (*blue/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = 255*dolerp(working[0][q], working[1][q]);
		if (working[0][q] > 255.0)
			working[0][q] = 255.0;
		else if (working[0][q]  < 0.0)
			working[0][q] = 0.0;
	}
#undef dolerp

	*red = (UINT8)(working[0][0]);
	*green = (UINT8)(working[0][1]);
	*blue = (UINT8)(working[0][2]);
}

const char *R_GetPalname(UINT16 num)
{
	static char palname[9];
	char newpal[9] = "PLAYPAL";

	if (num > 0 && num <= 10000)
		snprintf(newpal, 8, "PAL%04u", num-1);

	strncpy(palname, newpal, 8);
	return palname;
}

const char *GetPalette(void)
{
	if (gamestate == GS_LEVEL)
		return R_GetPalname(mapheaderinfo[gamemap-1]->palette);
	return "PLAYPAL";
}

static void LoadMapPalette(void)
{
	LoadPalette(GetPalette());
}

// -------------+
// V_SetPalette : Set the current palette to use for palettized graphics
//              :
// -------------+
void V_SetPalette(INT32 palettenum)
{
	if (!pLocalPalette)
		LoadMapPalette();

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
		HWR_SetPalette(&pLocalPalette[palettenum*256]);
#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(&pLocalPalette[palettenum*256]);
}

void V_SetPaletteLump(const char *pal)
{
	LoadPalette(pal);
#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
		HWR_SetPalette(pLocalPalette);
#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(pLocalPalette);
}

static void CV_palette_OnChange(void)
{
	// reload palette
	LoadMapPalette();
	V_SetPalette(0);
}

#if defined (__GNUC__) && defined (__i386__) && !defined (NOASM) && !defined (__APPLE__) && !defined (NORUSEASM)
void VID_BlitLinearScreen_ASM(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes);
#define HAVE_VIDCOPY
#endif

static void CV_constextsize_OnChange(void)
{
	con_recalc = true;
}


// --------------------------------------------------------------------------
// Copy a rectangular area from one bitmap to another (8bpp)
// --------------------------------------------------------------------------
void VID_BlitLinearScreen(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes)
{
#ifdef HAVE_VIDCOPY
    VID_BlitLinearScreen_ASM(srcptr,destptr,width,height,srcrowbytes,destrowbytes);
#else
	if (srcrowbytes == destrowbytes)
		M_Memcpy(destptr, srcptr, srcrowbytes * height);
	else
	{
		while (height--)
		{
			M_Memcpy(destptr, srcptr, width);

			destptr += destrowbytes;
			srcptr += srcrowbytes;
		}
	}
#endif
}

boolean *heatshifter = NULL;
INT32 lastheight = 0;
INT32 heatindex[2] = { 0, 0 };

//
// V_DoPostProcessor
//
// Perform a particular image postprocessing function.
//
#include "p_local.h"
void V_DoPostProcessor(INT32 view, postimg_t type, INT32 param)
{
#if NUMSCREENS < 5
	// do not enable image post processing for ARM, SH and MIPS CPUs
	(void)view;
	(void)type;
	(void)param;
#else
	INT32 height, yoffset;

#ifdef HWRENDER
	// draw a hardware converted patch
	if (rendermode != render_soft && rendermode != render_none)
		return;
#endif

	if (view < 0 || view >= 2 || (view == 1 && !splitscreen))
		return;

	if (splitscreen)
		height = vid.height/2;
	else
		height = vid.height;

	if (view == 1)
		yoffset = vid.height/2;
	else
		yoffset = 0;

	if (type == postimg_water)
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;
		angle_t disStart = (leveltime * 128) & FINEMASK; // in 0 to FINEANGLE
		INT32 newpix;
		INT32 sine;
		//UINT8 *transme = transtables + ((tr_trans50-1)<<FF_TRANSSHIFT);

		for (y = yoffset; y < yoffset+height; y++)
		{
			sine = (FINESINE(disStart)*5)>>FRACBITS;
			newpix = abs(sine);

			if (sine < 0)
			{
				M_Memcpy(&tmpscr[y*vid.width+newpix], &srcscr[y*vid.width], vid.width-newpix);

				// Cleanup edge
				while (newpix)
				{
					tmpscr[y*vid.width+newpix] = srcscr[y*vid.width];
					newpix--;
				}
			}
			else
			{
				M_Memcpy(&tmpscr[y*vid.width+0], &srcscr[y*vid.width+sine], vid.width-newpix);

				// Cleanup edge
				while (newpix)
				{
					tmpscr[y*vid.width+vid.width-newpix] = srcscr[y*vid.width+(vid.width-1)];
					newpix--;
				}
			}

/*
Unoptimized version
			for (x = 0; x < vid.width*vid.bpp; x++)
			{
				newpix = (x + sine);

				if (newpix < 0)
					newpix = 0;
				else if (newpix >= vid.width)
					newpix = vid.width-1;

				tmpscr[y*vid.width + x] = srcscr[y*vid.width+newpix]; // *(transme + (srcscr[y*vid.width+x]<<8) + srcscr[y*vid.width+newpix]);
			}*/
			disStart += 22;//the offset into the displacement map, increment each game loop
			disStart &= FINEMASK; //clip it to FINEMASK
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_motion) // Motion Blur!
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 x, y;

		// TODO: Add a postimg_param so that we can pick the translucency level...
		UINT8 *transme = transtables + ((param-1)<<FF_TRANSSHIFT);

		for (y = yoffset; y < yoffset+height; y++)
		{
			for (x = 0; x < vid.width; x++)
			{
				tmpscr[y*vid.width + x]
					=     colormaps[*(transme     + (srcscr   [y*vid.width+x ] <<8) + (tmpscr[y*vid.width+x]))];
			}
		}
		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_flip) // Flip the screen upside-down
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y, y2;

		for (y = yoffset, y2 = yoffset+height - 1; y < yoffset+height; y++, y2--)
			M_Memcpy(&tmpscr[y2*vid.width], &srcscr[y*vid.width], vid.width);

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_heat) // Heat wave
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;

		// Make sure table is built
		if (heatshifter == NULL || lastheight != height)
		{
			if (heatshifter)
				Z_Free(heatshifter);

			heatshifter = Z_Calloc(height * sizeof(boolean), PU_STATIC, NULL);

			for (y = 0; y < height; y++)
			{
				if (M_RandomChance(FRACUNIT/8)) // 12.5%
					heatshifter[y] = true;
			}

			heatindex[0] = heatindex[1] = 0;
			lastheight = height;
		}

		for (y = yoffset; y < yoffset+height; y++)
		{
			if (heatshifter[heatindex[view]++])
			{
				// Shift this row of pixels to the right by 2
				tmpscr[y*vid.width] = srcscr[y*vid.width];
				M_Memcpy(&tmpscr[y*vid.width+vid.dupx], &srcscr[y*vid.width], vid.width-vid.dupx);
			}
			else
				M_Memcpy(&tmpscr[y*vid.width], &srcscr[y*vid.width], vid.width);

			heatindex[view] %= height;
		}

		heatindex[view]++;
		heatindex[view] %= vid.height;

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset, screens[0]+vid.width*vid.bpp*yoffset,
				vid.width*vid.bpp, height, vid.width*vid.bpp, vid.width);
	}
#endif
}

// Generates a color look-up table
// which has up to 64 colors at each channel
// (see the defines in v_video.h)

UINT8 colorlookup[CLUTSIZE][CLUTSIZE][CLUTSIZE];

void InitColorLUT(RGBA_t *palette)
{
	UINT8 r, g, b;
	static boolean clutinit = false;
	static RGBA_t *lastpalette = NULL;
	if ((!clutinit) || (lastpalette != palette))
	{
		for (r = 0; r < CLUTSIZE; r++)
			for (g = 0; g < CLUTSIZE; g++)
				for (b = 0; b < CLUTSIZE; b++)
					colorlookup[r][g][b] = NearestPaletteColor(r << SHIFTCOLORBITS, g << SHIFTCOLORBITS, b << SHIFTCOLORBITS, palette);
		clutinit = true;
		lastpalette = palette;
	}
}

// V_Init
// old software stuff, buffers are allocated at video mode setup
// here we set the screens[x] pointers accordingly
// WARNING: called at runtime (don't init cvar here)
void V_Init(void)
{
	INT32 i;
	UINT8 *base = vid.buffer;
	const INT32 screensize = vid.rowbytes * vid.height;

	LoadMapPalette();

	for (i = 0; i < NUMSCREENS; i++)
		screens[i] = NULL;

	// start address of NUMSCREENS * width*height vidbuffers
	if (base)
	{
		for (i = 0; i < NUMSCREENS; i++)
			screens[i] = base + i*screensize;
	}

	if (vid.direct)
		screens[0] = vid.direct;

#ifdef DEBUG
	CONS_Debug(DBG_RENDER, "V_Init done:\n");
	for (i = 0; i < NUMSCREENS+1; i++)
		CONS_Debug(DBG_RENDER, " screens[%d] = %x\n", i, screens[i]);
#endif
}

// V_Init
// Used to be portions of SCR_Startup and SCR_Recalc.
void V_Recalc(void)
{
	// scale 1,2,3 times in x and y the patches for the menus and overlays...
	// calculated once and for all, used by routines in v_video.c and v_draw.c
	vid.dupx = vid.width / BASEVIDWIDTH;
	vid.dupy = vid.height / BASEVIDHEIGHT;
	vid.dupx = vid.dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	vid.fdupx = FixedDiv(vid.width*FRACUNIT, BASEVIDWIDTH*FRACUNIT);
	vid.fdupy = FixedDiv(vid.height*FRACUNIT, BASEVIDHEIGHT*FRACUNIT);

#ifdef HWRENDER
	//if (rendermode != render_opengl && rendermode != render_none) // This was just placing it incorrectly at non aspect correct resolutions in opengl
	// 13/11/18:
	// The above is no longer necessary, since we want OpenGL to be just like software now
	// -- Monster Iestyn
#endif
		vid.fdupx = vid.fdupy = (vid.fdupx < vid.fdupy ? vid.fdupx : vid.fdupy);

	vid.meddupx = (UINT8)(vid.dupx >> 1) + 1;
	vid.meddupy = (UINT8)(vid.dupy >> 1) + 1;
#ifdef HWRENDER
	vid.fmeddupx = vid.meddupx*FRACUNIT;
	vid.fmeddupy = vid.meddupy*FRACUNIT;
#endif

	vid.smalldupx = (UINT8)(vid.dupx / 3) + 1;
	vid.smalldupy = (UINT8)(vid.dupy / 3) + 1;
#ifdef HWRENDER
	vid.fsmalldupx = vid.smalldupx*FRACUNIT;
	vid.fsmalldupy = vid.smalldupy*FRACUNIT;
#endif
}

// After a renderer switch, flush and reload patches.
void V_FlushPatches(void)
{
	// flush all patches from memory
	// (also frees memory tagged with PU_CACHE)
	// (which are not necessarily patches but I don't care)
	if (needpatchflush)
		Z_FlushCachedPatches();

	// some patches have been freed,
	// so cache them again
	if (needpatchrecache)
		V_ReloadHUDGraphics();
}

void V_ReloadHUDGraphics(void)
{
	CONS_Debug(DBG_RENDER, "V_ReloadHUDGraphics()...\n");
	ST_LoadGraphics();
	HU_LoadGraphics();
	ST_ReloadSkinFaceGraphics();
}
