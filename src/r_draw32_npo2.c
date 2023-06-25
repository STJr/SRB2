// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw32_npo2.c
/// \brief 32bpp span drawer functions (for non-powers-of-two flat dimensions)
/// \note  no includes because this is included as part of r_draw.c

// ==========================================================================
// SPANS
// ==========================================================================

/**	\brief The R_DrawSpan_NPO2_32 function
	Draws the actual span.
*/
void R_DrawSpan_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

				*dest++ = GetTrueColor(colormap[source[((y * ds_flatwidth) + x)]]);
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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

				*dest++ = TC_Colormap32Mix(colormapu32[source[((y * ds_flatwidth) + x)]]);
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

			*dest = TC_ColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

/**	\brief The R_DrawTiltedSpan_NPO2_32 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_NPO2_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	CALC_SLOPE_LIGHT

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);
	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

						*dest = GetTrueColor(colormap[source[((y * ds_flatwidth) + x)]]);
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

						*dest = GetTrueColor(colormap[source[((y * ds_flatwidth) + x)]]);
					}
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

							*dest = GetTrueColor(colormap[source[((y * ds_flatwidth) + x)]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						*dest = TC_Colormap32Mix(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						*dest = TC_Colormap32Mix(colormapu32[source[((y * ds_flatwidth) + x)]]);
					}
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							*dest = TC_Colormap32Mix(colormapu32[source[((y * ds_flatwidth) + x)]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = TC_ColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest);
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = TC_ColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest);
				}
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
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						*dest = TC_ColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest);
					}
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
}

/**	\brief The R_DrawTiltedTranslucentSpan_NPO2_32 function
	Like DrawTiltedSpan, but translucent
*/
void R_DrawTiltedTranslucentSpan_NPO2_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	CALC_SLOPE_LIGHT

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);
	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

						WriteTranslucentSpan_s8d32(colormap[source[((y * ds_flatwidth) + x)]]);
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

						WriteTranslucentSpan_s8d32(colormap[source[((y * ds_flatwidth) + x)]]);
					}
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

							WriteTranslucentSpan_s8d32(colormap[source[((y * ds_flatwidth) + x)]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						WriteTranslucentSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						WriteTranslucentSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
					}
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							WriteTranslucentSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dest, ds_alpha);
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dest, ds_alpha);
				}
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
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dest, ds_alpha);
					}
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
}

/**	\brief The R_DrawTiltedWaterSpan_NPO2_32 function
	Like DrawTiltedTranslucentSpan, but for water
*/
void R_DrawTiltedWaterSpan_NPO2_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;
	UINT32 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	CALC_SLOPE_LIGHT

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);
	dsrc = ((UINT32 *)screens[1]) + (ds_y+ds_bgofs)*vid.width + ds_x1;
	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

						WriteTranslucentWaterSpan_s8d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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

						WriteTranslucentWaterSpan_s8d32(colormap[source[((y * ds_flatwidth) + x)]]);
					}
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

							WriteTranslucentWaterSpan_s8d32(colormap[source[((y * ds_flatwidth) + x)]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						WriteTranslucentWaterSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						WriteTranslucentWaterSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
					}
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							WriteTranslucentWaterSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
				}
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
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
					}
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
}

