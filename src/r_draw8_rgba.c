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

#include "v_video.h" // for GetColorLUT (should be moved to somewhere else)

#define GET_COLOR(f) \
	ocolor = source[f]; \
	idx = colormap[GetColorLUT(&r_colorlookup, ocolor.s.red, ocolor.s.green, ocolor.s.blue)]; \
	color = pMasterPalette[idx]; \
	color.s.alpha = ocolor.s.alpha; \
	color.rgba = BlendPixel(pMasterPalette[*dest], color)

#define GET_COLOR_BLENDED(f) \
	ocolor = source[f]; \
	idx = colormap[GetColorLUT(&r_colorlookup, ocolor.s.red, ocolor.s.green, ocolor.s.blue)]; \
	color = pMasterPalette[idx]; \
	color.s.alpha = ocolor.s.alpha; \
	color.rgba = ASTBlendPixel(pMasterPalette[*dest], color, dc_blendmode, dc_opacity)

static UINT32 BlendPixel(RGBA_t background, RGBA_t foreground)
{
	UINT8 beta = 0xFF - foreground.s.alpha;
	RGBA_t output;
	output.s.red = ((background.s.red * beta) + (foreground.s.red * foreground.s.alpha)) / 0xFF;
	output.s.green = ((background.s.green * beta) + (foreground.s.green * foreground.s.alpha)) / 0xFF;
	output.s.blue = ((background.s.blue * beta) + (foreground.s.blue * foreground.s.alpha)) / 0xFF;
	return output.rgba;
}

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
				GET_COLOR_BLENDED(frac >> FRACBITS);
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
				GET_COLOR_BLENDED((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;
				frac += fracstep;
				GET_COLOR_BLENDED((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				GET_COLOR_BLENDED((frac>>FRACBITS) & heightmask);
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			}
		}
	}
}

// ==========================================================================
// SPANS
// ==========================================================================

