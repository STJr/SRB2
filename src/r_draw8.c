// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw8.c
/// \brief 8bpp span/column drawer functions
/// \note  no includes because this is included as part of r_draw.c

// ==========================================================================
// COLUMNS
// ==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.
//

/**	\brief The R_DrawColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawColumn_8(colcontext_t *dc)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const lighttable_t *colormap = dc->colormap;
		register INT32 heightmask = dc->texheight-1;
		if (dc->texheight & heightmask)   // not a power of 2 -- killough
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
				*dest = colormap[source[frac>>FRACBITS]];
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
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
		}
	}
}

void R_Draw2sMultiPatchColumn_8(colcontext_t *dc)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const lighttable_t *colormap = dc->colormap;
		register INT32 heightmask = dc->texheight-1;
		register UINT8 val;
		if (dc->texheight & heightmask)   // not a power of 2 -- killough
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
				val = source[frac>>FRACBITS];

				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];

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
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest += vid.width;
				frac += fracstep;
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
			}
		}
	}
}

void R_Draw2sMultiPatchTranslucentColumn_8(colcontext_t *dc)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const UINT8 *transmap = dc->transmap;
		register const lighttable_t *colormap = dc->colormap;
		register INT32 heightmask = dc->texheight-1;
		register UINT8 val;
		if (dc->texheight & heightmask)   // not a power of 2 -- killough
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
				val = source[frac>>FRACBITS];

				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));

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
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));
			}
		}
	}
}

/**	\brief The R_DrawShadeColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawShadeColumn_8(colcontext_t *dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	// check out coords for src*
	if ((dc->yl < 0) || (dc->x >= vid.width))
		return;

	count = dc->yh - dc->yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawShadeColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Here we do an additional index re-mapping.
	do
	{
		*dest = colormaps[(dc->source[frac>>FRACBITS] <<8) + (*dest)];
		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

/**	\brief The R_DrawTranslucentColumn_8 function
	I've made an asm routine for the transparency, because it slows down
	a lot in 640x480 with big sprites (bfg on all screen, or transparent
	walls on fullscreen)
*/
void R_DrawTranslucentColumn_8(colcontext_t *dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc->yh - dc->yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const UINT8 *transmap = dc->transmap;
		register const lighttable_t *colormap = dc->colormap;
		register INT32 heightmask = dc->texheight - 1;
		if (dc->texheight & heightmask)
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
				*dest = *(transmap + (colormap[source[frac>>FRACBITS]]<<8) + (*dest));
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
				*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
		}
	}
}

/**	\brief The R_DrawTranslatedTranslucentColumn_8 function
	Spiffy function. Not only does it colormap a sprite, but does translucency as well.
	Uber-kudos to Cyan Helkaraxe
*/
void R_DrawTranslatedTranslucentColumn_8(colcontext_t *dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc->yh - dc->yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	// FIXME. As above.
	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register INT32 heightmask = dc->texheight - 1;
		if (dc->texheight & heightmask)
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
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix

				*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[frac>>FRACBITS]]]<<8) + (*dest));

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
				*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
		}
	}
}

/**	\brief The R_DrawTranslatedColumn_8 function
	Draw columns up to 128 high but remap the green ramp to other colors

  \warning STILL NOT IN ASM, TO DO..
*/
void R_DrawTranslatedColumn_8(colcontext_t *dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc->yh - dc->yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc->texturemid + (dc->yl-centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep));

	// Here we do an additional index re-mapping.
	do
	{
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo.
		*dest = dc->colormap[dc->translation[dc->source[frac>>FRACBITS]]];

		dest += vid.width;

		frac += fracstep;
	} while (count--);
}

// ==========================================================================
// SPANS
// ==========================================================================

#define SPANSIZE 16
#define INVSPAN 0.0625f

/**	\brief The R_DrawSpan_8 function
	Draws the actual span.
*/
void R_DrawSpan_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	if (dest+8 > deststop)
		return;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		*dest++ = colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedSpan_8 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_8(spancontext_t *ds)
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

	iz = ds->szp->z + ds->szp->y*(centery-ds->y) + ds->szp->x*(ds->x1-centerx);

	

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

		*dest = colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]];
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
			*dest = colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]];
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
			*dest = colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]];
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
				*dest = colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]];
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedTranslucentSpan_8 function
	Like DrawTiltedSpan, but translucent
*/
void R_DrawTiltedTranslucentSpan_8(spancontext_t *ds)
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
		*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dest);
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
			*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dest);
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
			*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dest);
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
				*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dest);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedTranslucentWaterSpan_8 function
	Like DrawTiltedTranslucentSpan, but for water
*/
void R_DrawTiltedTranslucentWaterSpan_8(spancontext_t *ds)
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
		*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dsrc++);
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
			*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dsrc++);
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
			*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dsrc++);
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
				*dest = *(ds->transmap + (colormap[source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)]] << 8) + *dsrc++);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_8(spancontext_t *ds)
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

		val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
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
			val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
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
			val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
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
				val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
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

/**	\brief The R_DrawSplat_8 function
	Just like R_DrawSpan_8, but skips transparent pixels.
*/
void R_DrawSplat_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		//
		// <Callum> 4194303 = (2048x2048)-1 (2048x2048 is maximum flat size)
		// Why decimal? 0x3FFFFF == 4194303... ~Golden
		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[0] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[1] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[2] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[3] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[4] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[5] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[6] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[7] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentSplat_8 function
	Just like R_DrawSplat_8, but is translucent!
