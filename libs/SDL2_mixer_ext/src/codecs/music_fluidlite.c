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

  James Le Cuirot
  chewi@aura-online.co.uk
*/

#ifdef MUSIC_MID_FLUIDSYNTH

#include "SDL_loadso.h"
#include "SDL_rwops.h"

#include "utils.h"
#include "music_fluidsynth.h"
#include "midi_seq/mix_midi_seq.h"

#include <fluidlite.h>

typedef struct {
    int loaded;
    void *handle;

    int (*delete_fluid_synth)(fluid_synth_t*);
    void (*delete_fluid_settings)(fluid_settings_t*);
    int (*fluid_settings_setnum)(fluid_settings_t*, const char*, double);
    int (*fluid_settings_getnum)(fluid_settings_t*, const char*, double*);
    fluid_settings_t* (*fluid_synth_get_settings)(fluid_synth_t*);
    void (*fluid_synth_set_gain)(fluid_synth_t*, float);
    int (*fluid_synth_sfload)(fluid_synth_t*, const char*, int);
    int (*fluid_synth_write_s16)(fluid_synth_t*, int, void*, int, int, void*, int, int);
    int (*fluid_synth_write_float)(fluid_synth_t*, int, void*, int, int, void*, int, int);
    fluid_settings_t* (*new_fluid_settings)(void);
    fluid_synth_t* (*new_fluid_synth)(fluid_settings_t*);
    int (*fluid_synth_noteon)(fluid_synth_t*, int, int, int);
    int (*fluid_synth_noteoff)(fluid_synth_t*, int, int);
    int (*fluid_synth_cc)(fluid_synth_t*, int, int, int);
    int (*fluid_synth_pitch_bend)(fluid_synth_t*, int, int);
    int (*fluid_synth_key_pressure)(fluid_synth_t*, int, int, int);
    int (*fluid_synth_channel_pressure)(fluid_synth_t*, int, int);
    int (*fluid_synth_program_change)(fluid_synth_t*, int, int);
    int (*fluid_synth_sysex)(fluid_synth_t *, const char *, int, char *, int *, int *, int);
    void (*fluid_synth_set_reverb_on)(fluid_synth_t*, int);
    void (*fluid_synth_set_reverb)(fluid_synth_t*, double, double, double, double);
    void (*fluid_synth_set_chorus_on)(fluid_synth_t*, int);
    void (*fluid_synth_set_chorus)(fluid_synth_t*, int, double, double, double, int);
    int (*fluid_synth_set_polyphony)(fluid_synth_t*, int);
} fluidsynth_loader;

static fluidsynth_loader fluidsynth;

