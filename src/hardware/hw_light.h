// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_light.h
/// \brief Dynamic lighting & coronas add on by Hurdler

#ifndef _HW_LIGHTS_
#define _HW_LIGHTS_

#include "hw_glob.h"
#include "hw_defs.h"

#define NUMLIGHTFREESLOTS 32 // Free light slots (for SOCs)

#ifdef ALAM_LIGHTING
#define NEWCORONAS

#define DL_MAX_LIGHT 256 // maximum number of lights (extra lights are ignored)

void HWR_InitLight(void);
void HWR_DL_AddLight(gr_vissprite_t *spr, GLPatch_t *patch);
void HWR_PlaneLighting(FOutVector *clVerts, int nrClipVerts);
void HWR_WallLighting(FOutVector *wlVerts);
void HWR_ResetLights(void);
void HWR_SetLights(int viewnumber);

#ifdef NEWCORONAS
void HWR_DrawCoronas(void);
#else
void HWR_DoCoronasLighting(FOutVector *outVerts, gr_vissprite_t *spr);
#endif

typedef struct
{
	int nb;
	light_t *p_lspr[DL_MAX_LIGHT];
	FVector position[DL_MAX_LIGHT]; // actually maximum DL_MAX_LIGHT lights
	mobj_t *mo[DL_MAX_LIGHT];
} dynlights_t;

#endif

typedef enum
{
	NOLIGHT = 0,
	RINGSPARK_L,
	SUPERSONIC_L, // Cool. =)
	SUPERSPARK_L,
	INVINCIBLE_L,
	GREENSHIELD_L,
	BLUESHIELD_L,
	YELLOWSHIELD_L,
	REDSHIELD_L,
	BLACKSHIELD_L,
	WHITESHIELD_L,
	SMALLREDBALL_L,
	RINGLIGHT_L,
	GREENSMALL_L,
	REDSMALL_L,
	GREENSHINE_L,
	ORANGESHINE_L,
	PINKSHINE_L,
	BLUESHINE_L,
	REDSHINE_L,
	LBLUESHINE_L,
	GREYSHINE_L,
	REDBALL_L,
	GREENBALL_L,
	BLUEBALL_L,
	NIGHTSLIGHT_L,
	JETLIGHT_L,
	GOOPLIGHT_L,
	STREETLIGHT_L,

	// free slots for SOCs at run-time --------------------
	FREESLOT0_L,
	//
	// ... 32 free lights here ...
	//
	LASTFREESLOT_L = (FREESLOT0_L+NUMLIGHTFREESLOTS-1),
	// end of freeslots ---------------------------------------------

	NUMLIGHTS
} lightspritenum_t;

extern light_t lspr[NUMLIGHTS];
extern light_t *t_lspr[NUMSPRITES];
#endif
