/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* This file supports libEDMIDI music streams */

#include "music_midi_edmidi.h"

#ifdef MUSIC_MID_EDMIDI

#include "SDL_loadso.h"
#include "utils.h"

#include <emu_de_midi.h>

typedef struct {
    int loaded;
    void *handle;

    struct EDMIDIPlayer *(*edmidi_init)(long sample_rate);
    struct EDMIDIPlayer *(*edmidi_initEx)(long sample_rate, int mods);
    void (*edmidi_close)(struct EDMIDIPlayer *device);
    const char *(*edmidi_errorInfo)(struct EDMIDIPlayer *device);
    void (*edmidi_setTempo)(struct EDMIDIPlayer *device, double tempo);
    size_t (*edmidi_trackCount)(struct EDMIDIPlayer *device);
    int (*edmidi_setTrackEnabled)(struct EDMIDIPlayer *device, size_t trackNumber, int enabled);
    int (*edmidi_setChannelEnabled)(struct EDMIDIPlayer *device, size_t channelNumber, int enabled);
    int (*edmidi_openData)(struct EDMIDIPlayer *device, const void *mem, unsigned long size);
    void (*edmidi_selectSongNum)(struct EDMIDIPlayer *device, int songNumber);
    int (*edmidi_getSongsCount)(struct EDMIDIPlayer *device);
    const char *(*edmidi_metaMusicTitle)(struct EDMIDIPlayer *device);
    const char *(*edmidi_metaMusicCopyright)(struct EDMIDIPlayer *device);
    void (*edmidi_positionRewind)(struct EDMIDIPlayer *device);
    void (*edmidi_setLoopEnabled)(struct EDMIDIPlayer *device, int loopEn);
    void (*edmidi_setLoopCount)(struct EDMIDIPlayer *device, int loopCount);
    int  (*edmidi_playFormat)(struct EDMIDIPlayer *device, int sampleCount,
                           EDMIDI_UInt8 *left, EDMIDI_UInt8 *right,
                           const struct EDMIDI_AudioFormat *format);
    void (*edmidi_positionSeek)(struct EDMIDIPlayer *device, double seconds);
    double (*edmidi_positionTell)(struct EDMIDIPlayer *device);
    double (*edmidi_totalTimeLength)(struct EDMIDIPlayer *device);
    double (*edmidi_loopStartTime)(struct EDMIDIPlayer *device);
    double (*edmidi_loopEndTime)(struct EDMIDIPlayer *device);
} edmidi_loader;

static edmidi_loader EDMIDI;