#ifdef FLUIDSYNTH_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    fluidsynth.FUNC = (SIG) SDL_LoadFunction(fluidsynth.handle, #FUNC); \
    if (fluidsynth.FUNC == NULL) { SDL_UnloadObject(fluidsynth.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    fluidsynth.FUNC = FUNC; \
    if (fluidsynth.FUNC == NULL) { Mix_SetError("Missing fluidlite.framework"); return -1; }
#endif

static int FLUIDSYNTH_Load()
{
    if (fluidsynth.loaded == 0) {
#ifdef FLUIDSYNTH_DYNAMIC
        fluidsynth.handle = SDL_LoadObject(FLUIDSYNTH_DYNAMIC);
        if (fluidsynth.handle == NULL) {
            return -1;
        }
#endif
        FUNCTION_LOADER(delete_fluid_synth, int (*)(fluid_synth_t*))
        FUNCTION_LOADER(delete_fluid_settings, void (*)(fluid_settings_t*))
        FUNCTION_LOADER(fluid_settings_setnum, int (*)(fluid_settings_t*, const char*, double))
        FUNCTION_LOADER(fluid_settings_getnum, int (*)(fluid_settings_t*, const char*, double*))
        FUNCTION_LOADER(fluid_synth_get_settings, fluid_settings_t* (*)(fluid_synth_t*))
        FUNCTION_LOADER(fluid_synth_set_gain, void (*)(fluid_synth_t*, float))
        FUNCTION_LOADER(fluid_synth_sfload, int(*)(fluid_synth_t*, const char*, int))
        FUNCTION_LOADER(fluid_synth_write_s16, int(*)(fluid_synth_t*, int, void*, int, int, void*, int, int))
        FUNCTION_LOADER(fluid_synth_write_float, int(*)(fluid_synth_t*, int, void*, int, int, void*, int, int))
        FUNCTION_LOADER(new_fluid_settings, fluid_settings_t* (*)(void))
        FUNCTION_LOADER(new_fluid_synth, fluid_synth_t* (*)(fluid_settings_t*))
        FUNCTION_LOADER(fluid_synth_noteon, int (*)(fluid_synth_t*, int, int, int))
        FUNCTION_LOADER(fluid_synth_noteoff, int (*)(fluid_synth_t*, int, int))
        FUNCTION_LOADER(fluid_synth_cc, int (*)(fluid_synth_t*, int, int, int))
        FUNCTION_LOADER(fluid_synth_pitch_bend, int (*)(fluid_synth_t*, int, int))
        FUNCTION_LOADER(fluid_synth_key_pressure, int (*)(fluid_synth_t*, int, int, int))
        FUNCTION_LOADER(fluid_synth_channel_pressure, int (*)(fluid_synth_t*, int, int))
        FUNCTION_LOADER(fluid_synth_program_change, int (*)(fluid_synth_t*, int, int))
        FUNCTION_LOADER(fluid_synth_sysex, int (*)(fluid_synth_t *, const char *, int, char *, int *, int *, int))
        FUNCTION_LOADER(fluid_synth_set_reverb_on, void (*)(fluid_synth_t*, int))
        FUNCTION_LOADER(fluid_synth_set_reverb, void (*)(fluid_synth_t*, double, double, double, double))
        FUNCTION_LOADER(fluid_synth_set_chorus_on, void (*)(fluid_synth_t*, int))
        FUNCTION_LOADER(fluid_synth_set_chorus, void (*)(fluid_synth_t*, int, double, double, double, int))
        FUNCTION_LOADER(fluid_synth_set_polyphony, int (*)(fluid_synth_t*, int))
    }
    ++fluidsynth.loaded;

    return 0;
}

static void FLUIDSYNTH_Unload()
{
    if (fluidsynth.loaded == 0) {
        return;
    }
    if (fluidsynth.loaded == 1) {
#ifdef FLUIDSYNTH_DYNAMIC
        SDL_UnloadObject(fluidsynth.handle);
#endif
    }
    --fluidsynth.loaded;
}

/* Global OPNMIDI flags which are applying on initializing of MIDI player with a file */
typedef struct {
    char custom_soundfonts[2048];
    double tempo;
    float  gain;

    int      chorus;
    int      chorus_nr;
    double   chorus_level;
    double   chorus_speed;
    double   chorus_depth;
    int      chorus_type;

    int      reverb;
    double   reverb_roomsize;
    double   reverb_damping;
    double   reverb_width;
    double   reverb_level;

    int polyphony;
} FluidSynth_Setup;

static FluidSynth_Setup fluidsynth_setup = {
    "", 1.0, 1.0,
    SDL_TRUE, FLUID_CHORUS_DEFAULT_N, FLUID_CHORUS_DEFAULT_LEVEL,
    FLUID_CHORUS_DEFAULT_SPEED, FLUID_CHORUS_DEFAULT_DEPTH, FLUID_CHORUS_DEFAULT_TYPE,
    SDL_TRUE, FLUID_REVERB_DEFAULT_ROOMSIZE, FLUID_REVERB_DEFAULT_DAMP,
    FLUID_REVERB_DEFAULT_WIDTH, FLUID_REVERB_DEFAULT_LEVEL,
    256
};

static void FLUIDSYNTH_SetDefault(FluidSynth_Setup *setup)
{
    setup->custom_soundfonts[0] = '\0';
    setup->tempo = 1.0;
    setup->gain = 1.0f;

    setup->chorus = SDL_FALSE;
    setup->chorus_nr = FLUID_CHORUS_DEFAULT_N;
    setup->chorus_level = FLUID_CHORUS_DEFAULT_LEVEL;
    setup->chorus_speed = FLUID_CHORUS_DEFAULT_SPEED;
    setup->chorus_depth = FLUID_CHORUS_DEFAULT_DEPTH;
    setup->chorus_type = FLUID_CHORUS_DEFAULT_TYPE;

    setup->reverb = SDL_FALSE;
    setup->reverb_roomsize = FLUID_REVERB_DEFAULT_ROOMSIZE;
    setup->reverb_damping = FLUID_REVERB_DEFAULT_DAMP;
    setup->reverb_width = FLUID_REVERB_DEFAULT_WIDTH;
    setup->reverb_level = FLUID_REVERB_DEFAULT_LEVEL;
    setup->polyphony = 256;
}

typedef struct {
    fluid_synth_t *synth;
    fluid_settings_t *settings;
    BW_MidiRtInterface seq_if;
    int (*synth_write)(fluid_synth_t*, int, void*, int, int, void*, int, int);
    int synth_write_ret;
    void *player;
    SDL_AudioStream *stream;
    void *buffer;
    int buffer_size;
    int sample_size;
    int volume;
    int play_count;
    double tempo;
    float gain;

    Mix_MusicMetaTags tags;
} FLUIDSYNTH_Music;



/****************************************************
 *           Real-Time MIDI calls proxies           *
 ****************************************************/

static void rtNoteOn(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_noteon(music->synth, channel, note, velocity);
}

static void rtNoteOff(void *userdata, uint8_t channel, uint8_t note)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_noteoff(music->synth, channel, note);
}

static void rtNoteAfterTouch(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_key_pressure(music->synth, channel, note, atVal);
}

static void rtChannelAfterTouch(void *userdata, uint8_t channel, uint8_t atVal)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_channel_pressure(music->synth, channel, atVal);
}

