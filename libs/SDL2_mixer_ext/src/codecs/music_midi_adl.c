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

/* This file supports libADLMIDI music streams */

#include "music_midi_adl.h"

#ifdef MUSIC_MID_ADLMIDI

#include "SDL_loadso.h"
#include "utils.h"

#include <adlmidi.h>

typedef struct {
    int loaded;
    void *handle;

    int (*adl_getBanksCount)(void);
    const char *const *(*adl_getBankNames)(void);
    struct ADL_MIDIPlayer *(*adl_init)(long sample_rate);
    void (*adl_close)(struct ADL_MIDIPlayer *device);
    void (*adl_setHVibrato)(struct ADL_MIDIPlayer *device, int hvibro);
    void (*adl_setHTremolo)(struct ADL_MIDIPlayer *device, int htremo);
    int (*adl_openBankFile)(struct ADL_MIDIPlayer *device, const char *filePath);
    int (*adl_setBank)(struct ADL_MIDIPlayer *device, int bank);
    const char *(*adl_errorInfo)(struct ADL_MIDIPlayer *device);
    int (*adl_switchEmulator)(struct ADL_MIDIPlayer *device, int emulator);
    void (*adl_setScaleModulators)(struct ADL_MIDIPlayer *device, int smod);
    void (*adl_setVolumeRangeModel)(struct ADL_MIDIPlayer *device, int volumeModel);
    void (*adl_setChannelAllocMode)(struct ADL_MIDIPlayer *device, int chanalloc);
    void (*adl_setFullRangeBrightness)(struct ADL_MIDIPlayer *device, int fr_brightness);
    void (*adl_setAutoArpeggio)(struct ADL_MIDIPlayer *device, int aaEn);
    void (*adl_setSoftPanEnabled)(struct ADL_MIDIPlayer *device, int softPanEn);
    int (*adl_setNumChips)(struct ADL_MIDIPlayer *device, int numChips);
    int (*adl_setNumFourOpsChn)(struct ADL_MIDIPlayer *device, int ops4);
    void (*adl_setTempo)(struct ADL_MIDIPlayer *device, double tempo);
    size_t (*adl_trackCount)(struct ADL_MIDIPlayer *device);
    int (*adl_setTrackOptions)(struct ADL_MIDIPlayer *device, size_t trackNumber, unsigned trackOptions);
    int (*adl_setChannelEnabled)(struct ADL_MIDIPlayer *device, size_t channelNumber, int enabled);
    int (*adl_openData)(struct ADL_MIDIPlayer *device, const void *mem, unsigned long size);
    void (*adl_selectSongNum)(struct ADL_MIDIPlayer *device, int songNumber);
    int (*adl_getSongsCount)(struct ADL_MIDIPlayer *device);
    const char *(*adl_metaMusicTitle)(struct ADL_MIDIPlayer *device);
    const char *(*adl_metaMusicCopyright)(struct ADL_MIDIPlayer *device);
    void (*adl_positionRewind)(struct ADL_MIDIPlayer *device);
    void (*adl_setLoopEnabled)(struct ADL_MIDIPlayer *device, int loopEn);
    void (*adl_setLoopCount)(struct ADL_MIDIPlayer *device, int loopCount);
    int  (*adl_playFormat)(struct ADL_MIDIPlayer *device, int sampleCount,
                           ADL_UInt8 *left, ADL_UInt8 *right,
                           const struct ADLMIDI_AudioFormat *format);
    void (*adl_positionSeek)(struct ADL_MIDIPlayer *device, double seconds);
    double (*adl_positionTell)(struct ADL_MIDIPlayer *device);
    double (*adl_totalTimeLength)(struct ADL_MIDIPlayer *device);
    double (*adl_loopStartTime)(struct ADL_MIDIPlayer *device);
    double (*adl_loopEndTime)(struct ADL_MIDIPlayer *device);
} adlmidi_loader;

static adlmidi_loader ADLMIDI;

