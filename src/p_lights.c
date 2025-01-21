// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_lights.c
/// \brief Sector lighting effects
///        Fire flicker, light flash, strobe flash, lightning flash, glow, and fade.

#include "doomdef.h"
#include "p_local.h"
#include "r_state.h"
#include "z_zone.h"
#include "m_random.h"
#include "netcode/d_netcmd.h"

/** Removes any active lighting effects in a sector.
  *
  * \param sector The sector to remove effects from.
  */
void P_RemoveLighting(sector_t *sector)
{
	if (sector->lightingdata)
	{
		// The thinker is the first member in all the lighting action structs,
		// so just let the thinker get freed, and that will free the whole
		// structure.
		P_RemoveThinker(&((thinkerdata_t *)sector->lightingdata)->thinker);
		sector->lightingdata = NULL;
	}
}

// =========================================================================
//                           FIRELIGHT FLICKER
// =========================================================================

/** Thinker function for fire flicker.
  *
  * \param flick Action structure for this effect.
  * \sa P_SpawnAdjustableFireFlicker
  */
void T_FireFlicker(fireflicker_t *flick)
{
	INT16 amount;

	if (--flick->count)
		return;

	amount = (INT16)((UINT8)(P_RandomByte() & 3) * 16);

	if (flick->sector->lightlevel - amount < flick->minlight)
		flick->sector->lightlevel = flick->minlight;
	else
		flick->sector->lightlevel = flick->maxlight - amount;

	flick->count = flick->resetcount;
}

/** Spawns an adjustable fire flicker effect in a sector.
  *
  * \param sector    Target sector for the effect.
  * \param lighta    One of the two light levels to move between.
  * \param lightb    The other light level.
  * \param length    Four times the number of tics between flickers.
  * \sa T_FireFlicker
  */
fireflicker_t *P_SpawnAdjustableFireFlicker(sector_t *sector, INT16 lighta, INT16 lightb, INT32 length)
{
	fireflicker_t *flick;

	P_RemoveLighting(sector); // out with the old, in with the new
	flick = Z_Calloc(sizeof (*flick), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flick->thinker);

	flick->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
	flick->sector = sector;
	flick->maxlight = max(lighta, lightb);
	flick->minlight = min(lighta, lightb);
	flick->count = flick->resetcount = length/4;
	sector->lightingdata = flick;

	// input bounds checking and stuff
	if (!flick->resetcount)
		flick->resetcount = 1;
	if (flick->minlight == flick->maxlight)
	{
		if (flick->minlight > 0)
			flick->minlight--;
		if (flick->maxlight < 255)
			flick->maxlight++;
	}

	// Make sure the starting light level is in range.
	sector->lightlevel = max(flick->minlight, min(flick->maxlight, sector->lightlevel));

	return flick;
}

//
// LIGHTNING FLASH EFFECT
//

/** Thinker function for a lightning flash storm effect.
  *
  * \param flash The effect being considered.
  * \sa P_SpawnLightningFlash
  */
void T_LightningFlash(lightflash_t *flash)
{
	flash->sector->lightlevel -= 4;

	if (flash->sector->lightlevel <= flash->minlight)
	{
		flash->sector->lightlevel = (INT16)flash->minlight;
		P_RemoveLighting(flash->sector);
	}
}

/** Spawns a one-time lightning flash.
  *
  * \param sector Sector to light up.
  * \sa T_LightningFlash
  */
void P_SpawnLightningFlash(sector_t *sector)
{
	INT32 minlight;
	lightflash_t *flash;

	minlight = sector->lightlevel;

	if (sector->lightingdata)
	{
		if (((lightflash_t *)sector->lightingdata)->thinker.function.acp1
			== (actionf_p1)T_LightningFlash)
		{
			// lightning was already flashing in this sector
			// save the original light level value
			minlight = ((lightflash_t *)sector->lightingdata)->minlight;
		}

		P_RemoveThinker(&((thinkerdata_t *)sector->lightingdata)->thinker);
	}

	sector->lightingdata = NULL;

	flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LightningFlash;
	flash->sector = sector;
	flash->maxlight = 255;
	flash->minlight = minlight;
	sector->lightlevel = (INT16)flash->maxlight;

	sector->lightingdata = flash;
}

