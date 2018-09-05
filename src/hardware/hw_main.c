// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief hardware renderer, using the standard HardWareRender driver DLL for SRB2

#include <math.h>

#include "../doomstat.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_light.h"
#include "hw_drv.h"

#include "../i_video.h" // for rendermode == render_glide
#include "../v_video.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../r_local.h"
#include "../r_bsp.h"
#include "../d_clisrv.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_splats.h"
#include "../g_game.h"
#include "../st_stuff.h"
#include "../i_system.h"
#include "../m_cheat.h"
#ifdef ESLOPE
#include "../p_slopes.h"
#endif
#include "hw_md2.h"

#define R_FAKEFLOORS
#define HWPRECIP
#define SORTING
//#define POLYSKY

// ==========================================================================
// the hardware driver object
// ==========================================================================
struct hwdriver_s hwdriver;

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================


static void HWR_AddSprites(sector_t *sec);
static void HWR_ProjectSprite(mobj_t *thing);
#ifdef HWPRECIP
static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing);
#endif

#ifdef SORTING
void HWR_AddTransparentFloor(lumpnum_t lumpnum, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap);
void HWR_AddTransparentPolyobjectFloor(lumpnum_t lumpnum, polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
                             INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap);
#else
static void HWR_Add3DWater(lumpnum_t lumpnum, extrasubsector_t *xsub, fixed_t fixedheight,
                           INT32 lightlevel, INT32 alpha, sector_t *FOFSector);
static void HWR_Render3DWater(void);
static void HWR_RenderTransparentWalls(void);
#endif
static void HWR_FoggingOn(void);
static UINT32 atohex(const char *s);

static void CV_filtermode_ONChange(void);
static void CV_anisotropic_ONChange(void);
static void CV_FogDensity_ONChange(void);
static void CV_grFov_OnChange(void);
// ==========================================================================
//                                          3D ENGINE COMMANDS & CONSOLE VARS
// ==========================================================================

static CV_PossibleValue_t grfov_cons_t[] = {{0, "MIN"}, {179*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t grfiltermode_cons_t[]= {{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Nearest"},
	{HWD_SET_TEXTUREFILTER_BILINEAR, "Bilinear"}, {HWD_SET_TEXTUREFILTER_TRILINEAR, "Trilinear"},
	{HWD_SET_TEXTUREFILTER_MIXED1, "Linear_Nearest"},
	{HWD_SET_TEXTUREFILTER_MIXED2, "Nearest_Linear"},
	{HWD_SET_TEXTUREFILTER_MIXED3, "Nearest_Mipmap"},
	{0, NULL}};
CV_PossibleValue_t granisotropicmode_cons_t[] = {{1, "MIN"}, {16, "MAX"}, {0, NULL}};

boolean drawsky = true;

// needs fix: walls are incorrectly clipped one column less
static consvar_t cv_grclipwalls = {"gr_clipwalls", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

//development variables for diverse uses
static consvar_t cv_gralpha = {"gr_alpha", "160", 0, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_grbeta = {"gr_beta", "0", 0, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

static float HWRWipeCounter = 1.0f;
consvar_t cv_grrounddown = {"gr_rounddown", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grfov = {"gr_fov", "90", CV_FLOAT|CV_CALL, grfov_cons_t, CV_grFov_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grfogdensity = {"gr_fogdensity", "150", CV_CALL|CV_NOINIT, CV_Unsigned,
                             CV_FogDensity_ONChange, 0, NULL, NULL, 0, 0, NULL};

// Unfortunately, this can no longer be saved..
consvar_t cv_grfiltermode = {"gr_filtermode", "Nearest", CV_CALL, grfiltermode_cons_t,
                             CV_filtermode_ONChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_granisotropicmode = {"gr_anisotropicmode", "1", CV_CALL, granisotropicmode_cons_t,
                             CV_anisotropic_ONChange, 0, NULL, NULL, 0, 0, NULL};
//static consvar_t cv_grzbuffer = {"gr_zbuffer", "On", 0, CV_OnOff};
consvar_t cv_grcorrecttricks = {"gr_correcttricks", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grsolvetjoin = {"gr_solvetjoin", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void CV_FogDensity_ONChange(void)
{
	HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
}

static void CV_filtermode_ONChange(void)
{
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
}

static void CV_anisotropic_ONChange(void)
{
	HWD.pfnSetSpecialState(HWD_SET_TEXTUREANISOTROPICMODE, cv_granisotropicmode.value);
}

/*
 * lookuptable for lightvalues
 * calculated as follow:
 * floatlight = (1.0-exp((light^3)*gamma)) / (1.0-exp(1.0*gamma));
 * gamma=-0,2;-2,0;-4,0;-6,0;-8,0
 * light = 0,0 .. 1,0
 */
static const float lighttable[5][256] =
{
  {
    0.00000f,0.00000f,0.00000f,0.00000f,0.00000f,0.00001f,0.00001f,0.00002f,0.00003f,0.00004f,
    0.00006f,0.00008f,0.00010f,0.00013f,0.00017f,0.00020f,0.00025f,0.00030f,0.00035f,0.00041f,
    0.00048f,0.00056f,0.00064f,0.00073f,0.00083f,0.00094f,0.00106f,0.00119f,0.00132f,0.00147f,
    0.00163f,0.00180f,0.00198f,0.00217f,0.00237f,0.00259f,0.00281f,0.00305f,0.00331f,0.00358f,
    0.00386f,0.00416f,0.00447f,0.00479f,0.00514f,0.00550f,0.00587f,0.00626f,0.00667f,0.00710f,
    0.00754f,0.00800f,0.00848f,0.00898f,0.00950f,0.01003f,0.01059f,0.01117f,0.01177f,0.01239f,
    0.01303f,0.01369f,0.01437f,0.01508f,0.01581f,0.01656f,0.01734f,0.01814f,0.01896f,0.01981f,
    0.02069f,0.02159f,0.02251f,0.02346f,0.02444f,0.02544f,0.02647f,0.02753f,0.02862f,0.02973f,
    0.03088f,0.03205f,0.03325f,0.03448f,0.03575f,0.03704f,0.03836f,0.03971f,0.04110f,0.04252f,
    0.04396f,0.04545f,0.04696f,0.04851f,0.05009f,0.05171f,0.05336f,0.05504f,0.05676f,0.05852f,
    0.06031f,0.06214f,0.06400f,0.06590f,0.06784f,0.06981f,0.07183f,0.07388f,0.07597f,0.07810f,
    0.08027f,0.08248f,0.08473f,0.08702f,0.08935f,0.09172f,0.09414f,0.09659f,0.09909f,0.10163f,
    0.10421f,0.10684f,0.10951f,0.11223f,0.11499f,0.11779f,0.12064f,0.12354f,0.12648f,0.12946f,
    0.13250f,0.13558f,0.13871f,0.14188f,0.14511f,0.14838f,0.15170f,0.15507f,0.15850f,0.16197f,
    0.16549f,0.16906f,0.17268f,0.17635f,0.18008f,0.18386f,0.18769f,0.19157f,0.19551f,0.19950f,
    0.20354f,0.20764f,0.21179f,0.21600f,0.22026f,0.22458f,0.22896f,0.23339f,0.23788f,0.24242f,
    0.24702f,0.25168f,0.25640f,0.26118f,0.26602f,0.27091f,0.27587f,0.28089f,0.28596f,0.29110f,
    0.29630f,0.30156f,0.30688f,0.31226f,0.31771f,0.32322f,0.32879f,0.33443f,0.34013f,0.34589f,
    0.35172f,0.35761f,0.36357f,0.36960f,0.37569f,0.38185f,0.38808f,0.39437f,0.40073f,0.40716f,
    0.41366f,0.42022f,0.42686f,0.43356f,0.44034f,0.44718f,0.45410f,0.46108f,0.46814f,0.47527f,
    0.48247f,0.48974f,0.49709f,0.50451f,0.51200f,0.51957f,0.52721f,0.53492f,0.54271f,0.55058f,
    0.55852f,0.56654f,0.57463f,0.58280f,0.59105f,0.59937f,0.60777f,0.61625f,0.62481f,0.63345f,
    0.64217f,0.65096f,0.65984f,0.66880f,0.67783f,0.68695f,0.69615f,0.70544f,0.71480f,0.72425f,
    0.73378f,0.74339f,0.75308f,0.76286f,0.77273f,0.78268f,0.79271f,0.80283f,0.81304f,0.82333f,
    0.83371f,0.84417f,0.85472f,0.86536f,0.87609f,0.88691f,0.89781f,0.90880f,0.91989f,0.93106f,
    0.94232f,0.95368f,0.96512f,0.97665f,0.98828f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00003f,0.00005f,0.00007f,0.00010f,
    0.00014f,0.00019f,0.00024f,0.00031f,0.00038f,0.00047f,0.00057f,0.00069f,0.00081f,0.00096f,
    0.00112f,0.00129f,0.00148f,0.00170f,0.00193f,0.00218f,0.00245f,0.00274f,0.00306f,0.00340f,
    0.00376f,0.00415f,0.00456f,0.00500f,0.00547f,0.00597f,0.00649f,0.00704f,0.00763f,0.00825f,
    0.00889f,0.00957f,0.01029f,0.01104f,0.01182f,0.01264f,0.01350f,0.01439f,0.01532f,0.01630f,
    0.01731f,0.01836f,0.01945f,0.02058f,0.02176f,0.02298f,0.02424f,0.02555f,0.02690f,0.02830f,
    0.02974f,0.03123f,0.03277f,0.03436f,0.03600f,0.03768f,0.03942f,0.04120f,0.04304f,0.04493f,
    0.04687f,0.04886f,0.05091f,0.05301f,0.05517f,0.05738f,0.05964f,0.06196f,0.06434f,0.06677f,
    0.06926f,0.07181f,0.07441f,0.07707f,0.07979f,0.08257f,0.08541f,0.08831f,0.09126f,0.09428f,
    0.09735f,0.10048f,0.10368f,0.10693f,0.11025f,0.11362f,0.11706f,0.12056f,0.12411f,0.12773f,
    0.13141f,0.13515f,0.13895f,0.14281f,0.14673f,0.15072f,0.15476f,0.15886f,0.16303f,0.16725f,
    0.17153f,0.17587f,0.18028f,0.18474f,0.18926f,0.19383f,0.19847f,0.20316f,0.20791f,0.21272f,
    0.21759f,0.22251f,0.22748f,0.23251f,0.23760f,0.24274f,0.24793f,0.25318f,0.25848f,0.26383f,
    0.26923f,0.27468f,0.28018f,0.28573f,0.29133f,0.29697f,0.30266f,0.30840f,0.31418f,0.32001f,
    0.32588f,0.33179f,0.33774f,0.34374f,0.34977f,0.35585f,0.36196f,0.36810f,0.37428f,0.38050f,
    0.38675f,0.39304f,0.39935f,0.40570f,0.41207f,0.41847f,0.42490f,0.43136f,0.43784f,0.44434f,
    0.45087f,0.45741f,0.46398f,0.47057f,0.47717f,0.48379f,0.49042f,0.49707f,0.50373f,0.51041f,
    0.51709f,0.52378f,0.53048f,0.53718f,0.54389f,0.55061f,0.55732f,0.56404f,0.57075f,0.57747f,
    0.58418f,0.59089f,0.59759f,0.60429f,0.61097f,0.61765f,0.62432f,0.63098f,0.63762f,0.64425f,
    0.65086f,0.65746f,0.66404f,0.67060f,0.67714f,0.68365f,0.69015f,0.69662f,0.70307f,0.70948f,
    0.71588f,0.72224f,0.72857f,0.73488f,0.74115f,0.74739f,0.75359f,0.75976f,0.76589f,0.77199f,
    0.77805f,0.78407f,0.79005f,0.79599f,0.80189f,0.80774f,0.81355f,0.81932f,0.82504f,0.83072f,
    0.83635f,0.84194f,0.84747f,0.85296f,0.85840f,0.86378f,0.86912f,0.87441f,0.87964f,0.88482f,
    0.88995f,0.89503f,0.90005f,0.90502f,0.90993f,0.91479f,0.91959f,0.92434f,0.92903f,0.93366f,
    0.93824f,0.94276f,0.94723f,0.95163f,0.95598f,0.96027f,0.96451f,0.96868f,0.97280f,0.97686f,
    0.98086f,0.98481f,0.98869f,0.99252f,0.99629f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00003f,0.00005f,0.00008f,0.00013f,0.00018f,
    0.00025f,0.00033f,0.00042f,0.00054f,0.00067f,0.00083f,0.00101f,0.00121f,0.00143f,0.00168f,
    0.00196f,0.00227f,0.00261f,0.00299f,0.00339f,0.00383f,0.00431f,0.00483f,0.00538f,0.00598f,
    0.00661f,0.00729f,0.00802f,0.00879f,0.00961f,0.01048f,0.01140f,0.01237f,0.01340f,0.01447f,
    0.01561f,0.01680f,0.01804f,0.01935f,0.02072f,0.02215f,0.02364f,0.02520f,0.02682f,0.02850f,
    0.03026f,0.03208f,0.03397f,0.03594f,0.03797f,0.04007f,0.04225f,0.04451f,0.04684f,0.04924f,
    0.05172f,0.05428f,0.05691f,0.05963f,0.06242f,0.06530f,0.06825f,0.07129f,0.07441f,0.07761f,
    0.08089f,0.08426f,0.08771f,0.09125f,0.09487f,0.09857f,0.10236f,0.10623f,0.11019f,0.11423f,
    0.11836f,0.12257f,0.12687f,0.13125f,0.13571f,0.14027f,0.14490f,0.14962f,0.15442f,0.15931f,
    0.16427f,0.16932f,0.17445f,0.17966f,0.18496f,0.19033f,0.19578f,0.20130f,0.20691f,0.21259f,
    0.21834f,0.22417f,0.23007f,0.23605f,0.24209f,0.24820f,0.25438f,0.26063f,0.26694f,0.27332f,
    0.27976f,0.28626f,0.29282f,0.29944f,0.30611f,0.31284f,0.31962f,0.32646f,0.33334f,0.34027f,
    0.34724f,0.35426f,0.36132f,0.36842f,0.37556f,0.38273f,0.38994f,0.39718f,0.40445f,0.41174f,
    0.41907f,0.42641f,0.43378f,0.44116f,0.44856f,0.45598f,0.46340f,0.47084f,0.47828f,0.48573f,
    0.49319f,0.50064f,0.50809f,0.51554f,0.52298f,0.53042f,0.53784f,0.54525f,0.55265f,0.56002f,
    0.56738f,0.57472f,0.58203f,0.58932f,0.59658f,0.60381f,0.61101f,0.61817f,0.62529f,0.63238f,
    0.63943f,0.64643f,0.65339f,0.66031f,0.66717f,0.67399f,0.68075f,0.68746f,0.69412f,0.70072f,
    0.70726f,0.71375f,0.72017f,0.72653f,0.73282f,0.73905f,0.74522f,0.75131f,0.75734f,0.76330f,
    0.76918f,0.77500f,0.78074f,0.78640f,0.79199f,0.79751f,0.80295f,0.80831f,0.81359f,0.81880f,
    0.82393f,0.82898f,0.83394f,0.83883f,0.84364f,0.84836f,0.85301f,0.85758f,0.86206f,0.86646f,
    0.87078f,0.87502f,0.87918f,0.88326f,0.88726f,0.89118f,0.89501f,0.89877f,0.90245f,0.90605f,
    0.90957f,0.91301f,0.91638f,0.91966f,0.92288f,0.92601f,0.92908f,0.93206f,0.93498f,0.93782f,
    0.94059f,0.94329f,0.94592f,0.94848f,0.95097f,0.95339f,0.95575f,0.95804f,0.96027f,0.96244f,
    0.96454f,0.96658f,0.96856f,0.97049f,0.97235f,0.97416f,0.97591f,0.97760f,0.97924f,0.98083f,
    0.98237f,0.98386f,0.98530f,0.98669f,0.98803f,0.98933f,0.99058f,0.99179f,0.99295f,0.99408f,
    0.99516f,0.99620f,0.99721f,0.99817f,0.99910f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00005f,0.00008f,0.00012f,0.00019f,0.00026f,
    0.00036f,0.00048f,0.00063f,0.00080f,0.00099f,0.00122f,0.00148f,0.00178f,0.00211f,0.00249f,
    0.00290f,0.00335f,0.00386f,0.00440f,0.00500f,0.00565f,0.00636f,0.00711f,0.00793f,0.00881f,
    0.00975f,0.01075f,0.01182f,0.01295f,0.01416f,0.01543f,0.01678f,0.01821f,0.01971f,0.02129f,
    0.02295f,0.02469f,0.02652f,0.02843f,0.03043f,0.03252f,0.03469f,0.03696f,0.03933f,0.04178f,
    0.04433f,0.04698f,0.04973f,0.05258f,0.05552f,0.05857f,0.06172f,0.06498f,0.06834f,0.07180f,
    0.07537f,0.07905f,0.08283f,0.08672f,0.09072f,0.09483f,0.09905f,0.10337f,0.10781f,0.11236f,
    0.11701f,0.12178f,0.12665f,0.13163f,0.13673f,0.14193f,0.14724f,0.15265f,0.15817f,0.16380f,
    0.16954f,0.17538f,0.18132f,0.18737f,0.19351f,0.19976f,0.20610f,0.21255f,0.21908f,0.22572f,
    0.23244f,0.23926f,0.24616f,0.25316f,0.26023f,0.26739f,0.27464f,0.28196f,0.28935f,0.29683f,
    0.30437f,0.31198f,0.31966f,0.32740f,0.33521f,0.34307f,0.35099f,0.35896f,0.36699f,0.37506f,
    0.38317f,0.39133f,0.39952f,0.40775f,0.41601f,0.42429f,0.43261f,0.44094f,0.44929f,0.45766f,
    0.46604f,0.47443f,0.48283f,0.49122f,0.49962f,0.50801f,0.51639f,0.52476f,0.53312f,0.54146f,
    0.54978f,0.55807f,0.56633f,0.57457f,0.58277f,0.59093f,0.59905f,0.60713f,0.61516f,0.62314f,
    0.63107f,0.63895f,0.64676f,0.65452f,0.66221f,0.66984f,0.67739f,0.68488f,0.69229f,0.69963f,
    0.70689f,0.71407f,0.72117f,0.72818f,0.73511f,0.74195f,0.74870f,0.75536f,0.76192f,0.76839f,
    0.77477f,0.78105f,0.78723f,0.79331f,0.79930f,0.80518f,0.81096f,0.81664f,0.82221f,0.82768f,
    0.83305f,0.83832f,0.84347f,0.84853f,0.85348f,0.85832f,0.86306f,0.86770f,0.87223f,0.87666f,
    0.88098f,0.88521f,0.88933f,0.89334f,0.89726f,0.90108f,0.90480f,0.90842f,0.91194f,0.91537f,
    0.91870f,0.92193f,0.92508f,0.92813f,0.93109f,0.93396f,0.93675f,0.93945f,0.94206f,0.94459f,
    0.94704f,0.94941f,0.95169f,0.95391f,0.95604f,0.95810f,0.96009f,0.96201f,0.96386f,0.96564f,
    0.96735f,0.96900f,0.97059f,0.97212f,0.97358f,0.97499f,0.97634f,0.97764f,0.97888f,0.98007f,
    0.98122f,0.98231f,0.98336f,0.98436f,0.98531f,0.98623f,0.98710f,0.98793f,0.98873f,0.98949f,
    0.99021f,0.99090f,0.99155f,0.99218f,0.99277f,0.99333f,0.99387f,0.99437f,0.99486f,0.99531f,
    0.99575f,0.99616f,0.99654f,0.99691f,0.99726f,0.99759f,0.99790f,0.99819f,0.99847f,0.99873f,
    0.99897f,0.99920f,0.99942f,0.99963f,0.99982f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00003f,0.00006f,0.00010f,0.00017f,0.00025f,0.00035f,
    0.00048f,0.00064f,0.00083f,0.00106f,0.00132f,0.00163f,0.00197f,0.00237f,0.00281f,0.00330f,
    0.00385f,0.00446f,0.00513f,0.00585f,0.00665f,0.00751f,0.00845f,0.00945f,0.01054f,0.01170f,
    0.01295f,0.01428f,0.01569f,0.01719f,0.01879f,0.02048f,0.02227f,0.02415f,0.02614f,0.02822f,
    0.03042f,0.03272f,0.03513f,0.03765f,0.04028f,0.04303f,0.04589f,0.04887f,0.05198f,0.05520f,
    0.05855f,0.06202f,0.06561f,0.06933f,0.07318f,0.07716f,0.08127f,0.08550f,0.08987f,0.09437f,
    0.09900f,0.10376f,0.10866f,0.11369f,0.11884f,0.12414f,0.12956f,0.13512f,0.14080f,0.14662f,
    0.15257f,0.15865f,0.16485f,0.17118f,0.17764f,0.18423f,0.19093f,0.19776f,0.20471f,0.21177f,
    0.21895f,0.22625f,0.23365f,0.24117f,0.24879f,0.25652f,0.26435f,0.27228f,0.28030f,0.28842f,
    0.29662f,0.30492f,0.31329f,0.32175f,0.33028f,0.33889f,0.34756f,0.35630f,0.36510f,0.37396f,
    0.38287f,0.39183f,0.40084f,0.40989f,0.41897f,0.42809f,0.43723f,0.44640f,0.45559f,0.46479f,
    0.47401f,0.48323f,0.49245f,0.50167f,0.51088f,0.52008f,0.52927f,0.53843f,0.54757f,0.55668f,
    0.56575f,0.57479f,0.58379f,0.59274f,0.60164f,0.61048f,0.61927f,0.62799f,0.63665f,0.64524f,
    0.65376f,0.66220f,0.67056f,0.67883f,0.68702f,0.69511f,0.70312f,0.71103f,0.71884f,0.72655f,
    0.73415f,0.74165f,0.74904f,0.75632f,0.76348f,0.77053f,0.77747f,0.78428f,0.79098f,0.79756f,
    0.80401f,0.81035f,0.81655f,0.82264f,0.82859f,0.83443f,0.84013f,0.84571f,0.85117f,0.85649f,
    0.86169f,0.86677f,0.87172f,0.87654f,0.88124f,0.88581f,0.89026f,0.89459f,0.89880f,0.90289f,
    0.90686f,0.91071f,0.91445f,0.91807f,0.92157f,0.92497f,0.92826f,0.93143f,0.93450f,0.93747f,
    0.94034f,0.94310f,0.94577f,0.94833f,0.95081f,0.95319f,0.95548f,0.95768f,0.95980f,0.96183f,
    0.96378f,0.96565f,0.96744f,0.96916f,0.97081f,0.97238f,0.97388f,0.97532f,0.97669f,0.97801f,
    0.97926f,0.98045f,0.98158f,0.98266f,0.98369f,0.98467f,0.98560f,0.98648f,0.98732f,0.98811f,
    0.98886f,0.98958f,0.99025f,0.99089f,0.99149f,0.99206f,0.99260f,0.99311f,0.99359f,0.99404f,
    0.99446f,0.99486f,0.99523f,0.99559f,0.99592f,0.99623f,0.99652f,0.99679f,0.99705f,0.99729f,
    0.99751f,0.99772f,0.99792f,0.99810f,0.99827f,0.99843f,0.99857f,0.99871f,0.99884f,0.99896f,
    0.99907f,0.99917f,0.99926f,0.99935f,0.99943f,0.99951f,0.99958f,0.99964f,0.99970f,0.99975f,
    0.99980f,0.99985f,0.99989f,0.99993f,0.99997f,1.00000f
  }
};

#define gld_CalcLightLevel(lightlevel) (lighttable[1][max(min((lightlevel),255),0)])

// ==========================================================================
//                                                               VIEW GLOBALS
// ==========================================================================
// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW ANGLE_90
#define ABS(x) ((x) < 0 ? -(x) : (x))

static angle_t gr_clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
static INT32 gr_viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t gr_xtoviewangle[MAXVIDWIDTH+1];

// ==========================================================================
//                                                                    GLOBALS
// ==========================================================================

// uncomment to remove the plane rendering
#define DOPLANES
//#define DOWALLS

// test of drawing sky by polygons like in software with visplane, unfortunately
// this doesn't work since we must have z for pixel and z for texture (not like now with z = oow)
//#define POLYSKY

// test change fov when looking up/down but bsp projection messup :(
//#define NOCRAPPYMLOOK

/// \note crappy
#define drawtextured true

// base values set at SetViewSize
static float gr_basecentery;

float gr_baseviewwindowy, gr_basewindowcentery;
float gr_viewwidth, gr_viewheight; // viewport clipping boundaries (screen coords)
float gr_viewwindowx;

static float gr_centerx, gr_centery;
static float gr_viewwindowy; // top left corner of view window
static float gr_windowcenterx; // center of view window, for projection
static float gr_windowcentery;

static float gr_pspritexscale, gr_pspriteyscale;

static seg_t *gr_curline;
static side_t *gr_sidedef;
static line_t *gr_linedef;
static sector_t *gr_frontsector;
static sector_t *gr_backsector;

// --------------------------------------------------------------------------
//                                              STUFF FOR THE PROJECTION CODE
// --------------------------------------------------------------------------

FTransform atransform;
// duplicates of the main code, set after R_SetupFrame() passed them into sharedstruct,
// copied here for local use
static fixed_t dup_viewx, dup_viewy, dup_viewz;
static angle_t dup_viewangle;

static float gr_viewx, gr_viewy, gr_viewz;
static float gr_viewsin, gr_viewcos;

// Maybe not necessary with the new T&L code (needs to be checked!)
static float gr_viewludsin, gr_viewludcos; // look up down kik test
static float gr_fovlud;

// ==========================================================================
//                                    LIGHT stuffs
// ==========================================================================

static UINT8 lightleveltonumlut[256];

// added to SRB2's sector lightlevel to make things a bit brighter (sprites/walls/planes)
FUNCMATH UINT8 LightLevelToLum(INT32 l)
{
	return (UINT8)(255*gld_CalcLightLevel(l));
}

static inline void InitLumLut(void)
{
	INT32 i, k = 0;
	for (i = 0; i < 256; i++)
	{
		if (i > 128)
			k += 2;
		else
			k = 1;
		lightleveltonumlut[i] = (UINT8)(k);
	}
}

//#define FOGFACTOR 300 //was 600 >> Covered by cv_grfogdensity
#define NORMALFOG 0x00000000
#define FADEFOG 0x19000000
#define CALCFOGDENSITY(x) ((float)((5220.0f*(1.0f/((x)/41.0f+1.0f)))-(5220.0f*(1.0f/(255.0f/41.0f+1.0f))))) // Approximate fog calculation based off of software walls
#define CALCFOGDENSITYFLOOR(x) ((float)((40227.0f*(1.0f/((x)/11.0f+1.0f)))-(40227.0f*(1.0f/(255.0f/11.0f+1.0f))))) // Approximate fog calculation based off of software floors
#define CALCLIGHT(x,y) ((float)(x)*((y)/255.0f))
UINT32 HWR_Lighting(INT32 light, UINT32 color, UINT32 fadecolor, boolean fogblockpoly, boolean plane)
{
	RGBA_t realcolor, fogcolor, surfcolor;
	INT32 alpha, fogalpha;

	(void)fogblockpoly;

	// Don't go out of bounds
	if (light < 0)
		light = 0;
	else if (light > 255)
		light = 255;

	realcolor.rgba = color;
	fogcolor.rgba = fadecolor;

	alpha = (realcolor.s.alpha*255)/25;
	fogalpha = (fogcolor.s.alpha*255)/25;

	if (cv_grfog.value && cv_grsoftwarefog.value) // Only do this when fog is on, software fog mode is on, and the poly is not from a fog block
	{
		// Modulate the colors by alpha.
		realcolor.s.red = (UINT8)(CALCLIGHT(alpha,realcolor.s.red));
		realcolor.s.green = (UINT8)(CALCLIGHT(alpha,realcolor.s.green));
		realcolor.s.blue = (UINT8)(CALCLIGHT(alpha,realcolor.s.blue));

		// Set the surface colors and further modulate the colors by light.
		surfcolor.s.red = (UINT8)(CALCLIGHT((0xFF-alpha),255)+CALCLIGHT(realcolor.s.red,255));
		surfcolor.s.green = (UINT8)(CALCLIGHT((0xFF-alpha),255)+CALCLIGHT(realcolor.s.green,255));
		surfcolor.s.blue = (UINT8)(CALCLIGHT((0xFF-alpha),255)+CALCLIGHT(realcolor.s.blue,255));
		surfcolor.s.alpha = 0xFF;

		// Modulate the colors by alpha.
		fogcolor.s.red = (UINT8)(CALCLIGHT(fogalpha,fogcolor.s.red));
		fogcolor.s.green = (UINT8)(CALCLIGHT(fogalpha,fogcolor.s.green));
		fogcolor.s.blue = (UINT8)(CALCLIGHT(fogalpha,fogcolor.s.blue));
	}
	else
	{
		// Modulate the colors by alpha.
		realcolor.s.red = (UINT8)(CALCLIGHT(alpha,realcolor.s.red));
		realcolor.s.green = (UINT8)(CALCLIGHT(alpha,realcolor.s.green));
		realcolor.s.blue = (UINT8)(CALCLIGHT(alpha,realcolor.s.blue));

		// Set the surface colors and further modulate the colors by light.
		surfcolor.s.red = (UINT8)(CALCLIGHT((0xFF-alpha),light)+CALCLIGHT(realcolor.s.red,light));
		surfcolor.s.green = (UINT8)(CALCLIGHT((0xFF-alpha),light)+CALCLIGHT(realcolor.s.green,light));
		surfcolor.s.blue = (UINT8)(CALCLIGHT((0xFF-alpha),light)+CALCLIGHT(realcolor.s.blue,light));

		// Modulate the colors by alpha.
		fogcolor.s.red = (UINT8)(CALCLIGHT(fogalpha,fogcolor.s.red));
		fogcolor.s.green = (UINT8)(CALCLIGHT(fogalpha,fogcolor.s.green));
		fogcolor.s.blue = (UINT8)(CALCLIGHT(fogalpha,fogcolor.s.blue));

		// Set the surface colors and further modulate the colors by light.
		surfcolor.s.red = surfcolor.s.red+((UINT8)(CALCLIGHT((0xFF-fogalpha),(255-light))+CALCLIGHT(fogcolor.s.red,(255-light))));
		surfcolor.s.green = surfcolor.s.green+((UINT8)(CALCLIGHT((0xFF-fogalpha),(255-light))+CALCLIGHT(fogcolor.s.green,(255-light))));
		surfcolor.s.blue = surfcolor.s.blue+((UINT8)(CALCLIGHT((0xFF-fogalpha),(255-light))+CALCLIGHT(fogcolor.s.blue,(255-light))));
		surfcolor.s.alpha = 0xFF;
	}

	if(cv_grfog.value)
	{
		if (cv_grsoftwarefog.value)
		{
			fogcolor.s.red = (UINT8)((CALCLIGHT(fogcolor.s.red,(255-light)))+(CALCLIGHT(realcolor.s.red,light)));
			fogcolor.s.green = (UINT8)((CALCLIGHT(fogcolor.s.green,(255-light)))+(CALCLIGHT(realcolor.s.green,light)));
			fogcolor.s.blue = (UINT8)((CALCLIGHT(fogcolor.s.blue,(255-light)))+(CALCLIGHT(realcolor.s.blue,light)));

			// Set the fog options.
			if (cv_grsoftwarefog.value == 1 && plane) // With floors, software draws them way darker for their distance
				HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, (INT32)(CALCFOGDENSITYFLOOR(light)));
			else // everything else is drawn like walls
				HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, (INT32)(CALCFOGDENSITY(light)));
		}
		else
		{
			fogcolor.s.red = (UINT8)((CALCLIGHT(fogcolor.s.red,(255-light)))+(CALCLIGHT(realcolor.s.red,light)));
			fogcolor.s.green = (UINT8)((CALCLIGHT(fogcolor.s.green,(255-light)))+(CALCLIGHT(realcolor.s.green,light)));
			fogcolor.s.blue = (UINT8)((CALCLIGHT(fogcolor.s.blue,(255-light)))+(CALCLIGHT(realcolor.s.blue,light)));

			fogalpha = (UINT8)((CALCLIGHT(fogalpha,(255-light)))+(CALCLIGHT(alpha,light)));

			// Set the fog options.
			light = (UINT8)(CALCLIGHT(light,(255-fogalpha)));
			HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, (INT32)(cv_grfogdensity.value-(cv_grfogdensity.value*(float)light/255.0f)));
		}

		HWD.pfnSetSpecialState(HWD_SET_FOG_COLOR, (fogcolor.s.red*0x10000)+(fogcolor.s.green*0x100)+fogcolor.s.blue);
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 1);
	}
	return surfcolor.rgba;
}


static UINT8 HWR_FogBlockAlpha(INT32 light, UINT32 color) // Let's see if this can work
{
	RGBA_t realcolor, surfcolor;
	INT32 alpha;

	// Don't go out of bounds
	if (light < 0)
		light = 0;
	else if (light > 255)
		light = 255;

	realcolor.rgba = color;

	alpha = (realcolor.s.alpha*255)/25;

	// at 255 brightness, alpha is between 0 and 127, at 0 brightness alpha will always be 255
	surfcolor.s.alpha = (alpha*light)/(2*256)+255-light;

	return surfcolor.s.alpha;
}

// ==========================================================================
//                                   FLOOR/CEILING GENERATION FROM SUBSECTORS
// ==========================================================================

#ifdef DOPLANES

// -----------------+
// HWR_RenderPlane  : Render a floor or ceiling convex polygon
// -----------------+
static void HWR_RenderPlane(sector_t *sector, extrasubsector_t *xsub, boolean isceiling, fixed_t fixedheight,
                           FBITFIELD PolyFlags, INT32 lightlevel, lumpnum_t lumpnum, sector_t *FOFsector, UINT8 alpha, boolean fogplane, extracolormap_t *planecolormap)
{
	polyvertex_t *  pv;
	float           height; //constant y for all points on the convex flat polygon
	FOutVector      *v3d;
	INT32             nrPlaneVerts;   //verts original define of convex flat polygon
	INT32             i;
	float           flatxref,flatyref;
	float fflatsize;
	INT32 flatflag;
	size_t len;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;
	FSurfaceInfo    Surf;
	fixed_t tempxsow, tempytow;
#ifdef ESLOPE
	pslope_t *slope = NULL;
#endif

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	(void)sector; ///@TODO remove shitty unused variable

	// no convex poly were generated for this subsector
	if (!xsub->planepoly)
		return;

#ifdef ESLOPE
	// Get the slope pointer to simplify future code
	if (FOFsector)
	{
		if (FOFsector->f_slope && !isceiling)
			slope = FOFsector->f_slope;
		else if (FOFsector->c_slope && isceiling)
			slope = FOFsector->c_slope;
	}
	else
	{
		if (gr_frontsector->f_slope && !isceiling)
			slope = gr_frontsector->f_slope;
		else if (gr_frontsector->c_slope && isceiling)
			slope = gr_frontsector->c_slope;
	}

	// Set fixedheight to the slope's height from our viewpoint, if we have a slope
	if (slope)
		fixedheight = P_GetZAt(slope, viewx, viewy);
#endif

	height = FIXED_TO_FLOAT(fixedheight);

	pv  = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3)   //not even a triangle ?
		return;

	if (nrPlaneVerts > UINT16_MAX) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %d exceeds max value of %d vertices\n", nrPlaneVerts, UINT16_MAX);
		return;
	}

	// Allocate plane-vertex buffer if we need to
	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}

	len = W_LumpLength(lumpnum);

	switch (len)
	{
		case 4194304: // 2048x2048 lump
			fflatsize = 2048.0f;
			flatflag = 2047;
			break;
		case 1048576: // 1024x1024 lump
			fflatsize = 1024.0f;
			flatflag = 1023;
			break;
		case 262144:// 512x512 lump
			fflatsize = 512.0f;
			flatflag = 511;
			break;
		case 65536: // 256x256 lump
			fflatsize = 256.0f;
			flatflag = 255;
			break;
		case 16384: // 128x128 lump
			fflatsize = 128.0f;
			flatflag = 127;
			break;
		case 1024: // 32x32 lump
			fflatsize = 32.0f;
			flatflag = 31;
			break;
		default: // 64x64 lump
			fflatsize = 64.0f;
			flatflag = 63;
			break;
	}

	// reference point for flat texture coord for each vertex around the polygon
	flatxref = (float)(((fixed_t)pv->x & (~flatflag)) / fflatsize);
	flatyref = (float)(((fixed_t)pv->y & (~flatflag)) / fflatsize);

	// transform
	v3d = planeVerts;

	if (FOFsector != NULL)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatsize;
			angle = FOFsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatsize;
			angle = FOFsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}
	else if (gr_frontsector)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatsize;
			angle = gr_frontsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatsize;
			angle = gr_frontsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}

	if (angle) // Only needs to be done if there's an altered angle
	{

		// This needs to be done so that it scrolls in a different direction after rotation like software
		tempxsow = FLOAT_TO_FIXED(scrollx);
		tempytow = FLOAT_TO_FIXED(scrolly);
		scrollx = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		scrolly = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));

		// This needs to be done so everything aligns after rotation
		// It would be done so that rotation is done, THEN the translation, but I couldn't get it to rotate AND scroll like software does
		tempxsow = FLOAT_TO_FIXED(flatxref);
		tempytow = FLOAT_TO_FIXED(flatyref);
		flatxref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		flatyref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));
	}

	for (i = 0; i < nrPlaneVerts; i++,v3d++,pv++)
	{
		// Hurdler: add scrolling texture on floor/ceiling
		v3d->sow = (float)((pv->x / fflatsize) - flatxref + scrollx);
		v3d->tow = (float)(flatyref - (pv->y / fflatsize) + scrolly);

		//v3d->sow = (float)(pv->x / fflatsize);
		//v3d->tow = (float)(pv->y / fflatsize);

		// Need to rotate before translate
		if (angle) // Only needs to be done if there's an altered angle
		{
			tempxsow = FLOAT_TO_FIXED(v3d->sow);
			tempytow = FLOAT_TO_FIXED(v3d->tow);
			v3d->sow = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
			v3d->tow = (FIXED_TO_FLOAT(-FixedMul(tempxsow, FINESINE(angle)) - FixedMul(tempytow, FINECOSINE(angle))));
		}

		//v3d->sow = (float)(v3d->sow - flatxref + scrollx);
		//v3d->tow = (float)(flatyref - v3d->tow + scrolly);

		v3d->x = pv->x;
		v3d->y = height;
		v3d->z = pv->y;

#ifdef ESLOPE
		if (slope)
		{
			fixedheight = P_GetZAt(slope, FLOAT_TO_FIXED(pv->x), FLOAT_TO_FIXED(pv->y));
			v3d->y = FIXED_TO_FLOAT(fixedheight);
		}
#endif
	}

	// only useful for flat coloured triangles
	//Surf.FlatColor = 0xff804020;

	// use different light tables
	// for horizontal / vertical / diagonal
	// note: try to get the same visual feel as the original
	Surf.FlatColor.s.red = Surf.FlatColor.s.green =
	Surf.FlatColor.s.blue = LightLevelToLum(lightlevel); //  Don't take from the frontsector, or the game will crash

