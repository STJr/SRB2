/// \file
/// \brief FMOD Ex interface for sound
#include "../doomdef.h"
#include "../sounds.h"
#include "../i_sound.h"
#include "../s_sound.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../byteptr.h"

#include <fmod.h> // FMOD Ex
#define errcode _errcode
#include <fmod_errors.h>
#undef errcode

#ifdef HAVE_LIBGME
#include "gme/gme.h"
#define GME_TREBLE 5.0
#define GME_BASS 1.0

#ifdef HAVE_ZLIB
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
#endif // HAVE_ZLIB
#endif // HAVE_LIBGME

static FMOD_SYSTEM *fsys;
static FMOD_SOUND *music_stream;
static FMOD_CHANNEL *music_channel;

static UINT8 music_volume, midi_volume, sfx_volume;
static INT32 current_track;

#ifdef HAVE_LIBGME
static Music_Emu *gme;
#endif

//
// SYSTEM
//

// spew console messages for general errors.
static void FMR_Debug(FMOD_RESULT e, int line)
{
	if (e != FMOD_OK)
		CONS_Alert(CONS_ERROR, "FMOD:%d - %s\n", line, FMOD_ErrorString(e));
}
#define FMR(x) FMR_Debug(x, __LINE__)

// for operations on music_channel ONLY.
// returns false if music was released.
// (in other words, if !FMR_MUSIC(op) return immediately,
// because music_stream and music_channel are no longer valid.)
static boolean FMR_Music_Debug(FMOD_RESULT e, int line)
{
	switch(e)
	{
	case FMOD_ERR_INVALID_HANDLE: // non-looping song ended
	case FMOD_ERR_CHANNEL_STOLEN: // song ended and then sfx was played
		FMOD_Sound_Release(music_stream);
		music_stream = NULL;
		return false;
	default:
		FMR_Debug(e, line);
		break;
	}
	return true;
}
#define FMR_MUSIC(x) FMR_Music_Debug(x, __LINE__)

void I_StartupSound(void)
{
	I_Assert(!sound_started);
	sound_started = true;

	FMR(FMOD_System_Create(&fsys));
	FMR(FMOD_System_SetDSPBufferSize(fsys, 44100 / 30, 2));
	FMR(FMOD_System_Init(fsys, 64, FMOD_INIT_VOL0_BECOMES_VIRTUAL, NULL));
	music_stream = NULL;
#ifdef HAVE_LIBGME
	gme = NULL;
#endif
	current_track = -1;
}

void I_ShutdownSound(void)
{
	I_Assert(sound_started);
	sound_started = false;

#ifdef HAVE_LIBGME
	if (gme)
		gme_delete(gme);
#endif
	FMR(FMOD_System_Release(fsys));
}

void I_UpdateSound(void)
{
	FMR(FMOD_System_Update(fsys));
}

#ifdef HAVE_LIBGME
// Callback hook to read streaming GME data.
static FMOD_RESULT F_CALLBACK GMEReadCallback(FMOD_SOUND *sound, void *data, unsigned int datalen)
{
	Music_Emu *emu;
	void *emuvoid = NULL;
	// get our emu
	FMR(FMOD_Sound_GetUserData(sound, &emuvoid));
	emu = emuvoid;
	// no emu? no play.
	if (!emu)
		return FMOD_ERR_FILE_EOF;
	if (gme_track_ended(emu))
	{
		// don't delete the primary music stream
		if (emu == gme)
			return FMOD_ERR_FILE_EOF;
		// do delete sfx streams
		FMR(FMOD_Sound_SetUserData(sound, NULL));
		gme_delete(emu);
		return FMOD_ERR_FILE_EOF;
	}
	// play beautiful musics theme of ancient land.
	if (gme_play(emu, datalen/2, data))
		return FMOD_ERR_FILE_BAD;
	// O.K
	return FMOD_OK;
}
#endif

//
// SFX
//

