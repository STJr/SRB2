// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2001 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw3sound.c
/// \brief Hardware 3D sound general code

#include "../doomdef.h"

#ifdef HW3SOUND

#include "../i_sound.h"
#include "../s_sound.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../g_game.h"
#include "../tables.h"
#include "../sounds.h"
#include "../r_main.h"
#include "../r_skins.h"
#include "../m_random.h"
#include "../p_local.h"
#include "hw3dsdrv.h"
#include "hw3sound.h"

#define ANGLE2DEG(x) (((double)(x)) / ((double)ANGLE_45/45))
#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128
#if 1
#define TPS(x) (x)
#else //Alam_GBC: MPS, not MPF!
#define TPS(x) ((float)(x)/(float)TICRATE)
#endif

struct hardware3ds_s hw3ds_driver;


typedef struct source_s
{
	sfxinfo_t       *sfxinfo;
	const void      *origin;
	INT32           handle;     // Internal source handle
	channel_type_t  type;       // Sound type (attack, scream, etc)
} source_t;


typedef struct ambient_sdata_s
{
	source3D_data_t left;
	source3D_data_t right;
} ambient_sdata_t;


typedef struct ambient_source_s
{
	source_t    left;
	source_t    right;
} ambient_source_t;

// Static sources
// This sources always creates
static source_t p_attack_source;        // Player attack source
static source_t p_attack_source2;        // Player2 attack source
static source_t p_scream_source;        // Player scream source
static source_t p_scream_source2;        // Player2 scream source


static ambient_source_t ambient_source; // Ambinet sound sources

//  abuse???
//static source3D_data_t  p_attack_sdata; // ?? Now just holds precalculated player attack source data
//static source3D_data_t  p_default_sdata;// ?? ---- // ---- // ---- // ---- player scream source data

static ambient_sdata_t  ambient_sdata;  // Precalculated datas of an ambient sound sources

// Stack of dynamic sources
static source_t      *sources       = NULL;     // Much like original channels
static INT32           num_sources    = 0;

// Current mode of 3D sound system
// Default is original (stereo) mode
INT32                  hws_mode       = HWS_DEFAULT_MODE;

//=============================================================================
void HW3S_SetSourcesNum(void)
{
	INT32 i;

	// Allocating the internal channels for mixing
	// (the maximum number of sounds rendered
	// simultaneously) within zone memory.
	if (sources)
		HW3S_StopSounds();
	Z_Free(sources);

	if (cv_numChannels.value <= STATIC_SOURCES_NUM)
		I_Error("HW3S_SetSourcesNum: Number of sound sources cannot be less than %d\n", STATIC_SOURCES_NUM + 1);

	num_sources = cv_numChannels.value - STATIC_SOURCES_NUM;

	sources = (source_t *) Z_Malloc(num_sources * sizeof (*sources), PU_STATIC, 0);

	// Free all channels for use
	for (i = 0; i < num_sources; i++)
	{
		sources[i].sfxinfo = NULL;
		sources[i].type = CT_NORMAL;
		sources[i].handle = -1;
	}
}


//=============================================================================
static void HW3S_KillSource(INT32 snum)
{
	source_t *  s = &sources[snum];

	if (s->sfxinfo)
	{
		HW3DS.pfnStopSource(s->handle);
		HW3DS.pfnKillSource(s->handle);
		s->handle = -1;
		s->sfxinfo->usefulness--;
		s->origin = NULL;
		s->sfxinfo = NULL;
	}
}


//=============================================================================
/*
static void HW3S_StopSource(INT32 snum)
{
	source_t * s = &sources[snum];

	if (s->sfxinfo)
	{
		// stop the sound playing
		HW3DS.pfnStopSource(s->handle);
	}
}
*/


//=============================================================================
void HW3S_StopSound(void *origin)
{
	INT32 snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].sfxinfo && sources[snum].origin == origin)
		{
			HW3S_KillSource(snum);
			break;
		}
	}
}

void HW3S_StopSoundByID(void *origin, sfxenum_t sfx_id)
{
	INT32 snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].sfxinfo == &S_sfx[sfx_id] && sources[snum].origin == origin)
		{
			HW3S_KillSource(snum);
			break;
		}
	}
}

