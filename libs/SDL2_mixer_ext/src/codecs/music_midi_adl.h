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

/* This file supports playing MIDI files with libADLMIDI */

#include "music.h"

extern Mix_MusicInterface Mix_MusicInterface_ADLMIDI;
extern Mix_MusicInterface Mix_MusicInterface_ADLIMF;

#ifdef MUSIC_MID_ADLMIDI

extern int _Mix_ADLMIDI_getTotalBanks(void);

extern const char *const * _Mix_ADLMIDI_getBankNames(void);

extern int _Mix_ADLMIDI_getBankID(void);
extern void _Mix_ADLMIDI_setBankID(int bnk);

extern int _Mix_ADLMIDI_getTremolo(void);
extern void _Mix_ADLMIDI_setTremolo(int tr);

extern int _Mix_ADLMIDI_getVibrato(void);
extern void _Mix_ADLMIDI_setVibrato(int vib);

extern int _Mix_ADLMIDI_getScaleMod(void);
extern void _Mix_ADLMIDI_setScaleMod(int sc);

extern int _Mix_ADLMIDI_getVolumeModel(void);
extern void _Mix_ADLMIDI_setVolumeModel(int vm);

extern int _Mix_ADLMIDI_getFullRangeBrightness(void);
extern void _Mix_ADLMIDI_setFullRangeBrightness(int frb);

extern int _Mix_ADLMIDI_getAutoArpeggio(void);
extern void _Mix_ADLMIDI_setAutoArpeggio(int aaEn);

extern int _Mix_ADLMIDI_getChannelAllocMode();
extern void _Mix_ADLMIDI_setChannelAllocMode(int ch_alloc);

extern int _Mix_ADLMIDI_getFullPanStereo(void);
extern void _Mix_ADLMIDI_setFullPanStereo(int fp);

extern int _Mix_ADLMIDI_getEmulator(void);
extern void _Mix_ADLMIDI_setEmulator(int emu);

extern int _Mix_ADLMIDI_getChipsCount(void);
extern void _Mix_ADLMIDI_setChipsCount(int chips);

extern void _Mix_ADLMIDI_setSetDefaults(void);

extern void _Mix_ADLMIDI_setCustomBankFile(const char *bank_wonl_path);

#endif /* MUSIC_MID_ADLMIDI */

/* vi: set ts=4 sw=4 expandtab: */
