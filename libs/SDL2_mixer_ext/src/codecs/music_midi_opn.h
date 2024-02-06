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

/* This file supports playing MIDI files with libOPNMIDI */

#include "music.h"

extern Mix_MusicInterface Mix_MusicInterface_OPNMIDI;
extern Mix_MusicInterface Mix_MusicInterface_OPNXMI;

#ifdef MUSIC_MID_OPNMIDI

extern int _Mix_OPNMIDI_getVolumeModel(void);
extern void _Mix_OPNMIDI_setVolumeModel(int vm);

extern int _Mix_OPNMIDI_getFullRangeBrightness(void);
extern void _Mix_OPNMIDI_setFullRangeBrightness(int frb);

extern int _Mix_OPNMIDI_getAutoArpeggio(void);
extern void _Mix_OPNMIDI_setAutoArpeggio(int aaEn);

extern int _Mix_OPNMIDI_getChannelAllocMode();
extern void _Mix_OPNMIDI_setChannelAllocMode(int ch_alloc);

extern int _Mix_OPNMIDI_getFullPanStereo(void);
extern void _Mix_OPNMIDI_setFullPanStereo(int fp);

extern int _Mix_OPNMIDI_getEmulator(void);
extern void _Mix_OPNMIDI_setEmulator(int emu);

extern int _Mix_OPNMIDI_getChipsCount(void);
extern void _Mix_OPNMIDI_setChipsCount(int chips);

extern void _Mix_OPNMIDI_setSetDefaults(void);
extern void _Mix_OPNMIDI_setCustomBankFile(const char *bank_wonp_path);

#endif /* MUSIC_MID_OPNMIDI */

/* vi: set ts=4 sw=4 expandtab: */