static void rtControllerChange(void *userdata, uint8_t channel, uint8_t type, uint8_t value)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_cc(music->synth, channel, type, value);
}

static void rtPatchChange(void *userdata, uint8_t channel, uint8_t patch)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_program_change(music->synth, channel, patch);
}

static void rtPitchBend(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_pitch_bend(music->synth, channel, (msb << 7) | lsb);
}

static void rtSysEx(void *userdata, const uint8_t *msg, size_t size)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    fluidsynth.fluid_synth_sysex(music->synth, (const char*)(msg), (int)(size), NULL, NULL, NULL, 0);
}

static void playSynthBuffer(void *userdata, uint8_t *stream, size_t length)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music*)(userdata);
    int samples = (int)length / (music->sample_size * 2);

    if (music->synth_write_ret < 0) {
        return;
    }

    music->synth_write_ret = music->synth_write(music->synth, samples, stream, 0, 2, stream, 1, 2);
}

static int init_interface(FLUIDSYNTH_Music *music)
{
    int in_format = AUDIO_S16SYS;
    SDL_memset(&music->seq_if, 0, sizeof(BW_MidiRtInterface));

    /* seq->onDebugMessage             = hooks.onDebugMessage; */
    /* seq->onDebugMessage_userData    = hooks.onDebugMessage_userData; */

    /* MIDI Real-Time calls */
    music->seq_if.rtUserData = (void *)music;
    music->seq_if.rt_noteOn  = rtNoteOn;
    music->seq_if.rt_noteOff = rtNoteOff;
    music->seq_if.rt_noteAfterTouch = rtNoteAfterTouch;
    music->seq_if.rt_channelAfterTouch = rtChannelAfterTouch;
    music->seq_if.rt_controllerChange = rtControllerChange;
    music->seq_if.rt_patchChange = rtPatchChange;
    music->seq_if.rt_pitchBend = rtPitchBend;
    music->seq_if.rt_systemExclusive = rtSysEx;

    music->seq_if.onPcmRender = playSynthBuffer;
    music->seq_if.pcmSampleRate = music_spec.freq;

    music->synth_write = fluidsynth.fluid_synth_write_s16;
    music->sample_size = sizeof(Sint16);
    music->seq_if.pcmFrameSize = 2 * music->sample_size;
    music->seq_if.onPcmRender_userData = music;

    if (music_spec.format & 0x0020) { /* 32 bit. */
        music->synth_write = fluidsynth.fluid_synth_write_float;
        music->sample_size <<= 1;
        music->seq_if.pcmFrameSize <<= 1;
        in_format = AUDIO_F32SYS;
    }

    return in_format;
}

