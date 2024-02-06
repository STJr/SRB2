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

#ifndef MIX_MIDI_SEQUENCER
#define MIX_MIDI_SEQUENCER

#include "midi_sequencer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *midi_seq_init_interface(BW_MidiRtInterface *iface);
extern void midi_seq_free(void *seq);

extern int midi_seq_openData(void *seq, void *bytes, unsigned long len);
extern int midi_seq_openFile(void *seq, const char *path);

extern const char *midi_seq_meta_title(void *seq);
extern const char *midi_seq_meta_copyright(void *seq);

extern const char *midi_seq_get_error(void *seq);

extern void midi_seq_rewind(void *seq);
extern void midi_seq_seek(void *seq, double pos);
extern double midi_seq_tell(void *seq);
extern double midi_seq_length(void *seq);
extern double midi_seq_loop_start(void *seq);
extern double midi_seq_loop_end(void *seq);
extern int midi_seq_at_end(void *seq);

extern double midi_seq_get_tempo_multiplier(void *seq);
extern void midi_seq_set_tempo_multiplier(void *seq, double tempo);
extern void midi_seq_set_loop_enabled(void *seq, int loopEn);
extern void midi_seq_set_loop_count(void *seq, int loops);

extern double midi_seq_tick(void *seq, double s, double granularity);
extern int midi_seq_play_buffer(void *seq, uint8_t *stream, int len);

extern int midi_get_tracks_number(void *seq);
extern int midi_set_track_enabled(void *seq, int track, int enabled);
extern int midi_set_channel_enabled(void *seq, int channel, int enabled);

extern int midi_get_songs_count(void *seq);
extern void midi_switch_song_number(void *seq, int song);

#ifdef __cplusplus
}
#endif

#endif /* MIX_MIDI_SEQUENCER */