// convert doom format sound (signed 8bit)
// to an fmod format sound (unsigned 8bit)
// and return the FMOD_SOUND.
static inline FMOD_SOUND *ds2fmod(char *stream)
{
	FMOD_SOUND *sound;
	UINT16 ver;
	UINT16 freq;
	UINT32 samples;
	FMOD_CREATESOUNDEXINFO fmt;
	UINT32 i;
	UINT8 *p;

	// lump header
	ver = READUINT16(stream); // sound version format?
	if (ver != 3) // It should be 3 if it's a doomsound...
		return NULL; // onos! it's not a doomsound!
	freq = READUINT16(stream);
	samples = READUINT32(stream);
	//CONS_Printf("FMT: v%d f%d, s%d, b%d\n", ver, freq, samples, bps);

	memset(&fmt, 0, sizeof(FMOD_CREATESOUNDEXINFO));
	fmt.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

	// convert to unsigned
	fmt.format = FMOD_SOUND_FORMAT_PCM8; // 8bit
	fmt.length = samples*1; // 1 bps
	fmt.numchannels = 1; // mono
	fmt.defaultfrequency = freq;

	// Make a duplicate of the stream to change format.
	p = Z_Malloc(fmt.length, PU_SOUND, NULL);
	for (i = 0; i < fmt.length; i++)
		p[i] = (UINT8)(stream[i]+0x80); // convert from signed to unsigned
	stream = (char *)p;

	// Create FMOD_SOUND.
	FMR(FMOD_System_CreateSound(fsys, stream, FMOD_CREATESAMPLE|FMOD_OPENMEMORY|FMOD_OPENRAW|FMOD_SOFTWARE|FMOD_LOWMEM, &fmt, &sound));

	Z_Free(stream); // FMOD made its own copy, so we don't need this anymore.
	return sound;
}

void *I_GetSfx(sfxinfo_t *sfx)
{
	FMOD_SOUND *sound;
	char *lump;
	FMOD_CREATESOUNDEXINFO fmt;
#ifdef HAVE_LIBGME
	Music_Emu *emu;
	gme_info_t *info;
#endif

	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum(sfx);
	sfx->length = W_LumpLength(sfx->lumpnum);

	lump = W_CacheLumpNum(sfx->lumpnum, PU_SOUND);
	sound = ds2fmod(lump);
	if (sound)
	{
		Z_Free(lump);
		return sound;
	}

	// It's not a doom sound.
	// Try to read it as an FMOD sound?

	memset(&fmt, 0, sizeof(FMOD_CREATESOUNDEXINFO));
	fmt.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	fmt.length = sfx->length;

#ifdef HAVE_LIBGME
	// VGZ format
	if ((UINT8)lump[0] == 0x1F
		&& (UINT8)lump[1] == 0x8B)
	{
#ifdef HAVE_ZLIB
		UINT8 *inflatedData;
		size_t inflatedLen;
		z_stream stream;
		int zErr; // Somewhere to handle any error messages zlib tosses out

		memset(&stream, 0x00, sizeof (z_stream)); // Init zlib stream
		// Begin the inflation process
		inflatedLen = *(UINT32 *)(lump + (sfx->length-4)); // Last 4 bytes are the decompressed size, typically
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
					Z_Free(inflatedData); // GME supposedly makes a copy for itself, so we don't need this lying around
					Z_Free(lump); // We're done with the uninflated lump now, too.

					gme_start_track(emu, 0);
					gme_track_info(emu, &info, 0);

					fmt.format = FMOD_SOUND_FORMAT_PCM16;
					fmt.defaultfrequency = 44100;
					fmt.numchannels = 2;
					fmt.length = ((UINT32)info->play_length * 441 / 10) * 4;
					fmt.decodebuffersize = (44100 * 2) / 35;
					fmt.pcmreadcallback = GMEReadCallback;
					fmt.userdata = emu;

					FMR(FMOD_System_CreateSound(fsys, NULL, FMOD_CREATESAMPLE|FMOD_OPENUSER|FMOD_SOFTWARE|FMOD_LOWMEM, &fmt, &sound));
					return sound;
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
		Z_Free(lump);

		gme_start_track(emu, 0);
		gme_track_info(emu, &info, 0);

		fmt.format = FMOD_SOUND_FORMAT_PCM16;
		fmt.defaultfrequency = 44100;
		fmt.numchannels = 2;
		fmt.length = ((UINT32)info->play_length * 441 / 10) * 4;
		fmt.decodebuffersize = (44100 * 2) / 35;
		fmt.pcmreadcallback = GMEReadCallback;
		fmt.userdata = emu;

		gme_free_info(info);

		FMR(FMOD_System_CreateSound(fsys, NULL, FMOD_CREATESAMPLE|FMOD_OPENUSER|FMOD_SOFTWARE|FMOD_LOWMEM, &fmt, &sound));
		return sound;
	}
#endif

	// Ogg, Mod, Midi, etc.
	FMR(FMOD_System_CreateSound(fsys, lump, FMOD_CREATESAMPLE|FMOD_OPENMEMORY|FMOD_SOFTWARE|FMOD_LOWMEM, &fmt, &sound));
	Z_Free(lump); // We're done with the lump now, at least.
	return sound;
}

