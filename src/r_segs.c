// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
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

#include "r_portal.h"
#include "r_splats.h"

#include "w_wad.h"
#include "z_zone.h"
#include "d_netcmd.h"
#include "m_misc.h"
#include "p_local.h" // Camera...
#include "p_slopes.h"
#include "console.h" // con_clipviewtop
#include "taglist.h"

// OPTIMIZE: closed two sided lines as single sided

typedef struct segloopcontext_s
{
	INT32 x, stopx;

	// True if any of the segs textures might be visible.
	boolean segtextured;
	boolean markfloor; // False if the back side is the same plane.
	boolean markceiling;

	boolean maskedtexture;
	INT32 toptexture, bottomtexture, midtexture;
	INT32 numthicksides, numbackffloors;

	boolean floormarked;
	boolean ceilingmarked;

	fixed_t pixhigh, pixlow, pixhighstep, pixlowstep;
	fixed_t topfrac, topstep;
	fixed_t bottomfrac, bottomstep;

	lighttable_t **walllights;
	INT16 *maskedtexturecol;
	fixed_t *maskedtextureheight;
} segloopcontext_t;

// ==========================================================================
// R_RenderMaskedSegRange
// ==========================================================================

// If we have a multi-patch texture on a 2sided wall (rare) then we draw
//  it using R_DrawColumn, else we draw it using R_DrawMaskedColumn, this
//  way we don't have to store extra post_t info with each column for
//  multi-patch textures. They are not normally needed as multi-patch
//  textures don't have holes in it. At least not for now.

static void R_Render2sidedMultiPatchColumn(colcontext_t *dc, column_t *column)
{
	INT32 topscreen, bottomscreen;

	topscreen = dc->sprtopscreen; // + spryscale*column->topdelta;  topdelta is 0 for the wall
	bottomscreen = topscreen + dc->spryscale * dc->lengthcol;

	dc->yl = (dc->sprtopscreen+FRACUNIT-1)>>FRACBITS;
	dc->yh = (bottomscreen-1)>>FRACBITS;

	if (dc->windowtop != INT32_MAX && dc->windowbottom != INT32_MAX)
	{
		dc->yl = ((dc->windowtop + FRACUNIT)>>FRACBITS);
		dc->yh = (dc->windowbottom - 1)>>FRACBITS;
	}

	if (dc->yh >= dc->mfloorclip[dc->x])
		dc->yh =  dc->mfloorclip[dc->x] - 1;
	if (dc->yl <= dc->mceilingclip[dc->x])
		dc->yl =  dc->mceilingclip[dc->x] + 1;

	if (dc->yl >= vid.height || dc->yh < 0)
		return;

	if (dc->yl <= dc->yh && dc->yh < vid.height && dc->yh > 0)
	{
		dc->source = (UINT8 *)column + 3;

		if (dc->func == colfuncs[BASEDRAWFUNC])
			(colfuncs[COLDRAWFUNC_TWOSMULTIPATCH])(dc);
		else if (dc->func == colfuncs[COLDRAWFUNC_FUZZY])
			(colfuncs[COLDRAWFUNC_TWOSMULTIPATCHTRANS])(dc);
		else
			dc->func(dc);
	}
}

transnum_t R_GetLinedefTransTable(fixed_t alpha)
{
	return (20*(FRACUNIT - alpha - 1) + FRACUNIT) >> (FRACBITS+1);
}

void R_RenderMaskedSegRange(rendercontext_t *context, drawseg_t *ds, INT32 x1, INT32 x2)
{
	seg_t *curline = ds->curline;
	sector_t *frontsector = curline->frontsector;
	sector_t *backsector = curline->backsector;

	colcontext_t *dc = &context->colcontext;
	viewcontext_t *viewcontext = &context->viewcontext;

	size_t pindex;
	column_t *col;
	INT16 *maskedtexturecol;
	INT32 lightnum, texnum, i;
	fixed_t height, realbot;
	lightlist_t *light;
	r_lightlist_t *rlight;
	lighttable_t **walllights = NULL;
	void (*colfunc_2s)(colcontext_t *, column_t *);
	line_t *ldef;
	sector_t *front, *back;
	INT32 times, repeats;
	INT64 overflow_test;
	INT32 range;
	fixed_t scalestep;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	texnum = R_GetTextureNum(curline->sidedef->midtexture);
	dc->windowbottom = dc->windowtop = dc->sprbotscreen = INT32_MAX;

	ldef = curline->linedef;
	if (!ldef->alpha)
		return;

	if (ldef->alpha > 0 && ldef->alpha < FRACUNIT)
	{
		dc->transmap = R_GetTranslucencyTable(R_GetLinedefTransTable(ldef->alpha));
		dc->func = colfuncs[COLDRAWFUNC_FUZZY];

	}
	else if (ldef->special == 909)
	{
		dc->func = colfuncs[COLDRAWFUNC_FOG];
		dc->windowtop = frontsector->ceilingheight;
		dc->windowbottom = frontsector->floorheight;
	}
	else
		dc->func = colfuncs[BASEDRAWFUNC];

	if (curline->polyseg && curline->polyseg->translucency > 0)
	{
		if (curline->polyseg->translucency >= NUMTRANSMAPS)
			return;

		dc->transmap = R_GetTranslucencyTable(curline->polyseg->translucency);
		dc->func = colfuncs[COLDRAWFUNC_FUZZY];
	}

	range = max(ds->x2-ds->x1, 1);
	scalestep = ds->scalestep;
	dc->spryscale = ds->scale1 + (x1 - ds->x1)*scalestep;

	// Texture must be cached before setting colfunc_2s,
	// otherwise texture[texnum]->holes may be false when it shouldn't be
	R_CheckTextureCache(texnum);

	// handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
	// are not stored per-column with post info in SRB2
	if (textures[texnum]->holes)
	{
		if (textures[texnum]->flip & 2) // vertically flipped?
		{
			colfunc_2s = R_DrawFlippedMaskedColumn;
			dc->lengthcol = textures[texnum]->height;
		}
		else
			colfunc_2s = R_DrawMaskedColumn; // render the usual 2sided single-patch packed texture
	}
	else
	{
		colfunc_2s = R_Render2sidedMultiPatchColumn; // render multipatch with no holes (no post_t info)
		dc->lengthcol = textures[texnum]->height;
	}

	// Setup lighting based on the presence/lack-of 3D floors.
	dc->numlights = 0;
	if (frontsector->numlights)
	{
		dc->numlights = frontsector->numlights;
		if (dc->numlights > dc->maxlights)
		{
			dc->maxlights = dc->numlights;
			dc->lightlist = Z_Realloc(dc->lightlist, sizeof (*dc->lightlist) * dc->maxlights, PU_STATIC, NULL);
		}

		for (i = 0; i < dc->numlights; i++)
		{
			fixed_t leftheight, rightheight;
			light = &frontsector->lightlist[i];
			rlight = &dc->lightlist[i];
			leftheight  = P_GetLightZAt(light, ds-> leftpos.x, ds-> leftpos.y);
			rightheight = P_GetLightZAt(light, ds->rightpos.x, ds->rightpos.y);

			leftheight  -= viewcontext->z;
			rightheight -= viewcontext->z;

			rlight->height     = (centeryfrac) - FixedMul(leftheight , ds->scale1);
			rlight->heightstep = (centeryfrac) - FixedMul(rightheight, ds->scale2);
			rlight->heightstep = (rlight->heightstep-rlight->height)/(range);
			//if (x1 > ds->x1)
				//rlight->height -= (x1 - ds->x1)*rlight->heightstep;
			rlight->startheight = rlight->height; // keep starting value here to reset for each repeat
			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = *light->extra_colormap;
			rlight->flags = light->flags;

			if ((dc->func != colfuncs[COLDRAWFUNC_FUZZY])
				|| (rlight->flags & FF_FOG)
				|| (rlight->extra_colormap && (rlight->extra_colormap->flags & CMF_FOG)))
				lightnum = (rlight->lightlevel >> LIGHTSEGSHIFT);
			else
				lightnum = LIGHTLEVELS - 1;

			if (rlight->extra_colormap && (rlight->extra_colormap->flags & CMF_FOG))
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
		if ((dc->func != colfuncs[COLDRAWFUNC_FUZZY])
			|| (frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)))
			lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);
		else
			lightnum = LIGHTLEVELS - 1;

		if (dc->func == colfuncs[COLDRAWFUNC_FOG]
			|| (frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)))
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

	dc->mfloorclip = ds->sprbottomclip;
	dc->mceilingclip = ds->sprtopclip;

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
			scalestep = ds->scalestep;
			dc->spryscale = ds->scale1 + (x1 - ds->x1)*scalestep;

			// reset all lights to their starting heights
			for (i = 0; i < dc->numlights; i++)
			{
				rlight = &dc->lightlist[i];
				rlight->height = rlight->startheight;
			}
		}

		dc->texheight = textureheight[texnum]>>FRACBITS;

		// draw the columns
		for (dc->x = x1; dc->x <= x2; dc->x++)
		{
			dc->texturemid = ds->maskedtextureheight[dc->x];

			if (!!(curline->linedef->flags & ML_DONTPEGBOTTOM) ^ !!(curline->linedef->flags & ML_EFFECT3))
				dc->texturemid += (textureheight[texnum])*times + textureheight[texnum];
			else
				dc->texturemid -= (textureheight[texnum])*times;
			// calculate lighting
			if (maskedtexturecol[dc->x] != INT16_MAX)
			{
				// Check for overflows first
				overflow_test = (INT64)centeryfrac - (((INT64)dc->texturemid*dc->spryscale)>>FRACBITS);
				if (overflow_test < 0) overflow_test = -overflow_test;
				if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL)
				{
					// Eh, no, go away, don't waste our time
					for (i = 0; i < dc->numlights; i++)
					{
						rlight = &dc->lightlist[i];
						rlight->height += rlight->heightstep;
					}
					dc->spryscale += scalestep;
					continue;
				}

				if (dc->numlights)
				{
					lighttable_t **xwalllights;

					dc->sprbotscreen = INT32_MAX;
					dc->sprtopscreen = dc->windowtop = (centeryfrac - FixedMul(dc->texturemid, dc->spryscale));

					realbot = dc->windowbottom = FixedMul(textureheight[texnum], dc->spryscale) + dc->sprtopscreen;
					dc->iscale = 0xffffffffu / (unsigned)dc->spryscale;

					// draw the texture
					col = (column_t *)((UINT8 *)R_GetColumn(texnum, maskedtexturecol[dc->x]) - 3);

					for (i = 0; i < dc->numlights; i++)
					{
						rlight = &dc->lightlist[i];

						if ((rlight->flags & FF_NOSHADE))
							continue;

						if (rlight->lightnum < 0)
							xwalllights = scalelight[0];
						else if (rlight->lightnum >= LIGHTLEVELS)
							xwalllights = scalelight[LIGHTLEVELS-1];
						else
							xwalllights = scalelight[rlight->lightnum];

						pindex = FixedMul(dc->spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;
						if (pindex >= MAXLIGHTSCALE)
							pindex = MAXLIGHTSCALE - 1;

						if (rlight->extra_colormap)
							rlight->rcolormap = rlight->extra_colormap->colormap + (xwalllights[pindex] - colormaps);
						else
							rlight->rcolormap = xwalllights[pindex];

						height = rlight->height;
						rlight->height += rlight->heightstep;

						if (height <= dc->windowtop)
						{
							dc->colormap = rlight->rcolormap;
							continue;
						}

						dc->windowbottom = height;
						if (dc->windowbottom >= realbot)
						{
							dc->windowbottom = realbot;
							colfunc_2s(dc, col);
							for (i++; i < dc->numlights; i++)
							{
								rlight = &dc->lightlist[i];
								rlight->height += rlight->heightstep;
							}

							continue;
						}
						colfunc_2s(dc, col);
						dc->windowtop = dc->windowbottom + 1;
						dc->colormap = rlight->rcolormap;
					}
					dc->windowbottom = realbot;
					if (dc->windowtop < dc->windowbottom)
						colfunc_2s(dc, col);

					dc->spryscale += scalestep;
					continue;
				}

				// calculate lighting
				pindex = FixedMul(dc->spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;
				if (pindex >= MAXLIGHTSCALE)
					pindex = MAXLIGHTSCALE - 1;

				dc->colormap = walllights[pindex];

				if (frontsector->extra_colormap)
					dc->colormap = frontsector->extra_colormap->colormap + (dc->colormap - colormaps);

				dc->sprtopscreen = centeryfrac - FixedMul(dc->texturemid, dc->spryscale);
				dc->iscale = 0xffffffffu / (unsigned)dc->spryscale;

				// draw the texture
				col = (column_t *)((UINT8 *)R_GetColumn(texnum, maskedtexturecol[dc->x]) - 3);

#if 0 // Disabling this allows inside edges to render below the planes, for until the clipping is fixed to work right when POs are near the camera. -Red
				if (curline->dontrenderme && curline->polyseg && (curline->polyseg->flags & POF_RENDERPLANES))
				{
					fixed_t my_topscreen;
					fixed_t my_bottomscreen;
					fixed_t my_yl, my_yh;

					my_topscreen = dc->sprtopscreen + dc->spryscale*col->topdelta;
					my_bottomscreen = dc->sprbotscreen == INT32_MAX ? my_topscreen + dc->spryscale*col->length
					                                         : dc->sprbotscreen + dc->spryscale*col->length;

					my_yl = (my_topscreen+FRACUNIT-1)>>FRACBITS;
					my_yh = (my_bottomscreen-1)>>FRACBITS;
	//				CONS_Debug(DBG_RENDER, "my_topscreen: %d\nmy_bottomscreen: %d\nmy_yl: %d\nmy_yh: %d\n", my_topscreen, my_bottomscreen, my_yl, my_yh);

					if (planecontext->numffloors)
					{
						INT32 top = my_yl;
						INT32 bottom = my_yh;

						for (i = 0; i < planecontext->numffloors; i++)
						{
							if (!planecontext->ffloor[i].polyobj || planecontext->ffloor[i].polyobj != curline->polyseg)
								continue;

							if (planecontext->ffloor[i].height < viewcontext->z)
							{
								INT32 top_w = planecontext->ffloor[i].plane->top[dc->x];

	//							CONS_Debug(DBG_RENDER, "Leveltime : %d\n", leveltime);
	//							CONS_Debug(DBG_RENDER, "Top is %d, top_w is %d\n", top, top_w);
								if (top_w < top)
								{
									planecontext->ffloor[i].plane->top[dc->x] = (INT16)top;
									planecontext->ffloor[i].plane->picnum = 0;
								}
	//							CONS_Debug(DBG_RENDER, "top_w is now %d\n", planecontext->ffloor[i].plane->top[dc->x]);
							}
							else if (planecontext->ffloor[i].height > viewcontext->z)
							{
								INT32 bottom_w = planecontext->ffloor[i].plane->bottom[dc->x];

								if (bottom_w > bottom)
								{
									planecontext->ffloor[i].plane->bottom[dc->x] = (INT16)bottom;
									planecontext->ffloor[i].plane->picnum = 0;
								}
							}
						}
					}
				}
				else
#endif
					colfunc_2s(dc, col);
			}

			dc->spryscale += scalestep;
		}
	}

	dc->func = colfuncs[BASEDRAWFUNC];
}

