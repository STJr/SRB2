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

#ifdef MUSIC_MID_NATIVE

/* This file supports playing MIDI files with OS APIs */

#include "SDL_timer.h"
#include "SDL_atomic.h"
#include "SDL_mutex.h"
#include "utils.h"
#include "music_nativemidi.h"
#include "midi_seq/mix_midi_seq.h"
#include <windows.h>


static SDL_INLINE unsigned int makeEvt(Uint8 t, Uint8 channel)
{
    return ((t << 4) & 0xF0) | (channel & 0x0F);
}

typedef struct _NativeMidiSong
{
    void *song;
    SDL_atomic_t running;
    SDL_mutex *lock;
    SDL_mutex *pause;
    SDL_Thread* thread;
    SDL_bool paused;
    double tempo;
    int loops;
    int volume;
    Uint32 doResetControls;

    BW_MidiRtInterface seq_if;
    HMIDIOUT out;
    double volumeFactor;
    int channelVolume[16];
    HANDLE hTimer;

    Mix_MusicMetaTags tags;
} NativeMidiSong;


static void update_volume(NativeMidiSong *seq)
{
    DWORD msg;
    Uint8 i, value;

    if (!seq->out) {
        return; /* Nothing to do*/
    }

    for (i = 0; i < 16; i++) {
        value = (Uint8)(seq->channelVolume[i] * seq->volumeFactor);
        msg = 0;
        msg |= (value << 16) & 0xFF0000;
        msg |= (7 << 8) & 0x00FF00;
        msg |= makeEvt(0x0B, i);
        midiOutShortMsg(seq->out, msg);
    }
}

static void all_notes_off(NativeMidiSong *seq)
{
    DWORD msg = 0;
    Uint8 channel, note;

    if (!seq->out) {
        return; /* Nothing to do */
    }

    for (channel = 0; channel < 16; channel++) {
        for (note = 0; note < 127 ; note++) {
            msg = 0;
            msg |= (0 << 16) & 0xFF0000;
            msg |= (note << 8) & 0x00FF00;
            msg |= makeEvt(0x09, channel);
            while(midiOutShortMsg(seq->out, msg) == MIDIERR_NOTREADY) {
                SDL_Delay(1);
            }
        }
    }
}

static void reset_midi_device(NativeMidiSong *seq)
{
    while(midiOutReset(seq->out) == MIDIERR_NOTREADY){
        SDL_Delay(1);
    }
}


/****************************************************
 *           Real-Time MIDI calls proxies           *
 ****************************************************/

static void rtNoteOn(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;
    msg |= (velocity << 16) & 0xFF0000;
    msg |= (note << 8) & 0x00FF00;
    msg |= makeEvt(0x09, channel);
    midiOutShortMsg(seqi->out, msg);
}

static void rtNoteOff(void *userdata, uint8_t channel, uint8_t note)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;
    msg |= (note << 8) & 0x00FF00;
    msg |= makeEvt(0x08, channel);
    midiOutShortMsg(seqi->out, msg);
}

static void rtNoteAfterTouch(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;
    msg |= (atVal << 16) & 0xFF0000;
    msg |= (note << 8) & 0x00FF00;
    msg |= makeEvt(0x0A, channel);
    midiOutShortMsg(seqi->out, msg);
}

static void rtChannelAfterTouch(void *userdata, uint8_t channel, uint8_t atVal)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;
    msg |= (atVal << 8) & 0x00FF00;
    msg |= makeEvt(0x0D, channel);
    midiOutShortMsg(seqi->out, msg);
}

static void rtControllerChange(void *userdata, uint8_t channel, uint8_t type, uint8_t value)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;

    if (type == 7) {
        seqi->channelVolume[channel] = value;
        if(seqi->volumeFactor < 1.0) /* Pseudo-volume */
            value = (Uint8)(value * seqi->volumeFactor);
    }

    msg |= (value << 16) & 0xFF0000;
    msg |= (type << 8) & 0x00FF00;
    msg |= makeEvt(0x0B, channel);
    while(midiOutShortMsg(seqi->out, msg) == MIDIERR_NOTREADY) {
        SDL_Delay(1);
    }
}

