/// \file
/// \brief SDL Mixer interface for sound

#include "../doomdef.h"

#if defined(HAVE_SDL) && defined(HAVE_MIXER) && SOUND==SOUND_MIXER

#include "../sounds.h"
#include "../s_sound.h"
#include "../i_sound.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../byteptr.h"

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif
#include "SDL.h"
#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "SDL_mixer.h"

/* This is the version number macro for the current SDL_mixer version: */
#ifndef SDL_MIXER_COMPILEDVERSION
#define SDL_MIXER_COMPILEDVERSION \
	SDL_VERSIONNUM(MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL)
#endif

/* This macro will evaluate to true if compiled with SDL_mixer at least X.Y.Z */
#ifndef SDL_MIXER_VERSION_ATLEAST
#define SDL_MIXER_VERSION_ATLEAST(X, Y, Z) \
	(SDL_MIXER_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))
#endif

#ifdef HAVE_LIBGME
#include "gme/gme.h"
#define GME_TREBLE 5.0
#define GME_BASS 1.0
#ifdef HAVE_PNG /// TODO: compile with zlib support without libpng

#define HAVE_ZLIB

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include "zlib.h"
#endif
#endif

UINT8 sound_started = false;

static Mix_Music *music;
static UINT8 music_volume, sfx_volume, internal_volume;
static float loop_point;
static float song_length; // length in seconds
static boolean songpaused;
static UINT32 music_bytes;
static boolean is_looping;

// fading
static boolean is_fading;
static UINT8 fading_target;
static UINT32 fading_steps;
static INT16 fading_volume_step;
static INT32 fading_id;
static void (*fading_callback)(void);

#ifdef HAVE_LIBGME
static Music_Emu *gme;
static INT32 current_track;
#endif

static void var_cleanup(void)
{
	loop_point = song_length =\
	 music_bytes = fading_target =\
	 fading_steps = fading_volume_step = 0;

	songpaused = is_looping =\
	 is_fading = false;

	fading_callback = NULL;

	internal_volume = 100;
}

/// ------------------------
/// Audio System
/// ------------------------

void I_StartupSound(void)
{
	I_Assert(!sound_started);

	// EE inits audio first so we're following along.
	if (SDL_WasInit(SDL_INIT_AUDIO) == SDL_INIT_AUDIO)
		CONS_Debug(DBG_BASIC, "SDL Audio already started\n");
	else if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		CONS_Alert(CONS_ERROR, "Error initializing SDL Audio: %s\n", SDL_GetError());
		// call to start audio failed -- we do not have it
		return;
	}

	var_cleanup();

	music = NULL;
	music_volume = sfx_volume = 0;

#if SDL_MIXER_VERSION_ATLEAST(1,2,11)
	Mix_Init(MIX_INIT_FLAC|MIX_INIT_MOD|MIX_INIT_MP3|MIX_INIT_OGG);
#endif

	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048) < 0)
	{
		CONS_Alert(CONS_ERROR, "Error starting SDL_Mixer: %s\n", Mix_GetError());
		// call to start audio failed -- we do not have it
		return;
	}

	sound_started = true;
	songpaused = false;
	Mix_AllocateChannels(256);
}

void I_ShutdownSound(void)
{
	if (!sound_started)
		return; // not an error condition
	sound_started = false;

	Mix_CloseAudio();
#if SDL_MIXER_VERSION_ATLEAST(1,2,11)
	Mix_Quit();
#endif

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

#ifdef HAVE_LIBGME
	if (gme)
		gme_delete(gme);
#endif
}

FUNCMATH void I_UpdateSound(void)
{
}

/// ------------------------
/// SFX
/// ------------------------

