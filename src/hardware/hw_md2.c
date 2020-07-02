// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_md2.c
/// \brief 3D Model Handling
///	Inspired from md2.c by Mete Ciragan (mete@swissquake.ch)

#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../d_main.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../fastcmp.h"

#ifdef HWRENDER
#include "hw_drv.h"
#include "hw_light.h"
#include "hw_md2.h"
#include "../d_main.h"
#include "../r_bsp.h"
#include "../r_main.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_things.h"
#include "../r_draw.h"
#include "../p_tick.h"
#include "hw_model.h"

#include "hw_main.h"
#include "../v_video.h"
#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "png.h"
 #ifndef PNG_READ_SUPPORTED
 #undef HAVE_PNG
 #endif
 #if PNG_LIBPNG_VER < 100207
 //#undef HAVE_PNG
 #endif
#endif

#ifndef errno
#include "errno.h"
#endif

md2_t md2_models[NUMSPRITES];
md2_t md2_playermodels[MAXSKINS];


/*
 * free model
 */
#if 0
static void md2_freeModel (model_t *model)
{
	UnloadModel(model);
}
#endif


//
// load model
//
// Hurdler: the current path is the Legacy.exe path
static model_t *md2_readModel(const char *filename)
{
	//Filename checking fixed ~Monster Iestyn and Golden
	return LoadModel(va("%s"PATHSEP"%s", srb2home, filename), PU_STATIC);
}

static inline void md2_printModelInfo (model_t *model)
{
#if 0
	INT32 i;

	CONS_Debug(DBG_RENDER, "magic:\t\t\t%c%c%c%c\n", model->header.magic>>24,
	            (model->header.magic>>16)&0xff,
	            (model->header.magic>>8)&0xff,
	             model->header.magic&0xff);
	CONS_Debug(DBG_RENDER, "version:\t\t%d\n", model->header.version);
	CONS_Debug(DBG_RENDER, "skinWidth:\t\t%d\n", model->header.skinWidth);
	CONS_Debug(DBG_RENDER, "skinHeight:\t\t%d\n", model->header.skinHeight);
	CONS_Debug(DBG_RENDER, "frameSize:\t\t%d\n", model->header.frameSize);
	CONS_Debug(DBG_RENDER, "numSkins:\t\t%d\n", model->header.numSkins);
	CONS_Debug(DBG_RENDER, "numVertices:\t\t%d\n", model->header.numVertices);
	CONS_Debug(DBG_RENDER, "numTexCoords:\t\t%d\n", model->header.numTexCoords);
	CONS_Debug(DBG_RENDER, "numTriangles:\t\t%d\n", model->header.numTriangles);
	CONS_Debug(DBG_RENDER, "numGlCommands:\t\t%d\n", model->header.numGlCommands);
	CONS_Debug(DBG_RENDER, "numFrames:\t\t%d\n", model->header.numFrames);
	CONS_Debug(DBG_RENDER, "offsetSkins:\t\t%d\n", model->header.offsetSkins);
	CONS_Debug(DBG_RENDER, "offsetTexCoords:\t%d\n", model->header.offsetTexCoords);
	CONS_Debug(DBG_RENDER, "offsetTriangles:\t%d\n", model->header.offsetTriangles);
	CONS_Debug(DBG_RENDER, "offsetFrames:\t\t%d\n", model->header.offsetFrames);
	CONS_Debug(DBG_RENDER, "offsetGlCommands:\t%d\n", model->header.offsetGlCommands);
	CONS_Debug(DBG_RENDER, "offsetEnd:\t\t%d\n", model->header.offsetEnd);

	for (i = 0; i < model->header.numFrames; i++)
		CONS_Debug(DBG_RENDER, "%s ", model->frames[i].name);
	CONS_Debug(DBG_RENDER, "\n");
#else
	(void)model;
#endif
}

#ifdef HAVE_PNG
static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);
	//I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static GrTextureFormat_t PNG_Load(const char *filename, int *w, int *h, GLPatch_t *grpatch)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;
	//Filename checking fixed ~Monster Iestyn and Golden
	char *pngfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2home, filename);

	FIL_ForceExtension(pngfilename, ".png");
	png_FILE = fopen(pngfilename, "rb");
	if (!png_FILE)
	{
		//CONS_Debug(DBG_RENDER, "M_SavePNG: Error on opening %s for loading\n", filename);
		return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on initialize libpng\n");
		fclose(png_FILE);
		return 0;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(png_FILE);
		return 0;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng load error on %s\n", filename);
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		fclose(png_FILE);
		Z_Free(grpatch->mipmap->grInfo.data);
		return 0;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);

	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type,
	 NULL, NULL, NULL);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);

	{
		png_uint_32 i, pitch = png_get_rowbytes(png_ptr, png_info_ptr);
		png_bytep PNG_image = Z_Malloc(pitch*height, PU_HWRMODELTEXTURE, &grpatch->mipmap->grInfo.data);
		png_bytepp row_pointers = png_malloc(png_ptr, height * sizeof (png_bytep));
		for (i = 0; i < height; i++)
			row_pointers[i] = PNG_image + i*pitch;
		png_read_image(png_ptr, row_pointers);
		png_free(png_ptr, (png_voidp)row_pointers);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	fclose(png_FILE);
	*w = (int)width;
	*h = (int)height;
	return GR_RGBA;
}
#endif

typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bitsPerPixel;
	INT16 xmin;
	INT16 ymin;
	INT16 xmax;
	INT16 ymax;
	INT16 hDpi;
	INT16 vDpi;
	UINT8 colorMap[48];
	UINT8 reserved;
	UINT8 numPlanes;
	INT16 bytesPerLine;
	INT16 paletteInfo;
	INT16 hScreenSize;
	INT16 vScreenSize;
	UINT8 filler[54];
} PcxHeader;