void I_FreeSfx(sfxinfo_t *sfx)
{
	if (sfx->data)
		FMOD_Sound_Release(sfx->data);
	sfx->data = NULL;
}

INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority, INT32 channel)
{
	FMOD_SOUND *sound;
	FMOD_CHANNEL *chan;
	INT32 i;
	float frequency;
	(void)channel;

	sound = (FMOD_SOUND *)S_sfx[id].data;
	I_Assert(sound != NULL);

	FMR(FMOD_System_PlaySound(fsys, FMOD_CHANNEL_FREE, sound, true, &chan));

	if (sep == 0)
		sep = 1;

	FMR(FMOD_Channel_SetVolume(chan, (vol / 255.0) * (sfx_volume / 31.0)));
	FMR(FMOD_Channel_SetPan(chan, (sep - 128) / 127.0));

	FMR(FMOD_Sound_GetDefaults(sound, &frequency, NULL, NULL, NULL));
	FMR(FMOD_Channel_SetFrequency(chan, (pitch / 128.0) * frequency));

	FMR(FMOD_Channel_SetPriority(chan, priority));
	//UNREFERENCED_PARAMETER(priority);
	//FMR(FMOD_Channel_SetPriority(chan, 1 + ((0xff-vol)>>1))); // automatic priority 1 - 128 based on volume (priority 0 is music)

	FMR(FMOD_Channel_GetIndex(chan, &i));
	FMR(FMOD_Channel_SetPaused(chan, false));
	return i;
}

void I_StopSound(INT32 handle)
{
	FMOD_CHANNEL *chan;
	FMR(FMOD_System_GetChannel(fsys, handle, &chan));
	if (music_stream && chan == music_channel)
		return;
	FMR(FMOD_Channel_Stop(chan));
}

boolean I_SoundIsPlaying(INT32 handle)
{
	FMOD_CHANNEL *chan;
	FMOD_BOOL playing;
	FMOD_RESULT e;
	FMR(FMOD_System_GetChannel(fsys, handle, &chan));
	if (music_stream && chan == music_channel)
		return false;
	e = FMOD_Channel_IsPlaying(chan, &playing);
	switch(e)
	{
	case FMOD_ERR_INVALID_HANDLE: // Sound effect finished.
	case FMOD_ERR_CHANNEL_STOLEN: // Sound effect interrupted. -- technically impossible due to GetChannel by handle. :(
		return false; // therefore, it's not playing anymore.
	default:
		FMR(e);
		break;
	}
	return (boolean)playing;
}

// seems to never be called on an invalid channel (calls I_SoundIsPlaying first?)
// so I'm not gonna worry about it.
void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch)
{
	FMOD_CHANNEL *chan;
	FMOD_SOUND *sound;
	float frequency;

	FMR(FMOD_System_GetChannel(fsys, handle, &chan));
	FMR(FMOD_Channel_SetVolume(chan, (vol / 255.0) * (sfx_volume / 31.0)));
	FMR(FMOD_Channel_SetPan(chan, (sep - 128) / 127.0));

	FMR(FMOD_Channel_GetCurrentSound(chan, &sound));
	FMR(FMOD_Sound_GetDefaults(sound, &frequency, NULL, NULL, NULL));
	FMR(FMOD_Channel_SetFrequency(chan, (pitch / 128.0) * frequency));

	//FMR(FMOD_Channel_SetPriority(chan, 1 + ((0xff-vol)>>1))); // automatic priority 1 - 128 based on volume (priority 0 is music)
}

void I_SetSfxVolume(UINT8 volume)
{
	// volume is 0 to 31.
	sfx_volume = volume;
}

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

void I_InitMusic(void)
{
}

void I_ShutdownMusic(void)
{
	I_StopSong();
}

/// ------------------------
//  MUSIC PROPERTIES
/// ------------------------