#ifdef ADLMIDI_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    ADLMIDI.FUNC = (SIG) SDL_LoadFunction(ADLMIDI.handle, #FUNC); \
    if (ADLMIDI.FUNC == NULL) { SDL_UnloadObject(ADLMIDI.handle); return -1; }
#define FUNCTION_LOADER_OPTIONAL(FUNC, SIG) \
    ADLMIDI.FUNC = (SIG) SDL_LoadFunction(ADLMIDI.handle, #FUNC);
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    ADLMIDI.FUNC = FUNC; \
    if (ADLMIDI.FUNC == NULL) { Mix_SetError("Missing ADLMIDI.framework"); return -1; }
#define FUNCTION_LOADER_OPTIONAL(FUNC, SIG) \
    ADLMIDI.FUNC = FUNC;
#endif

static int ADLMIDI_Load(void)
{
    if (ADLMIDI.loaded == 0) {
#ifdef ADLMIDI_DYNAMIC
        ADLMIDI.handle = SDL_LoadObject(ADLMIDI_DYNAMIC);
        if (ADLMIDI.handle == NULL) {
            return -1;
        }
#endif
        FUNCTION_LOADER(adl_getBanksCount, int (*)(void))
        FUNCTION_LOADER(adl_getBankNames, const char *const *(*)(void))
        FUNCTION_LOADER(adl_init, struct ADL_MIDIPlayer *(*)(long))
        FUNCTION_LOADER(adl_close, void(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_setHVibrato, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setHTremolo, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_openBankFile, int(*)(struct ADL_MIDIPlayer*,const char*))
        FUNCTION_LOADER(adl_setBank, int(*)(struct ADL_MIDIPlayer *, int))
        FUNCTION_LOADER(adl_errorInfo, const char *(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_switchEmulator, int(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setScaleModulators, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setVolumeRangeModel, void(*)(struct ADL_MIDIPlayer*,int))
#if defined(ADLMIDI_HAS_CHANNEL_ALLOC_MODE)
        FUNCTION_LOADER_OPTIONAL(adl_setChannelAllocMode, void(*)(struct ADL_MIDIPlayer*,int))
#else
        ADLMIDI.adl_setChannelAllocMode = NULL;
#endif
        FUNCTION_LOADER(adl_setFullRangeBrightness, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setAutoArpeggio, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setSoftPanEnabled, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setNumChips, int(*)(struct ADL_MIDIPlayer *, int))
        FUNCTION_LOADER(adl_setNumFourOpsChn, int(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setTempo, void(*)(struct ADL_MIDIPlayer*,double))
        FUNCTION_LOADER(adl_trackCount, size_t(*)(struct ADL_MIDIPlayer *))
        FUNCTION_LOADER(adl_setTrackOptions, int(*)(struct ADL_MIDIPlayer *, size_t, unsigned))
        FUNCTION_LOADER(adl_setChannelEnabled, int(*)(struct ADL_MIDIPlayer *, size_t, int))
        FUNCTION_LOADER(adl_openData, int(*)(struct ADL_MIDIPlayer *, const void *, unsigned long))
#if defined(ADLMIDI_HAS_SELECT_SONG_NUM)
        FUNCTION_LOADER_OPTIONAL(adl_selectSongNum, void(*)(struct ADL_MIDIPlayer *device, int songNumber))
#else
        ADLMIDI.adl_selectSongNum = NULL;
#endif
#if defined(ADLMIDI_HAS_GET_SONGS_COUNT)
        FUNCTION_LOADER(adl_getSongsCount, int(*)(struct ADL_MIDIPlayer *device))
#else
        ADLMIDI.adl_getSongsCount = NULL;
#endif
        FUNCTION_LOADER(adl_metaMusicTitle, const char*(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_metaMusicCopyright, const char*(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_positionRewind, void (*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_setLoopEnabled, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setLoopCount, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_playFormat, int(*)(struct ADL_MIDIPlayer *,int,
                               ADL_UInt8*,ADL_UInt8*,const struct ADLMIDI_AudioFormat*))
        FUNCTION_LOADER(adl_positionSeek, void(*)(struct ADL_MIDIPlayer*,double))
        FUNCTION_LOADER(adl_positionTell, double(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_totalTimeLength, double(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_loopStartTime, double(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_loopEndTime, double(*)(struct ADL_MIDIPlayer*))
    }
    ++ADLMIDI.loaded;

    return 0;
}

static void ADLMIDI_Unload(void)
{
    if (ADLMIDI.loaded == 0) {
        return;
    }
    if (ADLMIDI.loaded == 1) {
#ifdef ADLMIDI_DYNAMIC
        SDL_UnloadObject(ADLMIDI.handle);
#endif
    }
    --ADLMIDI.loaded;
}


/* Global ADLMIDI flags which are applying on initializing of MIDI player with a file */
typedef struct {
    int bank;
    int tremolo;
    int vibrato;
    int scalemod;
    int volume_model;
    int alloc_mode;
    int chips_count;
    int four_op_channels;
    int full_brightness_range;
    int auto_arpeggio;
    int soft_pan;
    int emulator;
    char custom_bank_path[2048];
    double tempo;
    float gain;
} AdlMidi_Setup;

#define ADLMIDI_DEFAULT_CHIPS_COUNT     4

static AdlMidi_Setup adlmidi_setup = {
    58,
    -1, -1, -1,
    ADLMIDI_VolumeModel_AUTO,
    ADLMIDI_ChanAlloc_AUTO,
    -1, -1,
    0, 0, 1,
    ADLMIDI_EMU_DOSBOX, "",
    1.0, 2.0
};

static void ADLMIDI_SetDefaultMin(AdlMidi_Setup *setup)
{
    setup->tremolo     = -1;
    setup->vibrato     = -1;
    setup->scalemod    = -1;
    setup->volume_model = ADLMIDI_VolumeModel_AUTO;
    setup->alloc_mode = ADLMIDI_ChanAlloc_AUTO;
    setup->four_op_channels = -1;
    setup->full_brightness_range = 0;
    setup->auto_arpeggio = 0;
    setup->soft_pan = 1;
    setup->tempo = 1.0;
    setup->gain = 2.0f;
}

static void ADLMIDI_SetDefault(AdlMidi_Setup *setup)
{
    ADLMIDI_SetDefaultMin(setup);
    setup->bank        = 58;
    setup->custom_bank_path[0] = '\0';
    setup->chips_count = -1;
    setup->emulator = -1;
}

int _Mix_ADLMIDI_getTotalBanks(void)
{
    /* Calling of this static call requires pre-loaindg of the library */
    if (ADLMIDI.loaded == 0) {
        ADLMIDI_Load();
    }
    if (ADLMIDI.adl_getBanksCount) {
        return ADLMIDI.adl_getBanksCount();
    }
    return 0;
}

const char *const * _Mix_ADLMIDI_getBankNames()
{
    static const char *const empty[] = {"<no instruments>", NULL};

    /* Calling of this static call requires pre-loaindg of the library */
    if (ADLMIDI.loaded == 0) {
        ADLMIDI_Load();
    }
    if (ADLMIDI.adl_getBankNames) {
        return ADLMIDI.adl_getBankNames();
    }

    return empty;
}

int _Mix_ADLMIDI_getBankID()
{
    return adlmidi_setup.bank;
}

void _Mix_ADLMIDI_setBankID(int bnk)
{
    adlmidi_setup.bank = bnk;
}

int _Mix_ADLMIDI_getTremolo()
{
    return adlmidi_setup.tremolo;
}
void _Mix_ADLMIDI_setTremolo(int tr)
{
    adlmidi_setup.tremolo = tr;
}

int _Mix_ADLMIDI_getVibrato()
{
    return adlmidi_setup.vibrato;
}

void _Mix_ADLMIDI_setVibrato(int vib)
{
    adlmidi_setup.vibrato = vib;
}

int _Mix_ADLMIDI_getScaleMod()
{
    return adlmidi_setup.scalemod;
}

void _Mix_ADLMIDI_setScaleMod(int sc)
{
    adlmidi_setup.scalemod = sc;
}

int _Mix_ADLMIDI_getVolumeModel()
{
    return adlmidi_setup.volume_model;
}

void _Mix_ADLMIDI_setVolumeModel(int vm)
{
    adlmidi_setup.volume_model = vm;
    if (vm < 0) {
        adlmidi_setup.volume_model = 0;
    }
}

int _Mix_ADLMIDI_getFullRangeBrightness()
{
    return adlmidi_setup.full_brightness_range;
}

void _Mix_ADLMIDI_setFullRangeBrightness(int frb)
{
    adlmidi_setup.full_brightness_range = frb;
}

int _Mix_ADLMIDI_getAutoArpeggio()
{
    return adlmidi_setup.auto_arpeggio;
}

void _Mix_ADLMIDI_setAutoArpeggio(int aa_en)
{
    adlmidi_setup.auto_arpeggio = aa_en;
}

int _Mix_ADLMIDI_getChannelAllocMode()
{
    return adlmidi_setup.alloc_mode;
}

void _Mix_ADLMIDI_setChannelAllocMode(int ch_alloc)
{
    adlmidi_setup.alloc_mode = ch_alloc;
}

int _Mix_ADLMIDI_getFullPanStereo()
{
    return adlmidi_setup.soft_pan;
}

void _Mix_ADLMIDI_setFullPanStereo(int fp)
{
    adlmidi_setup.soft_pan = fp;
}

int _Mix_ADLMIDI_getEmulator()
{
    return adlmidi_setup.emulator;
}

void _Mix_ADLMIDI_setEmulator(int emu)
{
    adlmidi_setup.emulator = emu;
}

int _Mix_ADLMIDI_getChipsCount()
{
    return adlmidi_setup.chips_count;
}

void _Mix_ADLMIDI_setChipsCount(int chips)
{
    adlmidi_setup.chips_count = chips;
}

void _Mix_ADLMIDI_setSetDefaults()
{
    ADLMIDI_SetDefault(&adlmidi_setup);
}

void _Mix_ADLMIDI_setCustomBankFile(const char *bank_wonl_path)
{
    if (bank_wonl_path) {
        SDL_strlcpy(adlmidi_setup.custom_bank_path, bank_wonl_path, 2048);
    } else {
        adlmidi_setup.custom_bank_path[0] = '\0';
    }
}

typedef struct
{
    int play_count;
    struct ADL_MIDIPlayer *adlmidi;
    int volume;
    double tempo;
    float gain;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    size_t buffer_samples;
    Mix_MusicMetaTags tags;
    struct ADLMIDI_AudioFormat sample_format;
} AdlMIDI_Music;

/* Set the volume for a ADLMIDI stream */
static void ADLMIDI_setvolume(void *music_p, int volume)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    float v = SDL_floorf(((float)(volume) * music->gain) + 0.5f);
    music->volume = (int)v;
}

/* Get the volume for a ADLMIDI stream */
static int ADLMIDI_getvolume(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    float v = SDL_floorf(((float)(music->volume) / music->gain) + 0.5f);
    return (int)v;
}

static void process_args(const char *args, AdlMidi_Setup *setup)
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
    ADLMIDI_SetDefaultMin(setup);

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
                case 'f':
                    setup->four_op_channels = value;
                    break;
                case 'b':
                    setup->bank = value;
                    break;
                case 't':
                    if (arg[0] == '=') {
                        setup->tempo = SDL_strtod(arg + 1, NULL);
                        if (setup->tempo <= 0.0) {
                            setup->tempo = 1.0;
                        }
                    } else {
                        setup->tremolo = value;
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
                case 'v':
                    setup->vibrato = value;
                    break;
                case 'a':
                    /* Deprecated and useless */
                    break;
                case 'j':
                    setup->auto_arpeggio = value;
                    break;
                case 'o':
                    setup->alloc_mode = value;
                    break;
                case 'm':
                    setup->scalemod = value;
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

static void ADLMIDI_delete(void *music_p);

static AdlMIDI_Music *ADLMIDI_LoadSongRW(SDL_RWops *src, const char *args)
{
    void *bytes = 0;
    int err = 0;
    size_t length = 0;
    AdlMIDI_Music *music = NULL;
    AdlMidi_Setup setup = adlmidi_setup;
    unsigned short src_format = music_spec.format;

    if (src == NULL) {
        return NULL;
    }

    process_args(args, &setup);

    music = (AdlMIDI_Music *)SDL_calloc(1, sizeof(AdlMIDI_Music));

    music->tempo = setup.tempo;
    music->gain = setup.gain;
    music->volume = MIX_MAX_VOLUME;

    switch (music_spec.format) {
    case AUDIO_U8:
        music->sample_format.type = ADLMIDI_SampleType_U8;
        music->sample_format.containerSize = sizeof(Uint8);
        music->sample_format.sampleOffset = sizeof(Uint8) * 2;
        break;
    case AUDIO_S8:
        music->sample_format.type = ADLMIDI_SampleType_S8;
        music->sample_format.containerSize = sizeof(Sint8);
        music->sample_format.sampleOffset = sizeof(Sint8) * 2;
        break;
    case AUDIO_S16LSB:
    case AUDIO_S16MSB:
        music->sample_format.type = ADLMIDI_SampleType_S16;
        music->sample_format.containerSize = sizeof(Sint16);
        music->sample_format.sampleOffset = sizeof(Sint16) * 2;
        src_format = AUDIO_S16SYS;
        break;
    case AUDIO_U16LSB:
    case AUDIO_U16MSB:
        music->sample_format.type = ADLMIDI_SampleType_U16;
        music->sample_format.containerSize = sizeof(Uint16);
        music->sample_format.sampleOffset = sizeof(Uint16) * 2;
        src_format = AUDIO_U16SYS;
        break;
    case AUDIO_S32LSB:
    case AUDIO_S32MSB:
        music->sample_format.type = ADLMIDI_SampleType_S32;
        music->sample_format.containerSize = sizeof(Sint32);
        music->sample_format.sampleOffset = sizeof(Sint32) * 2;
        src_format = AUDIO_S32SYS;
        break;
    case AUDIO_F32LSB:
    case AUDIO_F32MSB:
    default:
        music->sample_format.type = ADLMIDI_SampleType_F32;
        music->sample_format.containerSize = sizeof(float);
        music->sample_format.sampleOffset = sizeof(float) * 2;
        src_format = AUDIO_F32SYS;
    }

    music->stream = SDL_NewAudioStream(src_format, 2, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);

    if (!music->stream) {
        ADLMIDI_delete(music);
        return NULL;
    }

    music->buffer_samples = music_spec.samples * 2 /*channels*/;
    music->buffer_size = music->buffer_samples * music->sample_format.containerSize;
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        ADLMIDI_delete(music);
        return NULL;
    }

    bytes = SDL_LoadFile_RW(src, &length, SDL_FALSE);
    if (!bytes) {
        SDL_OutOfMemory();
        ADLMIDI_delete(music);
        return NULL;
    }

    music->adlmidi = ADLMIDI.adl_init(music_spec.freq);
    if (!music->adlmidi) {
        SDL_free(bytes);
        SDL_OutOfMemory();
        ADLMIDI_delete(music);
        return NULL;
    }

    ADLMIDI.adl_setHVibrato(music->adlmidi, setup.vibrato);
    ADLMIDI.adl_setHTremolo(music->adlmidi, setup.tremolo);

    if (setup.custom_bank_path[0] != '\0') {
        err = ADLMIDI.adl_openBankFile(music->adlmidi, (char*)setup.custom_bank_path);
    } else {
        err = ADLMIDI.adl_setBank(music->adlmidi, setup.bank);
    }

    if (err < 0) {
        Mix_SetError("ADL-MIDI: %s", ADLMIDI.adl_errorInfo(music->adlmidi));
        SDL_free(bytes);
        ADLMIDI_delete(music);
        return NULL;
    }

    ADLMIDI.adl_switchEmulator( music->adlmidi, (setup.emulator >= 0) ? setup.emulator : ADLMIDI_EMU_DOSBOX );
    ADLMIDI.adl_setScaleModulators(music->adlmidi, setup.scalemod);
    ADLMIDI.adl_setVolumeRangeModel(music->adlmidi, setup.volume_model);
    ADLMIDI.adl_setFullRangeBrightness(music->adlmidi, setup.full_brightness_range);
    ADLMIDI.adl_setSoftPanEnabled(music->adlmidi, setup.soft_pan);
    ADLMIDI.adl_setAutoArpeggio(music->adlmidi, setup.auto_arpeggio);
    if (ADLMIDI.adl_setChannelAllocMode) {
        ADLMIDI.adl_setChannelAllocMode(music->adlmidi, setup.alloc_mode);
    }
    ADLMIDI.adl_setNumChips(music->adlmidi, (setup.chips_count >= 0) ? setup.chips_count : ADLMIDI_DEFAULT_CHIPS_COUNT);
    if (setup.four_op_channels >= 0) {
        ADLMIDI.adl_setNumFourOpsChn(music->adlmidi, setup.four_op_channels);
    }
    ADLMIDI.adl_setTempo(music->adlmidi, music->tempo);

    err = ADLMIDI.adl_openData(music->adlmidi, bytes, (unsigned long)length);
    SDL_free(bytes);

    if (err != 0) {
        Mix_SetError("ADL-MIDI: %s", ADLMIDI.adl_errorInfo(music->adlmidi));
        ADLMIDI_delete(music);
        return NULL;
    }

    meta_tags_init(&music->tags);
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_TITLE, ADLMIDI.adl_metaMusicTitle(music->adlmidi));
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_COPYRIGHT, ADLMIDI.adl_metaMusicCopyright(music->adlmidi));
    return music;
}

/* Load ADLMIDI stream from an SDL_RWops object */
static void *ADLMIDI_new_RWex(struct SDL_RWops *src, int freesrc, const char *args)
{
    AdlMIDI_Music *adlmidiMusic;

    adlmidiMusic = ADLMIDI_LoadSongRW(src, args);
    if (adlmidiMusic && freesrc) {
        SDL_RWclose(src);
    }
    return adlmidiMusic;
}

static void *ADLMIDI_new_RW(struct SDL_RWops *src, int freesrc)
{
    return ADLMIDI_new_RWex(src, freesrc, NULL);
}


/* Start playback of a given Game Music Emulators stream */
static int ADLMIDI_play(void *music_p, int play_counts)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    ADLMIDI.adl_setLoopEnabled(music->adlmidi, 1);
    ADLMIDI.adl_setLoopCount(music->adlmidi, play_counts);
    ADLMIDI.adl_positionRewind(music->adlmidi);
    music->play_count = play_counts;
    return 0;
}

static int ADLMIDI_playSome(void *context, void *data, int bytes, SDL_bool *done)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)context;
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

    gottenLen = ADLMIDI.adl_playFormat(music->adlmidi,
                                      music->buffer_samples,
                                      (ADL_UInt8*)music->buffer,
                                      (ADL_UInt8*)music->buffer + music->sample_format.containerSize,
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
            ADLMIDI.adl_positionRewind(music->adlmidi);
            music->play_count = play_count;
        }
    }

    return 0;
}

/* Play some of a stream previously started with ADLMIDI_play() */
static int ADLMIDI_playAudio(void *music_p, void *stream, int len)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    return music_pcm_getaudio(music_p, stream, len, music->volume, ADLMIDI_playSome);
}

/* Close the given Game Music Emulators stream */
static void ADLMIDI_delete(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        meta_tags_clear(&music->tags);
        if (music->adlmidi) {
            ADLMIDI.adl_close(music->adlmidi);
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

static const char* ADLMIDI_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int ADLMIDI_Seek(void *music_p, double time)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    ADLMIDI.adl_positionSeek(music->adlmidi, time);
    return 0;
}

static double ADLMIDI_Tell(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_positionTell(music->adlmidi);
    }
    return -1;
}

static double ADLMIDI_Duration(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_totalTimeLength(music->adlmidi);
    }
    return -1;
}

static int ADLMIDI_StartTrack(void *music_p, int track)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music && ADLMIDI.adl_selectSongNum) {
        ADLMIDI.adl_selectSongNum(music->adlmidi, track);
        return 0;
    }
    return -1;
}