static GrTextureFormat_t PCX_Load(const char *filename, int *w, int *h,
	GLPatch_t *grpatch)
{
	PcxHeader header;
#define PALSIZE 768
	UINT8 palette[PALSIZE];
	const UINT8 *pal;
	RGBA_t *image;
	size_t pw, ph, size, ptr = 0;
	INT32 ch, rep;
	FILE *file;
	//Filename checking fixed ~Monster Iestyn and Golden
	char *pcxfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2home, filename);

	FIL_ForceExtension(pcxfilename, ".pcx");
	file = fopen(pcxfilename, "rb");
	if (!file)
		return 0;

	if (fread(&header, sizeof (PcxHeader), 1, file) != 1)
	{
		fclose(file);
		return 0;
	}

	if (header.bitsPerPixel != 8)
	{
		fclose(file);
		return 0;
	}

	fseek(file, -PALSIZE, SEEK_END);

	pw = *w = header.xmax - header.xmin + 1;
	ph = *h = header.ymax - header.ymin + 1;
	image = Z_Malloc(pw*ph*4, PU_HWRMODELTEXTURE, &grpatch->mipmap->grInfo.data);

	if (fread(palette, sizeof (UINT8), PALSIZE, file) != PALSIZE)
	{
		Z_Free(image);
		fclose(file);
		return 0;
	}
	fseek(file, sizeof (PcxHeader), SEEK_SET);

	size = pw * ph;
	while (ptr < size)
	{
		ch = fgetc(file);  //Hurdler: beurk
		if (ch >= 192)
		{
			rep = ch - 192;
			ch = fgetc(file);
		}
		else
		{
			rep = 1;
		}
		while (rep--)
		{
			pal = palette + ch*3;
			image[ptr].s.red   = *pal++;
			image[ptr].s.green = *pal++;
			image[ptr].s.blue  = *pal++;
			image[ptr].s.alpha = 0xFF;
			ptr++;
		}
	}
	fclose(file);
	return GR_RGBA;
}

// -----------------+
// md2_loadTexture  : Download a pcx or png texture for models
// -----------------+
static void md2_loadTexture(md2_t *model)
{
	GLPatch_t *grpatch;
	const char *filename = model->filename;

	if (model->grpatch)
	{
		grpatch = model->grpatch;
		Z_Free(grpatch->mipmap->grInfo.data);
	}
	else
	{
		grpatch = Z_Calloc(sizeof *grpatch, PU_HWRPATCHINFO,
		                   &(model->grpatch));
		grpatch->mipmap = Z_Calloc(sizeof (GLMipmap_t), PU_HWRPATCHINFO, NULL);
	}

	if (!grpatch->mipmap->downloaded && !grpatch->mipmap->grInfo.data)
	{
		int w = 0, h = 0;
		UINT32 size;
		RGBA_t *image;

#ifdef HAVE_PNG
		grpatch->mipmap->grInfo.format = PNG_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap->grInfo.format == 0)
#endif
		grpatch->mipmap->grInfo.format = PCX_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap->grInfo.format == 0)
		{
			model->notexturefile = true; // mark it so its not searched for again repeatedly
			return;
		}

		grpatch->mipmap->downloaded = 0;
		grpatch->mipmap->flags = 0;

		grpatch->width = (INT16)w;
		grpatch->height = (INT16)h;
		grpatch->mipmap->width = (UINT16)w;
		grpatch->mipmap->height = (UINT16)h;

		// Lactozilla: Apply colour cube
		image = grpatch->mipmap->grInfo.data;
		size = w*h;
		while (size--)
		{
			V_CubeApply(&image->s.red, &image->s.green, &image->s.blue);
			image++;
		}
	}
	HWD.pfnSetTexture(grpatch->mipmap);
}

// -----------------+
// md2_loadBlendTexture  : Download a pcx or png texture for blending MD2 models
// -----------------+
static void md2_loadBlendTexture(md2_t *model)
{
	GLPatch_t *grpatch;
	char *filename = Z_Malloc(strlen(model->filename)+7, PU_STATIC, NULL);
	strcpy(filename, model->filename);

	FIL_ForceExtension(filename, "_blend.png");

	if (model->blendgrpatch)
	{
		grpatch = model->blendgrpatch;
		Z_Free(grpatch->mipmap->grInfo.data);
	}
	else
	{
		grpatch = Z_Calloc(sizeof *grpatch, PU_HWRPATCHINFO,
		                   &(model->blendgrpatch));
		grpatch->mipmap = Z_Calloc(sizeof (GLMipmap_t), PU_HWRPATCHINFO, NULL);
	}

	if (!grpatch->mipmap->downloaded && !grpatch->mipmap->grInfo.data)
	{
		int w = 0, h = 0;
#ifdef HAVE_PNG
		grpatch->mipmap->grInfo.format = PNG_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap->grInfo.format == 0)
#endif
		grpatch->mipmap->grInfo.format = PCX_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap->grInfo.format == 0)
		{
			model->noblendfile = true; // mark it so its not searched for again repeatedly
			Z_Free(filename);
			return;
		}

		grpatch->mipmap->downloaded = 0;
		grpatch->mipmap->flags = 0;

		grpatch->width = (INT16)w;
		grpatch->height = (INT16)h;
		grpatch->mipmap->width = (UINT16)w;
		grpatch->mipmap->height = (UINT16)h;
	}
	HWD.pfnSetTexture(grpatch->mipmap); // We do need to do this so that it can be cleared and knows to recreate it when necessary

	Z_Free(filename);
}

// Don't spam the console, or the OS with fopen requests!
static boolean nomd2s = false;