// Loop through R_DrawMaskedColumn calls
static void R_DrawRepeatMaskedColumn(colcontext_t *dc, column_t *col)
{
	while (dc->sprtopscreen < dc->sprbotscreen) {
		R_DrawMaskedColumn(dc, col);
		if ((INT64)dc->sprtopscreen + dc->texheight*dc->spryscale > (INT64)INT32_MAX) // prevent overflow
			dc->sprtopscreen = INT32_MAX;
		else
			dc->sprtopscreen += dc->texheight*dc->spryscale;
	}
}

static void R_DrawRepeatFlippedMaskedColumn(colcontext_t *dc, column_t *col)
{
	do {
		R_DrawFlippedMaskedColumn(dc, col);
		dc->sprtopscreen += dc->texheight*dc->spryscale;
	} while (dc->sprtopscreen < dc->sprbotscreen);
}

// Returns true if a fake floor is translucent.
static boolean R_IsFFloorTranslucent(planemgr_t *pfloor)
{
	if (pfloor->polyobj)
		return (pfloor->polyobj->translucency > 0);

	// Polyobjects have no ffloors, and they're handled in the conditional above.
	if (pfloor->ffloor != NULL)
		return (pfloor->ffloor->flags & (FF_TRANSLUCENT|FF_FOG));

	return false;
}