static void rtPatchChange(void *userdata, uint8_t channel, uint8_t patch)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;
    msg |= (patch << 8) & 0x00FF00;
    msg |= makeEvt(0x0C, channel);
    midiOutShortMsg(seqi->out, msg);
}

static void rtPitchBend(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    DWORD msg = 0;
    msg |= (((int)(msb) << 8) | (int)(lsb)) << 8;
    msg |= makeEvt(0x0E, channel);
    midiOutShortMsg(seqi->out, msg);
}

static void rtSysEx(void *userdata, const uint8_t *msg, size_t size)
{
    NativeMidiSong *seqi = (NativeMidiSong*)(userdata);
    MIDIHDR hd;
    hd.dwBufferLength = size;
    hd.dwBytesRecorded = size;
    hd.lpData = (LPSTR)msg;/*(LPSTR)malloc(size);
    memcpy(hd.lpData, msg, size);*/
    midiOutPrepareHeader(seqi->out, &hd, sizeof(hd));
    midiOutLongMsg(seqi->out, &hd, sizeof(hd));
}


/* NonStandard calls */
/*
static void rtDeviceSwitch(void *userdata, size_t track, const char *data, size_t length)
{

}

static size_t rtCurrentDevice(void *userdata, size_t track)
{
    return 0;
}
*/
/* NonStandard calls End */


static int init_midi_out(NativeMidiSong *seqi)
{
    Uint8 i;
    MMRESULT err = midiOutOpen(&seqi->out, 0, 0, 0, CALLBACK_NULL);

    if (err != MMSYSERR_NOERROR)
    {
        seqi->out = NULL;
        return -1;
    }

    for(i = 0; i < 16; i++)
        seqi->channelVolume[i] = 100;

    update_volume(seqi);

    return 0;
}

static void close_midi_out(NativeMidiSong *seqi)
{
    if(seqi->out)
        midiOutClose(seqi->out);
    seqi->out = NULL;
}

static void init_interface(NativeMidiSong *seqi)
{
    SDL_memset(&seqi->seq_if, 0, sizeof(BW_MidiRtInterface));

    /* seq->onDebugMessage             = hooks.onDebugMessage; */
    /* seq->onDebugMessage_userData    = hooks.onDebugMessage_userData; */

    /* MIDI Real-Time calls */
    seqi->seq_if.rtUserData = (void *)seqi;
    seqi->seq_if.rt_noteOn  = rtNoteOn;
    seqi->seq_if.rt_noteOff = rtNoteOff;
    seqi->seq_if.rt_noteAfterTouch = rtNoteAfterTouch;
    seqi->seq_if.rt_channelAfterTouch = rtChannelAfterTouch;
    seqi->seq_if.rt_controllerChange = rtControllerChange;
    seqi->seq_if.rt_patchChange = rtPatchChange;
    seqi->seq_if.rt_pitchBend = rtPitchBend;
    seqi->seq_if.rt_systemExclusive = rtSysEx;

    /* NonStandard calls */
    /*seqi->seq_if->rt_deviceSwitch = rtDeviceSwitch; */
    /* seqi->seq_if->rt_currentDevice = rtCurrentDevice; */
    /* NonStandard calls End */
}

static void win32_seq_set_volume(NativeMidiSong *seqi, double volume)
{
    seqi->volumeFactor = volume;
    update_volume(seqi);
}

int native_midi_detect(void)
{
    HMIDIOUT out;
    MMRESULT err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);

    if (err == MMSYSERR_NOERROR) {
        return -1;
    }

    midiOutClose(out);

    return 1;
}

static Uint64 GetTicksUS()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    Uint64 tt = ft.dwHighDateTime;
    tt <<=32;
    tt |= ft.dwLowDateTime;
    tt /=10;
    tt -= 11644473600000000ULL;
    return tt;
}

