// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
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

/**	\brief The R_DrawSpan_NPO2_8 function
	Draws the actual span.
*/
void R_DrawSpan_NPO2_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	source = ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];

	if (dest+8 > deststop)
		return;

	while (count-- && dest <= deststop)
	{
		fixed_t x = (xposition >> FRACBITS);
		fixed_t y = (yposition >> FRACBITS);

		// Carefully align all of my Friends.
		if (x < 0)
			x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
		if (y < 0)
			y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

		x %= ds_flatwidth;
		y %= ds_flatheight;

		*dest++ = colormap[source[((y * ds_flatwidth) + x)]];
		xposition += xstep;
		yposition += ystep;
	}
}

#define PLANELIGHTFLOAT (BASEVIDWIDTH * BASEVIDWIDTH / vid.width / (zeroheight - FIXED_TO_FLOAT(viewz)) / 21.0f * FIXED_TO_FLOAT(fovtan))

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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z) + viewx;
		v = (INT64)(vz*z) + viewy;

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
			fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			*dest = colormap[source[((y * ds_flatwidth) + x)]];
		}
		dest++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
#define SPANSIZE 16
#define INVSPAN	0.0625f

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
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
		u = (INT64)(startu) + viewx;
		v = (INT64)(startv) + viewy;

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

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
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

				*dest = colormap[source[((y * ds_flatwidth) + x)]];
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z) + viewx;
		v = (INT64)(vz*z) + viewy;

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
			fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dest);
		}
		dest++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
#define SPANSIZE 16
#define INVSPAN	0.0625f

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
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
		u = (INT64)(startu) + viewx;
		v = (INT64)(startv) + viewy;

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

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
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

				*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dest);
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z) + viewx;
		v = (INT64)(vz*z) + viewy;

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
			fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			val = source[((y * ds_flatwidth) + x)];
		}

		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];

		dest++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
#define SPANSIZE 16
#define INVSPAN	0.0625f

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
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
		u = (INT64)(startu) + viewx;
		v = (INT64)(startv) + viewy;

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

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
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

				val = source[((y * ds_flatwidth) + x)];
			}
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

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
	dest = ylookup[ds_y] + columnofs[ds_x1];

	while (count-- && dest <= deststop)
	{
		fixed_t x = (xposition >> FRACBITS);
		fixed_t y = (yposition >> FRACBITS);

		// Carefully align all of my Friends.
		if (x < 0)
			x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
		if (y < 0)
			y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

		x %= ds_flatwidth;
		y %= ds_flatheight;

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
	dest = ylookup[ds_y] + columnofs[ds_x1];

	while (count-- && dest <= deststop)
	{
		fixed_t x = (xposition >> FRACBITS);
		fixed_t y = (yposition >> FRACBITS);

		// Carefully align all of my Friends.
		if (x < 0)
			x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
		if (y < 0)
			y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

		x %= ds_flatwidth;
		y %= ds_flatheight;

		val = source[((y * ds_flatwidth) + x)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
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
	dest = ylookup[ds_y] + columnofs[ds_x1];

	while (count-- && dest <= deststop)
	{
		fixed_t x = (xposition >> FRACBITS);
		fixed_t y = (yposition >> FRACBITS);

		// Carefully align all of my Friends.
		if (x < 0)
			x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
		if (y < 0)
			y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

		x %= ds_flatwidth;
		y %= ds_flatheight;

		val = ((y * ds_flatwidth) + x);
		*dest = *(ds_transmap + (colormap[source[val]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

#ifndef NOWATER
void R_DrawTranslucentWaterSpan_NPO2_8(void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

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
	dest = ylookup[ds_y] + columnofs[ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	while (count-- && dest <= deststop)
	{
		fixed_t x = (xposition >> FRACBITS);
		fixed_t y = (yposition >> FRACBITS);

		// Carefully align all of my Friends.
		if (x < 0)
			x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
		if (y < 0)
			y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

		x %= ds_flatwidth;
		y %= ds_flatheight;

		*dest++ = colormap[*(ds_transmap + (source[((y * ds_flatwidth) + x)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedTranslucentWaterSpan_NPO2_8 function
	Like DrawTiltedTranslucentSpan_NPO2, but for water
*/
void R_DrawTiltedTranslucentWaterSpan_NPO2_8(void)
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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);

	dest = ylookup[ds_y] + columnofs[ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z) + viewx;
		v = (INT64)(vz*z) + viewy;

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
			fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dsrc++);
		}
		dest++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
#define SPANSIZE 16
#define INVSPAN	0.0625f

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_szp->x * SPANSIZE;
	uzstep = ds_sup->x * SPANSIZE;
	vzstep = ds_svp->x * SPANSIZE;
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
		u = (INT64)(startu) + viewx;
		v = (INT64)(startv) + viewy;

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

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
				fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
				fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
				if (y < 0)
					y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

				x %= ds_flatwidth;
				y %= ds_flatheight;

				*dest = *(ds_transmap + (colormap[source[((y * ds_flatwidth) + x)]] << 8) + *dsrc++);
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

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
#endif // NOWATER