#ifdef EDMIDI_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    EDMIDI.FUNC = (SIG) SDL_LoadFunction(EDMIDI.handle, #FUNC); \
    if (EDMIDI.FUNC == NULL) { SDL_UnloadObject(EDMIDI.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    EDMIDI.FUNC = FUNC; \
    if (EDMIDI.FUNC == NULL) { Mix_SetError("Missing EDMIDI.framework"); return -1; }
#endif

static int EDMIDI_Load(void)
{
    if (EDMIDI.loaded == 0) {
#ifdef EDMIDI_DYNAMIC
        EDMIDI.handle = SDL_LoadObject(EDMIDI_DYNAMIC);
        if (EDMIDI.handle == NULL) {
            return -1;
        }
#endif
        FUNCTION_LOADER(edmidi_init, struct EDMIDIPlayer *(*)(long))
        FUNCTION_LOADER(edmidi_initEx, struct EDMIDIPlayer *(*)(long, int))
        FUNCTION_LOADER(edmidi_close, void(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_errorInfo, const char *(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_setTempo, void(*)(struct EDMIDIPlayer*,double))
        FUNCTION_LOADER(edmidi_trackCount, size_t(*)(struct EDMIDIPlayer *))
        FUNCTION_LOADER(edmidi_setTrackEnabled, int(*)(struct EDMIDIPlayer *, size_t, int))
        FUNCTION_LOADER(edmidi_setChannelEnabled, int(*)(struct EDMIDIPlayer *, size_t, int))
        FUNCTION_LOADER(edmidi_openData, int(*)(struct EDMIDIPlayer *, const void *, unsigned long))
        FUNCTION_LOADER(edmidi_selectSongNum, void(*)(struct EDMIDIPlayer *, int))
        FUNCTION_LOADER(edmidi_getSongsCount, int(*)(struct EDMIDIPlayer *))
        FUNCTION_LOADER(edmidi_metaMusicTitle, const char*(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_metaMusicCopyright, const char*(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_positionRewind, void (*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_setLoopEnabled, void(*)(struct EDMIDIPlayer*,int))
        FUNCTION_LOADER(edmidi_setLoopCount, void(*)(struct EDMIDIPlayer*,int))
        FUNCTION_LOADER(edmidi_playFormat, int(*)(struct EDMIDIPlayer *,int,
                               EDMIDI_UInt8*,EDMIDI_UInt8*,const struct EDMIDI_AudioFormat*))
        FUNCTION_LOADER(edmidi_positionSeek, void(*)(struct EDMIDIPlayer*,double))
        FUNCTION_LOADER(edmidi_positionTell, double(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_totalTimeLength, double(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_loopStartTime, double(*)(struct EDMIDIPlayer*))
        FUNCTION_LOADER(edmidi_loopEndTime, double(*)(struct EDMIDIPlayer*))
    }
    ++EDMIDI.loaded;

    return 0;
}

static void EDMIDI_Unload(void)
{
    if (EDMIDI.loaded == 0) {
        return;
    }
    if (EDMIDI.loaded == 1) {
#ifdef EDMIDI_DYNAMIC
        SDL_UnloadObject(EDMIDI.handle);
#endif
    }
    --EDMIDI.loaded;
}


/* Global EDMIDI flags which are applying on initializing of MIDI player with a file */
typedef struct {
    int mods_num;
    double tempo;
    float gain;
} EDMidi_Setup;

#define EDMIDI_DEFAULT_MODS_COUNT     2

static EDMidi_Setup edmidi_setup = {
    EDMIDI_DEFAULT_MODS_COUNT, 1.0, 2.0
};

static void EDMIDI_SetDefault(EDMidi_Setup *setup)
{
    setup->mods_num = EDMIDI_DEFAULT_MODS_COUNT;
    setup->tempo = 1.0;
    setup->gain = 2.0f;
}

void _Mix_EDMIDI_setSetDefaults()
{
    EDMIDI_SetDefault(&edmidi_setup);
}

typedef struct
{
    int play_count;
    struct EDMIDIPlayer *edmidi;
    int volume;
    double tempo;
    float gain;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    size_t buffer_samples;
    Mix_MusicMetaTags tags;
    struct EDMIDI_AudioFormat sample_format;
} EDMIDI_Music;

/* Set the volume for a EDMIDI stream */
static void EDMIDI_setvolume(void *music_p, int volume)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    float v = SDL_floorf(((float)(volume) * music->gain) + 0.5f);
    music->volume = (int)v;
}

/* Get the volume for a EDMIDI stream */
static int EDMIDI_getvolume(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    float v = SDL_floorf(((float)(music->volume) / music->gain) + 0.5f);
    return (int)v;
}

static void process_args(const char *args, EDMidi_Setup *setup)
{
#define ARG_BUFFER_SIZE    1024
    char arg[ARG_BUFFER_SIZE];
    char type = '-';
    size_t maxlen = 0;
    size_t i, j = 0;
    int value_opened = 0;
    if (args == NULL) {
        return;
    }
    maxlen = SDL_strlen(args);
    if (maxlen == 0) {
        return;
    }

    maxlen += 1;
    EDMIDI_SetDefault(setup);

    for (i = 0; i < maxlen; i++) {
        char c = args[i];
        if (value_opened == 1) {
            if ((c == ';') || (c == '\0')) {
                int value = 0;
                arg[j] = '\0';
                if (type != 'x') {
                    value = SDL_atoi(arg);
                }
                switch(type)
                {
                case 'm':
                    setup->mods_num = value;
                    break;
                case 't':
                    if (arg[0] == '=') {
                        setup->tempo = SDL_strtod(arg + 1, NULL);
                        if (setup->tempo <= 0.0) {
                            setup->tempo = 1.0;
                        }
                    }
                    break;
                case 'g':
                    if (arg[0] == '=') {
                        setup->gain = (float)SDL_strtod(arg + 1, NULL);
                        if (setup->gain < 0.0f) {
                            setup->gain = 1.0f;
                        }
                    }
                    break;
                case '\0':
                    break;
                default:
                    break;
                }
                value_opened = 0;
            }
            arg[j++] = c;
        } else {
            if (c == '\0') {
                return;
            }
            type = c;
            value_opened = 1;
            j = 0;
        }
    }
#undef ARG_BUFFER_SIZE
}

static void EDMIDI_delete(void *music_p);

static EDMIDI_Music *EDMIDI_LoadSongRW(SDL_RWops *src, const char *args)
{
    void *bytes = 0;
    int err = 0;
    size_t length = 0;
    EDMIDI_Music *music = NULL;
    EDMidi_Setup setup = edmidi_setup;
    unsigned short src_format = music_spec.format;

    if (src == NULL) {
        return NULL;
    }

    process_args(args, &setup);

    music = (EDMIDI_Music *)SDL_calloc(1, sizeof(EDMIDI_Music));

    music->tempo = setup.tempo;
    music->gain = setup.gain;
    music->volume = MIX_MAX_VOLUME;

    switch (music_spec.format) {
    case AUDIO_U8:
        music->sample_format.type = EDMIDI_SampleType_U8;
        music->sample_format.containerSize = sizeof(Uint8);
        music->sample_format.sampleOffset = sizeof(Uint8) * 2;
        break;
    case AUDIO_S8:
        music->sample_format.type = EDMIDI_SampleType_S8;
        music->sample_format.containerSize = sizeof(Sint8);
        music->sample_format.sampleOffset = sizeof(Sint8) * 2;
        break;
    case AUDIO_S16LSB:
    case AUDIO_S16MSB:
        music->sample_format.type = EDMIDI_SampleType_S16;
        music->sample_format.containerSize = sizeof(Sint16);
        music->sample_format.sampleOffset = sizeof(Sint16) * 2;
        src_format = AUDIO_S16SYS;
        break;
    case AUDIO_U16LSB:
    case AUDIO_U16MSB:
        music->sample_format.type = EDMIDI_SampleType_U16;
        music->sample_format.containerSize = sizeof(Uint16);
        music->sample_format.sampleOffset = sizeof(Uint16) * 2;
        src_format = AUDIO_U16SYS;
        break;
    case AUDIO_S32LSB:
    case AUDIO_S32MSB:
        music->sample_format.type = EDMIDI_SampleType_S32;
        music->sample_format.containerSize = sizeof(Sint32);
        music->sample_format.sampleOffset = sizeof(Sint32) * 2;
        src_format = AUDIO_S32SYS;
        break;
    case AUDIO_F32LSB:
    case AUDIO_F32MSB:
    default:
        music->sample_format.type = EDMIDI_SampleType_F32;
        music->sample_format.containerSize = sizeof(float);
        music->sample_format.sampleOffset = sizeof(float) * 2;
        src_format = AUDIO_F32SYS;
    }

    music->stream = SDL_NewAudioStream(src_format, 2, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);

    if (!music->stream) {
        EDMIDI_delete(music);
        return NULL;
    }

    music->buffer_samples = music_spec.samples * 2 /*channels*/;
    music->buffer_size = music->buffer_samples * music->sample_format.containerSize;
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        EDMIDI_delete(music);
        return NULL;
    }

    bytes = SDL_LoadFile_RW(src, &length, SDL_FALSE);
    if (!bytes) {
        SDL_OutOfMemory();
        EDMIDI_delete(music);
        return NULL;
    }

    music->edmidi = EDMIDI.edmidi_initEx(music_spec.freq, setup.mods_num);
    if (!music->edmidi) {
        SDL_free(bytes);
        SDL_OutOfMemory();
        EDMIDI_delete(music);
        return NULL;
    }

    if (err < 0) {
        Mix_SetError("EDMIDI: %s", EDMIDI.edmidi_errorInfo(music->edmidi));
        SDL_free(bytes);
        EDMIDI_delete(music);
        return NULL;
    }

    EDMIDI.edmidi_setTempo(music->edmidi, music->tempo);

    err = EDMIDI.edmidi_openData(music->edmidi, bytes, (unsigned long)length);
    SDL_free(bytes);

    if (err != 0) {
        Mix_SetError("ADL-MIDI: %s", EDMIDI.edmidi_errorInfo(music->edmidi));
        EDMIDI_delete(music);
        return NULL;
    }

    meta_tags_init(&music->tags);
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_TITLE, EDMIDI.edmidi_metaMusicTitle(music->edmidi));
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_COPYRIGHT, EDMIDI.edmidi_metaMusicCopyright(music->edmidi));
    return music;
}

/* Load EDMIDI stream from an SDL_RWops object */
static void *EDMIDI_new_RWex(struct SDL_RWops *src, int freesrc, const char *args)
{
    EDMIDI_Music *adlmidiMusic;

    adlmidiMusic = EDMIDI_LoadSongRW(src, args);
    if (adlmidiMusic && freesrc) {
        SDL_RWclose(src);
    }
    return adlmidiMusic;
}

static void *EDMIDI_new_RW(struct SDL_RWops *src, int freesrc)
{
    return EDMIDI_new_RWex(src, freesrc, NULL);
}


/* Start playback of a given Game Music Emulators stream */
static int EDMIDI_play(void *music_p, int play_counts)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    EDMIDI.edmidi_setLoopEnabled(music->edmidi, 1);
    EDMIDI.edmidi_setLoopCount(music->edmidi, play_counts);
    EDMIDI.edmidi_positionRewind(music->edmidi);
    music->play_count = play_counts;
    return 0;
}

static int EDMIDI_playSome(void *context, void *data, int bytes, SDL_bool *done)
{
    EDMIDI_Music *music = (EDMIDI_Music *)context;
    int filled, gottenLen, amount;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    gottenLen = EDMIDI.edmidi_playFormat(music->edmidi,
                                         music->buffer_samples,
                                         (EDMIDI_UInt8*)music->buffer,
                                         (EDMIDI_UInt8*)music->buffer + music->sample_format.containerSize,
                                         &music->sample_format);

    if (gottenLen <= 0) {
        *done = SDL_TRUE;
        return 0;
    }

    amount = gottenLen * (int)music->sample_format.containerSize;
    if (amount > 0) {
        if (SDL_AudioStreamPut(music->stream, music->buffer, amount) < 0) {
            return -1;
        }
    } else {
        if (music->play_count == 1) {
            music->play_count = 0;
            SDL_AudioStreamFlush(music->stream);
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            EDMIDI.edmidi_positionRewind(music->edmidi);
            music->play_count = play_count;
        }
    }

    return 0;
}

/* Play some of a stream previously started with EDMIDI_play() */
static int EDMIDI_playAudio(void *music_p, void *stream, int len)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    return music_pcm_getaudio(music_p, stream, len, music->volume, EDMIDI_playSome);
}

/* Close the given Game Music Emulators stream */
static void EDMIDI_delete(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        meta_tags_clear(&music->tags);
        if (music->edmidi) {
            EDMIDI.edmidi_close(music->edmidi);
        }
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        SDL_free(music);
    }
}

static const char* EDMIDI_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    EDMIDI_Music *music = (EDMIDI_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int EDMIDI_Seek(void *music_p, double time)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    EDMIDI.edmidi_positionSeek(music->edmidi, time);
    return 0;
}

static double EDMIDI_Tell(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        return EDMIDI.edmidi_positionTell(music->edmidi);
    }
    return -1;
}

static double EDMIDI_Duration(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        return EDMIDI.edmidi_totalTimeLength(music->edmidi);
    }
    return -1;
}