static int NativeMidiThread(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    LARGE_INTEGER liDueTime;
    Uint64 start, end;
    Uint8 i;
    double t = 0.000001, w, wait = -1.0;

    liDueTime.QuadPart = -10000000LL;

    /* Create an unnamed waitable timer.*/
    music->hTimer = CreateWaitableTimerW(NULL, TRUE, NULL);
    if (NULL == music->hTimer) {
        Mix_SetError("Native MIDI Win32-Alt: CreateWaitableTimer failed (%lu)\n", (unsigned long)GetLastError());
        return 1;
    }

    init_midi_out(music);

    midi_seq_set_loop_enabled(music->song, 1);
    midi_seq_set_loop_count(music->song, music->loops < 0 ? -1 : (music->loops + 1));
    midi_seq_set_tempo_multiplier(music->song, music->tempo);

    SDL_AtomicSet(&music->running, 1);

    while (SDL_AtomicGet(&music->running)) {
        start = GetTicksUS();

        SDL_LockMutex(music->lock);
        if(music->doResetControls) /* Workaround for some synths to don't miss notes */
        {
            SDL_Delay(100);
            for (i = 0; i < 16; ++i) { /* Reset pedal controllers */
                rtControllerChange(music->song, i, 121, 0);
                rtControllerChange(music->song, i, 64, 0);
                rtControllerChange(music->song, i, 66, 0);
            }
            start = GetTicksUS();
            music->doResetControls = 0;
            t = 0.000001;
        }
        if (midi_seq_at_end(music->song)) {
            SDL_UnlockMutex(music->lock);
            break;
        }
        t = midi_seq_tick(music->song, t, 0.0001);
        SDL_UnlockMutex(music->lock);
        end = GetTicksUS();

        if (t < 0.000001) {
            t = 0.000001;
        }

        w = (double)(end - start) / 1000000;
        wait = t - w;

        if (wait > 0.0) {
            liDueTime.QuadPart = (LONGLONG)-(wait * 10000000.0);
            if (!SetWaitableTimer(music->hTimer, &liDueTime, 0, NULL, NULL, 0)) {
                Mix_SetError("Native MIDI Win32-Alt: SetWaitableTimer failed (%lu)\n", (unsigned long)GetLastError());
                return 2;
            }

            if (WaitForSingleObject(music->hTimer, INFINITE) != WAIT_OBJECT_0) {
                Mix_SetError("Native MIDI Win32-Alt: WaitForSingleObject failed (%lu)\n", (unsigned long)GetLastError());
            }
        }

        if (music->paused) { /* Hacky "pause" done by a mutex */
            SDL_LockMutex(music->pause);
            SDL_UnlockMutex(music->pause);
        }
    }

    SDL_AtomicSet(&music->running, 0);

    close_midi_out(music);
    CloseHandle(music->hTimer);
    music->hTimer = NULL;

    return 0;
}

static NativeMidiSong *NATIVEMIDI_Create()
{
    size_t i;

    NativeMidiSong *s = (NativeMidiSong*)SDL_calloc(1, sizeof(NativeMidiSong));
    if (s) {
        s->lock = SDL_CreateMutex();
        s->pause = SDL_CreateMutex();
        SDL_AtomicSet(&s->running, 0);
        s->out = NULL;
        s->loops = 0;
        s->tempo = 1.0;
        s->volume = MIX_MAX_VOLUME;
        s->volumeFactor = 1.0;
        for (i = 0; i < 16; ++i) {
            s->channelVolume[i] = 100;
        }
    }
    return s;
}

static void NATIVEMIDI_Destroy(NativeMidiSong *music)
{
    if (music) {
        meta_tags_clear(&music->tags);
        SDL_DestroyMutex(music->lock);
        SDL_DestroyMutex(music->pause);
        SDL_free(music);
    }
}