void R_DrawTiltedSplat_NPO2_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;

	UINT8 val;
	UINT32 valu32;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	CALC_SLOPE_LIGHT

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);
	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

						val = colormap[source[((y * ds_flatwidth) + x)]];
					}
					if (val != TRANSPARENTPIXEL)
						*dest = GetTrueColor(colormap[val]);
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

						val = colormap[source[((y * ds_flatwidth) + x)]];
					}
					if (val != TRANSPARENTPIXEL)
						*dest = GetTrueColor(colormap[val]);
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

							val = colormap[source[((y * ds_flatwidth) + x)]];
						}
						if (val != TRANSPARENTPIXEL)
							*dest = GetTrueColor(colormap[val]);
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						val = colormap[source[((y * ds_flatwidth) + x)]];
					}
					if (val != TRANSPARENTPIXEL)
						*dest = TC_Colormap32Mix(colormapu32[val]);
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
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						val = colormap[source[((y * ds_flatwidth) + x)]];
					}
					if (val != TRANSPARENTPIXEL)
						*dest = TC_Colormap32Mix(colormapu32[val]);
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							val = colormap[source[((y * ds_flatwidth) + x)]];
						}
						if (val != TRANSPARENTPIXEL)
							*dest = TC_Colormap32Mix(colormapu32[val]);
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					valu32 = sourceu32[((y * ds_flatwidth) + x)];
				}
				if (R_GetRgbaA(valu32))
					*dest = TC_ColorMix(valu32, *dest);
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
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					valu32 = sourceu32[((y * ds_flatwidth) + x)];
				}
				if (R_GetRgbaA(valu32))
					*dest = TC_ColorMix(valu32, *dest);
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
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						valu32 = sourceu32[((y * ds_flatwidth) + x)];
					}
					if (R_GetRgbaA(valu32))
						*dest = TC_ColorMix(valu32, *dest);
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
}

/**	\brief The R_DrawSplat_NPO2_32 function
	Just like R_DrawSpan_NPO2_32, but skips transparent pixels.
*/
void R_DrawSplat_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
					*dest = GetTrueColor(colormap[val]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					*dest = TC_Colormap32Mix(colormapu32[val]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

			val = sourceu32[((y * ds_flatwidth) + x)];
			if (R_GetRgbaA(val))
				*dest = TC_ColorMix(val, *dest);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

/**	\brief The R_DrawTranslucentSplat_NPO2_32 function
	Just like R_DrawSplat_NPO2_32, but is translucent!
*/
void R_DrawTranslucentSplat_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
					WriteTranslucentSpan_s8d32(colormap[val]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					WriteTranslucentSpan_s32d32(colormapu32[val]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

			val = sourceu32[((y * ds_flatwidth) + x)];
			if (R_GetRgbaA(val))
				*dest = R_BlendModeMix(val, *dest, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

/**	\brief The R_DrawFloorSprite_NPO2_8 function
	Just like R_DrawSplat_NPO2_32, but for floor sprites.
*/
void R_DrawFloorSprite_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT16 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT8 *translation = ds_translation;

	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = (UINT16 *)ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

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

	if (ds_picfmt == PICFMT_FLAT16)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
					*dest = GetTrueColor(colormap[translation[val & 0xFF]]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					*dest = TC_Colormap32Mix(colormapu32[translation[val & 0xFF]]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

			val = sourceu32[((y * ds_flatwidth) + x)];
			if (R_GetRgbaA(val))
				*dest = TC_ColorMix(val, *dest);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

/**	\brief The R_DrawTranslucentFloorSprite_NPO2_32 function
	Just like R_DrawFloorSprite_NPO2_32, but is translucent!
*/
void R_DrawTranslucentFloorSprite_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT16 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT8 *translation = ds_translation;

	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = (UINT16 *)ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

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

	if (ds_picfmt == PICFMT_FLAT16)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
					WriteTranslucentSpan_s8d32(colormap[translation[val & 0xFF]]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
				{
					val = colormapu32[translation[val & 0xFF]];
					WriteTranslucentSpan_s32d32(val);
				}
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

			val = sourceu32[((y * ds_flatwidth) + x)];
			if (R_GetRgbaA(val))
				*dest = R_BlendModeMix(val, *dest, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

/**	\brief The R_DrawTiltedFloorSprite_NPO2_32 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_NPO2_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT8 *translation = ds_translation;

	UINT32 *dest;
	UINT32 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	source = (UINT16 *)ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	if (ds_picfmt == PICFMT_FLAT16)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
						*dest = GetTrueColor(colormap[translation[val & 0xFF]]);
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
							*dest = GetTrueColor(colormap[translation[val & 0xFF]]);
					}
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
							*dest = GetTrueColor(colormap[translation[val & 0xFF]]);
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
							*dest = TC_Colormap32Mix(colormapu32[translation[val & 0xFF]]);
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
							*dest = TC_Colormap32Mix(colormapu32[translation[val & 0xFF]]);
					}
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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
								*dest = TC_Colormap32Mix(colormapu32[translation[val & 0xFF]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

				val = sourceu32[((y * ds_flatwidth) + x)];
				if (R_GetRgbaA(val))
					*dest = TC_ColorMix(val, *dest);
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

					val = sourceu32[((y * ds_flatwidth) + x)];
					if (R_GetRgbaA(val))
						*dest = TC_ColorMix(val, *dest);
				}
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

					val = sourceu32[((y * ds_flatwidth) + x)];
					if (R_GetRgbaA(val))
						*dest = TC_ColorMix(val, *dest);
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
}

/**	\brief The R_DrawTiltedTranslucentFloorSprite_NPO2_32 function
	Draws a tilted, translucent, floor sprite.
*/
void R_DrawTiltedTranslucentFloorSprite_NPO2_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT8 *translation = ds_translation;

	UINT32 *dest;
	UINT32 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds_flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds_flatheight);

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);
	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	source = (UINT16 *)ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
	width++;

	if (ds_picfmt == PICFMT_FLAT16)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
						WriteTranslucentSpan_s8d32(colormap[translation[val & 0xFF]]);
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
							WriteTranslucentSpan_s8d32(colormap[translation[val & 0xFF]]);
					}
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
							WriteTranslucentSpan_s8d32(colormap[translation[val & 0xFF]]);
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
							WriteTranslucentSpan_s32d32(colormapu32[translation[val & 0xFF]]);
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
							WriteTranslucentSpan_s32d32(colormapu32[translation[val & 0xFF]]);
					}
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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
								WriteTranslucentSpan_s32d32(colormapu32[translation[val & 0xFF]]);
						}
						dest++;
						u += stepu;
						v += stepv;
					}
				}
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

				val = sourceu32[((y * ds_flatwidth) + x)];
				if (R_GetRgbaA(val))
					*dest = R_BlendModeMix(val, *dest, ds_alpha);
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

					val = sourceu32[((y * ds_flatwidth) + x)];
					if (R_GetRgbaA(val))
						*dest = R_BlendModeMix(val, *dest, ds_alpha);
				}
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

					val = sourceu32[((y * ds_flatwidth) + x)];
					if (R_GetRgbaA(val))
						*dest = R_BlendModeMix(val, *dest, ds_alpha);
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
}

