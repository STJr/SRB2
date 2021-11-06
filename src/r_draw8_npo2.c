// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw8_npo2.c
/// \brief 8bpp span drawer functions (for non-powers-of-two flat dimensions)
/// \note  no includes because this is included as part of r_draw.c

// ==========================================================================
// SPANS
// ==========================================================================

#define SPANSIZE 16
#define INVSPAN 0.0625f

/**	\brief The R_DrawSpan_NPO2_8 function
	Draws the actual span.
*/
void R_DrawSpan_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	if (dest+8 > deststop)
		return;

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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

		*dest++ = colormap[source[((y * ds->flatwidth) + x)]];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedSpan_NPO2_8 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_NPO2_8(spancontext_t *ds)
{
	// x1, x2 = ds->x1, ds->x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds->flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds->flatheight);

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);

	CALC_TILTED_LIGHTING

	uz = ds->sup->z + ds->sup->y*(centery-ds->y) + ds->sup->x*(ds->x1-centerx);
	vz = ds->svp->z + ds->svp->y*(centery-ds->y) + ds->svp->x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = ds->source;
	//colormap = ds->colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

			*dest = colormap[source[((y * ds->flatwidth) + x)]];
		}
		dest++;
		iz += ds->szp->x;
		uz += ds->sup->x;
		vz += ds->svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp->x * SPANSIZE;
	uzstep = ds->sup->x * SPANSIZE;
	vzstep = ds->svp->x * SPANSIZE;
	//x1 = 0;
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = colormap[source[((y * ds->flatwidth) + x)]];
			}
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = colormap[source[((y * ds->flatwidth) + x)]];
			}
		}
		else
		{
			double left = width;
			iz += ds->szp->x * left;
			uz += ds->sup->x * left;
			vz += ds->svp->x * left;

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
				colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

					*dest = colormap[source[((y * ds->flatwidth) + x)]];
				}
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedTranslucentSpan_NPO2_8 function
	Like DrawTiltedSpan_NPO2, but translucent
*/
void R_DrawTiltedTranslucentSpan_NPO2_8(spancontext_t *ds)
{
	// x1, x2 = ds->x1, ds->x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds->flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds->flatheight);

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);

	CALC_TILTED_LIGHTING

	uz = ds->sup->z + ds->sup->y*(centery-ds->y) + ds->sup->x*(ds->x1-centerx);
	vz = ds->svp->z + ds->svp->y*(centery-ds->y) + ds->svp->x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = ds->source;
	//colormap = ds->colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

			*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dest);
		}
		dest++;
		iz += ds->szp->x;
		uz += ds->sup->x;
		vz += ds->svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp->x * SPANSIZE;
	uzstep = ds->sup->x * SPANSIZE;
	vzstep = ds->svp->x * SPANSIZE;
	//x1 = 0;
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dest);
			}
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dest);
			}
		}
		else
		{
			double left = width;
			iz += ds->szp->x * left;
			uz += ds->sup->x * left;
			vz += ds->svp->x * left;

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
				colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

					*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dest);
				}
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_NPO2_8(spancontext_t *ds)
{
	// x1, x2 = ds->x1, ds->x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	UINT8 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds->flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds->flatheight);

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);

	CALC_TILTED_LIGHTING

	uz = ds->sup->z + ds->sup->y*(centery-ds->y) + ds->sup->x*(ds->x1-centerx);
	vz = ds->svp->z + ds->svp->y*(centery-ds->y) + ds->svp->x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = ds->source;
	//colormap = ds->colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

			val = source[((y * ds->flatwidth) + x)];
		}

		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];

		dest++;
		iz += ds->szp->x;
		uz += ds->sup->x;
		vz += ds->svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp->x * SPANSIZE;
	uzstep = ds->sup->x * SPANSIZE;
	vzstep = ds->svp->x * SPANSIZE;
	//x1 = 0;
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				val = source[((y * ds->flatwidth) + x)];
			}
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				val = source[((y * ds->flatwidth) + x)];
			}
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
		}
		else
		{
			double left = width;
			iz += ds->szp->x * left;
			uz += ds->sup->x * left;
			vz += ds->svp->x * left;

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
				colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

					val = source[((y * ds->flatwidth) + x)];
				}
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawSplat_NPO2_8 function
	Just like R_DrawSpan_NPO2_8, but skips transparent pixels.