// this is as fast as I can possibly make it.
// sorry. more asm needed.
static Mix_Chunk *ds2chunk(void *stream)
{
	UINT16 ver,freq;
	UINT32 samples, i, newsamples;
	UINT8 *sound;

	SINT8 *s;
	INT16 *d;
	INT16 o;
	fixed_t step, frac;

	// lump header
	ver = READUINT16(stream); // sound version format?
	if (ver != 3) // It should be 3 if it's a doomsound...
		return NULL; // onos! it's not a doomsound!
	freq = READUINT16(stream);
	samples = READUINT32(stream);

	// convert from signed 8bit ???hz to signed 16bit 44100hz.
	switch(freq)
	{
	case 44100:
		if (samples >= UINT32_MAX>>2)
			return NULL; // would wrap, can't store.
		newsamples = samples;
		break;
	case 22050:
		if (samples >= UINT32_MAX>>3)
			return NULL; // would wrap, can't store.
		newsamples = samples<<1;
		break;
	case 11025:
		if (samples >= UINT32_MAX>>4)
			return NULL; // would wrap, can't store.
		newsamples = samples<<2;
		break;
	default:
		frac = (44100 << FRACBITS) / (UINT32)freq;
		if (!(frac & 0xFFFF)) // other solid multiples (change if FRACBITS != 16)
			newsamples = samples * (frac >> FRACBITS);
		else // strange and unusual fractional frequency steps, plus anything higher than 44100hz.
			newsamples = FixedMul(FixedDiv(samples, freq), 44100) + 1; // add 1 to counter truncation.
		if (newsamples >= UINT32_MAX>>2)
			return NULL; // would and/or did wrap, can't store.
		break;
	}
	sound = malloc(newsamples<<2); // samples * frequency shift * bytes per sample * channels

	s = (SINT8 *)stream;
	d = (INT16 *)sound;

	i = 0;
	switch(freq)
	{
	case 44100: // already at the same rate? well that makes it simple.
		while(i++ < samples)
		{
			o = ((INT16)(*s++)+0x80)<<8; // changed signedness and shift up to 16 bits
			*d++ = o; // left channel
			*d++ = o; // right channel
		}
		break;
	case 22050: // unwrap 2x
		while(i++ < samples)
		{
			o = ((INT16)(*s++)+0x80)<<8; // changed signedness and shift up to 16 bits
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
		}
		break;
	case 11025: // unwrap 4x
		while(i++ < samples)
		{
			o = ((INT16)(*s++)+0x80)<<8; // changed signedness and shift up to 16 bits
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
		}
		break;
	default: // convert arbitrary hz to 44100.
		step = 0;
		frac = ((UINT32)freq << FRACBITS) / 44100 + 1; //Add 1 to counter truncation.
		while (i < samples)
		{
			o = (INT16)(*s+0x80)<<8; // changed signedness and shift up to 16 bits
			while (step < FRACUNIT) // this is as fast as I can make it.
			{
				*d++ = o; // left channel
				*d++ = o; // right channel
				step += frac;
			}
			do {
				i++; s++;
				step -= FRACUNIT;
			} while (step >= FRACUNIT);
		}
		break;
	}

	// return Mixer Chunk.
	return Mix_QuickLoad_RAW(sound, (Uint32)((UINT8*)d-sound));
}

void *I_GetSfx(sfxinfo_t *sfx)
{
	void *lump;
	Mix_Chunk *chunk;
#ifdef HAVE_LIBGME
	Music_Emu *emu;
	gme_info_t *info;
#endif

	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum(sfx);
	sfx->length = W_LumpLength(sfx->lumpnum);

	lump = W_CacheLumpNum(sfx->lumpnum, PU_SOUND);

	// convert from standard DoomSound format.
	chunk = ds2chunk(lump);
	if (chunk)
	{
		Z_Free(lump);
		return chunk;
	}

	// Not a doom sound? Try something else.
#ifdef HAVE_LIBGME
	// VGZ format
	if (((UINT8 *)lump)[0] == 0x1F
		&& ((UINT8 *)lump)[1] == 0x8B)
	{
#ifdef HAVE_ZLIB
		UINT8 *inflatedData;
		size_t inflatedLen;
		z_stream stream;
		int zErr; // Somewhere to handle any error messages zlib tosses out

		memset(&stream, 0x00, sizeof (z_stream)); // Init zlib stream
		// Begin the inflation process
		inflatedLen = *(UINT32 *)lump + (sfx->length-4); // Last 4 bytes are the decompressed size, typically
		inflatedData = (UINT8 *)Z_Malloc(inflatedLen, PU_SOUND, NULL); // Make room for the decompressed data
		stream.total_in = stream.avail_in = sfx->length;
		stream.total_out = stream.avail_out = inflatedLen;
		stream.next_in = (UINT8 *)lump;
		stream.next_out = inflatedData;

		zErr = inflateInit2(&stream, 32 + MAX_WBITS);
		if (zErr == Z_OK) // We're good to go
		{
			zErr = inflate(&stream, Z_FINISH);
			if (zErr == Z_STREAM_END) {
				// Run GME on new data
				if (!gme_open_data(inflatedData, inflatedLen, &emu, 44100))
				{
					short *mem;
					UINT32 len;
					gme_equalizer_t eq = {GME_TREBLE, GME_BASS, 0,0,0,0,0,0,0,0};

					Z_Free(inflatedData); // GME supposedly makes a copy for itself, so we don't need this lying around
					Z_Free(lump); // We're done with the uninflated lump now, too.

					gme_start_track(emu, 0);
					gme_set_equalizer(emu, &eq);
					gme_track_info(emu, &info, 0);

					len = (info->play_length * 441 / 10) << 2;
					mem = malloc(len);
					gme_play(emu, len >> 1, mem);
					gme_free_info(info);
					gme_delete(emu);

					return Mix_QuickLoad_RAW((Uint8 *)mem, len);
				}
			}
			else
			{
				const char *errorType;
				switch (zErr)
				{
					case Z_ERRNO:
						errorType = "Z_ERRNO"; break;
					case Z_STREAM_ERROR:
						errorType = "Z_STREAM_ERROR"; break;
					case Z_DATA_ERROR:
						errorType = "Z_DATA_ERROR"; break;
					case Z_MEM_ERROR:
						errorType = "Z_MEM_ERROR"; break;
					case Z_BUF_ERROR:
						errorType = "Z_BUF_ERROR"; break;
					case Z_VERSION_ERROR:
						errorType = "Z_VERSION_ERROR"; break;
					default:
						errorType = "unknown error";
				}
				CONS_Alert(CONS_ERROR,"Encountered %s when running inflate: %s\n", errorType, stream.msg);
			}
			(void)inflateEnd(&stream);
		}
		else // Hold up, zlib's got a problem
		{
			const char *errorType;
			switch (zErr)
			{
				case Z_ERRNO:
					errorType = "Z_ERRNO"; break;
				case Z_STREAM_ERROR:
					errorType = "Z_STREAM_ERROR"; break;
				case Z_DATA_ERROR:
					errorType = "Z_DATA_ERROR"; break;
				case Z_MEM_ERROR:
					errorType = "Z_MEM_ERROR"; break;
				case Z_BUF_ERROR:
					errorType = "Z_BUF_ERROR"; break;
				case Z_VERSION_ERROR:
					errorType = "Z_VERSION_ERROR"; break;
				default:
					errorType = "unknown error";
			}
			CONS_Alert(CONS_ERROR,"Encountered %s when running inflateInit: %s\n", errorType, stream.msg);
		}
		Z_Free(inflatedData); // GME didn't open jack, but don't let that stop us from freeing this up
#else
		//CONS_Alert(CONS_ERROR,"Cannot decompress VGZ; no zlib support\n");
#endif
	}
	// Try to read it as a GME sound
	else if (!gme_open_data(lump, sfx->length, &emu, 44100))
	{
		short *mem;
		UINT32 len;
		gme_equalizer_t eq = {GME_TREBLE, GME_BASS, 0,0,0,0,0,0,0,0};

		Z_Free(lump);

		gme_start_track(emu, 0);
		gme_set_equalizer(emu, &eq);
		gme_track_info(emu, &info, 0);

		len = (info->play_length * 441 / 10) << 2;
		mem = malloc(len);
		gme_play(emu, len >> 1, mem);
		gme_free_info(info);
		gme_delete(emu);

		return Mix_QuickLoad_RAW((Uint8 *)mem, len);
	}
#endif

	// Try to load it as a WAVE or OGG using Mixer.
	return Mix_LoadWAV_RW(SDL_RWFromMem(lump, sfx->length), 1);
}