#if 0 // no colormap test
	// colormap test
	if (gr_frontsector)
	{
		sector_t *psector = gr_frontsector;

#ifdef ESLOPE
		if (slope)
			fixedheight = P_GetZAt(slope, psector->soundorg.x, psector->soundorg.y);
#endif

		if (psector->ffloors)
		{
			ffloor_t *caster = psector->lightlist[R_GetPlaneLight(psector, fixedheight, false)].caster;
			psector = caster ? &sectors[caster->secnum] : psector;

			if (caster)
			{
				lightlevel = psector->lightlevel;
				Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = LightLevelToLum(lightlevel);
			}
		}
		if (psector->extra_colormap)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel,psector->extra_colormap->rgba,psector->extra_colormap->fadergba, false, true);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel,NORMALFOG,FADEFOG, false, true);
	}
	else
		Surf.FlatColor.rgba = HWR_Lighting(lightlevel,NORMALFOG,FADEFOG, false, true);

#endif // NOPE

	if (planecolormap)
	{
		if (fogplane)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, planecolormap->rgba, planecolormap->fadergba, true, false);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, planecolormap->rgba, planecolormap->fadergba, false, true);
	}
	else
	{
		if (fogplane)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, true, false);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, true);
	}

	if (PolyFlags & (PF_Translucent|PF_Fog))
	{
		Surf.FlatColor.s.alpha = (UINT8)alpha;
		PolyFlags |= PF_Modulated|PF_Clip;
	}
	else
		PolyFlags |= PF_Masked|PF_Modulated|PF_Clip;

	HWD.pfnDrawPolygon(&Surf, planeVerts, nrPlaneVerts, PolyFlags);

#ifdef ALAM_LIGHTING
	// add here code for dynamic lighting on planes
	HWR_PlaneLighting(planeVerts, nrPlaneVerts);
#endif
}

#ifdef POLYSKY
// this don't draw anything it only update the z-buffer so there isn't problem with
// wall/things upper that sky (map12)
static void HWR_RenderSkyPlane(extrasubsector_t *xsub, fixed_t fixedheight)
{
	polyvertex_t *pv;
	float height; //constant y for all points on the convex flat polygon
	FOutVector *v3d;
	INT32 nrPlaneVerts;   //verts original define of convex flat polygon
	INT32 i;

	// no convex poly were generated for this subsector
	if (!xsub->planepoly)
		return;

	height = FIXED_TO_FLOAT(fixedheight);

	pv  = xsub->planepoly->pts;
	nrPlaneVerts = xsub->planepoly->numpts;

	if (nrPlaneVerts < 3) // not even a triangle?
		return;

	if (nrPlaneVerts > MAXPLANEVERTICES) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %d exceeds max value of %d vertices\n", nrPlaneVerts, MAXPLANEVERTICES);
		return;
	}

	// transform
	v3d = planeVerts;
	for (i = 0; i < nrPlaneVerts; i++,v3d++,pv++)
	{
		v3d->sow = 0.0f;
		v3d->tow = 0.0f;
		v3d->x = pv->x;
		v3d->y = height;
		v3d->z = pv->y;
	}

	HWD.pfnDrawPolygon(NULL, planeVerts, nrPlaneVerts,
	 PF_Clip|PF_Invisible|PF_NoTexture|PF_Occlude);
}
#endif //polysky

#endif //doplanes

/*
   wallVerts order is :
		3--2
		| /|
		|/ |
		0--1
*/
#ifdef WALLSPLATS
static void HWR_DrawSegsSplats(FSurfaceInfo * pSurf)
{
	FOutVector trVerts[4], *wv;
	wallVert3D wallVerts[4];
	wallVert3D *pwallVerts;
	wallsplat_t *splat;
	GLPatch_t *gpatch;
	fixed_t i;
	FSurfaceInfo pSurf2;
	// seg bbox
	fixed_t segbbox[4];

	M_ClearBox(segbbox);
	M_AddToBox(segbbox,
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v1)->x),
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v1)->y));
	M_AddToBox(segbbox,
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v2)->x),
		FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v2)->y));

	splat = (wallsplat_t *)gr_curline->linedef->splats;
	for (; splat; splat = splat->next)
	{
		//BP: don't draw splat extern to this seg
		//    this is quick fix best is explain in logboris.txt at 12-4-2000
		if (!M_PointInBox(segbbox,splat->v1.x,splat->v1.y) && !M_PointInBox(segbbox,splat->v2.x,splat->v2.y))
			continue;

		gpatch = W_CachePatchNum(splat->patch, PU_CACHE);
		HWR_GetPatch(gpatch);

		wallVerts[0].x = wallVerts[3].x = FIXED_TO_FLOAT(splat->v1.x);
		wallVerts[0].z = wallVerts[3].z = FIXED_TO_FLOAT(splat->v1.y);
		wallVerts[2].x = wallVerts[1].x = FIXED_TO_FLOAT(splat->v2.x);
		wallVerts[2].z = wallVerts[1].z = FIXED_TO_FLOAT(splat->v2.y);

		i = splat->top;
		if (splat->yoffset)
			i += *splat->yoffset;

		wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(i)+(gpatch->height>>1);
		wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(i)-(gpatch->height>>1);

		wallVerts[3].s = wallVerts[3].t = wallVerts[2].s = wallVerts[0].t = 0.0f;
		wallVerts[1].s = wallVerts[1].t = wallVerts[2].t = wallVerts[0].s = 1.0f;

		// transform
		wv = trVerts;
		pwallVerts = wallVerts;
		for (i = 0; i < 4; i++,wv++,pwallVerts++)
		{
			wv->x   = pwallVerts->x;
			wv->z = pwallVerts->z;
			wv->y   = pwallVerts->y;

			// Kalaron: TOW and SOW needed to be switched
			wv->sow = pwallVerts->t;
			wv->tow = pwallVerts->s;
		}
		M_Memcpy(&pSurf2,pSurf,sizeof (FSurfaceInfo));
		switch (splat->flags & SPLATDRAWMODE_MASK)
		{
			case SPLATDRAWMODE_OPAQUE :
				pSurf2.FlatColor.s.alpha = 0xff;
				i = PF_Translucent;
				break;
			case SPLATDRAWMODE_TRANS :
				pSurf2.FlatColor.s.alpha = 128;
				i = PF_Translucent;
				break;
			case SPLATDRAWMODE_SHADE :
				pSurf2.FlatColor.s.alpha = 0xff;
				i = PF_Substractive;
				break;
		}

		HWD.pfnDrawPolygon(&pSurf2, trVerts, 4, i|PF_Modulated|PF_Clip|PF_Decal);
	}
}
#endif

// ==========================================================================
//                                        WALL GENERATION FROM SUBSECTOR SEGS
// ==========================================================================


FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf)
{
	switch (transtablenum)
	{
		case tr_trans10 : pSurf->FlatColor.s.alpha = 0xe6;return  PF_Translucent;
		case tr_trans20 : pSurf->FlatColor.s.alpha = 0xcc;return  PF_Translucent;
		case tr_trans30 : pSurf->FlatColor.s.alpha = 0xb3;return  PF_Translucent;
		case tr_trans40 : pSurf->FlatColor.s.alpha = 0x99;return  PF_Translucent;
		case tr_trans50 : pSurf->FlatColor.s.alpha = 0x80;return  PF_Translucent;
		case tr_trans60 : pSurf->FlatColor.s.alpha = 0x66;return  PF_Translucent;
		case tr_trans70 : pSurf->FlatColor.s.alpha = 0x4c;return  PF_Translucent;
		case tr_trans80 : pSurf->FlatColor.s.alpha = 0x33;return  PF_Translucent;
		case tr_trans90 : pSurf->FlatColor.s.alpha = 0x19;return  PF_Translucent;
	}
	return PF_Translucent;
}

// v1,v2 : the start & end vertices along the original wall segment, that may have been
//         clipped so that only a visible portion of the wall seg is drawn.
// floorheight, ceilingheight : depend on wall upper/lower/middle, comes from the sectors.

static void HWR_AddTransparentWall(wallVert3D *wallVerts, FSurfaceInfo * pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap);

// -----------------+
// HWR_ProjectWall  :
// -----------------+
/*
   wallVerts order is :
		3--2
		| /|
		|/ |
		0--1
*/
static void HWR_ProjectWall(wallVert3D   * wallVerts,
                                    FSurfaceInfo * pSurf,
                                    FBITFIELD blendmode, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	FOutVector  trVerts[4];
	FOutVector  *wv;

	// transform
	wv = trVerts;
	// it sounds really stupid to do this conversion with the new T&L code
	// we should directly put the right information in the right structure
	// wallVerts3D seems ok, doesn't need FOutVector
	// also remove the light copy

	// More messy to unwrap, but it's also quicker, uses less memory.
	wv->sow = wallVerts->s;
	wv->tow = wallVerts->t;
	wv->x   = wallVerts->x;
	wv->y   = wallVerts->y;
	wv->z   = wallVerts->z;
	wv++; wallVerts++;
	wv->sow = wallVerts->s;
	wv->tow = wallVerts->t;
	wv->x   = wallVerts->x;
	wv->y   = wallVerts->y;
	wv->z   = wallVerts->z;
	wv++; wallVerts++;
	wv->sow = wallVerts->s;
	wv->tow = wallVerts->t;
	wv->x   = wallVerts->x;
	wv->y   = wallVerts->y;
	wv->z   = wallVerts->z;
	wv++; wallVerts++;
	wv->sow = wallVerts->s;
	wv->tow = wallVerts->t;
	wv->x   = wallVerts->x;
	wv->y   = wallVerts->y;
	wv->z   = wallVerts->z;

	if (wallcolormap)
	{
		pSurf->FlatColor.rgba = HWR_Lighting(lightlevel, wallcolormap->rgba, wallcolormap->fadergba, false, false);
	}
	else
	{
		pSurf->FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);
	}

	HWD.pfnDrawPolygon(pSurf, trVerts, 4, blendmode|PF_Modulated|PF_Occlude|PF_Clip);

#ifdef WALLSPLATS
	if (gr_curline->linedef->splats && cv_splats.value)
		HWR_DrawSegsSplats(pSurf);
#endif
#ifdef ALAM_LIGHTING
	//Hurdler: TDOD: do static lighting using gr_curline->lm
	HWR_WallLighting(trVerts);

	//Hurdler: for better dynamic light in dark area, we should draw the light first
	//         and then the wall all that with the right blending func
	//HWD.pfnDrawPolygon(pSurf, trVerts, 4, PF_Additive|PF_Modulated|PF_Occlude|PF_Clip);
#endif
}

// ==========================================================================
//                                                          BSP, CULL, ETC..
// ==========================================================================

// return the frac from the interception of the clipping line
// (in fact a clipping plane that has a constant, so can clip with simple 2d)
// with the wall segment
//
static float HWR_ClipViewSegment(INT32 x, polyvertex_t *v1, polyvertex_t *v2)
{
	float num, den;
	float v1x, v1y, v1dx, v1dy, v2dx, v2dy;
	angle_t pclipangle = gr_xtoviewangle[x];

	// a segment of a polygon
	v1x  = v1->x;
	v1y  = v1->y;
	v1dx = (v2->x - v1->x);
	v1dy = (v2->y - v1->y);

	// the clipping line
	pclipangle = pclipangle + dup_viewangle; //back to normal angle (non-relative)
	v2dx = FIXED_TO_FLOAT(FINECOSINE(pclipangle>>ANGLETOFINESHIFT));
	v2dy = FIXED_TO_FLOAT(FINESINE(pclipangle>>ANGLETOFINESHIFT));

	den = v2dy*v1dx - v2dx*v1dy;
	if (den == 0)
		return -1; // parallel

	// calc the frac along the polygon segment,
	//num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
	//num = -v1x * v2dy + v1y * v2dx;
	num = (gr_viewx - v1x)*v2dy + (v1y - gr_viewy)*v2dx;

	return num / den;
}

//
// HWR_SplitWall
//
static void HWR_SplitWall(sector_t *sector, wallVert3D *wallVerts, INT32 texnum, FSurfaceInfo* Surf, UINT32 cutflag, ffloor_t *pfloor)
{
	/* SoM: split up and light walls according to the
	 lightlist. This may also include leaving out parts
	 of the wall that can't be seen */

	float realtop, realbot, top, bot;
	float pegt, pegb, pegmul;
	float height = 0.0f, bheight = 0.0f;

#ifdef ESLOPE
	float endrealtop, endrealbot, endtop, endbot;
	float endpegt, endpegb, endpegmul;
	float endheight = 0.0f, endbheight = 0.0f;

	fixed_t v1x = FLOAT_TO_FIXED(wallVerts[0].x);
	fixed_t v1y = FLOAT_TO_FIXED(wallVerts[0].z); // not a typo
	fixed_t v2x = FLOAT_TO_FIXED(wallVerts[1].x);
	fixed_t v2y = FLOAT_TO_FIXED(wallVerts[1].z); // not a typo
	// compiler complains when P_GetZAt is used in FLOAT_TO_FIXED directly
	// use this as a temp var to store P_GetZAt's return value each time
	fixed_t temp;
#endif

	INT32   solid, i;
	lightlist_t *  list = sector->lightlist;
	const UINT8 alpha = Surf->FlatColor.s.alpha;
	FUINT lightnum = sector->lightlevel;
	extracolormap_t *colormap = NULL;

	realtop = top = wallVerts[3].y;
	realbot = bot = wallVerts[0].y;
	pegt = wallVerts[3].t;
	pegb = wallVerts[0].t;
	pegmul = (pegb - pegt) / (top - bot);

#ifdef ESLOPE
	endrealtop = endtop = wallVerts[2].y;
	endrealbot = endbot = wallVerts[1].y;
	endpegt = wallVerts[2].t;
	endpegb = wallVerts[1].t;
	endpegmul = (endpegb - endpegt) / (endtop - endbot);
#endif

	for (i = 0; i < sector->numlights; i++)
	{
#ifdef ESLOPE
        if (endtop < endrealbot)
#endif
		if (top < realbot)
			return;

	// There's a compiler warning here if this comment isn't here because of indentation
		if (!(list[i].flags & FF_NOSHADE))
		{
			if (pfloor && (pfloor->flags & FF_FOG))
			{
				lightnum = pfloor->master->frontsector->lightlevel;
				colormap = pfloor->master->frontsector->extra_colormap;
			}
			else
			{
				lightnum = *list[i].lightlevel;
				colormap = list[i].extra_colormap;
			}
		}

		solid = false;

		if ((sector->lightlist[i].flags & FF_CUTSOLIDS) && !(cutflag & FF_EXTRA))
			solid = true;
		else if ((sector->lightlist[i].flags & FF_CUTEXTRA) && (cutflag & FF_EXTRA))
		{
			if (sector->lightlist[i].flags & FF_EXTRA)
			{
				if ((sector->lightlist[i].flags & (FF_FOG|FF_SWIMMABLE)) == (cutflag & (FF_FOG|FF_SWIMMABLE))) // Only merge with your own types
					solid = true;
			}
			else
				solid = true;
		}
		else
			solid = false;

#ifdef ESLOPE
		if (list[i].slope)
		{
			temp = P_GetZAt(list[i].slope, v1x, v1y);
			height = FIXED_TO_FLOAT(temp);
			temp = P_GetZAt(list[i].slope, v2x, v2y);
			endheight = FIXED_TO_FLOAT(temp);
		}
		else
			height = endheight = FIXED_TO_FLOAT(list[i].height);
		if (solid)
		{
			if (*list[i].caster->b_slope)
			{
				temp = P_GetZAt(*list[i].caster->b_slope, v1x, v1y);
				bheight = FIXED_TO_FLOAT(temp);
				temp = P_GetZAt(*list[i].caster->b_slope, v2x, v2y);
				endbheight = FIXED_TO_FLOAT(temp);
			}
			else
				bheight = endbheight = FIXED_TO_FLOAT(*list[i].caster->bottomheight);
		}
#else
		height = FIXED_TO_FLOAT(list[i].height);
		if (solid)
			bheight = FIXED_TO_FLOAT(*list[i].caster->bottomheight);
#endif

#ifdef ESLOPE
		if (endheight >= endtop)
#endif
		if (height >= top)
		{
			if (solid && top > bheight)
				top = bheight;
#ifdef ESLOPE
			if (solid && endtop > endbheight)
				endtop = endbheight;
#endif
		}

#ifdef ESLOPE
		if (i + 1 < sector->numlights)
		{
			if (list[i+1].slope)
			{
				temp = P_GetZAt(list[i+1].slope, v1x, v1y);
				bheight = FIXED_TO_FLOAT(temp);
				temp = P_GetZAt(list[i+1].slope, v2x, v2y);
				endbheight = FIXED_TO_FLOAT(temp);
			}
			else
				bheight = endbheight = FIXED_TO_FLOAT(list[i+1].height);
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}
#else
		if (i + 1 < sector->numlights)
		{
			bheight = FIXED_TO_FLOAT(list[i+1].height);
		}
		else
		{
			bheight = realbot;
		}
#endif

#ifdef ESLOPE
		if (endbheight >= endtop)
#endif
		if (bheight >= top)
			continue;

		//Found a break;
		bot = bheight;

		if (bot < realbot)
			bot = realbot;

#ifdef ESLOPE
		endbot = endbheight;

		if (endbot < endrealbot)
			endbot = endrealbot;
#endif
		Surf->FlatColor.s.alpha = alpha;

#ifdef ESLOPE
		wallVerts[3].t = pegt + ((realtop - top) * pegmul);
		wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
		wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
		wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);

		// set top/bottom coords
		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;
#else
		wallVerts[3].t = wallVerts[2].t = pegt + ((realtop - top) * pegmul);
		wallVerts[0].t = wallVerts[1].t = pegt + ((realtop - bot) * pegmul);

		// set top/bottom coords
		wallVerts[2].y = wallVerts[3].y = top;
		wallVerts[0].y = wallVerts[1].y = bot;
#endif

		if (cutflag & FF_FOG)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture, true, lightnum, colormap);
		else if (cutflag & FF_TRANSLUCENT)
			HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Translucent, false, lightnum, colormap);
		else
			HWR_ProjectWall(wallVerts, Surf, PF_Masked, lightnum, colormap);

		top = bot;