/**	\brief The R_DrawTranslucentSpan_NPO2_32 function
	Draws the actual span with translucency.
*/
void R_DrawTranslucentSpan_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

				val = colormap[source[((y * ds_flatwidth) + x)]];
				WriteTranslucentSpan_s8d32(val);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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

				val = colormapu32[source[((y * ds_flatwidth) + x)]];
				WriteTranslucentSpan_s32d32(val);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

			val = sourceu32[((y * ds_flatwidth) + x)];
			*dest = R_BlendModeMix(val, *dest, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

void R_DrawWaterSpan_NPO2_32(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *source;
	UINT32 *sourceu32;

	UINT8 *colormap = NULL;
	UINT32 *colormapu32 = NULL;

	UINT32 *dest;
	UINT32 *dsrc;

	size_t count;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;

	if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		colormap = ds_colormap;
	else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		colormapu32 = (UINT32 *)ds_colormap;

	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);
	dsrc = ((UINT32 *)screens[1]) + (ds_y+ds_bgofs)*vid.width + ds_x1;
	count = ds_x2 - ds_x1 + 1;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
			while (count--)
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

				WriteTranslucentWaterSpan_s8d32(colormap[source[((y * ds_flatwidth) + x)]]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
			while (count--)
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

				WriteTranslucentWaterSpan_s32d32(colormapu32[source[((y * ds_flatwidth) + x)]]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
		while (count--)
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

			*dest = R_BlendModeMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}