//
// R_RenderThickSideRange
// Renders all the thick sides in the given range.
void R_RenderThickSideRange(rendercontext_t *context, drawseg_t *ds, INT32 x1, INT32 x2, ffloor_t *pfloor)
{
	seg_t *curline = ds->curline;
	sector_t *backsector = pfloor->target;
	sector_t *frontsector = curline->frontsector == pfloor->target ? curline->backsector : curline->frontsector;

	viewcontext_t *viewcontext = &context->viewcontext;
	colcontext_t *dc = &context->colcontext;

	size_t pindex;
	column_t *col;
	INT16 *maskedtexturecol;
	INT32 lightnum;
	INT32 texnum;
	sector_t tempsec;
	INT32 templight;
	INT32 i, p, range;
	fixed_t bottombounds = viewheight << FRACBITS;
	fixed_t topbounds = (con_clipviewtop - 1) << FRACBITS;
	fixed_t offsetvalue = 0;
	lightlist_t *light;
	r_lightlist_t *rlight;
	lighttable_t **walllights = NULL;
	line_t *newline = NULL;
	fixed_t scalestep;

	// Render FOF sides kinda like normal sides, with the frac and step and everything
	// NOTE: INT64 instead of fixed_t because overflow concerns
	INT64 top_frac, top_step, bottom_frac, bottom_step;

	// skew FOF walls with slopes?
	boolean slopeskew = false;
	fixed_t ffloortextureslide = 0;
	INT32 oldx = -1;
	fixed_t left_top, left_bottom; // needed here for slope skewing
	pslope_t *skewslope = NULL;

	void (*colfunc_2s) (colcontext_t *, column_t *);

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	texnum = R_GetTextureNum(sides[pfloor->master->sidenum[0]].midtexture);

	dc->func = colfuncs[BASEDRAWFUNC];

	if (pfloor->master->flags & ML_TFERLINE)
	{
		size_t linenum = curline->linedef-backsector->lines[0];
		newline = pfloor->master->frontsector->lines[0] + linenum;
		texnum = R_GetTextureNum(sides[newline->sidenum[0]].midtexture);
	}

	if (pfloor->flags & FF_TRANSLUCENT)
	{
		boolean fuzzy = true;

		// Hacked up support for alpha value in software mode Tails 09-24-2002
		if (pfloor->alpha < 12)
			return; // Don't even draw it
		else if (pfloor->alpha < 38)
			dc->transmap = R_GetTranslucencyTable(tr_trans90);
		else if (pfloor->alpha < 64)
			dc->transmap = R_GetTranslucencyTable(tr_trans80);
		else if (pfloor->alpha < 89)
			dc->transmap = R_GetTranslucencyTable(tr_trans70);
		else if (pfloor->alpha < 115)
			dc->transmap = R_GetTranslucencyTable(tr_trans60);
		else if (pfloor->alpha < 140)
			dc->transmap = R_GetTranslucencyTable(tr_trans50);
		else if (pfloor->alpha < 166)
			dc->transmap = R_GetTranslucencyTable(tr_trans40);
		else if (pfloor->alpha < 192)
			dc->transmap = R_GetTranslucencyTable(tr_trans30);
		else if (pfloor->alpha < 217)
			dc->transmap = R_GetTranslucencyTable(tr_trans20);
		else if (pfloor->alpha < 243)
			dc->transmap = R_GetTranslucencyTable(tr_trans10);
		else
			fuzzy = false; // Opaque

		if (fuzzy)
			dc->func = colfuncs[COLDRAWFUNC_FUZZY];
	}
	else if (pfloor->flags & FF_FOG)
		dc->func = colfuncs[COLDRAWFUNC_FOG];

	range = max(ds->x2-ds->x1, 1);
	//SoM: Moved these up here so they are available for my lightlist calculations
	scalestep = ds->scalestep;
	dc->spryscale = ds->scale1 + (x1 - ds->x1)*scalestep;

	dc->numlights = 0;
	if (frontsector->numlights)
	{
		dc->numlights = frontsector->numlights;
		if (dc->numlights > dc->maxlights)
		{
			dc->maxlights = dc->numlights;
			dc->lightlist = Z_Realloc(dc->lightlist, sizeof (*dc->lightlist) * dc->maxlights, PU_STATIC, NULL);
		}

		for (i = p = 0; i < dc->numlights; i++)
		{
			fixed_t leftheight, rightheight;
			fixed_t pfloorleft, pfloorright;
			INT64 overflow_test;
			light = &frontsector->lightlist[i];
			rlight = &dc->lightlist[p];

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	end1 = P_GetZAt(slope, ds-> leftpos.x, ds-> leftpos.y, normalheight); \
	end2 = P_GetZAt(slope, ds->rightpos.x, ds->rightpos.y, normalheight);

			SLOPEPARAMS(light->slope,     leftheight, rightheight, light->height)
			SLOPEPARAMS(*pfloor->b_slope, pfloorleft, pfloorright, *pfloor->bottomheight)

			if (leftheight < pfloorleft && rightheight < pfloorright)
				continue;

			SLOPEPARAMS(*pfloor->t_slope, pfloorleft, pfloorright, *pfloor->topheight)

			if (leftheight > pfloorleft && rightheight > pfloorright && i+1 < dc->numlights)
			{
				lightlist_t *nextlight = &frontsector->lightlist[i+1];
				if (P_GetZAt(nextlight->slope, ds-> leftpos.x, ds-> leftpos.y, nextlight->height) > pfloorleft
				 && P_GetZAt(nextlight->slope, ds->rightpos.x, ds->rightpos.y, nextlight->height) > pfloorright)
					continue;
			}

			leftheight -= viewcontext->z;
			rightheight -= viewcontext->z;

#define CLAMPMAX INT32_MAX
#define CLAMPMIN (-INT32_MAX) // This is not INT32_MIN on purpose! INT32_MIN makes the drawers freak out.
			// Monster Iestyn (25/03/18): do not skip these lights if they fail overflow test, just clamp them instead so they behave.
			overflow_test = (INT64)centeryfrac - (((INT64)leftheight*ds->scale1)>>FRACBITS);
			if      (overflow_test > (INT64)CLAMPMAX) rlight->height = CLAMPMAX;
			else if (overflow_test > (INT64)CLAMPMIN) rlight->height = (fixed_t)overflow_test;
			else                                      rlight->height = CLAMPMIN;

			overflow_test = (INT64)centeryfrac - (((INT64)rightheight*ds->scale2)>>FRACBITS);
			if      (overflow_test > (INT64)CLAMPMAX) rlight->heightstep = CLAMPMAX;
			else if (overflow_test > (INT64)CLAMPMIN) rlight->heightstep = (fixed_t)overflow_test;
			else                                      rlight->heightstep = CLAMPMIN;
			rlight->heightstep = (rlight->heightstep-rlight->height)/(range);
			rlight->flags = light->flags;
			if (light->flags & FF_CUTLEVEL)
			{
				SLOPEPARAMS(*light->caster->b_slope, leftheight, rightheight, *light->caster->bottomheight)
#undef SLOPEPARAMS
				leftheight -= viewcontext->z;
				rightheight -= viewcontext->z;

				// Monster Iestyn (25/03/18): do not skip these lights if they fail overflow test, just clamp them instead so they behave.
				overflow_test = (INT64)centeryfrac - (((INT64)leftheight*ds->scale1)>>FRACBITS);
				if      (overflow_test > (INT64)CLAMPMAX) rlight->botheight = CLAMPMAX;
				else if (overflow_test > (INT64)CLAMPMIN) rlight->botheight = (fixed_t)overflow_test;
				else                                      rlight->botheight = CLAMPMIN;

				overflow_test = (INT64)centeryfrac - (((INT64)rightheight*ds->scale2)>>FRACBITS);
				if      (overflow_test > (INT64)CLAMPMAX) rlight->botheightstep = CLAMPMAX;
				else if (overflow_test > (INT64)CLAMPMIN) rlight->botheightstep = (fixed_t)overflow_test;
				else                                      rlight->botheightstep = CLAMPMIN;
				rlight->botheightstep = (rlight->botheightstep-rlight->botheight)/(range);
			}

			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = *light->extra_colormap;

			// Check if the current light effects the colormap/lightlevel
			if (pfloor->flags & FF_FOG)
				rlight->lightnum = (pfloor->master->frontsector->lightlevel >> LIGHTSEGSHIFT);
			else
				rlight->lightnum = (rlight->lightlevel >> LIGHTSEGSHIFT);

			if (pfloor->flags & FF_FOG || rlight->flags & FF_FOG || (rlight->extra_colormap && (rlight->extra_colormap->flags & CMF_FOG)))
				;
			else if (curline->v1->y == curline->v2->y)
				rlight->lightnum--;
			else if (curline->v1->x == curline->v2->x)
				rlight->lightnum++;

			p++;
		}

		dc->numlights = p;
	}
	else
	{
		// Get correct light level!
		if ((frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)))
			lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);
		else if (pfloor->flags & FF_FOG)
			lightnum = (pfloor->master->frontsector->lightlevel >> LIGHTSEGSHIFT);
		else if (dc->func == colfuncs[COLDRAWFUNC_FUZZY])
			lightnum = LIGHTLEVELS-1;
		else {
			sector_t *fakeflat = R_FakeFlat(&context->viewcontext, frontsector, &tempsec, &templight, &templight, false);
			lightnum = fakeflat->lightlevel >> LIGHTSEGSHIFT;
		}

		if (pfloor->flags & FF_FOG || (frontsector->extra_colormap && (frontsector->extra_colormap->flags & CMF_FOG)));
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

	dc->mfloorclip = ds->sprbottomclip;
	dc->mceilingclip = ds->sprtopclip;
	dc->texheight = textureheight[texnum]>>FRACBITS;

	// calculate both left ends
	left_top    = P_GetFFloorTopZAt   (pfloor, ds->leftpos.x, ds->leftpos.y) - viewcontext->z;
	left_bottom = P_GetFFloorBottomZAt(pfloor, ds->leftpos.x, ds->leftpos.y) - viewcontext->z;

	skewslope = *pfloor->t_slope; // skew using top slope by default
	if (newline)
	{
		if (newline->flags & ML_DONTPEGTOP)
			slopeskew = true;
	}
	else if (pfloor->master->flags & ML_DONTPEGTOP)
		slopeskew = true;

	if (slopeskew)
		dc->texturemid = left_top;
	else
		dc->texturemid = *pfloor->topheight - viewcontext->z;

	if (newline)
	{
		offsetvalue = sides[newline->sidenum[0]].rowoffset;
		if (newline->flags & ML_DONTPEGBOTTOM)
		{
			skewslope = *pfloor->b_slope; // skew using bottom slope
			if (slopeskew)
				dc->texturemid = left_bottom;
			else
				offsetvalue -= *pfloor->topheight - *pfloor->bottomheight;
		}
	}
	else
	{
		offsetvalue = sides[pfloor->master->sidenum[0]].rowoffset;
		if (curline->linedef->flags & ML_DONTPEGBOTTOM)
		{
			skewslope = *pfloor->b_slope; // skew using bottom slope
			if (slopeskew)
				dc->texturemid = left_bottom;
			else
				offsetvalue -= *pfloor->topheight - *pfloor->bottomheight;
		}
	}

	if (slopeskew)
	{
		angle_t lineangle = R_PointToAngle2(curline->v1->x, curline->v1->y, curline->v2->x, curline->v2->y);

		if (skewslope)
			ffloortextureslide = FixedMul(skewslope->zdelta, FINECOSINE((lineangle-skewslope->xydirection)>>ANGLETOFINESHIFT));
	}

	dc->texturemid += offsetvalue;

	// Texture must be cached before setting colfunc_2s,
	// otherwise texture[texnum]->holes may be false when it shouldn't be
	R_CheckTextureCache(texnum);

	//faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
	//     are not stored per-column with post info anymore in Doom Legacy
	if (textures[texnum]->holes)
	{
		if (textures[texnum]->flip & 2) // vertically flipped?
		{
			colfunc_2s = R_DrawRepeatFlippedMaskedColumn;
			dc->lengthcol = textures[texnum]->height;
		}
		else
			colfunc_2s = R_DrawRepeatMaskedColumn; // render the usual 2sided single-patch packed texture
	}
	else
	{
		colfunc_2s = R_Render2sidedMultiPatchColumn;        //render multipatch with no holes (no post_t info)
		dc->lengthcol = textures[texnum]->height;
	}

	// Set heights according to plane, or slope, whichever
	{
		fixed_t right_top, right_bottom;

		// calculate right ends now
		right_top    = P_GetFFloorTopZAt   (pfloor, ds->rightpos.x, ds->rightpos.y) - viewcontext->z;
		right_bottom = P_GetFFloorBottomZAt(pfloor, ds->rightpos.x, ds->rightpos.y) - viewcontext->z;

		// using INT64 to avoid 32bit overflow
		top_frac =    (INT64)centeryfrac - (((INT64)left_top     * ds->scale1) >> FRACBITS);
		bottom_frac = (INT64)centeryfrac - (((INT64)left_bottom  * ds->scale1) >> FRACBITS);
		top_step =    (INT64)centeryfrac - (((INT64)right_top    * ds->scale2) >> FRACBITS);
		bottom_step = (INT64)centeryfrac - (((INT64)right_bottom * ds->scale2) >> FRACBITS);

		top_step = (top_step-top_frac)/(range);
		bottom_step = (bottom_step-bottom_frac)/(range);

		top_frac += top_step * (x1 - ds->x1);
		bottom_frac += bottom_step * (x1 - ds->x1);
	}

	// draw the columns
	for (dc->x = x1; dc->x <= x2; dc->x++)
	{
		if (maskedtexturecol[dc->x] != INT16_MAX)
		{
			if (ffloortextureslide) { // skew FOF walls
				if (oldx != -1)
					dc->texturemid += FixedMul(ffloortextureslide, (maskedtexturecol[oldx]-maskedtexturecol[dc->x])<<FRACBITS);
				oldx = dc->x;
			}
			// Calculate bounds
			// clamp the values if necessary to avoid overflows and rendering glitches caused by them

			if      (top_frac > (INT64)CLAMPMAX) dc->sprtopscreen = dc->windowtop = CLAMPMAX;
			else if (top_frac > (INT64)CLAMPMIN) dc->sprtopscreen = dc->windowtop = (fixed_t)top_frac;
			else                                 dc->sprtopscreen = dc->windowtop = CLAMPMIN;
			if      (bottom_frac > (INT64)CLAMPMAX) dc->sprbotscreen = dc->windowbottom = CLAMPMAX;
			else if (bottom_frac > (INT64)CLAMPMIN) dc->sprbotscreen = dc->windowbottom = (fixed_t)bottom_frac;
			else                                    dc->sprbotscreen = dc->windowbottom = CLAMPMIN;

			top_frac += top_step;
			bottom_frac += bottom_step;

			// SoM: If column is out of range, why bother with it??
			if (dc->windowbottom < topbounds || dc->windowtop > bottombounds)
			{
				for (i = 0; i < dc->numlights; i++)
				{
					rlight = &dc->lightlist[i];
					rlight->height += rlight->heightstep;
					if (rlight->flags & FF_CUTLEVEL)
						rlight->botheight += rlight->botheightstep;
				}

				dc->spryscale += scalestep;
				continue;
			}

			dc->iscale = 0xffffffffu / (unsigned)dc->spryscale;

			// Get data for the column
			col = (column_t *)((UINT8 *)R_GetColumn(texnum,maskedtexturecol[dc->x]) - 3);

			// SoM: New code does not rely on R_DrawColumnShadowed_8 which
			// will (hopefully) put less strain on the stack.
			if (dc->numlights)
			{
				lighttable_t **xwalllights;
				fixed_t height;
				fixed_t bheight = 0;
				INT32 solid = 0;
				INT32 lighteffect = 0;

				for (i = 0; i < dc->numlights; i++)
				{
					// Check if the current light effects the colormap/lightlevel
					rlight = &dc->lightlist[i];
					lighteffect = !(dc->lightlist[i].flags & FF_NOSHADE);
					if (lighteffect)
					{
						lightnum = rlight->lightnum;

						if (lightnum < 0)
							xwalllights = scalelight[0];
						else if (lightnum >= LIGHTLEVELS)
							xwalllights = scalelight[LIGHTLEVELS-1];
						else
							xwalllights = scalelight[lightnum];

						pindex = FixedMul(dc->spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;
						if (pindex >= MAXLIGHTSCALE)
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

					solid = 0; // don't carry over solid-cutting flag from the previous light

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

					height = rlight->height;
					rlight->height += rlight->heightstep;

					if (solid)
					{
						bheight = rlight->botheight - (FRACUNIT >> 1);
						rlight->botheight += rlight->botheightstep;
					}

					if (height <= dc->windowtop)
					{
						if (lighteffect)
							dc->colormap = rlight->rcolormap;
						if (solid && dc->windowtop < bheight)
							dc->windowtop = bheight;
						continue;
					}

					dc->windowbottom = height;
					if (dc->windowbottom >= dc->sprbotscreen)
					{
						dc->windowbottom = dc->sprbotscreen;
						// draw the texture
						colfunc_2s (dc, col);
						for (i++; i < dc->numlights; i++)
						{
							rlight = &dc->lightlist[i];
							rlight->height += rlight->heightstep;
							if (rlight->flags & FF_CUTLEVEL)
								rlight->botheight += rlight->botheightstep;
						}
						continue;
					}
					// draw the texture
					colfunc_2s (dc, col);
					if (solid)
						dc->windowtop = bheight;
					else
						dc->windowtop = dc->windowbottom + 1;
					if (lighteffect)
						dc->colormap = rlight->rcolormap;
				}
				dc->windowbottom = dc->sprbotscreen;
				// draw the texture, if there is any space left
				if (dc->windowtop < dc->windowbottom)
					colfunc_2s (dc, col);

				dc->spryscale += scalestep;
				continue;
			}

			// calculate lighting
			pindex = FixedMul(dc->spryscale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;
			if (pindex >= MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE - 1;

			dc->colormap = walllights[pindex];

			if (pfloor->flags & FF_FOG && pfloor->master->frontsector->extra_colormap)
				dc->colormap = pfloor->master->frontsector->extra_colormap->colormap + (dc->colormap - colormaps);
			else if (frontsector->extra_colormap)
				dc->colormap = frontsector->extra_colormap->colormap + (dc->colormap - colormaps);

			// draw the texture
			colfunc_2s (dc, col);
			dc->spryscale += scalestep;
		}
	}

	dc->func = colfuncs[BASEDRAWFUNC];

#undef CLAMPMAX
#undef CLAMPMIN
}

// R_ExpandPlaneY
//
// A simple function to modify a visplane's top and bottom for a particular column
// Sort of like R_ExpandPlane in r_plane.c, except this is vertical expansion
static inline void R_ExpandPlaneY(visplane_t *pl, INT32 x, INT16 top, INT16 bottom)
{
	// Expand the plane, don't shrink it!
	// note: top and bottom default to 0xFFFF and 0x0000 respectively, which is totally compatible with this
	if (pl->top[x] > top)       pl->top[x] = top;
	if (pl->bottom[x] < bottom) pl->bottom[x] = bottom;
}

// R_FFloorCanClip
//
// Returns true if a fake floor can clip a column away.
static boolean R_FFloorCanClip(planemgr_t *pfloor)
{
	return (cv_ffloorclip.value && !R_IsFFloorTranslucent(pfloor) && !pfloor->polyobj);
}

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//

#define HEIGHTBITS 12
#define HEIGHTUNIT (1<<HEIGHTBITS)

static void R_RenderSegLoop(rendercontext_t *context, wallcontext_t *wallcontext, segloopcontext_t *segcontext, colcontext_t *dc)
{
	bspcontext_t *bspcontext = &context->bspcontext;
	planecontext_t *planecontext = &context->planecontext;
	viewcontext_t *viewcontext = &context->viewcontext;

	angle_t angle;
	size_t pindex;
	INT32 yl;
	INT32 yh;

	INT32 mid;
	fixed_t texturecolumn = 0;
	fixed_t oldtexturecolumn = -1;
	INT32 top;
	INT32 bottom;
	INT32 i;
	INT32 currx = segcontext->x;

	if (dc->numlights)
		dc->func = colfuncs[COLDRAWFUNC_SHADOWED];

	for (; currx < segcontext->stopx; currx++)
	{
		// mark floor / ceiling areas
		yl = (segcontext->topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

		top = planecontext->ceilingclip[currx]+1;

		// no space above wall?
		if (yl < top)
			yl = top;

		if (segcontext->markceiling)
		{
#if 0
			bottom = yl-1;

			if (bottom >= planecontext->floorclip[currx])
				bottom = planecontext->floorclip[currx]-1;

			if (top <= bottom)
#else
			bottom = yl > planecontext->floorclip[currx] ? planecontext->floorclip[currx] : yl;

			if (top <= --bottom && planecontext->ceilingplane)
#endif
				R_ExpandPlaneY(planecontext->ceilingplane, currx, top, bottom);
		}


		yh = segcontext->bottomfrac>>HEIGHTBITS;

		bottom = planecontext->floorclip[currx]-1;

		if (yh > bottom)
			yh = bottom;

		if (segcontext->markfloor)
		{
			top = yh < planecontext->ceilingclip[currx] ? planecontext->ceilingclip[currx] : yh;

			if (++top <= bottom && planecontext->floorplane)
				R_ExpandPlaneY(planecontext->floorplane, currx, top, bottom);
		}

		segcontext->floormarked = false;
		segcontext->ceilingmarked = false;

		if (planecontext->numffloors)
		{
			INT16 fftop, ffbottom;

			bspcontext->firstseg->frontscale[currx] = planecontext->frontscale[currx];
			top = planecontext->ceilingclip[currx]+1; // PRBoom
			bottom = planecontext->floorclip[currx]-1; // PRBoom

			for (i = 0; i < planecontext->numffloors; i++)
			{
				if (planecontext->ffloor[i].polyobj && (!bspcontext->curline->polyseg || planecontext->ffloor[i].polyobj != bspcontext->curline->polyseg))
					continue;

				if (planecontext->ffloor[i].height < viewcontext->z)
				{
					INT32 top_w = (planecontext->ffloor[i].f_frac >> HEIGHTBITS) + 1;
					INT32 bottom_w = planecontext->ffloor[i].f_clip[currx];

					if (top_w < top)
						top_w = top;

					if (bottom_w > bottom)
						bottom_w = bottom;

					// Polyobject-specific hack to fix plane leaking -Red
					if (planecontext->ffloor[i].polyobj && top_w >= bottom_w)
					{
						planecontext->ffloor[i].plane->top[currx] = 0xFFFF;
						planecontext->ffloor[i].plane->bottom[currx] = 0x0000; // fix for sky plane drawing crashes - Monster Iestyn 25/05/18
					}
					else
					{
						if (top_w <= bottom_w)
						{
							fftop = (INT16)top_w;
							ffbottom = (INT16)bottom_w;

							planecontext->ffloor[i].plane->top[currx] = fftop;
							planecontext->ffloor[i].plane->bottom[currx] = ffbottom;

							// Cull part of the column by the 3D floor if it can't be seen
							// "bottom" is the top pixel of the floor column
							if (ffbottom >= bottom-1 && R_FFloorCanClip(&planecontext->ffloor[i]) && !bspcontext->curline->polyseg)
							{
								segcontext->floormarked = true;
								planecontext->floorclip[currx] = fftop;
								if (yh > fftop)
									yh = fftop;

								if (segcontext->markfloor && planecontext->floorplane)
									planecontext->floorplane->top[currx] = bottom;

								if (wallcontext->silhouette)
								{
									(*wallcontext->silhouette) |= SIL_BOTTOM;
									(*wallcontext->bsilheight) = INT32_MAX;
								}
							}
						}
					}
				}
				else if (planecontext->ffloor[i].height > viewcontext->z)
				{
					INT32 top_w = planecontext->ffloor[i].c_clip[currx] + 1;
					INT32 bottom_w = (planecontext->ffloor[i].f_frac >> HEIGHTBITS);

					if (top_w < top)
						top_w = top;

					if (bottom_w > bottom)
						bottom_w = bottom;

					// Polyobject-specific hack to fix plane leaking -Red
					if (planecontext->ffloor[i].polyobj && top_w >= bottom_w)
					{
						planecontext->ffloor[i].plane->top[currx] = 0xFFFF;
						planecontext->ffloor[i].plane->bottom[currx] = 0x0000; // fix for sky plane drawing crashes - Monster Iestyn 25/05/18
					}
					else
					{
						if (top_w <= bottom_w)
						{
							fftop = (INT16)top_w;
							ffbottom = (INT16)bottom_w;

							planecontext->ffloor[i].plane->top[currx] = fftop;
							planecontext->ffloor[i].plane->bottom[currx] = ffbottom;

							// Cull part of the column by the 3D floor if it can't be seen
							// "top" is the height of the ceiling column
							if (fftop <= top+1 && R_FFloorCanClip(&planecontext->ffloor[i]) && !bspcontext->curline->polyseg)
							{
								segcontext->ceilingmarked = true;
								planecontext->ceilingclip[currx] = ffbottom;
								if (yl < ffbottom)
									yl = ffbottom;

								if (segcontext->markceiling && planecontext->ceilingplane)
									planecontext->ceilingplane->bottom[currx] = top;

								if (wallcontext->silhouette)
								{
									(*wallcontext->silhouette) |= SIL_TOP;
									(*wallcontext->tsilheight) = INT32_MIN;
								}
							}
						}
					}
				}
			}
		}

		//SoM: Calculate offsets for Thick fake floors.
		// calculate texture offset
		angle = (wallcontext->centerangle + xtoviewangle[currx])>>ANGLETOFINESHIFT;
		texturecolumn = wallcontext->offset-FixedMul(FINETANGENT(angle),wallcontext->distance);

		if (oldtexturecolumn != -1) {
			wallcontext->bottomtexturemid += FixedMul(wallcontext->bottomtextureslide,  oldtexturecolumn-texturecolumn);
			wallcontext->midtexturemid    += FixedMul(wallcontext->midtextureslide,     oldtexturecolumn-texturecolumn);
			wallcontext->toptexturemid    += FixedMul(wallcontext->toptextureslide,     oldtexturecolumn-texturecolumn);
			wallcontext->midtextureback   += FixedMul(wallcontext->midtexturebackslide, oldtexturecolumn-texturecolumn);
		}
		oldtexturecolumn = texturecolumn;

		texturecolumn >>= FRACBITS;

		// texturecolumn and lighting are independent of wall tiers
		if (segcontext->segtextured)
		{
			// calculate lighting
			pindex = FixedMul(wallcontext->scale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;
			if (pindex >= MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE-1;

			dc->colormap = segcontext->walllights[pindex];
			dc->x = currx;
			dc->iscale = 0xffffffffu / (unsigned)wallcontext->scale;

			if (bspcontext->frontsector->extra_colormap)
				dc->colormap = bspcontext->frontsector->extra_colormap->colormap + (dc->colormap - colormaps);
		}

		for (i = 0; i < dc->numlights; i++)
		{
			INT32 lightnum = (dc->lightlist[i].lightlevel >> LIGHTSEGSHIFT);
			lighttable_t **xwalllights;

			if (dc->lightlist[i].extra_colormap)
				;
			else if (bspcontext->curline->v1->y == bspcontext->curline->v2->y)
				lightnum--;
			else if (bspcontext->curline->v1->x == bspcontext->curline->v2->x)
				lightnum++;

			if (lightnum < 0)
				xwalllights = scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
				xwalllights = scalelight[LIGHTLEVELS-1];
			else
				xwalllights = scalelight[lightnum];

			pindex = FixedMul(wallcontext->scale, LIGHTRESOLUTIONFIX)>>LIGHTSCALESHIFT;
			if (pindex >= MAXLIGHTSCALE)
				pindex = MAXLIGHTSCALE-1;

			if (dc->lightlist[i].extra_colormap)
				dc->lightlist[i].rcolormap = dc->lightlist[i].extra_colormap->colormap + (xwalllights[pindex] - colormaps);
			else
				dc->lightlist[i].rcolormap = xwalllights[pindex];
		}

		planecontext->frontscale[currx] = wallcontext->scale;

		// draw the wall tiers
		if (segcontext->midtexture)
		{
			// single sided line
			if (yl <= yh && yh >= 0 && yl < viewheight)
			{
				dc->yl = yl;
				dc->yh = yh;
				dc->texturemid = wallcontext->midtexturemid;
				dc->source = R_GetColumn(segcontext->midtexture, texturecolumn);
				dc->texheight = textureheight[segcontext->midtexture]>>FRACBITS;

				dc->func(dc);

				// dont draw anything more for this column, since
				// a midtexture blocks the view
				if (!segcontext->ceilingmarked)
					planecontext->ceilingclip[currx] = (INT16)viewheight;
				if (!segcontext->floormarked)
					planecontext->floorclip[currx] = -1;
			}
			else
			{
				// note: don't use min/max macros, since casting from INT32 to INT16 is involved here
				if (segcontext->markceiling && (!segcontext->ceilingmarked))
					planecontext->ceilingclip[currx] = (yl >= 0) ? ((yl > viewheight) ? (INT16)viewheight : (INT16)((INT16)yl - 1)) : -1;
				if (segcontext->markfloor && (!segcontext->floormarked))
					planecontext->floorclip[currx] = (yh < viewheight) ? ((yh < -1) ? -1 : (INT16)((INT16)yh + 1)) : (INT16)viewheight;
			}
		}
		else
		{
			INT16 topclip = (yl >= 0) ? ((yl > viewheight) ? (INT16)viewheight : (INT16)((INT16)yl - 1)) : -1;
			INT16 bottomclip = (yh < viewheight) ? ((yh < -1) ? -1 : (INT16)((INT16)yh + 1)) : (INT16)viewheight;

			// two sided line
			if (segcontext->toptexture)
			{
				// top wall
				mid = segcontext->pixhigh>>HEIGHTBITS;
				segcontext->pixhigh += segcontext->pixhighstep;

				if (mid >= planecontext->floorclip[currx])
					mid = planecontext->floorclip[currx]-1;

				if (mid >= yl) // back ceiling lower than front ceiling ?
				{
					if (yl >= viewheight) // entirely off bottom of screen
					{
						if (!segcontext->ceilingmarked)
							planecontext->ceilingclip[currx] = (INT16)viewheight;
					}
					else if (mid >= 0) // safe to draw top texture
					{
						dc->yl = yl;
						dc->yh = mid;
						dc->texturemid = wallcontext->toptexturemid;
						dc->source = R_GetColumn(segcontext->toptexture, texturecolumn);
						dc->texheight = textureheight[segcontext->toptexture]>>FRACBITS;
						dc->func(dc);
						planecontext->ceilingclip[currx] = (INT16)mid;
					}
					else if (!segcontext->ceilingmarked) // entirely off top of screen
						planecontext->ceilingclip[currx] = -1;
				}
				else if (!segcontext->ceilingmarked)
					planecontext->ceilingclip[currx] = topclip;
			}
			else if (segcontext->markceiling && (!segcontext->ceilingmarked)) // no top wall
				planecontext->ceilingclip[currx] = topclip;

			if (segcontext->bottomtexture)
			{
				// bottom wall
				mid = (segcontext->pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
				segcontext->pixlow += segcontext->pixlowstep;

				// no space above wall?
				if (mid <= planecontext->ceilingclip[currx])
					mid = planecontext->ceilingclip[currx]+1;

				if (mid <= yh) // back floor higher than front floor ?
				{
					if (yh < 0) // entirely off top of screen
					{
						if (!segcontext->floormarked)
							planecontext->floorclip[currx] = -1;
					}
					else if (mid < viewheight) // safe to draw bottom texture
					{
						dc->yl = mid;
						dc->yh = yh;
						dc->texturemid = wallcontext->bottomtexturemid;
						dc->source = R_GetColumn(segcontext->bottomtexture, texturecolumn);
						dc->texheight = textureheight[segcontext->bottomtexture]>>FRACBITS;
						dc->func(dc);
						planecontext->floorclip[currx] = (INT16)mid;
					}
					else if (!segcontext->floormarked)  // entirely off bottom of screen
						planecontext->floorclip[currx] = (INT16)viewheight;
				}
				else if (!segcontext->floormarked)
					planecontext->floorclip[currx] = bottomclip;
			}
			else if (segcontext->markfloor && (!segcontext->floormarked)) // no bottom wall
				planecontext->floorclip[currx] = bottomclip;
		}

		if (segcontext->maskedtexture || segcontext->numthicksides)
		{
			// save texturecol
			//  for backdrawing of masked mid texture
			segcontext->maskedtexturecol[currx] = (INT16)texturecolumn;

			if (segcontext->maskedtextureheight != NULL) {
				segcontext->maskedtextureheight[currx] = (!!(bspcontext->curline->linedef->flags & ML_DONTPEGBOTTOM) ^ !!(bspcontext->curline->linedef->flags & ML_EFFECT3) ?
											max(wallcontext->midtexturemid, wallcontext->midtextureback) :
											min(wallcontext->midtexturemid, wallcontext->midtextureback));
			}
		}

		for (i = 0; i < dc->numlights; i++)
		{
			dc->lightlist[i].height += dc->lightlist[i].heightstep;
			if (dc->lightlist[i].flags & FF_CUTSOLIDS)
				dc->lightlist[i].botheight += dc->lightlist[i].botheightstep;
		}

		for (i = 0; i < planecontext->numffloors; i++)
		{
			if (bspcontext->curline->polyseg && (planecontext->ffloor[i].polyobj != bspcontext->curline->polyseg))
				continue;

			planecontext->ffloor[i].f_frac += planecontext->ffloor[i].f_step;
		}

		for (i = 0; i < segcontext->numbackffloors; i++)
		{
			if (bspcontext->curline->polyseg && (planecontext->ffloor[i].polyobj != bspcontext->curline->polyseg))
				continue;

			planecontext->ffloor[i].f_clip[currx] = planecontext->ffloor[i].c_clip[currx] = (INT16)((planecontext->ffloor[i].b_frac >> HEIGHTBITS) & 0xFFFF);
			planecontext->ffloor[i].b_frac += planecontext->ffloor[i].b_step;
		}

		wallcontext->scale += wallcontext->scalestep;
		segcontext->topfrac += segcontext->topstep;
		segcontext->bottomfrac += segcontext->bottomstep;
	}
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
// killough 5/2/98: reformatted, cleaned up
//
// note: THIS IS USED ONLY FOR WALLS!
static fixed_t R_ScaleFromGlobalAngle(wallcontext_t *wallcontext, angle_t anglea, angle_t angleb)
{
	fixed_t den, num;

	angleb += anglea;

	anglea = ANGLE_90 + (angleb - anglea);
	angleb = ANGLE_90 + (angleb - wallcontext->normalangle);

	den = FixedMul(wallcontext->distance, FINESINE(anglea>>ANGLETOFINESHIFT));
	num = FixedMul(projectiony, FINESINE(angleb>>ANGLETOFINESHIFT)); // proff 11/06/98: Changed for high-res

	if (den > num>>16)
	{
		num = FixedDiv(num, den);
		if (num > 64*FRACUNIT)
			return 64*FRACUNIT;
		if (num < 256)
			return 256;
		return num;
	}

	return 64*FRACUNIT;
}

// Uses precalculated seg->length
static INT64 R_CalcSegDist(seg_t* seg, INT64 x2, INT64 y2)
{
	if (!seg->linedef->dy)
		return llabs(y2 - seg->v1->y);
	else if (!seg->linedef->dx)
		return llabs(x2 - seg->v1->x);
	else
	{
		INT64 dx = (seg->v2->x)-(seg->v1->x);
		INT64 dy = (seg->v2->y)-(seg->v1->y);
		INT64 vdx = x2-(seg->v1->x);
		INT64 vdy = y2-(seg->v1->y);
		return ((dy*vdx)-(dx*vdy))/(seg->length);
	}
}

//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange(rendercontext_t *context, wallcontext_t *wallcontext, INT32 start, INT32 stop)
{
	bspcontext_t *bspcontext = &context->bspcontext;
	planecontext_t *planecontext = &context->planecontext;
	viewcontext_t *viewcontext = &context->viewcontext;

	sector_t *frontsector = bspcontext->frontsector;
	sector_t *backsector = bspcontext->backsector;

	INT32 worldtop, worldbottom, worldhigh, worldlow;
	INT32 worldtopslope, worldbottomslope, worldhighslope, worldlowslope; // worldtop/bottom at end of slope

	drawseg_t *ds_p;
	segloopcontext_t loopcontext;
	colcontext_t *dc = &context->colcontext;

	fixed_t hyp, sineval;
	angle_t distangle, offsetangle;
	boolean longboi;

	INT32 range;
	INT32 i, p;

	INT32 lightnum;
	lightlist_t *light;
	r_lightlist_t *rlight;

	vertex_t segleft, segright;
	fixed_t ceilingfrontslide, floorfrontslide, ceilingbackslide, floorbackslide;

	memset(&loopcontext, 0x00, sizeof(loopcontext));

	// initialize segleft and segright
	memset(&segleft, 0x00, sizeof(segleft));
	memset(&segright, 0x00, sizeof(segright));

	dc->func = colfuncs[BASEDRAWFUNC];

	if (bspcontext->ds_p == bspcontext->drawsegs+bspcontext->maxdrawsegs)
	{
		size_t curpos = bspcontext->curdrawsegs - bspcontext->drawsegs;
		size_t pos = bspcontext->ds_p - bspcontext->drawsegs;
		size_t newmax = bspcontext->maxdrawsegs ? bspcontext->maxdrawsegs*2 : 128;
		if (bspcontext->firstseg)
			bspcontext->firstseg = (drawseg_t *)(bspcontext->firstseg - bspcontext->drawsegs);
		bspcontext->drawsegs = Z_Realloc(bspcontext->drawsegs, newmax*sizeof (*bspcontext->drawsegs), PU_STATIC, NULL);
		bspcontext->ds_p = bspcontext->drawsegs + pos;
		bspcontext->maxdrawsegs = newmax;
		bspcontext->curdrawsegs = bspcontext->drawsegs + curpos;
		if (bspcontext->firstseg)
			bspcontext->firstseg = bspcontext->drawsegs + (size_t)bspcontext->firstseg;
	}

	ds_p = bspcontext->ds_p;

	bspcontext->sidedef = bspcontext->curline->sidedef;
	bspcontext->linedef = bspcontext->curline->linedef;

	// calculate wallcontext->distance for scale calculation
	wallcontext->normalangle = bspcontext->curline->angle + ANGLE_90;
	offsetangle = abs((INT32)(wallcontext->normalangle-wallcontext->angle1));

	if (offsetangle > ANGLE_90)
		offsetangle = ANGLE_90;

	distangle = ANGLE_90 - offsetangle;
	sineval = FINESINE(distangle>>ANGLETOFINESHIFT);

	hyp = R_PointToDist2(viewcontext->x, viewcontext->y, bspcontext->curline->v1->x, bspcontext->curline->v1->y);
	wallcontext->distance = FixedMul(hyp, sineval);
	longboi = (hyp >= INT32_MAX);

	// big room fix
	if (longboi)
		wallcontext->distance = (fixed_t)R_CalcSegDist(bspcontext->curline, viewcontext->x, viewcontext->y);

	ds_p->x1 = loopcontext.x = start;
	ds_p->x2 = stop;
	ds_p->curline = bspcontext->curline;
	loopcontext.stopx = stop+1;

	//SoM: Code to remove limits on openings.
	{
		size_t pos = planecontext->lastopening - planecontext->openings;
		size_t need = (loopcontext.stopx - start)*4 + pos;
		if (need > planecontext->maxopenings)
		{
			drawseg_t *ds;  //needed for fix from *cough* zdoom *cough*
			INT16 *oldopenings = planecontext->openings;
			INT16 *oldlast = planecontext->lastopening;

			do
				planecontext->maxopenings = planecontext->maxopenings ? planecontext->maxopenings*2 : 16384;
			while (need > planecontext->maxopenings);
			planecontext->openings = Z_Realloc(planecontext->openings, planecontext->maxopenings * sizeof (*planecontext->openings), PU_STATIC, NULL);
			planecontext->lastopening = planecontext->openings + pos;

			// borrowed fix from *cough* zdoom *cough*
			// [RH] We also need to adjust the openings pointers that
			//    were already stored in drawsegs.
			for (ds = bspcontext->drawsegs; ds < ds_p; ds++)
			{
#define ADJUST(p) if (ds->p + ds->x1 >= oldopenings && ds->p + ds->x1 <= oldlast) ds->p = ds->p - oldopenings + planecontext->openings;
				ADJUST(maskedtexturecol);
				ADJUST(sprtopclip);
				ADJUST(sprbottomclip);
				ADJUST(thicksidecol);
#undef ADJUST
			}
		}
	}  // end of code to remove limits on openings

	// calculate scale at both ends and step
	ds_p->scale1 = wallcontext->scale = R_ScaleFromGlobalAngle(wallcontext, viewcontext->angle, xtoviewangle[start]);

	if (stop > start)
	{
		ds_p->scale2 = R_ScaleFromGlobalAngle(wallcontext, viewcontext->angle, xtoviewangle[stop]);
		range = stop-start;
	}
	else
	{
		// UNUSED: try to fix the stretched line bug
#if 0
		if (wallcontext->distance < FRACUNIT/2)
		{
			fixed_t         tr_x,tr_y;
			fixed_t         gxt,gyt;
			CONS_Debug(DBG_RENDER, "TRYING TO FIX THE STRETCHED ETC\n");

			tr_x = bspcontext->curline->v1->x - viewcontext->x;
			tr_y = bspcontext->curline->v1->y - viewcontext->y;

			gxt = FixedMul(tr_x, viewcontext->cos);
			gyt = -FixedMul(tr_y, viewcontext->sin);
			ds_p->scale1 = FixedDiv(projection, gxt - gyt);
		}
#endif
		ds_p->scale2 = ds_p->scale1;
		range = 1;
	}

	ds_p->scalestep = wallcontext->scalestep = (ds_p->scale2 - wallcontext->scale) / (range);

	// calculate texture boundaries
	//  and decide if floor / ceiling marks are needed
	// Figure out map coordinates of where start and end are mapping to on seg, so we can clip right for slope bullshit
	if (frontsector->hasslope || (backsector && backsector->hasslope)) // Commenting this out for FOFslop. -Red
	{
		angle_t temp;

		// left
		temp = xtoviewangle[start]+viewcontext->angle;

#define FIXED_TO_DOUBLE(x) (((double)(x)) / ((double)FRACUNIT))
#define DOUBLE_TO_FIXED(x) (fixed_t)((x) * ((double)FRACUNIT))

		{
			// Both lines can be written in slope-intercept form, so figure out line intersection
			double a1, b1, c1, a2, b2, c2, det; // 1 is the seg, 2 is the view angle vector...
			///TODO: convert to fixed point

			a1 = FIXED_TO_DOUBLE(bspcontext->curline->v2->y-bspcontext->curline->v1->y);
			b1 = FIXED_TO_DOUBLE(bspcontext->curline->v1->x-bspcontext->curline->v2->x);
			c1 = a1*FIXED_TO_DOUBLE(bspcontext->curline->v1->x) + b1*FIXED_TO_DOUBLE(bspcontext->curline->v1->y);

			a2 = -FIXED_TO_DOUBLE(FINESINE(temp>>ANGLETOFINESHIFT));
			b2 = FIXED_TO_DOUBLE(FINECOSINE(temp>>ANGLETOFINESHIFT));
			c2 = a2*FIXED_TO_DOUBLE(viewcontext->x) + b2*FIXED_TO_DOUBLE(viewcontext->y);

			det = a1*b2 - a2*b1;

			ds_p->leftpos.x = segleft.x = DOUBLE_TO_FIXED((b2*c1 - b1*c2)/det);
			ds_p->leftpos.y = segleft.y = DOUBLE_TO_FIXED((a1*c2 - a2*c1)/det);
		}

		// right
		temp = xtoviewangle[stop]+viewcontext->angle;

		{
			// Both lines can be written in slope-intercept form, so figure out line intersection
			double a1, b1, c1, a2, b2, c2, det; // 1 is the seg, 2 is the view angle vector...
			///TODO: convert to fixed point

			a1 = FIXED_TO_DOUBLE(bspcontext->curline->v2->y-bspcontext->curline->v1->y);
			b1 = FIXED_TO_DOUBLE(bspcontext->curline->v1->x-bspcontext->curline->v2->x);
			c1 = a1*FIXED_TO_DOUBLE(bspcontext->curline->v1->x) + b1*FIXED_TO_DOUBLE(bspcontext->curline->v1->y);

			a2 = -FIXED_TO_DOUBLE(FINESINE(temp>>ANGLETOFINESHIFT));
			b2 = FIXED_TO_DOUBLE(FINECOSINE(temp>>ANGLETOFINESHIFT));
			c2 = a2*FIXED_TO_DOUBLE(viewcontext->x) + b2*FIXED_TO_DOUBLE(viewcontext->y);

			det = a1*b2 - a2*b1;

			ds_p->rightpos.x = segright.x = DOUBLE_TO_FIXED((b2*c1 - b1*c2)/det);
			ds_p->rightpos.y = segright.y = DOUBLE_TO_FIXED((a1*c2 - a2*c1)/det);
		}

#undef FIXED_TO_DOUBLE
#undef DOUBLE_TO_FIXED

	}


#define SLOPEPARAMS(slope, end1, end2, normalheight) \
	end1 = P_GetZAt(slope,  segleft.x,  segleft.y, normalheight); \
	end2 = P_GetZAt(slope, segright.x, segright.y, normalheight);

	SLOPEPARAMS(frontsector->c_slope, worldtop,    worldtopslope,    frontsector->ceilingheight)
	SLOPEPARAMS(frontsector->f_slope, worldbottom, worldbottomslope, frontsector->floorheight)
	// subtract viewz from these to turn them into
	// positions relative to the camera's z position
	worldtop -= viewcontext->z;
	worldtopslope -= viewcontext->z;
	worldbottom -= viewcontext->z;
	worldbottomslope -= viewcontext->z;

	loopcontext.midtexture = loopcontext.toptexture = loopcontext.bottomtexture = 0;
	loopcontext.maskedtexture = false;
	ds_p->maskedtexturecol = NULL;
	ds_p->numthicksides = loopcontext.numthicksides = 0;
	ds_p->thicksidecol = NULL;
	ds_p->tsilheight = 0;

	loopcontext.numbackffloors = 0;

	for (i = 0; i < MAXFFLOORS; i++)
		ds_p->thicksides[i] = NULL;

	for (i = 0; i < planecontext->numffloors; i++)
	{
		if (planecontext->ffloor[i].polyobj && (!ds_p->curline->polyseg || planecontext->ffloor[i].polyobj != ds_p->curline->polyseg))
			continue;

		planecontext->ffloor[i].f_pos       = P_GetZAt(planecontext->ffloor[i].slope, segleft .x, segleft .y, planecontext->ffloor[i].height) - viewcontext->z;
		planecontext->ffloor[i].f_pos_slope = P_GetZAt(planecontext->ffloor[i].slope, segright.x, segright.y, planecontext->ffloor[i].height) - viewcontext->z;
	}

	// Set up texture Y offset slides for sloped walls
	wallcontext->toptextureslide = wallcontext->midtextureslide = wallcontext->bottomtextureslide = 0;
	ceilingfrontslide = floorfrontslide = ceilingbackslide = floorbackslide = 0;

	{
		angle_t lineangle = R_PointToAngle2(bspcontext->curline->v1->x, bspcontext->curline->v1->y, bspcontext->curline->v2->x, bspcontext->curline->v2->y);

		if (frontsector->f_slope)
			floorfrontslide = FixedMul(frontsector->f_slope->zdelta, FINECOSINE((lineangle-frontsector->f_slope->xydirection)>>ANGLETOFINESHIFT));

		if (frontsector->c_slope)
			ceilingfrontslide = FixedMul(frontsector->c_slope->zdelta, FINECOSINE((lineangle-frontsector->c_slope->xydirection)>>ANGLETOFINESHIFT));

		if (backsector && backsector->f_slope)
			floorbackslide = FixedMul(backsector->f_slope->zdelta, FINECOSINE((lineangle-backsector->f_slope->xydirection)>>ANGLETOFINESHIFT));

		if (backsector && backsector->c_slope)
			ceilingbackslide = FixedMul(backsector->c_slope->zdelta, FINECOSINE((lineangle-backsector->c_slope->xydirection)>>ANGLETOFINESHIFT));
	}

	if (!backsector)
	{
		fixed_t texheight;
		// single sided line
		loopcontext.midtexture = R_GetTextureNum(bspcontext->sidedef->midtexture);
		texheight = textureheight[loopcontext.midtexture];
		// a single sided line is terminal, so it must mark ends
		loopcontext.markfloor = loopcontext.markceiling = true;
		if (bspcontext->linedef->flags & ML_EFFECT2) {
			if (bspcontext->linedef->flags & ML_DONTPEGBOTTOM)
				wallcontext->midtexturemid = frontsector->floorheight + texheight - viewcontext->z;
			else
				wallcontext->midtexturemid = frontsector->ceilingheight - viewcontext->z;
		}
		else if (bspcontext->linedef->flags & ML_DONTPEGBOTTOM)
		{
			wallcontext->midtexturemid = worldbottom + texheight;
			wallcontext->midtextureslide = floorfrontslide;
		}
		else
		{
			// top of texture at top
			wallcontext->midtexturemid = worldtop;
			wallcontext->midtextureslide = ceilingfrontslide;
		}
		wallcontext->midtexturemid += bspcontext->sidedef->rowoffset;

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = screenheightarray;
		ds_p->sprbottomclip = negonearray;
		ds_p->bsilheight = INT32_MAX;
		ds_p->tsilheight = INT32_MIN;
	}
	else
	{
		// two sided line
		boolean bothceilingssky = false; // turned on if both back and front ceilings are sky
		boolean bothfloorssky = false; // likewise, but for floors

		SLOPEPARAMS(backsector->c_slope, worldhigh, worldhighslope, backsector->ceilingheight)
		SLOPEPARAMS(backsector->f_slope, worldlow,  worldlowslope,  backsector->floorheight)
		worldhigh -= viewcontext->z;
		worldhighslope -= viewcontext->z;
		worldlow -= viewcontext->z;
		worldlowslope -= viewcontext->z;

		// hack to allow height changes in outdoor areas
		// This is what gets rid of the upper textures if there should be sky
		if (frontsector->ceilingpic == skyflatnum
			&& backsector->ceilingpic == skyflatnum)
		{
			bothceilingssky = true;
		}

		// likewise, but for floors and upper textures
		if (frontsector->floorpic == skyflatnum
			&& backsector->floorpic == skyflatnum)
		{
			bothfloorssky = true;
		}

		ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
		ds_p->silhouette = 0;

		if (!bothfloorssky)
		{
			if (worldbottomslope > worldlowslope || worldbottom > worldlow)
			{
				ds_p->silhouette = SIL_BOTTOM;
				if (P_GetSectorFloorZAt(backsector, viewcontext->x, viewcontext->y) > viewcontext->z)
					ds_p->bsilheight = INT32_MAX;
				else
					ds_p->bsilheight = (frontsector->f_slope ? INT32_MAX : frontsector->floorheight);
			}
			else if (P_GetSectorFloorZAt(backsector, viewcontext->x, viewcontext->y) > viewcontext->z)
			{
				ds_p->silhouette = SIL_BOTTOM;
				ds_p->bsilheight = INT32_MAX;
				// ds_p->sprbottomclip = negonearray;
			}
		}

		if (!bothceilingssky)
		{
			if (worldtopslope < worldhighslope || worldtop < worldhigh)
			{
				ds_p->silhouette |= SIL_TOP;
				if (P_GetSectorCeilingZAt(backsector, viewcontext->x, viewcontext->y) < viewcontext->z)
					ds_p->tsilheight = INT32_MIN;
				else
					ds_p->tsilheight = (frontsector->c_slope ? INT32_MIN : frontsector->ceilingheight);
			}
			else if (P_GetSectorCeilingZAt(backsector, viewcontext->x, viewcontext->y) < viewcontext->z)
			{
				ds_p->silhouette |= SIL_TOP;
				ds_p->tsilheight = INT32_MIN;
				// ds_p->sprtopclip = screenheightarray;
			}
		}

		if (!bothceilingssky && !bothfloorssky)
		{
			if (worldhigh <= worldbottom && worldhighslope <= worldbottomslope)
			{
				ds_p->sprbottomclip = negonearray;
				ds_p->bsilheight = INT32_MAX;
				ds_p->silhouette |= SIL_BOTTOM;
			}

			if (worldlow >= worldtop && worldlowslope >= worldtopslope)
			{
				ds_p->sprtopclip = screenheightarray;
				ds_p->tsilheight = INT32_MIN;
				ds_p->silhouette |= SIL_TOP;
			}
		}

		//SoM: 3/25/2000: This code fixes an automap bug that didn't check
		// frontsector->ceiling and backsector->floor to see if a door was closed.
		// Without the following code, sprites get displayed behind closed doors.
		if (!bothceilingssky && !bothfloorssky)
		{
			if (bspcontext->doorclosed || (worldhigh <= worldbottom && worldhighslope <= worldbottomslope))
			{
				ds_p->sprbottomclip = negonearray;
				ds_p->bsilheight = INT32_MAX;
				ds_p->silhouette |= SIL_BOTTOM;
			}
			if (bspcontext->doorclosed || (worldlow >= worldtop && worldlowslope >= worldtopslope))
			{                   // killough 1/17/98, 2/8/98
				ds_p->sprtopclip = screenheightarray;
				ds_p->tsilheight = INT32_MIN;
				ds_p->silhouette |= SIL_TOP;
			}
		}

		if (bothfloorssky)
		{
			// see double ceiling skies comment
			// this is the same but for upside down thok barriers where the floor is sky and the ceiling is normal
			loopcontext.markfloor = false;
		}
		else if (worldlow != worldbottom
			|| worldlowslope != worldbottomslope
			|| backsector->f_slope != frontsector->f_slope
		    || backsector->floorpic != frontsector->floorpic
		    || backsector->lightlevel != frontsector->lightlevel
		    //SoM: 3/22/2000: Check floor x and y offsets.
		    || backsector->floor_xoffs != frontsector->floor_xoffs
		    || backsector->floor_yoffs != frontsector->floor_yoffs
		    || backsector->floorpic_angle != frontsector->floorpic_angle
		    //SoM: 3/22/2000: Prevents bleeding.
		    || (frontsector->heightsec != -1 && frontsector->floorpic != skyflatnum)
		    || backsector->floorlightsec != frontsector->floorlightsec
		    //SoM: 4/3/2000: Check for colormaps
		    || frontsector->extra_colormap != backsector->extra_colormap
		    || (frontsector->ffloors != backsector->ffloors && !Tag_Compare(&frontsector->tags, &backsector->tags)))
		{
			loopcontext.markfloor = true;
		}
		else
		{
			// same plane on both sides
			loopcontext.markfloor = false;
		}

		if (bothceilingssky)
		{
			// double ceiling skies are special
			// we don't want to lower the ceiling clipping, (no new plane is drawn anyway)
			// so we can see the floor of thok barriers always regardless of sector properties
			loopcontext.markceiling = false;
		}
		else if (worldhigh != worldtop
			|| worldhighslope != worldtopslope
			|| backsector->c_slope != frontsector->c_slope
		    || backsector->ceilingpic != frontsector->ceilingpic
		    || backsector->lightlevel != frontsector->lightlevel
		    //SoM: 3/22/2000: Check floor x and y offsets.
		    || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
		    || backsector->ceiling_yoffs != frontsector->ceiling_yoffs
		    || backsector->ceilingpic_angle != frontsector->ceilingpic_angle
		    //SoM: 3/22/2000: Prevents bleeding.
		    || (frontsector->heightsec != -1 && frontsector->ceilingpic != skyflatnum)
		    || backsector->ceilinglightsec != frontsector->ceilinglightsec
		    //SoM: 4/3/2000: Check for colormaps
		    || frontsector->extra_colormap != backsector->extra_colormap
		    || (frontsector->ffloors != backsector->ffloors && !Tag_Compare(&frontsector->tags, &backsector->tags)))
		{
				loopcontext.markceiling = true;
		}
		else
		{
			// same plane on both sides
			loopcontext.markceiling = false;
		}

		if (!bothceilingssky && !bothfloorssky)
		{
			if ((worldhigh <= worldbottom && worldhighslope <= worldbottomslope)
			 || (worldlow >= worldtop && worldlowslope >= worldtopslope))
			{
				// closed door
				loopcontext.markceiling = loopcontext.markfloor = true;
			}
		}

		// check TOP TEXTURE
		if (!bothceilingssky // never draw the top texture if on
			&& (worldhigh < worldtop || worldhighslope < worldtopslope))
		{
			fixed_t texheight;
			// top texture
			if ((bspcontext->linedef->flags & (ML_DONTPEGTOP) && (bspcontext->linedef->flags & ML_DONTPEGBOTTOM))
				&& bspcontext->linedef->sidenum[1] != 0xffff)
			{
				// Special case... use offsets from 2nd side but only if it has a texture.
				side_t *def = &sides[bspcontext->linedef->sidenum[1]];
				loopcontext.toptexture = R_GetTextureNum(def->toptexture);

				if (!loopcontext.toptexture) //Second side has no texture, use the first side's instead.
					loopcontext.toptexture = R_GetTextureNum(bspcontext->sidedef->toptexture);
				texheight = textureheight[loopcontext.toptexture];
			}
			else
			{
				loopcontext.toptexture = R_GetTextureNum(bspcontext->sidedef->toptexture);
				texheight = textureheight[loopcontext.toptexture];
			}
			if (!(bspcontext->linedef->flags & ML_EFFECT1)) { // Ignore slopes for lower/upper textures unless flag is checked
				if (bspcontext->linedef->flags & ML_DONTPEGTOP)
					wallcontext->toptexturemid = frontsector->ceilingheight - viewcontext->z;
				else
					wallcontext->toptexturemid = backsector->ceilingheight - viewcontext->z;
			}
			else if (bspcontext->linedef->flags & ML_DONTPEGTOP)
			{
				// top of texture at top
				wallcontext->toptexturemid = worldtop;
				wallcontext->toptextureslide = ceilingfrontslide;
			}
			else
			{
				wallcontext->toptexturemid = worldhigh + texheight;
				wallcontext->toptextureslide = ceilingbackslide;
			}
		}
		// check BOTTOM TEXTURE
		if (!bothfloorssky // never draw the bottom texture if on
			&& (worldlow > worldbottom || worldlowslope > worldbottomslope)) // Only if VISIBLE!!!
		{
			// bottom texture
			loopcontext.bottomtexture = R_GetTextureNum(bspcontext->sidedef->bottomtexture);

			if (!(bspcontext->linedef->flags & ML_EFFECT1)) { // Ignore slopes for lower/upper textures unless flag is checked
				if (bspcontext->linedef->flags & ML_DONTPEGBOTTOM)
					wallcontext->bottomtexturemid = frontsector->floorheight - viewcontext->z;
				else
					wallcontext->bottomtexturemid = backsector->floorheight - viewcontext->z;
			}
			else if (bspcontext->linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom
				// top of texture at top
				wallcontext->bottomtexturemid = worldbottom;
				wallcontext->bottomtextureslide = floorfrontslide;
			}
			else {   // top of texture at top
				wallcontext->bottomtexturemid = worldlow;
				wallcontext->bottomtextureslide = floorbackslide;
			}
		}

		wallcontext->toptexturemid += bspcontext->sidedef->rowoffset;
		wallcontext->bottomtexturemid += bspcontext->sidedef->rowoffset;

		// allocate space for masked texture tables
		if (frontsector && backsector && !Tag_Compare(&frontsector->tags, &backsector->tags) && (backsector->ffloors || frontsector->ffloors))
		{
			ffloor_t *rover;
			ffloor_t *r2;
			fixed_t   lowcut, highcut;
			fixed_t lowcutslope, highcutslope;

			// Used for height comparisons and etc across FOFs and slopes
			fixed_t high1, highslope1, low1, lowslope1, high2, highslope2, low2, lowslope2;

			//loopcontext.markceiling = loopcontext.markfloor = true;
			loopcontext.maskedtexture = true;

			ds_p->thicksidecol = loopcontext.maskedtexturecol = planecontext->lastopening - loopcontext.x;
			planecontext->lastopening += loopcontext.stopx - loopcontext.x;

			lowcut = max(worldbottom, worldlow) + viewcontext->z;
			highcut = min(worldtop, worldhigh) + viewcontext->z;
			lowcutslope = max(worldbottomslope, worldlowslope) + viewcontext->z;
			highcutslope = min(worldtopslope, worldhighslope) + viewcontext->z;

			if (frontsector->ffloors && backsector->ffloors)
			{
				i = 0;
				for (rover = backsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS))
						continue;
					if (!(rover->flags & FF_ALLSIDES) && rover->flags & FF_INVERTSIDES)
						continue;

					if (rover->norender == leveltime)
						continue;

					SLOPEPARAMS(*rover->t_slope, high1, highslope1, *rover->topheight)
					SLOPEPARAMS(*rover->b_slope, low1,  lowslope1,  *rover->bottomheight)

					if ((high1 < lowcut && highslope1 < lowcutslope) || (low1 > highcut && lowslope1 > highcutslope))
						continue;

					for (r2 = frontsector->ffloors; r2; r2 = r2->next)
					{
						if (r2->master == rover->master) // Skip if same control line.
							break;

						if (!(r2->flags & FF_EXISTS) || !(r2->flags & FF_RENDERSIDES))
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

						SLOPEPARAMS(*r2->t_slope, high2, highslope2, *r2->topheight)
						SLOPEPARAMS(*r2->b_slope, low2,  lowslope2,  *r2->bottomheight)

						if ((high2 < lowcut || highslope2 < lowcutslope) || (low2 > highcut || lowslope2 > highcutslope))
							continue;
						if ((high1 > high2 || highslope1 > highslope2) || (low1 < low2 || lowslope1 < lowslope2))
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
					if (!(rover->flags & FF_ALLSIDES || rover->flags & FF_INVERTSIDES))
						continue;

					if (rover->norender == leveltime)
						continue;

					SLOPEPARAMS(*rover->t_slope, high1, highslope1, *rover->topheight)
					SLOPEPARAMS(*rover->b_slope, low1,  lowslope1,  *rover->bottomheight)

					if ((high1 < lowcut && highslope1 < lowcutslope) || (low1 > highcut && lowslope1 > highcutslope))
						continue;

					for (r2 = backsector->ffloors; r2; r2 = r2->next)
					{
						if (r2->master == rover->master) // Skip if same control line.
							break;

						if (!(r2->flags & FF_EXISTS) || !(r2->flags & FF_RENDERSIDES))
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

						SLOPEPARAMS(*r2->t_slope, high2, highslope2, *r2->topheight)
						SLOPEPARAMS(*r2->b_slope, low2,  lowslope2,  *r2->bottomheight)
#undef SLOPEPARAMS
						if ((high2 < lowcut || highslope2 < lowcutslope) || (low2 > highcut || lowslope2 > highcutslope))
							continue;
						if ((high1 > high2 || highslope1 > highslope2) || (low1 < low2 || lowslope1 < lowslope2))
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
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS))
						continue;
					if (!(rover->flags & FF_ALLSIDES) && rover->flags & FF_INVERTSIDES)
						continue;
					if (rover->norender == leveltime)
						continue;

					// Oy vey.
					if (      ((P_GetFFloorTopZAt   (rover, segleft .x, segleft .y)) <= worldbottom      + viewcontext->z
					        && (P_GetFFloorTopZAt   (rover, segright.x, segright.y)) <= worldbottomslope + viewcontext->z)
					        ||((P_GetFFloorBottomZAt(rover, segleft .x, segleft .y)) >= worldtop         + viewcontext->z
					        && (P_GetFFloorBottomZAt(rover, segright.x, segright.y)) >= worldtopslope    + viewcontext->z))
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}
			else if (frontsector->ffloors)
			{
				for (rover = frontsector->ffloors, i = 0; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_RENDERSIDES) || !(rover->flags & FF_EXISTS))
						continue;
					if (!(rover->flags & FF_ALLSIDES || rover->flags & FF_INVERTSIDES))
						continue;
					if (rover->norender == leveltime)
						continue;
					// Oy vey.
					if (      (P_GetFFloorTopZAt   (rover, segleft .x, segleft .y) <= worldbottom      + viewcontext->z
					        && P_GetFFloorTopZAt   (rover, segright.x, segright.y) <= worldbottomslope + viewcontext->z)
					        ||(P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) >= worldtop         + viewcontext->z
					        && P_GetFFloorBottomZAt(rover, segright.x, segright.y) >= worldtopslope    + viewcontext->z))
						continue;

					if (      (P_GetFFloorTopZAt   (rover, segleft .x, segleft .y) <= worldlow       + viewcontext->z
					        && P_GetFFloorTopZAt   (rover, segright.x, segright.y) <= worldlowslope  + viewcontext->z)
					        ||(P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) >= worldhigh      + viewcontext->z
					        && P_GetFFloorBottomZAt(rover, segright.x, segright.y) >= worldhighslope + viewcontext->z))
						continue;

					ds_p->thicksides[i] = rover;
					i++;
				}
			}

			ds_p->numthicksides = loopcontext.numthicksides = i;
		}
		if (bspcontext->sidedef->midtexture > 0 && bspcontext->sidedef->midtexture < numtextures)
		{
			// masked midtexture
			if (!ds_p->thicksidecol)
			{
				ds_p->maskedtexturecol = loopcontext.maskedtexturecol = planecontext->lastopening - loopcontext.x;
				planecontext->lastopening += loopcontext.stopx - loopcontext.x;
			}
			else
				ds_p->maskedtexturecol = ds_p->thicksidecol;

			loopcontext.maskedtextureheight = ds_p->maskedtextureheight; // note to red, this == &(ds_p->maskedtextureheight[0])

			if (bspcontext->curline->polyseg)
			{ // use REAL front and back floors please, so midtexture rendering isn't mucked up
				wallcontext->midtextureslide = wallcontext->midtexturebackslide = 0;
				if (!!(bspcontext->linedef->flags & ML_DONTPEGBOTTOM) ^ !!(bspcontext->linedef->flags & ML_EFFECT3))
					wallcontext->midtexturemid = wallcontext->midtextureback = max(bspcontext->curline->frontsector->floorheight, bspcontext->curline->backsector->floorheight) - viewcontext->z;
				else
					wallcontext->midtexturemid = wallcontext->midtextureback = min(bspcontext->curline->frontsector->ceilingheight, bspcontext->curline->backsector->ceilingheight) - viewcontext->z;
			}
			else
			{
				// Set midtexture starting height
				if (bspcontext->linedef->flags & ML_EFFECT2)
				{ // Ignore slopes when texturing
					wallcontext->midtextureslide = wallcontext->midtexturebackslide = 0;
					if (!!(bspcontext->linedef->flags & ML_DONTPEGBOTTOM) ^ !!(bspcontext->linedef->flags & ML_EFFECT3))
						wallcontext->midtexturemid = wallcontext->midtextureback = max(frontsector->floorheight, backsector->floorheight) - viewcontext->z;
					else
						wallcontext->midtexturemid = wallcontext->midtextureback = min(frontsector->ceilingheight, backsector->ceilingheight) - viewcontext->z;

				}
				else if (!!(bspcontext->linedef->flags & ML_DONTPEGBOTTOM) ^ !!(bspcontext->linedef->flags & ML_EFFECT3))
				{
					wallcontext->midtexturemid = worldbottom;
					wallcontext->midtextureslide = floorfrontslide;
					wallcontext->midtextureback = worldlow;
					wallcontext->midtexturebackslide = floorbackslide;
				}
				else
				{
					wallcontext->midtexturemid = worldtop;
					wallcontext->midtextureslide = ceilingfrontslide;
					wallcontext->midtextureback = worldhigh;
					wallcontext->midtexturebackslide = ceilingbackslide;
				}
			}
			wallcontext->midtexturemid += bspcontext->sidedef->rowoffset;
			wallcontext->midtextureback += bspcontext->sidedef->rowoffset;

			loopcontext.maskedtexture = true;
		}
	}

	// calculate wallcontext->offset (only needed for textured lines)
	loopcontext.segtextured = loopcontext.midtexture || loopcontext.toptexture || loopcontext.bottomtexture || loopcontext.maskedtexture || (loopcontext.numthicksides > 0);

	if (loopcontext.segtextured)
	{
		offsetangle = wallcontext->normalangle-wallcontext->angle1;

		if (offsetangle > ANGLE_180)
			offsetangle = -(signed)offsetangle;

		if (offsetangle > ANGLE_90)
			offsetangle = ANGLE_90;

		sineval = FINESINE(offsetangle>>ANGLETOFINESHIFT);
		wallcontext->offset = FixedMul(hyp, sineval);

		// big room fix
		if (longboi)
		{
			INT64 dx = (bspcontext->curline->v2->x)-(bspcontext->curline->v1->x);
			INT64 dy = (bspcontext->curline->v2->y)-(bspcontext->curline->v1->y);
			INT64 vdx = viewcontext->x-(bspcontext->curline->v1->x);
			INT64 vdy = viewcontext->y-(bspcontext->curline->v1->y);
			wallcontext->offset = ((dx*vdx-dy*vdy))/(bspcontext->curline->length);
		}

		if (wallcontext->normalangle-wallcontext->angle1 < ANGLE_180)
			wallcontext->offset = -wallcontext->offset;

		/// don't use texture offset for splats
		wallcontext->offset2 = wallcontext->offset + bspcontext->curline->offset;
		wallcontext->offset += bspcontext->sidedef->textureoffset + bspcontext->curline->offset;
		wallcontext->centerangle = ANGLE_90 + viewcontext->angle - wallcontext->normalangle;

		// calculate light table
		//  use different light tables
		//  for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT);

		if (bspcontext->curline->v1->y == bspcontext->curline->v2->y)
			lightnum--;
		else if (bspcontext->curline->v1->x == bspcontext->curline->v2->x)
			lightnum++;

		if (lightnum < 0)
			loopcontext.walllights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			loopcontext.walllights = scalelight[LIGHTLEVELS - 1];
		else
			loopcontext.walllights = scalelight[lightnum];
	}

	// if a floor / ceiling plane is on the wrong side
	//  of the view plane, it is definitely invisible
	//  and doesn't need to be marked.
	if (frontsector->heightsec == -1)
	{
		if (frontsector->floorpic != skyflatnum && P_GetSectorFloorZAt(frontsector, viewcontext->x, viewcontext->y) >= viewcontext->z)
		{
			// above view plane
			loopcontext.markfloor = false;
		}

		if (frontsector->ceilingpic != skyflatnum && P_GetSectorCeilingZAt(frontsector, viewcontext->x, viewcontext->y) <= viewcontext->z)
		{
			// below view plane
			loopcontext.markceiling = false;
		}
	}

	// calculate incremental stepping values for texture edges
	worldtop >>= 4;
	worldbottom >>= 4;
	worldtopslope >>= 4;
	worldbottomslope >>= 4;

	if (bspcontext->linedef->special == HORIZONSPECIAL) { // HORIZON LINES
		loopcontext.topstep = loopcontext.bottomstep = 0;
		loopcontext.topfrac = loopcontext.bottomfrac = (centeryfrac>>4);
		loopcontext.topfrac++; // Prevent 1px HOM
	} else {
		loopcontext.topstep = -FixedMul (wallcontext->scalestep, worldtop);
		loopcontext.topfrac = (centeryfrac>>4) - FixedMul (worldtop, wallcontext->scale);

		loopcontext.bottomstep = -FixedMul (wallcontext->scalestep,worldbottom);
		loopcontext.bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, wallcontext->scale);

		if (frontsector->c_slope) {
			fixed_t topfracend = (centeryfrac>>4) - FixedMul (worldtopslope, ds_p->scale2);
			loopcontext.topstep = (topfracend-loopcontext.topfrac)/(range);
		}
		if (frontsector->f_slope) {
			fixed_t bottomfracend = (centeryfrac>>4) - FixedMul (worldbottomslope, ds_p->scale2);
			loopcontext.bottomstep = (bottomfracend-loopcontext.bottomfrac)/(range);
		}
	}

	dc->numlights = 0;

	if (frontsector->numlights)
	{
		dc->numlights = frontsector->numlights;
		if (dc->numlights > dc->maxlights)
		{
			dc->maxlights = dc->numlights;
			dc->lightlist = Z_Realloc(dc->lightlist, sizeof (*dc->lightlist) * dc->maxlights, PU_STATIC, NULL);
		}

		for (i = p = 0; i < dc->numlights; i++)
		{
			fixed_t leftheight, rightheight;

			light = &frontsector->lightlist[i];
			rlight = &dc->lightlist[p];

			leftheight  = P_GetLightZAt(light,  segleft.x,  segleft.y);
			rightheight = P_GetLightZAt(light, segright.x, segright.y);

			if (light->slope)
				// Flag sector as having slopes
				frontsector->hasslope = true;

			leftheight -= viewcontext->z;
			rightheight -= viewcontext->z;

			leftheight >>= 4;
			rightheight >>= 4;

			if (i != 0)
			{
				if (leftheight < worldbottom && rightheight < worldbottomslope)
					continue;

				if (leftheight > worldtop && rightheight > worldtopslope && i+1 < dc->numlights && frontsector->lightlist[i+1].height > frontsector->ceilingheight)
					continue;
			}

			rlight->height = (centeryfrac>>4) - FixedMul(leftheight, wallcontext->scale);
			rlight->heightstep = (centeryfrac>>4) - FixedMul(rightheight, ds_p->scale2);
			rlight->heightstep = (rlight->heightstep-rlight->height)/(range);
			rlight->flags = light->flags;

			if (light->caster && light->caster->flags & FF_CUTSOLIDS)
			{
				leftheight  = P_GetFFloorBottomZAt(light->caster,  segleft.x,  segleft.y);
				rightheight = P_GetFFloorBottomZAt(light->caster, segright.x, segright.y);

				if (*light->caster->b_slope)
					// Flag sector as having slopes
					frontsector->hasslope = true;

				leftheight  -= viewcontext->z;
				rightheight -= viewcontext->z;

				leftheight  >>= 4;
				rightheight >>= 4;

				rlight->botheight = (centeryfrac>>4) - FixedMul(leftheight, wallcontext->scale);
				rlight->botheightstep = (centeryfrac>>4) - FixedMul(rightheight, ds_p->scale2);
				rlight->botheightstep = (rlight->botheightstep-rlight->botheight)/(range);

			}

			rlight->lightlevel = *light->lightlevel;
			rlight->extra_colormap = *light->extra_colormap;
			p++;
		}

		dc->numlights = p;
	}

	for (i = 0; i < planecontext->numffloors; i++)
	{
		planecontext->ffloor[i].f_pos >>= 4;
		planecontext->ffloor[i].f_pos_slope >>= 4;
		if (bspcontext->linedef->special == HORIZONSPECIAL) // Horizon lines extend FOFs in contact with them too.
		{
			planecontext->ffloor[i].f_step = 0;
			planecontext->ffloor[i].f_frac = (centeryfrac>>4);
			loopcontext.topfrac++; // Prevent 1px HOM
		}
		else
		{
			planecontext->ffloor[i].f_frac = (centeryfrac>>4) - FixedMul(planecontext->ffloor[i].f_pos, wallcontext->scale);
			planecontext->ffloor[i].f_step = ((centeryfrac>>4) - FixedMul(planecontext->ffloor[i].f_pos_slope, ds_p->scale2) - planecontext->ffloor[i].f_frac)/(range);
		}
	}

	if (backsector)
	{
		worldhigh >>= 4;
		worldlow >>= 4;
		worldhighslope >>= 4;
		worldlowslope >>= 4;

		if (loopcontext.toptexture)
		{
			loopcontext.pixhigh = (centeryfrac>>4) - FixedMul (worldhigh, wallcontext->scale);
			loopcontext.pixhighstep = -FixedMul (wallcontext->scalestep,worldhigh);

			if (backsector->c_slope) {
				fixed_t topfracend = (centeryfrac>>4) - FixedMul (worldhighslope, ds_p->scale2);
				loopcontext.pixhighstep = (topfracend-loopcontext.pixhigh)/(range);
			}
		}

		if (loopcontext.bottomtexture)
		{
			loopcontext.pixlow = (centeryfrac>>4) - FixedMul (worldlow, wallcontext->scale);
			loopcontext.pixlowstep = -FixedMul (wallcontext->scalestep,worldlow);
			if (backsector->f_slope) {
				fixed_t bottomfracend = (centeryfrac>>4) - FixedMul (worldlowslope, ds_p->scale2);
				loopcontext.pixlowstep = (bottomfracend-loopcontext.pixlow)/(range);
			}
		}

		{
			ffloor_t * rover;
			fixed_t roverleft, roverright;
			fixed_t planevistest;
			i = 0;

			if (backsector->ffloors)
			{
				for (rover = backsector->ffloors; rover && i < MAXFFLOORS; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
						continue;
					if (rover->norender == leveltime)
						continue;

					// Let the renderer know this sector is sloped.
					if (*rover->b_slope || *rover->t_slope)
						backsector->hasslope = true;

					roverleft    = P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) - viewcontext->z;
					roverright   = P_GetFFloorBottomZAt(rover, segright.x, segright.y) - viewcontext->z;
					planevistest = P_GetFFloorBottomZAt(rover, viewcontext->x, viewcontext->y);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewcontext->z < planevistest && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES))) ||
					     (viewcontext->z > planevistest && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
					{
						//planecontext->ffloor[i].slope = *rover->b_slope;
						planecontext->ffloor[i].b_pos = roverleft;
						planecontext->ffloor[i].b_pos_slope = roverright;
						planecontext->ffloor[i].b_pos >>= 4;
						planecontext->ffloor[i].b_pos_slope >>= 4;
						planecontext->ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos, wallcontext->scale);
						planecontext->ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos_slope, ds_p->scale2);
						planecontext->ffloor[i].b_step = (planecontext->ffloor[i].b_step-planecontext->ffloor[i].b_frac)/(range);
						i++;
					}

					if (i >= MAXFFLOORS)
						break;

					roverleft    = P_GetFFloorTopZAt(rover, segleft .x, segleft .y) - viewcontext->z;
					roverright   = P_GetFFloorTopZAt(rover, segright.x, segright.y) - viewcontext->z;
					planevistest = P_GetFFloorTopZAt(rover, viewcontext->x, viewcontext->y);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewcontext->z > planevistest && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES))) ||
					     (viewcontext->z < planevistest && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
					{
						//planecontext->ffloor[i].slope = *rover->t_slope;
						planecontext->ffloor[i].b_pos = roverleft;
						planecontext->ffloor[i].b_pos_slope = roverright;
						planecontext->ffloor[i].b_pos >>= 4;
						planecontext->ffloor[i].b_pos_slope >>= 4;
						planecontext->ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos, wallcontext->scale);
						planecontext->ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos_slope, ds_p->scale2);
						planecontext->ffloor[i].b_step = (planecontext->ffloor[i].b_step-planecontext->ffloor[i].b_frac)/(range);
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

					// Let the renderer know this sector is sloped.
					if (*rover->b_slope || *rover->t_slope)
						frontsector->hasslope = true;

					roverleft  = P_GetFFloorBottomZAt(rover, segleft .x, segleft .y) - viewcontext->z;
					roverright = P_GetFFloorBottomZAt(rover, segright.x, segright.y) - viewcontext->z;
					planevistest = P_GetFFloorBottomZAt(rover, viewcontext->x, viewcontext->y);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewcontext->z < planevistest && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES))) ||
					     (viewcontext->z > planevistest && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
					{
						//planecontext->ffloor[i].slope = *rover->b_slope;
						planecontext->ffloor[i].b_pos = roverleft;
						planecontext->ffloor[i].b_pos_slope = roverright;
						planecontext->ffloor[i].b_pos >>= 4;
						planecontext->ffloor[i].b_pos_slope >>= 4;
						planecontext->ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos, wallcontext->scale);
						planecontext->ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos_slope, ds_p->scale2);
						planecontext->ffloor[i].b_step = (planecontext->ffloor[i].b_step-planecontext->ffloor[i].b_frac)/(range);
						i++;
					}

					if (i >= MAXFFLOORS)
						break;

					roverleft  = P_GetFFloorTopZAt(rover, segleft .x, segleft .y) - viewcontext->z;
					roverright = P_GetFFloorTopZAt(rover, segright.x, segright.y) - viewcontext->z;
					planevistest = P_GetFFloorTopZAt(rover, viewcontext->x, viewcontext->y);

					if ((roverleft>>4 <= worldhigh || roverright>>4 <= worldhighslope) &&
					    (roverleft>>4 >= worldlow || roverright>>4 >= worldlowslope) &&
					    ((viewcontext->z > planevistest && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES))) ||
					     (viewcontext->z < planevistest && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
					{
						//planecontext->ffloor[i].slope = *rover->t_slope;
						planecontext->ffloor[i].b_pos = roverleft;
						planecontext->ffloor[i].b_pos_slope = roverright;
						planecontext->ffloor[i].b_pos >>= 4;
						planecontext->ffloor[i].b_pos_slope >>= 4;
						planecontext->ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos, wallcontext->scale);
						planecontext->ffloor[i].b_step = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos_slope, ds_p->scale2);
						planecontext->ffloor[i].b_step = (planecontext->ffloor[i].b_step-planecontext->ffloor[i].b_frac)/(range);
						i++;
					}
				}
			}
			if (bspcontext->curline->polyseg && frontsector && (bspcontext->curline->polyseg->flags & POF_RENDERPLANES))
			{
				while (i < planecontext->numffloors && planecontext->ffloor[i].polyobj != bspcontext->curline->polyseg) i++;
				if (i < planecontext->numffloors && backsector->floorheight <= frontsector->ceilingheight &&
					backsector->floorheight >= frontsector->floorheight &&
					(viewcontext->z < backsector->floorheight))
				{
					if (planecontext->ffloor[i].plane->minx > ds_p->x1)
						planecontext->ffloor[i].plane->minx = ds_p->x1;

					if (planecontext->ffloor[i].plane->maxx < ds_p->x2)
						planecontext->ffloor[i].plane->maxx = ds_p->x2;

					planecontext->ffloor[i].slope = NULL;
					planecontext->ffloor[i].b_pos = backsector->floorheight;
					planecontext->ffloor[i].b_pos = (planecontext->ffloor[i].b_pos - viewcontext->z) >> 4;
					planecontext->ffloor[i].b_step = FixedMul(-wallcontext->scalestep, planecontext->ffloor[i].b_pos);
					planecontext->ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos, wallcontext->scale);
					i++;
				}
				if (i < planecontext->numffloors && backsector->ceilingheight >= frontsector->floorheight &&
					backsector->ceilingheight <= frontsector->ceilingheight &&
					(viewcontext->z > backsector->ceilingheight))
				{
					if (planecontext->ffloor[i].plane->minx > ds_p->x1)
						planecontext->ffloor[i].plane->minx = ds_p->x1;

					if (planecontext->ffloor[i].plane->maxx < ds_p->x2)
						planecontext->ffloor[i].plane->maxx = ds_p->x2;

					planecontext->ffloor[i].slope = NULL;
					planecontext->ffloor[i].b_pos = backsector->ceilingheight;
					planecontext->ffloor[i].b_pos = (planecontext->ffloor[i].b_pos - viewcontext->z) >> 4;
					planecontext->ffloor[i].b_step = FixedMul(-wallcontext->scalestep, planecontext->ffloor[i].b_pos);
					planecontext->ffloor[i].b_frac = (centeryfrac >> 4) - FixedMul(planecontext->ffloor[i].b_pos, wallcontext->scale);
					i++;
				}
			}

			loopcontext.numbackffloors = i;
		}
	}

	// get a new or use the same visplane
	if (loopcontext.markceiling)
	{
		if (planecontext->ceilingplane) //SoM: 3/29/2000: Check for null ceiling planes
			planecontext->ceilingplane = R_CheckPlane (planecontext, planecontext->ceilingplane, loopcontext.x, loopcontext.stopx-1);
		else
			loopcontext.markceiling = false;

		// Don't mark ceiling flat lines for polys unless this line has an upper texture, otherwise we get flat leakage pulling downward
		// (If it DOES have an upper texture and we do this, the ceiling won't render at all)
		if (bspcontext->curline->polyseg && !bspcontext->curline->sidedef->toptexture)
			loopcontext.markceiling = false;
	}

	// get a new or use the same visplane
	if (loopcontext.markfloor)
	{
		if (planecontext->floorplane) //SoM: 3/29/2000: Check for null planes
			planecontext->floorplane = R_CheckPlane (planecontext, planecontext->floorplane, loopcontext.x, loopcontext.stopx-1);
		else
			loopcontext.markfloor = false;

		// Don't mark floor flat lines for polys unless this line has a lower texture, otherwise we get flat leakage pulling upward
		// (If it DOES have a lower texture and we do this, the floor won't render at all)
		if (bspcontext->curline->polyseg && !bspcontext->curline->sidedef->bottomtexture)
			loopcontext.markfloor = false;
	}

	ds_p->numffloorplanes = 0;
	if (planecontext->numffloors)
	{
		if (!bspcontext->firstseg)
		{
			ds_p->numffloorplanes = planecontext->numffloors;

			for (i = 0; i < planecontext->numffloors; i++)
			{
				ds_p->ffloorplanes[i] = planecontext->ffloor[i].plane =
					R_CheckPlane(planecontext, planecontext->ffloor[i].plane, loopcontext.x, loopcontext.stopx - 1);
			}

			bspcontext->firstseg = ds_p;
		}
		else
		{
			for (i = 0; i < planecontext->numffloors; i++)
				R_ExpandPlane(planecontext->ffloor[i].plane, loopcontext.x, loopcontext.stopx - 1);
		}
		// FIXME hack to fix planes disappearing when a seg goes behind the camera. This NEEDS to be changed to be done properly. -Red
		if (bspcontext->curline->polyseg)
		{
			for (i = 0; i < planecontext->numffloors; i++)
			{
				if (!planecontext->ffloor[i].polyobj || planecontext->ffloor[i].polyobj != bspcontext->curline->polyseg)
					continue;
				if (planecontext->ffloor[i].plane->minx > loopcontext.x)
					planecontext->ffloor[i].plane->minx = loopcontext.x;

				if (planecontext->ffloor[i].plane->maxx < loopcontext.stopx - 1)
					planecontext->ffloor[i].plane->maxx = loopcontext.stopx - 1;
			}
		}
	}

	wallcontext->silhouette = &(ds_p->silhouette);
	wallcontext->tsilheight = &(ds_p->tsilheight);
	wallcontext->bsilheight = &(ds_p->bsilheight);

	R_RenderSegLoop(context, wallcontext, &loopcontext, dc);
	dc->func = colfuncs[BASEDRAWFUNC];

	if (bspcontext->portalline) // if curline is a portal, set portalrender for drawseg
		ds_p->portalpass = bspcontext->portalrender+1;
	else
		ds_p->portalpass = 0;

	// save sprite clipping info
	if (((ds_p->silhouette & SIL_TOP) || loopcontext.maskedtexture) && !ds_p->sprtopclip)
	{
		M_Memcpy(planecontext->lastopening, planecontext->ceilingclip+start, 2*(loopcontext.stopx - start));
		ds_p->sprtopclip = planecontext->lastopening - start;
		planecontext->lastopening += loopcontext.stopx - start;
	}

	if (((ds_p->silhouette & SIL_BOTTOM) || loopcontext.maskedtexture) && !ds_p->sprbottomclip)
	{
		M_Memcpy(planecontext->lastopening, planecontext->floorclip + start, 2*(loopcontext.stopx-start));
		ds_p->sprbottomclip = planecontext->lastopening - start;
		planecontext->lastopening += loopcontext.stopx - start;
	}

	if (loopcontext.maskedtexture && !(ds_p->silhouette & SIL_TOP))
	{
		ds_p->silhouette |= SIL_TOP;
		ds_p->tsilheight = (bspcontext->sidedef->midtexture > 0 && bspcontext->sidedef->midtexture < numtextures) ? INT32_MIN: INT32_MAX;
	}
	if (loopcontext.maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
	{
		ds_p->silhouette |= SIL_BOTTOM;
		ds_p->bsilheight = (bspcontext->sidedef->midtexture > 0 && bspcontext->sidedef->midtexture < numtextures) ? INT32_MAX: INT32_MIN;
	}
	bspcontext->ds_p++;
}