#ifdef ESLOPE
		endtop = endbot;
#endif
	}

	bot = realbot;
#ifdef ESLOPE
	endbot = endrealbot;
	if (endtop <= endrealbot)
#endif
	if (top <= realbot)
		return;

	Surf->FlatColor.s.alpha = alpha;

#ifdef ESLOPE
	wallVerts[3].t = pegt + ((realtop - top) * pegmul);
	wallVerts[2].t = endpegt + ((endrealtop - endtop) * endpegmul);
	wallVerts[0].t = pegt + ((realtop - bot) * pegmul);
	wallVerts[1].t = endpegt + ((endrealtop - endbot) * endpegmul);

	// set top/bottom coords
	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;
#else
    wallVerts[3].t = wallVerts[2].t = pegt + ((realtop - top) * pegmul);
    wallVerts[0].t = wallVerts[1].t = pegt + ((realtop - bot) * pegmul);

    // set top/bottom coords
    wallVerts[2].y = wallVerts[3].y = top;
    wallVerts[0].y = wallVerts[1].y = bot;
#endif

	if (cutflag & FF_FOG)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Fog|PF_NoTexture, true, lightnum, colormap);
	else if (cutflag & FF_TRANSLUCENT)
		HWR_AddTransparentWall(wallVerts, Surf, texnum, PF_Translucent, false, lightnum, colormap);
	else
		HWR_ProjectWall(wallVerts, Surf, PF_Masked, lightnum, colormap);
}

// HWR_DrawSkyWalls
// Draw walls into the depth buffer so that anything behind is culled properly
static void HWR_DrawSkyWall(wallVert3D *wallVerts, FSurfaceInfo *Surf, fixed_t bottom, fixed_t top)
{
	HWD.pfnSetTexture(NULL);
	// no texture
	wallVerts[3].t = wallVerts[2].t = 0;
	wallVerts[0].t = wallVerts[1].t = 0;
	wallVerts[0].s = wallVerts[3].s = 0;
	wallVerts[2].s = wallVerts[1].s = 0;
	// set top/bottom coords
	wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(top); // No real way to find the correct height of this
	wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(bottom); // worldlow/bottom because it needs to cover up the lower thok barrier wall
	HWR_ProjectWall(wallVerts, Surf, PF_Invisible|PF_Clip|PF_NoTexture, 255, NULL);
	// PF_Invisible so it's not drawn into the colour buffer
	// PF_NoTexture for no texture
	// PF_Occlude is set in HWR_ProjectWall to draw into the depth buffer
}

//
// HWR_StoreWallRange
// A portion or all of a wall segment will be drawn, from startfrac to endfrac,
//  where 0 is the start of the segment, 1 the end of the segment
// Anything between means the wall segment has been clipped with solidsegs,
//  reducing wall overdraw to a minimum
//
static void HWR_StoreWallRange(double startfrac, double endfrac)
{
	wallVert3D wallVerts[4];
	v2d_t vs, ve; // start, end vertices of 2d line (view from above)

	fixed_t worldtop, worldbottom;
	fixed_t worldhigh = 0, worldlow = 0;
#ifdef ESLOPE
	fixed_t worldtopslope, worldbottomslope;
	fixed_t worldhighslope = 0, worldlowslope = 0;
	fixed_t v1x, v1y, v2x, v2y;
#endif

	GLTexture_t *grTex = NULL;
	float cliplow = 0.0f, cliphigh = 0.0f;
	INT32 gr_midtexture;
	fixed_t h, l; // 3D sides and 2s middle textures
#ifdef ESLOPE
	fixed_t hS, lS;
#endif

	FUINT lightnum = 0; // shut up compiler
	extracolormap_t *colormap;
	FSurfaceInfo Surf;

	if (startfrac > endfrac)
		return;

	gr_sidedef = gr_curline->sidedef;
	gr_linedef = gr_curline->linedef;

	vs.x = ((polyvertex_t *)gr_curline->v1)->x;
	vs.y = ((polyvertex_t *)gr_curline->v1)->y;
	ve.x = ((polyvertex_t *)gr_curline->v2)->x;
	ve.y = ((polyvertex_t *)gr_curline->v2)->y;

#ifdef ESLOPE
	v1x = FLOAT_TO_FIXED(vs.x);
	v1y = FLOAT_TO_FIXED(vs.y);
	v2x = FLOAT_TO_FIXED(ve.x);
	v2y = FLOAT_TO_FIXED(ve.y);
#endif

	if (gr_frontsector->heightsec != -1)
	{
#ifdef ESLOPE
		worldtop = worldtopslope = sectors[gr_frontsector->heightsec].ceilingheight;
		worldbottom = worldbottomslope = sectors[gr_frontsector->heightsec].floorheight;
#else
		worldtop = sectors[gr_frontsector->heightsec].ceilingheight;
		worldbottom = sectors[gr_frontsector->heightsec].floorheight;
#endif
	}
	else
	{
#ifdef ESLOPE
		if (gr_frontsector->c_slope)
		{
			worldtop      = P_GetZAt(gr_frontsector->c_slope, v1x, v1y);
			worldtopslope = P_GetZAt(gr_frontsector->c_slope, v2x, v2y);
		}
		else
		{
			worldtop = worldtopslope = gr_frontsector->ceilingheight;
		}

		if (gr_frontsector->f_slope)
		{
			worldbottom      = P_GetZAt(gr_frontsector->f_slope, v1x, v1y);
			worldbottomslope = P_GetZAt(gr_frontsector->f_slope, v2x, v2y);
		}
		else
		{
			worldbottom = worldbottomslope = gr_frontsector->floorheight;
		}
#else
		worldtop    = gr_frontsector->ceilingheight;
		worldbottom = gr_frontsector->floorheight;
#endif
	}

	// remember vertices ordering
	//  3--2
	//  | /|
	//  |/ |
	//  0--1
	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].x = wallVerts[3].x = vs.x;
	wallVerts[0].z = wallVerts[3].z = vs.y;
	wallVerts[2].x = wallVerts[1].x = ve.x;
	wallVerts[2].z = wallVerts[1].z = ve.y;
	wallVerts[0].w = wallVerts[1].w = wallVerts[2].w = wallVerts[3].w = 1.0f;

	if (drawtextured)
	{
		// x offset the texture
		fixed_t texturehpeg = gr_sidedef->textureoffset + gr_curline->offset;

		// clip texture s start/end coords with solidsegs
		if (startfrac > 0.0f && startfrac < 1.0f)
			cliplow = (float)(texturehpeg + (gr_curline->flength*FRACUNIT) * startfrac);
		else
			cliplow = (float)texturehpeg;

		if (endfrac > 0.0f && endfrac < 1.0f)
			cliphigh = (float)(texturehpeg + (gr_curline->flength*FRACUNIT) * endfrac);
		else
			cliphigh = (float)(texturehpeg + (gr_curline->flength*FRACUNIT));
	}

	lightnum = gr_frontsector->lightlevel;
	colormap = gr_frontsector->extra_colormap;

	if (gr_frontsector)
	{
		Surf.FlatColor.s.alpha = 255;
	}

	if (gr_backsector)
	{
		INT32 gr_toptexture, gr_bottomtexture;
		// two sided line
		if (gr_backsector->heightsec != -1)
		{
#ifdef ESLOPE
			worldhigh = worldhighslope = sectors[gr_backsector->heightsec].ceilingheight;
			worldlow = worldlowslope = sectors[gr_backsector->heightsec].floorheight;
#else
			worldhigh = sectors[gr_backsector->heightsec].ceilingheight;
			worldlow = sectors[gr_backsector->heightsec].floorheight;
#endif
		}
		else
		{
#ifdef ESLOPE
			if (gr_backsector->c_slope)
			{
				worldhigh      = P_GetZAt(gr_backsector->c_slope, v1x, v1y);
				worldhighslope = P_GetZAt(gr_backsector->c_slope, v2x, v2y);
			}
			else
			{
				worldhigh = worldhighslope = gr_backsector->ceilingheight;
			}

			if (gr_backsector->f_slope)
			{
				worldlow      = P_GetZAt(gr_backsector->f_slope, v1x, v1y);
				worldlowslope = P_GetZAt(gr_backsector->f_slope, v2x, v2y);
			}
			else
			{
				worldlow = worldlowslope = gr_backsector->floorheight;
			}
#else
			worldhigh = gr_backsector->ceilingheight;
			worldlow  = gr_backsector->floorheight;
#endif
		}

		// hack to allow height changes in outdoor areas
		// This is what gets rid of the upper textures if there should be sky
		if (gr_frontsector->ceilingpic == skyflatnum &&
			gr_backsector->ceilingpic  == skyflatnum)
		{
			worldtop = worldhigh;
#ifdef ESLOPE
			worldtopslope = worldhighslope;
#endif
		}

		gr_toptexture = R_GetTextureNum(gr_sidedef->toptexture);
		gr_bottomtexture = R_GetTextureNum(gr_sidedef->bottomtexture);

		// check TOP TEXTURE
		if ((
#ifdef ESLOPE
			worldhighslope < worldtopslope ||
#endif
            worldhigh < worldtop
            ) && gr_toptexture)
		{
			if (drawtextured)
			{
				fixed_t texturevpegtop; // top

				grTex = HWR_GetTexture(gr_toptexture);

				// PEGGING
				if (gr_linedef->flags & ML_DONTPEGTOP)
					texturevpegtop = 0;
#ifdef ESLOPE
				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegtop = worldhigh + textureheight[gr_sidedef->toptexture] - worldtop;
				else
					texturevpegtop = gr_backsector->ceilingheight + textureheight[gr_sidedef->toptexture] - gr_frontsector->ceilingheight;
#else
                else
                    texturevpegtop = worldhigh + textureheight[gr_sidedef->toptexture] - worldtop;
#endif

				texturevpegtop += gr_sidedef->rowoffset;

				// This is so that it doesn't overflow and screw up the wall, it doesn't need to go higher than the texture's height anyway
				texturevpegtop %= SHORT(textures[gr_toptexture]->height)<<FRACBITS;

				wallVerts[3].t = wallVerts[2].t = texturevpegtop * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpegtop + gr_frontsector->ceilingheight - gr_backsector->ceilingheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;

#ifdef ESLOPE
				// Adjust t value for sloped walls
				if (!(gr_linedef->flags & ML_EFFECT1))
				{
					// Unskewed
					wallVerts[3].t -= (worldtop - gr_frontsector->ceilingheight) * grTex->scaleY;
					wallVerts[2].t -= (worldtopslope - gr_frontsector->ceilingheight) * grTex->scaleY;
					wallVerts[0].t -= (worldhigh - gr_backsector->ceilingheight) * grTex->scaleY;
					wallVerts[1].t -= (worldhighslope - gr_backsector->ceilingheight) * grTex->scaleY;
				}
				else if (gr_linedef->flags & ML_DONTPEGTOP)
				{
					// Skewed by top
					wallVerts[0].t = (texturevpegtop + worldtop - worldhigh) * grTex->scaleY;
					wallVerts[1].t = (texturevpegtop + worldtopslope - worldhighslope) * grTex->scaleY;
				}
				else
				{
					// Skewed by bottom
					wallVerts[0].t = wallVerts[1].t = (texturevpegtop + worldtop - worldhigh) * grTex->scaleY;
					wallVerts[3].t = wallVerts[0].t - (worldtop - worldhigh) * grTex->scaleY;
					wallVerts[2].t = wallVerts[1].t - (worldtopslope - worldhighslope) * grTex->scaleY;
				}
#endif
			}

			// set top/bottom coords
#ifdef ESLOPE
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldhigh);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldhighslope);
#else
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldhigh);
#endif

			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_toptexture, &Surf, FF_CUTLEVEL, NULL);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gr_toptexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}

		// check BOTTOM TEXTURE
		if ((
#ifdef ESLOPE
			worldlowslope > worldbottomslope ||
#endif
            worldlow > worldbottom) && gr_bottomtexture) //only if VISIBLE!!!
		{
			if (drawtextured)
			{
				fixed_t texturevpegbottom = 0; // bottom

				grTex = HWR_GetTexture(gr_bottomtexture);

				// PEGGING
#ifdef ESLOPE
				if (!(gr_linedef->flags & ML_DONTPEGBOTTOM))
					texturevpegbottom = 0;
				else if (gr_linedef->flags & ML_EFFECT1)
					texturevpegbottom = worldbottom - worldlow;
				else
					texturevpegbottom = gr_frontsector->floorheight - gr_backsector->floorheight;
#else
				if (gr_linedef->flags & ML_DONTPEGBOTTOM)
					texturevpegbottom = worldbottom - worldlow;
                else
                    texturevpegbottom = 0;
#endif

				texturevpegbottom += gr_sidedef->rowoffset;

				// This is so that it doesn't overflow and screw up the wall, it doesn't need to go higher than the texture's height anyway
				texturevpegbottom %= SHORT(textures[gr_bottomtexture]->height)<<FRACBITS;

				wallVerts[3].t = wallVerts[2].t = texturevpegbottom * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpegbottom + gr_backsector->floorheight - gr_frontsector->floorheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;

#ifdef ESLOPE
				// Adjust t value for sloped walls
				if (!(gr_linedef->flags & ML_EFFECT1))
				{
					// Unskewed
					wallVerts[0].t -= (worldbottom - gr_frontsector->floorheight) * grTex->scaleY;
					wallVerts[1].t -= (worldbottomslope - gr_frontsector->floorheight) * grTex->scaleY;
					wallVerts[3].t -= (worldlow - gr_backsector->floorheight) * grTex->scaleY;
					wallVerts[2].t -= (worldlowslope - gr_backsector->floorheight) * grTex->scaleY;
				}
				else if (gr_linedef->flags & ML_DONTPEGBOTTOM)
				{
					// Skewed by bottom
					wallVerts[0].t = wallVerts[1].t = (texturevpegbottom + worldlow - worldbottom) * grTex->scaleY;
					//wallVerts[3].t = wallVerts[0].t - (worldlow - worldbottom) * grTex->scaleY; // no need, [3] is already this
					wallVerts[2].t = wallVerts[1].t - (worldlowslope - worldbottomslope) * grTex->scaleY;
				}
				else
				{
					// Skewed by top
					wallVerts[0].t = (texturevpegbottom + worldlow - worldbottom) * grTex->scaleY;
					wallVerts[1].t = (texturevpegbottom + worldlowslope - worldbottomslope) * grTex->scaleY;
				}
#endif
			}

			// set top/bottom coords
#ifdef ESLOPE
			wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldlowslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);
#else
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(worldlow);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldbottom);
#endif

			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_bottomtexture, &Surf, FF_CUTLEVEL, NULL);
			else if (grTex->mipmap.flags & TF_TRANSPARENT)
				HWR_AddTransparentWall(wallVerts, &Surf, gr_bottomtexture, PF_Environment, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
		}
		gr_midtexture = R_GetTextureNum(gr_sidedef->midtexture);
		if (gr_midtexture)
		{
			FBITFIELD blendmode;
			sector_t *front, *back;
			fixed_t  popentop, popenbottom, polytop, polybottom, lowcut, highcut;
			fixed_t     texturevpeg = 0;
			INT32 repeats;

			if (gr_linedef->frontsector->heightsec != -1)
				front = &sectors[gr_linedef->frontsector->heightsec];
			else
				front = gr_linedef->frontsector;

			if (gr_linedef->backsector->heightsec != -1)
				back = &sectors[gr_linedef->backsector->heightsec];
			else
				back = gr_linedef->backsector;

			if (gr_sidedef->repeatcnt)
				repeats = 1 + gr_sidedef->repeatcnt;
			else if (gr_linedef->flags & ML_EFFECT5)
			{
				fixed_t high, low;

				if (front->ceilingheight > back->ceilingheight)
					high = back->ceilingheight;
				else
					high = front->ceilingheight;

				if (front->floorheight > back->floorheight)
					low = front->floorheight;
				else
					low = back->floorheight;

				repeats = (high - low)/textureheight[gr_sidedef->midtexture];
				if ((high-low)%textureheight[gr_sidedef->midtexture])
					repeats++; // tile an extra time to fill the gap -- Monster Iestyn
			}
			else
				repeats = 1;

			// SoM: a little note: This code re-arranging will
			// fix the bug in Nimrod map02. popentop and popenbottom
			// record the limits the texture can be displayed in.
			// polytop and polybottom, are the ideal (i.e. unclipped)
			// heights of the polygon, and h & l, are the final (clipped)
			// poly coords.

#ifdef POLYOBJECTS
			// NOTE: With polyobjects, whenever you need to check the properties of the polyobject sector it belongs to,
			// you must use the linedef's backsector to be correct
			// From CB
			if (gr_curline->polyseg)
			{
				popentop = back->ceilingheight;
				popenbottom = back->floorheight;
			}
			else
#endif
            {
#ifdef ESLOPE
				popentop = min(worldtop, worldhigh);
				popenbottom = max(worldbottom, worldlow);
#else
				popentop = min(front->ceilingheight, back->ceilingheight);
				popenbottom = max(front->floorheight, back->floorheight);
#endif
			}

#ifdef ESLOPE
			if (gr_linedef->flags & ML_EFFECT2)
			{
				if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
				{
					polybottom = max(front->floorheight, back->floorheight) + gr_sidedef->rowoffset;
					polytop = polybottom + textureheight[gr_midtexture]*repeats;
				}
				else
				{
					polytop = min(front->ceilingheight, back->ceilingheight) + gr_sidedef->rowoffset;
					polybottom = polytop - textureheight[gr_midtexture]*repeats;
				}
			}
			else if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
#else
            if (gr_linedef->flags & ML_DONTPEGBOTTOM)
#endif
			{
				polybottom = popenbottom + gr_sidedef->rowoffset;
				polytop = polybottom + textureheight[gr_midtexture]*repeats;
			}
			else
			{
				polytop = popentop + gr_sidedef->rowoffset;
				polybottom = polytop - textureheight[gr_midtexture]*repeats;
			}
			// CB
#ifdef POLYOBJECTS
			// NOTE: With polyobjects, whenever you need to check the properties of the polyobject sector it belongs to,
			// you must use the linedef's backsector to be correct
			if (gr_curline->polyseg)
			{
				lowcut = polybottom;
				highcut = polytop;
			}
#endif
			else
			{
				// The cut-off values of a linedef can always be constant, since every line has an absoulute front and or back sector
				lowcut = popenbottom;
				highcut = popentop;
			}

			h = min(highcut, polytop);
			l = max(polybottom, lowcut);

			if (drawtextured)
			{
				// PEGGING
#ifdef ESLOPE
				if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
#else
				if (gr_linedef->flags & ML_DONTPEGBOTTOM)
#endif
					texturevpeg = textureheight[gr_sidedef->midtexture]*repeats - h + polybottom;
				else
					texturevpeg = polytop - h;

				grTex = HWR_GetTexture(gr_midtexture);

				wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (h - l + texturevpeg) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
			}

			// set top/bottom coords
			// Take the texture peg into account, rather than changing the offsets past
			// where the polygon might not be.
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(h);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(l);

#ifdef ESLOPE
			// Correct to account for slopes
			{
				fixed_t midtextureslant;

				if (gr_linedef->flags & ML_EFFECT2)
					midtextureslant = 0;
				else if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
					midtextureslant = worldlow < worldbottom
							  ? worldbottomslope-worldbottom
							  : worldlowslope-worldlow;
				else
					midtextureslant = worldtop < worldhigh
							  ? worldtopslope-worldtop
							  : worldhighslope-worldhigh;

				polytop += midtextureslant;
				polybottom += midtextureslant;

				highcut += worldtop < worldhigh
						 ? worldtopslope-worldtop
						 : worldhighslope-worldhigh;
				lowcut += worldlow < worldbottom
						? worldbottomslope-worldbottom
						: worldlowslope-worldlow;

				// Texture stuff
				h = min(highcut, polytop);
				l = max(polybottom, lowcut);

				if (drawtextured)
				{
					// PEGGING
					if (!!(gr_linedef->flags & ML_DONTPEGBOTTOM) ^ !!(gr_linedef->flags & ML_EFFECT3))
						texturevpeg = textureheight[gr_sidedef->midtexture]*repeats - h + polybottom;
					else
						texturevpeg = polytop - h;
					wallVerts[2].t = texturevpeg * grTex->scaleY;
					wallVerts[1].t = (h - l + texturevpeg) * grTex->scaleY;
				}

				wallVerts[2].y = FIXED_TO_FLOAT(h);
				wallVerts[1].y = FIXED_TO_FLOAT(l);
			}
#endif

			// set alpha for transparent walls (new boom and legacy linedef types)
			// ooops ! this do not work at all because render order we should render it in backtofront order
			switch (gr_linedef->special)
			{
				case 900:
					blendmode = HWR_TranstableToAlpha(tr_trans10, &Surf);
					break;
				case 901:
					blendmode = HWR_TranstableToAlpha(tr_trans20, &Surf);
					break;
				case 902:
					blendmode = HWR_TranstableToAlpha(tr_trans30, &Surf);
					break;
				case 903:
					blendmode = HWR_TranstableToAlpha(tr_trans40, &Surf);
					break;
				case 904:
					blendmode = HWR_TranstableToAlpha(tr_trans50, &Surf);
					break;
				case 905:
					blendmode = HWR_TranstableToAlpha(tr_trans60, &Surf);
					break;
				case 906:
					blendmode = HWR_TranstableToAlpha(tr_trans70, &Surf);
					break;
				case 907:
					blendmode = HWR_TranstableToAlpha(tr_trans80, &Surf);
					break;
				case 908:
					blendmode = HWR_TranstableToAlpha(tr_trans90, &Surf);
					break;
				//  Translucent
				case 102:
				case 121:
				case 123:
				case 124:
				case 125:
				case 141:
				case 142:
				case 144:
				case 145:
				case 174:
				case 175:
				case 192:
				case 195:
				case 221:
				case 253:
				case 256:
					blendmode = PF_Translucent;
					break;
				default:
					blendmode = PF_Masked;
					break;
			}

#ifdef POLYOBJECTS
			if (gr_curline->polyseg && gr_curline->polyseg->translucency > 0)
			{
				if (gr_curline->polyseg->translucency >= NUMTRANSMAPS) // wall not drawn
				{
					Surf.FlatColor.s.alpha = 0x00; // This shouldn't draw anything regardless of blendmode
					blendmode = PF_Masked;
				}
				else
					blendmode = HWR_TranstableToAlpha(gr_curline->polyseg->translucency, &Surf);
			}
#endif

			if (gr_frontsector->numlights)
			{
				if (!(blendmode & PF_Masked))
					HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_TRANSLUCENT, NULL);
				else
				{
					HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_CUTLEVEL, NULL);
				}
			}
			else if (!(blendmode & PF_Masked))
				HWR_AddTransparentWall(wallVerts, &Surf, gr_midtexture, blendmode, false, lightnum, colormap);
			else
				HWR_ProjectWall(wallVerts, &Surf, blendmode, lightnum, colormap);

			// If there is a colormap change, remove it.
/*			if (!(Surf.FlatColor.s.red + Surf.FlatColor.s.green + Surf.FlatColor.s.blue == Surf.FlatColor.s.red/3)
			{
				Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
				Surf.FlatColor.rgba = 0xffffffff;
			}*/
		}

		// Isn't this just the most lovely mess
		if (!gr_curline->polyseg) // Don't do it for polyobjects
		{
			if (gr_frontsector->ceilingpic == skyflatnum || gr_backsector->ceilingpic == skyflatnum)
			{
				fixed_t depthwallheight;

				if (!gr_sidedef->toptexture || (gr_frontsector->ceilingpic == skyflatnum && gr_backsector->ceilingpic == skyflatnum)) // when both sectors are sky, the top texture isn't drawn
					depthwallheight = gr_frontsector->ceilingheight < gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;
				else
					depthwallheight = gr_frontsector->ceilingheight > gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;

				if (gr_frontsector->ceilingheight-gr_frontsector->floorheight <= 0) // current sector is a thok barrier
				{
					if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is also a thok barrier
					{
						if (!gr_sidedef->bottomtexture) // Only extend further down if there's no texture
							HWR_DrawSkyWall(wallVerts, &Surf, worldbottom < worldlow ? worldbottom : worldlow, INT32_MAX);
						else
							HWR_DrawSkyWall(wallVerts, &Surf, worldbottom > worldlow ? worldbottom : worldlow, INT32_MAX);
					}
					// behind sector is not a thok barrier
					else if (gr_backsector->ceilingheight <= gr_frontsector->ceilingheight) // behind sector ceiling is lower or equal to current sector
						HWR_DrawSkyWall(wallVerts, &Surf, depthwallheight, INT32_MAX);
						// gr_front/backsector heights need to be used here because of the worldtop being set to worldhigh earlier on
				}
				else if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is a thok barrier, current sector is not
				{
					if (gr_backsector->ceilingheight >= gr_frontsector->ceilingheight // thok barrier ceiling height is equal to or greater than current sector ceiling height
						|| gr_backsector->floorheight <= gr_frontsector->floorheight // thok barrier ceiling height is equal to or less than current sector floor height
						|| gr_backsector->ceilingpic != skyflatnum) // thok barrier is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, depthwallheight, INT32_MAX);
				}
				else // neither sectors are thok barriers
				{
					if ((gr_backsector->ceilingheight < gr_frontsector->ceilingheight && !gr_sidedef->toptexture) // no top texture and sector behind is lower
						|| gr_backsector->ceilingpic != skyflatnum) // behind sector is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, depthwallheight, INT32_MAX);
				}
			}
			// And now for sky floors!
			if (gr_frontsector->floorpic == skyflatnum || gr_backsector->floorpic == skyflatnum)
			{
				fixed_t depthwallheight;

				if (!gr_sidedef->bottomtexture)
					depthwallheight = worldbottom > worldlow ? worldbottom : worldlow;
				else
					depthwallheight = worldbottom < worldlow ? worldbottom : worldlow;

				if (gr_frontsector->ceilingheight-gr_frontsector->floorheight <= 0) // current sector is a thok barrier
				{
					if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is also a thok barrier
					{
						if (!gr_sidedef->toptexture) // Only extend up if there's no texture
							HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, worldtop > worldhigh ? worldtop : worldhigh);
						else
							HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, worldtop < worldhigh ? worldtop : worldhigh);
					}
					// behind sector is not a thok barrier
					else if (gr_backsector->floorheight >= gr_frontsector->floorheight) // behind sector floor is greater or equal to current sector
						HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, depthwallheight);
				}
				else if (gr_backsector->ceilingheight-gr_backsector->floorheight <= 0) // behind sector is a thok barrier, current sector is not
				{
					if (gr_backsector->floorheight <= gr_frontsector->floorheight // thok barrier floor height is equal to or less than current sector floor height
						|| gr_backsector->ceilingheight >= gr_frontsector->ceilingheight // thok barrier floor height is equal to or greater than current sector ceiling height
						|| gr_backsector->floorpic != skyflatnum) // thok barrier is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, depthwallheight);
				}
				else // neither sectors are thok barriers
				{
					if ((gr_backsector->floorheight > gr_frontsector->floorheight && !gr_sidedef->bottomtexture) // no bottom texture and sector behind is higher
						|| gr_backsector->floorpic != skyflatnum) // behind sector is not a sky
						HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, depthwallheight);
				}
			}
		}
	}
	else
	{
		// Single sided line... Deal only with the middletexture (if one exists)
		gr_midtexture = R_GetTextureNum(gr_sidedef->midtexture);
		if (gr_midtexture)
		{
			if (drawtextured)
			{
				fixed_t     texturevpeg;
				// PEGGING
#ifdef ESLOPE
				if ((gr_linedef->flags & (ML_DONTPEGBOTTOM|ML_EFFECT2)) == (ML_DONTPEGBOTTOM|ML_EFFECT2))
					texturevpeg = gr_frontsector->floorheight + textureheight[gr_sidedef->midtexture] - gr_frontsector->ceilingheight + gr_sidedef->rowoffset;
				else
#endif
				if (gr_linedef->flags & ML_DONTPEGBOTTOM)
					texturevpeg = worldbottom + textureheight[gr_sidedef->midtexture] - worldtop + gr_sidedef->rowoffset;
				else
					// top of texture at top
					texturevpeg = gr_sidedef->rowoffset;

				grTex = HWR_GetTexture(gr_midtexture);

				wallVerts[3].t = wallVerts[2].t = texturevpeg * grTex->scaleY;
				wallVerts[0].t = wallVerts[1].t = (texturevpeg + gr_frontsector->ceilingheight - gr_frontsector->floorheight) * grTex->scaleY;
				wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
				wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;

#ifdef ESLOPE
				// Texture correction for slopes
				if (gr_linedef->flags & ML_EFFECT2) {
					wallVerts[3].t += (gr_frontsector->ceilingheight - worldtop) * grTex->scaleY;
					wallVerts[2].t += (gr_frontsector->ceilingheight - worldtopslope) * grTex->scaleY;
					wallVerts[0].t += (gr_frontsector->floorheight - worldbottom) * grTex->scaleY;
					wallVerts[1].t += (gr_frontsector->floorheight - worldbottomslope) * grTex->scaleY;
				} else if (gr_linedef->flags & ML_DONTPEGBOTTOM) {
					wallVerts[3].t = wallVerts[0].t + (worldbottom-worldtop) * grTex->scaleY;
					wallVerts[2].t = wallVerts[1].t + (worldbottomslope-worldtopslope) * grTex->scaleY;
				} else {
					wallVerts[0].t = wallVerts[3].t - (worldbottom-worldtop) * grTex->scaleY;
					wallVerts[1].t = wallVerts[2].t - (worldbottomslope-worldtopslope) * grTex->scaleY;
				}
#endif
			}
#ifdef ESLOPE
			//Set textures properly on single sided walls that are sloped
			wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = FIXED_TO_FLOAT(worldbottom);
			wallVerts[2].y = FIXED_TO_FLOAT(worldtopslope);
			wallVerts[1].y = FIXED_TO_FLOAT(worldbottomslope);
#else
			// set top/bottom coords
			wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(worldtop);
			wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(worldbottom);
#endif
			// I don't think that solid walls can use translucent linedef types...
			if (gr_frontsector->numlights)
				HWR_SplitWall(gr_frontsector, wallVerts, gr_midtexture, &Surf, FF_CUTLEVEL, NULL);
			else
			{
				if (grTex->mipmap.flags & TF_TRANSPARENT)
					HWR_AddTransparentWall(wallVerts, &Surf, gr_midtexture, PF_Environment, false, lightnum, colormap);
				else
					HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
			}
		}

		if (!gr_curline->polyseg)
		{
			if (gr_frontsector->ceilingpic == skyflatnum) // It's a single-sided line with sky for its sector
				HWR_DrawSkyWall(wallVerts, &Surf, worldtop, INT32_MAX);
			if (gr_frontsector->floorpic == skyflatnum)
				HWR_DrawSkyWall(wallVerts, &Surf, INT32_MIN, worldbottom);
		}
	}


	//Hurdler: 3d-floors test
