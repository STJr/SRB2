// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime "Lactozilla" Passos.
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
	EASINGFUNC(Linear) sep \
 \
	EASINGFUNC(InSine) sep \
	EASINGFUNC(OutSine) sep \
	EASINGFUNC(InOutSine) sep \
 \
	EASINGFUNC(InQuad) sep \
	EASINGFUNC(OutQuad) sep \
	EASINGFUNC(InOutQuad) sep \
 \
	EASINGFUNC(InCubic) sep \
	EASINGFUNC(OutCubic) sep \
	EASINGFUNC(InOutCubic) sep \
 \
	EASINGFUNC(InQuart) sep \
	EASINGFUNC(OutQuart) sep \
	EASINGFUNC(InOutQuart) sep \
 \
	EASINGFUNC(InQuint) sep \
	EASINGFUNC(OutQuint) sep \
	EASINGFUNC(InOutQuint) sep \
 \
	EASINGFUNC(InExpo) sep \
	EASINGFUNC(OutExpo) sep \
	EASINGFUNC(InOutExpo) sep \
 \
	EASINGFUNC(InBack) sep \
	EASINGFUNC(OutBack) sep \
	EASINGFUNC(InOutBack) sep

#define EASINGFUNC(type) fixed_t Easing_ ## type (fixed_t start, fixed_t end, fixed_t t);

EASINGFUNCLIST()

#undef EASINGFUNC

#endif
