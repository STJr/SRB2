// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_defs.h
/// \brief 3D hardware renderer definitions

#ifndef _HWR_DEFS_
#define _HWR_DEFS_
#include "../doomtype.h"
#include "../r_defs.h"

#define ZCLIP_PLANE 4.0f // Used for the actual game drawing
#define NZCLIP_PLANE 0.9f // Seems to be only used for the HUD and screen textures

// ==========================================================================
//                                                                     COLORS
// ==========================================================================

// byte value for paletted graphics, which represent the transparent color
#define GPU_PATCHES_CHROMAKEY_COLORINDEX   255

// the chroma key color shows on border sprites, set it to black
#define GPU_PATCHES_CHROMAKEY_COLORVALUE     (0x00000000)    //RGBA format as in grSstWinOpen()

#define GPU_DEFAULTMIX 0x00000000
#define GPU_DEFAULTFOG 0xFF000000

// RGBA Color components with float type ranging [ 0 ... 1 ]
struct FRGBAFloat
{
	float red, green, blue, alpha;
};
typedef struct FRGBAFloat FRGBAFloat;

// ==========================================================================
//                                                                    VECTORS
// ==========================================================================

// Simple 2D coordinate
typedef struct
{
	float x,y;
} F2DCoord, v2d_t;

// Simple 3D vector
typedef struct FVector
{
	float x,y,z;
} FVector;

// -----------
// structures
// -----------

//Hurdler: Transform (coords + angles)
//BP: transform order : scale(rotation_x(rotation_y(translation(v))))

// Kart features
//#define USE_FTRANSFORM_ANGLEZ
//#define USE_FTRANSFORM_MIRROR

// Vanilla features
#define USE_MODEL_NEXTFRAME

typedef struct
{
	float       x,y,z;           // position
#ifdef USE_FTRANSFORM_ANGLEZ
	float       anglex,angley,anglez;   // aimingangle / viewangle
#else
	float       anglex,angley;   // aimingangle / viewangle
#endif
	float       scalex,scaley,scalez;
	float       fovxangle, fovyangle;
	UINT8       splitscreen;
	boolean     flip;            // screenflip
	boolean     roll;
	SINT8       rollflip;
	float       rollangle; // done to not override USE_FTRANSFORM_ANGLEZ
	UINT8       rotaxis;
	float       centerx, centery;
#ifdef USE_FTRANSFORM_MIRROR
	boolean     mirror;          // SRB2Kart: Encore Mode
#endif
	boolean     shearing;        // 14042019
	float       viewaiming;      // 17052019
} FTransform;

// Transformed vector, as passed to HWR API
typedef struct
{
	float       x, y, z;
	float       s;            // s texture ordinate (s over w)
	float       t;            // t texture ordinate (t over w)
} FOutVector;

// ==========================================================================
//                                                               RENDER MODES
// ==========================================================================

// Flags describing how to render a polygon
// You pass a combination of these flags to DrawPolygon()
enum EPolyFlags
{
	// Mutually exclusive blend flags
	PF_Masked           = 0x00000001,   // Poly is alpha scaled and 0 alpha pixels are discarded (holes in texture)
	PF_Translucent      = 0x00000002,   // Poly is transparent, alpha = level of transparency
	PF_Environment      = 0x00000004,   // Poly should be drawn environment mapped. (Hurdler: used for text drawing)
	PF_Additive         = 0x00000008,   // Additive color blending
	PF_AdditiveSource   = 0x00000010,   // Source blending factor is additive. This is the opposite of regular additive blending.
	PF_Subtractive      = 0x00000020,   // Subtractive color blending
	PF_ReverseSubtract  = 0x00000040,   // Reverse subtract, used in wall splats (decals)
	PF_Multiplicative   = 0x00000080,   // Multiplicative color blending
	PF_Fog              = 0x20000000,   // Fog blocks
	PF_NoAlphaTest      = 0x40000000,   // Disables alpha testing
	PF_Blending         = (PF_Masked|PF_Translucent|PF_Environment|PF_Additive|PF_AdditiveSource|PF_Subtractive|PF_ReverseSubtract|PF_Multiplicative|PF_Fog) & ~PF_NoAlphaTest,