musictype_t I_SongType(void)
{
	FMOD_SOUND_TYPE type;

#ifdef HAVE_LIBGME
	if (gme)
		return MU_GME;
#endif

	if (!music_stream)
		return MU_NONE;

	if (FMOD_Sound_GetFormat(music_stream, &type, NULL, NULL, NULL) == FMOD_OK)
	{
		switch(type)
		{
			case FMOD_SOUND_TYPE_WAV:
				return MU_WAV;
			case FMOD_SOUND_TYPE_MOD:
				return MU_MOD;
			case FMOD_SOUND_TYPE_MIDI:
				return MU_MID;
			case FMOD_SOUND_TYPE_OGGVORBIS:
				return MU_OGG;
			case FMOD_SOUND_TYPE_MPEG:
				return MU_MP3;
			case FMOD_SOUND_TYPE_FLAC:
				return MU_FLAC;
			default:
				return MU_NONE;
		}
	}
	else
		return MU_NONE;
}

boolean I_SongPlaying(void)
{
	return (music_stream != NULL);
}

boolean I_SongPaused(void)
{
	FMOD_BOOL fmpaused = false;
	if (music_stream)
		FMOD_Channel_GetPaused(music_channel, &fmpaused);
	return (boolean)fmpaused;
}

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

boolean I_SetSongSpeed(float speed)
{
	FMOD_RESULT e;
	float frequency;
	if (!music_stream)
		return false;
	if (speed > 250.0f)
		speed = 250.0f; //limit speed up to 250x

#ifdef HAVE_LIBGME
	// Try to set GME speed
	if (gme)
	{
		gme_set_tempo(gme, speed);
		return true;
	}
#endif

	// Try to set Mod/Midi speed
	e = FMOD_Sound_SetMusicSpeed(music_stream, speed);

	if (e == FMOD_ERR_FORMAT)
	{
		// Just change pitch instead for Ogg/etc.
		FMR(FMOD_Sound_GetDefaults(music_stream, &frequency, NULL, NULL, NULL));
		FMR_MUSIC(FMOD_Channel_SetFrequency(music_channel, speed*frequency));
	}
	else
		FMR_MUSIC(e);

	return true;
}

/// ------------------------
//  MUSIC PLAYBACK
/// ------------------------

boolean I_LoadSong(char *data, size_t len)
{
	FMOD_CREATESOUNDEXINFO fmt;
	FMOD_RESULT e;
	FMOD_TAG tag;
	unsigned int loopstart, loopend;

	if (
#ifdef HAVE_LIBGME
		gme ||
#endif
		music_stream
	)
		I_UnloadSong();

	memset(&fmt, 0, sizeof(FMOD_CREATESOUNDEXINFO));
	fmt.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

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
					gme_equalizer_t gmeq = {GME_TREBLE, GME_BASS, 0,0,0,0,0,0,0,0};
					Z_Free(inflatedData); // GME supposedly makes a copy for itself, so we don't need this lying around
					Z_Free(data); // We don't need this, either.
					gme_set_equalizer(gme,&gmeq);
					fmt.format = FMOD_SOUND_FORMAT_PCM16;
					fmt.defaultfrequency = 44100;
					fmt.numchannels = 2;
					fmt.length = -1;
					fmt.decodebuffersize = (44100 * 2) / 35;
					fmt.pcmreadcallback = GMEReadCallback;
					fmt.userdata = gme;
					FMR(FMOD_System_CreateStream(fsys, NULL, FMOD_OPENUSER, &fmt, &music_stream));
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
		gme_equalizer_t gmeq = {GME_TREBLE, GME_BASS, 0,0,0,0,0,0,0,0};
		Z_Free(data); // We don't need this anymore.
		gme_set_equalizer(gme,&gmeq);
		fmt.format = FMOD_SOUND_FORMAT_PCM16;
		fmt.defaultfrequency = 44100;
		fmt.numchannels = 2;
		fmt.length = -1;
		fmt.decodebuffersize = (44100 * 2) / 35;
		fmt.pcmreadcallback = GMEReadCallback;
		fmt.userdata = gme;
		FMR(FMOD_System_CreateStream(fsys, NULL, FMOD_OPENUSER, &fmt, &music_stream));
		return true;
	}