void I_FreeSfx(sfxinfo_t *sfx)
{
	if (sfx->data)
		Mix_FreeChunk(sfx->data);
	sfx->data = NULL;
	sfx->lumpnum = LUMPERROR;
}

INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority)
{
	UINT8 volume = (((UINT16)vol + 1) * (UINT16)sfx_volume) / 62; // (256 * 31) / 62 == 127
	INT32 handle = Mix_PlayChannel(-1, S_sfx[id].data, 0);
	Mix_Volume(handle, volume);
	Mix_SetPanning(handle, min((UINT16)(0xff-sep)<<1, 0xff), min((UINT16)(sep)<<1, 0xff));
	(void)pitch; // Mixer can't handle pitch
	(void)priority; // priority and channel management is handled by SRB2...
	return handle;
}

void I_StopSound(INT32 handle)
{
	Mix_HaltChannel(handle);
}

boolean I_SoundIsPlaying(INT32 handle)
{
	return Mix_Playing(handle);
}

void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch)
{
	UINT8 volume = (((UINT16)vol + 1) * (UINT16)sfx_volume) / 62; // (256 * 31) / 62 == 127
	Mix_Volume(handle, volume);
	Mix_SetPanning(handle, min((UINT16)(0xff-sep)<<1, 0xff), min((UINT16)(sep)<<1, 0xff));
	(void)pitch;
}

void I_SetSfxVolume(UINT8 volume)
{
	sfx_volume = volume;
}

/// ------------------------
/// Music Utilities
/// ------------------------

static UINT32 get_real_volume(UINT8 volume)
{
#ifdef _WIN32
	if (I_SongType() == MU_MID)
		// HACK: Until we stop using native MIDI,
		// disable volume changes
		return ((UINT32)31*128/31); // volume = 31
	else
#endif
		// convert volume to mixer's 128 scale
		// then apply internal_volume as a percentage
		return ((UINT32)volume*128/31) * (UINT32)internal_volume / 100;
}

static UINT32 get_adjusted_position(UINT32 position)
{
	// all in milliseconds
	UINT32 length = I_GetSongLength();
	UINT32 looppoint = I_GetSongLoopPoint();
	if (length)
		return position >= length ? (position % (length-looppoint)) : position;
	else
		return position;
}

static void do_fading_callback()
{
	if (fading_callback)
		(*fading_callback)();
	fading_callback = NULL;
}

/// ------------------------
/// Music Hooks
/// ------------------------

static void count_music_bytes(int chan, void *stream, int len, void *udata)
{
	if (
#ifdef HAVE_LIBGME
		gme ||
#endif
		!music || I_SongType() == MU_MOD || I_SongType() == MU_MID)
		return;
	music_bytes += len;
}