static void *NATIVEMIDI_CreateFromRW(SDL_RWops *src, int freesrc)
{
    void *bytes = 0;
    long filesize = 0;
    unsigned char byte[1];
    Sint64 length = 0;
    size_t bytes_l;
    NativeMidiSong *music = NATIVEMIDI_Create();

    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    length = SDL_RWseek(src, 0, RW_SEEK_END);
    if (length < 0) {
        Mix_SetError("NativeMIDI: wrong file\n");
        NATIVEMIDI_Destroy(music);
        return NULL;
    }

    SDL_RWseek(src, 0, RW_SEEK_SET);
    bytes = SDL_malloc((size_t)length);
    if (!bytes) {
        SDL_OutOfMemory();
        NATIVEMIDI_Destroy(music);
        return NULL;
    }

    filesize = 0;
    while ((bytes_l = SDL_RWread(src, &byte, sizeof(Uint8), 1)) != 0) {
        ((Uint8 *)bytes)[filesize] = byte[0];
        filesize++;
    }

    if (filesize == 0) {
        SDL_free(bytes);
        NATIVEMIDI_Destroy(music);
        Mix_SetError("NativeMIDI: wrong file\n");
        return NULL;
    }

    init_interface(music);
    music->song = midi_seq_init_interface(&music->seq_if);
    if (!music->song) {
        SDL_OutOfMemory();
        NATIVEMIDI_Destroy(music);
        return NULL;
    }


    if (midi_seq_openData(music->song, bytes, filesize) < 0) {
        Mix_SetError("NativeMIDI: %s", midi_seq_get_error(music->song));
        NATIVEMIDI_Destroy(music);
        return NULL;
    }

    if (freesrc) {
        SDL_RWclose(src);
    }

    meta_tags_init(&music->tags);
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_TITLE, midi_seq_meta_title(music->song));
    _Mix_ParseMidiMetaTag(&music->tags, MIX_META_COPYRIGHT, midi_seq_meta_copyright(music->song));
    return music;
}

static int NATIVEMIDI_Play(void *context, int play_count)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    int loops = play_count;
    if (loops > 0) {
        --loops;
    }

    if (!SDL_AtomicGet(&music->running) && !music->thread) {
        music->loops = loops;
        SDL_AtomicSet(&music->running, 1);
        music->thread = SDL_CreateThread(NativeMidiThread, "NativeMidiLoop", (void *)music);
    }

    return 0;
}

static void NATIVEMIDI_SetVolume(void *context, int volume)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double vf = (double)volume / MIX_MAX_VOLUME;
    music->volume = volume;
    SDL_LockMutex(music->lock);
    win32_seq_set_volume(music, vf);
    SDL_UnlockMutex(music->lock);
}

static int NATIVEMIDI_GetVolume(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    return music->volume;
}

static SDL_bool NATIVEMIDI_IsPlaying(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    return SDL_AtomicGet(&music->running) ? SDL_TRUE : SDL_FALSE;
}

static void NATIVEMIDI_Pause(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (!music->paused) {
        SDL_LockMutex(music->pause);
        music->paused = SDL_TRUE;
        all_notes_off(music);
    }
}

static void NATIVEMIDI_Resume(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music->paused) {
        music->paused = SDL_FALSE;
        SDL_UnlockMutex(music->pause);
    }
}

static void NATIVEMIDI_Stop(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;

    if (music->paused) { /* Unpause if paused */
        music->paused = SDL_FALSE;
        SDL_UnlockMutex(music->pause);
    }

    if (music && NATIVEMIDI_IsPlaying(context)) {
        SDL_AtomicSet(&music->running, 0);
        SDL_WaitThread(music->thread, NULL);
    }
}

static void NATIVEMIDI_Delete(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    NATIVEMIDI_Stop(music);
    if (music->song) {
        midi_seq_free(music->song);
        music->song = NULL;
    }
    NATIVEMIDI_Destroy(music);
}

static const char* NATIVEMIDI_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int NATIVEMIDI_Seek(void *context, double time)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    SDL_LockMutex(music->lock);
    all_notes_off(music);
    midi_seq_seek(music->song, time);
    SDL_UnlockMutex(music->lock);
    return 0;
}

