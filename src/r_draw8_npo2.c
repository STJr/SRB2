// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
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

#if defined(__GNUC__) || defined(__clang__) // Suppress intentional libdivide compiler warnings - Also added to libdivide.h
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waggregate-return"
#endif

/**	\brief The R_DrawSpan_NPO2_8 function
	Draws the actual span.
*/
void R_DrawSpan_NPO2_8 (void)
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

	size_t count = (ds_x2 - ds_x1 + 1);

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	if (dest+8 > deststop)
		return;

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

		*dest++ = colormap[source[((y * ds_flatwidth) + x)]];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedSpan_NPO2_8 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_NPO2_8(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
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

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			*dest = colormap[source[((y * ds_flatwidth) + x)]];
		}
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				*dest = colormap[source[((y * ds_flatwidth) + x)]];
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				*dest = colormap[source[((y * ds_flatwidth) + x)]];
			}
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

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
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

					*dest = colormap[source[((y * ds_flatwidth) + x)]];
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
void R_DrawTiltedTranslucentSpan_NPO2_8(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
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

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dest);
		}
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dest);
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dest);
			}
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

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
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

					*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dest);
				}
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_NPO2_8(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
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

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			val = source[((y * ds_flatwidth) + x)];
		}

		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];

		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
			}
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

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
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

					val = source[((y * ds_flatwidth) + x)];
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
void R_DrawSplat_NPO2_8 (void)
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

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

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
		val = source[((y * ds_flatwidth) + x)];
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
void R_DrawTranslucentSplat_NPO2_8 (void)
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

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

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
		val = source[((y * ds_flatwidth) + x)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFloorSprite_NPO2_8 function
	Just like R_DrawSplat_NPO2_8, but for floor sprites.
*/
void R_DrawFloorSprite_NPO2_8 (void)
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

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;
	dest = &topleft[ds_y*vid.width + ds_x1];

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
		val = source[((y * ds_flatwidth) + x)];
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
void R_DrawTranslucentFloorSprite_NPO2_8 (void)
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

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;
	dest = &topleft[ds_y*vid.width + ds_x1];

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
		val = source[((y * ds_flatwidth) + x)];
		if (val & 0xFF00)
			*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedFloorSprite_NPO2_8 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_NPO2_8(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
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

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
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
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			val = source[((y * ds_flatwidth) + x)];
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
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
				if (val & 0xFF00)
					*dest = colormap[translation[val & 0xFF]];
			}
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

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
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
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
void R_DrawTiltedTranslucentFloorSprite_NPO2_8(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
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

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
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
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			val = source[((y * ds_flatwidth) + x)];
			if (val & 0xFF00)
				*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
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
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
				if (val & 0xFF00)
					*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
			}
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

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
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
				if (val & 0xFF00)
					*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
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
void R_DrawTranslucentSpan_NPO2_8 (void)
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

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

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
		val = ((y * ds_flatwidth) + x);
		*dest = *(ds_transmap + (colormap[source[val]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawWaterSpan_NPO2_8(void)
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

	size_t count = (ds_x2 - ds_x1 + 1);

	xposition = ds_xfrac; yposition = (ds_yfrac + ds_waterofs);
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

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
		*dest++ = colormap[*(ds_transmap + (source[((y * ds_flatwidth) + x)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedWaterSpan_NPO2_8 function
	Like DrawTiltedTranslucentSpan_NPO2, but for water
*/
void R_DrawTiltedWaterSpan_NPO2_8(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
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

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dsrc++);
		}
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dsrc++);
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
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

				*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dsrc++);
			}
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

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
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

					*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dsrc++);
				}
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

#if defined(__GNUC__) || defined(__clang__) // Stop suppressing intentional libdivide compiler warnings
    #pragma GCC diagnostic pop
#endif
