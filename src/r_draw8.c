// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
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

#define TRANSPARENTPIXEL 247

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

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep))*(!dc_hires);

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
				*dest = colormap[*(transmap + (source[frac>>FRACBITS]<<8) + (*dest))];
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
				*dest = colormap[*(transmap + ((source[(frac>>FRACBITS)&heightmask]<<8)) + (*dest))];
				dest += vid.width;
				frac += fracstep;
				*dest = colormap[*(transmap + ((source[(frac>>FRACBITS)&heightmask]<<8)) + (*dest))];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = colormap[*(transmap + ((source[(frac>>FRACBITS)&heightmask]<<8)) + (*dest))];
		}
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

				*dest = dc_colormap[*(dc_transmap
					+ (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest))];

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
				*dest = dc_colormap[*(dc_transmap
					+ (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest))];
				dest += vid.width;
				frac += fracstep;
				*dest = dc_colormap[*(dc_transmap
					+ (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest))];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = dc_colormap[*(dc_transmap + (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]] <<8) + (*dest))];
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

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	//frac = dc_texturemid + (dc_yl-centery)*fracstep;
	frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep))*(!dc_hires);

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

/**	\brief The R_DrawSpan_8 function
	Draws the actual span.
*/
void R_DrawSpan_8 (void)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds_xfrac << nflatshiftup; yposition = ds_yfrac << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];
	count = ds_x2 - ds_x1 + 1;

	if (dest+8 > deststop)
		return;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		*dest++ = colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawSplat_8 function
	Just like R_DrawSpan_8, but skips transparent pixels.
*/
void R_DrawSplat_8 (void)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;
	UINT32 val;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds_xfrac << nflatshiftup; yposition = ds_yfrac << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		//
		// <Callum> 4194303 = (2048x2048)-1 (2048x2048 is maximum flat size)
		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[0] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[1] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[2] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[3] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[4] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[5] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[6] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[7] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		val = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift);
		val &= 4194303;
		val = source[val];
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
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;
	UINT8 val;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds_xfrac << nflatshiftup; yposition = ds_yfrac << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[0] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[0])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[1] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[1])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[2] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[2])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[3] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[3])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[4] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[4])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[5] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[5])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[6] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[6])];
		xposition += xstep;
		yposition += ystep;

		val = source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[7] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[7])];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		val =colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dest)];

		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentSpan_8 function
	Draws the actual span with translucent.
*/
void R_DrawTranslucentSpan_8 (void)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds_xfrac << nflatshiftup; yposition = ds_yfrac << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[0])];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[1])];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[2])];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[3])];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[4])];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[5])];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[6])];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + dest[7])];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		*dest = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dest)];
		dest++;
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
	//dest = ylookup[ds_y] + columnofs[ds_x1];
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
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	//dest = ylookup[dc_yl] + columnofs[dc_x];
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
		solid = dc_lightlist[i].flags & FF_CUTSOLIDS;

		height = dc_lightlist[i].height >> LIGHTSCALESHIFT;
		if (solid)
			bheight = dc_lightlist[i].botheight >> LIGHTSCALESHIFT;
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
		basecolfunc();		// R_DrawColumn_8 for the appropriate architecture
		if (solid)
			dc_yl = bheight;
		else
			dc_yl = dc_yh + 1;

		dc_colormap = dc_lightlist[i].rcolormap;
	}
	dc_yh = realyh;
	if (dc_yl <= realyh)
		walldrawerfunc();		// R_DrawWallColumn_8 for the appropriate architecture
}
