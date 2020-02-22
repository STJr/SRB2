// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw16.c
/// \brief 16bpp (HIGHCOLOR) span/column drawer functions
/// \note  no includes because this is included as part of r_draw.c

// ==========================================================================
// COLUMNS
// ==========================================================================

/// \brief kick out the upper bit of each component (we're in 5 : 5 : 5)
#define HIMASK1 0x7bde

/**	\brief The R_DrawColumn_16 function
	standard upto 128high posts column drawer
*/
void R_DrawColumn_16(void)
{
	INT32 count;
	INT16 *dest;
	fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	// Zero length, column does not exceed a pixel.
	if (count <= 0)
		return;

#ifdef RANGECHECK
	if (dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawColumn_16: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	dest = (INT16 *)(void *)(ylookup[dc_yl] + columnofs[dc_x]);

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.

	do
	{
		// Re-map color indices from wall texture column using a lighting/special effects LUT.
		*dest = hicolormaps[((INT16 *)(void *)dc_source)[(frac>>FRACBITS)&127]>>1];

		dest += vid.width;
		frac += fracstep;
	} while (--count);
}

/**	\brief The R_DrawWallColumn_16 function
	LAME cutnpaste: same as R_DrawColumn_16 but wraps around 256
	instead of 128 for the tall sky textures (256x240)
*/
void R_DrawWallColumn_16(void)
{
	INT32 count;
	INT16 *dest;
	fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	// Zero length, column does not exceed a pixel.
	if (count <= 0)
		return;

#ifdef RANGECHECK
	if (dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawWallColumn_16: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = (INT16 *)(void *)(ylookup[dc_yl] + columnofs[dc_x]);

	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	do
	{
		*dest = hicolormaps[((INT16 *)(void *)dc_source)[(frac>>FRACBITS)&255]>>1];

		dest += vid.width;
		frac += fracstep;
	} while (--count);
}

/**	\brief The R_DrawTranslucentColumn_16 function
		LAME cutnpaste: same as R_DrawColumn_16 but does
		translucent
*/
void R_DrawTranslucentColumn_16(void)
{
	INT32 count;
	INT16 *dest;
	fixed_t frac, fracstep;

	// check out coords for src*
	if ((dc_yl < 0) || (dc_x >= vid.width))
		return;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if (dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_16: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// FIXME. As above.
	dest = (INT16 *)(void *)(ylookup[dc_yl] + columnofs[dc_x]);

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Here we do an additional index re-mapping.
	do
	{
		*dest = (INT16)((INT16)((color8to16[dc_source[frac>>FRACBITS]]>>1) & 0x39ce)
			+ (INT16)(((*dest & HIMASK1)) & 0x7fff));

		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

/**	\brief The R_DrawTranslatedColumn_16 function
	?
*/
void R_DrawTranslatedColumn_16(void)
{
	INT32 count;
	INT16 *dest;
	fixed_t frac, fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if (dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_16: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = (INT16 *)(void *)(ylookup[dc_yl] + columnofs[dc_x]);

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Here we do an additional index re-mapping.
	do
	{
		*dest = color8to16[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]];
		dest += vid.width;

		frac += fracstep;
	} while (count--);
}

// ==========================================================================
// SPANS
// ==========================================================================

/**	\brief The R_*_16 function
	Draws the actual span.
*/
void R_DrawSpan_16(void)
{
	fixed_t xfrac, yfrac;
	INT16 *dest;
	INT32 count, spot;

#ifdef RANGECHECK
	if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= vid.width || ds_y > vid.height)
		I_Error("R_DrawSpan_16: %d to %d at %d", ds_x1, ds_x2, ds_y);
#endif

	xfrac = ds_xfrac;
	yfrac = ds_yfrac;

	dest = (INT16 *)(void *)(ylookup[ds_y] + columnofs[ds_x1]);

	// We do not check for zero spans here?
	count = ds_x2 - ds_x1;

	if (count <= 0) // We do now!
		return;

	do
	{
		// Current texture index in u, v.
		spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);

		// Lookup pixel from flat texture tile, re-index using light/colormap.
		*dest++ = hicolormaps[((INT16 *)(void *)ds_source)[spot]>>1];

		// Next step in u, v.
		xfrac += ds_xstep;
		yfrac += ds_ystep;
	} while (count--);
}