static void music_loop(void)
{
	if (is_looping)
	{
		Mix_PlayMusic(music, 0);
		Mix_SetMusicPosition(loop_point);
		music_bytes = loop_point*44100.0L*4; //assume 44.1khz, 4-byte length (see I_GetSongPosition)
	}
	else
		I_StopSong();
}

static UINT32 music_fade(UINT32 interval, void *param)
{
	if (!is_fading ||
		internal_volume == fading_target ||
		fading_steps == 0 ||
		fading_volume_step == 0)
	{
		I_StopFadingSong();
		do_fading_callback();
		return 0;
	}
	else if (
		(internal_volume > fading_target && internal_volume + fading_volume_step <= fading_target) || // finish fade out
		(internal_volume < fading_target && internal_volume + fading_volume_step >= fading_target)) // finish fade in
	{
		internal_volume = fading_target;
		Mix_VolumeMusic(get_real_volume(music_volume));
		I_StopFadingSong();
		do_fading_callback();
		return 0;
	}
	else
	{
		internal_volume += fading_volume_step;
		Mix_VolumeMusic(get_real_volume(music_volume));
		return interval;
	}
}

#ifdef HAVE_LIBGME
static void mix_gme(void *udata, Uint8 *stream, int len)
{
	int i;
	short *p;

	(void)udata;

	// no gme? no music.
	if (!gme || gme_track_ended(gme) || songpaused)
		return;

	// play gme into stream
	gme_play(gme, len/2, (short *)stream);

	// apply volume to stream
	for (i = 0, p = (short *)stream; i < len/2; i++, p++)
		*p = ((INT32)*p) * (music_volume*internal_volume/100)*2 / 42;
}
#endif


/// ------------------------
/// Music System
/// ------------------------

FUNCMATH void I_InitMusic(void)
{
#ifdef HAVE_LIBGME
	gme = NULL;
	current_track = -1;
#endif
}

void I_ShutdownMusic(void)
{
#ifdef HAVE_LIBGME
	if (gme)
	{
		Mix_HookMusic(NULL, NULL);
		gme_delete(gme);
		gme = NULL;
	}
#endif
	if (!music)
		return;
	var_cleanup();
	SDL_RemoveTimer(fading_id);
	Mix_UnregisterEffect(MIX_CHANNEL_POST, count_music_bytes);
	Mix_HookMusicFinished(NULL);
	Mix_FreeMusic(music);
	music = NULL;
}

/// ------------------------
/// Music Properties
/// ------------------------

musictype_t I_SongType(void)
{
#ifdef HAVE_LIBGME
	if (gme)
		return MU_GME;
	else
#endif
	if (!music)
		return MU_NONE;
	else if (Mix_GetMusicType(music) == MUS_MID)
		return MU_MID;
	else if (Mix_GetMusicType(music) == MUS_MOD || Mix_GetMusicType(music) == MUS_MODPLUG_UNUSED)
		return MU_MOD;
	else if (Mix_GetMusicType(music) == MUS_MP3 || Mix_GetMusicType(music) == MUS_MP3_MAD_UNUSED)
		return MU_MP3;
	else
		return (musictype_t)Mix_GetMusicType(music);
}

boolean I_SongPlaying(void)
{
	return (boolean)music;
}

boolean I_SongPaused(void)
{
	return songpaused;
}

/// ------------------------
/// Music Effects
/// ------------------------

boolean I_SetSongSpeed(float speed)
{
	if (speed > 250.0f)
		speed = 250.0f; //limit speed up to 250x
#ifdef HAVE_LIBGME
	if (gme)
	{
		SDL_LockAudio();
		gme_set_tempo(gme, speed);
		SDL_UnlockAudio();
		return true;
	}
#else
	(void)speed;
#endif
	return false;
}

/// ------------------------
///  MUSIC SEEKING
/// ------------------------

UINT32 I_GetSongLength(void)
{
	INT32 length;

#ifdef HAVE_LIBGME
	if (gme)
	{
		gme_info_t *info;
		gme_err_t gme_e = gme_track_info(gme, &info, current_track);

		if (gme_e != NULL)
		{
			CONS_Alert(CONS_ERROR, "GME error: %s\n", gme_e);
			length = 0;
		}
		else
		{
			// reconstruct info->play_length, from GME source
			// we only want intro + 1 loop, not 2
			length = info->length;
			if (length <= 0)
			{
				length = info->intro_length + info->loop_length; // intro + 1 loop
				if (length <= 0)
					length = 150 * 1000; // 2.5 minutes
			}
		}

		gme_free_info(info);
		return max(length, 0);
	}
	else
#endif
	if (!music || I_SongType() == MU_MOD || I_SongType() == MU_MID)
		return 0;
	else
	{
		// VERY IMPORTANT to set your LENGTHMS= in your song files, folks!
		// SDL mixer can't read music length itself.
		length = (UINT32)(song_length*1000);
		if (!length)
			CONS_Debug(DBG_DETAILED, "Getting music length: music is missing LENGTHMS= tag. Needed for seeking.\n");
		return length;
	}
}

