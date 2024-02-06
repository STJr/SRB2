/*
 * BW_Midi_Sequencer - MIDI Sequencer for C++
 *
 * Copyright (c) 2015-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once
#ifndef BW_MIDI_SEQUENCER_HHHH
#define BW_MIDI_SEQUENCER_HHHH

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/*! Raw MIDI event hook */
typedef void (*RawEventHook)(void *userdata, uint8_t type, uint8_t subtype, uint8_t channel, const uint8_t *data, size_t len);
/*! PCM render */
typedef void (*PcmRender)(void *userdata, uint8_t *stream, size_t length);
/*! Library internal debug messages */
typedef void (*DebugMessageHook)(void *userdata, const char *fmt, ...);
/*! Loop Start event hook */
typedef void (*LoopStartHook)(void *userdata);
/*! Loop Start event hook */
typedef void (*LoopEndHook)(void *userdata);
typedef void (*SongStartHook)(void *userdata);


/*! Note-On MIDI event */
typedef void (*RtNoteOn)(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity);
/*! Note-Off MIDI event */
typedef void (*RtNoteOff)(void *userdata, uint8_t channel, uint8_t note);
/*! Note-Off MIDI event with a velocity */
typedef void (*RtNoteOffVel)(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity);
/*! Note aftertouch MIDI event */
typedef void (*RtNoteAfterTouch)(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal);
/*! Channel aftertouch MIDI event */
typedef void (*RtChannelAfterTouch)(void *userdata, uint8_t channel, uint8_t atVal);
/*! Controller change MIDI event */
typedef void (*RtControllerChange)(void *userdata, uint8_t channel, uint8_t type, uint8_t value);
/*! Patch change MIDI event */
typedef void (*RtPatchChange)(void *userdata, uint8_t channel, uint8_t patch);
/*! Pitch bend MIDI event */
typedef void (*RtPitchBend)(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb);
/*! System Exclusive MIDI event */
typedef void (*RtSysEx)(void *userdata, const uint8_t *msg, size_t size);
/*! Meta event hook */
typedef void (*MetaEventHook)(void *userdata, uint8_t type, const uint8_t *data, size_t len);
/*! Device Switch MIDI event */
typedef void (*RtDeviceSwitch)(void *userdata, size_t track, const char *data, size_t length);
/*! Get the channels offset for current MIDI device */
typedef size_t (*RtCurrentDevice)(void *userdata, size_t track);
/*! [Non-Standard] Pass raw OPL3 data to the chip (when playing IMF files) */
typedef void (*RtRawOPL)(void *userdata, uint8_t reg, uint8_t value);

/**
  \brief Real-Time MIDI interface between Sequencer and the Synthesizer
 */
typedef struct BW_MidiRtInterface
{
    /*! MIDI event hook which catches all MIDI events */
    RawEventHook onEvent;
    /*! User data which will be passed through On-Event hook */
    void         *onEvent_userData;

    /*! PCM render hook which catches passing of loop start point */
    PcmRender    onPcmRender;
    /*! User data which will be passed through On-PCM-render hook */
    void         *onPcmRender_userData;

    /*! Sample rate */
    uint32_t pcmSampleRate;

    /*! Size of one sample in bytes */
    uint32_t pcmFrameSize;

    /*! Debug message hook */
    DebugMessageHook onDebugMessage;
    /*! User data which will be passed through Debug Message hook */
    void *onDebugMessage_userData;

    /*! Loop start hook which catches passing of loop start point */
    LoopStartHook onloopStart;
    /*! User data which will be passed through On-LoopStart hook */
    void         *onloopStart_userData;

    /*! Loop start hook which catches passing of loop start point */
    LoopEndHook   onloopEnd;
    /*! User data which will be passed through On-LoopStart hook */
    void         *onloopEnd_userData;

    /*! Song start hook which is calling when starting playing song at begin */
    SongStartHook onSongStart;
    /*! User data which will be passed through On-SongStart hook */
    void         *onSongStart_userData;

    /*! MIDI Run Time event calls user data */
    void *rtUserData;


    /***************************************************
     * Standard MIDI events. All of them are required! *
     ***************************************************/

    /*! Note-On MIDI event hook */
    RtNoteOn            rt_noteOn;
    /*! Note-Off MIDI event hook */
    RtNoteOff           rt_noteOff;

    /*! Note-Off MIDI event hook with a velocity */
    RtNoteOffVel        rt_noteOffVel;

    /*! Note aftertouch MIDI event hook */
    RtNoteAfterTouch    rt_noteAfterTouch;

    /*! Channel aftertouch MIDI event hook */
    RtChannelAfterTouch rt_channelAfterTouch;

    /*! Controller change MIDI event hook */
    RtControllerChange   rt_controllerChange;

    /*! Patch change MIDI event hook */
    RtPatchChange       rt_patchChange;

    /*! Pitch bend MIDI event hook */
    RtPitchBend         rt_pitchBend;

    /*! System Exclusive MIDI event hook */
    RtSysEx             rt_systemExclusive;


    /*******************
     * Optional events *
     *******************/

    /*! Meta event hook which catches all meta events */
    MetaEventHook       rt_metaEvent;

    /*! Device Switch MIDI event hook */
    RtDeviceSwitch      rt_deviceSwitch;

    /*! Get the channels offset for current MIDI device hook. Returms multiple to 16 value. */
    RtCurrentDevice     rt_currentDevice;


    /******************************************
     * NonStandard events. There are optional *
     ******************************************/
    /*! [Non-Standard] Pass raw OPL3 data to the chip hook */
    RtRawOPL            rt_rawOPL;

} BW_MidiRtInterface;

#ifdef __cplusplus
}
#endif

#endif /* BW_MIDI_SEQUENCER_HHHH */
