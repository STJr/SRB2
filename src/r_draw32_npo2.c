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
void R_DrawSpan_NPO2_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

				*dest++ = GetTrueColor(colormap[source[((y * ds_flatwidth) + x)]]);
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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

				*dest++ = colormapu32[source[((y * ds_flatwidth) + x)]];
				xposition += xstep;
				yposition += ystep;
			}
		}
	}
	else if (ds_picfmt == PICFMT_FLAT32)
	{
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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = BASEVIDWIDTH*BASEVIDWIDTH/vid.width / (zeroheight - FIXED_TO_FLOAT(viewz)) / 21.0f;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;
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
						fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
						fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

						// Carefully align all of my Friends.
						if (x < 0)
							x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
						if (y < 0)
							y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

						x %= ds_flatwidth;
						y %= ds_flatheight;

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
				u = (INT64)(startu) + viewx;
				v = (INT64)(startv) + viewy;

				for (i = SPANSIZE-1; i >= 0; i--)
				{
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						*dest = colormapu32[source[((y * ds_flatwidth) + x)]];
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
						fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
						fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

						// Carefully align all of my Friends.
						if (x < 0)
							x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
						if (y < 0)
							y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

						x %= ds_flatwidth;
						y %= ds_flatheight;

						*dest = colormapu32[source[((y * ds_flatwidth) + x)]];
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							*dest = colormapu32[source[((y * ds_flatwidth) + x)]];
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (i = SPANSIZE-1; i >= 0; i--)
			{
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

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
				u = (INT64)(startu) + viewx;
				v = (INT64)(startv) + viewy;

				for (; width != 0; width--)
				{
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						*dest = TC_ColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest);
					}
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
#endif
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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = BASEVIDWIDTH*BASEVIDWIDTH/vid.width / (zeroheight - FIXED_TO_FLOAT(viewz)) / 21.0f;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;
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
		*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
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

						WriteTranslucentSpan(colormap[source[((y * ds_flatwidth) + x)]]);
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

						WriteTranslucentSpan(colormap[source[((y * ds_flatwidth) + x)]]);
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

							WriteTranslucentSpan(colormap[source[((y * ds_flatwidth) + x)]]);
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
				u = (INT64)(startu) + viewx;
				v = (INT64)(startv) + viewy;

				for (i = SPANSIZE-1; i >= 0; i--)
				{
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						WriteTranslucentSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
						fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
						fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

						// Carefully align all of my Friends.
						if (x < 0)
							x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
						if (y < 0)
							y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

						x %= ds_flatwidth;
						y %= ds_flatheight;

						WriteTranslucentSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							WriteTranslucentSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (i = SPANSIZE-1; i >= 0; i--)
			{
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest, ds_alpha);
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
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

					*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest, ds_alpha);
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
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dest, ds_alpha);
					}
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
#endif
}

#ifndef NOWATER
/**	\brief The R_DrawTiltedTranslucentWaterSpan_NPO2_32 function
	Like DrawTiltedTranslucentSpan, but for water
*/
void R_DrawTiltedTranslucentWaterSpan_NPO2_32(void)
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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = BASEVIDWIDTH*BASEVIDWIDTH/vid.width / (zeroheight - FIXED_TO_FLOAT(viewz)) / 21.0f;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	dsrc = ((UINT32 *)screens[1]) + (ds_y+ds_bgofs)*vid.width + ds_x1;
	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;
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
		*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
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

						WriteTranslucentWaterSpan(colormapu32[source[((y * ds_flatwidth) + x)]]);
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

						WriteTranslucentWaterSpan(colormap[source[((y * ds_flatwidth) + x)]]);
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

							WriteTranslucentWaterSpan(colormap[source[((y * ds_flatwidth) + x)]]);
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
				u = (INT64)(startu) + viewx;
				v = (INT64)(startv) + viewy;

				for (i = SPANSIZE-1; i >= 0; i--)
				{
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						WriteTranslucentWaterSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
						fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
						fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

						// Carefully align all of my Friends.
						if (x < 0)
							x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
						if (y < 0)
							y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

						x %= ds_flatwidth;
						y %= ds_flatheight;

						WriteTranslucentWaterSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							WriteTranslucentWaterSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (i = SPANSIZE-1; i >= 0; i--)
			{
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

					*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
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
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

					*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
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
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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

						*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
					}
					dest++;
					u += stepu;
					v += stepv;
				}
			}
		}
	}