*/
void R_DrawSplat_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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
		val = source[((y * ds->flatwidth) + x)];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentSplat_NPO2_8 function
	Just like R_DrawSplat_NPO2_8, but is translucent!
*/
void R_DrawTranslucentSplat_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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
		val = source[((y * ds->flatwidth) + x)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds->transmap + (colormap[val] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFloorSprite_NPO2_8 function
	Just like R_DrawSplat_NPO2_8, but for floor sprites.
*/
void R_DrawFloorSprite_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT16 *source;
	UINT8 *translation;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	source = (UINT16 *)ds->source;
	colormap = ds->colormap;
	translation = ds->translation;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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
		val = source[((y * ds->flatwidth) + x)];
		if (val & 0xFF00)
			*dest = colormap[translation[val & 0xFF]];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentFloorSprite_NPO2_8 function
	Just like R_DrawFloorSprite_NPO2_8, but is translucent!
*/
void R_DrawTranslucentFloorSprite_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT16 *source;
	UINT8 *translation;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	source = (UINT16 *)ds->source;
	colormap = ds->colormap;
	translation = ds->translation;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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
		val = source[((y * ds->flatwidth) + x)];
		if (val & 0xFF00)
			*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedFloorSprite_NPO2_8 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_NPO2_8(spancontext_t *ds)
{
	// x1, x2 = ds->x1, ds->x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);
	uz = ds->sup->z + ds->sup->y*(centery-ds->y) + ds->sup->x*(ds->x1-centerx);
	vz = ds->svp->z + ds->svp->y*(centery-ds->y) + ds->svp->x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = (UINT16 *)ds->source;
	colormap = ds->colormap;
	translation = ds->translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp->x * SPANSIZE;
	uzstep = ds->sup->x * SPANSIZE;
	vzstep = ds->svp->x * SPANSIZE;
	//x1 = 0;
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
			// Lactozilla: Non-powers-of-two
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds->flatwidth - ((UINT32)(ds->flatwidth - x) % ds->flatwidth);
			if (y < 0)
				y = ds->flatheight - ((UINT32)(ds->flatheight - y) % ds->flatheight);

			x %= ds->flatwidth;
			y %= ds->flatheight;

			val = source[((y * ds->flatwidth) + x)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
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
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds->flatwidth - ((UINT32)(ds->flatwidth - x) % ds->flatwidth);
				if (y < 0)
					y = ds->flatheight - ((UINT32)(ds->flatheight - y) % ds->flatheight);

				x %= ds->flatwidth;
				y %= ds->flatheight;

				val = source[((y * ds->flatwidth) + x)];
				if (val & 0xFF00)
					*dest = colormap[translation[val & 0xFF]];
			}
		}
		else
		{
			double left = width;
			iz += ds->szp->x * left;
			uz += ds->sup->x * left;
			vz += ds->svp->x * left;

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
				// Lactozilla: Non-powers-of-two
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds->flatwidth - ((UINT32)(ds->flatwidth - x) % ds->flatwidth);
				if (y < 0)
					y = ds->flatheight - ((UINT32)(ds->flatheight - y) % ds->flatheight);

				x %= ds->flatwidth;
				y %= ds->flatheight;

				val = source[((y * ds->flatwidth) + x)];
				if (val & 0xFF00)
					*dest = colormap[translation[val & 0xFF]];
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTiltedTranslucentFloorSprite_NPO2_8 function
	Draws a tilted, translucent, floor sprite.
*/
void R_DrawTiltedTranslucentFloorSprite_NPO2_8(spancontext_t *ds)
{
	// x1, x2 = ds->x1, ds->x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);
	uz = ds->sup->z + ds->sup->y*(centery-ds->y) + ds->sup->x*(ds->x1-centerx);
	vz = ds->svp->z + ds->svp->y*(centery-ds->y) + ds->svp->x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = (UINT16 *)ds->source;
	colormap = ds->colormap;
	translation = ds->translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp->x * SPANSIZE;
	uzstep = ds->sup->x * SPANSIZE;
	vzstep = ds->svp->x * SPANSIZE;
	//x1 = 0;
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
			// Lactozilla: Non-powers-of-two
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds->flatwidth - ((UINT32)(ds->flatwidth - x) % ds->flatwidth);
			if (y < 0)
				y = ds->flatheight - ((UINT32)(ds->flatheight - y) % ds->flatheight);

			x %= ds->flatwidth;
			y %= ds->flatheight;

			val = source[((y * ds->flatwidth) + x)];
			if (val & 0xFF00)
				*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
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
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds->flatwidth - ((UINT32)(ds->flatwidth - x) % ds->flatwidth);
				if (y < 0)
					y = ds->flatheight - ((UINT32)(ds->flatheight - y) % ds->flatheight);

				x %= ds->flatwidth;
				y %= ds->flatheight;

				val = source[((y * ds->flatwidth) + x)];
				if (val & 0xFF00)
					*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
			}
		}
		else
		{
			double left = width;
			iz += ds->szp->x * left;
			uz += ds->sup->x * left;
			vz += ds->svp->x * left;

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
				// Lactozilla: Non-powers-of-two
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds->flatwidth - ((UINT32)(ds->flatwidth - x) % ds->flatwidth);
				if (y < 0)
					y = ds->flatheight - ((UINT32)(ds->flatheight - y) % ds->flatheight);

				x %= ds->flatwidth;
				y %= ds->flatheight;

				val = source[((y * ds->flatwidth) + x)];
				if (val & 0xFF00)
					*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTranslucentSpan_NPO2_8 function
	Draws the actual span with translucency.
*/
void R_DrawTranslucentSpan_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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
		val = ((y * ds->flatwidth) + x);
		*dest = *(ds->transmap + (colormap[source[val]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTranslucentWaterSpan_NPO2_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);

	xposition = ds->xfrac; yposition = (ds->yfrac + ds->waterofs);
	xstep = ds->xstep; ystep = ds->ystep;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];
	dsrc = screens[1] + (ds->y+ds->bgofs)*vid.width + ds->x1;

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

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
		*dest++ = colormap[*(ds->transmap + (source[((y * ds->flatwidth) + x)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedTranslucentWaterSpan_NPO2_8 function
	Like DrawTiltedTranslucentSpan_NPO2, but for water
*/
void R_DrawTiltedTranslucentWaterSpan_NPO2_8(spancontext_t *ds)
{
	// x1, x2 = ds->x1, ds->x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds->flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds->flatheight);

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);

	CALC_TILTED_LIGHTING

	uz = ds->sup->z + ds->sup->y*(centery-ds->y) + ds->sup->x*(ds->x1-centerx);
	vz = ds->svp->z + ds->svp->y*(centery-ds->y) + ds->svp->x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	dsrc = screens[1] + (ds->y+ds->bgofs)*vid.width + ds->x1;
	source = ds->source;
	//colormap = ds->colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

			*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dsrc++);
		}
		dest++;
		iz += ds->szp->x;
		uz += ds->sup->x;
		vz += ds->svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp->x * SPANSIZE;
	uzstep = ds->sup->x * SPANSIZE;
	vzstep = ds->svp->x * SPANSIZE;
	//x1 = 0;
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dsrc++);
			}
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
			colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dsrc++);
			}
		}
		else
		{
			double left = width;
			iz += ds->szp->x * left;
			uz += ds->sup->x * left;
			vz += ds->svp->x * left;

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
				colormap = ds->zlight[ds->tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

					*dest = *(ds->transmap + (colormap[source[((y * ds->flatwidth) + x)]] << 8) + *dsrc++);
				}
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}