boolean I_SetSongLoopPoint(UINT32 looppoint)
{
	if (
#ifdef HAVE_LIBGME
		gme ||
#endif
		!music || I_SongType() == MU_MOD || I_SongType() == MU_MID || !is_looping)
		return false;
	else
	{
		UINT32 length = I_GetSongLength();

		if (length > 0)
			looppoint %= length;

		loop_point = max((float)(looppoint / 1000.0L), 0);
		return true;
	}
}

UINT32 I_GetSongLoopPoint(void)
{
#ifdef HAVE_LIBGME
	if (gme)
	{
		INT32 looppoint;
		gme_info_t *info;
		gme_err_t gme_e = gme_track_info(gme, &info, current_track);

		if (gme_e != NULL)
		{
			CONS_Alert(CONS_ERROR, "GME error: %s\n", gme_e);
			looppoint = 0;
		}
		else
			looppoint = info->intro_length > 0 ? info->intro_length : 0;

		gme_free_info(info);
		return max(looppoint, 0);
	}
	else
#endif
	if (!music || I_SongType() == MU_MOD || I_SongType() == MU_MID)
		return 0;
	else
		return (UINT32)(loop_point * 1000);
}

boolean I_SetSongPosition(UINT32 position)
{
	UINT32 length;
#ifdef HAVE_LIBGME
	if (gme)
	{
		// this isn't required technically, but GME thread-locks for a second
		// if you seek too high from the counter
		length = I_GetSongLength();
		if (length)
			position = get_adjusted_position(position);

		SDL_LockAudio();
		gme_err_t gme_e = gme_seek(gme, position);
		SDL_UnlockAudio();

		if (gme_e != NULL)
		{
			CONS_Alert(CONS_ERROR, "GME error: %s\n", gme_e);
			return false;
		}
		else
			return true;
	}
	else
#endif
	if (!music || I_SongType() == MU_MID)
		return false;
	else if (I_SongType() == MU_MOD)
		return Mix_SetMusicPosition(position); // Goes by channels
	else
	{
		// Because SDL mixer can't identify song length, if you have
		// a position input greater than the real length, then
		// music_bytes becomes inaccurate.

		length = I_GetSongLength(); // get it in MS
		if (length)
			position = get_adjusted_position(position);

		Mix_RewindMusic(); // needed for mp3
		if(Mix_SetMusicPosition((float)(position/1000.0L)) == 0)
			music_bytes = position/1000.0L*44100.0L*4; //assume 44.1khz, 4-byte length (see I_GetSongPosition)
		else
			// NOTE: This block fires on incorrect song format,
			// NOT if position input is greater than song length.
			music_bytes = 0;

		return true;
	}
}

UINT32 I_GetSongPosition(void)
{
#ifdef HAVE_LIBGME
	if (gme)
	{
		INT32 position = gme_tell(gme);

		gme_info_t *info;
		gme_err_t gme_e = gme_track_info(gme, &info, current_track);

		if (gme_e != NULL)
		{
			CONS_Alert(CONS_ERROR, "GME error: %s\n", gme_e);
			return position;
		}
		else
		{
			// adjust position, since GME's counter keeps going past loop
			if (info->length > 0)
				position %= info->length;
			else if (info->intro_length + info->loop_length > 0)
				position = position >= (info->intro_length + info->loop_length) ? (position % info->loop_length) : position;
			else
				position %= 150 * 1000; // 2.5 minutes
		}

		gme_free_info(info);
		return max(position, 0);
	}
	else
#endif
	if (!music || I_SongType() == MU_MID)
		return 0;
	else
		return music_bytes/44100.0L*1000.0L/4; //assume 44.1khz
		// 4 = byte length for 16-bit samples (AUDIO_S16SYS), stereo (2-channel)
		// This is hardcoded in I_StartupSound. Other formats for factor:
		// 8M: 1 | 8S: 2 | 16M: 2 | 16S: 4
}

/// ------------------------
/// Music Playback
/// ------------------------

