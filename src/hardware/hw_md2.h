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
//
//-----------------------------------------------------------------------------
/// \file
/// \brief MD2 Handling
///	Inspired from md2.h by Mete Ciragan (mete@swissquake.ch)

#ifndef _HW_MD2_H_
#define _HW_MD2_H_

#include "hw_glob.h"

// magic number "IDP2" or 844121161
#define MD2_IDENT                       (INT32)(('2' << 24) + ('P' << 16) + ('D' << 8) + 'I')
// model version
#define MD2_VERSION                     8

#define MD2_MAX_TRIANGLES               8192
#define MD2_MAX_VERTICES                4096
#define MD2_MAX_TEXCOORDS               4096
#define MD2_MAX_FRAMES                  512
#define MD2_MAX_SKINS                   32
#define MD2_MAX_FRAMESIZE               (MD2_MAX_VERTICES * 4 + 128)

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	UINT32 magic;
	UINT32 version;
	UINT32 skinWidth;
	UINT32 skinHeight;
	UINT32 frameSize;
	UINT32 numSkins;
	UINT32 numVertices;
	UINT32 numTexCoords;
	UINT32 numTriangles;
	UINT32 numGlCommands;
	UINT32 numFrames;
	UINT32 offsetSkins;
	UINT32 offsetTexCoords;
	UINT32 offsetTriangles;
	UINT32 offsetFrames;
	UINT32 offsetGlCommands;
	UINT32 offsetEnd;
} ATTRPACK md2_header_t; //NOTE: each of md2_header's members are 4 unsigned bytes

typedef struct
{
	UINT8 vertex[3];
	UINT8 lightNormalIndex;
} ATTRPACK md2_alias_triangleVertex_t;

typedef struct
{
	float vertex[3];
	float normal[3];
} ATTRPACK md2_triangleVertex_t;

typedef struct
{
	INT16 vertexIndices[3];
	INT16 textureIndices[3];
} ATTRPACK md2_triangle_t;

typedef struct
{
	INT16 s, t;
} ATTRPACK md2_textureCoordinate_t;

typedef struct
{
	float scale[3];
	float translate[3];
	char name[16];
	md2_alias_triangleVertex_t alias_vertices[1];
} ATTRPACK md2_alias_frame_t;

typedef struct
{
	char name[16];
	md2_triangleVertex_t *vertices;
} ATTRPACK md2_frame_t;

typedef char md2_skin_t[64];

typedef struct
{
	float s, t;
	INT32 vertexIndex;
} ATTRPACK md2_glCommandVertex_t;

typedef struct
{
	md2_header_t            header;
	md2_skin_t              *skins;
	md2_textureCoordinate_t *texCoords;
	md2_triangle_t          *triangles;
	md2_frame_t             *frames;
	INT32                     *glCommandBuffer;
} ATTRPACK md2_model_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	char        filename[32];
	float       scale;
	float       offset;
	md2_model_t *model;
	void        *grpatch;
	void        *blendgrpatch;
	boolean     notfound;
	INT32       skin;
	boolean     error;
} md2_t;

extern md2_t md2_models[NUMSPRITES];
extern md2_t md2_playermodels[MAXSKINS];

void HWR_InitMD2(void);
void HWR_DrawMD2(gr_vissprite_t *spr);
void HWR_AddPlayerMD2(INT32 skin);
void HWR_AddSpriteMD2(size_t spritenum);

#endif // _HW_MD2_H_