#ifdef R_FAKEFLOORS
	if (gr_frontsector && gr_backsector && gr_frontsector->tag != gr_backsector->tag && (gr_backsector->ffloors || gr_frontsector->ffloors))
	{
		ffloor_t * rover;
		fixed_t    highcut = 0, lowcut = 0;

		INT32 texnum;
		line_t * newline = NULL; // Multi-Property FOF

        ///TODO add slope support (fixing cutoffs, proper wall clipping) - maybe just disable highcut/lowcut if either sector or FOF has a slope
        ///     to allow fun plane intersecting in OGL? But then people would abuse that and make software look bad. :C
		highcut = gr_frontsector->ceilingheight < gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;
		lowcut = gr_frontsector->floorheight > gr_backsector->floorheight ? gr_frontsector->floorheight : gr_backsector->floorheight;

		if (gr_backsector->ffloors)
		{
			for (rover = gr_backsector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERSIDES) || (rover->flags & FF_INVERTSIDES))
					continue;
				if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
					continue;

				texnum = R_GetTextureNum(sides[rover->master->sidenum[0]].midtexture);

				if (rover->master->flags & ML_TFERLINE)
				{
					size_t linenum = gr_curline->linedef-gr_backsector->lines[0];
					newline = rover->master->frontsector->lines[0] + linenum;
					texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
				}

#ifdef ESLOPE
				h  = *rover->t_slope ? P_GetZAt(*rover->t_slope, v1x, v1y) : *rover->topheight;
				hS = *rover->t_slope ? P_GetZAt(*rover->t_slope, v2x, v2y) : *rover->topheight;
				l  = *rover->b_slope ? P_GetZAt(*rover->b_slope, v1x, v1y) : *rover->bottomheight;
				lS = *rover->b_slope ? P_GetZAt(*rover->b_slope, v2x, v2y) : *rover->bottomheight;
				if (!(*rover->t_slope) && !gr_frontsector->c_slope && !gr_backsector->c_slope && h > highcut)
					h = hS = highcut;
				if (!(*rover->b_slope) && !gr_frontsector->f_slope && !gr_backsector->f_slope && l < lowcut)
					l = lS = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords

				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
#else
				h = *rover->topheight;
				l = *rover->bottomheight;
				if (h > highcut)
					h = highcut;
				if (l < lowcut)
					l = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords
				wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(l);
#endif
				if (rover->flags & FF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else if (drawtextured)
				{
					fixed_t texturevpeg;

					// Wow, how was this missing from OpenGL for so long?
					// ...Oh well, anyway, Lower Unpegged now changes pegging of FOFs like in software
					// -- Monster Iestyn 26/06/18
					if (newline)
					{
						texturevpeg = sides[newline->sidenum[0]].rowoffset;
						if (newline->flags & ML_DONTPEGBOTTOM)
							texturevpeg -= *rover->topheight - *rover->bottomheight;
					}
					else
					{
						texturevpeg = sides[rover->master->sidenum[0]].rowoffset;
						if (gr_linedef->flags & ML_DONTPEGBOTTOM)
							texturevpeg -= *rover->topheight - *rover->bottomheight;
					}

					grTex = HWR_GetTexture(texnum);

#ifdef ESLOPE
					wallVerts[3].t = (*rover->topheight - h + texturevpeg) * grTex->scaleY;
					wallVerts[2].t = (*rover->topheight - hS + texturevpeg) * grTex->scaleY;
					wallVerts[0].t = (*rover->topheight - l + texturevpeg) * grTex->scaleY;
					wallVerts[1].t = (*rover->topheight - lS + texturevpeg) * grTex->scaleY;
#else
					wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + texturevpeg) * grTex->scaleY;
					wallVerts[0].t = wallVerts[1].t = (*rover->topheight - l + texturevpeg) * grTex->scaleY;
#endif

					wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
					wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
				}
				if (rover->flags & FF_FOG)
				{
					FBITFIELD blendmode;

					blendmode = PF_Fog|PF_NoTexture;

					lightnum = rover->master->frontsector->lightlevel;
					colormap = rover->master->frontsector->extra_colormap;

					if (rover->master->frontsector->extra_colormap)
					{

						Surf.FlatColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel,rover->master->frontsector->extra_colormap->rgba);
					}
					else
					{
						Surf.FlatColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel,NORMALFOG);
					}

					if (gr_frontsector->numlights)
						HWR_SplitWall(gr_frontsector, wallVerts, 0, &Surf, rover->flags, rover);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					FBITFIELD blendmode = PF_Masked;

					if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
					{
						blendmode = PF_Translucent;
						Surf.FlatColor.s.alpha = (UINT8)rover->alpha-1 > 255 ? 255 : rover->alpha-1;
					}

					if (gr_frontsector->numlights)
						HWR_SplitWall(gr_frontsector, wallVerts, texnum, &Surf, rover->flags, rover);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}

		if (gr_frontsector->ffloors) // Putting this seperate should allow 2 FOF sectors to be connected without too many errors? I think?
		{
			for (rover = gr_frontsector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_ALLSIDES))
					continue;
				if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
					continue;

				texnum = R_GetTextureNum(sides[rover->master->sidenum[0]].midtexture);

				if (rover->master->flags & ML_TFERLINE)
				{
					size_t linenum = gr_curline->linedef-gr_backsector->lines[0];
					newline = rover->master->frontsector->lines[0] + linenum;
					texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
				}
#ifdef ESLOPE //backsides
				h  = *rover->t_slope ? P_GetZAt(*rover->t_slope, v1x, v1y) : *rover->topheight;
				hS = *rover->t_slope ? P_GetZAt(*rover->t_slope, v2x, v2y) : *rover->topheight;
				l  = *rover->b_slope ? P_GetZAt(*rover->b_slope, v1x, v1y) : *rover->bottomheight;
				lS = *rover->b_slope ? P_GetZAt(*rover->b_slope, v2x, v2y) : *rover->bottomheight;
				if (!(*rover->t_slope) && !gr_frontsector->c_slope && !gr_backsector->c_slope && h > highcut)
					h = hS = highcut;
				if (!(*rover->b_slope) && !gr_frontsector->f_slope && !gr_backsector->f_slope && l < lowcut)
					l = lS = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords

				wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[2].y = FIXED_TO_FLOAT(hS);
				wallVerts[0].y = FIXED_TO_FLOAT(l);
				wallVerts[1].y = FIXED_TO_FLOAT(lS);
#else
				h = *rover->topheight;
				l = *rover->bottomheight;
				if (h > highcut)
					h = highcut;
				if (l < lowcut)
					l = lowcut;
				//Hurdler: HW code starts here
				//FIXME: check if peging is correct
				// set top/bottom coords
				wallVerts[2].y = wallVerts[3].y = FIXED_TO_FLOAT(h);
				wallVerts[0].y = wallVerts[1].y = FIXED_TO_FLOAT(l);
#endif
				if (rover->flags & FF_FOG)
				{
					wallVerts[3].t = wallVerts[2].t = 0;
					wallVerts[0].t = wallVerts[1].t = 0;
					wallVerts[0].s = wallVerts[3].s = 0;
					wallVerts[2].s = wallVerts[1].s = 0;
				}
				else if (drawtextured)
				{
					grTex = HWR_GetTexture(texnum);

					if (newline)
					{
						wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + sides[newline->sidenum[0]].rowoffset) * grTex->scaleY;
						wallVerts[0].t = wallVerts[1].t = (h - l + (*rover->topheight - h + sides[newline->sidenum[0]].rowoffset)) * grTex->scaleY;
					}
					else
					{
						wallVerts[3].t = wallVerts[2].t = (*rover->topheight - h + sides[rover->master->sidenum[0]].rowoffset) * grTex->scaleY;
						wallVerts[0].t = wallVerts[1].t = (h - l + (*rover->topheight - h + sides[rover->master->sidenum[0]].rowoffset)) * grTex->scaleY;
					}

					wallVerts[0].s = wallVerts[3].s = cliplow * grTex->scaleX;
					wallVerts[2].s = wallVerts[1].s = cliphigh * grTex->scaleX;
				}

				if (rover->flags & FF_FOG)
				{
					FBITFIELD blendmode;

					blendmode = PF_Fog|PF_NoTexture;

					lightnum = rover->master->frontsector->lightlevel;
					colormap = rover->master->frontsector->extra_colormap;

					if (rover->master->frontsector->extra_colormap)
					{
						Surf.FlatColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel,rover->master->frontsector->extra_colormap->rgba);
					}
					else
					{
						Surf.FlatColor.s.alpha = HWR_FogBlockAlpha(rover->master->frontsector->lightlevel,NORMALFOG);
					}

					if (gr_backsector->numlights)
						HWR_SplitWall(gr_backsector, wallVerts, 0, &Surf, rover->flags, rover);
					else
						HWR_AddTransparentWall(wallVerts, &Surf, 0, blendmode, true, lightnum, colormap);
				}
				else
				{
					FBITFIELD blendmode = PF_Masked;

					if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
					{
						blendmode = PF_Translucent;
						Surf.FlatColor.s.alpha = (UINT8)rover->alpha-1 > 255 ? 255 : rover->alpha-1;
					}

					if (gr_backsector->numlights)
						HWR_SplitWall(gr_backsector, wallVerts, texnum, &Surf, rover->flags, rover);
					else
					{
						if (blendmode != PF_Masked)
							HWR_AddTransparentWall(wallVerts, &Surf, texnum, blendmode, false, lightnum, colormap);
						else
							HWR_ProjectWall(wallVerts, &Surf, PF_Masked, lightnum, colormap);
					}
				}
			}
		}
	}
#endif
//Hurdler: end of 3d-floors test
}

//Hurdler: just like in r_bsp.c
#if 1
#define MAXSEGS         MAXVIDWIDTH/2+1
#else
//Alam_GBC: Or not (may cause overflow)
#define MAXSEGS         128
#endif

// hw_newend is one past the last valid seg
static cliprange_t *   hw_newend;
static cliprange_t     gr_solidsegs[MAXSEGS];


static void printsolidsegs(void)
{
	cliprange_t *       start;
	if (!hw_newend || cv_grbeta.value != 2)
		return;
	for (start = gr_solidsegs;start != hw_newend;start++)
	{
		CONS_Debug(DBG_RENDER, "%d-%d|",start->first,start->last);
	}
	CONS_Debug(DBG_RENDER, "\n\n");
}

//
//
//
static void HWR_ClipSolidWallSegment(INT32 first, INT32 last)
{
	cliprange_t *next, *start;
	float lowfrac, highfrac;
	boolean poorhack = false;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start),
			//  so insert a new clippost.
			HWR_StoreWallRange(first, last);

			next = hw_newend;
			hw_newend++;

			while (next != start)
			{
				*next = *(next-1);
				next--;
			}

			next->first = first;
			next->last = last;
			printsolidsegs();
			return;
		}

		// There is a fragment above *start.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first, last);
			poorhack = true;
		}
		else
		{
			highfrac = HWR_ClipViewSegment(start->first+1, (polyvertex_t *)gr_curline->v1, (polyvertex_t *)gr_curline->v2);
			HWR_StoreWallRange(0, highfrac);
		}
		// Now adjust the clip size.
		start->first = first;
	}

	// Bottom contained in start?
	if (last <= start->last)
	{
		printsolidsegs();
		return;
	}
	next = start;
	while (last >= (next+1)->first-1)
	{
		// There is a fragment between two posts.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(next->last-1, (polyvertex_t *)gr_curline->v1, (polyvertex_t *)gr_curline->v2);
			highfrac = HWR_ClipViewSegment((next+1)->first+1, (polyvertex_t *)gr_curline->v1, (polyvertex_t *)gr_curline->v2);
			HWR_StoreWallRange(lowfrac, highfrac);
		}
		next++;

		if (last <= next->last)
		{
			// Bottom is contained in next.
			// Adjust the clip size.
			start->last = next->last;
			goto crunch;
		}
	}

	if (first == next->first+1) // 1 line texture
	{
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
			HWR_StoreWallRange(0, 1);
	}
	else
	{
	// There is a fragment after *next.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(first,last);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(next->last-1, (polyvertex_t *)gr_curline->v1, (polyvertex_t *)gr_curline->v2);
			HWR_StoreWallRange(lowfrac, 1);
		}
	}

	// Adjust the clip size.
	start->last = last;

	// Remove start+1 to next from the clip list,
	// because start now covers their area.
crunch:
	if (next == start)
	{
		printsolidsegs();
		// Post just extended past the bottom of one post.
		return;
	}


	while (next++ != hw_newend)
	{
		// Remove a post.
		*++start = *next;
	}

	hw_newend = start;
	printsolidsegs();
}

//
//  handle LineDefs with upper and lower texture (windows)
//
static void HWR_ClipPassWallSegment(INT32 first, INT32 last)
{
	cliprange_t *start;
	float lowfrac, highfrac;
	//to allow noclipwalls but still solidseg reject of non-visible walls
	boolean poorhack = false;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first - 1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start).
			HWR_StoreWallRange(0, 1);
			return;
		}

		// There is a fragment above *start.
		if (!cv_grclipwalls.value)
		{	//20/08/99: Changed by Hurdler (taken from faB's code)
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
		{
			highfrac = HWR_ClipViewSegment(min(start->first + 1,
				start->last), (polyvertex_t *)gr_curline->v1,
				(polyvertex_t *)gr_curline->v2);
			HWR_StoreWallRange(0, highfrac);
		}
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	while (last >= (start+1)->first-1)
	{
		// There is a fragment between two posts.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
		{
			lowfrac  = HWR_ClipViewSegment(max(start->last-1,start->first), (polyvertex_t *)gr_curline->v1, (polyvertex_t *)gr_curline->v2);
			highfrac = HWR_ClipViewSegment(min((start+1)->first+1,(start+1)->last), (polyvertex_t *)gr_curline->v1, (polyvertex_t *)gr_curline->v2);
			HWR_StoreWallRange(lowfrac, highfrac);
		}
		start++;

		if (last <= start->last)
			return;
	}

	if (first == start->first+1) // 1 line texture
	{
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0, 1);
			poorhack = true;
		}
		else
			HWR_StoreWallRange(0, 1);
	}
	else
	{
		// There is a fragment after *next.
		if (!cv_grclipwalls.value)
		{
			if (!poorhack) HWR_StoreWallRange(0,1);
			poorhack = true;
		}
		else
		{
			lowfrac = HWR_ClipViewSegment(max(start->last - 1,
				start->first), (polyvertex_t *)gr_curline->v1,
				(polyvertex_t *)gr_curline->v2);
			HWR_StoreWallRange(lowfrac, 1);
		}
	}
}

// --------------------------------------------------------------------------
//  HWR_ClipToSolidSegs check if it is hide by wall (solidsegs)
// --------------------------------------------------------------------------
static boolean HWR_ClipToSolidSegs(INT32 first, INT32 last)
{
	cliprange_t * start;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = gr_solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
		return true;

	// Bottom contained in start?
	if (last <= start->last)
		return false;

	return true;
}

//
// HWR_ClearClipSegs
//
static void HWR_ClearClipSegs(void)
{
	gr_solidsegs[0].first = -0x7fffffff;
	gr_solidsegs[0].last = -1;
	gr_solidsegs[1].first = vid.width; //viewwidth;
	gr_solidsegs[1].last = 0x7fffffff;
	hw_newend = gr_solidsegs+2;
}

// -----------------+
// HWR_AddLine      : Clips the given segment and adds any visible pieces to the line list.
// Notes            : gr_cursectorlight is set to the current subsector -> sector -> light value
//                  : (it may be mixed with the wall's own flat colour in the future ...)
// -----------------+
static void HWR_AddLine(seg_t * line)
{
	INT32 x1, x2;
	angle_t angle1, angle2;
	angle_t span, tspan;

	// SoM: Backsector needs to be run through R_FakeFlat
	static sector_t tempsec;

	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
		return;

	gr_curline = line;

	// OPTIMIZE: quickly reject orthogonal back sides.
	angle1 = R_PointToAngle(FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v1)->x),
	                        FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v1)->y));
	angle2 = R_PointToAngle(FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v2)->x),
	                        FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v2)->y));

	// Clip to view edges.
	span = angle1 - angle2;

	// backface culling : span is < ANGLE_180 if ang1 > ang2 : the seg is facing
	if (span >= ANGLE_180)
		return;

	// Global angle needed by segcalc.
	//rw_angle1 = angle1;
	angle1 -= dup_viewangle;
	angle2 -= dup_viewangle;

	tspan = angle1 + gr_clipangle;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle1 = gr_clipangle;
	}
	tspan = gr_clipangle - angle2;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle2 = (angle_t)-(signed)gr_clipangle;
	}

#if 0
	{
		float fx1,fx2,fy1,fy2;
		//BP: test with a better projection than viewangletox[R_PointToAngle(angle)]
		// do not enable this at release 4 mul and 2 div
		fx1 = ((polyvertex_t *)(line->v1))->x-gr_viewx;
		fy1 = ((polyvertex_t *)(line->v1))->y-gr_viewy;
		fy2 = (fx1 * gr_viewcos + fy1 * gr_viewsin);
		if (fy2 < 0)
			// the point is back
			fx1 = 0;
		else
			fx1 = gr_windowcenterx + (fx1 * gr_viewsin - fy1 * gr_viewcos) * gr_centerx / fy2;

		fx2 = ((polyvertex_t *)(line->v2))->x-gr_viewx;
		fy2 = ((polyvertex_t *)(line->v2))->y-gr_viewy;
		fy1 = (fx2 * gr_viewcos + fy2 * gr_viewsin);
		if (fy1 < 0)
			// the point is back
			fx2 = vid.width;
		else
			fx2 = gr_windowcenterx + (fx2 * gr_viewsin - fy2 * gr_viewcos) * gr_centerx / fy1;

		x1 = fx1+0.5f;
		x2 = fx2+0.5f;
	}
#else
	// The seg is in the view range,
	// but not necessarily visible.
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;

	x1 = gr_viewangletox[angle1];
	x2 = gr_viewangletox[angle2];
#endif
	// Does not cross a pixel?
//	if (x1 == x2)
/*	{
		// BP: HERE IS THE MAIN PROBLEM !
		//CONS_Debug(DBG_RENDER, "tineline\n");
		return;
	}
*/
	gr_backsector = line->backsector;

	// Single sided line?
	if (!gr_backsector)
		goto clipsolid;

	gr_backsector = R_FakeFlat(gr_backsector, &tempsec, NULL, NULL, true);

#ifdef ESLOPE
	if (gr_frontsector->f_slope || gr_frontsector->c_slope || gr_backsector->f_slope || gr_backsector->c_slope)
	{
		fixed_t v1x, v1y, v2x, v2y; // the seg's vertexes as fixed_t
		fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
		fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends

		v1x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v1)->x);
		v1y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v1)->y);
		v2x = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v2)->x);
		v2y = FLOAT_TO_FIXED(((polyvertex_t *)gr_curline->v2)->y);
#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		if (slope) { \
			end1 = P_GetZAt(slope, v1x, v1y); \
			end2 = P_GetZAt(slope, v2x, v2y); \
		} else \
			end1 = end2 = normalheight;

		SLOPEPARAMS(gr_frontsector->f_slope, frontf1, frontf2, gr_frontsector->floorheight)
		SLOPEPARAMS(gr_frontsector->c_slope, frontc1, frontc2, gr_frontsector->ceilingheight)
		SLOPEPARAMS( gr_backsector->f_slope, backf1,  backf2,  gr_backsector->floorheight)
		SLOPEPARAMS( gr_backsector->c_slope, backc1,  backc2,  gr_backsector->ceilingheight)
#undef SLOPEPARAMS

		// Closed door.
		if ((backc1 <= frontf1 && backc2 <= frontf2)
			|| (backf1 >= frontc1 && backf2 >= frontc2))
		{
			goto clipsolid;
		}

		// Window.
		if (backc1 != frontc1 || backc2 != frontc2
			|| backf1 != frontf1 || backf2 != frontf2)
		{
			goto clippass;
		}
	}
	else
#endif
	{
		// Closed door.
		if (gr_backsector->ceilingheight <= gr_frontsector->floorheight ||
			gr_backsector->floorheight >= gr_frontsector->ceilingheight)
			goto clipsolid;

		// Window.
		if (gr_backsector->ceilingheight != gr_frontsector->ceilingheight ||
			gr_backsector->floorheight != gr_frontsector->floorheight)
			goto clippass;
	}

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides,
	//  identical light levels on both sides,
	//  and no middle texture.
	if (
#ifdef POLYOBJECTS
		!line->polyseg &&
#endif
		gr_backsector->ceilingpic == gr_frontsector->ceilingpic
		&& gr_backsector->floorpic == gr_frontsector->floorpic
#ifdef ESLOPE
		&& gr_backsector->f_slope == gr_frontsector->f_slope
		&& gr_backsector->c_slope == gr_frontsector->c_slope
#endif
	    && gr_backsector->lightlevel == gr_frontsector->lightlevel
		&& gr_curline->sidedef->midtexture == 0
		&& !gr_backsector->ffloors && !gr_frontsector->ffloors)
		// SoM: For 3D sides... Boris, would you like to take a
		// crack at rendering 3D sides? You would need to add the
		// above check and add code to HWR_StoreWallRange...
	{
		return;
	}

clippass:
	if (x1 == x2)
		{  x2++;x1 -= 2; }
	HWR_ClipPassWallSegment(x1, x2-1);
	return;

clipsolid:
	if (x1 == x2)
		goto clippass;
	HWR_ClipSolidWallSegment(x1, x2-1);
}

// HWR_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
// modified to use local variables

static boolean HWR_CheckBBox(fixed_t *bspcoord)
{
	INT32 boxpos, sx1, sx2;
	fixed_t px1, py1, px2, py2;
	angle_t angle1, angle2, span, tspan;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (dup_viewx <= bspcoord[BOXLEFT])
		boxpos = 0;
	else if (dup_viewx < bspcoord[BOXRIGHT])
		boxpos = 1;
	else
		boxpos = 2;

	if (dup_viewy >= bspcoord[BOXTOP])
		boxpos |= 0;
	else if (dup_viewy > bspcoord[BOXBOTTOM])
		boxpos |= 1<<2;
	else
		boxpos |= 2<<2;

	if (boxpos == 5)
		return true;

	px1 = bspcoord[checkcoord[boxpos][0]];
	py1 = bspcoord[checkcoord[boxpos][1]];
	px2 = bspcoord[checkcoord[boxpos][2]];
	py2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space
	angle1 = R_PointToAngle2(dup_viewx>>1, dup_viewy>>1, px1>>1, py1>>1) - dup_viewangle;
	angle2 = R_PointToAngle2(dup_viewx>>1, dup_viewy>>1, px2>>1, py2>>1) - dup_viewangle;

	span = angle1 - angle2;

	// Sitting on a line?
	if (span >= ANGLE_180)
		return true;

	tspan = angle1 + gr_clipangle;

	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle1 = gr_clipangle;
	}
	tspan = gr_clipangle - angle2;
	if (tspan > 2*gr_clipangle)
	{
		tspan -= 2*gr_clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle2 = (angle_t)-(signed)gr_clipangle;
	}

	// Find the first clippost
	//  that touches the source post
	//  (adjacent pixels are touching).
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;
	sx1 = gr_viewangletox[angle1];
	sx2 = gr_viewangletox[angle2];

	// Does not cross a pixel.
	if (sx1 == sx2)
		return false;

	return HWR_ClipToSolidSegs(sx1, sx2 - 1);
}

#ifdef POLYOBJECTS

//
// HWR_AddPolyObjectSegs
//
// haleyjd 02/19/06
// Adds all segs in all polyobjects in the given subsector.
// Modified for hardware rendering.
//
static inline void HWR_AddPolyObjectSegs(void)
{
	size_t i, j;
	seg_t *gr_fakeline = Z_Calloc(sizeof(seg_t), PU_STATIC, NULL);
	polyvertex_t *pv1 = Z_Calloc(sizeof(polyvertex_t), PU_STATIC, NULL);
	polyvertex_t *pv2 = Z_Calloc(sizeof(polyvertex_t), PU_STATIC, NULL);

	// Sort through all the polyobjects
	for (i = 0; i < numpolys; ++i)
	{
		// Render the polyobject's lines
		for (j = 0; j < po_ptrs[i]->segCount; ++j)
		{
			// Copy the info of a polyobject's seg, then convert it to OpenGL floating point
			M_Memcpy(gr_fakeline, po_ptrs[i]->segs[j], sizeof(seg_t));

			// Now convert the line to float and add it to be rendered
			pv1->x = FIXED_TO_FLOAT(gr_fakeline->v1->x);
			pv1->y = FIXED_TO_FLOAT(gr_fakeline->v1->y);
			pv2->x = FIXED_TO_FLOAT(gr_fakeline->v2->x);
			pv2->y = FIXED_TO_FLOAT(gr_fakeline->v2->y);

			gr_fakeline->v1 = (vertex_t *)pv1;
			gr_fakeline->v2 = (vertex_t *)pv2;

			HWR_AddLine(gr_fakeline);
		}
	}

	// Free temporary data no longer needed
	Z_Free(pv2);
	Z_Free(pv1);
	Z_Free(gr_fakeline);
}

