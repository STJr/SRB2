/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/
#ifndef _R_VBO_H_
#define _R_VBO_H_

typedef struct
{
	float x, y, z;		// Vertex
	float nx, ny, nz;	// Normal
	float s0, t0;		// Texcoord0
} vbo32_t;

typedef struct
{
	float x, y, z;	// Vertex
	float s0, t0;	// Texcoord0
	unsigned char r, g, b, a; // Color
	float pad[2]; // Pad
} vbo2d32_t;

typedef struct
{
	float x, y; // Vertex
	float s0, t0; // Texcoord0
} vbofont_t;

typedef struct
{
	short x, y, z; // Vertex
	char nx, ny, nz; // Normal
	char tanx, tany, tanz; // Tangent
	float s0, t0; // Texcoord0
} vbotiny_t;

typedef struct
{
	float x, y, z;      // Vertex
	float nx, ny, nz;   // Normal
	float s0, t0;       // Texcoord0
	float s1, t1;       // Texcoord1
	float s2, t2;       // Texcoord2
	float tan0, tan1, tan2; // Tangent
	unsigned char r, g, b, a;	// Color
} vbo64_t;

#endif