static int ADLMIDI_GetNumTracks(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (ADLMIDI.adl_getSongsCount) {
        return ADLMIDI.adl_getSongsCount(music->adlmidi);
    } else {
        return -1;
    }
}

static int ADLMIDI_SetTempo(void *music_p, double tempo)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music && (tempo > 0.0)) {
        ADLMIDI.adl_setTempo(music->adlmidi, tempo);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double ADLMIDI_GetTempo(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int ADLMIDI_GetTracksCount(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return 16;
    }
    return -1;
}

static int ADLMIDI_SetTrackMute(void *music_p, int track, int mute)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    int ret = -1;
    if (music) {
        ret = ADLMIDI.adl_setChannelEnabled(music->adlmidi, track, mute ? 0 : 1);
        if (ret < 0) {
            Mix_SetError("ADLMIDI: %s", ADLMIDI.adl_errorInfo(music->adlmidi));
        }
    }
    return ret;
}

static double ADLMIDI_LoopStart(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_loopStartTime(music->adlmidi);
    }
    return -1;
}

static double ADLMIDI_LoopEnd(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_loopEndTime(music->adlmidi);
    }
    return -1;
}

static double ADLMIDI_LoopLength(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        double start = ADLMIDI.adl_loopStartTime(music->adlmidi);
        double end = ADLMIDI.adl_loopEndTime(music->adlmidi);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}


