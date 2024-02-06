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

/* This file supports libOPNMIDI music streams */

#include "music_midi_opn.h"

#ifdef MUSIC_MID_OPNMIDI

#include "SDL_loadso.h"
#include "utils.h"

#include <opnmidi.h>
#include "OPNMIDI/gm_opn_bank.h"

typedef struct {
    int loaded;
    void *handle;

    struct OPN2_MIDIPlayer *(*opn2_init)(long sample_rate);
    void (*opn2_close)(struct OPN2_MIDIPlayer *device);
    int (*opn2_openBankFile)(struct OPN2_MIDIPlayer *device, const char *filePath);
    int (*opn2_openBankData)(struct OPN2_MIDIPlayer *device, const void *mem, long size);
    const char *(*opn2_errorInfo)(struct OPN2_MIDIPlayer *device);
    int (*opn2_switchEmulator)(struct OPN2_MIDIPlayer *device, int emulator);
    void (*opn2_setScaleModulators)(struct OPN2_MIDIPlayer *device, int smod);
    void (*opn2_setVolumeRangeModel)(struct OPN2_MIDIPlayer *device, int volumeModel);
    void (*opn2_setChannelAllocMode)(struct OPN2_MIDIPlayer *device, int chanalloc);
    void (*opn2_setFullRangeBrightness)(struct OPN2_MIDIPlayer *device, int fr_brightness);
    void (*opn2_setAutoArpeggio)(struct OPN2_MIDIPlayer *device, int aaEn);
    void (*opn2_setSoftPanEnabled)(struct OPN2_MIDIPlayer *device, int softPanEn);
    int (*opn2_setNumChips)(struct OPN2_MIDIPlayer *device, int numChips);
    void (*opn2_setTempo)(struct OPN2_MIDIPlayer *device, double tempo);
    size_t (*opn2_trackCount)(struct OPN2_MIDIPlayer *device);
    int (*opn2_setTrackOptions)(struct OPN2_MIDIPlayer *device, size_t trackNumber, unsigned trackOptions);
    int (*opn2_setChannelEnabled)(struct OPN2_MIDIPlayer *device, size_t channelNumber, int enabled);
    int (*opn2_openData)(struct OPN2_MIDIPlayer *device, const void *mem, unsigned long size);
    void (*opn2_selectSongNum)(struct OPN2_MIDIPlayer *device, int songNumber);
    int (*opn2_getSongsCount)(struct OPN2_MIDIPlayer *device);
    const char *(*opn2_metaMusicTitle)(struct OPN2_MIDIPlayer *device);
    const char *(*opn2_metaMusicCopyright)(struct OPN2_MIDIPlayer *device);
    void (*opn2_positionRewind)(struct OPN2_MIDIPlayer *device);
    void (*opn2_setLoopEnabled)(struct OPN2_MIDIPlayer *device, int loopEn);
    void (*opn2_setLoopCount)(struct OPN2_MIDIPlayer *device, int loopCount);
    int  (*opn2_playFormat)(struct OPN2_MIDIPlayer *device, int sampleCount,
                           OPN2_UInt8 *left, OPN2_UInt8 *right,
                           const struct OPNMIDI_AudioFormat *format);
    void (*opn2_positionSeek)(struct OPN2_MIDIPlayer *device, double seconds);
    double (*opn2_positionTell)(struct OPN2_MIDIPlayer *device);
    double (*opn2_totalTimeLength)(struct OPN2_MIDIPlayer *device);
    double (*opn2_loopStartTime)(struct OPN2_MIDIPlayer *device);
    double (*opn2_loopEndTime)(struct OPN2_MIDIPlayer *device);
} opnmidi_loader;

static opnmidi_loader OPNMIDI;

#ifdef OPNMIDI_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    OPNMIDI.FUNC = (SIG) SDL_LoadFunction(OPNMIDI.handle, #FUNC); \
    if (OPNMIDI.FUNC == NULL) { SDL_UnloadObject(OPNMIDI.handle); return -1; }