#ifdef POLYOBJECTS_PLANES
static void HWR_RenderPolyObjectPlane(polyobj_t *polysector, boolean isceiling, fixed_t fixedheight,
									FBITFIELD blendmode, UINT8 lightlevel, lumpnum_t lumpnum, sector_t *FOFsector,
									UINT8 alpha, extracolormap_t *planecolormap)
{
	float           height; //constant y for all points on the convex flat polygon
	FOutVector      *v3d;
	INT32             i;
	float           flatxref,flatyref;
	float fflatsize;
	INT32 flatflag;
	size_t len;
	float scrollx = 0.0f, scrolly = 0.0f;
	angle_t angle = 0;
	FSurfaceInfo    Surf;
	fixed_t tempxsow, tempytow;
	size_t nrPlaneVerts;

	static FOutVector *planeVerts = NULL;
	static UINT16 numAllocedPlaneVerts = 0;

	nrPlaneVerts = polysector->numVertices;

	height = FIXED_TO_FLOAT(fixedheight);

	if (nrPlaneVerts < 3)   //not even a triangle ?
		return;

	if (nrPlaneVerts > UINT16_MAX) // FIXME: exceeds plVerts size
	{
		CONS_Debug(DBG_RENDER, "polygon size of %s exceeds max value of %d vertices\n", sizeu1(nrPlaneVerts), UINT16_MAX);
		return;
	}

	// Allocate plane-vertex buffer if we need to
	if (!planeVerts || nrPlaneVerts > numAllocedPlaneVerts)
	{
		numAllocedPlaneVerts = (UINT16)nrPlaneVerts;
		Z_Free(planeVerts);
		Z_Malloc(numAllocedPlaneVerts * sizeof (FOutVector), PU_LEVEL, &planeVerts);
	}

	len = W_LumpLength(lumpnum);

	switch (len)
	{
		case 4194304: // 2048x2048 lump
			fflatsize = 2048.0f;
			flatflag = 2047;
			break;
		case 1048576: // 1024x1024 lump
			fflatsize = 1024.0f;
			flatflag = 1023;
			break;
		case 262144:// 512x512 lump
			fflatsize = 512.0f;
			flatflag = 511;
			break;
		case 65536: // 256x256 lump
			fflatsize = 256.0f;
			flatflag = 255;
			break;
		case 16384: // 128x128 lump
			fflatsize = 128.0f;
			flatflag = 127;
			break;
		case 1024: // 32x32 lump
			fflatsize = 32.0f;
			flatflag = 31;
			break;
		default: // 64x64 lump
			fflatsize = 64.0f;
			flatflag = 63;
			break;
	}

	// reference point for flat texture coord for each vertex around the polygon
	flatxref = (float)(((fixed_t)FIXED_TO_FLOAT(polysector->origVerts[0].x) & (~flatflag)) / fflatsize);
	flatyref = (float)(((fixed_t)FIXED_TO_FLOAT(polysector->origVerts[0].y) & (~flatflag)) / fflatsize);

	// transform
	v3d = planeVerts;

	if (FOFsector != NULL)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->floor_yoffs)/fflatsize;
			angle = FOFsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(FOFsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(FOFsector->ceiling_yoffs)/fflatsize;
			angle = FOFsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}
	else if (gr_frontsector)
	{
		if (!isceiling) // it's a floor
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->floor_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->floor_yoffs)/fflatsize;
			angle = gr_frontsector->floorpic_angle>>ANGLETOFINESHIFT;
		}
		else // it's a ceiling
		{
			scrollx = FIXED_TO_FLOAT(gr_frontsector->ceiling_xoffs)/fflatsize;
			scrolly = FIXED_TO_FLOAT(gr_frontsector->ceiling_yoffs)/fflatsize;
			angle = gr_frontsector->ceilingpic_angle>>ANGLETOFINESHIFT;
		}
	}

	if (angle) // Only needs to be done if there's an altered angle
	{
		// This needs to be done so that it scrolls in a different direction after rotation like software
		tempxsow = FLOAT_TO_FIXED(scrollx);
		tempytow = FLOAT_TO_FIXED(scrolly);
		scrollx = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		scrolly = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));

		// This needs to be done so everything aligns after rotation
		// It would be done so that rotation is done, THEN the translation, but I couldn't get it to rotate AND scroll like software does
		tempxsow = FLOAT_TO_FIXED(flatxref);
		tempytow = FLOAT_TO_FIXED(flatyref);
		flatxref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
		flatyref = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINESINE(angle)) + FixedMul(tempytow, FINECOSINE(angle))));
	}

	for (i = 0; i < (INT32)nrPlaneVerts; i++,v3d++)
	{
		// Hurdler: add scrolling texture on floor/ceiling
		v3d->sow = (float)((FIXED_TO_FLOAT(polysector->origVerts[i].x) / fflatsize) - flatxref + scrollx); // Go from the polysector's original vertex locations
		v3d->tow = (float)(flatyref - (FIXED_TO_FLOAT(polysector->origVerts[i].y) / fflatsize) + scrolly); // Means the flat is offset based on the original vertex locations

		// Need to rotate before translate
		if (angle) // Only needs to be done if there's an altered angle
		{
			tempxsow = FLOAT_TO_FIXED(v3d->sow);
			tempytow = FLOAT_TO_FIXED(v3d->tow);
			v3d->sow = (FIXED_TO_FLOAT(FixedMul(tempxsow, FINECOSINE(angle)) - FixedMul(tempytow, FINESINE(angle))));
			v3d->tow = (FIXED_TO_FLOAT(-FixedMul(tempxsow, FINESINE(angle)) - FixedMul(tempytow, FINECOSINE(angle))));
		}

		v3d->x = FIXED_TO_FLOAT(polysector->vertices[i]->x);
		v3d->y = height;
		v3d->z = FIXED_TO_FLOAT(polysector->vertices[i]->y);
	}


	if (planecolormap)
		Surf.FlatColor.rgba = HWR_Lighting(lightlevel, planecolormap->rgba, planecolormap->fadergba, false, true);
	else
		Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, true);

	if (blendmode & PF_Translucent)
	{
		Surf.FlatColor.s.alpha = (UINT8)alpha;
		blendmode |= PF_Modulated|PF_Occlude|PF_Clip;
	}
	else
		blendmode |= PF_Masked|PF_Modulated|PF_Clip;

	HWD.pfnDrawPolygon(&Surf, planeVerts, nrPlaneVerts, blendmode);
}

static void HWR_AddPolyObjectPlanes(void)
{
	size_t i;
	sector_t *polyobjsector;

	// Polyobject Planes need their own function for drawing because they don't have extrasubsectors by themselves
	// It should be okay because polyobjects should always be convex anyway

	for (i  = 0; i < numpolys; i++)
	{
		polyobjsector = po_ptrs[i]->lines[0]->backsector; // the in-level polyobject sector

		if (!(po_ptrs[i]->flags & POF_RENDERPLANES)) // Only render planes when you should
			continue;

		if (po_ptrs[i]->translucency >= NUMTRANSMAPS)
			continue;

		if (polyobjsector->floorheight <= gr_frontsector->ceilingheight
			&& polyobjsector->floorheight >= gr_frontsector->floorheight
			&& (viewz < polyobjsector->floorheight))
		{
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(levelflats[polyobjsector->floorpic].lumpnum, po_ptrs[i], false, polyobjsector->floorheight,
													polyobjsector->lightlevel, Surf.FlatColor.s.alpha, polyobjsector, blendmode, NULL);
			}
			else
			{
				HWR_GetFlat(levelflats[polyobjsector->floorpic].lumpnum);
				HWR_RenderPolyObjectPlane(po_ptrs[i], false, polyobjsector->floorheight, PF_Occlude,
										polyobjsector->lightlevel, levelflats[polyobjsector->floorpic].lumpnum,
										polyobjsector, 255, NULL);
			}
		}

		if (polyobjsector->ceilingheight >= gr_frontsector->floorheight
			&& polyobjsector->ceilingheight <= gr_frontsector->ceilingheight
			&& (viewz > polyobjsector->ceilingheight))
		{
			if (po_ptrs[i]->translucency > 0)
			{
				FSurfaceInfo Surf;
				FBITFIELD blendmode;
				memset(&Surf, 0x00, sizeof(Surf));
				blendmode = HWR_TranstableToAlpha(po_ptrs[i]->translucency, &Surf);
				HWR_AddTransparentPolyobjectFloor(levelflats[polyobjsector->ceilingpic].lumpnum, po_ptrs[i], true, polyobjsector->ceilingheight,
				                                  polyobjsector->lightlevel, Surf.FlatColor.s.alpha, polyobjsector, blendmode, NULL);
			}
			else
			{
				HWR_GetFlat(levelflats[polyobjsector->ceilingpic].lumpnum);
				HWR_RenderPolyObjectPlane(po_ptrs[i], true, polyobjsector->ceilingheight, PF_Occlude,
				                          polyobjsector->lightlevel, levelflats[polyobjsector->floorpic].lumpnum,
				                          polyobjsector, 255, NULL);
			}
		}
	}
}
#endif
#endif

// -----------------+
// HWR_Subsector    : Determine floor/ceiling planes.
//                  : Add sprites of things in sector.
//                  : Draw one or more line segments.
// Notes            : Sets gr_cursectorlight to the light of the parent sector, to modulate wall textures
// -----------------+
static void HWR_Subsector(size_t num)
{
	INT16 count;
	seg_t *line;
	subsector_t *sub;
	static sector_t tempsec; //SoM: 4/7/2000
	INT32 floorlightlevel;
	INT32 ceilinglightlevel;
	INT32 locFloorHeight, locCeilingHeight;
	INT32 cullFloorHeight, cullCeilingHeight;
	INT32 light = 0;
	extracolormap_t *floorcolormap;
	extracolormap_t *ceilingcolormap;

#ifdef PARANOIA //no risk while developing, enough debugging nights!
	if (num >= addsubsector)
		I_Error("HWR_Subsector: ss %s with numss = %s, addss = %s\n",
			sizeu1(num), sizeu2(numsubsectors), sizeu3(addsubsector));

	/*if (num >= numsubsectors)
		I_Error("HWR_Subsector: ss %i with numss = %i",
		        num,
		        numsubsectors);*/
#endif

	if (num < numsubsectors)
	{
		sscount++;
		// subsector
		sub = &subsectors[num];
		// sector
		gr_frontsector = sub->sector;
		// how many linedefs
		count = sub->numlines;
		// first line seg
		line = &segs[sub->firstline];
	}
	else
	{
		// there are no segs but only planes
		sub = &subsectors[0];
		gr_frontsector = sub->sector;
		count = 0;
		line = NULL;
	}

	//SoM: 4/7/2000: Test to make Boom water work in Hardware mode.
	gr_frontsector = R_FakeFlat(gr_frontsector, &tempsec, &floorlightlevel,
								&ceilinglightlevel, false);
	//FIXME: Use floorlightlevel and ceilinglightlevel insted of lightlevel.

	floorcolormap = ceilingcolormap = gr_frontsector->extra_colormap;

	// ------------------------------------------------------------------------
	// sector lighting, DISABLED because it's done in HWR_StoreWallRange
	// ------------------------------------------------------------------------
	/// \todo store a RGBA instead of just intensity, allow coloured sector lighting
	//light = (FUBYTE)(sub->sector->lightlevel & 0xFF) / 255.0f;
	//gr_cursectorlight.red   = light;
	//gr_cursectorlight.green = light;
	//gr_cursectorlight.blue  = light;
	//gr_cursectorlight.alpha = light;

// ----- for special tricks with HW renderer -----
	if (gr_frontsector->pseudoSector)
	{
		cullFloorHeight = locFloorHeight = gr_frontsector->virtualFloorheight;
		cullCeilingHeight = locCeilingHeight = gr_frontsector->virtualCeilingheight;
	}
	else if (gr_frontsector->virtualFloor)
	{
		///@TODO Is this whole virtualFloor mess even useful? I don't think it even triggers ever.
		cullFloorHeight = locFloorHeight = gr_frontsector->virtualFloorheight;
		if (gr_frontsector->virtualCeiling)
			cullCeilingHeight = locCeilingHeight = gr_frontsector->virtualCeilingheight;
		else
			cullCeilingHeight = locCeilingHeight = gr_frontsector->ceilingheight;
	}
	else if (gr_frontsector->virtualCeiling)
	{
		cullCeilingHeight = locCeilingHeight = gr_frontsector->virtualCeilingheight;
		cullFloorHeight   = locFloorHeight   = gr_frontsector->floorheight;
	}
	else
	{
		cullFloorHeight   = locFloorHeight   = gr_frontsector->floorheight;
		cullCeilingHeight = locCeilingHeight = gr_frontsector->ceilingheight;

#ifdef ESLOPE
		if (gr_frontsector->f_slope)
		{
			cullFloorHeight = P_GetZAt(gr_frontsector->f_slope, viewx, viewy);
			locFloorHeight = P_GetZAt(gr_frontsector->f_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
		}

		if (gr_frontsector->c_slope)
		{
			cullCeilingHeight = P_GetZAt(gr_frontsector->c_slope, viewx, viewy);
			locCeilingHeight = P_GetZAt(gr_frontsector->c_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
		}
#endif
	}
// ----- end special tricks -----

	if (gr_frontsector->ffloors)
	{
		if (gr_frontsector->moved)
		{
			gr_frontsector->numlights = sub->sector->numlights = 0;
			R_Prep3DFloors(gr_frontsector);
			sub->sector->lightlist = gr_frontsector->lightlist;
			sub->sector->numlights = gr_frontsector->numlights;
			sub->sector->moved = gr_frontsector->moved = false;
		}

		light = R_GetPlaneLight(gr_frontsector, locFloorHeight, false);
		if (gr_frontsector->floorlightsec == -1)
			floorlightlevel = *gr_frontsector->lightlist[light].lightlevel;
		floorcolormap = gr_frontsector->lightlist[light].extra_colormap;

		light = R_GetPlaneLight(gr_frontsector, locCeilingHeight, false);
		if (gr_frontsector->ceilinglightsec == -1)
			ceilinglightlevel = *gr_frontsector->lightlist[light].lightlevel;
		ceilingcolormap = gr_frontsector->lightlist[light].extra_colormap;
	}

	sub->sector->extra_colormap = gr_frontsector->extra_colormap;

	// render floor ?
#ifdef DOPLANES
	// yeah, easy backface cull! :)
	if (cullFloorHeight < dup_viewz)
	{
		if (gr_frontsector->floorpic != skyflatnum)
		{
			if (sub->validcount != validcount)
			{
				HWR_GetFlat(levelflats[gr_frontsector->floorpic].lumpnum);
				HWR_RenderPlane(gr_frontsector, &extrasubsectors[num], false,
					// Hack to make things continue to work around slopes.
					locFloorHeight == cullFloorHeight ? locFloorHeight : gr_frontsector->floorheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, floorlightlevel, levelflats[gr_frontsector->floorpic].lumpnum, NULL, 255, false, floorcolormap);
			}
		}
		else
		{
#ifdef POLYSKY
			HWR_RenderSkyPlane(&extrasubsectors[num], locFloorHeight);
#endif
		}
	}

	if (cullCeilingHeight > dup_viewz)
	{
		if (gr_frontsector->ceilingpic != skyflatnum)
		{
			if (sub->validcount != validcount)
			{
				HWR_GetFlat(levelflats[gr_frontsector->ceilingpic].lumpnum);
				HWR_RenderPlane(NULL, &extrasubsectors[num], true,
					// Hack to make things continue to work around slopes.
					locCeilingHeight == cullCeilingHeight ? locCeilingHeight : gr_frontsector->ceilingheight,
					// We now return you to your regularly scheduled rendering.
					PF_Occlude, ceilinglightlevel, levelflats[gr_frontsector->ceilingpic].lumpnum,NULL, 255, false, ceilingcolormap);
			}
		}
		else
		{
#ifdef POLYSKY
			HWR_RenderSkyPlane(&extrasubsectors[num], locCeilingHeight);
#endif
		}
	}

#ifndef POLYSKY
	// Moved here because before, when above the ceiling and the floor does not have the sky flat, it doesn't draw the sky
	if (gr_frontsector->ceilingpic == skyflatnum || gr_frontsector->floorpic == skyflatnum)
	{
		drawsky = true;
	}
#endif

#ifdef R_FAKEFLOORS
	if (gr_frontsector->ffloors)
	{
		/// \todo fix light, xoffs, yoffs, extracolormap ?
		ffloor_t * rover;
		for (rover = gr_frontsector->ffloors;
			rover; rover = rover->next)
		{
			fixed_t cullHeight, centerHeight;

            // bottom plane
#ifdef ESLOPE
			if (*rover->b_slope)
			{
				cullHeight = P_GetZAt(*rover->b_slope, viewx, viewy);
				centerHeight = P_GetZAt(*rover->b_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
			}
			else
#endif
		    cullHeight = centerHeight = *rover->bottomheight;

			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
				continue;
			if (sub->validcount == validcount)
				continue;

			if (centerHeight <= locCeilingHeight &&
			    centerHeight >= locFloorHeight &&
			    ((dup_viewz < cullHeight && !(rover->flags & FF_INVERTPLANES)) ||
			     (dup_viewz > cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
			{
				if (rover->flags & FF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);

					if (rover->master->frontsector->extra_colormap)
						alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, rover->master->frontsector->extra_colormap->rgba);
					else
						alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, NORMALFOG);

					HWR_AddTransparentFloor(0,
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, rover->master->frontsector->extra_colormap);
				}
				else if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256) // SoM: Flags are more efficient
				{
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
#ifndef SORTING
					HWR_Add3DWater(levelflats[*rover->bottompic].lumpnum,
					               &extrasubsectors[num],
					               *rover->bottomheight,
					               *gr_frontsector->lightlist[light].lightlevel,
					               rover->alpha-1, rover->master->frontsector);
#else
					HWR_AddTransparentFloor(levelflats[*rover->bottompic].lumpnum,
					                       &extrasubsectors[num],
										   false,
					                       *rover->bottomheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, PF_Translucent,
					                       false, gr_frontsector->lightlist[light].extra_colormap);
#endif
				}
				else
				{
					HWR_GetFlat(levelflats[*rover->bottompic].lumpnum);
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					HWR_RenderPlane(NULL, &extrasubsectors[num], false, *rover->bottomheight, PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, levelflats[*rover->bottompic].lumpnum,
					                rover->master->frontsector, 255, false, gr_frontsector->lightlist[light].extra_colormap);
				}
			}

			// top plane
#ifdef ESLOPE
			if (*rover->t_slope)
			{
				cullHeight = P_GetZAt(*rover->t_slope, viewx, viewy);
				centerHeight = P_GetZAt(*rover->t_slope, gr_frontsector->soundorg.x, gr_frontsector->soundorg.y);
			}
			else
#endif
		    cullHeight = centerHeight = *rover->topheight;

			if (centerHeight >= locFloorHeight &&
			    centerHeight <= locCeilingHeight &&
			    ((dup_viewz > cullHeight && !(rover->flags & FF_INVERTPLANES)) ||
			     (dup_viewz < cullHeight && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
			{
				if (rover->flags & FF_FOG)
				{
					UINT8 alpha;

					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);

					if (rover->master->frontsector->extra_colormap)
						alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, rover->master->frontsector->extra_colormap->rgba);
					else
						alpha = HWR_FogBlockAlpha(*gr_frontsector->lightlist[light].lightlevel, NORMALFOG);

					HWR_AddTransparentFloor(0,
					                       &extrasubsectors[num],
										   true,
					                       *rover->topheight,
					                       *gr_frontsector->lightlist[light].lightlevel,
					                       alpha, rover->master->frontsector, PF_Fog|PF_NoTexture,
										   true, rover->master->frontsector->extra_colormap);
				}
				else if (rover->flags & FF_TRANSLUCENT && rover->alpha < 256)
				{
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
#ifndef SORTING
					HWR_Add3DWater(levelflats[*rover->toppic].lumpnum,
					                          &extrasubsectors[num],
					                          *rover->topheight,
					                          *gr_frontsector->lightlist[light].lightlevel,
					                          rover->alpha-1, rover->master->frontsector);
#else
					HWR_AddTransparentFloor(levelflats[*rover->toppic].lumpnum,
					                        &extrasubsectors[num],
											true,
					                        *rover->topheight,
					                        *gr_frontsector->lightlist[light].lightlevel,
					                        rover->alpha-1 > 255 ? 255 : rover->alpha-1, rover->master->frontsector, PF_Translucent,
					                        false, gr_frontsector->lightlist[light].extra_colormap);
#endif

				}
				else
				{
					HWR_GetFlat(levelflats[*rover->toppic].lumpnum);
					light = R_GetPlaneLight(gr_frontsector, centerHeight, dup_viewz < cullHeight ? true : false);
					HWR_RenderPlane(NULL, &extrasubsectors[num], true, *rover->topheight, PF_Occlude, *gr_frontsector->lightlist[light].lightlevel, levelflats[*rover->toppic].lumpnum,
					                  rover->master->frontsector, 255, false, gr_frontsector->lightlist[light].extra_colormap);
				}
			}
		}
	}
#endif
#endif //doplanes

#ifdef POLYOBJECTS
	// Draw all the polyobjects in this subsector
	if (sub->polyList)
	{
		polyobj_t *po = sub->polyList;

		numpolys = 0;

		// Count all the polyobjects, reset the list, and recount them
		while (po)
		{
			++numpolys;
			po = (polyobj_t *)(po->link.next);
		}

		// Sort polyobjects
		R_SortPolyObjects(sub);

		// Draw polyobject lines.
		HWR_AddPolyObjectSegs();

#ifdef POLYOBJECTS_PLANES
		if (sub->validcount != validcount) // This validcount situation seems to let us know that the floors have already been drawn.
		{
			// Draw polyobject planes
			HWR_AddPolyObjectPlanes();
		}
#endif
	}
#endif

// Hurder ici se passe les choses INT32�essantes!
// on vient de tracer le sol et le plafond
// on trace �pr�ent d'abord les sprites et ensuite les murs
// hurdler: faux: on ajoute seulement les sprites, le murs sont trac� d'abord
	if (line)
	{
		// draw sprites first, coz they are clipped to the solidsegs of
		// subsectors more 'in front'
		HWR_AddSprites(gr_frontsector);

		//Hurdler: at this point validcount must be the same, but is not because
		//         gr_frontsector doesn't point anymore to sub->sector due to
		//         the call gr_frontsector = R_FakeFlat(...)
		//         if it's not done, the sprite is drawn more than once,
		//         what looks really bad with translucency or dynamic light,
		//         without talking about the overdraw of course.
		sub->sector->validcount = validcount;/// \todo fix that in a better way

		while (count--)
		{
#ifdef POLYOBJECTS
				if (!line->polyseg) // ignore segs that belong to polyobjects
#endif
				HWR_AddLine(line);
				line++;
		}
	}

	sub->validcount = validcount;
}

//
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.

#ifdef coolhack
//t;b;l;r
static fixed_t hackbbox[4];
//BOXTOP,
//BOXBOTTOM,
//BOXLEFT,
//BOXRIGHT
static boolean HWR_CheckHackBBox(fixed_t *bb)
{
	if (bb[BOXTOP] < hackbbox[BOXBOTTOM]) //y up
		return false;
	if (bb[BOXBOTTOM] > hackbbox[BOXTOP])
		return false;
	if (bb[BOXLEFT] > hackbbox[BOXRIGHT])
		return false;
	if (bb[BOXRIGHT] < hackbbox[BOXLEFT])
		return false;
	return true;
}
#endif

// BP: big hack for a test in lighning ref : 1249753487AB
fixed_t *hwbbox;

static void HWR_RenderBSPNode(INT32 bspnum)
{
	/*//GZDoom code
	if(bspnum == -1)
	{
		HWR_Subsector(subsectors);
		return;
	}
	while(!((size_t)bspnum&(~NF_SUBSECTOR))) // Keep going until found a subsector
	{
		node_t *bsp = &nodes[bspnum];

		// Decide which side the view point is on
		INT32 side = R_PointOnSide(dup_viewx, dup_viewy, bsp);

		// Recursively divide front space (toward the viewer)
		HWR_RenderBSPNode(bsp->children[side]);

		// Possibly divide back space (away from viewer)
		side ^= 1;

		if (!HWR_CheckBBox(bsp->bbox[side]))
			return;

		bspnum = bsp->children[side];
	}

	HWR_Subsector(bspnum-1);
*/
	node_t *bsp = &nodes[bspnum];

	// Decide which side the view point is on
	INT32 side;

	// Found a subsector?
	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum == -1)
		{
			//*(gr_drawsubsector_p++) = 0;
			HWR_Subsector(0);
		}
		else
		{
			//*(gr_drawsubsector_p++) = bspnum&(~NF_SUBSECTOR);
			HWR_Subsector(bspnum&(~NF_SUBSECTOR));
		}
		return;
	}

	// Decide which side the view point is on.
	side = R_PointOnSide(dup_viewx, dup_viewy, bsp);

	// BP: big hack for a test in lighning ref : 1249753487AB
	hwbbox = bsp->bbox[side];

	// Recursively divide front space.
	HWR_RenderBSPNode(bsp->children[side]);

	// Possibly divide back space.
	if (HWR_CheckBBox(bsp->bbox[side^1]))
	{
		// BP: big hack for a test in lighning ref : 1249753487AB
		hwbbox = bsp->bbox[side^1];
		HWR_RenderBSPNode(bsp->children[side^1]);
	}
}

/*
//
// Clear 'stack' of subsectors to draw
//
static void HWR_ClearDrawSubsectors(void)
{
	gr_drawsubsector_p = gr_drawsubsectors;
}

//
// Draw subsectors pushed on the drawsubsectors 'stack', back to front
//
static void HWR_RenderSubsectors(void)
{
	while (gr_drawsubsector_p > gr_drawsubsectors)
	{
		HWR_RenderBSPNode(
		lastsubsec->nextsubsec = bspnum & (~NF_SUBSECTOR);
	}
}
*/

// ==========================================================================
//                                                              FROM R_MAIN.C
// ==========================================================================

//BP : exactely the same as R_InitTextureMapping
void HWR_InitTextureMapping(void)
{
	angle_t i;
	INT32 x;
	INT32 t;
	fixed_t focallength;
	fixed_t grcenterx;
	fixed_t grcenterxfrac;
	INT32 grviewwidth;

#define clipanglefov (FIELDOFVIEW>>ANGLETOFINESHIFT)

	grviewwidth = vid.width;
	grcenterx = grviewwidth/2;
	grcenterxfrac = grcenterx<<FRACBITS;

	// Use tangent table to generate viewangletox:
	//  viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv(grcenterxfrac,
		FINETANGENT(FINEANGLES/4+clipanglefov/2));

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > FRACUNIT*2)
			t = -1;
		else if (FINETANGENT(i) < -FRACUNIT*2)
			t = grviewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (grcenterxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > grviewwidth+1)
				t = grviewwidth+1;
		}
		gr_viewangletox[i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//  xtoviewangle will give the smallest view angle
	//  that maps to x.
	for (x = 0; x <= grviewwidth; x++)
	{
		i = 0;
		while (gr_viewangletox[i]>x)
			i++;
		gr_xtoviewangle[x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (gr_viewangletox[i] == -1)
			gr_viewangletox[i] = 0;
		else if (gr_viewangletox[i] == grviewwidth+1)
			gr_viewangletox[i]  = grviewwidth;
	}

	gr_clipangle = gr_xtoviewangle[0];
}

// ==========================================================================
// gr_things.c
// ==========================================================================

// sprites are drawn after all wall and planes are rendered, so that
// sprite translucency effects apply on the rendered view (instead of the background sky!!)

static UINT32 gr_visspritecount;
static gr_vissprite_t *gr_visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS] = {NULL};

// --------------------------------------------------------------------------
// HWR_ClearSprites
// Called at frame start.
// --------------------------------------------------------------------------
static void HWR_ClearSprites(void)
{
	gr_visspritecount = 0;
}

// --------------------------------------------------------------------------
// HWR_NewVisSprite
// --------------------------------------------------------------------------
static gr_vissprite_t gr_overflowsprite;

static gr_vissprite_t *HWR_GetVisSprite(UINT32 num)
{
		UINT32 chunk = num >> VISSPRITECHUNKBITS;

		// Allocate chunk if necessary
		if (!gr_visspritechunks[chunk])
			Z_Malloc(sizeof(gr_vissprite_t) * VISSPRITESPERCHUNK, PU_LEVEL, &gr_visspritechunks[chunk]);

		return gr_visspritechunks[chunk] + (num & VISSPRITEINDEXMASK);
}

static gr_vissprite_t *HWR_NewVisSprite(void)
{
	if (gr_visspritecount == MAXVISSPRITES)
		return &gr_overflowsprite;

	return HWR_GetVisSprite(gr_visspritecount++);
}

// Finds a floor through which light does not pass.
static fixed_t HWR_OpaqueFloorAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height)
{
	const sector_t *sec = R_PointInSubsector(x, y)->sector;
	fixed_t floorz = sec->floorheight;

	if (sec->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		const fixed_t thingtop = z + height;

		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS)
			|| !(rover->flags & FF_RENDERPLANES)
			|| rover->flags & FF_TRANSLUCENT
			|| rover->flags & FF_FOG
			|| rover->flags & FF_INVERTPLANES)
				continue;

			delta1 = z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if (*rover->topheight > floorz && abs(delta1) < abs(delta2))
				floorz = *rover->topheight;
		}
	}

	return floorz;
}