void R_DrawFloorSprite_8_RGBA(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	RGBA_t ocolor, color;
	UINT8 idx;

	xposition = ds_xfrac << nflatshiftup;
	yposition = ds_yfrac << nflatshiftup;
	xstep = ds_xstep << nflatshiftup;
	ystep = ds_ystep << nflatshiftup;

	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];

	while (count-- && dest <= deststop)
	{
		GET_COLOR((((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift));
		*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTranslucentFloorSprite_8_RGBA(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	RGBA_t ocolor, color;
	UINT8 idx;

	xposition = ds_xfrac << nflatshiftup;
	yposition = ds_yfrac << nflatshiftup;
	xstep = ds_xstep << nflatshiftup;
	ystep = ds_ystep << nflatshiftup;

	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];

	while (count-- && dest <= deststop)
	{
		GET_COLOR_BLENDED((((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift));
		*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTiltedFloorSprite_8_RGBA(void)
{
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	RGBA_t ocolor, color;
	UINT8 idx;

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			GET_COLOR(((v >> nflatyshift) & nflatmask) | (u >> nflatxshift));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			GET_COLOR(((v >> nflatyshift) & nflatmask) | (u >> nflatxshift));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		}
		else
		{
			double left = width;
			iz += ds_szp->x * left;
			uz += ds_sup->x * left;
			vz += ds_svp->x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				GET_COLOR(((v >> nflatyshift) & nflatmask) | (u >> nflatxshift));
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

void R_DrawTiltedTranslucentFloorSprite_8_RGBA(void)
{
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	RGBA_t ocolor, color;
	UINT8 idx;

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			GET_COLOR_BLENDED(((v >> nflatyshift) & nflatmask) | (u >> nflatxshift));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			GET_COLOR_BLENDED(((v >> nflatyshift) & nflatmask) | (u >> nflatxshift));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		}
		else
		{
			double left = width;
			iz += ds_szp->x * left;
			uz += ds_sup->x * left;
			vz += ds_svp->x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				GET_COLOR_BLENDED(((v >> nflatyshift) & nflatmask) | (u >> nflatxshift));
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

#if defined(__GNUC__) || defined(__clang__) // Suppress intentional libdivide compiler warnings - Also added to libdivide.h
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waggregate-return"
#endif

void R_DrawFloorSprite_NPO2_8_RGBA(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	RGBA_t ocolor, color;
	UINT8 idx;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];

	fixedwidth = ds_flatwidth << FRACBITS;
	fixedheight = ds_flatheight << FRACBITS;

	// Fix xposition and yposition if they are out of bounds.
	if (xposition < 0)
		xposition = fixedwidth - ((UINT32)(fixedwidth - xposition) % fixedwidth);
	else if (xposition >= fixedwidth)
		xposition %= fixedwidth;
	if (yposition < 0)
		yposition = fixedheight - ((UINT32)(fixedheight - yposition) % fixedheight);
	else if (yposition >= fixedheight)
		yposition %= fixedheight;

	while (count-- && dest <= deststop)
	{
		// The loops here keep the texture coordinates within the texture.
		// They will rarely iterate multiple times, and are cheaper than a modulo operation,
		// even if using libdivide.
		if (xstep < 0) // These if statements are hopefully hoisted by the compiler to above this loop
			while (xposition < 0)
				xposition += fixedwidth;
		else
			while (xposition >= fixedwidth)
				xposition -= fixedwidth;
		if (ystep < 0)
			while (yposition < 0)
				yposition += fixedheight;
		else
			while (yposition >= fixedheight)
				yposition -= fixedheight;

		x = (xposition >> FRACBITS);
		y = (yposition >> FRACBITS);
		GET_COLOR(((y * ds_flatwidth) + x));
		*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTranslucentFloorSprite_NPO2_8_RGBA(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	RGBA_t ocolor, color;
	UINT8 idx;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];

	fixedwidth = ds_flatwidth << FRACBITS;
	fixedheight = ds_flatheight << FRACBITS;

	// Fix xposition and yposition if they are out of bounds.
	if (xposition < 0)
		xposition = fixedwidth - ((UINT32)(fixedwidth - xposition) % fixedwidth);
	else if (xposition >= fixedwidth)
		xposition %= fixedwidth;
	if (yposition < 0)
		yposition = fixedheight - ((UINT32)(fixedheight - yposition) % fixedheight);
	else if (yposition >= fixedheight)
		yposition %= fixedheight;

	while (count-- && dest <= deststop)
	{
		// The loops here keep the texture coordinates within the texture.
		// They will rarely iterate multiple times, and are cheaper than a modulo operation,
		// even if using libdivide.
		if (xstep < 0) // These if statements are hopefully hoisted by the compiler to above this loop
			while (xposition < 0)
				xposition += fixedwidth;
		else
			while (xposition >= fixedwidth)
				xposition -= fixedwidth;
		if (ystep < 0)
			while (yposition < 0)
				yposition += fixedheight;
		else
			while (yposition >= fixedheight)
				yposition -= fixedheight;

		x = (xposition >> FRACBITS);
		y = (yposition >> FRACBITS);
		GET_COLOR_BLENDED(((y * ds_flatwidth) + x));
		*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTiltedFloorSprite_NPO2_8_RGBA(void)
{
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	RGBA_t ocolor, color;
	UINT8 idx;

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			GET_COLOR(((y * ds_flatwidth) + x));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);

			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			GET_COLOR(((y * ds_flatwidth) + x));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		}
		else
		{
			double left = width;
			iz += ds_szp->x * left;
			uz += ds_sup->x * left;
			vz += ds_svp->x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				GET_COLOR(((y * ds_flatwidth) + x));
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

void R_DrawTiltedTranslucentFloorSprite_NPO2_8_RGBA(void)
{
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	RGBA_t *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	RGBA_t ocolor, color;
	UINT8 idx;

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = (RGBA_t *)ds_source;
	colormap = ds_colormap;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			GET_COLOR(((y * ds_flatwidth) + x));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);

			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			GET_COLOR_BLENDED(((y * ds_flatwidth) + x));
			*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
		}
		else
		{
			double left = width;
			iz += ds_szp->x * left;
			uz += ds_sup->x * left;
			vz += ds_svp->x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				GET_COLOR_BLENDED(((y * ds_flatwidth) + x));
				*dest = GetColorLUT(&r_colorlookup, color.s.red, color.s.green, color.s.blue);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

#if defined(__GNUC__) || defined(__clang__) // Stop suppressing intentional libdivide compiler warnings
    #pragma GCC diagnostic pop
#endif