//
// STROBE LIGHT FLASHING
//

/** Thinker function for strobe light flashing.
  *
  * \param flash The effect under consideration.
  * \sa P_SpawnAdjustableStrobeFlash
  */
void T_StrobeFlash(strobe_t *flash)
{
	if (--flash->count)
		return;

	if (flash->sector->lightlevel == flash->minlight)
	{
		flash->sector->lightlevel = flash->maxlight;
		flash->count = flash->brighttime;
	}
	else
	{
		flash->sector->lightlevel = flash->minlight;
		flash->count = flash->darktime;
	}
}

/** Spawns an adjustable strobe light effect in a sector.
  *
  * \param sector     Target sector for the effect.
  * \param lighta     One of the two light levels to move between.
  * \param lightb     The other light level.
  * \param darktime   Time in tics for the light to be dark.
  * \param brighttime Time in tics for the light to be bright.
  * \param inSync     If true, the effect will be kept in sync
  *                   with other effects of this type, provided
  *                   they use the same time values, and
  *                   provided this function is called at level
  *                   load. Otherwise, the starting state of
  *                   the strobe flash is random.
  * \sa T_StrobeFlash
  */
strobe_t *P_SpawnAdjustableStrobeFlash(sector_t *sector, INT16 lighta, INT16 lightb, INT32 darktime, INT32 brighttime, boolean inSync)
{
	strobe_t *flash;

	P_RemoveLighting(sector); // out with the old, in with the new
	flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->sector = sector;
	flash->darktime = darktime;
	flash->brighttime = brighttime;
	flash->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
	flash->maxlight = max(lighta, lightb);
	flash->minlight = min(lighta, lightb);

	if (flash->minlight == flash->maxlight)
		flash->minlight = 0;

	if (!inSync)
		flash->count = (P_RandomByte() & 7) + 1;
	else
		flash->count = 1;

	// Make sure the starting light level is in range.
	sector->lightlevel = max(flash->minlight, min(flash->maxlight, sector->lightlevel));

	sector->lightingdata = flash;
	return flash;
}

/** Thinker function for glowing light.
  *
  * \param g Action structure for this effect.
  * \sa P_SpawnAdjustableGlowingLight
  */
void T_Glow(glow_t *g)
{
	switch (g->direction)
	{
		case -1:
			// DOWN
			g->sector->lightlevel -= g->speed;
			if (g->sector->lightlevel <= g->minlight)
			{
				g->sector->lightlevel += g->speed;
				g->direction = 1;
			}
			break;

		case 1:
			// UP
			g->sector->lightlevel += g->speed;
			if (g->sector->lightlevel >= g->maxlight)
			{
				g->sector->lightlevel -= g->speed;
				g->direction = -1;
			}
			break;
	}
}

/** Spawns an adjustable glowing light effect in a sector.
  *
  * \param sector    Target sector for the effect.
  * \param lighta    One of the two light levels to move between.
  * \param lightb    The other light level.
  * \param length    The speed of the effect.
  * \sa T_Glow
  */
glow_t *P_SpawnAdjustableGlowingLight(sector_t *sector, INT16 lighta, INT16 lightb, INT32 length)
{
	glow_t *g;

	P_RemoveLighting(sector); // out with the old, in with the new
	g = Z_Calloc(sizeof (*g), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &g->thinker);

	g->sector = sector;
	g->minlight = min(lighta, lightb);
	g->maxlight = max(lighta, lightb);
	g->thinker.function.acp1 = (actionf_p1)T_Glow;
	g->direction = 1;
	g->speed = (INT16)(length/4);
	if (g->speed > (g->maxlight - g->minlight)/2) // don't make it ridiculous speed
		g->speed = (g->maxlight - g->minlight)/2;

	while (g->speed < 1)
	{
		if (g->minlight > 0)
			g->minlight--;
		if (g->maxlight < 255)
			g->maxlight++;

		g->speed = (g->maxlight - g->minlight)/2;
	}

	// Make sure the starting light level is in range.
	sector->lightlevel = max(g->minlight, min(g->maxlight, sector->lightlevel));

	sector->lightingdata = g;

	return g;
}