boolean I_LoadSong(char *data, size_t len)
{
	I_Assert(!music);
#ifdef HAVE_LIBGME
	I_Assert(!gme);
#endif

	var_cleanup();

#ifdef HAVE_LIBGME
	if ((UINT8)data[0] == 0x1F
		&& (UINT8)data[1] == 0x8B)
	{
#ifdef HAVE_ZLIB
		UINT8 *inflatedData;
		size_t inflatedLen;
		z_stream stream;
		int zErr; // Somewhere to handle any error messages zlib tosses out

		memset(&stream, 0x00, sizeof (z_stream)); // Init zlib stream
		// Begin the inflation process
		inflatedLen = *(UINT32 *)(data + (len-4)); // Last 4 bytes are the decompressed size, typically
		inflatedData = (UINT8 *)Z_Calloc(inflatedLen, PU_MUSIC, NULL); // Make room for the decompressed data
		stream.total_in = stream.avail_in = len;
		stream.total_out = stream.avail_out = inflatedLen;
		stream.next_in = (UINT8 *)data;
		stream.next_out = inflatedData;

		zErr = inflateInit2(&stream, 32 + MAX_WBITS);
		if (zErr == Z_OK) // We're good to go
		{
			zErr = inflate(&stream, Z_FINISH);
			if (zErr == Z_STREAM_END) {
				// Run GME on new data
				if (!gme_open_data(inflatedData, inflatedLen, &gme, 44100))
				{
					gme_equalizer_t eq = {GME_TREBLE, GME_BASS, 0,0,0,0,0,0,0,0};
					gme_start_track(gme, 0);
					current_track = 0;
					gme_set_equalizer(gme, &eq);
					Mix_HookMusic(mix_gme, gme);
					Z_Free(inflatedData); // GME supposedly makes a copy for itself, so we don't need this lying around
					return true;
				}
			}
			else
			{
				const char *errorType;
				switch (zErr)
				{
					case Z_ERRNO:
						errorType = "Z_ERRNO"; break;
					case Z_STREAM_ERROR:
						errorType = "Z_STREAM_ERROR"; break;
					case Z_DATA_ERROR:
						errorType = "Z_DATA_ERROR"; break;
					case Z_MEM_ERROR:
						errorType = "Z_MEM_ERROR"; break;
					case Z_BUF_ERROR:
						errorType = "Z_BUF_ERROR"; break;
					case Z_VERSION_ERROR:
						errorType = "Z_VERSION_ERROR"; break;
					default:
						errorType = "unknown error";
				}
				CONS_Alert(CONS_ERROR,"Encountered %s when running inflate: %s\n", errorType, stream.msg);
			}
			(void)inflateEnd(&stream);
		}
		else // Hold up, zlib's got a problem
		{
			const char *errorType;
			switch (zErr)
			{
				case Z_ERRNO:
					errorType = "Z_ERRNO"; break;
				case Z_STREAM_ERROR:
					errorType = "Z_STREAM_ERROR"; break;
				case Z_DATA_ERROR:
					errorType = "Z_DATA_ERROR"; break;
				case Z_MEM_ERROR:
					errorType = "Z_MEM_ERROR"; break;
				case Z_BUF_ERROR:
					errorType = "Z_BUF_ERROR"; break;
				case Z_VERSION_ERROR:
					errorType = "Z_VERSION_ERROR"; break;
				default:
					errorType = "unknown error";
			}
			CONS_Alert(CONS_ERROR,"Encountered %s when running inflateInit: %s\n", errorType, stream.msg);
		}
		Z_Free(inflatedData); // GME didn't open jack, but don't let that stop us from freeing this up
#else
		//CONS_Alert(CONS_ERROR,"Cannot decompress VGZ; no zlib support\n");
#endif
	}
	else if (!gme_open_data(data, len, &gme, 44100))
	{
		gme_equalizer_t eq = {GME_TREBLE, GME_BASS, 0,0,0,0,0,0,0,0};
		gme_set_equalizer(gme, &eq);
		return true;
	}
#endif

	music = Mix_LoadMUS_RW(SDL_RWFromMem(data, len), SDL_FALSE);
	if (!music)
	{
		CONS_Alert(CONS_ERROR, "Mix_LoadMUS_RW: %s\n", Mix_GetError());
		return false;
	}

	if (I_SongType() == MU_MP3)
	{
		CONS_Debug(DBG_BASIC, "MP3 songs are unsupported and may crash! Use OGG instead.\n");
		CONS_Debug(DBG_DETAILED, "MP3 songs are unsupported and may crash! Use OGG instead.\n");
	}

	// Find the OGG loop point.
	loop_point = 0.0f;
	song_length = 0.0f;

	const char *key1 = "LOOP";
	const char *key2 = "POINT=";
	const char *key3 = "MS=";
	const char *key4 = "LENGTHMS=";
	const size_t key1len = strlen(key1);
	const size_t key2len = strlen(key2);
	const size_t key3len = strlen(key3);
	const size_t key4len = strlen(key4);

	// for mp3 wide chars
	const char *key1w = "L\0O\0O\0P\0";
	const char *key2w = "P\0O\0I\0N\0T\0\0\0\xFF\xFE";
	const char *key3w = "M\0S\0\0\0\xFF\xFE";
	const char *key4w = "L\0E\0N\0G\0T\0H\0M\0S\0\0\0\xFF\xFE";
	const char *wterm = "\0\0";
	char wval[10];

	size_t wstart, wp;
	char *p = data;
	while ((UINT32)(p - data) < len)
	{
		if (!loop_point && !strncmp(p, key1, key1len))
		{
			p += key1len; // skip LOOP
			if (!strncmp(p, key2, key2len)) // is it LOOPPOINT=?
			{
				p += key2len; // skip POINT=
				loop_point = (float)((44.1L+atoi(p)) / 44100.0L); // LOOPPOINT works by sample count.
				// because SDL_Mixer is USELESS and can't even tell us
				// something simple like the frequency of the streaming music,
				// we are unfortunately forced to assume that ALL MUSIC is 44100hz.
				// This means a lot of tracks that are only 22050hz for a reasonable downloadable file size will loop VERY badly.
			}
			else if (!strncmp(p, key3, key3len)) // is it LOOPMS=?
			{
				p += key3len; // skip MS=
				loop_point = (float)(atoi(p) / 1000.0L); // LOOPMS works by real time, as miliseconds.
				// Everything that uses LOOPMS will work perfectly with SDL_Mixer.
			}
		}
		else if (!song_length && !strncmp(p, key4, key4len)) // is it LENGTHMS=?
		{
			p += key4len; // skip LENGTHMS
			song_length = (float)(atoi(p) / 1000.0L);
		}
		// below: search MP3 or other tags that use wide char encoding
		else if (!loop_point && !memcmp(p, key1w, key1len*2)) // LOOP wide char
		{
			p += key1len*2;
			if (!memcmp(p, key2w, (key2len+1)*2)) // POINT= wide char
			{
				p += (key2len+1)*2;
				wstart = (size_t)p;
				wp = 0;
				while (wp < 9 && memcmp(p, wterm, 2))
				{
					wval[wp] = *p;
					p += 2;
					wp = ((size_t)(p-wstart))/2;
				}
				wval[min(wp, 9)] = 0;
				loop_point = (float)((44.1L+atoi(wval) / 44100.0L));
			}
			else if (!memcmp(p, key3w, (key3len+1)*2)) // MS= wide char
			{
				p += (key3len+1)*2;
				wstart = (size_t)p;
				wp = 0;
				while (wp < 9 && memcmp(p, wterm, 2))
				{
					wval[wp] = *p;
					p += 2;
					wp = ((size_t)(p-wstart))/2;
				}
				wval[min(wp, 9)] = 0;
				loop_point = (float)(atoi(wval) / 1000.0L);
			}
		}
		else if (!song_length && !memcmp(p, key4w, (key4len+1)*2)) // LENGTHMS= wide char
		{
			p += (key4len+1)*2;
			wstart = (size_t)p;
			wp = 0;
			while (wp < 9 && memcmp(p, wterm, 2))
			{
				wval[wp] = *p;
				p += 2;
				wp = ((size_t)(p-wstart))/2;
			}
			wval[min(wp, 9)] = 0;
			song_length = (float)(atoi(wval) / 1000.0L);
		}

		if (loop_point && song_length && song_length > loop_point) // Got what we needed
			// the last case is a sanity check, in case the wide char searches were false matches.
			break;
		else // continue searching
			p++;
	}
	return true;
}