static double NATIVEMIDI_Tell(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double ret;
    if (music) {
        SDL_LockMutex(music->lock);
        ret = midi_seq_tell(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static double NATIVEMIDI_Duration(void *context)
{
    double ret;
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        SDL_LockMutex(music->lock);
        ret = midi_seq_length(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static int NATIVEMIDI_StartTrack(void *context, int track)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        SDL_LockMutex(music->lock);
        midi_switch_song_number(music->song, track);
        reset_midi_device(music->song);
        music->doResetControls = 1;
        NATIVEMIDI_Resume(context);
        SDL_UnlockMutex(music->lock);
        return 0;
    }
    return -1;
}

static int NATIVEMIDI_GetNumTracks(void *context)
{
    int ret = -1;
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        SDL_LockMutex(music->lock);
        ret = midi_get_songs_count(music->song);
        SDL_UnlockMutex(music->lock);
    }
    return ret;
}

static int NATIVEMIDI_SetTempo(void *context, double tempo)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music && (tempo > 0.0)) {
        SDL_LockMutex(music->lock);
        midi_seq_set_tempo_multiplier(music->song, tempo);
        SDL_UnlockMutex(music->lock);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double NATIVEMIDI_GetTempo(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int NATIVEMIDI_GetTracksCount(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        return midi_get_tracks_number(music->song);
    }
    return -1;
}

static int NATIVEMIDI_SetTrackMuted(void *context, int track, int mute)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        return midi_set_track_enabled(music->song, track, mute ? 0 : 1);
    }
    return -1;
}

static double NATIVEMIDI_LoopStart(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double ret;

    if (music) {
        SDL_LockMutex(music->lock);
        ret = midi_seq_loop_start(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static double NATIVEMIDI_LoopEnd(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double ret;

    if (music) {
        SDL_LockMutex(music->lock);
        ret = midi_seq_loop_end(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static double NATIVEMIDI_LoopLength(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double start, end;
    if (music) {
        SDL_LockMutex(music->lock);
        start = midi_seq_loop_start(music->song);
        end = midi_seq_loop_end(music->song);
        SDL_UnlockMutex(music->lock);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}

Mix_MusicInterface Mix_MusicInterface_NATIVEMIDI =
{
    "NATIVEMIDI",
    MIX_MUSIC_NATIVEMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    NATIVEMIDI_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    NATIVEMIDI_SetVolume,
    NATIVEMIDI_GetVolume,   /* GetVolume [MIXER-X]*/
    NATIVEMIDI_Play,
    NATIVEMIDI_IsPlaying,
    NULL,   /* GetAudio */
    NULL,   /* Jump */
    NATIVEMIDI_Seek,   /* Seek */
    NATIVEMIDI_Tell,   /* Tell [MIXER-X]*/
    NATIVEMIDI_Duration,   /* Duration */
    NATIVEMIDI_SetTempo,   /* [MIXER-X] */
    NATIVEMIDI_GetTempo,   /*[MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    NATIVEMIDI_GetTracksCount,   /* [MIXER-X] */
    NATIVEMIDI_SetTrackMuted,   /* [MIXER-X] */
    NATIVEMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    NATIVEMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    NATIVEMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    NATIVEMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NATIVEMIDI_GetNumTracks,
    NATIVEMIDI_StartTrack,
    NATIVEMIDI_Pause,
    NATIVEMIDI_Resume,
    NATIVEMIDI_Stop,
    NATIVEMIDI_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

/* Special case to play XMI/MUS formats */
Mix_MusicInterface Mix_MusicInterface_NATIVEXMI =
{
    "NATIVEXMI",
    MIX_MUSIC_NATIVEMIDI,
    MUS_NATIVEMIDI,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    NATIVEMIDI_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    NATIVEMIDI_SetVolume,
    NATIVEMIDI_GetVolume,   /* GetVolume [MIXER-X]*/
    NATIVEMIDI_Play,
    NATIVEMIDI_IsPlaying,
    NULL,   /* GetAudio */
    NULL,   /* Jump */
    NATIVEMIDI_Seek,   /* Seek */
    NATIVEMIDI_Tell,   /* Tell [MIXER-X]*/
    NATIVEMIDI_Duration,   /* Duration */
    NATIVEMIDI_SetTempo,   /* [MIXER-X] */
    NATIVEMIDI_GetTempo,   /*[MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    NATIVEMIDI_GetTracksCount,   /* [MIXER-X] */
    NATIVEMIDI_SetTrackMuted,   /* [MIXER-X] */
    NATIVEMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    NATIVEMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    NATIVEMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    NATIVEMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NATIVEMIDI_GetNumTracks,
    NATIVEMIDI_StartTrack,
    NATIVEMIDI_Pause,
    NATIVEMIDI_Resume,
    NATIVEMIDI_Stop,
    NATIVEMIDI_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

#endif /* MUSIC_MID_NATIVE */

/* vi: set ts=4 sw=4 expandtab: */