void HW3S_StopSoundByNum(sfxenum_t sfxnum)
{
	INT32 snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].sfxinfo == &S_sfx[sfxnum])
		{
			HW3S_KillSource(snum);
			break;
		}
	}
}

//=============================================================================
void HW3S_StopSounds(void)
{
	INT32 snum;

	// kill all playing sounds at start of level
	//  (trust me - a good idea)
	for (snum = 0; snum < num_sources; snum++)
		if (sources[snum].sfxinfo)
			HW3S_KillSource(snum);

	// Also stop all static sources
	HW3DS.pfnStopSource(p_attack_source.handle);
	HW3DS.pfnStopSource(p_attack_source2.handle);
	HW3DS.pfnStopSource(p_scream_source.handle);
	HW3DS.pfnStopSource(p_scream_source2.handle);
	HW3DS.pfnStopSource(ambient_source.left.handle);
	HW3DS.pfnStopSource(ambient_source.right.handle);
}


//=============================================================================
static INT32 HW3S_GetSource(const void *origin, sfxinfo_t *sfxinfo, boolean splitsound)
{
	//
	//   If none available, return -1.  Otherwise source #.
	//   source number to use

	INT32         snum;
	source_t *  src;
	INT32 sep = NORM_SEP, pitch = NORM_PITCH, volume = 255;

	(void)splitsound;

	// Find an open source
	for (snum = 0, src = sources; snum < num_sources; src++, snum++)
	{
		if (!src->sfxinfo)
			break;

#if 0
		if (origin && src->origin ==  origin)
		{
			HW3S_KillSource(snum);
			break;
		}
#endif
	}

#if 0
	// Check to see if it is audible
	if (origin && origin != listenmobj)
	{
		INT32 rc;
		rc = S_AdjustSoundParams(listenmobj, origin, &volume, &sep, &pitch, sfxinfo);
		if (!rc)
			return -1;
	}
#else
	(void)origin;
	(void)pitch;
	(void)volume;
	(void)sep;
#endif

	// None available
	if (snum == num_sources)
	{
		// Look for lower priority
		for (snum = 0, src = sources; snum < num_sources; src++, snum++)
			if (src->sfxinfo->priority <= sfxinfo->priority)
				break;

		if (snum == num_sources)
		{
			// No lower priority. Sorry, Charlie.
			return -1;
		}
		else
		{
			// Otherwise, kick out lower priority
			HW3S_KillSource(snum);
		}
	}
	return snum;
}


//=============================================================================
static void HW3S_FillSourceParameters
                              (const mobj_t     *origin,
                               source3D_data_t  *data,
                               channel_type_t   c_type)
{
	fixed_t x = 0, y = 0, z = 0;

	data->max_distance = MAX_DISTANCE;
	data->min_distance = MIN_DISTANCE;

	if (origin && origin != players[displayplayer].mo)
	{
		data->head_relative = false;

		data->pos.momx = TPS(FIXED_TO_FLOAT(origin->momx));
		data->pos.momy = TPS(FIXED_TO_FLOAT(origin->momy));
		data->pos.momz = TPS(FIXED_TO_FLOAT(origin->momz));

		x = origin->x;
		y = origin->y;
		z = origin->z;

		if (c_type == CT_ATTACK)
		{
			const angle_t an = origin->angle >> ANGLETOFINESHIFT;
			x += FixedMul(16*FRACUNIT, FINECOSINE(an));
			y += FixedMul(16*FRACUNIT, FINESINE(an));
			z += origin->height >> 1;
		}

		else if (c_type == CT_SCREAM)
			z += origin->height - (5 * FRACUNIT);
	}
	else
	{
		data->head_relative = true;

		data->pos.momx = 0.0f;
		data->pos.momy = 0.0f;
		data->pos.momz = 0.0f;
	}
	data->pos.x = FIXED_TO_FLOAT(x);
	data->pos.y = FIXED_TO_FLOAT(y);
	data->pos.z = FIXED_TO_FLOAT(z);
}