void I_UnloadSong(void)
{
	// \todo unhook looper
	//var_cleanup();
	//Mix_FreeMusic(music);
	//music = NULL;
	I_StopSong();
}

boolean I_PlaySong(boolean looping)
{
#ifdef HAVE_LIBGME
	if (gme)
	{
		gme_start_track(gme, 0);
		current_track = 0;
		Mix_HookMusic(mix_gme, gme);
		return true;
	}
	else
#endif
	if (!music)
		return false;

	if (!song_length && (I_SongType() == MU_OGG || I_SongType() == MU_MP3 || I_SongType() == MU_FLAC))
		CONS_Debug(DBG_DETAILED, "This song is missing a LENGTHMS= tag! Required to make seeking work properly.\n");

	if (I_SongType() != MU_MOD && I_SongType() != MU_MID && Mix_PlayMusic(music, 0) == -1)
	{
		CONS_Alert(CONS_ERROR, "Mix_PlayMusic: %s\n", Mix_GetError());
		return false;
	}
	else if ((I_SongType() == MU_MOD || I_SongType() == MU_MID) && Mix_PlayMusic(music, -1) == -1) // if MOD, loop forever
	{
		CONS_Alert(CONS_ERROR, "Mix_PlayMusic: %s\n", Mix_GetError());
		return false;
	}

	is_looping = looping;

	I_SetMusicVolume(music_volume);

	if (I_SongType() != MU_MOD && I_SongType() != MU_MID)
		Mix_HookMusicFinished(music_loop); // don't bother counting if MOD

	if(I_SongType() != MU_MOD && I_SongType() != MU_MID && !Mix_RegisterEffect(MIX_CHANNEL_POST, count_music_bytes, NULL, NULL))
		CONS_Alert(CONS_WARNING, "Error registering SDL music position counter: %s\n", Mix_GetError());

	return true;
}

void I_StopSong(void)
{
#ifdef HAVE_LIBGME
	if (gme)
	{
		Mix_HookMusic(NULL, NULL);
		gme_delete(gme);
		gme = NULL;
		current_track = -1;
		return;
	}
#endif
	if (!music)
		return;
	var_cleanup();
	Mix_UnregisterEffect(MIX_CHANNEL_POST, count_music_bytes);
	Mix_HookMusicFinished(NULL);
	Mix_FreeMusic(music);
	music = NULL;
}