//
// HWR_DoCulling
// Hardware version of R_DoCulling
// (see r_main.c)
static boolean HWR_DoCulling(line_t *cullheight, line_t *viewcullheight, float vz, float bottomh, float toph)
{
	float cullplane;

	if (!cullheight)
		return false;

	cullplane = FIXED_TO_FLOAT(cullheight->frontsector->floorheight);
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

static void HWR_DrawSpriteShadow(gr_vissprite_t *spr, GLPatch_t *gpatch, float this_scale)
{
	FOutVector swallVerts[4];
	FSurfaceInfo sSurf;
	fixed_t floorheight, mobjfloor;
	float offset = 0;

	mobjfloor = HWR_OpaqueFloorAtPos(
		spr->mobj->x, spr->mobj->y,
		spr->mobj->z, spr->mobj->height);
	if (cv_shadowoffs.value)
	{
		angle_t shadowdir;

		// Set direction
		if (splitscreen && stplyr == &players[secondarydisplayplayer])
			shadowdir = localangle2 + FixedAngle(cv_cam2_rotate.value);
		else
			shadowdir = localangle + FixedAngle(cv_cam_rotate.value);

		// Find floorheight
		floorheight = HWR_OpaqueFloorAtPos(
			spr->mobj->x + P_ReturnThrustX(spr->mobj, shadowdir, spr->mobj->z - mobjfloor),
			spr->mobj->y + P_ReturnThrustY(spr->mobj, shadowdir, spr->mobj->z - mobjfloor),
			spr->mobj->z, spr->mobj->height);

		// The shadow is falling ABOVE it's mobj?
		// Don't draw it, then!
		if (spr->mobj->z < floorheight)
			return;
		else
		{
			fixed_t floorz;
			floorz = HWR_OpaqueFloorAtPos(
				spr->mobj->x + P_ReturnThrustX(spr->mobj, shadowdir, spr->mobj->z - floorheight),
				spr->mobj->y + P_ReturnThrustY(spr->mobj, shadowdir, spr->mobj->z - floorheight),
				spr->mobj->z, spr->mobj->height);
			// The shadow would be falling on a wall? Don't draw it, then.
			// Would draw midair otherwise.
			if (floorz < floorheight)
				return;
		}

		floorheight = FixedInt(spr->mobj->z - floorheight);

		offset = floorheight;
	}
	else
		floorheight = FixedInt(spr->mobj->z - mobjfloor);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	// x1/x2 were already scaled in HWR_ProjectSprite
	// First match the normal sprite
	swallVerts[0].x = swallVerts[3].x = spr->x1;
	swallVerts[2].x = swallVerts[1].x = spr->x2;
	swallVerts[0].z = swallVerts[3].z = spr->z1;
	swallVerts[2].z = swallVerts[1].z = spr->z2;

	if (spr->mobj && this_scale != 1.0f)
	{
		// Always a pixel above the floor, perfectly flat.
		swallVerts[0].y = swallVerts[1].y = swallVerts[2].y = swallVerts[3].y = spr->ty - gpatch->topoffset * this_scale - (floorheight+3);

		// Now transform the TOP vertices along the floor in the direction of the camera
		swallVerts[3].x = spr->x1 + ((gpatch->height * this_scale) + offset) * gr_viewcos;
		swallVerts[2].x = spr->x2 + ((gpatch->height * this_scale) + offset) * gr_viewcos;
		swallVerts[3].z = spr->z1 + ((gpatch->height * this_scale) + offset) * gr_viewsin;
		swallVerts[2].z = spr->z2 + ((gpatch->height * this_scale) + offset) * gr_viewsin;
	}
	else
	{
		// Always a pixel above the floor, perfectly flat.
		swallVerts[0].y = swallVerts[1].y = swallVerts[2].y = swallVerts[3].y = spr->ty - gpatch->topoffset - (floorheight+3);

		// Now transform the TOP vertices along the floor in the direction of the camera
		swallVerts[3].x = spr->x1 + (gpatch->height + offset) * gr_viewcos;
		swallVerts[2].x = spr->x2 + (gpatch->height + offset) * gr_viewcos;
		swallVerts[3].z = spr->z1 + (gpatch->height + offset) * gr_viewsin;
		swallVerts[2].z = spr->z2 + (gpatch->height + offset) * gr_viewsin;
	}

	// We also need to move the bottom ones away when shadowoffs is on
	if (cv_shadowoffs.value)
	{
		swallVerts[0].x = spr->x1 + offset * gr_viewcos;
		swallVerts[1].x = spr->x2 + offset * gr_viewcos;
		swallVerts[0].z = spr->z1 + offset * gr_viewsin;
		swallVerts[1].z = spr->z2 + offset * gr_viewsin;
	}

	if (spr->flip)
	{
		swallVerts[0].sow = swallVerts[3].sow = gpatch->max_s;
		swallVerts[2].sow = swallVerts[1].sow = 0;
	}
	else
	{
		swallVerts[0].sow = swallVerts[3].sow = 0;
		swallVerts[2].sow = swallVerts[1].sow = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		swallVerts[3].tow = swallVerts[2].tow = gpatch->max_t;
		swallVerts[0].tow = swallVerts[1].tow = 0;
	}
	else
	{
		swallVerts[3].tow = swallVerts[2].tow = 0;
		swallVerts[0].tow = swallVerts[1].tow = gpatch->max_t;
	}

	sSurf.FlatColor.s.red = 0x00;
	sSurf.FlatColor.s.blue = 0x00;
	sSurf.FlatColor.s.green = 0x00;

	/*if (spr->mobj->frame & FF_TRANSMASK || spr->mobj->flags2 & MF2_SHADOW)
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{
			INT32 light = R_GetPlaneLight(sector, spr->mobj->floorz, false);

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel;

			if (sector->lightlist[light].extra_colormap)
				colormap = sector->lightlist[light].extra_colormap;
		}
		else
		{
			lightlevel = sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		if (colormap)
			sSurf.FlatColor.rgba = HWR_Lighting(lightlevel/2, colormap->rgba, colormap->fadergba, false, true);
		else
			sSurf.FlatColor.rgba = HWR_Lighting(lightlevel/2, NORMALFOG, FADEFOG, false, true);
	}*/

	// shadow is always half as translucent as the sprite itself
	if (!cv_translucency.value) // use default translucency (main sprite won't have any translucency)
		sSurf.FlatColor.s.alpha = 0x80; // default
	else if (spr->mobj->flags2 & MF2_SHADOW)
		sSurf.FlatColor.s.alpha = 0x20;
	else if (spr->mobj->frame & FF_TRANSMASK)
	{
		HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &sSurf);
		sSurf.FlatColor.s.alpha /= 2; //cut alpha in half!
	}
	else
		sSurf.FlatColor.s.alpha = 0x80; // default

	if (sSurf.FlatColor.s.alpha > floorheight/4)
	{
		sSurf.FlatColor.s.alpha = (UINT8)(sSurf.FlatColor.s.alpha - floorheight/4);
		HWD.pfnDrawPolygon(&sSurf, swallVerts, 4, PF_Translucent|PF_Modulated|PF_Clip);
	}
}

static void HWR_SplitSprite(gr_vissprite_t *spr)
{
	float this_scale = 1.0f;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch;
	FSurfaceInfo Surf;
	const boolean hires = (spr->mobj && spr->mobj->skin && ((skin_t *)spr->mobj->skin)->flags & SF_HIRES);
	extracolormap_t *colormap;
	FUINT lightlevel;
	FBITFIELD blend = 0;
	UINT8 alpha;

	INT32 i;
	float realtop, realbot, top, bot;
	float towtop, towbot, towmult;
	float bheight;
	const sector_t *sector = spr->mobj->subsector->sector;
	const lightlist_t *list = sector->lightlist;
#ifdef ESLOPE
	float endrealtop, endrealbot, endtop, endbot;
	float endbheight;
	fixed_t temp;
	fixed_t v1x, v1y, v2x, v2y;
#endif

	this_scale = FIXED_TO_FLOAT(spr->mobj->scale);

	if (hires)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)spr->mobj->skin)->highresscale);

	gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// Draw shadow BEFORE sprite
	if (cv_shadow.value // Shadows enabled
		&& (spr->mobj->flags & (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY)) != (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY) // Ceiling scenery have no shadow.
		&& !(spr->mobj->flags2 & MF2_DEBRIS) // Debris have no corona or shadow.
#ifdef ALAM_LIGHTING
		&& !(t_lspr[spr->mobj->sprite]->type // Things with dynamic lights have no shadow.
		&& (!spr->mobj->player || spr->mobj->player->powers[pw_super])) // Except for non-super players.
#endif
		&& (spr->mobj->z >= spr->mobj->floorz)) // Without this, your shadow shows on the floor, even after you die and fall through the ground.
	{
		////////////////////
		// SHADOW SPRITE! //
		////////////////////
		HWR_DrawSpriteShadow(spr, gpatch, this_scale);
	}

	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	wallVerts[2].y = wallVerts[3].y = spr->ty;
	if (spr->mobj && this_scale != 1.0f)
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height * this_scale;
	else
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	v1x = FLOAT_TO_FIXED(spr->x1);
	v1y = FLOAT_TO_FIXED(spr->z1);
	v2x = FLOAT_TO_FIXED(spr->x2);
	v2y = FLOAT_TO_FIXED(spr->z2);

	if (spr->flip)
	{
		wallVerts[0].sow = wallVerts[3].sow = gpatch->max_s;
		wallVerts[2].sow = wallVerts[1].sow = 0;
	}else{
		wallVerts[0].sow = wallVerts[3].sow = 0;
		wallVerts[2].sow = wallVerts[1].sow = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		wallVerts[3].tow = wallVerts[2].tow = gpatch->max_t;
		wallVerts[0].tow = wallVerts[1].tow = 0;
	}else{
		wallVerts[3].tow = wallVerts[2].tow = 0;
		wallVerts[0].tow = wallVerts[1].tow = gpatch->max_t;
	}

	realtop = top = wallVerts[3].y;
	realbot = bot = wallVerts[0].y;
	towtop = wallVerts[3].tow;
	towbot = wallVerts[0].tow;
	towmult = (towbot - towtop) / (top - bot);

#ifdef ESLOPE
	endrealtop = endtop = wallVerts[2].y;
	endrealbot = endbot = wallVerts[1].y;
#endif

	if (!cv_translucency.value) // translucency disabled
	{
		Surf.FlatColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}
	else if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.FlatColor.s.alpha = 0x40;
		blend = PF_Translucent;
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
		blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
	else
	{
		// BP: i agree that is little better in environement but it don't
		//     work properly under glide nor with fogcolor to ffffff :(
		// Hurdler: PF_Environement would be cool, but we need to fix
		//          the issue with the fog before
		Surf.FlatColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}

	alpha = Surf.FlatColor.s.alpha;

	// Start with the lightlevel and colormap from the top of the sprite
	lightlevel = *list[sector->numlights - 1].lightlevel;
	colormap = list[sector->numlights - 1].extra_colormap;
	i = 0;
	temp = FLOAT_TO_FIXED(realtop);

	if (spr->mobj->frame & FF_FULLBRIGHT)
		lightlevel = 255;

#ifdef ESLOPE
	for (i = 1; i < sector->numlights; i++)
	{
		fixed_t h = sector->lightlist[i].slope ? P_GetZAt(sector->lightlist[i].slope, spr->mobj->x, spr->mobj->y)
					: sector->lightlist[i].height;
		if (h <= temp)
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *list[i-1].lightlevel;
			colormap = list[i-1].extra_colormap;
			break;
		}
	}
#else
	i = R_GetPlaneLight(sector, temp, false);
	if (!(spr->mobj->frame & FF_FULLBRIGHT))
		lightlevel = *list[i].lightlevel;
	colormap = list[i].extra_colormap;
#endif

	for (i = 0; i < sector->numlights; i++)
	{
#ifdef ESLOPE
		if (endtop < endrealbot)
#endif
		if (top < realbot)
			return;

		// even if we aren't changing colormap or lightlevel, we still need to continue drawing down the sprite
		if (!(list[i].flags & FF_NOSHADE) && (list[i].flags & FF_CUTSPRITES))
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *list[i].lightlevel;
			colormap = list[i].extra_colormap;
		}

#ifdef ESLOPE
		if (i + 1 < sector->numlights)
		{
			if (list[i+1].slope)
			{
				temp = P_GetZAt(list[i+1].slope, v1x, v1y);
				bheight = FIXED_TO_FLOAT(temp);
				temp = P_GetZAt(list[i+1].slope, v2x, v2y);
				endbheight = FIXED_TO_FLOAT(temp);
			}
			else
				bheight = endbheight = FIXED_TO_FLOAT(list[i+1].height);
		}
		else
		{
			bheight = realbot;
			endbheight = endrealbot;
		}
#else
		if (i + 1 < sector->numlights)
		{
			bheight = FIXED_TO_FLOAT(list[i+1].height);
		}
		else
		{
			bheight = realbot;
		}
#endif

#ifdef ESLOPE
		if (endbheight >= endtop)
#endif
		if (bheight >= top)
			continue;

		bot = bheight;

		if (bot < realbot)
			bot = realbot;

#ifdef ESLOPE
		endbot = endbheight;

		if (endbot < endrealbot)
			endbot = endrealbot;
#endif

#ifdef ESLOPE
		wallVerts[3].tow = towtop + ((realtop - top) * towmult);
		wallVerts[2].tow = towtop + ((endrealtop - endtop) * towmult);
		wallVerts[0].tow = towtop + ((realtop - bot) * towmult);
		wallVerts[1].tow = towtop + ((endrealtop - endbot) * towmult);

		wallVerts[3].y = top;
		wallVerts[2].y = endtop;
		wallVerts[0].y = bot;
		wallVerts[1].y = endbot;
#else
		wallVerts[3].tow = wallVerts[2].tow = towtop + ((realtop - top) * towmult);
		wallVerts[0].tow = wallVerts[1].tow = towtop + ((realtop - bot) * towmult);

		wallVerts[2].y = wallVerts[3].y = top;
		wallVerts[0].y = wallVerts[1].y = bot;
#endif

		if (colormap)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, colormap->rgba, colormap->fadergba, false, false);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);

		Surf.FlatColor.s.alpha = alpha;

		HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated|PF_Clip);

		top = bot;
#ifdef ESLOPE
		endtop = endbot;
#endif
	}

	bot = realbot;
#ifdef ESLOPE
	endbot = endrealbot;
	if (endtop <= endrealbot)
#endif
	if (top <= realbot)
		return;

	// If we're ever down here, somehow the above loop hasn't draw all the light levels of sprite
#ifdef ESLOPE
	wallVerts[3].tow = towtop + ((realtop - top) * towmult);
	wallVerts[2].tow = towtop + ((endrealtop - endtop) * towmult);
	wallVerts[0].tow = towtop + ((realtop - bot) * towmult);
	wallVerts[1].tow = towtop + ((endrealtop - endbot) * towmult);

	wallVerts[3].y = top;
	wallVerts[2].y = endtop;
	wallVerts[0].y = bot;
	wallVerts[1].y = endbot;
#else
	wallVerts[3].tow = wallVerts[2].tow = towtop + ((realtop - top) * towmult);
	wallVerts[0].tow = wallVerts[1].tow = towtop + ((realtop - bot) * towmult);

	wallVerts[2].y = wallVerts[3].y = top;
	wallVerts[0].y = wallVerts[1].y = bot;
#endif

	if (colormap)
		Surf.FlatColor.rgba = HWR_Lighting(lightlevel, colormap->rgba, colormap->fadergba, false, false);
	else
		Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);

	Surf.FlatColor.s.alpha = alpha;

	HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated|PF_Clip);
}

// -----------------+
// HWR_DrawSprite   : Draw flat sprites
//                  : (monsters, bonuses, weapons, lights, ...)
// Returns          :
// -----------------+
static void HWR_DrawSprite(gr_vissprite_t *spr)
{
	float this_scale = 1.0f;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;
	const boolean hires = (spr->mobj && spr->mobj->skin && ((skin_t *)spr->mobj->skin)->flags & SF_HIRES);
	if (spr->mobj)
		this_scale = FIXED_TO_FLOAT(spr->mobj->scale);
	if (hires)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)spr->mobj->skin)->highresscale);

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;

	if (spr->mobj->subsector->sector->numlights)
	{
		HWR_SplitSprite(spr);
		return;
	}

	// cache sprite graphics
	//12/12/99: Hurdler:
	//          OK, I don't change anything for MD2 support because I want to be
	//          sure to do it the right way. So actually, we keep normal sprite
	//          in memory and we add the md2 model if it exists for that sprite

	gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

#ifdef ALAM_LIGHTING
	if (!(spr->mobj->flags2 & MF2_DEBRIS) && (spr->mobj->sprite != SPR_PLAY ||
	 (spr->mobj->player && spr->mobj->player->powers[pw_super])))
		HWR_DL_AddLight(spr, gpatch);
#endif

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	// these were already scaled in HWR_ProjectSprite
	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->ty;
	if (spr->mobj && this_scale != 1.0f)
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height * this_scale;
	else
		wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	if (spr->flip)
	{
		wallVerts[0].sow = wallVerts[3].sow = gpatch->max_s;
		wallVerts[2].sow = wallVerts[1].sow = 0;
	}else{
		wallVerts[0].sow = wallVerts[3].sow = 0;
		wallVerts[2].sow = wallVerts[1].sow = gpatch->max_s;
	}

	// flip the texture coords (look familiar?)
	if (spr->vflip)
	{
		wallVerts[3].tow = wallVerts[2].tow = gpatch->max_t;
		wallVerts[0].tow = wallVerts[1].tow = 0;
	}else{
		wallVerts[3].tow = wallVerts[2].tow = 0;
		wallVerts[0].tow = wallVerts[1].tow = gpatch->max_t;
	}

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// Draw shadow BEFORE sprite
	if (cv_shadow.value // Shadows enabled
		&& (spr->mobj->flags & (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY)) != (MF_SCENERY|MF_SPAWNCEILING|MF_NOGRAVITY) // Ceiling scenery have no shadow.
		&& !(spr->mobj->flags2 & MF2_DEBRIS) // Debris have no corona or shadow.
#ifdef ALAM_LIGHTING
		&& !(t_lspr[spr->mobj->sprite]->type // Things with dynamic lights have no shadow.
		&& (!spr->mobj->player || spr->mobj->player->powers[pw_super])) // Except for non-super players.
#endif
		&& (spr->mobj->z >= spr->mobj->floorz)) // Without this, your shadow shows on the floor, even after you die and fall through the ground.
	{
		////////////////////
		// SHADOW SPRITE! //
		////////////////////
		HWR_DrawSpriteShadow(spr, gpatch, this_scale);
	}

	// This needs to be AFTER the shadows so that the regular sprites aren't drawn completely black.
	// sprite lighting by modulating the RGB components
	/// \todo coloured

	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (!(spr->mobj->frame & FF_FULLBRIGHT))
			lightlevel = sector->lightlevel;

		if (colormap)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, colormap->rgba, colormap->fadergba, false, false);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);
	}

	{
		FBITFIELD blend = 0;
		if (!cv_translucency.value) // translucency disabled
		{
			Surf.FlatColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}
		else if (spr->mobj->flags2 & MF2_SHADOW)
		{
			Surf.FlatColor.s.alpha = 0x40;
			blend = PF_Translucent;
		}
		else if (spr->mobj->frame & FF_TRANSMASK)
			blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else
		{
			// BP: i agree that is little better in environement but it don't
			//     work properly under glide nor with fogcolor to ffffff :(
			// Hurdler: PF_Environement would be cool, but we need to fix
			//          the issue with the fog before
			Surf.FlatColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}

		HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated|PF_Clip);
	}
}

#ifdef HWPRECIP
// Sprite drawer for precipitation
static inline void HWR_DrawPrecipitationSprite(gr_vissprite_t *spr)
{
	FBITFIELD blend = 0;
	FOutVector wallVerts[4];
	GLPatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;

	if (!spr->mobj)
		return;

	if (!spr->mobj->subsector)
		return;

	// cache sprite graphics
	gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	// create the sprite billboard
	//
	//  3--2
	//  | /|
	//  |/ |
	//  0--1
	wallVerts[0].x = wallVerts[3].x = spr->x1;
	wallVerts[2].x = wallVerts[1].x = spr->x2;
	wallVerts[2].y = wallVerts[3].y = spr->ty;
	wallVerts[0].y = wallVerts[1].y = spr->ty - gpatch->height;

	// make a wall polygon (with 2 triangles), using the floor/ceiling heights,
	// and the 2d map coords of start/end vertices
	wallVerts[0].z = wallVerts[3].z = spr->z1;
	wallVerts[1].z = wallVerts[2].z = spr->z2;

	wallVerts[0].sow = wallVerts[3].sow = 0;
	wallVerts[2].sow = wallVerts[1].sow = gpatch->max_s;

	wallVerts[3].tow = wallVerts[2].tow = 0;
	wallVerts[0].tow = wallVerts[1].tow = gpatch->max_t;

	// cache the patch in the graphics card memory
	//12/12/99: Hurdler: same comment as above (for md2)
	//Hurdler: 25/04/2000: now support colormap in hardware mode
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false); // Always use the light at the top instead of whatever I was doing before

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel;

			if (sector->lightlist[light].extra_colormap)
				colormap = sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		if (colormap)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, colormap->rgba, colormap->fadergba, false, false);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);
	}

	if (spr->mobj->flags2 & MF2_SHADOW)
	{
		Surf.FlatColor.s.alpha = 0x40;
		blend = PF_Translucent;
	}
	else if (spr->mobj->frame & FF_TRANSMASK)
		blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
	else
	{
		// BP: i agree that is little better in environement but it don't
		//     work properly under glide nor with fogcolor to ffffff :(
		// Hurdler: PF_Environement would be cool, but we need to fix
		//          the issue with the fog before
		Surf.FlatColor.s.alpha = 0xFF;
		blend = PF_Translucent|PF_Occlude;
	}

	HWD.pfnDrawPolygon(&Surf, wallVerts, 4, blend|PF_Modulated|PF_Clip);
}
#endif

// --------------------------------------------------------------------------
// Sort vissprites by distance
// --------------------------------------------------------------------------
static gr_vissprite_t gr_vsprsortedhead;

static void HWR_SortVisSprites(void)
{
	UINT32 i;
	gr_vissprite_t *ds, *dsprev, *dsnext, *dsfirst;
	gr_vissprite_t *best = NULL;
	gr_vissprite_t unsorted;
	float bestdist = 0.0f;
	INT32 bestdispoffset = 0;

	if (!gr_visspritecount)
		return;

	dsfirst = HWR_GetVisSprite(0);

	for (i = 0, dsnext = dsfirst, ds = NULL; i < gr_visspritecount; i++)
	{
		dsprev = ds;
		ds = dsnext;
		if (i < gr_visspritecount - 1) dsnext = HWR_GetVisSprite(i + 1);

		ds->next = dsnext;
		ds->prev = dsprev;
	}

	// Fix first and last. ds still points to the last one after the loop
	dsfirst->prev = &unsorted;
	unsorted.next = dsfirst;
	if (ds)
		ds->next = &unsorted;
	unsorted.prev = ds;

	// pull the vissprites out by scale
	gr_vsprsortedhead.next = gr_vsprsortedhead.prev = &gr_vsprsortedhead;
	for (i = 0; i < gr_visspritecount; i++)
	{
		best = NULL;
		for (ds = unsorted.next; ds != &unsorted; ds = ds->next)
		{
			if (!best || ds->tz > bestdist)
			{
				bestdist = ds->tz;
				bestdispoffset = ds->dispoffset;
				best = ds;
			}
			// order visprites of same scale by dispoffset, smallest first
			else if (ds->tz == bestdist && ds->dispoffset < bestdispoffset)
			{
				bestdispoffset = ds->dispoffset;
				best = ds;
			}
		}
		if (best)
		{
			best->next->prev = best->prev;
			best->prev->next = best->next;
			best->next = &gr_vsprsortedhead;
			best->prev = gr_vsprsortedhead.prev;
		}
		gr_vsprsortedhead.prev->next = best;
		gr_vsprsortedhead.prev = best;
	}

	// Sryder:	Oh boy, while it's nice having ALL the sprites sorted properly, it fails when we bring MD2's into the
	//			mix and they want to be translucent. So let's place all the translucent sprites and MD2's AFTER
	//			everything else, but still ordered of course, the depth buffer can handle the opaque ones plenty fine.
	//			We just need to move all translucent ones to the end in order
	// TODO:	Fully sort all sprites and MD2s with walls and floors, this part will be unnecessary after that
	best = gr_vsprsortedhead.next;
	for (i = 0; i < gr_visspritecount; i++)
	{
		if ((best->mobj->flags2 & MF2_SHADOW) || (best->mobj->frame & FF_TRANSMASK))
		{
			if (best == gr_vsprsortedhead.next)
			{
				gr_vsprsortedhead.next = best->next;
			}
			best->prev->next = best->next;
			best->next->prev = best->prev;
			best->prev = gr_vsprsortedhead.prev;
			gr_vsprsortedhead.prev->next = best;
			gr_vsprsortedhead.prev = best;
			ds = best;
			best = best->next;
			ds->next = &gr_vsprsortedhead;
		}
		else
			best = best->next;
	}
}

// A drawnode is something that points to a 3D floor, 3D side, or masked
// middle texture. This is used for sorting with sprites.
typedef struct
{
	wallVert3D    wallVerts[4];
	FSurfaceInfo  Surf;
	INT32         texnum;
	FBITFIELD     blend;
	INT32         drawcount;
#ifndef SORTING
	fixed_t       fixedheight;
#endif
	boolean fogwall;
	INT32 lightlevel;
	extracolormap_t *wallcolormap; // Doing the lighting in HWR_RenderWall now for correct fog after sorting
} wallinfo_t;

static wallinfo_t *wallinfo = NULL;
static size_t numwalls = 0; // a list of transparent walls to be drawn

static void HWR_RenderWall(wallVert3D   *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap);

#define MAX_TRANSPARENTWALL 256

typedef struct
{
	extrasubsector_t *xsub;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	lumpnum_t lumpnum;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	boolean fogplane;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} planeinfo_t;

static size_t numplanes = 0; // a list of transparent floors to be drawn
static planeinfo_t *planeinfo = NULL;

typedef struct
{
	polyobj_t *polysector;
	boolean isceiling;
	fixed_t fixedheight;
	INT32 lightlevel;
	lumpnum_t lumpnum;
	INT32 alpha;
	sector_t *FOFSector;
	FBITFIELD blend;
	extracolormap_t *planecolormap;
	INT32 drawcount;
} polyplaneinfo_t;

static size_t numpolyplanes = 0; // a list of transparent poyobject floors to be drawn
static polyplaneinfo_t *polyplaneinfo = NULL;

#ifndef SORTING
size_t numfloors = 0;
#else
//static floorinfo_t *floorinfo = NULL;
//static size_t numfloors = 0;
//Hurdler: 3D water sutffs
typedef struct gr_drawnode_s
{
	planeinfo_t *plane;
	polyplaneinfo_t *polyplane;
	wallinfo_t *wall;
	gr_vissprite_t *sprite;

//	struct gr_drawnode_s *next;
//	struct gr_drawnode_s *prev;
} gr_drawnode_t;

static INT32 drawcount = 0;

#define MAX_TRANSPARENTFLOOR 512

// This will likely turn into a copy of HWR_Add3DWater and replace it.
void HWR_AddTransparentFloor(lumpnum_t lumpnum, extrasubsector_t *xsub, boolean isceiling,
	fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, boolean fogplane, extracolormap_t *planecolormap)
{
	static size_t allocedplanes = 0;

	// Force realloc if buffer has been freed
	if (!planeinfo)
		allocedplanes = 0;

	if (allocedplanes < numplanes + 1)
	{
		allocedplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(planeinfo, allocedplanes * sizeof (*planeinfo), PU_LEVEL, &planeinfo);
	}

	planeinfo[numplanes].isceiling = isceiling;
	planeinfo[numplanes].fixedheight = fixedheight;
	planeinfo[numplanes].lightlevel = lightlevel;
	planeinfo[numplanes].lumpnum = lumpnum;
	planeinfo[numplanes].xsub = xsub;
	planeinfo[numplanes].alpha = alpha;
	planeinfo[numplanes].FOFSector = FOFSector;
	planeinfo[numplanes].blend = blend;
	planeinfo[numplanes].fogplane = fogplane;
	planeinfo[numplanes].planecolormap = planecolormap;
	planeinfo[numplanes].drawcount = drawcount++;

	numplanes++;
}