/** Fades all the lights in specified sector to a new
  * value.
  *
  * \param sector    Target sector
  * \param destvalue The final light value in these sectors.
  * \param speed     If tic-based: total duration of effect.
  *                  If speed-based: Speed of the fade; the change to the ligh
  *                  level in each sector per tic.
  * \param ticbased  Use a specific duration for the fade, defined by speed
  * \sa T_LightFade
  */
void P_FadeLightBySector(sector_t *sector, INT32 destvalue, INT32 speed, boolean ticbased)
{
	lightlevel_t *ll;

	P_RemoveLighting(sector); // remove the old lighting effect first

	if ((ticbased && !speed) || sector->lightlevel == destvalue) // set immediately
	{
		sector->lightlevel = destvalue;
		return;
	}

	ll = Z_Calloc(sizeof (*ll), PU_LEVSPEC, NULL);
	ll->thinker.function.acp1 = (actionf_p1)T_LightFade;
	sector->lightingdata = ll; // set it to the lightlevel_t

	P_AddThinker(THINK_MAIN, &ll->thinker); // add thinker

	ll->sector = sector;
	ll->sourcelevel = sector->lightlevel;
	ll->destlevel = destvalue;

	ll->fixedcurlevel = sector->lightlevel<<FRACBITS;

	if (ticbased)
	{
		// Speed means duration.
		ll->timer = abs(speed);
		ll->fixedpertic = FixedDiv((destvalue<<FRACBITS) - ll->fixedcurlevel, speed<<FRACBITS);
	}
	else
	{
		// Speed means increment per tic (literally speed).
		ll->timer = abs(FixedDiv((destvalue<<FRACBITS) - ll->fixedcurlevel, speed<<FRACBITS)>>FRACBITS);
		ll->fixedpertic = ll->destlevel < ll->sourcelevel ? -speed<<FRACBITS : speed<<FRACBITS;
	}
}

void P_FadeLight(INT16 tag, INT32 destvalue, INT32 speed, boolean ticbased, boolean force, boolean relative)
{
	INT32 i;
	INT32 realdestvalue;

	// search all sectors for ones with tag
	TAG_ITER_SECTORS(tag, i)
	{
		if (!force && ticbased // always let speed fader execute
			&& sectors[i].lightingdata
			&& ((lightlevel_t*)sectors[i].lightingdata)->thinker.function.acp1 == (actionf_p1)T_LightFade)
			// && ((lightlevel_t*)sectors[i].lightingdata)->timer > 2)
		{
			CONS_Debug(DBG_GAMELOGIC, "Line type 420 Executor: Fade light thinker already exists, timer: %d\n", ((lightlevel_t*)sectors[i].lightingdata)->timer);
			continue;
		}

		realdestvalue = relative ? max(0, min(255, sectors[i].lightlevel + destvalue)) : destvalue;
		P_FadeLightBySector(&sectors[i], realdestvalue, speed, ticbased);
	}
}

/** Fades the light level in a sector to a new value.
  *
  * \param ll The fade effect under consideration.
  * \sa P_FadeLight
  */
void T_LightFade(lightlevel_t *ll)
{
	if (--ll->timer <= 0)
	{
		ll->sector->lightlevel = ll->destlevel; // set to dest lightlevel
		P_RemoveLighting(ll->sector); // clear lightingdata, remove thinker
		return;
	}

	ll->fixedcurlevel = ll->fixedcurlevel + ll->fixedpertic;
	ll->sector->lightlevel = (ll->fixedcurlevel)>>FRACBITS;
}