#endif
}
#endif // NOWATER

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

	iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = BASEVIDWIDTH*BASEVIDWIDTH/vid.width / (zeroheight - FIXED_TO_FLOAT(viewz)) / 21.0f;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds_sup->z + ds_sup->y*(centery-ds_y) + ds_sup->x*(ds_x1-centerx);
	vz = ds_svp->z + ds_svp->y*(centery-ds_y) + ds_svp->x*(ds_x1-centerx);
	dest = (UINT32 *)(ylookup[ds_y] + columnofs[ds_x1]);

	source = ds_source;
	sourceu32 = (UINT32 *)ds_source;
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

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
						fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
						fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

						// Carefully align all of my Friends.
						if (x < 0)
							x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
						if (y < 0)
							y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

						x %= ds_flatwidth;
						y %= ds_flatheight;

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
				u = (INT64)(startu) + viewx;
				v = (INT64)(startv) + viewy;

				for (i = SPANSIZE-1; i >= 0; i--)
				{
					colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

						val = colormap[source[((y * ds_flatwidth) + x)]];
					}
					if (val != TRANSPARENTPIXEL)
						*dest = (colormapu32[val]);
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
						fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
						fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

						// Carefully align all of my Friends.
						if (x < 0)
							x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
						if (y < 0)
							y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

						x %= ds_flatwidth;
						y %= ds_flatheight;

						val = colormap[source[((y * ds_flatwidth) + x)]];
					}
					if (val != TRANSPARENTPIXEL)
						*dest = (colormapu32[val]);
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
						colormapu32 = planezlight_u32[tiltlighting[ds_x1++]] + ((UINT32*)ds_colormap - colormaps_u32);
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

							val = colormap[source[((y * ds_flatwidth) + x)]];
						}
						if (val != TRANSPARENTPIXEL)
							*dest = (colormapu32[val]);
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
			u = (INT64)(startu) + viewx;
			v = (INT64)(startv) + viewy;

			for (i = SPANSIZE-1; i >= 0; i--)
			{
				dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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
					fixed_t x = (((fixed_t)u-viewx) >> FRACBITS);
					fixed_t y = (((fixed_t)v-viewy) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
					if (y < 0)
						y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

					x %= ds_flatwidth;
					y %= ds_flatheight;

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
				u = (INT64)(startu) + viewx;
				v = (INT64)(startv) + viewy;

				for (; width != 0; width--)
				{
					dp_lighting = TC_CalcScaleLight(planezlight_u32[tiltlighting[ds_x1++]]);
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
#endif
}

/**	\brief The R_DrawSplat_NPO2_32 function
	Just like R_DrawSpan_NPO2_32, but skips transparent pixels.
*/
void R_DrawSplat_NPO2_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
					*dest = colormapu32[val];
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
			fixed_t x = (xposition >> FRACBITS);
			fixed_t y = (yposition >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			val = sourceu32[((y * ds_flatwidth) + x)];
			if (val != TRANSPARENTPIXEL)
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
void R_DrawTranslucentSplat_NPO2_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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
					WriteTranslucentSpan(colormap[val]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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
					WriteTranslucentSpan32(colormapu32[val]);
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
			fixed_t x = (xposition >> FRACBITS);
			fixed_t y = (yposition >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			val = sourceu32[((y * ds_flatwidth) + x)];
			if (val != TRANSPARENTPIXEL)
				*dest = TC_TranslucentColorMix(val, *dest, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

/**	\brief The R_DrawTranslucentSpan_NPO2_32 function
	Draws the actual span with translucency.
*/
void R_DrawTranslucentSpan_NPO2_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
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

				val = colormap[source[((y * ds_flatwidth) + x)]];
				WriteTranslucentSpan(val);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
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

				val = colormapu32[source[((y * ds_flatwidth) + x)]];
				WriteTranslucentSpan32(val);
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
			fixed_t x = (xposition >> FRACBITS);
			fixed_t y = (yposition >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			val = sourceu32[((y * ds_flatwidth) + x)];
			*dest = TC_TranslucentColorMix(val, *dest, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}

#ifndef NOWATER
void R_DrawTranslucentWaterSpan_NPO2_32(void)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

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

	if (ds_picfmt == PICFMT_FLAT)
	{
		if (ds_colmapstyle == TC_COLORMAPSTYLE_8BPP)
		{
			while (count--)
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

				WriteTranslucentWaterSpan(colormap[source[((y * ds_flatwidth) + x)]]);
				dest++;
				xposition += xstep;
				yposition += ystep;
			}
		}
		else if (ds_colmapstyle == TC_COLORMAPSTYLE_32BPP)
		{
			while (count--)
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

				WriteTranslucentWaterSpan32(colormapu32[source[((y * ds_flatwidth) + x)]]);
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
			fixed_t x = (xposition >> FRACBITS);
			fixed_t y = (yposition >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x = ds_flatwidth - ((UINT32)(ds_flatwidth - x) % ds_flatwidth);
			if (y < 0)
				y = ds_flatheight - ((UINT32)(ds_flatheight - y) % ds_flatheight);

			x %= ds_flatwidth;
			y %= ds_flatheight;

			*dest = TC_TranslucentColorMix(sourceu32[((y * ds_flatwidth) + x)], *dsrc++, ds_alpha);
			dest++;
			xposition += xstep;
			yposition += ystep;
		}
	}
}
#endif