// Adding this for now until I can create extrasubsector info for polyobjects
// When that happens it'll just be done through HWR_AddTransparentFloor and HWR_RenderPlane
void HWR_AddTransparentPolyobjectFloor(lumpnum_t lumpnum, polyobj_t *polysector, boolean isceiling,
	fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector, FBITFIELD blend, extracolormap_t *planecolormap)
{
	static size_t allocedpolyplanes = 0;

	// Force realloc if buffer has been freed
	if (!polyplaneinfo)
		allocedpolyplanes = 0;

	if (allocedpolyplanes < numpolyplanes + 1)
	{
		allocedpolyplanes += MAX_TRANSPARENTFLOOR;
		Z_Realloc(polyplaneinfo, allocedpolyplanes * sizeof (*polyplaneinfo), PU_LEVEL, &polyplaneinfo);
	}

	polyplaneinfo[numpolyplanes].isceiling = isceiling;
	polyplaneinfo[numpolyplanes].fixedheight = fixedheight;
	polyplaneinfo[numpolyplanes].lightlevel = lightlevel;
	polyplaneinfo[numpolyplanes].lumpnum = lumpnum;
	polyplaneinfo[numpolyplanes].polysector = polysector;
	polyplaneinfo[numpolyplanes].alpha = alpha;
	polyplaneinfo[numpolyplanes].FOFSector = FOFSector;
	polyplaneinfo[numpolyplanes].blend = blend;
	polyplaneinfo[numpolyplanes].planecolormap = planecolormap;
	polyplaneinfo[numpolyplanes].drawcount = drawcount++;
	numpolyplanes++;
}

//
// HWR_CreateDrawNodes
// Creates and sorts a list of drawnodes for the scene being rendered.
static void HWR_CreateDrawNodes(void)
{
	UINT32 i = 0, p = 0, prev = 0, loop;
	const fixed_t pviewz = dup_viewz;

	// Dump EVERYTHING into a huge drawnode list. Then we'll sort it!
	// Could this be optimized into _AddTransparentWall/_AddTransparentPlane?
	// Hell yes! But sort algorithm must be modified to use a linked list.
	gr_drawnode_t *sortnode = Z_Calloc((sizeof(planeinfo_t)*numplanes)
									+ (sizeof(polyplaneinfo_t)*numpolyplanes)
									+ (sizeof(wallinfo_t)*numwalls)
									,PU_STATIC, NULL);
	// todo:
	// However, in reality we shouldn't be re-copying and shifting all this information
	// that is already lying around. This should all be in some sort of linked list or lists.
	size_t *sortindex = Z_Calloc(sizeof(size_t) * (numplanes + numpolyplanes + numwalls), PU_STATIC, NULL);

	// If true, swap the draw order.
	boolean shift = false;

	for (i = 0; i < numplanes; i++, p++)
	{
		sortnode[p].plane = &planeinfo[i];
		sortindex[p] = p;
	}

	for (i = 0; i < numpolyplanes; i++, p++)
	{
		sortnode[p].polyplane = &polyplaneinfo[i];
		sortindex[p] = p;
	}

	for (i = 0; i < numwalls; i++, p++)
	{
		sortnode[p].wall = &wallinfo[i];
		sortindex[p] = p;
	}

	// p is the number of stuff to sort

	// Add the 3D floors, thicksides, and masked textures...
	// Instead of going through drawsegs, we need to iterate
	// through the lists of masked textures and
	// translucent ffloors being drawn.

	// This is a bubble sort! Wahoo!

	// Stuff is sorted:
	//      sortnode[sortindex[0]]   = farthest away
	//      sortnode[sortindex[p-1]] = closest
	// "i" should be closer. "prev" should be further.
	// The lower drawcount is, the further it is from the screen.

	for (loop = 0; loop < p; loop++)
	{
		for (i = 1; i < p; i++)
		{
			prev = i-1;
			if (sortnode[sortindex[i]].plane)
			{
				// What are we comparing it with?
				if (sortnode[sortindex[prev]].plane)
				{
					// Plane (i) is further away than plane (prev)
					if (ABS(sortnode[sortindex[i]].plane->fixedheight - pviewz) > ABS(sortnode[sortindex[prev]].plane->fixedheight - pviewz))
						shift = true;
				}
				if (sortnode[sortindex[prev]].polyplane)
				{
					// Plane (i) is further away than polyplane (prev)
					if (ABS(sortnode[sortindex[i]].plane->fixedheight - pviewz) > ABS(sortnode[sortindex[prev]].polyplane->fixedheight - pviewz))
						shift = true;
				}
				else if (sortnode[sortindex[prev]].wall)
				{
					// Plane (i) is further than wall (prev)
					if (sortnode[sortindex[i]].plane->drawcount > sortnode[sortindex[prev]].wall->drawcount)
						shift = true;
				}
			}
			else if (sortnode[sortindex[i]].polyplane)
			{
				// What are we comparing it with?
				if (sortnode[sortindex[prev]].plane)
				{
					// Plane (i) is further away than plane (prev)
					if (ABS(sortnode[sortindex[i]].polyplane->fixedheight - pviewz) > ABS(sortnode[sortindex[prev]].plane->fixedheight - pviewz))
						shift = true;
				}
				if (sortnode[sortindex[prev]].polyplane)
				{
					// Plane (i) is further away than polyplane (prev)
					if (ABS(sortnode[sortindex[i]].polyplane->fixedheight - pviewz) > ABS(sortnode[sortindex[prev]].polyplane->fixedheight - pviewz))
						shift = true;
				}
				else if (sortnode[sortindex[prev]].wall)
				{
					// Plane (i) is further than wall (prev)
					if (sortnode[sortindex[i]].polyplane->drawcount > sortnode[sortindex[prev]].wall->drawcount)
						shift = true;
				}
			}
			else if (sortnode[sortindex[i]].wall)
			{
				// What are we comparing it with?
				if (sortnode[sortindex[prev]].plane)
				{
					// Wall (i) is further than plane(prev)
					if (sortnode[sortindex[i]].wall->drawcount > sortnode[sortindex[prev]].plane->drawcount)
						shift = true;
				}
				if (sortnode[sortindex[prev]].polyplane)
				{
					// Wall (i) is further than polyplane(prev)
					if (sortnode[sortindex[i]].wall->drawcount > sortnode[sortindex[prev]].polyplane->drawcount)
						shift = true;
				}
				else if (sortnode[sortindex[prev]].wall)
				{
					// Wall (i) is further than wall (prev)
					if (sortnode[sortindex[i]].wall->drawcount > sortnode[sortindex[prev]].wall->drawcount)
						shift = true;
				}
			}

			if (shift)
			{
				size_t temp;

				temp = sortindex[prev];
				sortindex[prev] = sortindex[i];
				sortindex[i] = temp;

				shift = false;
			}

		} //i++
	} // loop++

	HWD.pfnSetTransform(&atransform);
	// Okay! Let's draw it all! Woo!
	for (i = 0; i < p; i++)
	{
		if (sortnode[sortindex[i]].plane)
		{
			// We aren't traversing the BSP tree, so make gr_frontsector null to avoid crashes.
			gr_frontsector = NULL;

			if (!(sortnode[sortindex[i]].plane->blend & PF_NoTexture))
				HWR_GetFlat(sortnode[sortindex[i]].plane->lumpnum);
			HWR_RenderPlane(NULL, sortnode[sortindex[i]].plane->xsub, sortnode[sortindex[i]].plane->isceiling, sortnode[sortindex[i]].plane->fixedheight, sortnode[sortindex[i]].plane->blend, sortnode[sortindex[i]].plane->lightlevel,
				sortnode[sortindex[i]].plane->lumpnum, sortnode[sortindex[i]].plane->FOFSector, sortnode[sortindex[i]].plane->alpha, sortnode[sortindex[i]].plane->fogplane, sortnode[sortindex[i]].plane->planecolormap);
		}
		else if (sortnode[sortindex[i]].polyplane)
		{
			// We aren't traversing the BSP tree, so make gr_frontsector null to avoid crashes.
			gr_frontsector = NULL;

			if (!(sortnode[sortindex[i]].polyplane->blend & PF_NoTexture))
				HWR_GetFlat(sortnode[sortindex[i]].polyplane->lumpnum);
			HWR_RenderPolyObjectPlane(sortnode[sortindex[i]].polyplane->polysector, sortnode[sortindex[i]].polyplane->isceiling, sortnode[sortindex[i]].polyplane->fixedheight, sortnode[sortindex[i]].polyplane->blend, sortnode[sortindex[i]].polyplane->lightlevel,
				sortnode[sortindex[i]].polyplane->lumpnum, sortnode[sortindex[i]].polyplane->FOFSector, sortnode[sortindex[i]].polyplane->alpha, sortnode[sortindex[i]].polyplane->planecolormap);
		}
		else if (sortnode[sortindex[i]].wall)
		{
			if (!(sortnode[sortindex[i]].wall->blend & PF_NoTexture))
				HWR_GetTexture(sortnode[sortindex[i]].wall->texnum);
			HWR_RenderWall(sortnode[sortindex[i]].wall->wallVerts, &sortnode[sortindex[i]].wall->Surf, sortnode[sortindex[i]].wall->blend, sortnode[sortindex[i]].wall->fogwall,
				sortnode[sortindex[i]].wall->lightlevel, sortnode[sortindex[i]].wall->wallcolormap);
		}
	}

	numwalls = 0;
	numplanes = 0;
	numpolyplanes = 0;

	// No mem leaks, please.
	Z_Free(sortnode);
	Z_Free(sortindex);
}

#endif

// --------------------------------------------------------------------------
//  Draw all vissprites
// --------------------------------------------------------------------------
#ifdef SORTING
// added the stransform so they can be switched as drawing happenes so MD2s and sprites are sorted correctly with each other
static void HWR_DrawSprites(void)
{
	if (gr_visspritecount > 0)
	{
		gr_vissprite_t *spr;

		// draw all vissprites back to front
		for (spr = gr_vsprsortedhead.next;
		     spr != &gr_vsprsortedhead;
		     spr = spr->next)
		{
#ifdef HWPRECIP
			if (spr->precip)
				HWR_DrawPrecipitationSprite(spr);
			else
#endif
				if (spr->mobj && spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
				{
					if (!cv_grmd2.value || md2_playermodels[(skin_t*)spr->mobj->skin-skins].notfound || md2_playermodels[(skin_t*)spr->mobj->skin-skins].scale < 0.0f)
						HWR_DrawSprite(spr);
					else
						HWR_DrawMD2(spr);
				}
				else
				{
					if (!cv_grmd2.value || md2_models[spr->mobj->sprite].notfound || md2_models[spr->mobj->sprite].scale < 0.0f)
						HWR_DrawSprite(spr);
					else
						HWR_DrawMD2(spr);
				}
		}
	}
}
#endif

// --------------------------------------------------------------------------
// HWR_AddSprites
// During BSP traversal, this adds sprites by sector.
// --------------------------------------------------------------------------
static UINT8 sectorlight;
static void HWR_AddSprites(sector_t *sec)
{
	mobj_t *thing;
#ifdef HWPRECIP
	precipmobj_t *precipthing;
#endif
	fixed_t approx_dist, limit_dist;

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if (sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	// sprite lighting
	sectorlight = sec->lightlevel & 0xff;

	// Handle all things in sector.
	// If a limit exists, handle things a tiny bit different.
	if ((limit_dist = (fixed_t)((maptol & TOL_NIGHTS) ? cv_drawdist_nights.value : cv_drawdist.value) << FRACBITS))
	{
		for (thing = sec->thinglist; thing; thing = thing->snext)
		{
			if (thing->sprite == SPR_NULL || thing->flags2 & MF2_DONTDRAW)
				continue;

			approx_dist = P_AproxDistance(viewx-thing->x, viewy-thing->y);

			if (approx_dist <= limit_dist)
				HWR_ProjectSprite(thing);
		}
	}
	else
	{
		// Draw everything in sector, no checks
		for (thing = sec->thinglist; thing; thing = thing->snext)
			if (!(thing->sprite == SPR_NULL || thing->flags2 & MF2_DONTDRAW))
				HWR_ProjectSprite(thing);
	}

#ifdef HWPRECIP
	// Someone seriously wants infinite draw distance for precipitation?
	if ((limit_dist = (fixed_t)cv_drawdist_precip.value << FRACBITS))
	{
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
		{
			if (precipthing->precipflags & PCF_INVISIBLE)
				continue;

			approx_dist = P_AproxDistance(viewx-precipthing->x, viewy-precipthing->y);

			if (approx_dist <= limit_dist)
				HWR_ProjectPrecipitationSprite(precipthing);
		}
	}
	else
	{
		// Draw everything in sector, no checks
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
			if (!(precipthing->precipflags & PCF_INVISIBLE))
				HWR_ProjectPrecipitationSprite(precipthing);
	}
#endif
}

// --------------------------------------------------------------------------
// HWR_ProjectSprite
//  Generates a vissprite for a thing if it might be visible.
// --------------------------------------------------------------------------
// BP why not use xtoviexangle/viewangletox like in bsp ?....
static void HWR_ProjectSprite(mobj_t *thing)
{
	gr_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	float this_scale;
	float gz, gzt;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lumpoff;
	unsigned rot;
	UINT8 flip;
	angle_t ang;
	INT32 heightsec, phs;

	if (!thing)
		return;
	else
		this_scale = FIXED_TO_FLOAT(thing->scale);

	// transform the origin point
	tr_x = FIXED_TO_FLOAT(thing->x) - gr_viewx;
	tr_y = FIXED_TO_FLOAT(thing->y) - gr_viewy;

	// rotation around vertical axis
	tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	// thing is behind view plane?
	if (tz < ZCLIP_PLANE && (!cv_grmd2.value || md2_models[thing->sprite].notfound == true)) //Yellow: Only MD2's dont disappear
		return;

	// The above can stay as it works for cutting sprites that are too close
	tr_x = FIXED_TO_FLOAT(thing->x);
	tr_y = FIXED_TO_FLOAT(thing->y);

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= numsprites)
		I_Error("HWR_ProjectSprite: invalid sprite number %i ", thing->sprite);
#endif

	rot = thing->frame&FF_FRAMEMASK;

	//Fab : 02-08-98: 'skin' override spritedef currently used for skin
	if (thing->skin && thing->sprite == SPR_PLAY)
		sprdef = &((skin_t *)thing->skin)->spritedef;
	else
		sprdef = &sprites[thing->sprite];

	if (rot >= sprdef->numframes)
	{
		CONS_Alert(CONS_ERROR, M_GetText("HWR_ProjectSprite: invalid sprite frame %s/%s for %s\n"),
			sizeu1(rot), sizeu2(sprdef->numframes), sprnames[thing->sprite]);
		thing->sprite = states[S_UNKNOWN].sprite;
		thing->frame = states[S_UNKNOWN].frame;
		sprdef = &sprites[thing->sprite];
		rot = thing->frame&FF_FRAMEMASK;
		thing->state->sprite = thing->sprite;
		thing->state->frame = thing->frame;
	}

	sprframe = &sprdef->spriteframes[rot];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("sprframes NULL for sprite %d\n", thing->sprite);
#endif

	if (sprframe->rotate)
	{
		// choose a different rotation based on player view
		ang = R_PointToAngle(thing->x, thing->y); // uses viewx,viewy
		rot = (ang-thing->angle+ANGLE_202h)>>29;
		//Fab: lumpid is the index for spritewidth,spriteoffset... tables
		lumpoff = sprframe->lumpid[rot];
		flip = sprframe->flip & (1<<rot);
	}
	else
	{
		// use single rotation for all views
		rot = 0;                        //Fab: for vis->patch below
		lumpoff = sprframe->lumpid[0];     //Fab: see note above
		flip = sprframe->flip; // Will only be 0x00 or 0xFF
	}

	if (thing->skin && ((skin_t *)thing->skin)->flags & SF_HIRES)
		this_scale = this_scale * FIXED_TO_FLOAT(((skin_t *)thing->skin)->highresscale);

	rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	if (flip)
	{
		x1 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset) * this_scale);
		x2 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset) * this_scale);
	}
	else
	{
		x1 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset) * this_scale);
		x2 = (FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset) * this_scale);
	}

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;

	if (thing->eflags & MFE_VERTICALFLIP)
	{
		gz = FIXED_TO_FLOAT(thing->z+thing->height) - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].topoffset) * this_scale;
		gzt = gz + FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height) * this_scale;
	}
	else
	{
		gzt = FIXED_TO_FLOAT(thing->z) + FIXED_TO_FLOAT(spritecachedinfo[lumpoff].topoffset) * this_scale;
		gz = gzt - FIXED_TO_FLOAT(spritecachedinfo[lumpoff].height) * this_scale;
	}

	if (thing->subsector->sector->cullheight)
	{
		if (HWR_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, gr_viewz, gz, gzt))
			return;
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1) // only clip things which are in special sectors
	{
		if (gr_viewz < FIXED_TO_FLOAT(sectors[phs].floorheight) ?
		FIXED_TO_FLOAT(thing->z) >= FIXED_TO_FLOAT(sectors[heightsec].floorheight) :
		gzt < FIXED_TO_FLOAT(sectors[heightsec].floorheight))
			return;
		if (gr_viewz > FIXED_TO_FLOAT(sectors[phs].ceilingheight) ?
		gzt < FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) && gr_viewz >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight) :
		FIXED_TO_FLOAT(thing->z) >= FIXED_TO_FLOAT(sectors[heightsec].ceilingheight))
			return;
	}

	// store information in a vissprite
	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;
	vis->tz = tz; // Keep tz for the simple sprite sorting that happens
	vis->dispoffset = thing->info->dispoffset; // Monster Iestyn: 23/11/15: HARDWARE SUPPORT AT LAST
	vis->patchlumpnum = sprframe->lumppat[rot];
	vis->flip = flip;
	vis->mobj = thing;

	//Hurdler: 25/04/2000: now support colormap in hardware mode
	if ((vis->mobj->flags & MF_BOSS) && (vis->mobj->flags2 & MF2_FRET) && (leveltime & 1)) // Bosses "flash"
	{
		if (vis->mobj->type == MT_CYBRAKDEMON)
			vis->colormap = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
		else if (vis->mobj->type == MT_METALSONIC_BATTLE)
			vis->colormap = R_GetTranslationColormap(TC_METALSONIC, 0, GTC_CACHE);
		else
			vis->colormap = R_GetTranslationColormap(TC_BOSS, 0, GTC_CACHE);
	}
	else if (thing->color)
	{
		// New colormap stuff for skins Tails 06-07-2002
		if (thing->skin && thing->sprite == SPR_PLAY) // This thing is a player!
		{
			size_t skinnum = (skin_t*)thing->skin-skins;
			vis->colormap = R_GetTranslationColormap((INT32)skinnum, thing->color, GTC_CACHE);
		}
		else
			vis->colormap = R_GetTranslationColormap(TC_DEFAULT, vis->mobj->color ? vis->mobj->color : SKINCOLOR_CYAN, GTC_CACHE);
	}
	else
		vis->colormap = colormaps;

	// set top/bottom coords
	vis->ty = gzt;

	//CONS_Debug(DBG_RENDER, "------------------\nH: sprite  : %d\nH: frame   : %x\nH: type    : %d\nH: sname   : %s\n\n",
	//            thing->sprite, thing->frame, thing->type, sprnames[thing->sprite]);

	if (thing->eflags & MFE_VERTICALFLIP)
		vis->vflip = true;
	else
		vis->vflip = false;

	vis->precip = false;
}

#ifdef HWPRECIP
// Precipitation projector for hardware mode
static void HWR_ProjectPrecipitationSprite(precipmobj_t *thing)
{
	gr_vissprite_t *vis;
	float tr_x, tr_y;
	float tz;
	float x1, x2;
	float z1, z2;
	float rightsin, rightcos;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lumpoff;
	unsigned rot = 0;
	UINT8 flip;

	// transform the origin point
	tr_x = FIXED_TO_FLOAT(thing->x) - gr_viewx;
	tr_y = FIXED_TO_FLOAT(thing->y) - gr_viewy;

	// rotation around vertical axis
	tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	// thing is behind view plane?
	if (tz < ZCLIP_PLANE)
		return;

	tr_x = FIXED_TO_FLOAT(thing->x);
	tr_y = FIXED_TO_FLOAT(thing->y);

	// decide which patch to use for sprite relative to player
	if ((unsigned)thing->sprite >= numsprites)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite number %i ",
		        thing->sprite);
#else
		return;
#endif

	sprdef = &sprites[thing->sprite];

	if ((size_t)(thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
#ifdef RANGECHECK
		I_Error("HWR_ProjectPrecipitationSprite: invalid sprite frame %i : %i for %s",
		        thing->sprite, thing->frame, sprnames[thing->sprite]);
#else
		return;
#endif

	sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

	// use single rotation for all views
	lumpoff = sprframe->lumpid[0];
	flip = sprframe->flip; // Will only be 0x00 or 0xFF

	rightsin = FIXED_TO_FLOAT(FINESINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	rightcos = FIXED_TO_FLOAT(FINECOSINE((viewangle + ANGLE_90)>>ANGLETOFINESHIFT));
	if (flip)
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
	}
	else
	{
		x1 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].offset);
		x2 = FIXED_TO_FLOAT(spritecachedinfo[lumpoff].width - spritecachedinfo[lumpoff].offset);
	}

	z1 = tr_y + x1 * rightsin;
	z2 = tr_y - x2 * rightsin;
	x1 = tr_x + x1 * rightcos;
	x2 = tr_x - x2 * rightcos;

	//
	// store information in a vissprite
	//
	vis = HWR_NewVisSprite();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->z1 = z1;
	vis->z2 = z2;
	vis->tz = tz;
	vis->dispoffset = 0; // Monster Iestyn: 23/11/15: HARDWARE SUPPORT AT LAST
	vis->patchlumpnum = sprframe->lumppat[rot];
	vis->flip = flip;
	vis->mobj = (mobj_t *)thing;

	vis->colormap = colormaps;

	// set top/bottom coords
	vis->ty = FIXED_TO_FLOAT(thing->z + spritecachedinfo[lumpoff].topoffset);

	vis->precip = true;
}
#endif

// ==========================================================================
//
// ==========================================================================
static void HWR_DrawSkyBackground(player_t *player)
{
	FOutVector v[4];
	angle_t angle;
	float dimensionmultiply;
	float aspectratio;
	float angleturn;

//  3--2
//  | /|
//  |/ |
//  0--1

	(void)player;
	HWR_GetTexture(skytexture);

	//Hurdler: the sky is the only texture who need 4.0f instead of 1.0
	//         because it's called just after clearing the screen
	//         and thus, the near clipping plane is set to 3.99
	// Sryder: Just use the near clipping plane value then
	v[0].x = v[3].x = -ZCLIP_PLANE-1;
	v[1].x = v[2].x =  ZCLIP_PLANE+1;
	v[0].y = v[1].y = -ZCLIP_PLANE-1;
	v[2].y = v[3].y =  ZCLIP_PLANE+1;

	v[0].z = v[1].z = v[2].z = v[3].z = ZCLIP_PLANE+1;

	// X

	// NOTE: This doesn't work right with texture widths greater than 1024
	// software doesn't draw any further than 1024 for skies anyway, but this doesn't overlap properly
	// The only time this will probably be an issue is when a sky wider than 1024 is used as a sky AND a regular wall texture

	angle = (dup_viewangle + gr_xtoviewangle[0]);

	dimensionmultiply = ((float)textures[skytexture]->width/256.0f);

	v[0].sow = v[3].sow = ((float) angle / ((ANGLE_90-1)*dimensionmultiply));
	v[2].sow = v[1].sow = (-1.0f/dimensionmultiply)+((float) angle / ((ANGLE_90-1)*dimensionmultiply));

	// Y
	angle = aimingangle;

	aspectratio = (float)vid.width/(float)vid.height;
	dimensionmultiply = ((float)textures[skytexture]->height/(128.0f*aspectratio));
	angleturn = (((float)ANGLE_45-1.0f)*aspectratio)*dimensionmultiply;

	// Middle of the sky should always be at angle 0
	// need to keep correct aspect ratio with X
	if (atransform.flip)
	{
		// During vertical flip the sky should be flipped and it's y movement should also be flipped obviously
		v[3].tow = v[2].tow = -(0.5f-(0.5f/dimensionmultiply));
		v[0].tow = v[1].tow = (-1.0f/dimensionmultiply)-(0.5f-(0.5f/dimensionmultiply));
	}
	else
	{
		v[3].tow = v[2].tow = (-1.0f/dimensionmultiply)-(0.5f-(0.5f/dimensionmultiply));
		v[0].tow = v[1].tow = -(0.5f-(0.5f/dimensionmultiply));
	}

	if (angle > ANGLE_180) // Do this because we don't want the sky to suddenly teleport when crossing over 0 to 360 and vice versa
	{
		angle = InvAngle(angle);
		v[3].tow = v[2].tow += ((float) angle / angleturn);
		v[0].tow = v[1].tow += ((float) angle / angleturn);
	}
	else
	{
		v[3].tow = v[2].tow -= ((float) angle / angleturn);
		v[0].tow = v[1].tow -= ((float) angle / angleturn);
	}

	HWD.pfnDrawPolygon(NULL, v, 4, 0);
}


// -----------------+
// HWR_ClearView : clear the viewwindow, with maximum z value
// -----------------+
static inline void HWR_ClearView(void)
{
	//  3--2
	//  | /|
	//  |/ |
	//  0--1

	/// \bug faB - enable depth mask, disable color mask

	HWD.pfnGClipRect((INT32)gr_viewwindowx,
	                 (INT32)gr_viewwindowy,
	                 (INT32)(gr_viewwindowx + gr_viewwidth),
	                 (INT32)(gr_viewwindowy + gr_viewheight),
	                 ZCLIP_PLANE);
	HWD.pfnClearBuffer(false, true, 0);

	//disable clip window - set to full size
	// rem by Hurdler
	// HWD.pfnGClipRect(0, 0, vid.width, vid.height);
}