#define HEADER_SIZE 8
//==============================================================
/*
static void make_outphase_sfx(void *dest, void *src, INT32 size)
{
	SINT8    *s = (SINT8 *)src + HEADER_SIZE, *d = (SINT8 *)dest + HEADER_SIZE;

	M_Memcpy(dest, src, HEADER_SIZE);
	size -= HEADER_SIZE;

	while (size--)
		*d++ = -(*s++);
}
*/

//INT32 HW3S_Start3DSound(const void *origin, source3D_data_t *source_parm, cone_def_t *cone_parm, channel_type_t channel, INT32 sfx_id, INT32 vol, INT32 pitch);
//=============================================================================
INT32 HW3S_I_StartSound(const void *origin_p, source3D_data_t *source_parm, channel_type_t c_type, sfxenum_t sfx_id, INT32 volume, INT32 pitch, INT32 sep)
{
	sfxinfo_t       *sfx;
	const mobj_t    *origin = (const mobj_t *)origin_p;
	source3D_data_t source3d_data;
	INT32             s_num = 0;
	source_t        *source = NULL;
	mobj_t *listenmobj = players[displayplayer].mo;
	mobj_t *listenmobj2 = NULL;

	if (splitscreen) listenmobj2 = players[secondarydisplayplayer].mo;

	if (sound_disabled)
		return -1;

	sfx = &S_sfx[sfx_id];

	if (sfx->skinsound!=-1 && origin && origin->skin)
	{
		// it redirect player sound to the sound in the skin table
		sfx_id = ((skin_t *)origin->skin)->soundsid[sfx->skinsound];
		sfx    = &S_sfx[sfx_id];
	}

	if (!sfx->data)
		sfx->data = HW3S_GetSfx(sfx);

	// judgecutor 08-16-2002
	// Sound pitching for both Doom and Heretic
#if 0
	if (cv_rndsoundpitch.value)
	{
		/*if (gamemode != heretic)
		{
			if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
				pitch += 8 - (M_RandomByte()&15);
			else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
				pitch += 16 - (M_RandomByte()&31);
		}
		else*/
			pitch = 128 + (M_RandomByte() & 7) - (M_RandomByte() & 7);
	}
#endif

	if (pitch < 0)
		pitch = NORMAL_PITCH;

	if (pitch > 255)
		pitch = 255;

	if (sep < 0)
		sep = 128;

	if (splitscreen && listenmobj2) // Copy the sound for the split player
	{
		if (c_type != CT_NORMAL && origin && (origin == listenmobj2))
		{
			if (c_type == CT_ATTACK)
			{
				if (origin == listenmobj2)
					source = &p_attack_source;
				else
					source = &p_attack_source2;
			}
			else
			{
				if (origin == listenmobj2)
					source = &p_scream_source;
				else
					source = &p_scream_source2;
			}

			if (source->sfxinfo != sfx)
			{
				HW3DS.pfnStopSource(source->handle);
				source->handle = HW3DS.pfnReloadSource(source->handle, sfx->volume);
				//I_OutputMsg("PlayerSound data reloaded\n");
			}
		}
		else if (c_type == CT_AMBIENT)
		{
	//        sfx_data_t  outphased_sfx;

			if (ambient_source.left.sfxinfo != sfx)
			{
				HW3DS.pfnStopSource(ambient_source.left.handle);
				HW3DS.pfnStopSource(ambient_source.right.handle);

				// judgecutor:
				// Outphased sfx's temporarily not used!!!
	/*
					outphased_sfx.data = Z_Malloc(sfx_data.length, PU_STATIC, 0);
					make_outphase_sfx(outphased_sfx.data, sfx_data.data, sfx_data.length);
					outphased_sfx.length = sfx_data.length;
					outphased_sfx.id = sfx_data.id;
	*/
				ambient_source.left.handle = HW3DS.pfnReloadSource(ambient_source.left.handle, (u_int)sfx->length);
				//ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, &outphased_sfx);
				ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, (u_int)sfx->length);
				ambient_source.left.sfxinfo = ambient_source.right.sfxinfo = sfx;
				//Z_Free(outphased_sfx.data);
			}

			HW3DS.pfnUpdateSourceParms(ambient_source.left.handle, volume, -1);
			HW3DS.pfnUpdateSourceParms(ambient_source.right.handle, volume, -1);

			if (sfx->usefulness++ < 0)
				sfx->usefulness = -1;

			// Ambient sound is special case
			HW3DS.pfnStartSource(ambient_source.left.handle);
			HW3DS.pfnStartSource(ambient_source.right.handle);
		}
		else
		{
			s_num = HW3S_GetSource(origin, sfx, true);

			if (s_num  < 0)
			{
				//I_OutputMsg("No free source, aborting\n");
				return -1;
			}

			source = &sources[s_num];

			if (origin && c_type == CT_NORMAL)
			{
				if (!source_parm)
				{
					source_parm = &source3d_data;
					source3d_data.permanent = 0;
					HW3S_FillSourceParameters(origin, source_parm, c_type);
				}

				source->handle = HW3DS.pfnAddSource(source_parm, (u_int)sfx->length);
			}
			else
				source->handle = HW3DS.pfnAddSource(NULL, (u_int)sfx->length);
		}

		// increase the usefulness
		if (sfx->usefulness++ < 0)
			sfx->usefulness = -1;

		source->sfxinfo = sfx;
		source->origin = origin;
		HW3DS.pfnStartSource(source->handle);
	}

	if (c_type != CT_NORMAL && origin && (origin == listenmobj))
	{
		if (c_type == CT_ATTACK)
		{
			if (origin == listenmobj)
				source = &p_attack_source;
			else
				source = &p_attack_source2;
		}
		else
		{
			if (origin == listenmobj)
				source = &p_scream_source;
			else
				source = &p_scream_source2;
		}

		if (source->sfxinfo != sfx)
		{
			HW3DS.pfnStopSource(source->handle);
			source->handle = HW3DS.pfnReloadSource(source->handle, sfx->volume);
			//I_OutputMsg("PlayerSound data reloaded\n");
		}
	}
	else if (c_type == CT_AMBIENT)
	{
//        sfx_data_t  outphased_sfx;

		if (ambient_source.left.sfxinfo != sfx)
		{
			HW3DS.pfnStopSource(ambient_source.left.handle);
			HW3DS.pfnStopSource(ambient_source.right.handle);

			// judgecutor:
			// Outphased sfx's temporarily not used!!!
/*
				outphased_sfx.data = Z_Malloc(sfx_data.length, PU_STATIC, 0);
				make_outphase_sfx(outphased_sfx.data, sfx_data.data, sfx_data.length);
				outphased_sfx.length = sfx_data.length;
				outphased_sfx.id = sfx_data.id;
*/
			ambient_source.left.handle = HW3DS.pfnReloadSource(ambient_source.left.handle, (u_int)sfx->length);
			//ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, &outphased_sfx);
			ambient_source.right.handle = HW3DS.pfnReloadSource(ambient_source.right.handle, (u_int)sfx->length);
			ambient_source.left.sfxinfo = ambient_source.right.sfxinfo = sfx;
			//Z_Free(outphased_sfx.data);
		}

		HW3DS.pfnUpdateSourceParms(ambient_source.left.handle, volume, -1);
		HW3DS.pfnUpdateSourceParms(ambient_source.right.handle, volume, -1);

		if (sfx->usefulness++ < 0)
			sfx->usefulness = -1;

		// Ambient sound is special case
		HW3DS.pfnStartSource(ambient_source.left.handle);
		HW3DS.pfnStartSource(ambient_source.right.handle);
		return -1;
	}
	else
	{
		s_num = HW3S_GetSource(origin, sfx, false);

		if (s_num  < 0)
		{
			//I_OutputMsg("No free source, aborting\n");
			return -1;
		}

		source = &sources[s_num];

		if (origin && c_type == CT_NORMAL)
		{
			if (!source_parm)
			{
				source_parm = &source3d_data;
				source3d_data.permanent = 0;
				HW3S_FillSourceParameters(origin, source_parm, c_type);
			}

			source->handle = HW3DS.pfnAddSource(source_parm, (u_int)sfx->length);
		}
		else
			source->handle = HW3DS.pfnAddSource(NULL, (u_int)sfx->length);

	}

	// increase the usefulness
	if (sfx->usefulness++ < 0)
		sfx->usefulness = -1;

	source->sfxinfo = sfx;
	source->origin = origin;
	HW3DS.pfnStartSource(source->handle);
	return s_num;

}