static void all_notes_off(fluid_synth_t *synth)
{
    int channel;

    if (!synth) {
        return; /* Nothing to do */
    }

    for (channel = 0; channel < 16; channel++) {
        fluidsynth.fluid_synth_cc(synth, channel, 123, 0);
    }
}


static void FLUIDSYNTH_Delete(void *context);

#if 0
static int SDLCALL fluidsynth_check_soundfont(const char *path, void *data)
{
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");

    (void)data;
    if (rw) {
        SDL_RWclose(rw);
        return 1;
    } else {
        Mix_SetError("Failed to access the SoundFont %s", path);
        return 0;
    }
}
#endif

static int SDLCALL fluidsynth_load_soundfont(const char *path, void *data)
{
    /* If this fails, it's too late to try Timidity so pray that at least one works. */
    fluidsynth.fluid_synth_sfload((fluid_synth_t*) data, path, 1);
    return 1;
}

static int FLUIDSYNTH_Open(const SDL_AudioSpec *spec)
{
    (void)spec;
    /* Don't pre-check SoundFonts here to allow path arguments being used without pre-loaded SoundFonts */
    /*
    if (!Mix_EachSoundFont(fluidsynth_check_soundfont, NULL)) {
        return -1;
    }
    */
    return 0;
}

static void replace_colon_to_semicolon(char *str)
{
    size_t i;
    size_t len = SDL_strlen(str);
    for (i = 0; str && i < len; ++str, ++i) {
        if(*str == '&') {
            *str = ';';
        }
    }
}