static int EDMIDI_StartTrack(void *music_p, int track)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music && EDMIDI.edmidi_selectSongNum) {
        EDMIDI.edmidi_selectSongNum(music->edmidi, track);
        return 0;
    }
    return -1;
}

static int EDMIDI_GetNumTracks(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (EDMIDI.edmidi_getSongsCount) {
        return EDMIDI.edmidi_getSongsCount(music->edmidi);
    } else {
        return -1;
    }
}

static int EDMIDI_SetTempo(void *music_p, double tempo)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music && (tempo > 0.0)) {
        EDMIDI.edmidi_setTempo(music->edmidi, tempo);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double EDMIDI_GetTempo(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int EDMIDI_GetTracksCount(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        return 16;
    }
    return -1;
}

static int EDMIDI_SetTrackMute(void *music_p, int track, int mute)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    int ret = -1;
    if (music) {
        ret = EDMIDI.edmidi_setChannelEnabled(music->edmidi, track, mute ? 0 : 1);
        if (ret < 0) {
            Mix_SetError("EDMIDI: %s", EDMIDI.edmidi_errorInfo(music->edmidi));
        }
    }
    return ret;
}

static double EDMIDI_LoopStart(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        return EDMIDI.edmidi_loopStartTime(music->edmidi);
    }
    return -1;
}