// Start normal sound
//=============================================================================
void HW3S_StartSound(const void *origin, sfxenum_t sfx_id)
{
	HW3S_I_StartSound(origin, NULL, CT_NORMAL, sfx_id, 255, NORMAL_PITCH, NORMAL_SEP);
}


//=============================================================================
void S_StartAttackSound(const void *origin, sfxenum_t sfx_id)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3S_I_StartSound(origin, NULL, CT_ATTACK, sfx_id, 255, NORMAL_PITCH, NORMAL_SEP);
	else
		S_StartSound(origin, sfx_id);
}

void S_StartScreamSound(const void *origin, sfxenum_t sfx_id)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3S_I_StartSound(origin, NULL, CT_SCREAM, sfx_id, 255, NORMAL_PITCH, NORMAL_SEP);
	else
		S_StartSound(origin, sfx_id);
}

#if 0 /// NOTE: not used
void S_StartAmbientSound(sfxenum_t sfx_id, INT32 volume)
{
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		volume += 30;
		if (volume > 255)
			volume = 255;

		HW3S_I_StartSound(NULL, NULL, CT_AMBIENT, sfx_id, volume, NORMAL_PITCH, NORMAL_SEP);
	}
	else
		S_StartSoundAtVolume(NULL, sfx_id, volume);
}
#endif