static void process_args(const char *args, FluidSynth_Setup *setup)
{
#define ARG_BUFFER_SIZE    4096
    char arg[ARG_BUFFER_SIZE];
    char type = '-';
    size_t maxlen = 0;
    size_t i, j = 0;
    int value_opened = 0;

    FLUIDSYNTH_SetDefault(setup);

    if (args == NULL) {
        return;
    }

    maxlen = SDL_strlen(args);
    if (maxlen == 0) {
        return;
    }

    maxlen += 1;

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
                    switch(arg[0])
                    {
                    case 'n':
                        setup->chorus_nr = SDL_atoi(arg + 1);
                        if (setup->chorus_nr < 0 || setup->chorus_nr > 99) {
                            setup->chorus_nr = FLUID_CHORUS_DEFAULT_N;
                        }
                        break;
                    case 'l':
                        setup->chorus_level = SDL_atof(arg + 1);
                        if (setup->chorus_level < 0.0 || setup->chorus_level > 10.0) {
                            setup->chorus_level = FLUID_CHORUS_DEFAULT_LEVEL;
                        }
                        break;
                    case 's':
                        setup->chorus_speed = SDL_atof(arg + 1);
                        if (setup->chorus_speed < 0.1 || setup->chorus_speed > 5.0) {
                            setup->chorus_speed = FLUID_CHORUS_DEFAULT_SPEED;
                        }
                        break;
                    case 'd':
                        setup->chorus_depth = SDL_atof(arg + 1);
                        if (setup->chorus_depth < 0.0 || setup->chorus_depth > 21.0) {
                            setup->chorus_depth = FLUID_CHORUS_DEFAULT_DEPTH;
                        }
                        break;
                    case 't':
                        setup->chorus_type = SDL_atoi(arg + 1);
                        if (setup->chorus_type < 0 || setup->chorus_type > 1) {
                            setup->chorus_type = FLUID_CHORUS_DEFAULT_N;
                        }
                        break;
                    default:
                        setup->chorus = (value != 0);
                        break;
                    }
                    break;
                case 'r':
                    switch(arg[0])
                    {
                    case 'r':
                        setup->reverb_roomsize = SDL_atof(arg + 1);
                        if (setup->reverb_roomsize < 0.0 || setup->reverb_roomsize > 1.0) {
                            setup->reverb_roomsize = FLUID_REVERB_DEFAULT_ROOMSIZE;
                        }
                        break;
                    case 'd':
                        setup->reverb_damping = SDL_atof(arg + 1);
                        if (setup->reverb_damping < 0.0 || setup->reverb_damping > 1.0) {
                            setup->reverb_damping = FLUID_REVERB_DEFAULT_DAMP;
                        }
                        break;
                    case 'w':
                        setup->reverb_width = SDL_atof(arg + 1);
                        if (setup->reverb_width < 0.0 || setup->reverb_width > 100.0) {
                            setup->reverb_width = FLUID_REVERB_DEFAULT_WIDTH;
                        }
                        break;
                    case 'l':
                        setup->reverb_level = SDL_atof(arg + 1);
                        if (setup->reverb_level < 0 || setup->reverb_level > 1.0) {
                            setup->reverb_level = FLUID_REVERB_DEFAULT_LEVEL;
                        }
                        break;
                    default:
                        setup->reverb = (value != 0);
                        break;
                    }
                    break;
                case 'p':
                    setup->polyphony = value;
                    if (setup->polyphony < 8 || setup->polyphony > 512) {
                        setup->polyphony = 256;
                    }
                    break;
                case 't':
                    if (arg[0] == '=') {
                        setup->tempo = SDL_atof(arg + 1);
                        if (setup->tempo <= 0.0) {
                            setup->tempo = 1.0;
                        }
                    }
                    break;
                case 'g':
                    if (arg[0] == '=') {
                        setup->gain = (float)SDL_atof(arg + 1);
                        if (setup->gain < 0.0f) {
                            setup->gain = 1.0f;
                        }
                    }
                    break;
                case 'x':
                    if (arg[0] == '=') {
                        SDL_strlcpy(setup->custom_soundfonts, arg + 1, (ARG_BUFFER_SIZE - 1));
                        replace_colon_to_semicolon(setup->custom_soundfonts);
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

static FLUIDSYNTH_Music *FLUIDSYNTH_LoadMusicArg(void *data, const char *args)
{
    SDL_RWops *src = (SDL_RWops *)data;
    FLUIDSYNTH_Music *music;
    FluidSynth_Setup setup = fluidsynth_setup;
    double samplerate; /* as set by the lib. */
    int src_format = AUDIO_S16SYS;
    const Uint8 channels = 2;
    void *rw_mem;
    size_t rw_size;
    int ret;

    process_args(args, &setup);

    if (!(music = SDL_calloc(1, sizeof(FLUIDSYNTH_Music)))) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->volume = MIX_MAX_VOLUME;
    music->tempo = setup.tempo;
    music->gain = setup.gain;
    music->play_count = 0;

    src_format = init_interface(music);
    music->buffer_size = music_spec.samples * music->sample_size * channels;

    if (!(music->buffer = SDL_malloc((size_t)music->buffer_size))) {
        SDL_OutOfMemory();
        goto fail;
    }

    if (!(music->settings = fluidsynth.new_fluid_settings())) {
        Mix_SetError("Failed to create FluidSynth settings");
        goto fail;
    }

    fluidsynth.fluid_settings_setnum(music->settings, "synth.sample-rate", (double) music_spec.freq);
    fluidsynth.fluid_settings_getnum(music->settings, "synth.sample-rate", &samplerate);
    music->seq_if.pcmSampleRate = samplerate;

    if (!(music->synth = fluidsynth.new_fluid_synth(music->settings))) {
        Mix_SetError("Failed to create FluidSynth synthesizer");
        goto fail;
    }


    if (setup.custom_soundfonts[0]) {
        ret = Mix_EachSoundFontEx(setup.custom_soundfonts, fluidsynth_load_soundfont, music->synth);
    } else {
        ret = Mix_EachSoundFont(fluidsynth_load_soundfont, music->synth);
    }

    if (!ret) {
        goto fail;
    }


    fluidsynth.fluid_synth_set_reverb_on(music->synth, setup.reverb);
    fluidsynth.fluid_synth_set_reverb(music->synth,
                                      setup.reverb_roomsize, setup.reverb_damping,
                                      setup.reverb_width, setup.reverb_level);
    fluidsynth.fluid_synth_set_chorus_on(music->synth, setup.chorus);
    fluidsynth.fluid_synth_set_chorus(music->synth,
                                      setup.chorus_nr, setup.chorus_level,
                                      setup.chorus_speed, setup.chorus_depth, setup.chorus_type);
    fluidsynth.fluid_synth_set_polyphony(music->synth, setup.polyphony);

    if (!(music->player = midi_seq_init_interface(&music->seq_if))) {
        Mix_SetError("Failed to create FluidSynth player");
        goto fail;
    }

    rw_mem = SDL_LoadFile_RW(src, &rw_size, SDL_FALSE);
    if (!rw_mem) {
        SDL_OutOfMemory();
        goto fail;
    }

    ret = midi_seq_openData(music->player, rw_mem, rw_size);
    SDL_free(rw_mem);

    if (ret < 0) {
        Mix_SetError("FluidSynth failed to load in-memory song: %s", midi_seq_get_error(music->player));
        goto fail;
    }

    midi_seq_set_tempo_multiplier(music->player, music->tempo);

    if (!(music->stream = SDL_NewAudioStream(src_format, channels, (int) samplerate,
                          music_spec.format, music_spec.channels, music_spec.freq))) {
        goto fail;
    }

    meta_tags_init(&music->tags);

    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_TITLE, midi_seq_meta_title(music->player));
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_COPYRIGHT, midi_seq_meta_copyright(music->player));

    return music;

fail:
    FLUIDSYNTH_Delete(music);
    return NULL;
}

