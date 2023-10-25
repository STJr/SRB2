// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_easing.h
/// \brief Easing functions

#ifndef __M_EASING_H__
#define __M_EASING_H__

#include "doomtype.h"
#include "m_fixed.h"

typedef enum
{
	EASE_LINEAR = 0,

	EASE_INSINE,
	EASE_OUTSINE,
	EASE_INOUTSINE,

	EASE_INQUAD,
	EASE_OUTQUAD,
	EASE_INOUTQUAD,

	EASE_INCUBIC,
	EASE_OUTCUBIC,
	EASE_INOUTCUBIC,

	EASE_INQUART,
	EASE_OUTQUART,
	EASE_INOUTQUART,

	EASE_INQUINT,
	EASE_OUTQUINT,
	EASE_INOUTQUINT,

	EASE_INEXPO,
	EASE_OUTEXPO,
	EASE_INOUTEXPO,

	EASE_INBACK,
	EASE_OUTBACK,
	EASE_INOUTBACK,

	EASE_MAX,
} easing_t;

typedef fixed_t (*easingfunc_t)(fixed_t, fixed_t, fixed_t);

extern easingfunc_t easing_funclist[EASE_MAX];
extern const char *easing_funcnames[EASE_MAX];

#define EASINGFUNCLIST(sep) \
	EASINGFUNC(Linear) sep /* Easing_Linear */ \
 \
	EASINGFUNC(InSine) sep /* Easing_InSine */ \
	EASINGFUNC(OutSine) sep /* Easing_OutSine */ \
	EASINGFUNC(InOutSine) sep /* Easing_InOutSine */ \
 \
	EASINGFUNC(InQuad) sep /* Easing_InQuad */ \
	EASINGFUNC(OutQuad) sep /* Easing_OutQuad */ \
	EASINGFUNC(InOutQuad) sep /* Easing_InOutQuad */ \
 \
	EASINGFUNC(InCubic) sep /* Easing_InCubic */ \
	EASINGFUNC(OutCubic) sep /* Easing_OutCubic */ \
	EASINGFUNC(InOutCubic) sep /* Easing_InOutCubic */ \
 \
	EASINGFUNC(InQuart) sep /* Easing_InQuart */ \
	EASINGFUNC(OutQuart) sep /* Easing_OutQuart */ \
	EASINGFUNC(InOutQuart) sep /* Easing_InOutQuart */ \
 \
	EASINGFUNC(InQuint) sep /* Easing_InQuint */ \
	EASINGFUNC(OutQuint) sep /* Easing_OutQuint */ \
	EASINGFUNC(InOutQuint) sep /* Easing_InOutQuint */ \
 \
	EASINGFUNC(InExpo) sep /* Easing_InExpo */ \
	EASINGFUNC(OutExpo) sep /* Easing_OutExpo */ \
	EASINGFUNC(InOutExpo) sep /* Easing_InOutExpo */ \
 \
	EASINGFUNC(InBack) sep /* Easing_InBack */ \
	EASINGFUNC(OutBack) sep /* Easing_OutBack */ \
	EASINGFUNC(InOutBack) sep /* Easing_InOutBack */

#define EASINGFUNC(type) fixed_t Easing_ ## type (fixed_t t, fixed_t start, fixed_t end);

EASINGFUNCLIST()

#undef EASINGFUNC
#define EASINGFUNC(type) fixed_t Easing_ ## type (fixed_t t, fixed_t start, fixed_t end, fixed_t param);

EASINGFUNC(InBackParameterized) /* Easing_InBackParameterized */
EASINGFUNC(OutBackParameterized) /* Easing_OutBackParameterized */
EASINGFUNC(InOutBackParameterized) /* Easing_InOutBackParameterized */

#undef EASINGFUNC
#endif