FUNCMATH static inline float AmbientPos(angle_t an)
{
	const fixed_t fm = FixedMul(FLOAT_TO_FIXED(MIN_DISTANCE), FINESINE(an>>ANGLETOFINESHIFT));
	return FIXED_TO_FLOAT(fm);
}


//=============================================================================
INT32 HW3S_Init(I_Error_t FatalErrorFunction, snddev_t *snd_dev)
{
	INT32                succ;
	source3D_data_t    source_data;

	if (HW3DS.pfnStartup(FatalErrorFunction, snd_dev))
	{
		// Attack source
		source_data.head_relative = 1;
		source_data.pos.x = 0.0f;
		source_data.pos.y = 16.0f;
		source_data.pos.z = -FIXED_TO_FLOAT(mobjinfo[MT_PLAYER].height >> 1);
		source_data.pos.momx = 0.0f;
		source_data.pos.momy = 0.0f;
		source_data.pos.momz = 0.0f;
		source_data.min_distance = MIN_DISTANCE;
		source_data.max_distance = MAX_DISTANCE;
		source_data.permanent = 1;

		p_attack_source.sfxinfo = NULL;

		M_Memcpy(&p_attack_source2, &p_attack_source, sizeof (source_t));

		p_attack_source.handle = HW3DS.pfnAddSource(&source_data, sfx_None);
		p_attack_source2.handle = HW3DS.pfnAddSource(&source_data, sfx_None);

		// Scream source
		source_data.pos.y = 0;
		source_data.pos.z = 0;

		p_scream_source.sfxinfo = NULL;

		M_Memcpy(&p_scream_source2, &p_scream_source, sizeof (source_t));

		p_scream_source.handle = HW3DS.pfnAddSource(&source_data, sfx_None);
		p_scream_source2.handle = HW3DS.pfnAddSource(&source_data, sfx_None);

		//FIXED_TO_FLOAT(mobjinfo[MT_PLAYER].height - (5 * FRACUNIT));

		// Ambient sources (left and right) at 210 and 330 degree
		// relative to listener
		memset(&ambient_sdata, 0, sizeof (ambient_sdata));

		ambient_sdata.left.head_relative = 1;
		ambient_sdata.left.pos.x = ambient_sdata.left.pos.y = AmbientPos(ANG210);
		ambient_sdata.left.max_distance = MAX_DISTANCE;
		ambient_sdata.left.min_distance = MIN_DISTANCE;

		ambient_sdata.left.permanent = 1;

		M_Memcpy(&ambient_sdata.right, &ambient_sdata.left, sizeof (source3D_data_t));

		ambient_sdata.right.pos.x = -ambient_sdata.left.pos.x;
		ambient_source.left.handle = HW3DS.pfnAddSource(&ambient_sdata.left, sfx_None);
		ambient_source.right.handle = HW3DS.pfnAddSource(&ambient_sdata.right, sfx_None);

		succ = p_attack_source.handle > -1 && p_scream_source.handle > -1 &&
			p_attack_source2.handle > -1 && p_scream_source2.handle > -1 &&
			ambient_source.left.handle > -1 && ambient_source.right.handle > -1;

		//I_OutputMsg("Player handles: attack %d, default %d\n", p_attack_source.handle, p_scream_source.handle);
		return succ;
	}
	return 0;
}