*/
void R_DrawTranslucentSplat_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[0] = *(ds->transmap + (colormap[val] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[1] = *(ds->transmap + (colormap[val] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[2] = *(ds->transmap + (colormap[val] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[3] = *(ds->transmap + (colormap[val] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[4] = *(ds->transmap + (colormap[val] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[5] = *(ds->transmap + (colormap[val] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[6] = *(ds->transmap + (colormap[val] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[7] = *(ds->transmap + (colormap[val] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds->transmap + (colormap[val] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFloorSprite_8 function
	Just like R_DrawSplat_8, but for floor sprites.
*/
void R_DrawFloorSprite_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = (UINT16 *)ds->source;
	colormap = ds->colormap;
	translation = ds->translation;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[0] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[1] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[2] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[3] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[4] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[5] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[6] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[7] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			*dest = colormap[translation[val & 0xFF]];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentFloorSplat_8 function
	Just like R_DrawFloorSprite_8, but is translucent!
*/
void R_DrawTranslucentFloorSprite_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = (UINT16 *)ds->source;
	colormap = ds->colormap;
	translation = ds->translation;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[0] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[1] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[2] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[3] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[4] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[5] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[6] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			dest[7] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)];
		if (val & 0xFF00)
			*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedFloorSprite_8 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_8(spancontext_t *ds)
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
			val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
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
			val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
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
				val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
				if (val & 0xFF00)
					*dest = colormap[translation[val & 0xFF]];
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTiltedTranslucentFloorSprite_8 function
	Draws a tilted, translucent, floor sprite.
*/
void R_DrawTiltedTranslucentFloorSprite_8(spancontext_t *ds)
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
			val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
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
			val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
			if (val & 0xFF00)
				*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
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
				val = source[((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift)];
				if (val & 0xFF00)
					*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTranslucentSpan_8 function
	Draws the actual span with translucency.
*/
void R_DrawTranslucentSpan_8(spancontext_t *ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		dest[1] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		dest[2] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		dest[3] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		dest[4] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		dest[5] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		dest[6] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		dest[7] = *(ds->transmap + (colormap[source[(((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift)]] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		*dest = *(ds->transmap + (colormap[source[val]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTranslucentWaterSpan_8(spancontext_t *ds)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	size_t count;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds->xfrac << ds->nflatshiftup; yposition = (ds->yfrac + ds->waterofs) << ds->nflatshiftup;
	xstep = ds->xstep << ds->nflatshiftup; ystep = ds->ystep << ds->nflatshiftup;

	source = ds->source;
	colormap = ds->colormap;
	dest = ylookup[ds->y] + columnofs[ds->x1];
	dsrc = screens[1] + (ds->y+ds->bgofs)*vid.width + ds->x1;
	count = ds->x2 - ds->x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		*dest++ = colormap[*(ds->transmap + (source[((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFogSpan_8 function
	Draws the actual span with fogging.
*/
void R_DrawFogSpan_8(spancontext_t *ds)
{
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;

	colormap = ds->colormap;
	//dest = ylookup[ds->y] + columnofs[ds->x1];
	dest = &topleft[ds->y *vid.width + ds->x1];

	count = ds->x2 - ds->x1 + 1;

	while (count >= 4)
	{
		dest[0] = colormap[dest[0]];
		dest[1] = colormap[dest[1]];
		dest[2] = colormap[dest[2]];
		dest[3] = colormap[dest[3]];

		dest += 4;
		count -= 4;
	}

	while (count--)
	{
		*dest = colormap[*dest];
		dest++;
	}
}

/**	\brief The R_DrawFogColumn_8 function
	Fog wall.
*/
void R_DrawFogColumn_8(colcontext_t *dc)
{
	INT32 count;
	UINT8 *dest;

	count = dc->yh - dc->yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawFogColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	//dest = ylookup[dc->yl] + columnofs[dc->x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Determine scaling, which is the only mapping to be done.
	do
	{
		// Simple. Apply the colormap to what's already on the screen.
		*dest = dc->colormap[*dest];
		dest += vid.width;
	} while (count--);
}

/**	\brief The R_DrawShadeColumn_8 function
	This is for 3D floors that cast shadows on walls.

	This function just cuts the column up into sections and calls R_DrawColumn_8
*/
void R_DrawColumnShadowed_8(colcontext_t *dc)
{
	INT32 i, height, bheight = 0, solid = 0;
	INT32 realyh = dc->yh;
	INT32 count = dc->yh - dc->yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawColumnShadowed_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// This runs through the lightlist from top to bottom and cuts up the column accordingly.
	for (i = 0; i < dc->numlights; i++)
	{
		// If the height of the light is above the column, get the colormap
		// anyway because the lighting of the top should be affected.
		solid = dc->lightlist[i].flags & FF_CUTSOLIDS;

		height = dc->lightlist[i].height >> LIGHTSCALESHIFT;
		if (solid)
		{
			bheight = dc->lightlist[i].botheight >> LIGHTSCALESHIFT;
			if (bheight < height)
			{
				// confounded slopes sometimes allow partial invertedness,
				// even including cases where the top and bottom heights
				// should actually be the same!
				// swap the height values as a workaround for this quirk
				INT32 temp = height;
				height = bheight;
				bheight = temp;
			}
		}
		if (height <= dc->yl)
		{
			dc->colormap = dc->lightlist[i].rcolormap;
			if (solid && dc->yl < bheight)
				dc->yl = bheight;
			continue;
		}
		// Found a break in the column!
		dc->yh = height;

		if (dc->yh > realyh)
			dc->yh = realyh;
		(colfuncs[BASEDRAWFUNC])(dc);		// R_DrawColumn_8 for the appropriate architecture
		if (solid)
			dc->yl = bheight;
		else
			dc->yl = dc->yh + 1;

		dc->colormap = dc->lightlist[i].rcolormap;
	}

	dc->yh = realyh;
	if (dc->yl <= realyh)
		(colfuncs[BASEDRAWFUNC])(dc);		// R_DrawColumn_8 for the appropriate architecture
}