static void *FLUIDSYNTH_CreateFromRWEx(SDL_RWops *src, int freesrc, const char *args)
{
    FLUIDSYNTH_Music *music;

    music = FLUIDSYNTH_LoadMusicArg(src, args);
    if (music && freesrc) {
        SDL_RWclose(src);
    }
    return music;
}

static void *FLUIDSYNTH_CreateFromRW(SDL_RWops *src, int freesrc)
{
    return FLUIDSYNTH_CreateFromRWEx(src, freesrc, NULL);
}


static void FLUIDSYNTH_SetVolume(void *context, int volume)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    /* FluidSynth's default gain is 0.2. Make 1.0 the maximum gain value to avoid sound overload. */
    music->volume = volume;
    fluidsynth.fluid_synth_set_gain(music->synth, ((float)volume / MIX_MAX_VOLUME) * music->gain);
}

static int FLUIDSYNTH_GetVolume(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    return music->volume;
}

static int FLUIDSYNTH_Play(void *context, int play_count)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    SDL_AudioStreamClear(music->stream);
    midi_seq_set_loop_enabled(music->player, 1);
    midi_seq_set_loop_count(music->player, play_count);
    midi_seq_rewind(music->player);
    music->play_count = play_count;
    return 0;
}

static const char* FLUIDSYNTH_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

static int FLUIDSYNTH_Seek(void *context, double time)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    midi_seq_seek(music->player, time);
    all_notes_off(music->synth);
    return 0;
}

static double FLUIDSYNTH_Tell(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    return midi_seq_tell(music->player);
}

static double FLUIDSYNTH_Duration(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    return midi_seq_length(music->player);
}

static int FLUIDSYNTH_StartTrack(void *music_p, int track)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)music_p;
    midi_switch_song_number(music->player, track);
    return -1;
}

static int FLUIDSYNTH_GetNumTracks(void *music_p)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)music_p;
    return midi_get_songs_count(music->player);
}

static int FLUIDSYNTH_SetTempo(void *context, double tempo)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music && (tempo > 0.0)) {
        midi_seq_set_tempo_multiplier(music->player, tempo);
        music->tempo = tempo;
        return 0;
    }
    return 0;
}

static double FLUIDSYNTH_GetTempo(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int FLUIDSYNTH_GetTracksCount(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music) {
        return 16; /* MIDI Channels count */
    }
    return -1;
}

static int FLUIDSYNTH_SetTrackMuted(void *context, int track, int mute)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music) {
        return midi_set_channel_enabled(music->player, track, mute ? 0 : 1);
    }
    return -1;
}

static double FLUIDSYNTH_LoopStart(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music) {
        return midi_seq_loop_start(music->player);
    }
    return -1;
}

static double FLUIDSYNTH_LoopEnd(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music) {
        return midi_seq_loop_end(music->player);
    }
    return -1;
}