#define FUNCTION_LOADER_OPTIONAL(FUNC, SIG) \
    OPNMIDI.FUNC = (SIG) SDL_LoadFunction(OPNMIDI.handle, #FUNC);
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    OPNMIDI.FUNC = FUNC; \
    if (OPNMIDI.FUNC == NULL) { Mix_SetError("Missing OPNMIDI.framework"); return -1; }
#define FUNCTION_LOADER_OPTIONAL(FUNC, SIG) \
    OPNMIDI.FUNC = FUNC;
#endif

static int OPNMIDI_Load(void)
{
    if (OPNMIDI.loaded == 0) {
#ifdef OPNMIDI_DYNAMIC
        OPNMIDI.handle = SDL_LoadObject(OPNMIDI_DYNAMIC);
        if (OPNMIDI.handle == NULL) {
            return -1;
        }
#endif
        FUNCTION_LOADER(opn2_init, struct OPN2_MIDIPlayer *(*)(long))
        FUNCTION_LOADER(opn2_close, void(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_openBankFile, int(*)(struct OPN2_MIDIPlayer*,const char*))
        FUNCTION_LOADER(opn2_openBankData, int(*)(struct OPN2_MIDIPlayer*,const void*,long))
        FUNCTION_LOADER(opn2_errorInfo, const char *(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_switchEmulator, int(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_setScaleModulators, void(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_setVolumeRangeModel, void(*)(struct OPN2_MIDIPlayer*,int))
#if defined(OPNMIDI_HAS_CHANNEL_ALLOC_MODE)
        FUNCTION_LOADER_OPTIONAL(opn2_setChannelAllocMode, void(*)(struct OPN2_MIDIPlayer*,int))
#else
        OPNMIDI.opn2_setChannelAllocMode = NULL;
#endif
        FUNCTION_LOADER(opn2_setFullRangeBrightness, void(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_setAutoArpeggio, void(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_setSoftPanEnabled, void(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_setNumChips, int(*)(struct OPN2_MIDIPlayer *, int))
        FUNCTION_LOADER(opn2_setTempo, void(*)(struct OPN2_MIDIPlayer*,double))
        FUNCTION_LOADER(opn2_trackCount, size_t(*)(struct OPN2_MIDIPlayer *))
        FUNCTION_LOADER(opn2_setTrackOptions, int(*)(struct OPN2_MIDIPlayer *, size_t, unsigned))
        FUNCTION_LOADER(opn2_setChannelEnabled, int(*)(struct OPN2_MIDIPlayer *, size_t, int))
        FUNCTION_LOADER(opn2_openData, int(*)(struct OPN2_MIDIPlayer *, const void *, unsigned long))
#if defined(OPNMIDI_HAS_SELECT_SONG_NUM)
        FUNCTION_LOADER_OPTIONAL(opn2_selectSongNum, void(*)(struct OPN2_MIDIPlayer *device, int songNumber))
#else
        OPNMIDI.opn2_selectSongNum = NULL;
#endif
#if defined(OPNMIDI_HAS_GET_SONGS_COUNT)
        FUNCTION_LOADER(opn2_getSongsCount, int(*)(struct OPN2_MIDIPlayer *device))
#else
        OPNMIDI.opn2_getSongsCount = NULL;
#endif
        FUNCTION_LOADER(opn2_metaMusicTitle, const char*(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_metaMusicCopyright, const char*(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_positionRewind, void (*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_setLoopEnabled, void(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_setLoopCount, void(*)(struct OPN2_MIDIPlayer*,int))
        FUNCTION_LOADER(opn2_playFormat, int(*)(struct OPN2_MIDIPlayer *,int,
                               OPN2_UInt8*,OPN2_UInt8*,const struct OPNMIDI_AudioFormat*))
        FUNCTION_LOADER(opn2_positionSeek, void(*)(struct OPN2_MIDIPlayer*,double))
        FUNCTION_LOADER(opn2_positionTell, double(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_totalTimeLength, double(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_loopStartTime, double(*)(struct OPN2_MIDIPlayer*))
        FUNCTION_LOADER(opn2_loopEndTime, double(*)(struct OPN2_MIDIPlayer*))
    }
    ++OPNMIDI.loaded;

    return 0;
}

static void OPNMIDI_Unload(void)
{
    if (OPNMIDI.loaded == 0) {
        return;
    }
    if (OPNMIDI.loaded == 1) {
#ifdef OPNMIDI_DYNAMIC
        SDL_UnloadObject(OPNMIDI.handle);
#endif
    }
    --OPNMIDI.loaded;
}

/* Global OPNMIDI flags which are applying on initializing of MIDI player with a file */
typedef struct {
    int volume_model;
    int alloc_mode;
    int chips_count;
    int full_brightness_range;
    int auto_arpeggio;
    int soft_pan;
    int emulator;
    char custom_bank_path[2048];
    double tempo;
    float gain;
} OpnMidi_Setup;

#define OPNMIDI_DEFAULT_CHIPS_COUNT     6

static OpnMidi_Setup opnmidi_setup = {
    OPNMIDI_VolumeModel_AUTO,
    OPNMIDI_ChanAlloc_AUTO,
    -1, 0, 0, 1, -1, "", 1.0, 2.0
};

static void OPNMIDI_SetDefaultMin(OpnMidi_Setup *setup)
{
    setup->volume_model = OPNMIDI_VolumeModel_AUTO;
    setup->alloc_mode = OPNMIDI_ChanAlloc_AUTO;
    setup->full_brightness_range = 0;
    setup->auto_arpeggio = 0;
    setup->soft_pan = 1;
    setup->tempo = 1.0;
    setup->gain = 2.0f;
}

static void OPNMIDI_SetDefault(OpnMidi_Setup *setup)
{
    OPNMIDI_SetDefaultMin(setup);
    setup->chips_count = -1;
    setup->emulator = -1;
    setup->custom_bank_path[0] = '\0';
}

int _Mix_OPNMIDI_getVolumeModel(void)
{
    return opnmidi_setup.volume_model;
}

void _Mix_OPNMIDI_setVolumeModel(int vm)
{
    opnmidi_setup.volume_model = vm;
    if (vm < 0) {
        opnmidi_setup.volume_model = 0;
    }
}

int _Mix_OPNMIDI_getFullRangeBrightness(void)
{
    return opnmidi_setup.full_brightness_range;
}

void _Mix_OPNMIDI_setFullRangeBrightness(int frb)
{
    opnmidi_setup.full_brightness_range = frb;
}

int _Mix_OPNMIDI_getAutoArpeggio()
{
    return opnmidi_setup.auto_arpeggio;
}

void _Mix_OPNMIDI_setAutoArpeggio(int aa_en)
{
    opnmidi_setup.auto_arpeggio = aa_en;
}

int _Mix_OPNMIDI_getChannelAllocMode()
{
    return opnmidi_setup.alloc_mode;
}

void _Mix_OPNMIDI_setChannelAllocMode(int ch_alloc)
{
    opnmidi_setup.alloc_mode = ch_alloc;
}

int _Mix_OPNMIDI_getFullPanStereo(void)
{
    return opnmidi_setup.soft_pan;
}

void _Mix_OPNMIDI_setFullPanStereo(int fp)
{
    opnmidi_setup.soft_pan = fp;
}

int _Mix_OPNMIDI_getEmulator(void)
{
    return opnmidi_setup.emulator;
}

void _Mix_OPNMIDI_setEmulator(int emu)
{
    opnmidi_setup.emulator = emu;
}

int _Mix_OPNMIDI_getChipsCount(void)
{
    return opnmidi_setup.chips_count;
}

void _Mix_OPNMIDI_setChipsCount(int chips)
{
    opnmidi_setup.chips_count = chips;
}

void _Mix_OPNMIDI_setSetDefaults(void)
{
    OPNMIDI_SetDefault(&opnmidi_setup);
}

void _Mix_OPNMIDI_setCustomBankFile(const char *bank_wonp_path)
{
    if (bank_wonp_path) {
        SDL_strlcpy(opnmidi_setup.custom_bank_path, bank_wonp_path, 2048);
    } else {
        opnmidi_setup.custom_bank_path[0] = '\0';
    }
}


/* This structure supports OPNMIDI-based MIDI music streams */
typedef struct
{
    int play_count;
    struct OPN2_MIDIPlayer *opnmidi;
    int playing;
    int volume;
    double tempo;
    float gain;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    size_t buffer_samples;
    Mix_MusicMetaTags tags;
    struct OPNMIDI_AudioFormat sample_format;
} OpnMIDI_Music;


/* Set the volume for a OPNMIDI stream */
static void OPNMIDI_setvolume(void *music_p, int volume)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    float v = SDL_floorf(((float)(volume) * music->gain) + 0.5f);
    music->volume = (int)v;
}

/* Get the volume for a OPNMIDI stream */
static int OPNMIDI_getvolume(void *music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    float v = SDL_floor(((float)(music->volume) / music->gain) + 0.5f);
    return (int)v;
}

static void process_args(const char *args, OpnMidi_Setup *setup)
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
    OPNMIDI_SetDefaultMin(setup);

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
                case 'c':
                    setup->chips_count = value;
                    break;
                case 'j':
                    setup->auto_arpeggio = value;
                    break;
                case 'o':
                    setup->alloc_mode = value;
                    break;
                case 'v':
                    setup->volume_model = value;
                    break;
                case 'l':
                    setup->volume_model = value;
                    break;
                case 'r':
                    setup->full_brightness_range = value;
                    break;
                case 'p':
                    setup->soft_pan = value;
                    break;
                case 'e':
                    setup->emulator = value;
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
                case 'x':
                    if (arg[0] == '=') {
                        SDL_strlcpy(setup->custom_bank_path, arg + 1, (ARG_BUFFER_SIZE - 1));
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

static void OPNMIDI_delete(void *music_p);

static OpnMIDI_Music *OPNMIDI_LoadSongRW(SDL_RWops *src, const char *args)
{
    void *bytes = 0;
    int err = 0;
    size_t length = 0;
    OpnMIDI_Music *music = NULL;
    OpnMidi_Setup setup = opnmidi_setup;
    unsigned short src_format = music_spec.format;

    if (src == NULL) {
        return NULL;
    }

    process_args(args, &setup);

    music = (OpnMIDI_Music *)SDL_calloc(1, sizeof(OpnMIDI_Music));

    music->tempo = setup.tempo;
    music->gain = setup.gain;
    music->volume = MIX_MAX_VOLUME;

    switch (music_spec.format) {
    case AUDIO_U8:
        music->sample_format.type = OPNMIDI_SampleType_U8;
        music->sample_format.containerSize = sizeof(Uint8);
        music->sample_format.sampleOffset = sizeof(Uint8) * 2;
        break;
    case AUDIO_S8:
        music->sample_format.type = OPNMIDI_SampleType_S8;
        music->sample_format.containerSize = sizeof(Sint8);
        music->sample_format.sampleOffset = sizeof(Sint8) * 2;
        break;
    case AUDIO_S16LSB:
    case AUDIO_S16MSB:
        music->sample_format.type = OPNMIDI_SampleType_S16;
        music->sample_format.containerSize = sizeof(Sint16);
        music->sample_format.sampleOffset = sizeof(Sint16) * 2;
        src_format = AUDIO_S16SYS;
        break;
    case AUDIO_U16LSB:
    case AUDIO_U16MSB:
        music->sample_format.type = OPNMIDI_SampleType_U16;
        music->sample_format.containerSize = sizeof(Uint16);
        music->sample_format.sampleOffset = sizeof(Uint16) * 2;
        src_format = AUDIO_U16SYS;
        break;
    case AUDIO_S32LSB:
    case AUDIO_S32MSB:
        music->sample_format.type = OPNMIDI_SampleType_S32;
        music->sample_format.containerSize = sizeof(Sint32);
        music->sample_format.sampleOffset = sizeof(Sint32) * 2;
        src_format = AUDIO_S32SYS;
        break;
    case AUDIO_F32LSB:
    case AUDIO_F32MSB:
    default:
        music->sample_format.type = OPNMIDI_SampleType_F32;
        music->sample_format.containerSize = sizeof(float);
        music->sample_format.sampleOffset = sizeof(float) * 2;
        src_format = AUDIO_F32SYS;
    }

    music->stream = SDL_NewAudioStream(src_format, 2, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);

    if (!music->stream) {
        OPNMIDI_delete(music);
        return NULL;
    }

    music->buffer_samples = music_spec.samples * 2 /*channels*/;
    music->buffer_size = music->buffer_samples * music->sample_format.containerSize;
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        OPNMIDI_delete(music);
        return NULL;
    }

    bytes = SDL_LoadFile_RW(src, &length, SDL_FALSE);
    if (!bytes) {
        SDL_OutOfMemory();
        OPNMIDI_delete(music);
        return NULL;
    }

    music->opnmidi = OPNMIDI.opn2_init(music_spec.freq);
    if (!music->opnmidi) {
        SDL_free(bytes);
        SDL_OutOfMemory();
        OPNMIDI_delete(music);
        return NULL;
    }

    if (setup.custom_bank_path[0] != '\0') {
        err = OPNMIDI.opn2_openBankFile(music->opnmidi, (char*)setup.custom_bank_path);
    } else {
        err = OPNMIDI.opn2_openBankData(music->opnmidi, g_gm_opn2_bank, sizeof(g_gm_opn2_bank));
    }

    if ( err < 0 ) {
        Mix_SetError("OPN2-MIDI: %s", OPNMIDI.opn2_errorInfo(music->opnmidi));
        SDL_free(bytes);
        OPNMIDI_delete(music);
        return NULL;
    }

    if (setup.emulator >= 0) {
        if(setup.emulator >= OPNMIDI_VGM_DUMPER) {
            setup.emulator++; /* Always skip the VGM Dumper */
        }
        OPNMIDI.opn2_switchEmulator(music->opnmidi, setup.emulator);
    }
    OPNMIDI.opn2_setVolumeRangeModel(music->opnmidi, setup.volume_model);
    OPNMIDI.opn2_setFullRangeBrightness(music->opnmidi, setup.full_brightness_range);
    OPNMIDI.opn2_setSoftPanEnabled(music->opnmidi, setup.soft_pan);
    OPNMIDI.opn2_setAutoArpeggio(music->opnmidi, setup.auto_arpeggio);
    if (OPNMIDI.opn2_setChannelAllocMode) {
        OPNMIDI.opn2_setChannelAllocMode(music->opnmidi, setup.alloc_mode);
    }
    OPNMIDI.opn2_setNumChips(music->opnmidi, (setup.chips_count >= 0) ? setup.chips_count : OPNMIDI_DEFAULT_CHIPS_COUNT);
    OPNMIDI.opn2_setTempo(music->opnmidi, music->tempo);

    err = OPNMIDI.opn2_openData( music->opnmidi, bytes, (unsigned long)length);
    SDL_free(bytes);

    if (err != 0) {
        Mix_SetError("OPN2-MIDI: %s", OPNMIDI.opn2_errorInfo(music->opnmidi));
        OPNMIDI_delete(music);
        return NULL;
    }

    meta_tags_init(&music->tags);
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_TITLE, OPNMIDI.opn2_metaMusicTitle(music->opnmidi));
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_COPYRIGHT, OPNMIDI.opn2_metaMusicCopyright(music->opnmidi));
    return music;
}

/* Load OPNMIDI stream from an SDL_RWops object */
static void *OPNMIDI_new_RWex(struct SDL_RWops *src, int freesrc, const char *args)
{
    OpnMIDI_Music *adlmidiMusic;

    adlmidiMusic = OPNMIDI_LoadSongRW(src, args);
    if (adlmidiMusic && freesrc) {
        SDL_RWclose(src);
    }
    return adlmidiMusic;
}

static void *OPNMIDI_new_RW(struct SDL_RWops *src, int freesrc)
{
    return OPNMIDI_new_RWex(src, freesrc, NULL);
}

/* Start playback of a given Game Music Emulators stream */
static int OPNMIDI_play(void *music_p, int play_counts)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    OPNMIDI.opn2_setLoopEnabled(music->opnmidi, 1);
    OPNMIDI.opn2_setLoopCount(music->opnmidi, play_counts);
    OPNMIDI.opn2_positionRewind(music->opnmidi);
    music->play_count = play_counts;
    return 0;
}

static int OPNMIDI_playSome(void *context, void *data, int bytes, SDL_bool *done)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)context;
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

    gottenLen = OPNMIDI.opn2_playFormat(music->opnmidi,
                                       music->buffer_samples,
                                       (OPN2_UInt8*)music->buffer,
                                       (OPN2_UInt8*)music->buffer + music->sample_format.containerSize,
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
            OPNMIDI.opn2_positionRewind(music->opnmidi);
            music->play_count = play_count;
        }
    }

    return 0;
}

/* Play some of a stream previously started with OPNMIDI_play() */
static int OPNMIDI_playAudio(void *music_p, void *stream, int len)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    return music_pcm_getaudio(music_p, stream, len, music->volume, OPNMIDI_playSome);
}

/* Close the given Game Music Emulators stream */
static void OPNMIDI_delete(void *music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    if (music) {
        meta_tags_clear(&music->tags);
        if (music->opnmidi) {
            OPNMIDI.opn2_close(music->opnmidi);
        }
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        SDL_free( music );
    }
}

static const char* OPNMIDI_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int OPNMIDI_Seek(void *music_p, double time)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    OPNMIDI.opn2_positionSeek(music->opnmidi, time);
    return 0;
}

static double OPNMIDI_Tell(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return OPNMIDI.opn2_positionTell(music->opnmidi);
}


static double OPNMIDI_Duration(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return OPNMIDI.opn2_totalTimeLength(music->opnmidi);
}

static int OPNMIDI_StartTrack(void *music_p, int track)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if (music && OPNMIDI.opn2_selectSongNum) {
        OPNMIDI.opn2_selectSongNum(music->opnmidi, track);
        return 0;
    }
    return -1;
}

static int OPNMIDI_GetNumTracks(void *music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if (OPNMIDI.opn2_getSongsCount) {
        return OPNMIDI.opn2_getSongsCount(music->opnmidi);
    } else {
        return -1;
    }
}

static int OPNMIDI_SetTempo(void *music_p, double tempo)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if (music && (tempo > 0.0)) {
        OPNMIDI.opn2_setTempo(music->opnmidi, tempo);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double OPNMIDI_GetTempo(void *music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int OPNMIDI_GetTracksCount(void *music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if (music) {
        return 16;
    }
    return -1;
}

static int OPNMIDI_SetTrackMute(void *music_p, int track, int mute)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    int ret = -1;
    if (music) {
        ret = OPNMIDI.opn2_setChannelEnabled(music->opnmidi, track, mute ? 0 : 1);
        if (ret < 0) {
            Mix_SetError("OPNMIDI: %s", OPNMIDI.opn2_errorInfo(music->opnmidi));
        }
    }
    return ret;
}

static double OPNMIDI_LoopStart(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return OPNMIDI.opn2_loopStartTime(music->opnmidi);
}

static double OPNMIDI_LoopEnd(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return OPNMIDI.opn2_loopEndTime(music->opnmidi);
}

static double OPNMIDI_LoopLength(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if (music) {
        double start = OPNMIDI.opn2_loopStartTime(music->opnmidi);
        double end = OPNMIDI.opn2_loopEndTime(music->opnmidi);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}


Mix_MusicInterface Mix_MusicInterface_OPNMIDI =
{
    "OPNMIDI",
    MIX_MUSIC_OPNMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    OPNMIDI_Load,
    NULL,   /* Open */
    OPNMIDI_new_RW,
    OPNMIDI_new_RWex,/* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    OPNMIDI_setvolume,
    OPNMIDI_getvolume,   /* GetVolume [MIXER-X]*/
    OPNMIDI_play,
    NULL,   /* IsPlaying */
    OPNMIDI_playAudio,
    NULL,       /* Jump */
    OPNMIDI_Seek,
    OPNMIDI_Tell,   /* Tell [MIXER-X]*/
    OPNMIDI_Duration,
    OPNMIDI_SetTempo,   /* [MIXER-X] */
    OPNMIDI_GetTempo,   /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    OPNMIDI_GetTracksCount,   /* [MIXER-X] */
    OPNMIDI_SetTrackMute,   /* [MIXER-X] */
    OPNMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    OPNMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    OPNMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    OPNMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    OPNMIDI_GetNumTracks,
    OPNMIDI_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    OPNMIDI_delete,
    NULL,   /* Close */
    OPNMIDI_Unload
};

/* Special case to play XMI/MUS formats */
Mix_MusicInterface Mix_MusicInterface_OPNXMI =
{
    "OPNXMI",
    MIX_MUSIC_OPNMIDI,
    MUS_OPNMIDI,
    SDL_FALSE,
    SDL_FALSE,

    OPNMIDI_Load,
    NULL,   /* Open */
    OPNMIDI_new_RW,
    OPNMIDI_new_RWex,/* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    OPNMIDI_setvolume,
    OPNMIDI_getvolume,   /* GetVolume [MIXER-X]*/
    OPNMIDI_play,
    NULL,   /* IsPlaying */
    OPNMIDI_playAudio,
    NULL,       /* Jump */
    OPNMIDI_Seek,
    OPNMIDI_Tell,   /* Tell [MIXER-X]*/
    OPNMIDI_Duration,
    OPNMIDI_SetTempo,   /* [MIXER-X] */
    OPNMIDI_GetTempo,   /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    OPNMIDI_GetTracksCount,   /* [MIXER-X] */
    OPNMIDI_SetTrackMute,   /* [MIXER-X] */
    OPNMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    OPNMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    OPNMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    OPNMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    OPNMIDI_GetNumTracks,
    OPNMIDI_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    OPNMIDI_delete,
    NULL,   /* Close */
    OPNMIDI_Unload
};

#endif /* MUSIC_MID_OPNMIDI */

/* vi: set ts=4 sw=4 expandtab: */
