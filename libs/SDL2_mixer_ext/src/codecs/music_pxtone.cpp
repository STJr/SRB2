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
#ifdef MUSIC_PXTONE

#include "music_pxtone.h"

#include "./pxtone/pxtnService.h"
#include "./pxtone/pxtnError.h"

/* Global flags which are applying on initializing of PXTone player with a file */
typedef struct {
    double tempo;
    float gain;
} PXTONE_Setup;

static PXTONE_Setup pxtone_setup = {
    1.0, 1.0f
};

static void PXTONE_SetDefault(PXTONE_Setup *setup)
{
    setup->tempo = 1.0;
    setup->gain = 1.0f;
}

/* This file supports PXTONE music streams */
typedef struct
{
    SDL_RWops *src;
    Sint64 src_start;
    int freesrc;

    int volume;
    double tempo;
    float gain;

    pxtnService *pxtn;
    bool evals_loaded;
    int flags;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    size_t buffer_samples;
    Mix_MusicMetaTags tags;
} PXTONE_Music;


static bool _pxtn_r(void* user, void* p_dst, Sint32 size, Sint32 num)
{
    return SDL_RWread((SDL_RWops*)user, p_dst, size, num) == (size_t)num;
}

static bool _pxtn_w(void* user,const void* p_dst, Sint32 size, Sint32 num)
{
    return SDL_RWwrite((SDL_RWops*)user, p_dst, size, num) == (size_t)num;
}

static bool _pxtn_s(void* user, Sint32 mode, Sint32 size)
{
    return SDL_RWseek((SDL_RWops*)user, size, mode) >= 0;
}

static bool _pxtn_p(void* user, int32_t* p_pos)
{
    int i = SDL_RWtell((SDL_RWops*)user);
    if( i < 0 ) {
        return false;
    }
    *p_pos = i;
    return true;
}

static void process_args(const char *args, PXTONE_Setup *setup)
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
    PXTONE_SetDefault(setup);

    for (i = 0; i < maxlen; i++) {
        char c = args[i];
        if (value_opened == 1) {
            if ((c == ';') || (c == '\0')) {
                arg[j] = '\0';
                switch(type)
                {
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

static void PXTONE_Delete(void *context);

static void *PXTONE_NewRWex(struct SDL_RWops *src, int freesrc, const char *args)
{
    PXTONE_Music *music = NULL;
    const char *name;
    int32_t name_len;
    const char *comment;
    char *temp_string;
    int32_t comment_len;
    pxtnERR ret;
    PXTONE_Setup setup = pxtone_setup;

    music = (PXTONE_Music *)SDL_calloc(1, sizeof *music);
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    process_args(args, &setup);

    music->tempo = setup.tempo;
    music->gain = setup.gain;
    music->volume = MIX_MAX_VOLUME;

    music->pxtn = new pxtnService(_pxtn_r, _pxtn_w, _pxtn_s, _pxtn_p);

    ret = music->pxtn->init();
    if (ret != pxtnOK ) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to initialize the library: %s", pxtnError_get_string(ret));
        return NULL;
    }

    if (!music->pxtn->set_destination_quality(music_spec.channels, music_spec.freq)) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to set the destination quality");
        return NULL;
    }

    /* LOAD MUSIC DATA */
    ret = music->pxtn->read(src);
    if (ret != pxtnOK) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to load a music data: %s", pxtnError_get_string(ret));
        return NULL;
    }

    ret = music->pxtn->tones_ready();
    if (ret != pxtnOK) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to initialize tones: %s", pxtnError_get_string(ret));
        return NULL;
    }

    music->evals_loaded = true;

    /* PREPARATION PLAYING MUSIC */
    music->pxtn->moo_get_total_sample();

    pxtnVOMITPREPARATION prep;
    SDL_memset(&prep, 0, sizeof(pxtnVOMITPREPARATION));
    prep.flags          |= pxtnVOMITPREPFLAG_loop | pxtnVOMITPREPFLAG_unit_mute;
    prep.start_pos_float = 0;
    prep.master_volume   = 1.0f;

    if (!music->pxtn->moo_preparation(&prep)) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to initialize the output (Moo)");
        return NULL;
    }

    music->stream = SDL_NewAudioStream(AUDIO_S16SYS, music_spec.channels, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);

    if (!music->stream) {
        PXTONE_Delete(music);
        return NULL;
    }

    music->buffer_samples = music_spec.samples * music_spec.channels;
    music->buffer_size = music->buffer_samples * sizeof(Sint16);
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        PXTONE_Delete(music);
        return NULL;
    }

    /* Attempt to load metadata */
    music->freesrc = freesrc;

    name = music->pxtn->text->get_name_buf(&name_len);
    if (name) {
        temp_string = SDL_iconv_string("UTF-8", "Shift-JIS", name, name_len + 1);
        meta_tags_set(&music->tags, MIX_META_TITLE, temp_string);
        SDL_free(temp_string);
    }

    comment = music->pxtn->text->get_comment_buf(&comment_len);
    if (comment) {
        temp_string = SDL_iconv_string("UTF-8", "Shift-JIS", comment, comment_len + 1);
        meta_tags_set(&music->tags, MIX_META_COPYRIGHT, temp_string);
        SDL_free(temp_string);
    }

    return music;
}

static void *PXTONE_NewRW(struct SDL_RWops *src, int freesrc)
{
    return PXTONE_NewRWex(src, freesrc, NULL);
}


/* Close the given PXTONE stream */
static void PXTONE_Delete(void *context)
{
    PXTONE_Music *music = (PXTONE_Music*)context;
    if (music) {
        meta_tags_clear(&music->tags);

        if (music->pxtn) {
            if (!music->evals_loaded) {
                music->pxtn->evels->Release();
            }
            music->evals_loaded = false;
            delete music->pxtn;
        }

        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }

        if (music->src && music->freesrc) {
            SDL_RWclose(music->src);
        }
        SDL_free(music);
    }
}

