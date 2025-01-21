// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
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
void R_DrawColumn_8(void)
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
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
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

/**	\brief The R_DrawColumnClamped_8 function
	Same as R_DrawColumn_8, but prevents artifacts from showing up (caused by fixed-point imprecisions)
*/
void R_DrawColumnClamped_8(void)
{
	INT32 count;
	UINT8 *dest;
	fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const UINT8 *source = dc_source;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight-1;
		INT32 idx;
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
				idx = frac>>FRACBITS;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
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
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;
				frac += fracstep;
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
			}
		}
	}
}

void R_Draw2sMultiPatchColumn_8(void)
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
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		register UINT8 val;
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

void R_Draw2sMultiPatchTranslucentColumn_8(void)
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
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		register const UINT8 *transmap = dc_transmap;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		register UINT8 val;
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
void R_DrawShadeColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	// check out coords for src*
	if ((dc_yl < 0) || (dc_x >= vid.width))
		return;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawShadeColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Here we do an additional index re-mapping.
	do
	{
		*dest = colormaps[(dc_source[frac>>FRACBITS] <<8) + (*dest)];
		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

/**	\brief The R_DrawTranslucentColumn_8 function
	I've made an asm routine for the transparency, because it slows down
	a lot in 640x480 with big sprites (bfg on all screen, or transparent
	walls on fullscreen)
*/
void R_DrawTranslucentColumn_8(void)
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

	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		register const UINT8 *transmap = dc_transmap;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight - 1;
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

/**	\brief The R_DrawTranslucentColumnClamped_8 function
	Same as R_DrawTranslucentColumn_8, but prevents artifacts from showing up (caused by fixed-point imprecisions)
*/
void R_DrawTranslucentColumnClamped_8(void)
{
	INT32 count;
	UINT8 *dest;
	fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumnClamped_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const UINT8 *source = dc_source;
		const UINT8 *transmap = dc_transmap;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight - 1;
		INT32 idx;
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
				idx = frac>>FRACBITS;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
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
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
			}
		}
	}
}

// Hack: A cut-down copy of R_DrawTranslucentColumn_8 that does not read texture
// data since something about calculating the texture reading address for drop shadows is broken.
// dc_texturemid and dc_iscale get wrong values for drop shadows, however those are not strictly
// needed for the current design of the shadows, so this function bypasses the issue
// by not using those variables at all.
void R_DrawDropShadowColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	dest = &topleft[dc_yl*vid.width + dc_x];

	{
#define DSCOLOR 31 // palette index for the color of the shadow
		register const UINT8 *transmap_offset = dc_transmap + (dc_colormap[DSCOLOR] << 8);
#undef DSCOLOR
		while ((count -= 2) >= 0)
		{
			*dest = *(transmap_offset + (*dest));
			dest += vid.width;
			*dest = *(transmap_offset + (*dest));
			dest += vid.width;
		}
		if (count & 1)
			*dest = *(transmap_offset + (*dest));
	}
}

/**	\brief The R_DrawTranslatedTranslucentColumn_8 function
	Spiffy function. Not only does it colormap a sprite, but does translucency as well.
	Uber-kudos to Cyan Helkaraxe
*/
void R_DrawTranslatedTranslucentColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register INT32 heightmask = dc_texheight - 1;
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
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix

				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest));

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
				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
		}
	}
}

/**	\brief The R_DrawTranslatedColumn_8 function
	Draw columns up to 128 high but remap the green ramp to other colors

  \warning STILL NOT IN ASM, TO DO..
*/
void R_DrawTranslatedColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Here we do an additional index re-mapping.
	do
	{
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo.
		*dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];

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
void R_DrawSpan_8 (void)
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

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	if (dest+8 > deststop)
		return;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		*dest++ = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedSpan_8 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_8(void)
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

		*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
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
			*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
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
			*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
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
				*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
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
void R_DrawTiltedTranslucentSpan_8(void)
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
		*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
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
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
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
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
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
				*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedWaterSpan_8 function
	Like DrawTiltedTranslucentSpan, but for water
*/
void R_DrawTiltedWaterSpan_8(void)
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
		*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
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
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
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
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
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
				*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_8(void)
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

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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