void I_PauseSong()
{
	if(I_SongType() == MU_MID) // really, SDL Mixer? why can't you pause MIDI???
		return;

	if(
#ifdef HAVE_LIBGME
		!gme &&
#endif
		I_SongType() != MU_MOD && I_SongType() != MU_MID)
		Mix_UnregisterEffect(MIX_CHANNEL_POST, count_music_bytes);

	Mix_PauseMusic();
	songpaused = true;
}

void I_ResumeSong()
{
	if (I_SongType() == MU_MID)
		return;

	if (
#ifdef HAVE_LIBGME
		!gme &&
#endif
		I_SongType() != MU_MOD && I_SongType() != MU_MID)
	{
		while(Mix_UnregisterEffect(MIX_CHANNEL_POST, count_music_bytes) != 0) { }
			// HACK: fixes issue of multiple effect callbacks being registered

		if(music && I_SongType() != MU_MOD && I_SongType() != MU_MID && !Mix_RegisterEffect(MIX_CHANNEL_POST, count_music_bytes, NULL, NULL))
			CONS_Alert(CONS_WARNING, "Error registering SDL music position counter: %s\n", Mix_GetError());
	}

	Mix_ResumeMusic();
	songpaused = false;
}

void I_SetMusicVolume(UINT8 volume)
{
	if (!I_SongPlaying())
		return;

#ifdef _WIN32
	if (I_SongType() == MU_MID)
		// HACK: Until we stop using native MIDI,
		// disable volume changes
		music_volume = 31;
	else
#endif
		music_volume = volume;

	Mix_VolumeMusic(get_real_volume(music_volume));
}

boolean I_SetSongTrack(int track)
{
#ifdef HAVE_LIBGME
	if (current_track == track)
		return false;

	// If the specified track is within the number of tracks playing, then change it
	if (gme)
	{
		SDL_LockAudio();
		if (track >= 0
			&& track < gme_track_count(gme))
		{
			gme_err_t gme_e = gme_start_track(gme, track);
			if (gme_e != NULL)
			{
				CONS_Alert(CONS_ERROR, "GME error: %s\n", gme_e);
				return false;
			}
			current_track = track;
			SDL_UnlockAudio();
			return true;
		}
		SDL_UnlockAudio();
		return false;
	}
	else
#endif
	if (I_SongType() == MU_MOD)
		return !Mix_SetMusicPosition(track);
	(void)track;
	return false;
}

/// ------------------------
/// MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(UINT8 volume)
{
	internal_volume = volume;
	if (!I_SongPlaying())
		return;
	Mix_VolumeMusic(get_real_volume(music_volume));
}

void I_StopFadingSong(void)
{
	if (fading_id)
		SDL_RemoveTimer(fading_id);
	is_fading = false;
	fading_target = fading_steps = fading_volume_step = fading_id = 0;
}

boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void))
{
	UINT32 target_steps, ms_per_step;
	INT16 target_volume_step, volume_delta;

	source_volume = min(source_volume, 100);
	volume_delta = (INT16)(target_volume - source_volume);

	I_StopFadingSong();

	if (!ms && volume_delta)
	{
		I_SetInternalMusicVolume(target_volume);
		if (callback)
			(*callback)();
		return true;

	}
	else if (!volume_delta)
	{
		if (callback)
			(*callback)();
		return true;
	}

	// Round MS to nearest 10
	// If n - lower > higher - n, then round up
	ms = (ms - ((ms / 10) * 10) > (((ms / 10) * 10) + 10) - ms) ?
		(((ms / 10) * 10) + 10) // higher
		: ((ms / 10) * 10); // lower

	ms_per_step = max(10, ms / abs(volume_delta));
		// 10ms is the usual minimum timer granularity, but platform-dependent
	target_steps = ms/ms_per_step;
	target_volume_step = volume_delta / (INT16)target_steps;

	if (!target_steps || !target_volume_step)
		I_SetInternalMusicVolume(target_volume);
	else if (source_volume != target_volume)
	{
		fading_id = SDL_AddTimer(ms_per_step, music_fade, NULL);
		if (fading_id)
		{
			is_fading = true;
			fading_target = target_volume;
			fading_steps = target_steps;
			fading_volume_step = target_volume_step;
			fading_callback = callback;

			if (internal_volume != source_volume)
				I_SetInternalMusicVolume(source_volume);
		}
	}

	return is_fading;
}

boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void))
{
	return I_FadeSongFromVolume(target_volume, internal_volume, ms, callback);
}

boolean I_FadeOutStopSong(UINT32 ms)
{
	return I_FadeSongFromVolume(0, internal_volume, ms, &I_StopSong);
}

boolean I_FadeInPlaySong(UINT32 ms, boolean looping)
{
	if (I_PlaySong(looping))
		return I_FadeSongFromVolume(100, 0, ms, NULL);
	else
		return false;
}
#endif
