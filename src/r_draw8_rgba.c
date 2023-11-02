// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw8_rgba.c
/// \brief 8bpp span/column drawer functions for RGBA sources

// ==========================================================================
// COLUMNS
// ==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.
//

#include "v_video.h"

void R_DrawColumn_8_RGBA(void)
{
	INT32 count = dc_yh - dc_yl;
	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	UINT8 *dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fixed_t fracstep = dc_iscale;
	fixed_t frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const RGBA_t *source = (RGBA_t *)dc_source;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight-1;

		RGBA_t ocolor, color;
		UINT8 idx;

		#define GET_COLOR(f) \
			ocolor = source[f]; \
			idx = colormap[GetColorLUT(&r_colorlookup, ocolor.s.red, ocolor.s.green, ocolor.s.blue)]; \
			color = pMasterPalette[idx]

		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				GET_COLOR(frac>>FRACBITS);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;
				frac += fracstep;
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
			}
		}

		#undef GET_COLOR
	}
}

static UINT32 BlendPixel(RGBA_t background, RGBA_t foreground)
{
	UINT8 beta = 0xFF - foreground.s.alpha;
	RGBA_t output;
	output.s.red = ((background.s.red * beta) + (foreground.s.red * foreground.s.alpha)) / 0xFF;
	output.s.green = ((background.s.green * beta) + (foreground.s.green * foreground.s.alpha)) / 0xFF;
	output.s.blue = ((background.s.blue * beta) + (foreground.s.blue * foreground.s.alpha)) / 0xFF;
	return output.rgba;
}

void R_DrawBlendedColumn_8_RGBA(void)
{
	INT32 count = dc_yh - dc_yl;
	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	UINT8 *dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fixed_t fracstep = dc_iscale;
	fixed_t frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const RGBA_t *source = (RGBA_t *)dc_source;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight-1;

		RGBA_t ocolor, color;
		UINT8 idx;

		#define GET_COLOR(f) \
			ocolor = source[f]; \
			idx = colormap[GetColorLUT(&r_colorlookup, ocolor.s.red, ocolor.s.green, ocolor.s.blue)]; \
			color = pMasterPalette[idx]; \
			color.s.alpha = ocolor.s.alpha; \
			color.rgba = BlendPixel(pMasterPalette[*dest], color)

		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				GET_COLOR(frac>>FRACBITS);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;
				frac += fracstep;
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			}
		}

		#undef GET_COLOR
	}
}

void R_DrawTranslucentColumn_8_RGBA(void)
{
	INT32 count = dc_yh - dc_yl;
	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	UINT8 *dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fixed_t fracstep = dc_iscale;
	fixed_t frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const RGBA_t *source = (RGBA_t *)dc_source;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight-1;

		RGBA_t ocolor, color;
		UINT8 idx;

		#define GET_COLOR(f) \
			ocolor = source[f]; \
			idx = colormap[GetColorLUT(&r_colorlookup, ocolor.s.red, ocolor.s.green, ocolor.s.blue)]; \
			color = pMasterPalette[idx]; \
			color.s.alpha = ocolor.s.alpha; \
			color.rgba = ASTBlendPixel(pMasterPalette[*dest], color, dc_blendmode, dc_opacity)

		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				GET_COLOR(frac >> FRACBITS);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;
				frac += fracstep;
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				GET_COLOR((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			}
		}

		#undef GET_COLOR
	}
}

// ==========================================================================
// SPANS
// ==========================================================================

void R_DrawFloorSprite_8_RGBA(void)
{
	// TODO
}

void R_DrawTranslucentFloorSprite_8_RGBA(void)
{
	// TODO
}

void R_DrawTiltedFloorSprite_8_RGBA(void)
{
	// TODO
}

void R_DrawTiltedTranslucentFloorSprite_8_RGBA(void)
{
	// TODO
}

void R_DrawFloorSprite_NPO2_8_RGBA(void)
{
	// TODO
}

void R_DrawTranslucentFloorSprite_NPO2_8_RGBA(void)
{
	// TODO
}

void R_DrawTiltedFloorSprite_NPO2_8_RGBA(void)
{
	// TODO
}

void R_DrawTiltedTranslucentFloorSprite_NPO2_8_RGBA(void)
{
	// TODO
}