//=============================================================================
INT32 HW3S_GetVersion(void)
{
	return HW3DS.pfnGetHW3DSVersion();
}


//=============================================================================
void HW3S_BeginFrameUpdate(void)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3DS.pfnBeginFrameUpdate();
}


//=============================================================================
void HW3S_EndFrameUpdate(void)
{
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3DS.pfnEndFrameUpdate();
}



//=============================================================================
INT32 HW3S_SoundIsPlaying(INT32 handle)
{
	return HW3DS.pfnIsPlaying(handle);
}

INT32 HW3S_OriginPlaying(void *origin)
{
	INT32         snum;

	if (!origin)
		return 0;
	for (snum = 0; snum < num_sources; snum++)
		if (sources[snum].origin == origin)
			return 1;
	return 0;
}

INT32 HW3S_IdPlaying(sfxenum_t id)
{
	INT32         snum;
	for (snum = 0; snum < num_sources; snum++)
		if ((size_t)(sources[snum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	return 0;
}

INT32 HW3S_SoundPlaying(void *origin, sfxenum_t id)
{
	INT32         snum;

	if (!origin)
		return 0;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].origin == origin
		 && (size_t)(sources[snum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	}
	return 0;
}

//=============================================================================
static void HW3S_UpdateListener(mobj_t *listener)
{
	listener_data_t data;

	if (!listener || !listener->player)
		return;

	if (camera.chase)
	{
		data.x = FIXED_TO_FLOAT(camera.x);
		data.y = FIXED_TO_FLOAT(camera.y);
		data.z = FIXED_TO_FLOAT(camera.z + camera.height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(camera.angle);
		data.h_angle = ANGLE2DEG(camera.aiming);

		data.momx = TPS(FIXED_TO_FLOAT(camera.momx));
		data.momy = TPS(FIXED_TO_FLOAT(camera.momy));
		data.momz = TPS(FIXED_TO_FLOAT(camera.momz));
	}
	else
	{
		data.x = FIXED_TO_FLOAT(listener->x);
		data.y = FIXED_TO_FLOAT(listener->y);
		data.z = FIXED_TO_FLOAT(listener->z + listener->height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(listener->angle);
		data.h_angle = ANGLE2DEG(listener->player->aiming);

		data.momx = TPS(FIXED_TO_FLOAT(listener->momx));
		data.momy = TPS(FIXED_TO_FLOAT(listener->momy));
		data.momz = TPS(FIXED_TO_FLOAT(listener->momz));
	}
	HW3DS.pfnUpdateListener(&data, 1);
}

static void HW3S_UpdateListener2(mobj_t *listener)
{
	listener_data_t data;

	if (!listener || !listener->player)
	{
		HW3DS.pfnUpdateListener(NULL, 2);
		return;
	}

	if (camera2.chase)
	{
		data.x = FIXED_TO_FLOAT(camera2.x);
		data.y = FIXED_TO_FLOAT(camera2.y);
		data.z = FIXED_TO_FLOAT(camera2.z + camera2.height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(camera2.angle);
		data.h_angle = ANGLE2DEG(camera2.aiming);

		data.momx = TPS(FIXED_TO_FLOAT(camera2.momx));
		data.momy = TPS(FIXED_TO_FLOAT(camera2.momy));
		data.momz = TPS(FIXED_TO_FLOAT(camera2.momz));
	}
	else
	{
		data.x = FIXED_TO_FLOAT(listener->x);
		data.y = FIXED_TO_FLOAT(listener->y);
		data.z = FIXED_TO_FLOAT(listener->z + listener->height - (5 * FRACUNIT));

		data.f_angle = ANGLE2DEG(listener->angle);
		data.h_angle = ANGLE2DEG(listener->player->aiming);

		data.momx = TPS(FIXED_TO_FLOAT(listener->momx));
		data.momy = TPS(FIXED_TO_FLOAT(listener->momy));
		data.momz = TPS(FIXED_TO_FLOAT(listener->momz));
	}

	HW3DS.pfnUpdateListener(&data, 2);
}

void HW3S_SetSfxVolume(INT32 volume)
{
	HW3DS.pfnSetGlobalSfxVolume(volume);
}


static void HW3S_Update3DSource(source_t *src)
{
	source3D_data_t data;
	data.permanent = 0;
	HW3S_FillSourceParameters(src->origin, &data, src->type);
	HW3DS.pfnUpdate3DSource(src->handle, &data.pos);

}

void HW3S_UpdateSources(void)
{
	mobj_t *listener = players[displayplayer].mo;
	mobj_t *listener2 = NULL;
	source_t    *src;
	INT32 audible, snum, volume, sep, pitch;

	if (splitscreen) listener2 = players[secondarydisplayplayer].mo;

	HW3S_UpdateListener2(listener2);
	HW3S_UpdateListener(listener);

	for (snum = 0, src = sources; snum < num_sources; src++, snum++)
	{
		if (src->sfxinfo)
		{
#if 0
			if (HW3DS.pfnIsPlaying(src->handle))
			{
				if (src->origin)
				{
					// initialize parameters
					volume = 255; // 8 bits internal volume precision
					pitch = NORM_PITCH;
					sep = NORM_SEP;

					// check non-local sounds for distance clipping
					//  or modify their params
					if (src->origin && listener != src->origin && !(listener2 && src->origin == listener2))
					{
						INT32 audible2;
						INT32 volume2 = volume, sep2 = sep, pitch2 = pitch;
						audible = S_AdjustSoundParams(listener, src->origin, &volume, &sep, &pitch,
							src->sfxinfo);

						if (listener2)
						{
							audible2 = S_AdjustSoundParams(listener2,
								src->origin, &volume2, &sep2, &pitch2, src->sfxinfo);
							if (audible2 && (!audible || (audible && volume2 > volume)))
							{
								audible = true;
								volume = volume2;
								sep = sep2;
								pitch = pitch2;
							}
						}

						if (audible)
							HW3S_Update3DSource(src); // Update positional sources
						else
							HW3S_KillSource(snum); //Kill it!
					}
				}
			}
			else
			{
				// Source allocated but stopped. Kill.
				HW3S_KillSource(snum);
			}
#else
			if (src->origin && listener != src->origin && !(listener2 && src->origin == listener2))
				HW3S_Update3DSource(src); // Update positional sources
			(void)pitch;
			(void)sep;
			(void)volume;
			(void)audible;
#endif
		}
	}
}

void HW3S_Shutdown(void)
{
	HW3DS.pfnShutdown();
}

void *HW3S_GetSfx(sfxinfo_t *sfx)
{
	sfx_data_t      sfx_data;

	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum (sfx);

	sfx_data.length = W_LumpLength(sfx->lumpnum);
	sfx_data.data = Z_Malloc(sfx_data.length, PU_SOUND, &sfx->data);
	W_ReadLump(sfx->lumpnum, sfx_data.data);
	sfx_data.priority = sfx->priority;

	sfx->length = HW3DS.pfnAddSfx(&sfx_data);

	Z_ChangeTag(sfx->data, PU_CACHE);

	return sfx_data.data;
}

void HW3S_FreeSfx(sfxinfo_t *sfx)
{
	INT32 snum;

	for (snum = 0; snum < num_sources; snum++)
	{
		if (sources[snum].sfxinfo == sfx)
		{
			HW3S_KillSource(snum);
			break;
		}
	}

	if (sfx->length > 0)
		HW3DS.pfnKillSfx((u_int)sfx->length);
	sfx->length = 0;

	sfx->lumpnum = LUMPERROR;

	Z_Free(sfx->data);
	sfx->data = NULL;
}

#endif
