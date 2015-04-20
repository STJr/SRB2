// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_segs.c
/// \brief All the clipping: columns, horizontal spans, sky columns

#include "doomdef.h"
#include "r_local.h"
#include "r_sky.h"

#include "r_splats.h"

#include "w_wad.h"
#include "z_zone.h"
#include "d_netcmd.h"
#include "m_misc.h"
#include "p_local.h" // Camera...
#include "console.h" // con_clipviewtop

// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
static boolean segtextured;
static boolean markfloor; // False if the back side is the same plane.
static boolean markceiling;

static boolean maskedtexture;
static INT32 toptexture, bottomtexture, midtexture;
static INT32 numthicksides, numbackffloors;

angle_t rw_normalangle;
// angle to line origin
angle_t rw_angle1;
fixed_t rw_distance;

//
// regular wall
//
static INT32 rw_x, rw_stopx;
static angle_t rw_centerangle;
static fixed_t rw_offset;
static fixed_t rw_offset2; // for splats
static fixed_t rw_scale, rw_scalestep;
static fixed_t rw_midtexturemid, rw_toptexturemid, rw_bottomtexturemid;
static INT32 worldtop, worldbottom, worldhigh, worldlow;
#ifdef ESLOPE
static INT32 worldtopslope, worldbottomslope, worldhighslope, worldlowslope; // worldtop/bottom at end of slope
#endif
static fixed_t pixhigh, pixlow, pixhighstep, pixlowstep;
static fixed_t topfrac, topstep;
static fixed_t bottomfrac, bottomstep;

static lighttable_t **walllights;
static INT16 *maskedtexturecol;

// ==========================================================================
// R_Splats Wall Splats Drawer
// ==========================================================================

#ifdef WALLSPLATS
static INT16 last_ceilingclip[MAXVIDWIDTH];
static INT16 last_floorclip[MAXVIDWIDTH];

static void R_DrawSplatColumn(column_t *column)
{
	INT32 topscreen, bottomscreen;
	fixed_t basetexturemid;
	INT32 topdelta, prevdelta = -1;

	basetexturemid = dc_texturemid;

	for (; column->topdelta != 0xff ;)
	{
		// calculate unclipped screen coordinates for post
		topdelta = column->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topscreen = sprtopscreen + spryscale*topdelta;
		bottomscreen = topscreen + spryscale*column->length;

		dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
		dc_yh = (bottomscreen-1)>>FRACBITS;

		if (dc_yh >= last_floorclip[dc_x])
			dc_yh = last_floorclip[dc_x] - 1;
		if (dc_yl <= last_ceilingclip[dc_x])
			dc_yl = last_ceilingclip[dc_x] + 1;
		if (dc_yl <= dc_yh && dl_yh < vid.height && yh > 0)
		{
			dc_source = (UINT8 *)column + 3;
			dc_texturemid = basetexturemid - (topdelta<<FRACBITS);

			// Drawn by R_DrawColumn.
			colfunc();
		}
		column = (column_t *)((UINT8 *)column + column->length + 4);
	}

	dc_texturemid = basetexturemid;
}

static void R_DrawWallSplats(void)
{
	wallsplat_t *splat;
	seg_t *seg;
	angle_t angle, angle1, angle2;
	INT32 x1, x2;
	size_t pindex;
	column_t *col;
	patch_t *patch;
	fixed_t texturecolumn;

	splat = (wallsplat_t *)linedef->splats;

	I_Assert(splat != NULL);

	seg = ds_p->curline;

	// draw all splats from the line that touches the range of the seg
	for (; splat; splat = splat->next)
	{
		angle1 = R_PointToAngle(splat->v1.x, splat->v1.y);
		angle2 = R_PointToAngle(splat->v2.x, splat->v2.y);
		angle1 = (angle1 - viewangle + ANGLE_90)>>ANGLETOFINESHIFT;
		angle2 = (angle2 - viewangle + ANGLE_90)>>ANGLETOFINESHIFT;
		// out of the viewangletox lut
		/// \todo clip it to the screen
		if (angle1 > FINEANGLES/2 || angle2 > FINEANGLES/2)
			continue;
		x1 = viewangletox[angle1];
		x2 = viewangletox[angle2];

		if (x1 >= x2)
			continue; // does not cross a pixel

		// splat is not in this seg range
		if (x2 < ds_p->x1 || x1 > ds_p->x2)
			continue;

		if (x1 < ds_p->x1)
			x1 = ds_p->x1;
		if (x2 > ds_p->x2)
			x2 = ds_p->x2;
		if (x2 <= x1)
			continue;

		// calculate incremental stepping values for texture edges
		rw_scalestep = ds_p->scalestep;
		spryscale = ds_p->scale1 + (x1 - ds_p->x1)*rw_scalestep;
		mfloorclip = floorclip;
		mceilingclip = ceilingclip;

		patch = W_CachePatchNum(splat->patch, PU_CACHE);

		dc_texturemid = splat->top + (SHORT(patch->height)<<(FRACBITS-1)) - viewz;
		if (splat->yoffset)
			dc_texturemid += *splat->yoffset;

		sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

		// set drawing mode
		switch (splat->flags & SPLATDRAWMODE_MASK)
		{
			case SPLATDRAWMODE_OPAQUE:
				colfunc = basecolfunc;
				break;
			case SPLATDRAWMODE_TRANS:
				if (!cv_translucency.value)
					colfunc = basecolfunc;
				else
				{
					dc_transmap = ((tr_trans50 - 1)<<FF_TRANSSHIFT) + transtables;
					colfunc = fuzzcolfunc;
				}

				break;
			case SPLATDRAWMODE_SHADE:
				colfunc = shadecolfunc;
				break;
		}

		dc_texheight = 0;

		// draw the columns
		for (dc_x = x1; dc_x <= x2; dc_x++, spryscale += rw_scalestep)
		{
			pindex = spryscale>>LIGHTSCALESHIFT;
			if (pindex >= MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE - 1;
			dc_colormap = walllights[pindex];

			if (frontsector->extra_colormap)
				dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);

			sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
			dc_iscale = 0xffffffffu / (unsigned)spryscale;

			// find column of patch, from perspective
			angle = (rw_centerangle + xtoviewangle[dc_x])>>ANGLETOFINESHIFT;
				texturecolumn = rw_offset2 - splat->offset
					- FixedMul(FINETANGENT(angle), rw_distance);

			// FIXME!
			texturecolumn >>= FRACBITS;
			if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
				continue;

			// draw the texture
			col = (column_t *)((UINT8 *)patch + LONG(patch->columnofs[texturecolumn]));
			R_DrawSplatColumn(col);
		}
	} // next splat

	colfunc = basecolfunc;
}

#endif //WALLSPLATS

// ==========================================================================
// R_RenderMaskedSegRange
// ==========================================================================

// If we have a multi-patch texture on a 2sided wall (rare) then we draw
//  it using R_DrawColumn, else we draw it using R_DrawMaskedColumn, this
//  way we don't have to store extra post_t info with each column for
//  multi-patch textures. They are not normally needed as multi-patch
//  textures don't have holes in it. At least not for now.
static INT32 column2s_length; // column->length : for multi-patch on 2sided wall = texture->height

static void R_Render2sidedMultiPatchColumn(column_t *column)
{
	INT32 topscreen, bottomscreen;

	topscreen = sprtopscreen; // + spryscale*column->topdelta;  topdelta is 0 for the wall
	bottomscreen = topscreen + spryscale * column2s_length;

	dc_yl = (sprtopscreen+FRACUNIT-1)>>FRACBITS;
	dc_yh = (bottomscreen-1)>>FRACBITS;

	if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
	{
		dc_yl = ((windowtop + FRACUNIT)>>FRACBITS);
		dc_yh = (windowbottom - 1)>>FRACBITS;
	}

	if (dc_yh >= mfloorclip[dc_x])
		dc_yh =  mfloorclip[dc_x] - 1;
	if (dc_yl <= mceilingclip[dc_x])
		dc_yl =  mceilingclip[dc_x] + 1;

	if (dc_yl >= vid.height || dc_yh < 0)
		return;

	if (dc_yl <= dc_yh && dc_yh < vid.height && dc_yh > 0)
	{
		dc_source = (UINT8 *)column + 3;

		if (colfunc == wallcolfunc)
			twosmultipatchfunc();
		else
			colfunc();
	}
}