static double FLUIDSYNTH_LoopLength(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    if (music) {
        double start = midi_seq_loop_start(music->player);
        double end = midi_seq_loop_end(music->player);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}

static int FLUIDSYNTH_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;
    int filled, gotten_len, amount;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    music->synth_write_ret = 0;

    gotten_len = midi_seq_play_buffer(music->player, music->buffer, music->buffer_size);

    if (music->synth_write_ret < 0) {
        Mix_SetError("Error generating FluidSynth audio");
        return -1;
    }

    if (gotten_len <= 0) {
        *done = SDL_TRUE;
        return 0;
    }

    amount = gotten_len;
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
            midi_seq_rewind(music->player);
            all_notes_off(music->synth);
            music->play_count = play_count;
        }
    }

    return 0;
}
static int FLUIDSYNTH_GetAudio(void *context, void *data, int bytes)
{
    return music_pcm_getaudio(context, data, bytes, MIX_MAX_VOLUME, FLUIDSYNTH_GetSome);
}

static void FLUIDSYNTH_Delete(void *context)
{
    FLUIDSYNTH_Music *music = (FLUIDSYNTH_Music *)context;

    if (music->player) {
        midi_seq_free(music->player);
    }

    meta_tags_clear(&music->tags);

    if (music->synth) {
        fluidsynth.delete_fluid_synth(music->synth);
    }
    if (music->settings) {
        fluidsynth.delete_fluid_settings(music->settings);
    }
    if (music->stream) {
        SDL_FreeAudioStream(music->stream);
    }
    if (music->buffer) {
        SDL_free(music->buffer);
    }
    SDL_free(music);
}

Mix_MusicInterface Mix_MusicInterface_FLUIDSYNTH =
{
    "FLUIDSYNTH",
    MIX_MUSIC_FLUIDSYNTH,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    FLUIDSYNTH_Load,
    FLUIDSYNTH_Open,
    FLUIDSYNTH_CreateFromRW,
    FLUIDSYNTH_CreateFromRWEx,
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    FLUIDSYNTH_SetVolume,
    FLUIDSYNTH_GetVolume,
    FLUIDSYNTH_Play,
    NULL,
    FLUIDSYNTH_GetAudio,
    NULL,   /* Jump */
    FLUIDSYNTH_Seek,
    FLUIDSYNTH_Tell,
    FLUIDSYNTH_Duration,
    FLUIDSYNTH_SetTempo,
    FLUIDSYNTH_GetTempo,
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    FLUIDSYNTH_GetTracksCount, /* [MIXER-X] */
    FLUIDSYNTH_SetTrackMuted,  /* [MIXER-X] */
    FLUIDSYNTH_LoopStart,
    FLUIDSYNTH_LoopEnd,
    FLUIDSYNTH_LoopLength,
    FLUIDSYNTH_GetMetaTag,
    FLUIDSYNTH_GetNumTracks,
    FLUIDSYNTH_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    FLUIDSYNTH_Delete,
    NULL,   /* Close */
    FLUIDSYNTH_Unload
};

/* Special case to play XMI/MUS formats */
Mix_MusicInterface Mix_MusicInterface_FLUIDXMI =
{
    "FLUIDXMI",
    MIX_MUSIC_FLUIDSYNTH,
    MUS_FLUIDLITE,
    SDL_FALSE,
    SDL_FALSE,

    FLUIDSYNTH_Load,
    FLUIDSYNTH_Open,
    FLUIDSYNTH_CreateFromRW,
    FLUIDSYNTH_CreateFromRWEx,
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    FLUIDSYNTH_SetVolume,
    FLUIDSYNTH_GetVolume,
    FLUIDSYNTH_Play,
    NULL,
    FLUIDSYNTH_GetAudio,
    NULL,   /* Jump */
    FLUIDSYNTH_Seek,
    FLUIDSYNTH_Tell,
    FLUIDSYNTH_Duration,
    FLUIDSYNTH_SetTempo,
    FLUIDSYNTH_GetTempo,
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    FLUIDSYNTH_GetTracksCount, /* [MIXER-X] */
    FLUIDSYNTH_SetTrackMuted,  /* [MIXER-X] */
    FLUIDSYNTH_LoopStart,
    FLUIDSYNTH_LoopEnd,
    FLUIDSYNTH_LoopLength,
    FLUIDSYNTH_GetMetaTag,
    FLUIDSYNTH_GetNumTracks,
    FLUIDSYNTH_StartTrack,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    FLUIDSYNTH_Delete,
    NULL,   /* Close */
    FLUIDSYNTH_Unload
};

#endif /* MUSIC_MID_FLUIDSYNTH */

/* vi: set ts=4 sw=4 expandtab: */
