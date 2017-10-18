// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2005 by SRB2 Jr. Team.
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
/// \brief 3D sound definitions

#ifndef __HWS_DATA_H__
#define __HWS_DATA_H__

#define NORMAL_SEP 128

// abuse?
#define STATIC_SOURCES_NUM      6       // Total number of static sources

#define MIN_DISTANCE 160.0f
#define MAX_DISTANCE 1200.0f

typedef struct source_pos_s
{
	double   x;
	double   y;
	double   z;
} source_pos_t;

typedef struct source3D_pos_s
{
	float   x;
	float   y;
	float   z;
	//float   angle;
	float   momx;
	float   momy;
	float   momz;

} source3D_pos_t;


enum {NORMAL_PITCH = 128};

/*typedef struct source2D_data_s
{
	INT32     volume;
	INT32     sep;

} source2D_data_t;*/


// General 3D sound source description
typedef struct source3D_data_s
{
	float           min_distance;       //
	float           max_distance;       //
	INT32           head_relative;      //
	INT32           permanent;          //
	source3D_pos_t  pos;                // source position in 3D

} source3D_data_t;


// Sound data
typedef struct sfx_data_s
{
	size_t  length;
	void    *data;
	INT32   priority;
	INT32   sep;                    // Only when source is 2D sound
} sfx_data_t;


// Sound cone (for 3D sources)
typedef struct cone_def_s
{
	float   inner;
	float   outer;
	INT32   outer_gain;
	/*float   f_angle;
	float   h_angle;*/
} cone_def_t;


typedef struct listener_data_s
{
	// Listener position
	double  x;
	double  y;
	double  z;

	// Listener front and head orientation (degrees)
	double  f_angle;
	double  h_angle;

	// Listener momentums
	double  momx;
	double  momy;
	double  momz;
} listener_data_t;

typedef struct snddev_s
{
	INT32   sample_rate;
	INT32   bps;
	size_t  numsfxs;

// Windows specific data
#ifdef _WIN32
	UINT32   cooplevel;
	HWND    hWnd;
#endif
} snddev_t;

#endif //__HWS_DATA_H__