#endif

	fmt.length = len;

	e = FMOD_System_CreateStream(fsys, data, FMOD_OPENMEMORY_POINT, &fmt, &music_stream);
	if (e != FMOD_OK)
	{
		if (e == FMOD_ERR_FORMAT)
			CONS_Alert(CONS_WARNING, "Failed to play music lump due to invalid format.\n");
		else
			FMR(e);
		return false;
	}

	// Try to find a loop point in streaming music formats (ogg, mp3)

	// A proper LOOPPOINT is its own tag, stupid.
	e = FMOD_Sound_GetTag(music_stream, "LOOPPOINT", 0, &tag);
	if (e != FMOD_ERR_TAGNOTFOUND)
	{
		FMR(e);
		loopstart = atoi((char *)tag.data); // assumed to be a string data tag.
		FMR(FMOD_Sound_GetLoopPoints(music_stream, NULL, FMOD_TIMEUNIT_PCM, &loopend, FMOD_TIMEUNIT_PCM));
		if (loopstart > 0)
			FMR(FMOD_Sound_SetLoopPoints(music_stream, loopstart, FMOD_TIMEUNIT_PCM, loopend, FMOD_TIMEUNIT_PCM));
		return true;
	}

	// Use LOOPMS for time in miliseconds.
	e = FMOD_Sound_GetTag(music_stream, "LOOPMS", 0, &tag);
	if (e != FMOD_ERR_TAGNOTFOUND)
	{
		FMR(e);
		loopstart = atoi((char *)tag.data); // assumed to be a string data tag.
		FMR(FMOD_Sound_GetLoopPoints(music_stream, NULL, FMOD_TIMEUNIT_MS, &loopend, FMOD_TIMEUNIT_PCM));
		if (loopstart > 0)
			FMR(FMOD_Sound_SetLoopPoints(music_stream, loopstart, FMOD_TIMEUNIT_MS, loopend, FMOD_TIMEUNIT_PCM));
		return true;
	}

	// Try to fetch it from the COMMENT tag, like A.J. Freda
	e = FMOD_Sound_GetTag(music_stream, "COMMENT", 0, &tag);
	if (e != FMOD_ERR_TAGNOTFOUND)
	{
		char *loopText;
		// Handle any errors that arose, first
		FMR(e);
		// Figure out where the number starts
		loopText = strstr((char *)tag.data,"LOOPPOINT=");
		if (loopText != NULL)
		{
			// Skip the "LOOPPOINT=" part.
			loopText += 10;
			// Convert it to our looppoint
			// FMOD seems to ensure the tag is properly NULL-terminated.
			// atoi will stop when it reaches anything that's not a number.
			loopstart = atoi(loopText);
			// Now do the rest like above
			FMR(FMOD_Sound_GetLoopPoints(music_stream, NULL, FMOD_TIMEUNIT_PCM, &loopend, FMOD_TIMEUNIT_PCM));
			if (loopstart > 0)
				FMR(FMOD_Sound_SetLoopPoints(music_stream, loopstart, FMOD_TIMEUNIT_PCM, loopend, FMOD_TIMEUNIT_PCM));
		}
		return true;
	}

	// No special loop point
	return true;
}

void I_UnloadSong(void)
{
	I_StopSong();
#ifdef HAVE_LIBGME
	if (gme)
	{
		gme_delete(gme);
		gme = NULL;
	}
#endif
	if (music_stream)
	{
		FMR(FMOD_Sound_Release(music_stream));
		music_stream = NULL;
	}
}

boolean I_PlaySong(boolean looping)
{
#ifdef HAVE_LIBGME
	if (gme)
	{
		gme_start_track(gme, 0);
		current_track = 0;
		FMR(FMOD_System_PlaySound(fsys, FMOD_CHANNEL_FREE, music_stream, false, &music_channel));
		FMR(FMOD_Channel_SetVolume(music_channel, music_volume / 31.0));
		FMR(FMOD_Channel_SetPriority(music_channel, 0));
		return true;
	}
#endif

	FMR(FMOD_Sound_SetMode(music_stream, (looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF)));
	FMR(FMOD_System_PlaySound(fsys, FMOD_CHANNEL_FREE, music_stream, false, &music_channel));
	if (I_SongType() != MU_MID)
		FMR(FMOD_Channel_SetVolume(music_channel, midi_volume / 31.0));
	else
		FMR(FMOD_Channel_SetVolume(music_channel, music_volume / 31.0));
	FMR(FMOD_Channel_SetPriority(music_channel, 0));
	current_track = 0;

	return true;
}