/* Start playback of a given PXTONE stream */
static int PXTONE_Play(void *music_p, int play_count)
{
    pxtnVOMITPREPARATION prep;
    PXTONE_Music *music = (PXTONE_Music*)music_p;

    if (music) {
        SDL_AudioStreamClear(music->stream);
        SDL_memset(&prep, 0, sizeof(pxtnVOMITPREPARATION));
        prep.flags |= pxtnVOMITPREPFLAG_unit_mute;
        if ((play_count < 0) || (play_count > 1)) {
            prep.flags |= pxtnVOMITPREPFLAG_loop;
        }
        music->flags = prep.flags;
        prep.start_pos_float = 0.0f;
        prep.master_volume   = 1.0f;

        if (!music->pxtn->moo_preparation(&prep, music->tempo)) {
            Mix_SetError("PXTONE: Failed to update the output (Moo)");
            return -1;
        }
        music->pxtn->moo_set_loops_num(play_count);
    }
    return 0;
}

static int PXTONE_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    PXTONE_Music *music = (PXTONE_Music *)context;
    int filled;
    bool ret;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    ret = music->pxtn->Moo(music->buffer, music->buffer_size);
    if (!ret) {
        *done = SDL_TRUE;
        return 0;
    }

    if (SDL_AudioStreamPut(music->stream, music->buffer, music->buffer_size) < 0) {
        return -1;
    }

    return 0;
}

/* Play some of a stream previously started with pxtn->moo_preparation() */
static int PXTONE_PlayAudio(void *music_p, void *data, int bytes)
{
    PXTONE_Music *music = (PXTONE_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, PXTONE_GetSome);
}

static const char* PXTONE_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    PXTONE_Music *music = (PXTONE_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Set the volume for a PXTONE stream */
static void PXTONE_SetVolume(void *music_p, int volume)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    float v = SDL_floorf(((float)(volume) * music->gain) + 0.5f);
    music->volume = (int)v;
}

/* Get the volume for a PXTONE stream */
static int PXTONE_GetVolume(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    float v = SDL_floorf(((float)(music->volume) / music->gain) + 0.5f);
    return (int)v;
}

/* Jump (seek) to a given position (time is in seconds) */
static int PXTONE_Seek(void *music_p, double time)
{
    pxtnVOMITPREPARATION prep;
    PXTONE_Music *music = (PXTONE_Music*)music_p;

    SDL_memset(&prep, 0, sizeof(pxtnVOMITPREPARATION));
    prep.flags = music->flags;
    prep.start_pos_sample = (int32_t)((time * music_spec.freq) / music->tempo);
    prep.master_volume   = 1.0f;
    if (!music->pxtn->moo_preparation(&prep, music->tempo)) {
        Mix_SetError("PXTONE: Failed to update the setup of output (Moo) for seek");
        return -1;
    }
    return 0;
}

static double PXTONE_Tell(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music*)music_p;
    int32_t ret = music->pxtn->moo_get_sampling_offset();
    return ((double)ret / music_spec.freq) * music->tempo;
}

static double PXTONE_Duration(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music*)music_p;
    int32_t ret = music->pxtn->moo_get_total_sample();
    return ret > 0 ? ((double)ret / music_spec.freq) * music->tempo : -1.0;
}

static int PXTONE_SetTempo(void *music_p, double tempo)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music && (tempo > 0.0)) {
        music->tempo = tempo;
        music->pxtn->moo_set_tempo_mod(music->tempo);
        return 0;
    }
    return -1;
}

static double PXTONE_GetTempo(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int PXTONE_GetTracksCount(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        return music->pxtn->Unit_Num();
    }
    return -1;
}

static int PXTONE_SetTrackMute(void *music_p, int track, int mute)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        pxtnUnit *u = music->pxtn->Unit_Get_variable( track );
        if (u) {
            u->set_played( !mute );
        }
        return 0;
    }
    return -1;
}

static double PXTONE_LoopStart(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    int32_t ret = music->pxtn->moo_get_sampling_repeat();
    return ((double)ret / music_spec.freq) * music->tempo;
}

static double PXTONE_LoopEnd(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    int32_t ret = music->pxtn->moo_get_sampling_end();
    return ((double)ret / music_spec.freq) * music->tempo;
}

static double PXTONE_LoopLength(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        int32_t start_i = music->pxtn->moo_get_sampling_repeat();
        int32_t end_i = music->pxtn->moo_get_sampling_end();
        double start = ((double)start_i / music_spec.freq) * music->tempo;;
        double end = ((double)end_i / music_spec.freq) * music->tempo;
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}

Mix_MusicInterface Mix_MusicInterface_PXTONE =
{
    "PXTONE",
    MIX_MUSIC_FFMPEG,
    MUS_PXTONE,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    PXTONE_NewRW,
    PXTONE_NewRWex, /* [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    PXTONE_SetVolume,
    PXTONE_GetVolume,
    PXTONE_Play,
    NULL,   /* IsPlaying */
    PXTONE_PlayAudio,
    NULL,   /* Jump */
    PXTONE_Seek,
    PXTONE_Tell,
    PXTONE_Duration,
    PXTONE_SetTempo,  /* [MIXER-X] */
    PXTONE_GetTempo,  /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    PXTONE_GetTracksCount,
    PXTONE_SetTrackMute,
    PXTONE_LoopStart,
    PXTONE_LoopEnd,
    PXTONE_LoopLength,
    PXTONE_GetMetaTag,
    NULL,   /* GetNumTracks */
    NULL,   /* StartTrack */
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    PXTONE_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

#endif // MUSIC_PXTONE