void HWR_InitModels(void)
{
	size_t i;
	INT32 s;
	FILE *f;
	char name[24], filename[32];
	float scale, offset;
	size_t prefixlen;

	CONS_Printf("HWR_InitModels()...\n");
	for (s = 0; s < MAXSKINS; s++)
	{
		md2_playermodels[s].scale = -1.0f;
		md2_playermodels[s].model = NULL;
		md2_playermodels[s].grpatch = NULL;
		md2_playermodels[s].notexturefile = false;
		md2_playermodels[s].noblendfile = false;
		md2_playermodels[s].skin = -1;
		md2_playermodels[s].notfound = true;
		md2_playermodels[s].error = false;
	}
	for (i = 0; i < NUMSPRITES; i++)
	{
		md2_models[i].scale = -1.0f;
		md2_models[i].model = NULL;
		md2_models[i].grpatch = NULL;
		md2_models[i].notexturefile = false;
		md2_models[i].noblendfile = false;
		md2_models[i].skin = -1;
		md2_models[i].notfound = true;
		md2_models[i].error = false;
	}

	// read the models.dat file
	//Filename checking fixed ~Monster Iestyn and Golden
	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		CONS_Printf("%s %s\n", M_GetText("Error while loading models.dat:"), strerror(errno));
		nomd2s = true;
		return;
	}

	// length of the player model prefix
	prefixlen = strlen(PLAYERMODELPREFIX);

	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		char *skinname = name;
		size_t len = strlen(name);

		// check for the player model prefix.
		if (!strnicmp(name, PLAYERMODELPREFIX, prefixlen) && (len > prefixlen))
		{
			skinname += prefixlen;
			goto addskinmodel;
		}

		// add sprite model
		if (len == 4) // must be 4 characters long exactly. otherwise it's not a sprite name.
		{
			for (i = 0; i < NUMSPRITES; i++)
			{
				if (stricmp(name, sprnames[i]) == 0)
				{
					md2_models[i].scale = scale;
					md2_models[i].offset = offset;
					md2_models[i].notfound = false;
					strcpy(md2_models[i].filename, filename);
					goto modelfound;
				}
			}
		}

addskinmodel:
		// add player model
		for (s = 0; s < MAXSKINS; s++)
		{
			if (stricmp(skinname, skins[s].name) == 0)
			{
				md2_playermodels[s].skin = s;
				md2_playermodels[s].scale = scale;
				md2_playermodels[s].offset = offset;
				md2_playermodels[s].notfound = false;
				strcpy(md2_playermodels[s].filename, filename);
				goto modelfound;
			}
		}

modelfound:
		// move on to next line...
		continue;
	}
	fclose(f);
}

void HWR_AddPlayerModel(int skin) // For skins that were added after startup
{
	FILE *f;
	char name[24], filename[32];
	float scale, offset;
	size_t prefixlen;

	if (nomd2s)
		return;

	//CONS_Printf("HWR_AddPlayerModel()...\n");

	// read the models.dat file
	//Filename checking fixed ~Monster Iestyn and Golden
	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		CONS_Printf("Error while loading models.dat\n");
		nomd2s = true;
		return;
	}

	// length of the player model prefix
	prefixlen = strlen(PLAYERMODELPREFIX);

	// Check for any models that match the names of player skins!
	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		char *skinname = name;
		size_t len = strlen(name);

		// ignore the player model prefix.
		if (!strnicmp(name, PLAYERMODELPREFIX, prefixlen) && (len > prefixlen))
			skinname += prefixlen;

		if (stricmp(skinname, skins[skin].name) == 0)
		{
			md2_playermodels[skin].skin = skin;
			md2_playermodels[skin].scale = scale;
			md2_playermodels[skin].offset = offset;
			md2_playermodels[skin].notfound = false;
			strcpy(md2_playermodels[skin].filename, filename);
			goto playermodelfound;
		}
	}

	md2_playermodels[skin].notfound = true;
playermodelfound:
	fclose(f);
}

void HWR_AddSpriteModel(size_t spritenum) // For sprites that were added after startup
{
	FILE *f;
	// name[24] is used to check for names in the models.dat file that match with sprites or player skins
	// sprite names are always 4 characters long, and names is for player skins can be up to 19 characters long
	// PLAYERMODELPREFIX is 6 characters long
	char name[24], filename[32];
	float scale, offset;

	if (nomd2s)
		return;

	if (spritenum == SPR_PLAY) // Handled already NEWMD2: Per sprite, per-skin check
		return;

	// Read the models.dat file
	//Filename checking fixed ~Monster Iestyn and Golden
	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		CONS_Printf("Error while loading models.dat\n");
		nomd2s = true;
		return;
	}

	// Check for any models that match the names of sprite names!
	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		// length of the sprite name
		size_t len = strlen(name);
		if (len != 4) // must be 4 characters long exactly. otherwise it's not a sprite name.
			continue;

		// check for the player model prefix.
		if (!strnicmp(name, PLAYERMODELPREFIX, strlen(PLAYERMODELPREFIX)))
			continue; // that's not a sprite...

		if (stricmp(name, sprnames[spritenum]) == 0)
		{
			md2_models[spritenum].scale = scale;
			md2_models[spritenum].offset = offset;
			md2_models[spritenum].notfound = false;
			strcpy(md2_models[spritenum].filename, filename);
			goto spritemodelfound;
		}
	}

	md2_models[spritenum].notfound = true;
spritemodelfound:
	fclose(f);
}

// Define for getting accurate color brightness readings according to how the human eye sees them.
// https://en.wikipedia.org/wiki/Relative_luminance
// 0.2126 to red
// 0.7152 to green
// 0.0722 to blue
#define SETBRIGHTNESS(brightness,r,g,b) \
	brightness = (UINT8)(((1063*(UINT16)(r))/5000) + ((3576*(UINT16)(g))/5000) + ((361*(UINT16)(b))/5000))

