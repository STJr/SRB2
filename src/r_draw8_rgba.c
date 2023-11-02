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
INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep))*(!dc_hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const RGBA_t *source = (RGBA_t *)dc_source;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		RGBA_t color;
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
				color = source[frac>>FRACBITS];
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
				color = source[(frac>>FRACBITS) & heightmask];
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;
				frac += fracstep;
				color = source[(frac>>FRACBITS) & heightmask];
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				color = source[(frac>>FRACBITS) & heightmask];
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
			}
		}
	}
}

void R_DrawBlendedColumn_8_RGBA(void)
{
INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep))*(!dc_hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const RGBA_t *source = (RGBA_t *)dc_source;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		RGBA_t color;
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
				color.rgba = ASTBlendPixel(pMasterPalette[*dest], source[frac>>FRACBITS], AST_TRANSLUCENT, 255);
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
				color.rgba = ASTBlendPixel(pMasterPalette[*dest], source[frac>>FRACBITS], AST_TRANSLUCENT, 255);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;
				frac += fracstep;
				color.rgba = ASTBlendPixel(pMasterPalette[*dest], source[frac>>FRACBITS], AST_TRANSLUCENT, 255);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				color.rgba = ASTBlendPixel(pMasterPalette[*dest], source[frac>>FRACBITS], AST_TRANSLUCENT, 255);
				*dest = colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)];
			}
		}
	}
}

void R_DrawTranslucentColumn_8_RGBA(void)
{
register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep))*(!dc_hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const RGBA_t *source = (RGBA_t *)dc_source;
		register const UINT8 *transmap = dc_transmap;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight - 1;
		RGBA_t color;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				// using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				color = source[frac>>FRACBITS];
				*dest = *(transmap + (colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)]<<8) + (*dest));
				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				color = source[(frac>>FRACBITS) & heightmask];
				*dest = *(transmap + (colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				color = source[(frac>>FRACBITS) & heightmask];
				*dest = *(transmap + (colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				color = source[(frac>>FRACBITS) & heightmask];
				*dest = *(transmap + (colormap[GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue)]<<8) + (*dest));
			}
		}
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
