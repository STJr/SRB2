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

  This file by Vitaly Novichkov (admin@wohlnet.ru).
*/

#include <cassert>
#include "SDL_assert.h"

#define FLAC__ASSERT_H // WORKAROUND
#ifdef assert
#   undef assert
#endif
#define assert SDL_assert

// Rename class to avoid ABI collisions
#define BW_MidiSequencer MixerMidiSequencer
// Inlucde MIDI sequencer class implementation
#include "midi_sequencer_impl.hpp"

#include "mix_midi_seq.h"


class MixerSeqInternal
{
public:
    MixerSeqInternal() :
        seq(),
        seq_if(NULL)
    {}
    ~MixerSeqInternal()
    {
        if(seq_if)
            delete seq_if;
    }

    MixerMidiSequencer seq;
    BW_MidiRtInterface *seq_if;
};


void *midi_seq_init_interface(BW_MidiRtInterface *iface)
{
    MixerSeqInternal *intSeq = new MixerSeqInternal;
    intSeq->seq_if = new BW_MidiRtInterface;
    std::memcpy(intSeq->seq_if, iface, sizeof(BW_MidiRtInterface));
    intSeq->seq.setInterface(intSeq->seq_if);

    return intSeq;
}


void midi_seq_free(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    delete seqi;
}

int midi_seq_openData(void *seq, void *bytes, unsigned long len)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    bool ret = seqi->seq.loadMIDI(bytes, static_cast<size_t>(len));
    return ret ? 0 : -1;
}

int midi_seq_openFile(void *seq, const char *path)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    bool ret = seqi->seq.loadMIDI(path);
    return ret ? 0 : -1;
}


const char *midi_seq_meta_title(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.getMusicTitle().c_str();
}

const char *midi_seq_meta_copyright(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.getMusicCopyright().c_str();
}

const char *midi_seq_get_error(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.getErrorString().c_str();
}

void midi_seq_rewind(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    seqi->seq.rewind();
}


void midi_seq_seek(void *seq, double pos)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    seqi->seq.seek(pos, 1);
}

double midi_seq_tell(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.tell();
}

double midi_seq_length(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.timeLength();
}

double midi_seq_loop_start(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.getLoopStart();
}

double midi_seq_loop_end(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.getLoopEnd();
}

int midi_seq_at_end(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.positionAtEnd() ? 1 : 0;
}


double midi_seq_get_tempo_multiplier(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.getTempoMultiplier();
}

void midi_seq_set_tempo_multiplier(void *seq, double tempo)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    seqi->seq.setTempo(tempo);
}

void midi_seq_set_loop_enabled(void *seq, int loopEn)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    seqi->seq.setLoopEnabled(loopEn > 0);
}

void midi_seq_set_loop_count(void *seq, int loops)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    seqi->seq.setLoopsCount(loops);
}

double midi_seq_tick(void *seq, double s, double granularity)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    double ret = seqi->seq.Tick(s, granularity);
    s *= seqi->seq.getTempoMultiplier();
    return ret;
}

int midi_seq_play_buffer(void *seq, uint8_t *stream, int len)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return seqi->seq.playStream(stream, len);
}

int midi_get_tracks_number(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return (int)seqi->seq.getTrackCount();
}

int midi_set_track_enabled(void *seq, int track, int enabled)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return (int)seqi->seq.setTrackEnabled(track, enabled);
}

int midi_set_channel_enabled(void *seq, int channel, int enabled)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return (int)seqi->seq.setChannelEnabled(channel, enabled);
}

int midi_get_songs_count(void *seq)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    return (int)seqi->seq.getSongsCount();
}

void midi_switch_song_number(void *seq, int song)
{
    MixerSeqInternal *seqi = reinterpret_cast<MixerSeqInternal*>(seq);
    seqi->seq.setSongNum(song);
}