static void HWR_CreateBlendedTexture(GLPatch_t *gpatch, GLPatch_t *blendgpatch, GLMipmap_t *grmip, INT32 skinnum, skincolornum_t color)
{
	UINT16 w = gpatch->width, h = gpatch->height;
	UINT32 size = w*h;
	RGBA_t *image, *blendimage, *cur, blendcolor;
	UINT16 translation[16]; // First the color index
	UINT8 cutoff[16]; // Brightness cutoff before using the next color
	UINT8 translen = 0;
	UINT8 i;

	blendcolor = V_GetColor(0); // initialize
	memset(translation, 0, sizeof(translation));
	memset(cutoff, 0, sizeof(cutoff));

	if (grmip->width == 0)
	{
		grmip->width = gpatch->width;
		grmip->height = gpatch->height;

		// no wrap around, no chroma key
		grmip->flags = 0;
		// setup the texture info
		grmip->grInfo.format = GR_RGBA;
	}

	if (grmip->grInfo.data)
	{
		Z_Free(grmip->grInfo.data);
		grmip->grInfo.data = NULL;
	}

	cur = Z_Malloc(size*4, PU_HWRMODELTEXTURE, &grmip->grInfo.data);
	memset(cur, 0x00, size*4);

	image = gpatch->mipmap->grInfo.data;
	blendimage = blendgpatch->mipmap->grInfo.data;

	// TC_METALSONIC includes an actual skincolor translation, on top of its flashing.
	if (skinnum == TC_METALSONIC)
		color = SKINCOLOR_COBALT;

	if (color != SKINCOLOR_NONE && color < numskincolors)
	{
		UINT8 numdupes = 1;

		translation[translen] = skincolors[color].ramp[0];
		cutoff[translen] = 255;

		for (i = 1; i < 16; i++)
		{
			if (translation[translen] == skincolors[color].ramp[i])
			{
				numdupes++;
				continue;
			}

			if (translen > 0)
			{
				cutoff[translen] = cutoff[translen-1] - (256 / (16 / numdupes));
			}

			numdupes = 1;
			translen++;

			translation[translen] = (UINT16)skincolors[color].ramp[i];
		}

		translen++;
	}

	while (size--)
	{
		if (skinnum == TC_BOSS)
		{
			// Turn everything below a certain threshold white
			if ((image->s.red == image->s.green) && (image->s.green == image->s.blue) && image->s.blue < 127)
			{
				// Lactozilla: Invert the colors
				cur->s.red = cur->s.green = cur->s.blue = (255 - image->s.blue);
			}
			else
			{
				cur->s.red = image->s.red;
				cur->s.green = image->s.green;
				cur->s.blue = image->s.blue;
			}

			cur->s.alpha = image->s.alpha;
		}
		else if (skinnum == TC_ALLWHITE)
		{
			// Turn everything white
			cur->s.red = cur->s.green = cur->s.blue = 255;
			cur->s.alpha = image->s.alpha;
		}
		else
		{
			// Everything below requires a blend image
			if (blendimage == NULL)
			{
				cur->rgba = image->rgba;
				goto skippixel;
			}

			// Metal Sonic dash mode
			if (skinnum == TC_DASHMODE)
			{
				if (image->s.alpha == 0 && blendimage->s.alpha == 0)
				{
					// Don't bother with blending the pixel if the alpha of the blend pixel is 0
					cur->rgba = image->rgba;
				}
				else
				{
					UINT8 ialpha = 255 - blendimage->s.alpha, balpha = blendimage->s.alpha;
					RGBA_t icolor = *image, bcolor;

					memset(&bcolor, 0x00, sizeof(RGBA_t));

					if (blendimage->s.alpha)
					{
						bcolor.s.blue = 0;
						bcolor.s.red = 255;
						bcolor.s.green = (blendimage->s.red + blendimage->s.green + blendimage->s.blue) / 3;
					}

					if (image->s.alpha && image->s.red > image->s.green << 1) // this is pretty arbitrary, but it works well for Metal Sonic
					{
						icolor.s.red = image->s.blue;
						icolor.s.blue = image->s.red;
					}

					cur->s.red = (ialpha * icolor.s.red + balpha * bcolor.s.red)/255;
					cur->s.green = (ialpha * icolor.s.green + balpha * bcolor.s.green)/255;
					cur->s.blue = (ialpha * icolor.s.blue + balpha * bcolor.s.blue)/255;
					cur->s.alpha = image->s.alpha;
				}
			}
			else
			{
				// All settings that use skincolors!
				UINT16 brightness;

				if (translen <= 0)
				{
					cur->rgba = image->rgba;
					goto skippixel;
				}

				// Don't bother with blending the pixel if the alpha of the blend pixel is 0
				if (skinnum == TC_RAINBOW)
				{
					if (image->s.alpha == 0 && blendimage->s.alpha == 0)
					{
						cur->rgba = image->rgba;
						goto skippixel;
					}
					else
					{
						UINT16 imagebright, blendbright;
						SETBRIGHTNESS(imagebright,image->s.red,image->s.green,image->s.blue);
						SETBRIGHTNESS(blendbright,blendimage->s.red,blendimage->s.green,blendimage->s.blue);
						// slightly dumb average between the blend image color and base image colour, usually one or the other will be fully opaque anyway
						brightness = (imagebright*(255-blendimage->s.alpha))/255 + (blendbright*blendimage->s.alpha)/255;
					}
				}
				else
				{
					if (blendimage->s.alpha == 0)
					{
						cur->rgba = image->rgba;
						goto skippixel; // for metal sonic blend
					}
					else
					{
						SETBRIGHTNESS(brightness,blendimage->s.red,blendimage->s.green,blendimage->s.blue);
					}
				}

				// Calculate a sort of "gradient" for the skincolor
				// (Me splitting this into a function didn't work, so I had to ruin this entire function's groove...)
				{
					RGBA_t nextcolor;
					UINT8 firsti, secondi, mul, mulmax;
					INT32 r, g, b;

					// Rainbow needs to find the closest match to the textures themselves, instead of matching brightnesses to other colors.
					// Ensue horrible mess.
					if (skinnum == TC_RAINBOW)
					{
						UINT16 brightdif = 256;
						UINT8 colorbrightnesses[16];
						INT32 compare, m, d;

						// Ignore pure white & pitch black
						if (brightness > 253 || brightness < 2)
						{
							cur->rgba = image->rgba;
							cur++; image++; blendimage++;
							continue;
						}

						firsti = 0;
						mul = 0;
						mulmax = 1;

						for (i = 0; i < translen; i++)
						{
							RGBA_t tempc = V_GetColor(translation[i]);
							SETBRIGHTNESS(colorbrightnesses[i], tempc.s.red, tempc.s.green, tempc.s.blue); // store brightnesses for comparison
						}

						for (i = 0; i < translen; i++)
						{
							if (brightness > colorbrightnesses[i]) // don't allow greater matches (because calculating a makeshift gradient for this is already a huge mess as is)
								continue;

							compare = abs((INT16)(colorbrightnesses[i]) - (INT16)(brightness));

							if (compare < brightdif)
							{
								brightdif = (UINT16)compare;
								firsti = i; // best matching color that's equal brightness or darker
							}
						}

						secondi = firsti+1; // next color in line
						if (secondi >= translen)
						{
							m = (INT16)brightness; // - 0;
							d = (INT16)colorbrightnesses[firsti]; // - 0;
						}
						else
						{
							m = (INT16)brightness - (INT16)colorbrightnesses[secondi];
							d = (INT16)colorbrightnesses[firsti] - (INT16)colorbrightnesses[secondi];
						}

						if (m >= d)
							m = d-1;

						mulmax = 16;

						// calculate the "gradient" multiplier based on how close this color is to the one next in line
						if (m <= 0 || d <= 0)
							mul = 0;
						else
							mul = (mulmax-1) - ((m * mulmax) / d);
					}
					else
					{
						// Just convert brightness to a skincolor value, use distance to next position to find the gradient multipler
						firsti = 0;

						for (i = 1; i < translen; i++)
						{
							if (brightness >= cutoff[i])
								break;
							firsti = i;
						}

						secondi = firsti+1;

						mulmax = cutoff[firsti];
						if (secondi < translen)
							mulmax -= cutoff[secondi];

						mul = cutoff[firsti] - brightness;
					}

					blendcolor = V_GetColor(translation[firsti]);

					if (secondi >= translen)
						mul = 0;

					if (mul > 0) // If it's 0, then we only need the first color.
					{
#if 0
						if (secondi >= translen)
						{
							// blend to black
							nextcolor = V_GetColor(31);
						}
						else
#endif
							nextcolor = V_GetColor(translation[secondi]);

						// Find difference between points
						r = (INT32)(nextcolor.s.red - blendcolor.s.red);
						g = (INT32)(nextcolor.s.green - blendcolor.s.green);
						b = (INT32)(nextcolor.s.blue - blendcolor.s.blue);

						// Find the gradient of the two points
						r = ((mul * r) / mulmax);
						g = ((mul * g) / mulmax);
						b = ((mul * b) / mulmax);

						// Add gradient value to color
						blendcolor.s.red += r;
						blendcolor.s.green += g;
						blendcolor.s.blue += b;
					}
				}

				if (skinnum == TC_RAINBOW)
				{
					UINT32 tempcolor;
					UINT16 colorbright;

					SETBRIGHTNESS(colorbright,blendcolor.s.red,blendcolor.s.green,blendcolor.s.blue);
					if (colorbright == 0)
						colorbright = 1; // no dividing by 0 please

					tempcolor = (brightness * blendcolor.s.red) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.red = (UINT8)tempcolor;

					tempcolor = (brightness * blendcolor.s.green) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.green = (UINT8)tempcolor;

					tempcolor = (brightness * blendcolor.s.blue) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.blue = (UINT8)tempcolor;
					cur->s.alpha = image->s.alpha;
				}
				else
				{
					// Color strength depends on image alpha
					INT32 tempcolor;

					tempcolor = ((image->s.red * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.red * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.red = (UINT8)tempcolor;

					tempcolor = ((image->s.green * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.green * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.green = (UINT8)tempcolor;

					tempcolor = ((image->s.blue * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.blue * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.blue = (UINT8)tempcolor;
					cur->s.alpha = image->s.alpha;
				}

skippixel:

				// *Now* we can do Metal Sonic's flashing
				if (skinnum == TC_METALSONIC)
				{
					// Blend dark blue into white
					if (cur->s.alpha > 0 && cur->s.red == 0 && cur->s.green == 0 && cur->s.blue < 255 && cur->s.blue > 31)
					{
						// Sal: Invert non-blue
						cur->s.red = cur->s.green = (255 - cur->s.blue);
						cur->s.blue = 255;
					}

					cur->s.alpha = image->s.alpha;
				}
			}
		}

		cur++; image++;

		if (blendimage != NULL)
			blendimage++;
	}

	return;
}

#undef SETBRIGHTNESS

static void HWR_GetBlendedTexture(GLPatch_t *gpatch, GLPatch_t *blendgpatch, INT32 skinnum, const UINT8 *colormap, skincolornum_t color)
{
	// mostly copied from HWR_GetMappedPatch, hence the similarities and comment
	GLMipmap_t *grmip, *newmip;

	if (colormap == colormaps || colormap == NULL)
	{
		// Don't do any blending
		HWD.pfnSetTexture(gpatch->mipmap);
		return;
	}

	if ((blendgpatch && blendgpatch->mipmap->grInfo.format)
		&& (gpatch->width != blendgpatch->width || gpatch->height != blendgpatch->height))
	{
		// Blend image exists, but it's bad.
		HWD.pfnSetTexture(gpatch->mipmap);
		return;
	}

	// search for the mipmap
	// skip the first (no colormap translated)
	for (grmip = gpatch->mipmap; grmip->nextcolormap; )
	{
		grmip = grmip->nextcolormap;
		if (grmip->colormap == colormap)
		{
			if (grmip->downloaded && grmip->grInfo.data)
			{
				HWD.pfnSetTexture(grmip); // found the colormap, set it to the correct texture
				Z_ChangeTag(grmip->grInfo.data, PU_HWRMODELTEXTURE_UNLOCKED);
				return;
			}
		}
	}

	// If here, the blended texture has not been created
	// So we create it

	//BP: WARNING: don't free it manually without clearing the cache of harware renderer
	//              (it have a liste of mipmap)
	//    this malloc is cleared in HWR_FreeTextureCache
	//    (...) unfortunately z_malloc fragment alot the memory :(so malloc is better
	newmip = calloc(1, sizeof (*newmip));
	if (newmip == NULL)
		I_Error("%s: Out of memory", "HWR_GetBlendedTexture");
	grmip->nextcolormap = newmip;
	newmip->colormap = colormap;

	HWR_CreateBlendedTexture(gpatch, blendgpatch, newmip, skinnum, color);

	HWD.pfnSetTexture(newmip);
	Z_ChangeTag(newmip->grInfo.data, PU_HWRMODELTEXTURE_UNLOCKED);
}

#define NORMALFOG 0x00000000
#define FADEFOG 0x19000000

static boolean HWR_AllowModel(mobj_t *mobj)
{
	// Signpost overlay. Not needed.
	if (mobj->state-states == S_PLAY_SIGN)
		return false;

	// Otherwise, render the model.
	return true;
}

static boolean HWR_CanInterpolateModel(mobj_t *mobj, model_t *model)
{
	if (cv_grmodelinterpolation.value == 2) // Always interpolate
		return true;
	return model->interpolate[(mobj->frame & FF_FRAMEMASK)];
}

static boolean HWR_CanInterpolateSprite2(modelspr2frames_t *spr2frame)
{
	if (cv_grmodelinterpolation.value == 2) // Always interpolate
		return true;
	return spr2frame->interpolate;
}

//
// HWR_GetModelSprite2 (see P_GetSkinSprite2)
// For non-super players, tries each sprite2's immediate predecessor until it finds one with a number of frames or ends up at standing.
// For super players, does the same as above - but tries the super equivalent for each sprite2 before the non-super version.
//

static UINT8 HWR_GetModelSprite2(md2_t *md2, skin_t *skin, UINT8 spr2, player_t *player)
{
	UINT8 super = 0, i = 0;

	if (!md2 || !md2->model || !md2->model->spr2frames || !skin)
		return 0;

	if ((playersprite_t)(spr2 & ~FF_SPR2SUPER) >= free_spr2)
		return 0;

	while (!md2->model->spr2frames[spr2].numframes
		&& spr2 != SPR2_STND
		&& ++i != 32) // recursion limiter
	{
		if (spr2 & FF_SPR2SUPER)
		{
			super = FF_SPR2SUPER;
			spr2 &= ~FF_SPR2SUPER;
			continue;
		}

		switch(spr2)
		{
		// Normal special cases.
		case SPR2_JUMP:
			spr2 = ((player
					? player->charflags
					: skin->flags)
					& SF_NOJUMPSPIN) ? SPR2_SPNG : SPR2_ROLL;
			break;
		case SPR2_TIRE:
			spr2 = ((player
					? player->charability
					: skin->ability)
					== CA_SWIM) ? SPR2_SWIM : SPR2_FLY;
			break;
		// Use the handy list, that's what it's there for!
		default:
			spr2 = spr2defaults[spr2];
			break;
		}

		spr2 |= super;
	}

	if (i >= 32) // probably an infinite loop...
		return 0;

	return spr2;
}

//
// HWR_DrawModel
//

boolean HWR_DrawModel(gr_vissprite_t *spr)
{
	md2_t *md2;

	char filename[64];
	INT32 frame = 0;
	INT32 nextFrame = -1;
	UINT8 spr2 = 0;
	FTransform p;
	FSurfaceInfo Surf;

	if (!cv_grmodels.value)
		return false;

	if (spr->precip)
		return false;

	memset(&p, 0x00, sizeof(FTransform));

	// MD2 colormap fix
	// colormap test
	if (spr->mobj->subsector)
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = NULL;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false); // Always use the light at the top instead of whatever I was doing before

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel > 255 ? 255 : *sector->lightlist[light].lightlevel;

			if (*sector->lightlist[light].extra_colormap)
				colormap = *sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = sector->lightlevel > 255 ? 255 : sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		HWR_Lighting(&Surf, lightlevel, colormap);
	}
	else
		Surf.PolyColor.rgba = 0xFFFFFFFF;

	// Look at HWR_ProjectSprite for more
	{
		GLPatch_t *gpatch;
		INT32 durs = spr->mobj->state->tics;
		INT32 tics = spr->mobj->tics;
		//mdlframe_t *next = NULL;
		const boolean papersprite = (spr->mobj->frame & FF_PAPERSPRITE);
		const UINT8 flip = (UINT8)(!(spr->mobj->eflags & MFE_VERTICALFLIP) != !(spr->mobj->frame & FF_VERTICALFLIP));
		spritedef_t *sprdef;
		spriteframe_t *sprframe;
		spriteinfo_t *sprinfo;
		angle_t ang;
		INT32 mod;
		float finalscale;

		// Apparently people don't like jump frames like that, so back it goes
		//if (tics > durs)
			//durs = tics;

		if (spr->mobj->flags2 & MF2_SHADOW)
			Surf.PolyColor.s.alpha = 0x40;
		else if (spr->mobj->frame & FF_TRANSMASK)
			HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else
			Surf.PolyColor.s.alpha = 0xFF;

		// dont forget to enabled the depth test because we can't do this like
		// before: polygons models are not sorted

		// 1. load model+texture if not already loaded
		// 2. draw model with correct position, rotation,...
		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY) // Use the player MD2 list if the mobj has a skin and is using the player sprites
		{
			md2 = &md2_playermodels[(skin_t*)spr->mobj->skin-skins];
			md2->skin = (skin_t*)spr->mobj->skin-skins;
			sprinfo = &((skin_t *)spr->mobj->skin)->sprinfo[spr->mobj->sprite2];
		}
		else
		{
			md2 = &md2_models[spr->mobj->sprite];
			sprinfo = &spriteinfo[spr->mobj->sprite];
		}

		if (md2->error)
			return false; // we already failed loading this before :(
		if (!md2->model)
		{
			//CONS_Debug(DBG_RENDER, "Loading model... (%s)", sprnames[spr->mobj->sprite]);
			sprintf(filename, "models/%s", md2->filename);
			md2->model = md2_readModel(filename);

			if (md2->model)
			{
				md2_printModelInfo(md2->model);
				HWD.pfnCreateModelVBOs(md2->model);
			}
			else
			{
				//CONS_Debug(DBG_RENDER, " FAILED\n");
				md2->error = true; // prevent endless fail
				return false;
			}
		}

		// Lactozilla: Disallow certain models from rendering
		if (!HWR_AllowModel(spr->mobj))
			return false;

		//HWD.pfnSetBlend(blend); // This seems to actually break translucency?
		finalscale = md2->scale;
		//Hurdler: arf, I don't like that implementation at all... too much crappy
		gpatch = md2->grpatch;
		if (!gpatch || ((!gpatch->mipmap->grInfo.format || !gpatch->mipmap->downloaded) && !md2->notexturefile))
			md2_loadTexture(md2);
		gpatch = md2->grpatch; // Load it again, because it isn't being loaded into gpatch after md2_loadtexture...

		if ((gpatch && gpatch->mipmap->grInfo.format) // don't load the blend texture if the base texture isn't available
			&& (!md2->blendgrpatch
			|| ((!((GLPatch_t *)md2->blendgrpatch)->mipmap->grInfo.format || !((GLPatch_t *)md2->blendgrpatch)->mipmap->downloaded)
			&& !md2->noblendfile)))
			md2_loadBlendTexture(md2);

		if (gpatch && gpatch->mipmap->grInfo.format) // else if meant that if a texture couldn't be loaded, it would just end up using something else's texture
		{
			INT32 skinnum = TC_DEFAULT;

			if ((spr->mobj->flags & (MF_ENEMY|MF_BOSS)) && (spr->mobj->flags2 & MF2_FRET) && !(spr->mobj->flags & MF_GRENADEBOUNCE) && (leveltime & 1)) // Bosses "flash"
			{
				if (spr->mobj->type == MT_CYBRAKDEMON || spr->mobj->colorized)
					skinnum = TC_ALLWHITE;
				else if (spr->mobj->type == MT_METALSONIC_BATTLE)
					skinnum = TC_METALSONIC;
				else
					skinnum = TC_BOSS;
			}
			else if ((skincolornum_t)spr->mobj->color != SKINCOLOR_NONE)
			{
				if (spr->mobj->colorized)
					skinnum = TC_RAINBOW;
				else if (spr->mobj->player && spr->mobj->player->dashmode >= DASHMODE_THRESHOLD
					&& (spr->mobj->player->charflags & SF_DASHMODE)
					&& ((leveltime/2) & 1))
				{
					if (spr->mobj->player->charflags & SF_MACHINE)
						skinnum = TC_DASHMODE;
					else
						skinnum = TC_RAINBOW;
				}
				else if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
					skinnum = (INT32)((skin_t*)spr->mobj->skin-skins);
				else
					skinnum = TC_DEFAULT;
			}

			// Translation or skin number found
			HWR_GetBlendedTexture(gpatch, (GLPatch_t *)md2->blendgrpatch, skinnum, spr->colormap, (skincolornum_t)spr->mobj->color);
		}
		else
		{
			// Sprite
			gpatch = spr->gpatch; //W_CachePatchNum(spr->patchlumpnum, PU_CACHE);
			HWR_GetMappedPatch(gpatch, spr->colormap);
		}

		if (spr->mobj->frame & FF_ANIMATE)
		{
			// set duration and tics to be the correct values for FF_ANIMATE states
			durs = spr->mobj->state->var2;
			tics = spr->mobj->anim_duration;
		}

		frame = (spr->mobj->frame & FF_FRAMEMASK);
		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY && md2->model->spr2frames)
		{
			spr2 = HWR_GetModelSprite2(md2, spr->mobj->skin, spr->mobj->sprite2, spr->mobj->player);
			mod = md2->model->spr2frames[spr2].numframes;
#ifndef DONTHIDEDIFFANIMLENGTH // by default, different anim length is masked by the mod
			if (mod > (INT32)((skin_t *)spr->mobj->skin)->sprites[spr2].numframes)
				mod = ((skin_t *)spr->mobj->skin)->sprites[spr2].numframes;
#endif
			if (!mod)
				mod = 1;
			frame = md2->model->spr2frames[spr2].frames[frame%mod];
		}
		else
		{
			mod = md2->model->meshes[0].numFrames;
			if (!mod)
				mod = 1;
		}

#ifdef USE_MODEL_NEXTFRAME
#define INTERPOLERATION_LIMIT TICRATE/4
		if (cv_grmodelinterpolation.value && tics <= durs && tics <= INTERPOLERATION_LIMIT)
		{
			if (durs > INTERPOLERATION_LIMIT)
				durs = INTERPOLERATION_LIMIT;

			if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY && md2->model->spr2frames)
			{
				if (HWR_CanInterpolateSprite2(&md2->model->spr2frames[spr2])
					&& (spr->mobj->frame & FF_ANIMATE
					|| (spr->mobj->state->nextstate != S_NULL
					&& states[spr->mobj->state->nextstate].sprite == SPR_PLAY
					&& ((P_GetSkinSprite2(spr->mobj->skin, (((spr->mobj->player && spr->mobj->player->powers[pw_super]) ? FF_SPR2SUPER : 0)|states[spr->mobj->state->nextstate].frame) & FF_FRAMEMASK, spr->mobj->player) == spr->mobj->sprite2)))))
				{
					nextFrame = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextFrame >= mod)
						nextFrame = 0;
					if (frame || !(spr->mobj->state->frame & FF_SPR2ENDSTATE))
						nextFrame = md2->model->spr2frames[spr2].frames[nextFrame];
					else
						nextFrame = -1;
				}
			}
			else if (HWR_CanInterpolateModel(spr->mobj, md2->model))
			{
				// frames are handled differently for states with FF_ANIMATE, so get the next frame differently for the interpolation
				if (spr->mobj->frame & FF_ANIMATE)
				{
					nextFrame = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextFrame >= (INT32)(spr->mobj->state->var1 + (spr->mobj->state->frame & FF_FRAMEMASK)))
						nextFrame = (spr->mobj->state->frame & FF_FRAMEMASK) % mod;
				}
				else
				{
					if (spr->mobj->state->nextstate != S_NULL && states[spr->mobj->state->nextstate].sprite != SPR_NULL
					&& !(spr->mobj->player && (spr->mobj->state->nextstate == S_PLAY_WAIT) && spr->mobj->state == &states[S_PLAY_STND]))
						nextFrame = (states[spr->mobj->state->nextstate].frame & FF_FRAMEMASK) % mod;
				}
			}
		}
#undef INTERPOLERATION_LIMIT
#endif

		//Hurdler: it seems there is still a small problem with mobj angle
		p.x = FIXED_TO_FLOAT(spr->mobj->x);
		p.y = FIXED_TO_FLOAT(spr->mobj->y)+md2->offset;

		if (flip)
			p.z = FIXED_TO_FLOAT(spr->mobj->z + spr->mobj->height);
		else
			p.z = FIXED_TO_FLOAT(spr->mobj->z);

		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			sprdef = &((skin_t *)spr->mobj->skin)->sprites[spr->mobj->sprite2];
		else
			sprdef = &sprites[spr->mobj->sprite];

		sprframe = &sprdef->spriteframes[spr->mobj->frame & FF_FRAMEMASK];

		if (sprframe->rotate || papersprite)
		{
			fixed_t anglef = AngleFixed(spr->mobj->angle);

			if (spr->mobj->player)
				anglef = AngleFixed(spr->mobj->player->drawangle);

			p.angley = FIXED_TO_FLOAT(anglef);
		}
		else
		{
			const fixed_t anglef = AngleFixed((R_PointToAngle(spr->mobj->x, spr->mobj->y))-ANGLE_180);
			p.angley = FIXED_TO_FLOAT(anglef);
		}

		p.rollangle = 0.0f;
		p.rollflip = 1;
		p.rotaxis = 0;
		if (spr->mobj->rollangle)
		{
			fixed_t anglef = AngleFixed(spr->mobj->rollangle);
			p.rollangle = FIXED_TO_FLOAT(anglef);
			p.roll = true;

			// rotation pivot
			p.centerx = FIXED_TO_FLOAT(spr->mobj->radius/2);
			p.centery = FIXED_TO_FLOAT(spr->mobj->height/(flip ? -2 : 2));

			// rotation axis
			if (sprinfo->available)
				p.rotaxis = (UINT8)(sprinfo->pivot[(spr->mobj->frame & FF_FRAMEMASK)].rotaxis);

			// for NiGHTS specifically but should work everywhere else
			ang = R_PointToAngle (spr->mobj->x, spr->mobj->y) - (spr->mobj->player ? spr->mobj->player->drawangle : spr->mobj->angle);
			if ((sprframe->rotate & SRF_RIGHT) && (ang < ANGLE_180)) // See from right
				p.rollflip = 1;
			else if ((sprframe->rotate & SRF_LEFT) && (ang >= ANGLE_180)) // See from left
				p.rollflip = -1;

			if (flip)
				p.rollflip *= -1;
		}

		p.anglex = 0.0f;

#ifdef USE_FTRANSFORM_ANGLEZ
		// Slope rotation from Kart
		p.anglez = 0.0f;
		if (spr->mobj->standingslope)
		{
			fixed_t tempz = spr->mobj->standingslope->normal.z;
			fixed_t tempy = spr->mobj->standingslope->normal.y;
			fixed_t tempx = spr->mobj->standingslope->normal.x;
			fixed_t tempangle = AngleFixed(R_PointToAngle2(0, 0, FixedSqrt(FixedMul(tempy, tempy) + FixedMul(tempz, tempz)), tempx));
			p.anglez = FIXED_TO_FLOAT(tempangle);
			tempangle = -AngleFixed(R_PointToAngle2(0, 0, tempz, tempy));
			p.anglex = FIXED_TO_FLOAT(tempangle);
		}
#endif

		// SRB2CBTODO: MD2 scaling support
		finalscale *= FIXED_TO_FLOAT(spr->mobj->scale);

		p.flip = atransform.flip;
#ifdef USE_FTRANSFORM_MIRROR
		p.mirror = atransform.mirror; // from Kart
#endif

		HWD.pfnSetShader(4);	// model shader
		HWD.pfnDrawModel(md2->model, frame, durs, tics, nextFrame, &p, finalscale, flip, &Surf);
	}

	return true;
}

#endif //HWRENDER