	// other flag bits
	PF_Occlude          = 0x00000100,   // Updates the depth buffer
	PF_NoDepthTest      = 0x00000200,   // Disables the depth test mode
	PF_Invisible        = 0x00000400,   // Disables write to color buffer
	PF_Decal            = 0x00000800,   // Enables polygon offset
	PF_Modulated        = 0x00001000,   // Modulation (multiply output with constant ARGB)
	                                    // When set, pass the color constant into the FSurfaceInfo -> PolyColor
	PF_NoTexture        = 0x00002000,   // Disables texturing
	PF_Corona           = 0x00004000,   // Tells the renderer we are drawing a corona
	PF_Ripple           = 0x00008000,   // Water effect shader
	PF_RemoveYWrap      = 0x00010000,   // Forces clamp texture on Y
	PF_ForceWrapX       = 0x00020000,   // Forces repeat texture on X
	PF_ForceWrapY       = 0x00040000    // Forces repeat texture on Y
};

enum ESurfFlags
{
	SF_DYNLIGHT         = 0x00000001,
};

enum ETextureFlags
{
	TF_WRAPX       = 0x00000001,        // wrap around X
	TF_WRAPY       = 0x00000002,        // wrap around Y
	TF_WRAPXY      = TF_WRAPY|TF_WRAPX, // very common so use alias is more easy
	TF_CHROMAKEYED = 0x00000010,
	TF_TRANSPARENT = 0x00000040,        // texture with some alpha == 0
};

struct FTextureInfo
{
	UINT32 width, height;
	UINT32 name;
	UINT32 format;

	struct HWRTexture_s *texture;
	struct FTextureInfo *prev, *next;
};
typedef struct FTextureInfo FTextureInfo;

struct FLightInfo
{
	UINT32 LightLevel;
	UINT32 FadeStart, FadeEnd;
};
typedef struct FLightInfo FLightInfo;

// Description of a renderable surface
struct FSurfaceInfo
{
	UINT32			PolyFlags;
	RGBA_t			PolyColor;
	RGBA_t			TintColor;
	RGBA_t			FadeColor;
	FLightInfo		LightInfo;
};
typedef struct FSurfaceInfo FSurfaceInfo;

// Hurdler: added for backward compatibility
enum EGPUState
{
	GPU_STATE_TEXTUREFILTERMODE,
	GPU_STATE_TEXTUREANISOTROPICMODE,
	GPU_STATE_SHADERS,
	GPU_STATE_MODEL_LIGHTING
};

enum EShaderOption
{
	GPU_SHADEROPTION_OFF,
	GPU_SHADEROPTION_ON,
	GPU_SHADEROPTION_NOCUSTOM,
};

enum EShaderInfo
{
	GPU_SHADERINFO_LEVELTIME = 1,
};

enum EFilterMode
{
	GPU_TEXFILTER_POINTSAMPLED,
	GPU_TEXFILTER_BILINEAR,
	GPU_TEXFILTER_TRILINEAR,
	GPU_TEXFILTER_MIXED1,
	GPU_TEXFILTER_MIXED2,
	GPU_TEXFILTER_MIXED3,
};

#ifdef GL_SHADERS
// Predefined shader types
enum
{
	SHADER_DEFAULT = 0,

	SHADER_FLOOR,
	SHADER_WALL,
	SHADER_SPRITE,
	SHADER_MODEL, SHADER_MODEL_LIGHTING,
	SHADER_WATER,
	SHADER_FOG,
	SHADER_SKY,

	NUMBASESHADERS,
};

// Maximum amount of shader programs
// Must be higher than NUMBASESHADERS
#define HWR_MAXSHADERS 16

// Shader sources (vertex and fragment)
struct FShaderSource
{
	char *vertex;
	char *fragment;
};
typedef struct FShaderSource FShaderSource;

// Custom shader reference table
struct FShaderReferenceArray
{
	const char *type;
	INT32 id;
};
typedef struct FShaderReferenceArray FShaderReferenceArray;

#endif

struct FSkyVertex
{
	float x, y, z;
	float u, v;
	unsigned char r, g, b, a;
};
typedef struct FSkyVertex FSkyVertex;

enum ESkyLoopMode
{
	GPU_SKYLOOP_FAN,
	GPU_SKYLOOP_STRIP
};

struct FSkyLoopDef
{
	int mode;
	int vertexcount;
	int vertexindex;
	boolean use_texture;
};
typedef struct FSkyLoopDef FSkyLoopDef;

struct FSkyDome
{
	unsigned int vbo;
	int rows, columns;
	int loopcount;

	int detail, vertex_count;
	int texture, width, height;
	boolean rebuild; // VBO needs to be rebuilt

	FSkyLoopDef *loops;
	FSkyVertex *data;
};
typedef struct FSkyDome FSkyDome;

#endif //_HWR_DEFS_