// -----------------+
// HWR_SetViewSize  : set projection and scaling values
// -----------------+
void HWR_SetViewSize(void)
{
	// setup view size
	gr_viewwidth = (float)vid.width;
	gr_viewheight = (float)vid.height;

	if (splitscreen)
		gr_viewheight /= 2;

	gr_centerx = gr_viewwidth / 2;
	gr_basecentery = gr_viewheight / 2; //note: this is (gr_centerx * gr_viewheight / gr_viewwidth)

	gr_viewwindowx = (vid.width - gr_viewwidth) / 2;
	gr_windowcenterx = (float)(vid.width / 2);
	if (gr_viewwidth == vid.width)
	{
		gr_baseviewwindowy = 0;
		gr_basewindowcentery = gr_viewheight / 2;               // window top left corner at 0,0
	}
	else
	{
		gr_baseviewwindowy = (vid.height-gr_viewheight) / 2;
		gr_basewindowcentery = (float)(vid.height / 2);
	}

	gr_pspritexscale = gr_viewwidth / BASEVIDWIDTH;
	gr_pspriteyscale = ((vid.height*gr_pspritexscale*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width;

	HWD.pfnFlushScreenTextures();
}

// ==========================================================================
// Same as rendering the player view, but from the skybox object
// ==========================================================================
void HWR_RenderSkyboxView(INT32 viewnumber, player_t *player)
{
	const float fpov = FIXED_TO_FLOAT(cv_grfov.value+player->fovadd);
	postimg_t *type;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	{
		// do we really need to save player (is it not the same)?
		player_t *saved_player = stplyr;
		stplyr = player;
		ST_doPaletteStuff();
		stplyr = saved_player;
#ifdef ALAM_LIGHTING
		HWR_SetLights(viewnumber);
#endif
	}

	// note: sets viewangle, viewx, viewy, viewz
	R_SkyboxFrame(player);

	// copy view cam position for local use
	dup_viewx = viewx;
	dup_viewy = viewy;
	dup_viewz = viewz;
	dup_viewangle = viewangle;

	// set window position
	gr_centery = gr_basecentery;
	gr_viewwindowy = gr_baseviewwindowy;
	gr_windowcentery = gr_basewindowcentery;
	if (splitscreen && viewnumber == 1)
	{
		gr_viewwindowy += (vid.height/2);
		gr_windowcentery += (vid.height/2);
	}

	// check for new console commands.
	NetUpdate();

	gr_viewx = FIXED_TO_FLOAT(dup_viewx);
	gr_viewy = FIXED_TO_FLOAT(dup_viewy);
	gr_viewz = FIXED_TO_FLOAT(dup_viewz);
	gr_viewsin = FIXED_TO_FLOAT(viewsin);
	gr_viewcos = FIXED_TO_FLOAT(viewcos);

	gr_viewludsin = FIXED_TO_FLOAT(FINECOSINE(aimingangle>>ANGLETOFINESHIFT));
	gr_viewludcos = FIXED_TO_FLOAT(-FINESINE(aimingangle>>ANGLETOFINESHIFT));

	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	atransform.anglex = (float)(aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;

	atransform.x      = gr_viewx;  // FIXED_TO_FLOAT(viewx)
	atransform.y      = gr_viewy;  // FIXED_TO_FLOAT(viewy)
	atransform.z      = gr_viewz;  // FIXED_TO_FLOAT(viewz)
	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;
	atransform.fovxangle = fpov; // Tails
	atransform.fovyangle = fpov; // Tails
	atransform.splitscreen = splitscreen;

	gr_fovlud = (float)(1.0l/tan((double)(fpov*M_PIl/360l)));

	//------------------------------------------------------------------------
	HWR_ClearView();

if (0)
{ // I don't think this is ever used.
	if (cv_grfog.value)
		HWR_FoggingOn(); // First of all, turn it on, set the default user settings too
	else
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0); // Turn it off
}

#ifndef _NDS
	if (drawsky)
		HWR_DrawSkyBackground(player);
#else
	(void)HWR_DrawSkyBackground;
#endif

	//Hurdler: it doesn't work in splitscreen mode
	drawsky = splitscreen;

	HWR_ClearSprites();

#ifdef SORTING
	drawcount = 0;
#endif
	HWR_ClearClipSegs();

	//04/01/2000: Hurdler: added for T&L
	//                     Actually it only works on Walls and Planes
	HWD.pfnSetTransform(&atransform);

	validcount++;

	HWR_RenderBSPNode((INT32)numnodes-1);

	// Make a viewangle int so we can render things based on mouselook
	if (player == &players[consoleplayer])
		viewangle = localaiming;
	else if (splitscreen && player == &players[secondarydisplayplayer])
		viewangle = localaiming2;

	// Handle stuff when you are looking farther up or down.
	if ((aimingangle || cv_grfov.value+player->fovadd > 90*FRACUNIT))
	{
		dup_viewangle += ANGLE_90;
		HWR_ClearClipSegs();
		HWR_RenderBSPNode((INT32)numnodes-1); //left

		dup_viewangle += ANGLE_90;
		if (((INT32)aimingangle > ANGLE_45 || (INT32)aimingangle<-ANGLE_45))
		{
			HWR_ClearClipSegs();
			HWR_RenderBSPNode((INT32)numnodes-1); //back
		}

		dup_viewangle += ANGLE_90;
		HWR_ClearClipSegs();
		HWR_RenderBSPNode((INT32)numnodes-1); //right

		dup_viewangle += ANGLE_90;
	}

	// Check for new console commands.
	NetUpdate();

#ifdef ALAM_LIGHTING
	//14/11/99: Hurdler: moved here because it doesn't work with
	// subsector, see other comments;
	HWR_ResetLights();
#endif

	// Draw MD2 and sprites
#ifdef SORTING
	HWR_SortVisSprites();
#endif

#ifdef SORTING
	HWR_DrawSprites();
#endif
#ifdef NEWCORONAS
	//Hurdler: they must be drawn before translucent planes, what about gl fog?
	HWR_DrawCoronas();
#endif

#ifdef SORTING
	if (numplanes || numpolyplanes || numwalls) //Hurdler: render 3D water and transparent walls after everything
	{
		HWR_CreateDrawNodes();
	}
#else
	if (numfloors || numwalls)
	{
		HWD.pfnSetTransform(&atransform);
		if (numfloors)
			HWR_Render3DWater();
		if (numwalls)
			HWR_RenderTransparentWalls();
	}
#endif

	HWD.pfnSetTransform(NULL);

	// put it off for menus etc
	if (cv_grfog.value)
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0);

	// Check for new console commands.
	NetUpdate();

	// added by Hurdler for correct splitscreen
	// moved here by hurdler so it works with the new near clipping plane
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}

// ==========================================================================
//
// ==========================================================================
void HWR_RenderPlayerView(INT32 viewnumber, player_t *player)
{
	const float fpov = FIXED_TO_FLOAT(cv_grfov.value+player->fovadd);
	postimg_t *type;

	const boolean skybox = (skyboxmo[0] && cv_skybox.value); // True if there's a skybox object and skyboxes are on

	FRGBAFloat ClearColor;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	ClearColor.red = 0.0f;
	ClearColor.green = 0.0f;
	ClearColor.blue = 0.0f;
	ClearColor.alpha = 1.0f;

	if (viewnumber == 0) // Only do it if it's the first screen being rendered
		HWD.pfnClearBuffer(true, false, &ClearColor); // Clear the Color Buffer, stops HOMs. Also seems to fix the skybox issue on Intel GPUs.

	if (skybox && drawsky) // If there's a skybox and we should be drawing the sky, draw the skybox
		HWR_RenderSkyboxView(viewnumber, player); // This is drawn before everything else so it is placed behind

	{
		// do we really need to save player (is it not the same)?
		player_t *saved_player = stplyr;
		stplyr = player;
		ST_doPaletteStuff();
		stplyr = saved_player;
#ifdef ALAM_LIGHTING
		HWR_SetLights(viewnumber);
#endif
	}

	// note: sets viewangle, viewx, viewy, viewz
	R_SetupFrame(player, false); // This can stay false because it is only used to set viewsky in r_main.c, which isn't used here

	// copy view cam position for local use
	dup_viewx = viewx;
	dup_viewy = viewy;
	dup_viewz = viewz;
	dup_viewangle = viewangle;

	// set window position
	gr_centery = gr_basecentery;
	gr_viewwindowy = gr_baseviewwindowy;
	gr_windowcentery = gr_basewindowcentery;
	if (splitscreen && viewnumber == 1)
	{
		gr_viewwindowy += (vid.height/2);
		gr_windowcentery += (vid.height/2);
	}

	// check for new console commands.
	NetUpdate();

	gr_viewx = FIXED_TO_FLOAT(dup_viewx);
	gr_viewy = FIXED_TO_FLOAT(dup_viewy);
	gr_viewz = FIXED_TO_FLOAT(dup_viewz);
	gr_viewsin = FIXED_TO_FLOAT(viewsin);
	gr_viewcos = FIXED_TO_FLOAT(viewcos);

	gr_viewludsin = FIXED_TO_FLOAT(FINECOSINE(aimingangle>>ANGLETOFINESHIFT));
	gr_viewludcos = FIXED_TO_FLOAT(-FINESINE(aimingangle>>ANGLETOFINESHIFT));

	//04/01/2000: Hurdler: added for T&L
	//                     It should replace all other gr_viewxxx when finished
	atransform.anglex = (float)(aimingangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);
	atransform.angley = (float)(viewangle>>ANGLETOFINESHIFT)*(360.0f/(float)FINEANGLES);

	if (*type == postimg_flip)
		atransform.flip = true;
	else
		atransform.flip = false;

	atransform.x      = gr_viewx;  // FIXED_TO_FLOAT(viewx)
	atransform.y      = gr_viewy;  // FIXED_TO_FLOAT(viewy)
	atransform.z      = gr_viewz;  // FIXED_TO_FLOAT(viewz)
	atransform.scalex = 1;
	atransform.scaley = (float)vid.width/vid.height;
	atransform.scalez = 1;
	atransform.fovxangle = fpov; // Tails
	atransform.fovyangle = fpov; // Tails
	atransform.splitscreen = splitscreen;

	gr_fovlud = (float)(1.0l/tan((double)(fpov*M_PIl/360l)));

	//------------------------------------------------------------------------
	HWR_ClearView(); // Clears the depth buffer and resets the view I believe

if (0)
{ // I don't think this is ever used.
	if (cv_grfog.value)
		HWR_FoggingOn(); // First of all, turn it on, set the default user settings too
	else
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0); // Turn it off
}

#ifndef _NDS
	if (!skybox && drawsky) // Don't draw the regular sky if there's a skybox
		HWR_DrawSkyBackground(player);
#else
	(void)HWR_DrawSkyBackground;
#endif

	//Hurdler: it doesn't work in splitscreen mode
	drawsky = splitscreen;

	HWR_ClearSprites();

#ifdef SORTING
	drawcount = 0;
#endif
	HWR_ClearClipSegs();

	//04/01/2000: Hurdler: added for T&L
	//                     Actually it only works on Walls and Planes
	HWD.pfnSetTransform(&atransform);

	validcount++;

	HWR_RenderBSPNode((INT32)numnodes-1);

	// Make a viewangle int so we can render things based on mouselook
	if (player == &players[consoleplayer])
		viewangle = localaiming;
	else if (splitscreen && player == &players[secondarydisplayplayer])
		viewangle = localaiming2;

	// Handle stuff when you are looking farther up or down.
	if ((aimingangle || cv_grfov.value+player->fovadd > 90*FRACUNIT))
	{
		dup_viewangle += ANGLE_90;
		HWR_ClearClipSegs();
		HWR_RenderBSPNode((INT32)numnodes-1); //left

		dup_viewangle += ANGLE_90;
		if (((INT32)aimingangle > ANGLE_45 || (INT32)aimingangle<-ANGLE_45))
		{
			HWR_ClearClipSegs();
			HWR_RenderBSPNode((INT32)numnodes-1); //back
		}

		dup_viewangle += ANGLE_90;
		HWR_ClearClipSegs();
		HWR_RenderBSPNode((INT32)numnodes-1); //right

		dup_viewangle += ANGLE_90;
	}

	// Check for new console commands.
	NetUpdate();

#ifdef ALAM_LIGHTING
	//14/11/99: Hurdler: moved here because it doesn't work with
	// subsector, see other comments;
	HWR_ResetLights();
#endif

	// Draw MD2 and sprites
#ifdef SORTING
	HWR_SortVisSprites();
#endif

#ifdef SORTING
	HWR_DrawSprites();
#endif
#ifdef NEWCORONAS
	//Hurdler: they must be drawn before translucent planes, what about gl fog?
	HWR_DrawCoronas();
#endif

#ifdef SORTING
	if (numplanes || numpolyplanes || numwalls) //Hurdler: render 3D water and transparent walls after everything
	{
		HWR_CreateDrawNodes();
	}
#else
	if (numfloors || numpolyplanes || numwalls)
	{
		HWD.pfnSetTransform(&atransform);
		if (numfloors)
			HWR_Render3DWater();
		if (numwalls)
			HWR_RenderTransparentWalls();
	}
#endif

	HWD.pfnSetTransform(NULL);

	// put it off for menus etc
	if (cv_grfog.value)
		HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0);

	HWR_DoPostProcessor(player);

	// Check for new console commands.
	NetUpdate();

	// added by Hurdler for correct splitscreen
	// moved here by hurdler so it works with the new near clipping plane
	HWD.pfnGClipRect(0, 0, vid.width, vid.height, NZCLIP_PLANE);
}

// ==========================================================================
//                                                                        FOG
// ==========================================================================

/// \author faB

static UINT32 atohex(const char *s)
{
	INT32 iCol;
	const char *sCol;
	char cCol;
	INT32 i;

	if (strlen(s)<6)
		return 0;

	iCol = 0;
	sCol = s;
	for (i = 0; i < 6; i++, sCol++)
	{
		iCol <<= 4;
		cCol = *sCol;
		if (cCol >= '0' && cCol <= '9')
			iCol |= cCol - '0';
		else
		{
			if (cCol >= 'F')
				cCol -= 'a' - 'A';
			if (cCol >= 'A' && cCol <= 'F')
				iCol = iCol | (cCol - 'A' + 10);
		}
	}
	//CONS_Debug(DBG_RENDER, "col %x\n", iCol);
	return iCol;
}

static void HWR_FoggingOn(void)
{
	HWD.pfnSetSpecialState(HWD_SET_FOG_COLOR, atohex(cv_grfogcolor.string));
	HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
	HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 1);
}

// ==========================================================================
//                                                         3D ENGINE COMMANDS
// ==========================================================================


static void CV_grFov_OnChange(void)
{
	if ((netgame || multiplayer) && !cv_debug && cv_grfov.value != 90*FRACUNIT)
		CV_Set(&cv_grfov, cv_grfov.defaultvalue);
}

static void Command_GrStats_f(void)
{
	Z_CheckHeap(9875); // debug

	CONS_Printf(M_GetText("Patch info headers: %7s kb\n"), sizeu1(Z_TagUsage(PU_HWRPATCHINFO)>>10));
	CONS_Printf(M_GetText("3D Texture cache  : %7s kb\n"), sizeu1(Z_TagUsage(PU_HWRCACHE)>>10));
	CONS_Printf(M_GetText("Plane polygon     : %7s kb\n"), sizeu1(Z_TagUsage(PU_HWRPLANE)>>10));
}


// **************************************************************************
//                                                            3D ENGINE SETUP
// **************************************************************************

// --------------------------------------------------------------------------
// Add hardware engine commands & consvars
// --------------------------------------------------------------------------
//added by Hurdler: console varibale that are saved
void HWR_AddCommands(void)
{
	CV_RegisterVar(&cv_grrounddown);
	CV_RegisterVar(&cv_grfov);
	CV_RegisterVar(&cv_grfogdensity);
	CV_RegisterVar(&cv_grfiltermode);
	CV_RegisterVar(&cv_granisotropicmode);
	CV_RegisterVar(&cv_grcorrecttricks);
	CV_RegisterVar(&cv_grsolvetjoin);
}

static inline void HWR_AddEngineCommands(void)
{
	// engine state variables
	//CV_RegisterVar(&cv_grzbuffer);
	CV_RegisterVar(&cv_grclipwalls);

	// engine development mode variables
	// - usage may vary from version to version..
	CV_RegisterVar(&cv_gralpha);
	CV_RegisterVar(&cv_grbeta);

	// engine commands
	COM_AddCommand("gr_stats", Command_GrStats_f);
}


// --------------------------------------------------------------------------
// Setup the hardware renderer
// --------------------------------------------------------------------------
void HWR_Startup(void)
{
	static boolean startupdone = false;

	// setup GLPatch_t scaling
	gr_patch_scalex = (float)(1.0f / vid.width);
	gr_patch_scaley = (float)(1.0f / vid.height);

	// initalze light lut translation
	InitLumLut();

	// do this once
	if (!startupdone)
	{
		CONS_Printf("HWR_Startup()\n");
		HWR_InitPolyPool();
		// add console cmds & vars
		HWR_AddEngineCommands();
		HWR_InitTextureCache();

		HWR_InitMD2();

#ifdef ALAM_LIGHTING
		HWR_InitLight();
#endif
	}

	if (rendermode == render_opengl)
		textureformat = patchformat =
#ifdef _NDS
			GR_TEXFMT_P_8;
#else
			GR_RGBA;
#endif

	startupdone = true;
}


// --------------------------------------------------------------------------
// Free resources allocated by the hardware renderer
// --------------------------------------------------------------------------
void HWR_Shutdown(void)
{
	CONS_Printf("HWR_Shutdown()\n");
	HWR_FreeExtraSubsectors();
	HWR_FreePolyPool();
	HWR_FreeTextureCache();
	HWD.pfnFlushScreenTextures();
}

void transform(float *cx, float *cy, float *cz)
{
	float tr_x,tr_y;
	// translation
	tr_x = *cx - gr_viewx;
	tr_y = *cz - gr_viewy;
//	*cy = *cy;

	// rotation around vertical y axis
	*cx = (tr_x * gr_viewsin) - (tr_y * gr_viewcos);
	tr_x = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

	//look up/down ----TOTAL SUCKS!!!--- do the 2 in one!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	tr_y = *cy - gr_viewz;

	*cy = (tr_x * gr_viewludcos) + (tr_y * gr_viewludsin);
	*cz = (tr_x * gr_viewludsin) - (tr_y * gr_viewludcos);

	//scale y before frustum so that frustum can be scaled to screen height
	*cy *= ORIGINAL_ASPECT * gr_fovlud;
	*cx *= gr_fovlud;
}


//Hurdler: 3D Water stuff
#define MAX_3DWATER 512

#ifndef SORTING
static void HWR_Add3DWater(lumpnum_t lumpnum, extrasubsector_t *xsub,
	fixed_t fixedheight, INT32 lightlevel, INT32 alpha, sector_t *FOFSector)
{
	static size_t allocedplanes = 0;

	// Force realloc if buffer has been freed
	if (!planeinfo)
		allocedplanes = 0;

	if (allocedplanes < numfloors + 1)
	{
		allocedplanes += MAX_3DWATER;
		Z_Realloc(planeinfo, allocedplanes * sizeof (*planeinfo), PU_LEVEL, &planeinfo);
	}
	planeinfo[numfloors].fixedheight = fixedheight;
	planeinfo[numfloors].lightlevel = lightlevel;
	planeinfo[numfloors].lumpnum = lumpnum;
	planeinfo[numfloors].xsub = xsub;
	planeinfo[numfloors].alpha = alpha;
	planeinfo[numfloors].FOFSector = FOFSector;
	numfloors++;
}
#endif

#define DIST_PLANE(i) ABS(planeinfo[(i)].fixedheight-dup_viewz)

#if 0
static void HWR_QuickSortPlane(INT32 start, INT32 finish)
{
	INT32 left = start;
	INT32 right = finish;
	INT32 starterval = (INT32)((right+left)/2); //pick a starter

	planeinfo_t temp;

	//'sort of sort' the two halves of the data list about starterval
	while (right > left);
	{
		while (DIST_PLANE(left) < DIST_PLANE(starterval)) left++; //attempt to find a bigger value on the left
		while (DIST_PLANE(right) > DIST_PLANE(starterval)) right--; //attempt to find a smaller value on the right

		if (left < right) //if we haven't gone too far
		{
			//switch them
			M_Memcpy(&temp, &planeinfo[left], sizeof (planeinfo_t));
			M_Memcpy(&planeinfo[left], &planeinfo[right], sizeof (planeinfo_t));
			M_Memcpy(&planeinfo[right], &temp, sizeof (planeinfo_t));
			//move the bounds
			left++;
			right--;
		}
	}

	if (start < right) HWR_QuickSortPlane(start, right);
	if (left < finish) HWR_QuickSortPlane(left, finish);
}
#endif

#ifndef SORTING
static void HWR_Render3DWater(void)
{
	size_t i;

	//bubble sort 3D Water for correct alpha blending
	{
		boolean permut = true;
		while (permut)
		{
			size_t j;
			for (j = 0, permut= false; j < numfloors-1; j++)
			{
				if (ABS(planeinfo[j].fixedheight-dup_viewz) < ABS(planeinfo[j+1].fixedheight-dup_viewz))
				{
					planeinfo_t temp;
					M_Memcpy(&temp, &planeinfo[j+1], sizeof (planeinfo_t));
					M_Memcpy(&planeinfo[j+1], &planeinfo[j], sizeof (planeinfo_t));
					M_Memcpy(&planeinfo[j], &temp, sizeof (planeinfo_t));
					permut = true;
				}
			}
		}
	}
#if 0 //thanks epat, but it's goes looping forever on CTF map Silver Cascade Zone
	HWR_QuickSortPlane(0, numplanes-1);
#endif

	gr_frontsector = NULL; //Hurdler: gr_fronsector is no longer valid
	for (i = 0; i < numfloors; i++)
	{
		HWR_GetFlat(planeinfo[i].lumpnum);
		HWR_RenderPlane(NULL, planeinfo[i].xsub, planeinfo[i].isceiling, planeinfo[i].fixedheight, PF_Translucent, planeinfo[i].lightlevel, planeinfo[i].lumpnum,
			planeinfo[i].FOFSector, planeinfo[i].alpha, planeinfo[i].fogplane, planeinfo[i].planecolormap);
	}
	numfloors = 0;
}
#endif

static void HWR_AddTransparentWall(wallVert3D *wallVerts, FSurfaceInfo *pSurf, INT32 texnum, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	static size_t allocedwalls = 0;

	// Force realloc if buffer has been freed
	if (!wallinfo)
		allocedwalls = 0;

	if (allocedwalls < numwalls + 1)
	{
		allocedwalls += MAX_TRANSPARENTWALL;
		Z_Realloc(wallinfo, allocedwalls * sizeof (*wallinfo), PU_LEVEL, &wallinfo);
	}

	M_Memcpy(wallinfo[numwalls].wallVerts, wallVerts, sizeof (wallinfo[numwalls].wallVerts));
	M_Memcpy(&wallinfo[numwalls].Surf, pSurf, sizeof (FSurfaceInfo));
	wallinfo[numwalls].texnum = texnum;
	wallinfo[numwalls].blend = blend;
#ifdef SORTING
	wallinfo[numwalls].drawcount = drawcount++;
#endif
	wallinfo[numwalls].fogwall = fogwall;
	wallinfo[numwalls].lightlevel = lightlevel;
	wallinfo[numwalls].wallcolormap = wallcolormap;
	numwalls++;
}
#ifndef SORTING
static void HWR_RenderTransparentWalls(void)
{
	size_t i;

	/*{ // sorting is disbale for now, do it!
		INT32 permut = 1;
		while (permut)
		{
			INT32 j;
			for (j = 0, permut = 0; j < numwalls-1; j++)
			{
				if (ABS(wallinfo[j].fixedheight-dup_viewz) < ABS(wallinfo[j+1].fixedheight-dup_viewz))
				{
					wallinfo_t temp;
					M_Memcpy(&temp, &wallinfo[j+1], sizeof (wallinfo_t));
					M_Memcpy(&wallinfo[j+1], &wallinfo[j], sizeof (wallinfo_t));
					M_Memcpy(&wallinfo[j], &temp, sizeof (wallinfo_t));
					permut = 1;
				}
			}
		}
	}*/

	for (i = 0; i < numwalls; i++)
	{
		HWR_GetTexture(wallinfo[i].texnum);
		HWR_RenderWall(wallinfo[i].wallVerts, &wallinfo[i].Surf, wallinfo[i].blend, wallinfo[i].wall->fogwall, wallinfo[i].wall->lightlevel, wallinfo[i].wall->wallcolormap);
	}
	numwalls = 0;
}
#endif
static void HWR_RenderWall(wallVert3D   *wallVerts, FSurfaceInfo *pSurf, FBITFIELD blend, boolean fogwall, INT32 lightlevel, extracolormap_t *wallcolormap)
{
	FOutVector  trVerts[4];
	UINT8       i;
	FOutVector  *wv;
	UINT8 alpha;

	// transform
	wv = trVerts;
	// it sounds really stupid to do this conversion with the new T&L code
	// we should directly put the right information in the right structure
	// wallVerts3D seems ok, doesn't need FOutVector
	// also remove the light copy
	for (i = 0; i < 4; i++, wv++, wallVerts++)
	{
		wv->sow = wallVerts->s;
		wv->tow = wallVerts->t;
		wv->x   = wallVerts->x;
		wv->y   = wallVerts->y;
		wv->z = wallVerts->z;
	}

	alpha = pSurf->FlatColor.s.alpha; // retain the alpha

	// Lighting is done here instead so that fog isn't drawn incorrectly on transparent walls after sorting
	if (wallcolormap)
	{
		if (fogwall)
			pSurf->FlatColor.rgba = HWR_Lighting(lightlevel, wallcolormap->rgba, wallcolormap->fadergba, true, false);
		else
			pSurf->FlatColor.rgba = HWR_Lighting(lightlevel, wallcolormap->rgba, wallcolormap->fadergba, false, false);
	}
	else
	{
		if (fogwall)
			pSurf->FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, true, false);
		else
			pSurf->FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);
	}

	pSurf->FlatColor.s.alpha = alpha; // put the alpha back after lighting

	if (blend & PF_Environment)
		HWD.pfnDrawPolygon(pSurf, trVerts, 4, blend|PF_Modulated|PF_Clip|PF_Occlude); // PF_Occlude must be used for solid objects
	else
		HWD.pfnDrawPolygon(pSurf, trVerts, 4, blend|PF_Modulated|PF_Clip); // No PF_Occlude means overlapping (incorrect) transparency

#ifdef WALLSPLATS
	if (gr_curline->linedef->splats && cv_splats.value)
		HWR_DrawSegsSplats(pSurf);

#ifdef ALAM_LIGHTING
	//Hurdler: TODO: do static lighting using gr_curline->lm
	HWR_WallLighting(trVerts);
#endif
#endif
}

void HWR_SetPaletteColor(INT32 palcolor)
{
	HWD.pfnSetSpecialState(HWD_SET_PALETTECOLOR, palcolor);
}

INT32 HWR_GetTextureUsed(void)
{
	return HWD.pfnGetTextureUsed();
}

void HWR_DoPostProcessor(player_t *player)
{
	postimg_t *type;

	if (splitscreen && player == &players[secondarydisplayplayer])
		type = &postimgtype2;
	else
		type = &postimgtype;

	// Armageddon Blast Flash!
	// Could this even be considered postprocessor?
	if (player->flashcount)
	{
		FOutVector      v[4];
		FSurfaceInfo Surf;

		v[0].x = v[2].y = v[3].x = v[3].y = -4.0f;
		v[0].y = v[1].x = v[1].y = v[2].x = 4.0f;
		v[0].z = v[1].z = v[2].z = v[3].z = 4.0f; // 4.0 because of the same reason as with the sky, just after the screen is cleared so near clipping plane is 3.99

		// This won't change if the flash palettes are changed unfortunately, but it works for its purpose
		if (player->flashpal == PAL_NUKE)
		{
			Surf.FlatColor.s.red = 0xff;
			Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0x7F; // The nuke palette is kind of pink-ish
		}
		else
			Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;

		Surf.FlatColor.s.alpha = 0xc0; // match software mode

		HWD.pfnDrawPolygon(&Surf, v, 4, PF_Modulated|PF_Additive|PF_NoTexture|PF_NoDepthTest|PF_Clip|PF_NoZClip);
	}

	// Capture the screen for intermission and screen waving
	if(gamestate != GS_INTERMISSION)
		HWD.pfnMakeScreenTexture();

	if (splitscreen) // Not supported in splitscreen - someone want to add support?
		return;

#ifdef SHUFFLE
	// Drunken vision! WooOOooo~
	if (*type == postimg_water || *type == postimg_heat)
	{
		// 10 by 10 grid. 2 coordinates (xy)
		float v[SCREENVERTS][SCREENVERTS][2];
		static double disStart = 0;
		UINT8 x, y;
		INT32 WAVELENGTH;
		INT32 AMPLITUDE;
		INT32 FREQUENCY;

		// Modifies the wave.
		if (*type == postimg_water)
		{
			WAVELENGTH = 20; // Lower is longer
			AMPLITUDE = 20; // Lower is bigger
			FREQUENCY = 16; // Lower is faster
		}
		else
		{
			WAVELENGTH = 10; // Lower is longer
			AMPLITUDE = 30; // Lower is bigger
			FREQUENCY = 4; // Lower is faster
		}

		for (x = 0; x < SCREENVERTS; x++)
		{
			for (y = 0; y < SCREENVERTS; y++)
			{
				// Change X position based on its Y position.
				v[x][y][0] = (x/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f + (float)sin((disStart+(y*WAVELENGTH))/FREQUENCY)/AMPLITUDE;
				v[x][y][1] = (y/((float)(SCREENVERTS-1.0f)/9.0f))-4.5f;
			}
		}
		HWD.pfnPostImgRedraw(v);
		disStart += 1;

		// Capture the screen again for screen waving on the intermission
		if(gamestate != GS_INTERMISSION)
			HWD.pfnMakeScreenTexture();
	}
	// Flipping of the screen isn't done here anymore
#endif // SHUFFLE
}

void HWR_StartScreenWipe(void)
{
	//CONS_Debug(DBG_RENDER, "In HWR_StartScreenWipe()\n");
	HWD.pfnStartScreenWipe();
}

void HWR_EndScreenWipe(void)
{
	HWRWipeCounter = 0.0f;
	//CONS_Debug(DBG_RENDER, "In HWR_EndScreenWipe()\n");
	HWD.pfnEndScreenWipe();
}

void HWR_DrawIntermissionBG(void)
{
	HWD.pfnDrawIntermissionBG();
}

void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum)
{
	static char lumpname[9] = "FADEmmss";
	lumpnum_t lumpnum;
	size_t lsize;

	if (wipenum > 99 || scrnnum > 99) // not a valid wipe number
		return; // shouldn't end up here really, the loop should've stopped running beforehand

	// puts the numbers into the lumpname
	sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)wipenum, (UINT16)scrnnum);
	lumpnum = W_CheckNumForName(lumpname);

	if (lumpnum == LUMPERROR) // again, shouldn't be here really
		return;

	lsize = W_LumpLength(lumpnum);

	if (!(lsize == 256000 || lsize == 64000 || lsize == 16000 || lsize == 4000))
	{
		CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		return; // again, shouldn't get here if it is a bad size
	}

	HWR_GetFadeMask(lumpnum);

	HWD.pfnDoScreenWipe(HWRWipeCounter); // Still send in wipecounter since old stuff might not support multitexturing

	HWRWipeCounter += 0.05f; // increase opacity of end screen

	if (HWRWipeCounter > 1.0f)
		HWRWipeCounter = 1.0f;
}

void HWR_MakeScreenFinalTexture(void)
{
    HWD.pfnMakeScreenFinalTexture();
}

void HWR_DrawScreenFinalTexture(int width, int height)
{
    HWD.pfnDrawScreenFinalTexture(width, height);
}

#endif // HWRENDER