void R_DrawTiltedTranslucentSplat_8(void)
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

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds_transmap + (colormap[val] << 8) + *dest);

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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
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
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val != TRANSPARENTPIXEL)
					*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
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
void R_DrawSplat_8 (void)
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

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		//
		// <Callum> 4194303 = (2048x2048)-1 (2048x2048 is maximum flat size)
		// Why decimal? 0x3FFFFF == 4194303... ~Golden
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[0] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[1] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[2] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[3] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[4] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[5] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[6] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
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
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
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
void R_DrawTranslucentSplat_8 (void)
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

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[0] = *(ds_transmap + (colormap[val] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[1] = *(ds_transmap + (colormap[val] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[2] = *(ds_transmap + (colormap[val] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[3] = *(ds_transmap + (colormap[val] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[4] = *(ds_transmap + (colormap[val] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[5] = *(ds_transmap + (colormap[val] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[6] = *(ds_transmap + (colormap[val] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[7] = *(ds_transmap + (colormap[val] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFloorSprite_8 function
	Just like R_DrawSplat_8, but for floor sprites.
*/
void R_DrawFloorSprite_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[0] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[1] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[2] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[3] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[4] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[5] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[6] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
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
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
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
void R_DrawTranslucentFloorSprite_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[0] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[1] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[2] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[3] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[4] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[5] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[6] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[7] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedFloorSprite_8 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_8(void)
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
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
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
void R_DrawTiltedTranslucentFloorSprite_8(void)
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
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
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
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
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val & 0xFF00)
					*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
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
void R_DrawTranslucentSpan_8 (void)
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

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		dest[1] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		dest[2] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		dest[3] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		dest[4] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		dest[5] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		dest[6] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		dest[7] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		*dest = *(ds_transmap + (colormap[source[val]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawWaterSpan_8(void)
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
	xposition = ds_xfrac << nflatshiftup; yposition = (ds_yfrac + ds_waterofs) << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		*dest++ = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFogSpan_8 function
	Draws the actual span with fogging.
*/
void R_DrawFogSpan_8(void)
{
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;

	colormap = ds_colormap;
	dest = &topleft[ds_y *vid.width + ds_x1];

	count = ds_x2 - ds_x1 + 1;

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

/**	\brief The R_DrawTiltedFogSpan_8 function
	Draws a tilted span with fogging.
*/
void R_DrawTiltedFogSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest = colormap[*dest];
		dest++;
	} while (--width >= 0);
}

/**	\brief The R_DrawSolidColorSpan_8 function
	Draws a solid color span.
*/
void R_DrawSolidColorSpan_8(void)
{
	size_t count = (ds_x2 - ds_x1 + 1);

	UINT8 source = ds_colormap[ds_source[0]];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	memset(dest, source, count);
}

/**	\brief The R_DrawTransSolidColorSpan_8 function
	Draws a translucent solid color span.
*/
void R_DrawTransSolidColorSpan_8(void)
{
	size_t count = (ds_x2 - ds_x1 + 1);

	UINT8 source = ds_colormap[ds_source[0]];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	while (count-- && dest <= deststop)
	{
		*dest = *(ds_transmap + (source << 8) + *dest);
		dest++;
	}
}

/**	\brief The R_DrawTiltedSolidColorSpan_8 function
	Draws a tilted solid color span.
*/
void R_DrawTiltedSolidColorSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest++ = colormap[source];
	} while (--width >= 0);
}

/**	\brief The R_DrawTiltedTransSolidColorSpan_8 function
	Draws a tilted and translucent solid color span.
*/
void R_DrawTiltedTransSolidColorSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest = *(ds_transmap + (colormap[source] << 8) + *dest);
		dest++;
	} while (--width >= 0);
}

/**	\brief The R_DrawWaterSolidColorSpan_8 function
	Draws a water solid color span.
*/
void R_DrawWaterSolidColorSpan_8(void)
{
	UINT8 source = ds_source[0];
	UINT8 *colormap = ds_colormap;
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];
	UINT8 *dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	size_t count = (ds_x2 - ds_x1 + 1);
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	while (count-- && dest <= deststop)
	{
		*dest = colormap[*(ds_transmap + (source << 8) + *dsrc++)];
		dest++;
	}
}

/**	\brief The R_DrawTiltedWaterSolidColorSpan_8 function
	Draws a tilted water solid color span.
*/
void R_DrawTiltedWaterSolidColorSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];
	UINT8 *dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest++ = *(ds_transmap + (colormap[source] << 8) + *dsrc++);
	} while (--width >= 0);
}

/**	\brief The R_DrawFogColumn_8 function
	Fog wall.
*/
void R_DrawFogColumn_8(void)
{
	INT32 count;
	UINT8 *dest;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawFogColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// Framebuffer destination address.
	dest = &topleft[dc_yl*vid.width + dc_x];

	// Determine scaling, which is the only mapping to be done.
	do
	{
		// Simple. Apply the colormap to what's already on the screen.
		*dest = dc_colormap[*dest];
		dest += vid.width;
	} while (count--);
}

/**	\brief The R_DrawShadeColumn_8 function
	This is for 3D floors that cast shadows on walls.

	This function just cuts the column up into sections and calls R_DrawColumn_8
*/
void R_DrawColumnShadowed_8(void)
{
	INT32 count, realyh, i, height, bheight = 0, solid = 0;

	realyh = dc_yh;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawColumnShadowed_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// This runs through the lightlist from top to bottom and cuts up the column accordingly.
	for (i = 0; i < dc_numlights; i++)
	{
		// If the height of the light is above the column, get the colormap
		// anyway because the lighting of the top should be affected.
		solid = dc_lightlist[i].flags & FOF_CUTSOLIDS;

		height = dc_lightlist[i].height >> LIGHTSCALESHIFT;
		if (solid)
		{
			bheight = dc_lightlist[i].botheight >> LIGHTSCALESHIFT;
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
		if (height <= dc_yl)
		{
			dc_colormap = dc_lightlist[i].rcolormap;
			if (solid && dc_yl < bheight)
				dc_yl = bheight;
			continue;
		}
		// Found a break in the column!
		dc_yh = height;

		if (dc_yh > realyh)
			dc_yh = realyh;
		(colfuncs[BASEDRAWFUNC])();		// R_DrawColumn_8 for the appropriate architecture
		if (solid)
			dc_yl = bheight;
		else
			dc_yl = dc_yh + 1;

		dc_colormap = dc_lightlist[i].rcolormap;
	}
	dc_yh = realyh;
	if (dc_yl <= realyh)
		(colfuncs[BASEDRAWFUNC])();		// R_DrawWallColumn_8 for the appropriate architecture
}