Mix_MusicInterface Mix_MusicInterface_ADLMIDI =
{
    "ADLMIDI",
    MIX_MUSIC_ADLMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    ADLMIDI_Load,
    NULL,   /* Open */
    ADLMIDI_new_RW,
    ADLMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    ADLMIDI_setvolume,
    ADLMIDI_getvolume,
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    NULL,       /* Jump */
    ADLMIDI_Seek,
    ADLMIDI_Tell,   /* Tell [MIXER-X]*/
    ADLMIDI_Duration,
    ADLMIDI_SetTempo,   /* [MIXER-X] */
    ADLMIDI_GetTempo,   /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    ADLMIDI_GetTracksCount,   /* [MIXER-X] */
    ADLMIDI_SetTrackMute,   /* [MIXER-X] */
    ADLMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    ADLMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    ADLMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    ADLMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    ADLMIDI_GetNumTracks,
    ADLMIDI_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    ADLMIDI_Unload
};

/* Same as Mix_MusicInterface_ADLMIDI. Created to play special music formats separately from off MIDI interfaces */
Mix_MusicInterface Mix_MusicInterface_ADLIMF =
{
    "ADLIMF",
    MIX_MUSIC_ADLMIDI,
    MUS_ADLMIDI,
    SDL_FALSE,
    SDL_FALSE,

    ADLMIDI_Load,
    NULL,   /* Open */
    ADLMIDI_new_RW,
    ADLMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    ADLMIDI_setvolume,
    ADLMIDI_getvolume, /* GetVolume [MIXER-X]*/
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    NULL,       /* Jump */
    ADLMIDI_Seek,
    ADLMIDI_Tell,   /* Tell [MIXER-X]*/
    ADLMIDI_Duration,
    ADLMIDI_SetTempo,   /* [MIXER-X] */
    ADLMIDI_GetTempo,   /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    NULL,   /* GetTracksCount [MIXER-X] */
    NULL,   /* SetTrackMute [MIXER-X] */
    ADLMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    ADLMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    ADLMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    ADLMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    ADLMIDI_GetNumTracks,
    ADLMIDI_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    ADLMIDI_Unload
};

#endif /* MUSIC_MID_ADLMIDI */

/* vi: set ts=4 sw=4 expandtab: */