void R_RenderMaskedSegRange(drawseg_t *ds, INT32 x1, INT32 x2)
{
	size_t pindex;
	column_t *col;
	INT32 lightnum, texnum, i;
	fixed_t height, realbot;
	lightlist_t *light;
	r_lightlist_t *rlight;
	void (*colfunc_2s)(column_t *);
	line_t *ldef;
	sector_t *front, *back;
	INT32 times, repeats;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];
	windowbottom = windowtop = sprbotscreen = INT32_MAX;

	// hack translucent linedef types (900-909 for transtables 1-9)
	ldef = curline->linedef;
	switch (ldef->special)
	{
		case 900:
			dc_transmap = ((tr_trans10)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 901:
			dc_transmap = ((tr_trans20)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 902:
			dc_transmap = ((tr_trans30)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 903:
			dc_transmap = ((tr_trans40)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 904:
			dc_transmap = ((tr_trans50)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 905:
			dc_transmap = ((tr_trans60)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 906:
			dc_transmap = ((tr_trans70)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 907:
			dc_transmap = ((tr_trans80)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 908:
			dc_transmap = ((tr_trans90)<<FF_TRANSSHIFT) - 0x10000 + transtables;
			colfunc = fuzzcolfunc;
			break;
		case 909:
			colfunc = R_DrawFogColumn_8;
			windowtop = frontsector->ceilingheight;
			windowbottom = frontsector->floorheight;
			break;
		default:
			colfunc = wallcolfunc;
			break;
	}

	if (curline->polyseg && curline->polyseg->translucency > 0)
	{
		if (curline->polyseg->translucency >= NUMTRANSMAPS)
			return;

		dc_transmap = ((curline->polyseg->translucency)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		colfunc = fuzzcolfunc;
	}

	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;

	// handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
	// are not stored per-column with post info in SRB2
	if (textures[texnum]->holes)
		colfunc_2s = R_DrawMaskedColumn; // render the usual 2sided single-patch packed texture
	else
	{
		colfunc_2s = R_Render2sidedMultiPatchColumn; // render multipatch with no holes (no post_t info)
		column2s_length = textures[texnum]->height;
	}

	// Setup lighting based on the presence/lack-of 3D floors.
	dc_numlights = 0;
	if (frontsector->numlights)
	{
		dc_numlights = frontsector->numlights;
		if (dc_numlights >= dc_maxlights)
		{
			dc_maxlights = dc_numlights;
			dc_lightlist = Z_Realloc(dc_lightlist, sizeof (*dc_lightlist) * dc_maxlights, PU_STATIC, NULL);
		}

		for (i = 0; i < dc_numlights; i++)
		{
			light = &frontsector->lightlist[i];
			rlight = &dc_lightlist[i];
			rlight->height = (centeryfrac) - FixedMul((light->height - viewz), spryscale);
			rlight->heightstep = -FixedMul(rw_scalestep, (light->height - viewz));
			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = light->extra_colormap;
			rlight->flags = light->flags;

			if (rlight->flags & FF_FOG || (rlight->extra_colormap && rlight->extra_colormap->fog))
				lightnum = (rlight->lightlevel >> LIGHTSEGSHIFT);
			else if (colfunc == fuzzcolfunc)
				lightnum = LIGHTLEVELS - 1;
			else
				lightnum = (rlight->lightlevel >> LIGHTSEGSHIFT);

			if (rlight->extra_colormap && rlight->extra_colormap->fog)
				;
			else if (curline->v1->y == curline->v2->y)
				lightnum--;
			else if (curline->v1->x == curline->v2->x)
				lightnum++;

			rlight->lightnum = lightnum;
		}
	}
	else
	{
		if (colfunc == fuzzcolfunc)
		{
			if (frontsector->extra_colormap && frontsector->extra_colormap->fog)
				lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);
			else
				lightnum = LIGHTLEVELS - 1;
		}
		else
			lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);

		if (colfunc == R_DrawFogColumn_8
			|| (frontsector->extra_colormap && frontsector->extra_colormap->fog))
			;
		else if (curline->v1->y == curline->v2->y)
			lightnum--;
		else if (curline->v1->x == curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			walllights = scalelight[LIGHTLEVELS - 1];
		else
			walllights = scalelight[lightnum];
	}

	maskedtexturecol = ds->maskedtexturecol;

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	if (frontsector->heightsec != -1)
		front = &sectors[frontsector->heightsec];
	else
		front = frontsector;

	if (backsector->heightsec != -1)
		back = &sectors[backsector->heightsec];
	else
		back = backsector;

	if (ds->curline->sidedef->repeatcnt)
		repeats = 1 + ds->curline->sidedef->repeatcnt;
	else if (ldef->flags & ML_EFFECT5)
	{
		fixed_t high, low;

		if (front->ceilingheight > back->ceilingheight)
			high = back->ceilingheight;
		else
			high = front->ceilingheight;

		if (front->floorheight > back->floorheight)
			low = front->floorheight;
		else
			low = back->floorheight;

		repeats = (high - low)/textureheight[texnum];
		if ((high-low)%textureheight[texnum])
			repeats++; // tile an extra time to fill the gap -- Monster Iestyn
	}
	else
		repeats = 1;

	for (times = 0; times < repeats; times++)
	{
		if (times > 0)
		{
			rw_scalestep = ds->scalestep;
			spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
		}

		if (curline->linedef->flags & ML_DONTPEGBOTTOM)
		{
			dc_texturemid = front->floorheight > back->floorheight
				? front->floorheight : back->floorheight;
			dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
		}
		else
		{
			dc_texturemid = front->ceilingheight < back->ceilingheight
				? front->ceilingheight : back->ceilingheight;
			dc_texturemid = dc_texturemid - viewz;
		}
		dc_texturemid += curline->sidedef->rowoffset;

		if (curline->linedef->flags & ML_DONTPEGBOTTOM)
			dc_texturemid += (textureheight[texnum])*times;
		else
			dc_texturemid -= (textureheight[texnum])*times;

		dc_texheight = textureheight[texnum]>>FRACBITS;

		// draw the columns
		for (dc_x = x1; dc_x <= x2; dc_x++)
		{
			// calculate lighting
			if (maskedtexturecol[dc_x] != INT16_MAX)
			{
				if (dc_numlights)
				{
					lighttable_t **xwalllights;

					sprbotscreen = INT32_MAX;
					sprtopscreen = windowtop = (centeryfrac - FixedMul(dc_texturemid, spryscale));

					realbot = windowbottom = FixedMul(textureheight[texnum], spryscale) + sprtopscreen;
					dc_iscale = 0xffffffffu / (unsigned)spryscale;

					// draw the texture
					col = (column_t *)((UINT8 *)R_GetColumn(texnum, maskedtexturecol[dc_x]) - 3);

					for (i = 0; i < dc_numlights; i++)
					{
						rlight = &dc_lightlist[i];

						if ((rlight->flags & FF_NOSHADE))
							continue;

						if (rlight->lightnum < 0)
							xwalllights = scalelight[0];
						else if (rlight->lightnum >= LIGHTLEVELS)
							xwalllights = scalelight[LIGHTLEVELS-1];
						else
							xwalllights = scalelight[rlight->lightnum];

						pindex = FixedMul(spryscale, FixedDiv(640, vid.width))>>LIGHTSCALESHIFT;

						if (pindex >= MAXLIGHTSCALE)
							pindex = MAXLIGHTSCALE - 1;

						if (rlight->extra_colormap)
							rlight->rcolormap = rlight->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
						else
							rlight->rcolormap = xwalllights[pindex];

						rlight->height += rlight->heightstep;
						height = rlight->height;

						if (height <= windowtop)
						{
							dc_colormap = rlight->rcolormap;
							continue;
						}

						windowbottom = height;
						if (windowbottom >= realbot)
						{
							windowbottom = realbot;
							colfunc_2s(col);
							for (i++; i < dc_numlights; i++)
							{
								rlight = &dc_lightlist[i];
								rlight->height += rlight->heightstep;
							}

							continue;
						}
						colfunc_2s(col);
						windowtop = windowbottom + 1;
						dc_colormap = rlight->rcolormap;
					}
					windowbottom = realbot;
					if (windowtop < windowbottom)
						colfunc_2s(col);

					spryscale += rw_scalestep;
					continue;
				}

				// calculate lighting
				pindex = FixedMul(spryscale, FixedDiv(640, vid.width))>>LIGHTSCALESHIFT;

				if (pindex >= MAXLIGHTSCALE)
					pindex = MAXLIGHTSCALE - 1;

				dc_colormap = walllights[pindex];

				if (frontsector->extra_colormap)
					dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);

				sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
				dc_iscale = 0xffffffffu / (unsigned)spryscale;

				// draw the texture
				col = (column_t *)((UINT8 *)R_GetColumn(texnum, maskedtexturecol[dc_x]) - 3);

//#ifdef POLYOBJECTS_PLANES
#if 0 // Disabling this allows inside edges to render below the planes, for until the clipping is fixed to work right when POs are near the camera. -Red
				if (curline->dontrenderme && curline->polyseg && (curline->polyseg->flags & POF_RENDERPLANES))
				{
					fixed_t my_topscreen;
					fixed_t my_bottomscreen;
					fixed_t my_yl, my_yh;

					my_topscreen = sprtopscreen + spryscale*col->topdelta;
					my_bottomscreen = sprbotscreen == INT32_MAX ? my_topscreen + spryscale*col->length
					                                         : sprbotscreen + spryscale*col->length;

					my_yl = (my_topscreen+FRACUNIT-1)>>FRACBITS;
					my_yh = (my_bottomscreen-1)>>FRACBITS;
	//				CONS_Debug(DBG_RENDER, "my_topscreen: %d\nmy_bottomscreen: %d\nmy_yl: %d\nmy_yh: %d\n", my_topscreen, my_bottomscreen, my_yl, my_yh);

					if (numffloors)
					{
						INT32 top = my_yl;
						INT32 bottom = my_yh;

						for (i = 0; i < numffloors; i++)
						{
							if (!ffloor[i].polyobj || ffloor[i].polyobj != curline->polyseg)
								continue;

							if (ffloor[i].height < viewz)
							{
								INT32 top_w = ffloor[i].plane->top[dc_x];

	//							CONS_Debug(DBG_RENDER, "Leveltime : %d\n", leveltime);
	//							CONS_Debug(DBG_RENDER, "Top is %d, top_w is %d\n", top, top_w);
								if (top_w < top)
								{
									ffloor[i].plane->top[dc_x] = (INT16)top;
									ffloor[i].plane->picnum = 0;
								}
	//							CONS_Debug(DBG_RENDER, "top_w is now %d\n", ffloor[i].plane->top[dc_x]);
							}
							else if (ffloor[i].height > viewz)
							{
								INT32 bottom_w = ffloor[i].plane->bottom[dc_x];

								if (bottom_w > bottom)
								{
									ffloor[i].plane->bottom[dc_x] = (INT16)bottom;
									ffloor[i].plane->picnum = 0;
								}
							}
						}
					}
				}
				else
#endif
					colfunc_2s(col);
			}
			spryscale += rw_scalestep;
		}
	}
	colfunc = wallcolfunc;
}

// Loop through R_DrawMaskedColumn calls
static void R_DrawRepeatMaskedColumn(column_t *col)
{
	do {
		R_DrawMaskedColumn(col);
		sprtopscreen += dc_texheight*spryscale;
	} while (sprtopscreen < sprbotscreen);
}

//
// R_RenderThickSideRange
// Renders all the thick sides in the given range.
void R_RenderThickSideRange(drawseg_t *ds, INT32 x1, INT32 x2, ffloor_t *pfloor)
{
	size_t          pindex;
	column_t *      col;
	INT32             lightnum;
	INT32            texnum;
	sector_t        tempsec;
	INT32             templight;
	INT32             i, p;
	fixed_t         bottombounds = viewheight << FRACBITS;
	fixed_t         topbounds = (con_clipviewtop - 1) << FRACBITS;
	fixed_t         offsetvalue = 0;
	lightlist_t     *light;
	r_lightlist_t   *rlight;
	fixed_t         lheight;
	line_t          *newline = NULL;

	void (*colfunc_2s) (column_t *);

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally

	curline = ds->curline;
	backsector = pfloor->target;
	frontsector = curline->frontsector == pfloor->target ? curline->backsector : curline->frontsector;
	texnum = texturetranslation[sides[pfloor->master->sidenum[0]].midtexture];

	colfunc = wallcolfunc;

	if (pfloor->master->flags & ML_TFERLINE)
	{
		size_t linenum = curline->linedef-backsector->lines[0];
		newline = pfloor->master->frontsector->lines[0] + linenum;
		texnum = texturetranslation[sides[newline->sidenum[0]].midtexture];
	}

	if (pfloor->flags & FF_TRANSLUCENT)
	{
		boolean fuzzy = true;

		// Hacked up support for alpha value in software mode Tails 09-24-2002
		if (pfloor->alpha < 12)
			return; // Don't even draw it
		else if (pfloor->alpha < 38)
			dc_transmap = ((tr_trans90)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 64)
			dc_transmap = ((tr_trans80)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 89)
			dc_transmap = ((tr_trans70)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 115)
			dc_transmap = ((tr_trans60)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 140)
			dc_transmap = ((tr_trans50)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 166)
			dc_transmap = ((tr_trans40)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 192)
			dc_transmap = ((tr_trans30)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 217)
			dc_transmap = ((tr_trans20)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else if (pfloor->alpha < 243)
			dc_transmap = ((tr_trans10)<<FF_TRANSSHIFT) - 0x10000 + transtables;
		else
			fuzzy = false; // Opaque

		if (fuzzy)
			colfunc = fuzzcolfunc;
	}
	else if (pfloor->flags & FF_FOG)
		colfunc = R_DrawFogColumn_8;

	//SoM: Moved these up here so they are available for my lightlist calculations
	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;

	dc_numlights = 0;
	if (frontsector->numlights)
	{
		dc_numlights = frontsector->numlights;
		if (dc_numlights > dc_maxlights)
		{
			dc_maxlights = dc_numlights;
			dc_lightlist = Z_Realloc(dc_lightlist, sizeof (*dc_lightlist) * dc_maxlights, PU_STATIC, NULL);
		}

		for (i = p = 0; i < dc_numlights; i++)
		{
			light = &frontsector->lightlist[i];
			rlight = &dc_lightlist[p];

			if (light->height < *pfloor->bottomheight)
				continue;

			if (light->height > *pfloor->topheight && i+1 < dc_numlights && frontsector->lightlist[i+1].height > *pfloor->topheight)
					continue;

			lheight = light->height;// > *pfloor->topheight ? *pfloor->topheight + FRACUNIT : light->height;
			rlight->heightstep = -FixedMul (rw_scalestep, (lheight - viewz));
			rlight->height = (centeryfrac) - FixedMul((lheight - viewz), spryscale) - rlight->heightstep;
			rlight->flags = light->flags;

			if (light->flags & FF_CUTLEVEL)
			{
				lheight = *light->caster->bottomheight;// > *pfloor->topheight ? *pfloor->topheight + FRACUNIT : *light->caster->bottomheight;
				rlight->botheightstep = -FixedMul (rw_scalestep, (lheight - viewz));
				rlight->botheight = (centeryfrac) - FixedMul((lheight - viewz), spryscale) - rlight->botheightstep;
			}

			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = light->extra_colormap;

			// Check if the current light effects the colormap/lightlevel
			if (pfloor->flags & FF_FOG)
				rlight->lightnum = (pfloor->master->frontsector->lightlevel >> LIGHTSEGSHIFT);
			else
				rlight->lightnum = (rlight->lightlevel >> LIGHTSEGSHIFT);

			if (pfloor->flags & FF_FOG || rlight->flags & FF_FOG || (rlight->extra_colormap && rlight->extra_colormap->fog))
				;
			else if (curline->v1->y == curline->v2->y)
				rlight->lightnum--;
			else if (curline->v1->x == curline->v2->x)
				rlight->lightnum++;

			p++;
		}

		dc_numlights = p;
	}
	else
	{
		// Get correct light level!
		if ((frontsector->extra_colormap && frontsector->extra_colormap->fog))
			lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);
		else if (pfloor->flags & FF_FOG)
			lightnum = (pfloor->master->frontsector->lightlevel >> LIGHTSEGSHIFT);
		else if (colfunc == fuzzcolfunc)
			lightnum = LIGHTLEVELS-1;
		else
			lightnum = R_FakeFlat(frontsector, &tempsec, &templight, &templight, false)
				->lightlevel >> LIGHTSEGSHIFT;

		if (pfloor->flags & FF_FOG || (frontsector->extra_colormap && frontsector->extra_colormap->fog));
			else if (curline->v1->y == curline->v2->y)
		lightnum--;
		else if (curline->v1->x == curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			walllights = scalelight[LIGHTLEVELS-1];
		else
			walllights = scalelight[lightnum];
	}

	maskedtexturecol = ds->thicksidecol;

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;
	dc_texheight = textureheight[texnum]>>FRACBITS;

	dc_texturemid = *pfloor->topheight - viewz;

	if (newline)
	{
		offsetvalue = sides[newline->sidenum[0]].rowoffset;
		if (newline->flags & ML_DONTPEGBOTTOM)
			offsetvalue -= *pfloor->topheight - *pfloor->bottomheight;
	}
	else
	{
		offsetvalue = sides[pfloor->master->sidenum[0]].rowoffset;
		if (curline->linedef->flags & ML_DONTPEGBOTTOM)
			offsetvalue -= *pfloor->topheight - *pfloor->bottomheight;
	}

	dc_texturemid += offsetvalue;

	//faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
	//     are not stored per-column with post info anymore in Doom Legacy
	if (textures[texnum]->holes)
		colfunc_2s = R_DrawRepeatMaskedColumn;                    //render the usual 2sided single-patch packed texture
	else
	{
		colfunc_2s = R_Render2sidedMultiPatchColumn;        //render multipatch with no holes (no post_t info)
		column2s_length = textures[texnum]->height;
	}

	// draw the columns
	for (dc_x = x1; dc_x <= x2; dc_x++)
	{
		if (maskedtexturecol[dc_x] != INT16_MAX)
		{
			// SoM: New code does not rely on R_DrawColumnShadowed_8 which
			// will (hopefully) put less strain on the stack.
			if (dc_numlights)
			{
				lighttable_t **xwalllights;
				fixed_t height;
				fixed_t bheight = 0;
				INT32 solid = 0;
				INT32 lighteffect = 0;

				sprtopscreen = windowtop = (centeryfrac - FixedMul((dc_texturemid - offsetvalue), spryscale));
				sprbotscreen = windowbottom = FixedMul(*pfloor->topheight - *pfloor->bottomheight, spryscale) + sprtopscreen;

				// SoM: If column is out of range, why bother with it??
				if (windowbottom < topbounds || windowtop > bottombounds)
				{
					for (i = 0; i < dc_numlights; i++)
					{
						rlight = &dc_lightlist[i];
						rlight->height += rlight->heightstep;
						if (rlight->flags & FF_CUTLEVEL)
							rlight->botheight += rlight->botheightstep;
					}
					spryscale += rw_scalestep;
					continue;
				}

				dc_iscale = 0xffffffffu / (unsigned)spryscale;

				// draw the texture
				col = (column_t *)((UINT8 *)R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);

				for (i = 0; i < dc_numlights; i++)
				{
					// Check if the current light effects the colormap/lightlevel
					rlight = &dc_lightlist[i];
					lighteffect = !(dc_lightlist[i].flags & FF_NOSHADE);
					if (lighteffect)
					{
						lightnum = rlight->lightnum;

						if (lightnum < 0)
							xwalllights = scalelight[0];
						else if (lightnum >= LIGHTLEVELS)
							xwalllights = scalelight[LIGHTLEVELS-1];
						else
							xwalllights = scalelight[lightnum];

						pindex = spryscale>>LIGHTSCALESHIFT;

						if (pindex >=  MAXLIGHTSCALE)
							pindex = MAXLIGHTSCALE-1;

						if (pfloor->flags & FF_FOG)
						{
							if (pfloor->master->frontsector->extra_colormap)
								rlight->rcolormap = pfloor->master->frontsector->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
							else
								rlight->rcolormap = xwalllights[pindex];
						}
						else
						{
							if (rlight->extra_colormap)
								rlight->rcolormap = rlight->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
							else
								rlight->rcolormap = xwalllights[pindex];
						}
					}

					// Check if the current light can cut the current 3D floor.
					if (rlight->flags & FF_CUTSOLIDS && !(pfloor->flags & FF_EXTRA))
						solid = 1;
					else if (rlight->flags & FF_CUTEXTRA && pfloor->flags & FF_EXTRA)
					{
						if (rlight->flags & FF_EXTRA)
						{
							// The light is from an extra 3D floor... Check the flags so
							// there are no undesired cuts.
							if ((rlight->flags & (FF_FOG|FF_SWIMMABLE)) == (pfloor->flags & (FF_FOG|FF_SWIMMABLE)))
								solid = 1;
						}
						else
							solid = 1;
					}
					else
						solid = 0;

					rlight->height += rlight->heightstep;
					height = rlight->height;

					if (solid)
					{
						rlight->botheight += rlight->botheightstep;
						bheight = rlight->botheight - (FRACUNIT >> 1);
					}

					if (height <= windowtop)
					{
						if (lighteffect)
							dc_colormap = rlight->rcolormap;
						if (solid && windowtop < bheight)
							windowtop = bheight;
						continue;
					}

					windowbottom = height;
					if (windowbottom >= sprbotscreen)
					{
						windowbottom = sprbotscreen;
						colfunc_2s (col);
						for (i++; i < dc_numlights; i++)
						{
							rlight = &dc_lightlist[i];
							rlight->height += rlight->heightstep;
							if (rlight->flags & FF_CUTLEVEL)
								rlight->botheight += rlight->botheightstep;
						}
						continue;
					}
					colfunc_2s (col);
					if (solid)
						windowtop = bheight;
					else
						windowtop = windowbottom + 1;
					if (lighteffect)
						dc_colormap = rlight->rcolormap;
				}
				windowbottom = sprbotscreen;
				if (windowtop < windowbottom)
					colfunc_2s (col);

				spryscale += rw_scalestep;
				continue;
			}

			// calculate lighting
			pindex = spryscale>>LIGHTSCALESHIFT;

			if (pindex >= MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE - 1;

			dc_colormap = walllights[pindex];
			if (frontsector->extra_colormap)
				dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);
			if (pfloor->flags & FF_FOG && pfloor->master->frontsector->extra_colormap)
				dc_colormap = pfloor->master->frontsector->extra_colormap->colormap + (dc_colormap - colormaps);

			//Handle over/underflows before they happen.  This fixes the textures part of the FOF rendering bug.
			//...for the most part, anyway.
			if (((signed)dc_texturemid > 0 && (spryscale>>FRACBITS > INT32_MAX / (signed)dc_texturemid))
			 || ((signed)dc_texturemid < 0 && (spryscale) && (signed)(dc_texturemid)>>FRACBITS < (INT32_MIN / spryscale)))
			{
				spryscale += rw_scalestep;
				continue;
			}

			sprtopscreen = windowtop = (centeryfrac - FixedMul((dc_texturemid - offsetvalue), spryscale));
			sprbotscreen = windowbottom = FixedMul(*pfloor->topheight - *pfloor->bottomheight, spryscale) + sprtopscreen;
			dc_iscale = 0xffffffffu / (unsigned)spryscale;

			// draw the texture
			col = (column_t *)((UINT8 *)R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);

			colfunc_2s (col);
			spryscale += rw_scalestep;
		}
	}
	colfunc = wallcolfunc;
}

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTBITS              12
#define HEIGHTUNIT              (1<<HEIGHTBITS)


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
INT64 mycount;
INT64 mytotal = 0;
UINT32 nombre = 100000;
//static   char runtest[10][80];
#endif
//profile stuff ---------------------------------------------------------


static void R_RenderSegLoop (void)
{
	angle_t angle;
	size_t  pindex;
	INT32     yl;
	INT32     yh;

	INT32     mid;
	fixed_t texturecolumn = 0;
	INT32     top;
	INT32     bottom;
	INT32     i;

	for (; rw_x < rw_stopx; rw_x++)
	{
		// mark floor / ceiling areas
		yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

		// no space above wall?
		top = ceilingclip[rw_x]+1;

		// no space above wall?
		if (yl < top)
			yl = top;

		if (markceiling)
		{
			bottom = yl-1;

			if (bottom >= floorclip[rw_x])
				bottom = floorclip[rw_x]-1;

			if (top <= bottom)
			{
				ceilingplane->top[rw_x] = (INT16)top;
				ceilingplane->bottom[rw_x] = (INT16)bottom;
			}
		}


		yh = bottomfrac>>HEIGHTBITS;

		bottom = floorclip[rw_x]-1;

		if (yh > bottom)
			yh = bottom;

		if (markfloor)
		{
#if 0 // Old Doom Legacy code
			bottom = floorclip[rw_x]-1;
			if (top <= ceilingclip[rw_x])
				top = ceilingclip[rw_x]+1;
			if (top <= bottom && floorplane)
			{
				floorplane->top[rw_x] = (INT16)top;
				floorplane->bottom[rw_x] = (INT16)bottom;
			}
#else // Spiffy new PRBoom code
			top  = yh < ceilingclip[rw_x] ? ceilingclip[rw_x] : yh;

			if (++top <= bottom && floorplane)
			{
				floorplane->top[rw_x] = (INT16)top;
				floorplane->bottom[rw_x] = (INT16)bottom;
			}
#endif
		}

		if (numffloors)
		{
			firstseg->frontscale[rw_x] = frontscale[rw_x];
			top = ceilingclip[rw_x]+1; // PRBoom
			bottom = floorclip[rw_x]-1; // PRBoom

			for (i = 0; i < numffloors; i++)
			{
#ifdef POLYOBJECTS_PLANES
				if (ffloor[i].polyobj && (!curline->polyseg || ffloor[i].polyobj != curline->polyseg))
					continue;

				// FIXME hack to fix planes disappearing when a seg goes behind the camera. This NEEDS to be changed to be done properly. -Red
				if (curline->polyseg) {
					if (ffloor[i].plane->minx > rw_x)
						ffloor[i].plane->minx = rw_x;
					else if (ffloor[i].plane->maxx < rw_x)
						ffloor[i].plane->maxx = rw_x;
				}
#endif

				if (ffloor[i].height < viewz)
				{
					INT32 top_w = (ffloor[i].f_frac >> HEIGHTBITS) + 1;
					INT32 bottom_w = ffloor[i].f_clip[rw_x];

					if (top_w < top)
						top_w = top;

					if (bottom_w > bottom)
						bottom_w = bottom;

#ifdef POLYOBJECTS_PLANES
					// Polyobject-specific hack to fix plane leaking -Red
					if (curline->polyseg && ffloor[i].polyobj && ffloor[i].polyobj == curline->polyseg && top_w >= bottom_w) {
						ffloor[i].plane->top[rw_x] = ffloor[i].plane->bottom[rw_x] = 0xFFFF;
					} else
#endif

					if (top_w <= bottom_w)
					{
						ffloor[i].plane->top[rw_x] = (INT16)top_w;
						ffloor[i].plane->bottom[rw_x] = (INT16)bottom_w;
					}
				}
				else if (ffloor[i].height > viewz)
				{
					INT32 top_w = ffloor[i].c_clip[rw_x] + 1;
					INT32 bottom_w = (ffloor[i].f_frac >> HEIGHTBITS);

					if (top_w < top)
						top_w = top;

					if (bottom_w > bottom)
						bottom_w = bottom;

#ifdef POLYOBJECTS_PLANES
					// Polyobject-specific hack to fix plane leaking -Red
					if (curline->polyseg && ffloor[i].polyobj && ffloor[i].polyobj == curline->polyseg && top_w >= bottom_w) {
						ffloor[i].plane->top[rw_x] = ffloor[i].plane->bottom[rw_x] = 0xFFFF;
					} else
#endif

					if (top_w <= bottom_w)
					{
						ffloor[i].plane->top[rw_x] = (INT16)top_w;
						ffloor[i].plane->bottom[rw_x] = (INT16)bottom_w;
					}
				}
			}
		}

		//SoM: Calculate offsets for Thick fake floors.
		// calculate texture offset
		angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
		texturecolumn = rw_offset-FixedMul(FINETANGENT(angle),rw_distance);
		texturecolumn >>= FRACBITS;

		// texturecolumn and lighting are independent of wall tiers
		if (segtextured)
		{
			// calculate lighting
			pindex = FixedMul(rw_scale, FixedDiv(640, vid.width))>>LIGHTSCALESHIFT;

			if (pindex >=  MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE-1;

			dc_colormap = walllights[pindex];
			dc_x = rw_x;
			dc_iscale = 0xffffffffu / (unsigned)rw_scale;

			if (frontsector->extra_colormap)
				dc_colormap = frontsector->extra_colormap->colormap + (dc_colormap - colormaps);
		}

		if (dc_numlights)
		{
			lighttable_t **xwalllights;
			for (i = 0; i < dc_numlights; i++)
			{
				INT32 lightnum;
				lightnum = (dc_lightlist[i].lightlevel >> LIGHTSEGSHIFT);

				if (dc_lightlist[i].extra_colormap)
					;
				else if (curline->v1->y == curline->v2->y)
					lightnum--;
				else if (curline->v1->x == curline->v2->x)
					lightnum++;

				if (lightnum < 0)
					xwalllights = scalelight[0];
				else if (lightnum >= LIGHTLEVELS)
					xwalllights = scalelight[LIGHTLEVELS-1];
				else
					xwalllights = scalelight[lightnum];

				pindex = FixedMul(rw_scale, FixedDiv(640, vid.width))>>LIGHTSCALESHIFT;

				if (pindex >=  MAXLIGHTSCALE)
					pindex = MAXLIGHTSCALE-1;

				if (dc_lightlist[i].extra_colormap)
					dc_lightlist[i].rcolormap = dc_lightlist[i].extra_colormap->colormap + (xwalllights[pindex] - colormaps);
				else
					dc_lightlist[i].rcolormap = xwalllights[pindex];

				colfunc = R_DrawColumnShadowed_8;
			}
		}

		frontscale[rw_x] = rw_scale;

		// draw the wall tiers
		if (midtexture && yl <= yh && yh < vid.height && yh > 0)
		{
			// single sided line
			dc_yl = yl;
			dc_yh = yh;
			dc_texturemid = rw_midtexturemid;
			dc_source = R_GetColumn(midtexture,texturecolumn);
			dc_texheight = textureheight[midtexture]>>FRACBITS;

			//profile stuff ---------------------------------------------------------
#ifdef TIMING
			ProfZeroTimer();
#endif
			colfunc();
#ifdef TIMING
			RDMSR(0x10,&mycount);
			mytotal += mycount;      //64bit add

			if (nombre--==0)
				I_Error("R_DrawColumn CPU Spy reports: 0x%d %d\n", *((INT32 *)&mytotal+1),
					(INT32)mytotal);
#endif
			//profile stuff ---------------------------------------------------------

			// dont draw anything more for this column, since
			// a midtexture blocks the view
			ceilingclip[rw_x] = (INT16)viewheight;
			floorclip[rw_x] = -1;
		}
		else
		{
			// two sided line
			if (toptexture)
			{
				// top wall
				mid = pixhigh>>HEIGHTBITS;
				pixhigh += pixhighstep;

				if (mid >= floorclip[rw_x])
					mid = floorclip[rw_x]-1;

				if (mid >= yl && yh < vid.height && yh > 0)
				{
					dc_yl = yl;
					dc_yh = mid;
					dc_texturemid = rw_toptexturemid;
					dc_source = R_GetColumn(toptexture,texturecolumn);
					dc_texheight = textureheight[toptexture]>>FRACBITS;
					colfunc();
					ceilingclip[rw_x] = (INT16)mid;
				}
				else
					ceilingclip[rw_x] = (INT16)((INT16)yl - 1);
			}
			else if (markceiling) // no top wall
				ceilingclip[rw_x] = (INT16)((INT16)yl - 1);

			if (bottomtexture)
			{
				// bottom wall
				mid = (pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
				pixlow += pixlowstep;

				// no space above wall?
				if (mid <= ceilingclip[rw_x])
					mid = ceilingclip[rw_x]+1;

				if (mid <= yh && yh < vid.height && yh > 0)
				{
					dc_yl = mid;
					dc_yh = yh;
					dc_texturemid = rw_bottomtexturemid;
					dc_source = R_GetColumn(bottomtexture,
						texturecolumn);
					dc_texheight = textureheight[bottomtexture]>>FRACBITS;
					colfunc();
					floorclip[rw_x] = (INT16)mid;
				}
				else
					floorclip[rw_x] = (INT16)((INT16)yh + 1);
			}
			else if (markfloor) // no bottom wall
				floorclip[rw_x] = (INT16)((INT16)yh + 1);
		}

		if (maskedtexture || numthicksides)
		{
			// save texturecol
			//  for backdrawing of masked mid texture
			maskedtexturecol[rw_x] = (INT16)texturecolumn;
		}

		if (dc_numlights)
		{
			for (i = 0; i < dc_numlights; i++)
			{
				dc_lightlist[i].height += dc_lightlist[i].heightstep;
				if (dc_lightlist[i].flags & FF_SOLID)
					dc_lightlist[i].botheight += dc_lightlist[i].botheightstep;
			}
		}

		for (i = 0; i < numffloors; i++)
		{
#ifdef POLYOBJECTS_PLANES
			if (ffloor[i].polyobj && (!curline->polyseg || ffloor[i].polyobj != curline->polyseg))
				continue;
#endif

			ffloor[i].f_frac += ffloor[i].f_step;
		}

		for (i = 0; i < numbackffloors; i++)
		{
			INT32 y_w;

#ifdef POLYOBJECTS_PLANES
			if (ffloor[i].polyobj && (!curline->polyseg || ffloor[i].polyobj != curline->polyseg))
				continue;
#endif
			y_w = ffloor[i].b_frac >> HEIGHTBITS;

			ffloor[i].f_clip[rw_x] = ffloor[i].c_clip[rw_x] = (INT16)(y_w & 0xFFFF);
			ffloor[i].b_frac += ffloor[i].b_step;
		}

		rw_scale += rw_scalestep;
		topfrac += topstep;
		bottomfrac += bottomstep;
	}
}

//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange(INT32 start, INT32 stop)
{
	fixed_t       hyp;
	fixed_t       sineval;
	angle_t       distangle, offsetangle;
	fixed_t       vtop;
	INT32           lightnum;
	INT32           i, p;
	lightlist_t   *light;
	r_lightlist_t *rlight;
#ifdef ESLOPE
	vertex_t segleft, segright;
#endif
	static size_t maxdrawsegs = 0;

	if (ds_p == drawsegs+maxdrawsegs)
	{
		size_t pos = ds_p - drawsegs;
		size_t pos2 = firstnewseg - drawsegs;
		size_t newmax = maxdrawsegs ? maxdrawsegs*2 : 128;
		if (firstseg)
			firstseg = (drawseg_t *)(firstseg - drawsegs);
		drawsegs = Z_Realloc(drawsegs, newmax*sizeof (*drawsegs), PU_STATIC, NULL);
		ds_p = drawsegs + pos;
		firstnewseg = drawsegs + pos2;
		maxdrawsegs = newmax;
		if (firstseg)
			firstseg = drawsegs + (size_t)firstseg;
	}

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// calculate rw_distance for scale calculation
	rw_normalangle = curline->angle + ANGLE_90;
	offsetangle = abs((INT32)(rw_normalangle-rw_angle1));

	if (offsetangle > ANGLE_90)
		offsetangle = ANGLE_90;

	distangle = ANGLE_90 - offsetangle;
	hyp = R_PointToDist (curline->v1->x, curline->v1->y);
	sineval = FINESINE(distangle>>ANGLETOFINESHIFT);
	rw_distance = FixedMul (hyp, sineval);


	ds_p->x1 = rw_x = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;
	rw_stopx = stop+1;

	//SoM: Code to remove limits on openings.
	{
		size_t pos = lastopening - openings;
		size_t need = (rw_stopx - start)*4 + pos;
		if (need > maxopenings)
		{
			drawseg_t *ds;  //needed for fix from *cough* zdoom *cough*
			INT16 *oldopenings = openings;
			INT16 *oldlast = lastopening;

			do
				maxopenings = maxopenings ? maxopenings*2 : 16384;
			while (need > maxopenings);
			openings = Z_Realloc(openings, maxopenings * sizeof (*openings), PU_STATIC, NULL);
			lastopening = openings + pos;

			// borrowed fix from *cough* zdoom *cough*
			// [RH] We also need to adjust the openings pointers that
			//    were already stored in drawsegs.
			for (ds = drawsegs; ds < ds_p; ds++)
			{
#define ADJUST(p) if (ds->p + ds->x1 >= oldopenings && ds->p + ds->x1 <= oldlast) ds->p = ds->p - oldopenings + openings;
				ADJUST(maskedtexturecol);
				ADJUST(sprtopclip);
				ADJUST(sprbottomclip);
				ADJUST(thicksidecol);
#undef ADJUST
			}
		}
	}  // end of code to remove limits on openings

	// calculate scale at both ends and step
	ds_p->scale1 = rw_scale = R_ScaleFromGlobalAngle(viewangle + xtoviewangle[start]);

	if (stop > start)
	{
		ds_p->scale2 = R_ScaleFromGlobalAngle(viewangle + xtoviewangle[stop]);
		ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (stop-start);
	}
	else
	{
		// UNUSED: try to fix the stretched line bug
#if 0
		if (rw_distance < FRACUNIT/2)
		{
			fixed_t         tr_x,tr_y;
			fixed_t         gxt,gyt;
			CONS_Debug(DBG_RENDER, "TRYING TO FIX THE STRETCHED ETC\n");

			tr_x = curline->v1->x - viewx;
			tr_y = curline->v1->y - viewy;

			gxt = FixedMul(tr_x, viewcos);
			gyt = -FixedMul(tr_y, viewsin);
			ds_p->scale1 = FixedDiv(projection, gxt - gyt);
		}
#endif
		ds_p->scale2 = ds_p->scale1;
	}

	// calculate texture boundaries
	//  and decide if floor / ceiling marks are needed
#ifdef ESLOPE
	// Figure out map coordinates of where start and end are mapping to on seg, so we can clip right for slope bullshit
	if (frontsector->c_slope || frontsector->f_slope || (backsector && (backsector->c_slope || backsector->f_slope))) {
		angle_t temp;
		fixed_t tan;

		// left
		temp = xtoviewangle[start]+viewangle;

		if (curline->v1->x == curline->v2->x) {
			// Line seg is vertical, so no line-slope form for it
			tan = FINETANGENT((temp+ANGLE_90)>>ANGLETOFINESHIFT);

			segleft.x = curline->v1->x;

			segleft.y = curline->v1->y-FixedMul(viewx-segleft.x, tan);
		} else if (temp>>ANGLETOFINESHIFT == ANGLE_90>>ANGLETOFINESHIFT || temp>>ANGLETOFINESHIFT == ANGLE_270>>ANGLETOFINESHIFT) {
			// Same problem as above, except this time with the view angle
			tan = FixedDiv(curline->v2->y-curline->v1->y, curline->v2->x-curline->v1->x);

			segleft.x = viewx;
			segleft.y = curline->v1->y-FixedMul(viewx-curline->v1->x, tan);
		} else {
			// Both lines can be written in slope-intercept form, so figure out line intersection
			float a1, b1, c1, a2, b2, c2, det; // 1 is the seg, 2 is the view angle vector...

			a1 = FIXED_TO_FLOAT(curline->v2->y-curline->v1->y);
			b1 = FIXED_TO_FLOAT(curline->v1->x-curline->v2->x);
			c1 = a1*FIXED_TO_FLOAT(curline->v1->x) + b1*FIXED_TO_FLOAT(curline->v1->y);

			a2 = -FIXED_TO_FLOAT(FINESINE(temp>>ANGLETOFINESHIFT));
			b2 = FIXED_TO_FLOAT(FINECOSINE(temp>>ANGLETOFINESHIFT));
			c2 = a2*FIXED_TO_FLOAT(viewx) + b2*FIXED_TO_FLOAT(viewy);

			det = a1*b2 - a2*b1;

			segleft.x = FLOAT_TO_FIXED((b2*c1 - b1*c2)/det);
			segleft.y = FLOAT_TO_FIXED((a1*c2 - a2*c1)/det);
		}

		// right
		temp = xtoviewangle[stop]+viewangle;

		if (curline->v1->x == curline->v2->x) {
			// Line seg is vertical, so no line-slope form for it
			tan = FINETANGENT((temp+ANGLE_90)>>ANGLETOFINESHIFT);

			segright.x = curline->v1->x;

			segright.y = curline->v1->y-FixedMul(viewx-segright.x, tan);
		} else if (temp>>ANGLETOFINESHIFT == ANGLE_90>>ANGLETOFINESHIFT || temp>>ANGLETOFINESHIFT == ANGLE_270>>ANGLETOFINESHIFT) {
			// Same problem as above, except this time with the view angle
			tan = FixedDiv(curline->v2->y-curline->v1->y, curline->v2->x-curline->v1->x);

			segright.x = viewx;
			segright.y = curline->v1->y-FixedMul(viewx-curline->v1->x, tan);
		} else {
			// Both lines can be written in slope-intercept form, so figure out line intersection
			float a1, b1, c1, a2, b2, c2, det; // 1 is the seg, 2 is the view angle vector...

			a1 = FIXED_TO_FLOAT(curline->v2->y-curline->v1->y);
			b1 = FIXED_TO_FLOAT(curline->v1->x-curline->v2->x);
			c1 = a1*FIXED_TO_FLOAT(curline->v1->x) + b1*FIXED_TO_FLOAT(curline->v1->y);

			a2 = -FIXED_TO_FLOAT(FINESINE(temp>>ANGLETOFINESHIFT));
			b2 = FIXED_TO_FLOAT(FINECOSINE(temp>>ANGLETOFINESHIFT));
			c2 = a2*FIXED_TO_FLOAT(viewx) + b2*FIXED_TO_FLOAT(viewy);

			det = a1*b2 - a2*b1;

			segright.x = FLOAT_TO_FIXED((b2*c1 - b1*c2)/det);
			segright.y = FLOAT_TO_FIXED((a1*c2 - a2*c1)/det);
		}
	}

	if (frontsector->c_slope) {
		worldtop = P_GetZAt(frontsector->c_slope, segleft.x, segleft.y) - viewz;
		worldtopslope = P_GetZAt(frontsector->c_slope, segright.x, segright.y) - viewz;
	} else {
		worldtopslope =
#else
	{
#endif
		worldtop = frontsector->ceilingheight - viewz;
	}


#ifdef ESLOPE
	if (frontsector->f_slope) {
		worldbottom = P_GetZAt(frontsector->f_slope, segleft.x, segleft.y) - viewz;
		worldbottomslope = P_GetZAt(frontsector->f_slope, segright.x, segright.y) - viewz;
	} else {
		worldbottomslope =
#else
	{
#endif
		worldbottom = frontsector->floorheight - viewz;
	}

	midtexture = toptexture = bottomtexture = maskedtexture = 0;
	ds_p->maskedtexturecol = NULL;
	ds_p->numthicksides = numthicksides = 0;
	ds_p->thicksidecol = NULL;
	ds_p->tsilheight = 0;

	numbackffloors = 0;

	for (i = 0; i < MAXFFLOORS; i++)
		ds_p->thicksides[i] = NULL;

	if (numffloors)
	{
		for (i = 0; i < numffloors; i++)
		{
#ifdef POLYOBJECTS_PLANES
			if (ffloor[i].polyobj && (!ds_p->curline->polyseg || ffloor[i].polyobj != ds_p->curline->polyseg))
				continue;
#endif
			ffloor[i].f_pos = ffloor[i].height - viewz;
		}
	}

	if (!backsector)
	{
		// single sided line
		midtexture = texturetranslation[sidedef->midtexture];
		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = true;

		if (linedef->flags & ML_DONTPEGBOTTOM)
		{
			vtop = frontsector->floorheight + textureheight[sidedef->midtexture];
			// bottom of texture at bottom
			rw_midtexturemid = vtop - viewz;
		}
		else
		{
			// top of texture at top
			rw_midtexturemid = worldtop;
		}
		rw_midtexturemid += sidedef->rowoffset;

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = screenheightarray;
		ds_p->sprbottomclip = negonearray;
		ds_p->bsilheight = INT32_MAX;
		ds_p->tsilheight = INT32_MIN;
	}
	else
	{
		// two sided line
		ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
		ds_p->silhouette = 0;

		if (frontsector->floorheight > backsector->floorheight)
		{
			ds_p->silhouette = SIL_BOTTOM;
			ds_p->bsilheight = frontsector->floorheight;
		}
		else if (backsector->floorheight > viewz)
		{
			ds_p->silhouette = SIL_BOTTOM;
			ds_p->bsilheight = INT32_MAX;
			// ds_p->sprbottomclip = negonearray;
		}

		if (frontsector->ceilingheight < backsector->ceilingheight)
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = frontsector->ceilingheight;
		}
		else if (backsector->ceilingheight < viewz)
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = INT32_MIN;
			// ds_p->sprtopclip = screenheightarray;
		}

		if (backsector->ceilingheight <= frontsector->floorheight)
		{
			ds_p->sprbottomclip = negonearray;
			ds_p->bsilheight = INT32_MAX;
			ds_p->silhouette |= SIL_BOTTOM;
		}

		if (backsector->floorheight >= frontsector->ceilingheight)
		{
			ds_p->sprtopclip = screenheightarray;
			ds_p->tsilheight = INT32_MIN;
			ds_p->silhouette |= SIL_TOP;
		}

		//SoM: 3/25/2000: This code fixes an automap bug that didn't check
		// frontsector->ceiling and backsector->floor to see if a door was closed.
		// Without the following code, sprites get displayed behind closed doors.
		{
			if (doorclosed || backsector->ceilingheight <= frontsector->floorheight)
			{
				ds_p->sprbottomclip = negonearray;
				ds_p->bsilheight = INT32_MAX;
				ds_p->silhouette |= SIL_BOTTOM;
			}
			if (doorclosed || backsector->floorheight >= frontsector->ceilingheight)
			{                   // killough 1/17/98, 2/8/98
				ds_p->sprtopclip = screenheightarray;
				ds_p->tsilheight = INT32_MIN;
				ds_p->silhouette |= SIL_TOP;
			}
		}

#ifdef ESLOPE
		if (backsector->c_slope) {
			worldhigh = P_GetZAt(backsector->c_slope, segleft.x, segleft.y) - viewz;
			worldhighslope = P_GetZAt(backsector->c_slope, segright.x, segright.y) - viewz;
		} else {
			worldhighslope =
#else
		{
#endif
			worldhigh = backsector->ceilingheight - viewz;
		}


#ifdef ESLOPE
		if (backsector->f_slope) {
			worldlow = P_GetZAt(backsector->f_slope, segleft.x, segleft.y) - viewz;
			worldlowslope = P_GetZAt(backsector->f_slope, segright.x, segright.y) - viewz;
		} else {
			worldlowslope =
#else
		{
#endif
			worldlow = backsector->floorheight - viewz;
		}


		// hack to allow height changes in outdoor areas
		if (frontsector->ceilingpic == skyflatnum
			&& backsector->ceilingpic == skyflatnum)
		{
#ifdef ESLOPE
			worldtopslope = worldhighslope =
#endif
			worldtop = worldhigh;
		}

		if (worldlow != worldbottom
#ifdef ESLOPE
			|| worldlowslope != worldbottomslope
#endif
		    || backsector->floorpic != frontsector->floorpic
		    || backsector->lightlevel != frontsector->lightlevel
		    //SoM: 3/22/2000: Check floor x and y offsets.
		    || backsector->floor_xoffs != frontsector->floor_xoffs
		    || backsector->floor_yoffs != frontsector->floor_yoffs
		    || backsector->floorpic_angle != frontsector->floorpic_angle
		    //SoM: 3/22/2000: Prevents bleeding.
		    || frontsector->heightsec != -1
		    || backsector->floorlightsec != frontsector->floorlightsec
		    //SoM: 4/3/2000: Check for colormaps
		    || frontsector->extra_colormap != backsector->extra_colormap
		    || (frontsector->ffloors != backsector->ffloors && frontsector->tag != backsector->tag))
		{
			markfloor = true;
		}
		else
		{
			// same plane on both sides
			markfloor = false;
		}

		if (worldhigh != worldtop
#ifdef ESLOPE
			|| worldhighslope != worldtopslope
#endif
		    || backsector->ceilingpic != frontsector->ceilingpic
		    || backsector->lightlevel != frontsector->lightlevel
		    //SoM: 3/22/2000: Check floor x and y offsets.
		    || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
		    || backsector->ceiling_yoffs != frontsector->ceiling_yoffs
		    || backsector->ceilingpic_angle != frontsector->ceilingpic_angle
		    //SoM: 3/22/2000: Prevents bleeding.
		    || (frontsector->heightsec != -1 && frontsector->ceilingpic != skyflatnum)
		    || backsector->floorlightsec != frontsector->floorlightsec
		    //SoM: 4/3/2000: Check for colormaps
		    || frontsector->extra_colormap != backsector->extra_colormap
		    || (frontsector->ffloors != backsector->ffloors && frontsector->tag != backsector->tag))
		{
				markceiling = true;
		}
		else
		{
			// same plane on both sides
			markceiling = false;
		}

		if (backsector->ceilingheight <= frontsector->floorheight ||
		    backsector->floorheight >= frontsector->ceilingheight)
		{
			// closed door
			markceiling = markfloor = true;
		}

		// check TOP TEXTURE
		if (worldhigh < worldtop
#ifdef ESLOPE
				|| worldhighslope < worldtopslope
#endif
			)
		{
			// top texture
			if ((linedef->flags & (ML_DONTPEGTOP) && (linedef->flags & ML_DONTPEGBOTTOM))
				&& linedef->sidenum[1] != 0xffff)
			{
				// Special case... use offsets from 2nd side but only if it has a texture.
				side_t *def = &sides[linedef->sidenum[1]];
				toptexture = texturetranslation[def->toptexture];

				if (!toptexture) //Second side has no texture, use the first side's instead.
					toptexture = texturetranslation[sidedef->toptexture];

				if (linedef->flags & ML_DONTPEGTOP)
				{
					// top of texture at top
					rw_toptexturemid = worldtop;
				}
				else
				{
					vtop = backsector->ceilingheight + textureheight[def->toptexture];
					// bottom of texture
					rw_toptexturemid = vtop - viewz;
				}
			}
			else
			{
				toptexture = texturetranslation[sidedef->toptexture];

				if (linedef->flags & ML_DONTPEGTOP)
				{
					// top of texture at top
					rw_toptexturemid = worldtop;
				}
				else
				{
					vtop = backsector->ceilingheight + textureheight[sidedef->toptexture];
					// bottom of texture
					rw_toptexturemid = vtop - viewz;
				}
			}
		}
		// check BOTTOM TEXTURE
		if (worldlow > worldbottom
#ifdef ESLOPE
				|| worldlowslope > worldbottomslope
#endif
			)     //seulement si VISIBLE!!!
		{
			// bottom texture
			bottomtexture = texturetranslation[sidedef->bottomtexture];

			if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom
				// top of texture at top
				rw_bottomtexturemid = worldtop;
			}
			else    // top of texture at top
				rw_bottomtexturemid = worldlow;
		}

		rw_toptexturemid += sidedef->rowoffset;
		rw_bottomtexturemid += sidedef->rowoffset;

		// allocate space for masked texture tables
		if (frontsector && backsector && frontsector->tag != backsector->tag && (backsector->ffloors || frontsector->ffloors))
		{
			ffloor_t *rover;
			ffloor_t *r2;
			fixed_t   lowcut, highcut;

			//markceiling = markfloor = true;
			maskedtexture = true;

			ds_p->thicksidecol = maskedtexturecol = lastopening - rw_x;
			lastopening += rw_stopx - rw_x;

			lowcut = frontsector->floorheight > backsector->floorheight ? frontsector->floorheight : backsector->floorheight;
			highcut = frontsector->ceilingheight < backsector->ceilingheight ? frontsector->ceilingheight : backsector->ceilingheight;

			if (frontsector->ffloors && backsector->ffloors)
			{
				i = 0;
				for (rover = backsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS))
						continue;
					if (rover->flags & FF_INVERTSIDES)
						continue;
					if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
						continue;

					if (rover->norender == leveltime)
						continue;

					for (r2 = frontsector->ffloors; r2; r2 = r2->next)
					{
						if (!(r2->flags & FF_EXISTS) || !(r2->flags & FF_RENDERSIDES)
						    || *r2->topheight < lowcut || *r2->bottomheight > highcut)
							continue;

						if (r2->norender == leveltime)
							continue;

						if (rover->flags & FF_EXTRA)
						{
							if (!(r2->flags & FF_CUTEXTRA))
								continue;

							if (r2->flags & FF_EXTRA && (r2->flags & (FF_TRANSLUCENT|FF_FOG)) != (rover->flags & (FF_TRANSLUCENT|FF_FOG)))
								continue;
						}
						else
						{
							if (!(r2->flags & FF_CUTSOLIDS))
								continue;
						}

						if (*rover->topheight > *r2->topheight || *rover->bottomheight < *r2->bottomheight)
							continue;

						break;
					}
					if (r2)
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}

				for (rover = frontsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS))
						continue;
					if (!(rover->flags & FF_ALLSIDES))
						continue;
					if (*rover->topheight < lowcut || *rover->bottomheight > highcut)
						continue;

					if (rover->norender == leveltime)
						continue;

					for (r2 = backsector->ffloors; r2; r2 = r2->next)
					{
						if (!(r2->flags & FF_EXISTS) || !(r2->flags & FF_RENDERSIDES)
						    || *r2->topheight < lowcut || *r2->bottomheight > highcut)
							continue;

						if (r2->norender == leveltime)
							continue;

						if (rover->flags & FF_EXTRA)
						{
							if (!(r2->flags & FF_CUTEXTRA))
								continue;

							if (r2->flags & FF_EXTRA && (r2->flags & (FF_TRANSLUCENT|FF_FOG)) != (rover->flags & (FF_TRANSLUCENT|FF_FOG)))
								continue;
						}
						else
						{
							if (!(r2->flags & FF_CUTSOLIDS))
								continue;
						}

						if (*rover->topheight > *r2->topheight || *rover->bottomheight < *r2->bottomheight)
							continue;

						break;
					}
					if (r2)
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}
			else if (backsector->ffloors)
			{
				for (rover = backsector->ffloors, i = 0; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS) || rover->flags & FF_INVERTSIDES)
						continue;
					if (*rover->topheight <= frontsector->floorheight || *rover->bottomheight >= frontsector->ceilingheight)
						continue;
					if (rover->norender == leveltime)
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}
			else if (frontsector->ffloors)
			{
				for (rover = frontsector->ffloors, i = 0; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS) || !(rover->flags & FF_ALLSIDES))
						continue;
					if (*rover->topheight <= frontsector->floorheight || *rover->bottomheight >= frontsector->ceilingheight)
						continue;
					if (*rover->topheight <= backsector->floorheight || *rover->bottomheight >= backsector->ceilingheight)
						continue;
					if (rover->norender == leveltime)
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}

			ds_p->numthicksides = numthicksides = i;
		}
		if (sidedef->midtexture)
		{
			// masked midtexture
			if (!ds_p->thicksidecol)
			{
				ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
				lastopening += rw_stopx - rw_x;
			}
			else
				ds_p->maskedtexturecol = ds_p->thicksidecol;

			maskedtexture = true;
		}
	}

	// calculate rw_offset (only needed for textured lines)
	segtextured = midtexture || toptexture || bottomtexture || maskedtexture || (numthicksides > 0);

	if (segtextured)
	{
		offsetangle = rw_normalangle-rw_angle1;

		if (offsetangle > ANGLE_180)
			offsetangle = -(signed)offsetangle;

		if (offsetangle > ANGLE_90)
			offsetangle = ANGLE_90;

		sineval = FINESINE(offsetangle >>ANGLETOFINESHIFT);
		rw_offset = FixedMul (hyp, sineval);

		if (rw_normalangle-rw_angle1 < ANGLE_180)
			rw_offset = -rw_offset;

		/// don't use texture offset for splats
		rw_offset2 = rw_offset + curline->offset;
		rw_offset += sidedef->textureoffset + curline->offset;
		rw_centerangle = ANGLE_90 + viewangle - rw_normalangle;

		// calculate light table
		//  use different light tables
		//  for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);

		if (curline->v1->y == curline->v2->y)
			lightnum--;
		else if (curline->v1->x == curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			walllights = scalelight[LIGHTLEVELS - 1];
		else
			walllights = scalelight[lightnum];
	}

	// if a floor / ceiling plane is on the wrong side
	//  of the view plane, it is definitely invisible
	//  and doesn't need to be marked.
	if (frontsector->heightsec == -1)
	{
		if (frontsector->floorheight >= viewz)
		{
			// above view plane
			markfloor = false;
		}

		if (frontsector->ceilingheight <= viewz &&
		    frontsector->ceilingpic != skyflatnum)
		{
			// below view plane
			markceiling = false;
		}
	}

	// calculate incremental stepping values for texture edges
	worldtop >>= 4;
	worldbottom >>= 4;
#ifdef ESLOPE
	worldtopslope >>= 4;
	worldbottomslope >>= 4;
#endif

	topstep = -FixedMul (rw_scalestep, worldtop);
	topfrac = (centeryfrac>>4) - FixedMul (worldtop, rw_scale);

	bottomstep = -FixedMul (rw_scalestep,worldbottom);
	bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, rw_scale);

#ifdef ESLOPE
	if (frontsector->c_slope) {
		fixed_t topfracend = (centeryfrac>>4) - FixedMul (worldtopslope, ds_p->scale2);
		topstep = (topfracend-topfrac)/(stop-start+1);
	}
	if (frontsector->f_slope) {
		fixed_t bottomfracend = (centeryfrac>>4) - FixedMul (worldbottomslope, ds_p->scale2);
		bottomstep = (bottomfracend-bottomfrac)/(stop-start+1);
	}
#endif

	dc_numlights = 0;

	if (frontsector->numlights)
	{
		dc_numlights = frontsector->numlights;
		if (dc_numlights >= dc_maxlights)
		{
			dc_maxlights = dc_numlights;
			dc_lightlist = Z_Realloc(dc_lightlist, sizeof (*dc_lightlist) * dc_maxlights, PU_STATIC, NULL);
		}

		for (i = p = 0; i < dc_numlights; i++)
		{
			light = &frontsector->lightlist[i];
			rlight = &dc_lightlist[p];

			if (i != 0)
			{
				if (light->height < frontsector->floorheight)
					continue;

				if (light->height > frontsector->ceilingheight && i+1 < dc_numlights && frontsector->lightlist[i+1].height > frontsector->ceilingheight)
					continue;
			}

			rlight->height = (centeryfrac>>4) - FixedMul((light->height - viewz) >> 4, rw_scale);
			rlight->heightstep = -FixedMul (rw_scalestep, (light->height - viewz) >> 4);
			rlight->flags = light->flags;

			if (light->caster && light->caster->flags & FF_SOLID)
			{
				rlight->botheight = (centeryfrac >> 4) - FixedMul((*light->caster->bottomheight - viewz) >> 4, rw_scale);
				rlight->botheightstep = -FixedMul (rw_scalestep, (*light->caster->bottomheight - viewz) >> 4);
			}

			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = light->extra_colormap;
			p++;
		}

		dc_numlights = p;
	}

	if (numffloors)
	{
		for (i = 0; i < numffloors; i++)
		{
#ifdef POLYOBJECTS_PLANES
			if (ffloor[i].polyobj && (!curline->polyseg || ffloor[i].polyobj != curline->polyseg))
				continue;
#endif

			ffloor[i].f_pos >>= 4;
			ffloor[i].f_step = FixedMul(-rw_scalestep, ffloor[i].f_pos);
			ffloor[i].f_frac = (centeryfrac>>4) - FixedMul(ffloor[i].f_pos, rw_scale);
		}
	}

	if (backsector)
	{
		worldhigh >>= 4;
		worldlow >>= 4;
#ifdef ESLOPE
		worldhighslope >>= 4;
		worldlowslope >>= 4;
#endif

		if (worldhigh < worldtop
#ifdef ESLOPE
			|| worldhighslope < worldtopslope
#endif
			)
		{
			pixhigh = (centeryfrac>>4) - FixedMul (worldhigh, rw_scale);
			pixhighstep = -FixedMul (rw_scalestep,worldhigh);

#ifdef ESLOPE
			if (backsector->c_slope) {
				fixed_t topfracend = (centeryfrac>>4) - FixedMul (worldhighslope, ds_p->scale2);
				pixhighstep = (topfracend-pixhigh)/(stop-start+1);
			}
#endif
		}

		if (worldlow > worldbottom
#ifdef ESLOPE
			|| worldlowslope > worldbottomslope
#endif
			)
		{
			pixlow = (centeryfrac>>4) - FixedMul (worldlow, rw_scale);
			pixlowstep = -FixedMul (rw_scalestep,worldlow);
#ifdef ESLOPE
			if (backsector->f_slope) {
				fixed_t bottomfracend = (centeryfrac>>4) - FixedMul (worldlowslope, ds_p->scale2);
				pixlowstep = (bottomfracend-pixlow)/(stop-start+1);
			}
#endif
		}

		{
			ffloor_t * rover;
			i = 0;

			if (backsector->ffloors)
			{
				for (rover = backsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
						continue;
					if (rover->norender == leveltime)
						continue;

					if (*rover->bottomheight <= backsector->ceilingheight &&
					    *rover->bottomheight >= backsector->floorheight &&
					    ((viewz < *rover->bottomheight && !(rover->flags & FF_INVERTPLANES)) ||
					     (viewz > *rover->bottomheight && (rover->flags & FF_BOTHPLANES))))
					{
						ffloor[i].b_pos = *rover->bottomheight;
						ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
						ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						i++;
					}
					if (i >= MAXFFLOORS)
						break;
					if (*rover->topheight >= backsector->floorheight &&
					    *rover->topheight <= backsector->ceilingheight &&
					    ((viewz > *rover->topheight && !(rover->flags & FF_INVERTPLANES)) ||
					     (viewz < *rover->topheight && (rover->flags & FF_BOTHPLANES))))
					{
						ffloor[i].b_pos = *rover->topheight;
						ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
						ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						i++;
					}
				}
			}
			else if (frontsector && frontsector->ffloors)
			{
				for (rover = frontsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
						continue;
					if (rover->norender == leveltime)
						continue;

					if (*rover->bottomheight <= frontsector->ceilingheight &&
					    *rover->bottomheight >= frontsector->floorheight &&
					    ((viewz < *rover->bottomheight && !(rover->flags & FF_INVERTPLANES)) ||
					     (viewz > *rover->bottomheight && (rover->flags & FF_BOTHPLANES))))
					{
						ffloor[i].b_pos = *rover->bottomheight;
						ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
						ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						i++;
					}
					if (i >= MAXFFLOORS)
						break;
					if (*rover->topheight >= frontsector->floorheight &&
					    *rover->topheight <= frontsector->ceilingheight &&
					    ((viewz > *rover->topheight && !(rover->flags & FF_INVERTPLANES)) ||
					     (viewz < *rover->topheight && (rover->flags & FF_BOTHPLANES))))
					{
						ffloor[i].b_pos = *rover->topheight;
						ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
						ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
						ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
						i++;
					}
				}
			}
#ifdef POLYOBJECTS_PLANES
			if (curline->polyseg && frontsector && (curline->polyseg->flags & POF_RENDERPLANES))
			{
				while (i < numffloors && ffloor[i].polyobj != curline->polyseg) i++;
				if (i < numffloors && backsector->floorheight <= frontsector->ceilingheight &&
					backsector->floorheight >= frontsector->floorheight &&
					(viewz < backsector->floorheight))
				{
					if (ffloor[i].plane->minx > ds_p->x1)
						ffloor[i].plane->minx = ds_p->x1;

					if (ffloor[i].plane->maxx < ds_p->x2)
						ffloor[i].plane->maxx = ds_p->x2;

					ffloor[i].b_pos = backsector->floorheight;
					ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
					ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
					ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
					i++;
				}
				if (i < numffloors && backsector->ceilingheight >= frontsector->floorheight &&
					backsector->ceilingheight <= frontsector->ceilingheight &&
					(viewz > backsector->ceilingheight))
				{
					if (ffloor[i].plane->minx > ds_p->x1)
						ffloor[i].plane->minx = ds_p->x1;

					if (ffloor[i].plane->maxx < ds_p->x2)
						ffloor[i].plane->maxx = ds_p->x2;

					ffloor[i].b_pos = backsector->ceilingheight;
					ffloor[i].b_pos = (ffloor[i].b_pos - viewz) >> 4;
					ffloor[i].b_step = FixedMul(-rw_scalestep, ffloor[i].b_pos);
					ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(ffloor[i].b_pos, rw_scale);
					i++;
				}
			}
#endif

			numbackffloors = i;
		}
	}

	// get a new or use the same visplane
	if (markceiling)
	{
		if (ceilingplane) //SoM: 3/29/2000: Check for null ceiling planes
			ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
		else
			markceiling = 0;
	}

	// get a new or use the same visplane
	if (markfloor)
	{
		if (floorplane) //SoM: 3/29/2000: Check for null planes
			floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);
		else
			markfloor = 0;
	}

	ds_p->numffloorplanes = 0;
	if (numffloors)
	{
		if (!firstseg)
		{
			ds_p->numffloorplanes = numffloors;

			for (i = 0; i < numffloors; i++)
			{
				ds_p->ffloorplanes[i] = ffloor[i].plane =
					R_CheckPlane(ffloor[i].plane, rw_x, rw_stopx - 1);
			}

			firstseg = ds_p;
		}
		else
		{
			for (i = 0; i < numffloors; i++)
				R_ExpandPlane(ffloor[i].plane, rw_x, rw_stopx - 1);
		}
	}

#ifdef WALLSPLATS
	if (linedef->splats && cv_splats.value)
	{
		// Isn't a bit wasteful to copy the ENTIRE array for every drawseg?
		M_Memcpy(last_ceilingclip + ds_p->x1, ceilingclip + ds_p->x1,
			sizeof (INT16) * (ds_p->x2 - ds_p->x1 + 1));
		M_Memcpy(last_floorclip + ds_p->x1, floorclip + ds_p->x1,
			sizeof (INT16) * (ds_p->x2 - ds_p->x1 + 1));
		R_RenderSegLoop();
		R_DrawWallSplats();
	}
	else
#endif
		R_RenderSegLoop();
	colfunc = wallcolfunc;

	// save sprite clipping info
	if (((ds_p->silhouette & SIL_TOP) || maskedtexture) && !ds_p->sprtopclip)
	{
		M_Memcpy(lastopening, ceilingclip+start, 2*(rw_stopx - start));
		ds_p->sprtopclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

	if (((ds_p->silhouette & SIL_BOTTOM) || maskedtexture) && !ds_p->sprbottomclip)
	{
		M_Memcpy(lastopening, floorclip + start, 2*(rw_stopx-start));
		ds_p->sprbottomclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

	if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
	{
		ds_p->silhouette |= SIL_TOP;
		ds_p->tsilheight = sidedef->midtexture ? INT32_MIN: INT32_MAX;
	}
	if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
	{
		ds_p->silhouette |= SIL_BOTTOM;
		ds_p->bsilheight = sidedef->midtexture ? INT32_MAX: INT32_MIN;
	}
	ds_p++;
}
