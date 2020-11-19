// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_md2.h
/// \brief 3D Model Handling
///	Inspired from md2.h by Mete Ciragan (mete@swissquake.ch)

#ifndef _HW_MD2_H_
#define _HW_MD2_H_

#include "hw_glob.h"
#include "hw_model.h"

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	char        filename[32];
	float       scale;
	float       offset;
	model_t     *model;
	void        *grpatch;
	boolean     notexturefile; // true if texture file was not found
	void        *blendgrpatch;
	boolean     noblendfile; // true if blend texture file was not found
	boolean     notfound;
	INT32       skin;
	boolean     error;
} md2_t;

extern md2_t md2_models[NUMSPRITES];
extern md2_t md2_playermodels[MAXSKINS];

void HWR_InitModels(void);
void HWR_AddPlayerModel(INT32 skin);
void HWR_AddSpriteModel(size_t spritenum);
boolean HWR_DrawModel(gl_vissprite_t *spr);

#define PLAYERMODELPREFIX "PLAYER"

#endif // _HW_MD2_H_