static double EDMIDI_LoopEnd(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        return EDMIDI.edmidi_loopEndTime(music->edmidi);
    }
    return -1;
}

static double EDMIDI_LoopLength(void *music_p)
{
    EDMIDI_Music *music = (EDMIDI_Music *)music_p;
    if (music) {
        double start = EDMIDI.edmidi_loopStartTime(music->edmidi);
        double end = EDMIDI.edmidi_loopEndTime(music->edmidi);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}


Mix_MusicInterface Mix_MusicInterface_EDMIDI =
{
    "EDMIDI",
    MIX_MUSIC_EDMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    EDMIDI_Load,
    NULL,   /* Open */
    EDMIDI_new_RW,
    EDMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    EDMIDI_setvolume,
    EDMIDI_getvolume,
    EDMIDI_play,
    NULL,   /* IsPlaying */
    EDMIDI_playAudio,
    NULL,       /* Jump */
    EDMIDI_Seek,
    EDMIDI_Tell,   /* Tell [MIXER-X]*/
    EDMIDI_Duration,
    EDMIDI_SetTempo,   /* [MIXER-X] */
    EDMIDI_GetTempo,   /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    EDMIDI_GetTracksCount,   /* [MIXER-X] */
    EDMIDI_SetTrackMute,   /* [MIXER-X] */
    EDMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    EDMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    EDMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    EDMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    EDMIDI_GetNumTracks,
    EDMIDI_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    EDMIDI_delete,
    NULL,   /* Close */
    EDMIDI_Unload
};

Mix_MusicInterface Mix_MusicInterface_EDXMI =
{
    "EDxmi",
    MIX_MUSIC_EDMIDI,
    MUS_EDMIDI,
    SDL_FALSE,
    SDL_FALSE,

    EDMIDI_Load,
    NULL,   /* Open */
    EDMIDI_new_RW,
    EDMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    EDMIDI_setvolume,
    EDMIDI_getvolume,
    EDMIDI_play,
    NULL,   /* IsPlaying */
    EDMIDI_playAudio,
    NULL,       /* Jump */
    EDMIDI_Seek,
    EDMIDI_Tell,   /* Tell [MIXER-X]*/
    EDMIDI_Duration,
    EDMIDI_SetTempo,   /* [MIXER-X] */
    EDMIDI_GetTempo,   /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    EDMIDI_GetTracksCount,   /* [MIXER-X] */
    EDMIDI_SetTrackMute,   /* [MIXER-X] */
    EDMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    EDMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    EDMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    EDMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    EDMIDI_GetNumTracks,
    EDMIDI_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    EDMIDI_delete,
    NULL,   /* Close */
    EDMIDI_Unload
};

#endif /* MUSIC_MID_EDMIDI */

/* vi: set ts=4 sw=4 expandtab: */