void I_StopSong(void)
{
	if (music_channel)
		FMR_MUSIC(FMOD_Channel_Stop(music_channel));
}

void I_PauseSong(void)
{
	if (music_channel)
		FMR_MUSIC(FMOD_Channel_SetPaused(music_channel, true));
}

void I_ResumeSong(void)
{
	if (music_channel)
		FMR_MUSIC(FMOD_Channel_SetPaused(music_channel, false));
}

void I_SetMusicVolume(UINT8 volume)
{
	if (!music_channel)
		return;

	// volume is 0 to 31.
	if (I_SongType() == MU_MID)
		music_volume = 31; // windows bug hack
	else
		music_volume = volume;

	FMR_MUSIC(FMOD_Channel_SetVolume(music_channel, music_volume / 31.0));
}

UINT32 I_GetSongLength(void)
{
	UINT32 length;
	if (I_SongType() == MU_MID)
		return 0;
	FMR_MUSIC(FMOD_Sound_GetLength(music_stream, &length, FMOD_TIMEUNIT_MS));
	return length;
}

boolean I_SetSongLoopPoint(UINT32 looppoint)
{
        (void)looppoint;
        return false;
}

UINT32 I_GetSongLoopPoint(void)
{
	return 0;
}

boolean I_SetSongPosition(UINT32 position)
{
	FMOD_RESULT e;
	if(I_SongType() == MU_MID)
		// Dummy out; this works for some MIDI, but not others.
		// SDL does not support this for any MIDI.
		return false;
	e = FMOD_Channel_SetPosition(music_channel, position, FMOD_TIMEUNIT_MS);
	if (e == FMOD_OK)
		return true;
	else if (e == FMOD_ERR_UNSUPPORTED // Only music modules, numbnuts!
			|| e == FMOD_ERR_INVALID_POSITION) // Out-of-bounds!
		return false;
	else // Congrats, you horribly broke it somehow
	{
		FMR_MUSIC(e);
		return false;
	}
}

UINT32 I_GetSongPosition(void)
{
	FMOD_RESULT e;
	unsigned int fmposition = 0;
	if(I_SongType() == MU_MID)
		// Dummy out because unsupported, even though FMOD does this correctly.
		return 0;
	e = FMOD_Channel_GetPosition(music_channel, &fmposition, FMOD_TIMEUNIT_MS);
	if (e == FMOD_OK)
		return (UINT32)fmposition;
	else
		return 0;
}

boolean I_SetSongTrack(INT32 track)
{
	if (track != current_track) // If the track's already playing, then why bother?
	{
		FMOD_RESULT e;

		#ifdef HAVE_LIBGME
		// If the specified track is within the number of tracks playing, then change it
		if (gme)
		{
			if (track >= 0
				&& track < gme_track_count(gme))
			{
				gme_err_t gme_e = gme_start_track(gme,track);
				if (gme_e == NULL)
				{
					current_track = track;
					return true;
				}
				else
					CONS_Alert(CONS_ERROR, "Encountered GME error: %s\n", gme_e);
			}
			return false;
		}
		#endif // HAVE_LIBGME

		// Try to set it via FMOD
		e = FMOD_Channel_SetPosition(music_channel, (UINT32)track, FMOD_TIMEUNIT_MODORDER);
		if (e == FMOD_OK) // We good
		{
			current_track = track;
			return true;
		}
		else if (e == FMOD_ERR_UNSUPPORTED // Only music modules, numbnuts!
				|| e == FMOD_ERR_INVALID_POSITION) // Out-of-bounds!
			return false;
		else // Congrats, you horribly broke it somehow
		{
			FMR_MUSIC(e);
			return false;
		}
	}
	return false;
}

/// ------------------------
/// MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(UINT8 volume)
{
	(void)volume;
}

void I_StopFadingSong(void)
{
}

boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void))
{
	(void)target_volume;
	(void)source_volume;
	(void)ms;
	(void)callback;
	return false;
}

boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void))
{
	(void)target_volume;
	(void)ms;
	(void)callback;
	return false;
}

boolean I_FadeOutStopSong(UINT32 ms)
{
	(void)ms;
	return false;
}

boolean I_FadeInPlaySong(UINT32 ms, boolean looping)
{
        (void)ms;
        (void)looping;
        return false;
}
