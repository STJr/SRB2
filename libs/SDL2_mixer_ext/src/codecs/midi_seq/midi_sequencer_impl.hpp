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

#include "midi_sequencer.hpp"
#include <stdio.h>
#include <memory>
#include <cstring>
#include <cerrno>
#include <iterator>  // std::back_inserter
#include <algorithm> // std::copy
#include <set>
#include <assert.h>

#if defined(VITA)
#define timingsafe_memcmp  timingsafe_memcmp_workaround // Workaround to fix the C declaration conflict
#include <psp2kern/kernel/sysclib.h> // snprintf
#endif

#if defined(_WIN32) && !defined(__WATCOMC__)
#   ifdef _MSC_VER
#       ifdef _WIN64
typedef __int64 ssize_t;
#       else
typedef __int32 ssize_t;
#       endif
#   else
#       ifdef _WIN64
typedef int64_t ssize_t;
#       else
typedef int32_t ssize_t;
#       endif
#   endif
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}
#endif

#ifndef BWMIDI_DISABLE_MUS_SUPPORT
#include "cvt_mus2mid.hpp"
#endif // MUS

#ifndef BWMIDI_DISABLE_XMI_SUPPORT
#include "cvt_xmi2mid.hpp"
#endif // XMI

/**
 * @brief Utility function to read Big-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted unsigned integer
 */
static inline uint64_t readBEint(const void *buffer, size_t nbytes)
{
    uint64_t result = 0;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buffer);

    for(size_t n = 0; n < nbytes; ++n)
        result = (result << 8) + data[n];

    return result;
}

/**
 * @brief Utility function to read Little-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted unsigned integer
 */
static inline uint64_t readLEint(const void *buffer, size_t nbytes)
{
    uint64_t result = 0;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buffer);

    for(size_t n = 0; n < nbytes; ++n)
        result = result + static_cast<uint64_t>(data[n] << (n * 8));

    return result;
}

/**
 * @brief Secure Standard MIDI Variable-Length numeric value parser with anti-out-of-range protection
 * @param [_inout] ptr Pointer to memory block that contains begin of variable-length value, will be iterated forward
 * @param [_in end Pointer to end of memory block where variable-length value is stored (after end of track)
 * @param [_out] ok Reference to boolean which takes result of variable-length value parsing
 * @return Unsigned integer that conains parsed variable-length value
 */
static inline uint64_t readVarLenEx(const uint8_t **ptr, const uint8_t *end, bool &ok)
{
    uint64_t result = 0;
    ok = false;

    for(;;)
    {
        if(*ptr >= end)
            return 2;
        unsigned char byte = *((*ptr)++);
        result = (result << 7) + (byte & 0x7F);
        if(!(byte & 0x80))
            break;
    }

    ok = true;
    return result;
}

BW_MidiSequencer::MidiEvent::MidiEvent() :
    type(T_UNKNOWN),
    subtype(T_UNKNOWN),
    channel(0),
    isValid(1),
    absPosition(0)
{}

BW_MidiSequencer::MidiTrackRow::MidiTrackRow() :
    time(0.0),
    delay(0),
    absPos(0),
    timeDelay(0.0)
{}

void BW_MidiSequencer::MidiTrackRow::clear()
{
    time = 0.0;
    delay = 0;
    absPos = 0;
    timeDelay = 0.0;
    events.clear();
}

void BW_MidiSequencer::MidiTrackRow::sortEvents(bool *noteStates)
{
    typedef std::vector<MidiEvent> EvtArr;
    EvtArr sysEx;
    EvtArr metas;
    EvtArr noteOffs;
    EvtArr controllers;
    EvtArr anyOther;

    for(size_t i = 0; i < events.size(); i++)
    {
        if(events[i].type == MidiEvent::T_NOTEOFF)
        {
            if(noteOffs.capacity() == 0)
                noteOffs.reserve(events.size());
            noteOffs.push_back(events[i]);
        }
        else if(events[i].type == MidiEvent::T_SYSEX ||
                events[i].type == MidiEvent::T_SYSEX2)
        {
            if(sysEx.capacity() == 0)
                sysEx.reserve(events.size());
            sysEx.push_back(events[i]);
        }
        else if((events[i].type == MidiEvent::T_CTRLCHANGE)
                || (events[i].type == MidiEvent::T_PATCHCHANGE)
                || (events[i].type == MidiEvent::T_WHEEL)
                || (events[i].type == MidiEvent::T_CHANAFTTOUCH))
        {
            if(controllers.capacity() == 0)
                controllers.reserve(events.size());
            controllers.push_back(events[i]);
        }
        else if((events[i].type == MidiEvent::T_SPECIAL) && (
            (events[i].subtype == MidiEvent::ST_MARKER) ||
            (events[i].subtype == MidiEvent::ST_DEVICESWITCH) ||
            (events[i].subtype == MidiEvent::ST_SONG_BEGIN_HOOK) ||
            (events[i].subtype == MidiEvent::ST_LOOPSTART) ||
            (events[i].subtype == MidiEvent::ST_LOOPEND) ||
            (events[i].subtype == MidiEvent::ST_LOOPSTACK_BEGIN) ||
            (events[i].subtype == MidiEvent::ST_LOOPSTACK_END) ||
            (events[i].subtype == MidiEvent::ST_LOOPSTACK_BREAK)
            ))
        {
            if(metas.capacity() == 0)
                metas.reserve(events.size());
            metas.push_back(events[i]);
        }
        else
        {
            if(anyOther.capacity() == 0)
                anyOther.reserve(events.size());
            anyOther.push_back(events[i]);
        }
    }

    /*
     * If Note-Off and it's Note-On is on the same row - move this damned note off down!
     */
    if(noteStates)
    {
        std::set<size_t> markAsOn;
        for(size_t i = 0; i < anyOther.size(); i++)
        {
            const MidiEvent e = anyOther[i];
            if(e.type == MidiEvent::T_NOTEON)
            {
                const size_t note_i = static_cast<size_t>(e.channel * 255) + (e.data[0] & 0x7F);
                //Check, was previously note is on or off
                bool wasOn = noteStates[note_i];
                markAsOn.insert(note_i);
                // Detect zero-length notes are following previously pressed note
                int noteOffsOnSameNote = 0;
                for(EvtArr::iterator j = noteOffs.begin(); j != noteOffs.end();)
                {
                    // If note was off, and note-off on same row with note-on - move it down!
                    if(
                        ((*j).channel == e.channel) &&
                        ((*j).data[0] == e.data[0])
                    )
                    {
                        // If note is already off OR more than one note-off on same row and same note
                        if(!wasOn || (noteOffsOnSameNote != 0))
                        {
                            anyOther.push_back(*j);
                            j = noteOffs.erase(j);
                            markAsOn.erase(note_i);
                            continue;
                        }
                        else
                        {
                            // When same row has many note-offs on same row
                            // that means a zero-length note follows previous note
                            // it must be shuted down
                            noteOffsOnSameNote++;
                        }
                    }
                    j++;
                }
            }
        }

        // Mark other notes as released
        for(EvtArr::iterator j = noteOffs.begin(); j != noteOffs.end(); j++)
        {
            size_t note_i = static_cast<size_t>(j->channel * 255) + (j->data[0] & 0x7F);
            noteStates[note_i] = false;
        }

        for(std::set<size_t>::iterator j = markAsOn.begin(); j != markAsOn.end(); j++)
            noteStates[*j] = true;
    }
    /***********************************************************************************/

    events.clear();
    if(!sysEx.empty())
        events.insert(events.end(), sysEx.begin(), sysEx.end());
    if(!noteOffs.empty())
        events.insert(events.end(), noteOffs.begin(), noteOffs.end());
    if(!metas.empty())
        events.insert(events.end(), metas.begin(), metas.end());
    if(!controllers.empty())
        events.insert(events.end(), controllers.begin(), controllers.end());
    if(!anyOther.empty())
        events.insert(events.end(), anyOther.begin(), anyOther.end());
}

BW_MidiSequencer::BW_MidiSequencer() :
    m_interface(NULL),
    m_format(Format_MIDI),
    m_smfFormat(0),
    m_loopFormat(Loop_Default),
    m_loopEnabled(false),
    m_loopHooksOnly(false),
    m_fullSongTimeLength(0.0),
    m_postSongWaitDelay(1.0),
    m_loopStartTime(-1.0),
    m_loopEndTime(-1.0),
    m_tempoMultiplier(1.0),
    m_atEnd(false),
    m_loopCount(-1),
    m_loadTrackNumber(0),
    m_trackSolo(~static_cast<size_t>(0)),
    m_triggerHandler(NULL),
    m_triggerUserData(NULL)
{
    m_loop.reset();
    m_loop.invalidLoop = false;
    m_time.init();
}

BW_MidiSequencer::~BW_MidiSequencer()
{}

void BW_MidiSequencer::setInterface(const BW_MidiRtInterface *intrf)
{
    // Interface must NOT be NULL
    assert(intrf);

    // Note ON hook is REQUIRED
    assert(intrf->rt_noteOn);
    // Note OFF hook is REQUIRED
    assert(intrf->rt_noteOff || intrf->rt_noteOffVel);
    // Note Aftertouch hook is REQUIRED
    assert(intrf->rt_noteAfterTouch);
    // Channel Aftertouch hook is REQUIRED
    assert(intrf->rt_channelAfterTouch);
    // Controller change hook is REQUIRED
    assert(intrf->rt_controllerChange);
    // Patch change hook is REQUIRED
    assert(intrf->rt_patchChange);
    // Pitch bend hook is REQUIRED
    assert(intrf->rt_pitchBend);
    // System Exclusive hook is REQUIRED
    assert(intrf->rt_systemExclusive);

    if(intrf->pcmSampleRate != 0 && intrf->pcmFrameSize != 0)
    {
        m_time.sampleRate = intrf->pcmSampleRate;
        m_time.frameSize = intrf->pcmFrameSize;
        m_time.reset();
    }

    m_interface = intrf;
}

int BW_MidiSequencer::playStream(uint8_t *stream, size_t length)
{
    int count = 0;
    size_t samples = static_cast<size_t>(length / static_cast<size_t>(m_time.frameSize));
    size_t left = samples;
    size_t periodSize = 0;
    uint8_t *stream_pos = stream;

    assert(m_interface->onPcmRender);

    while(left > 0)
    {
        const double leftDelay = left / double(m_time.sampleRate);
        const double maxDelay = m_time.timeRest < leftDelay ? m_time.timeRest : leftDelay;
        if((positionAtEnd()) && (m_time.delay <= 0.0))
            break; // Stop to fetch samples at reaching the song end with disabled loop

        m_time.timeRest -= maxDelay;
        periodSize = static_cast<size_t>(static_cast<double>(m_time.sampleRate) * maxDelay);

        if(stream)
        {
            size_t generateSize = periodSize > left ? static_cast<size_t>(left) : static_cast<size_t>(periodSize);
            m_interface->onPcmRender(m_interface->onPcmRender_userData, stream_pos, generateSize * m_time.frameSize);
            stream_pos += generateSize * m_time.frameSize;
            count += generateSize;
            left -= generateSize;
            assert(left <= samples);
        }

        if(m_time.timeRest <= 0.0)
        {
            m_time.delay = Tick(m_time.delay, m_time.minDelay);
            m_time.timeRest += m_time.delay;
        }
    }

    return count * static_cast<int>(m_time.frameSize);
}

BW_MidiSequencer::FileFormat BW_MidiSequencer::getFormat()
{
    return m_format;
}

size_t BW_MidiSequencer::getTrackCount() const
{
    return m_trackData.size();
}

bool BW_MidiSequencer::setTrackEnabled(size_t track, bool enable)
{
    size_t trackCount = m_trackData.size();
    if(track >= trackCount)
        return false;
    m_trackDisable[track] = !enable;
    return true;
}

bool BW_MidiSequencer::setChannelEnabled(size_t channel, bool enable)
{
    if(channel >= 16)
        return false;

    if(!enable && m_channelDisable[channel] != !enable)
    {
        uint8_t ch = static_cast<uint8_t>(channel);

        // Releae all pedals
        m_interface->rt_controllerChange(m_interface->rtUserData, ch, 64, 0);
        m_interface->rt_controllerChange(m_interface->rtUserData, ch, 66, 0);

        // Release all notes on the channel now
        for(int i = 0; i < 127; ++i)
        {
            if(m_interface->rt_noteOff)
                m_interface->rt_noteOff(m_interface->rtUserData, ch, i);
            if(m_interface->rt_noteOffVel)
                m_interface->rt_noteOffVel(m_interface->rtUserData, ch, i, 0);
        }
    }

    m_channelDisable[channel] = !enable;
    return true;
}

void BW_MidiSequencer::setSoloTrack(size_t track)
{
    m_trackSolo = track;
}

void BW_MidiSequencer::setSongNum(int track)
{
    m_loadTrackNumber = track;

    if(!m_rawSongsData.empty() && m_format == Format_XMIDI) // Reload the song
    {
        if(m_loadTrackNumber >= (int)m_rawSongsData.size())
            m_loadTrackNumber = m_rawSongsData.size() - 1;

        if(m_interface && m_interface->rt_controllerChange)
        {
            for(int i = 0; i < 15; i++)
                m_interface->rt_controllerChange(m_interface->rtUserData, i, 123, 0);
        }

        m_atEnd            = false;
        m_loop.fullReset();
        m_loop.caughtStart = true;

        m_smfFormat = 0;

        FileAndMemReader fr;
        fr.openData(m_rawSongsData[m_loadTrackNumber].data(),
                    m_rawSongsData[m_loadTrackNumber].size());
        parseSMF(fr);

        m_format = Format_XMIDI;
    }
}

int BW_MidiSequencer::getSongsCount()
{
    return (int)m_rawSongsData.size();
}


void BW_MidiSequencer::setTriggerHandler(TriggerHandler handler, void *userData)
{
    m_triggerHandler = handler;
    m_triggerUserData = userData;
}

const std::vector<BW_MidiSequencer::CmfInstrument> BW_MidiSequencer::getRawCmfInstruments()
{
    return m_cmfInstruments;
}

const std::string &BW_MidiSequencer::getErrorString()
{
    return m_errorString;
}

bool BW_MidiSequencer::getLoopEnabled()
{
    return m_loopEnabled;
}

void BW_MidiSequencer::setLoopEnabled(bool enabled)
{
    m_loopEnabled = enabled;
}

int BW_MidiSequencer::getLoopsCount()
{
    return m_loopCount >= 0 ? (m_loopCount + 1) : m_loopCount;
}

void BW_MidiSequencer::setLoopsCount(int loops)
{
    if(loops >= 1)
        loops -= 1; // Internally, loops count has the 0 base
    m_loopCount = loops;
}

void BW_MidiSequencer::setLoopHooksOnly(bool enabled)
{
    m_loopHooksOnly = enabled;
}

const std::string &BW_MidiSequencer::getMusicTitle()
{
    return m_musTitle;
}

const std::string &BW_MidiSequencer::getMusicCopyright()
{
    return m_musCopyright;
}

const std::vector<std::string> &BW_MidiSequencer::getTrackTitles()
{
    return m_musTrackTitles;
}

const std::vector<BW_MidiSequencer::MIDI_MarkerEntry> &BW_MidiSequencer::getMarkers()
{
    return m_musMarkers;
}

bool BW_MidiSequencer::positionAtEnd()
{
    return m_atEnd;
}

double BW_MidiSequencer::getTempoMultiplier()
{
    return m_tempoMultiplier;
}


void BW_MidiSequencer::buildSmfSetupReset(size_t trackCount)
{
    m_fullSongTimeLength = 0.0;
    m_loopStartTime = -1.0;
    m_loopEndTime = -1.0;
    m_loopFormat = Loop_Default;
    m_trackDisable.clear();
    std::memset(m_channelDisable, 0, sizeof(m_channelDisable));
    m_trackSolo = ~(size_t)0;
    m_musTitle.clear();
    m_musCopyright.clear();
    m_musTrackTitles.clear();
    m_musMarkers.clear();
    m_trackData.clear();
    m_trackData.resize(trackCount, MidiTrackQueue());
    m_trackDisable.resize(trackCount);

    m_loop.reset();
    m_loop.invalidLoop = false;
    m_time.reset();

    m_currentPosition.began = false;
    m_currentPosition.absTimePosition = 0.0;
    m_currentPosition.wait = 0.0;
    m_currentPosition.track.clear();
    m_currentPosition.track.resize(trackCount);
}

bool BW_MidiSequencer::buildSmfTrackData(const std::vector<std::vector<uint8_t> > &trackData)
{
    const size_t trackCount = trackData.size();
    buildSmfSetupReset(trackCount);

    bool gotGlobalLoopStart = false,
         gotGlobalLoopEnd = false,
         gotStackLoopStart = false,
         gotLoopEventInThisRow = false;

    //! Tick position of loop start tag
    uint64_t loopStartTicks = 0;
    //! Tick position of loop end tag
    uint64_t loopEndTicks = 0;
    //! Full length of song in ticks
    uint64_t ticksSongLength = 0;
    //! Cache for error message strign
    char error[150];

    //! Caches note on/off states.
    bool noteStates[16 * 255];
    /* This is required to carefully detect zero-length notes           *
     * and avoid a move of "note-off" event over "note-on" while sort.  *
     * Otherwise, after sort those notes will play infinite sound       */

    //! Tempo change events list
    std::vector<MidiEvent> temposList;

    /*
     * TODO: Make this be safer for memory in case of broken input data
     * which may cause going away of available track data (and then give a crash!)
     *
     * POST: Check this more carefully for possible vulnuabilities are can crash this
     */
    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        uint64_t abs_position = 0;
        int status = 0;
        MidiEvent event;
        bool ok = false;
        const uint8_t *end      = trackData[tk].data() + trackData[tk].size();
        const uint8_t *trackPtr = trackData[tk].data();
        std::memset(noteStates, 0, sizeof(noteStates));

        // Time delay that follows the first event in the track
        {
            MidiTrackRow evtPos;
            if(m_format == Format_RSXX)
                ok = true;
            else
                evtPos.delay = readVarLenEx(&trackPtr, end, ok);
            if(!ok)
            {
                int len = snprintf(error, 150, "buildTrackData: Can't read variable-length value at begin of track %d.\n", (int)tk);
                if((len > 0) && (len < 150))
                    m_parsingErrorsString += std::string(error, (size_t)len);
                return false;
            }

            // HACK: Begin every track with "Reset all controllers" event to avoid controllers state break came from end of song
            if(tk == 0)
            {
                MidiEvent resetEvent;
                resetEvent.type = MidiEvent::T_SPECIAL;
                resetEvent.subtype = MidiEvent::ST_SONG_BEGIN_HOOK;
                evtPos.events.push_back(resetEvent);
            }

            evtPos.absPos = abs_position;
            abs_position += evtPos.delay;
            m_trackData[tk].push_back(evtPos);
        }

        MidiTrackRow evtPos;
        do
        {
            event = parseEvent(&trackPtr, end, status);
            if(!event.isValid)
            {
                int len = snprintf(error, 150, "buildTrackData: Fail to parse event in the track %d.\n", (int)tk);
                if((len > 0) && (len < 150))
                    m_parsingErrorsString += std::string(error, (size_t)len);
                return false;
            }

            evtPos.events.push_back(event);
            if(event.type == MidiEvent::T_SPECIAL)
            {
                if(event.subtype == MidiEvent::ST_TEMPOCHANGE)
                {
                    event.absPosition = abs_position;
                    temposList.push_back(event);
                }
                else if(!m_loop.invalidLoop && (event.subtype == MidiEvent::ST_LOOPSTART))
                {
                    /*
                     * loopStart is invalid when:
                     * - starts together with loopEnd
                     * - appears more than one time in same MIDI file
                     */
                    if(gotGlobalLoopStart || gotLoopEventInThisRow)
                        m_loop.invalidLoop = true;
                    else
                    {
                        gotGlobalLoopStart = true;
                        loopStartTicks = abs_position;
                    }
                    // In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
                else if(!m_loop.invalidLoop && (event.subtype == MidiEvent::ST_LOOPEND))
                {
                    /*
                     * loopEnd is invalid when:
                     * - starts before loopStart
                     * - starts together with loopStart
                     * - appars more than one time in same MIDI file
                     */
                    if(gotGlobalLoopEnd || gotLoopEventInThisRow)
                    {
                        m_loop.invalidLoop = true;
                        if(m_interface->onDebugMessage)
                        {
                            m_interface->onDebugMessage(
                                m_interface->onDebugMessage_userData,
                                "== Invalid loop detected! %s %s ==",
                                (gotGlobalLoopEnd ? "[Caught more than 1 loopEnd!]" : ""),
                                (gotLoopEventInThisRow ? "[loopEnd in same row as loopStart!]" : "")
                            );
                        }
                    }
                    else
                    {
                        gotGlobalLoopEnd = true;
                        loopEndTicks = abs_position;
                    }
                    // In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
                else if(!m_loop.invalidLoop && (event.subtype == MidiEvent::ST_LOOPSTACK_BEGIN))
                {
                    if(!gotStackLoopStart)
                    {
                        if(!gotGlobalLoopStart)
                            loopStartTicks = abs_position;
                        gotStackLoopStart = true;
                    }

                    m_loop.stackUp();
                    if(m_loop.stackLevel >= static_cast<int>(m_loop.stack.size()))
                    {
                        LoopStackEntry e;
                        e.loops = event.data[0];
                        e.infinity = (event.data[0] == 0);
                        e.start = abs_position;
                        e.end = abs_position;
                        m_loop.stack.push_back(e);
                    }
                }
                else if(!m_loop.invalidLoop &&
                    ((event.subtype == MidiEvent::ST_LOOPSTACK_END) ||
                     (event.subtype == MidiEvent::ST_LOOPSTACK_BREAK))
                )
                {
                    if(m_loop.stackLevel <= -1)
                    {
                        m_loop.invalidLoop = true; // Caught loop end without of loop start!
                        if(m_interface->onDebugMessage)
                        {
                            m_interface->onDebugMessage(
                                m_interface->onDebugMessage_userData,
                                "== Invalid loop detected! [Caught loop end without of loop start] =="
                            );
                        }
                    }
                    else
                    {
                        if(loopEndTicks < abs_position)
                            loopEndTicks = abs_position;
                        m_loop.getCurStack().end = abs_position;
                        m_loop.stackDown();
                    }
                }
            }

            if(event.subtype != MidiEvent::ST_ENDTRACK) // Don't try to read delta after EndOfTrack event!
            {
                evtPos.delay = readVarLenEx(&trackPtr, end, ok);
                if(!ok)
                {
                    /* End of track has been reached! However, there is no EOT event presented */
                    event.type = MidiEvent::T_SPECIAL;
                    event.subtype = MidiEvent::ST_ENDTRACK;
                }
            }

#ifdef ENABLE_END_SILENCE_SKIPPING
            //Have track end on its own row? Clear any delay on the row before
            if(event.subtype == MidiEvent::ST_ENDTRACK && evtPos.events.size() == 1)
            {
                if (!m_trackData[tk].empty())
                {
                    MidiTrackRow &previous = m_trackData[tk].back();
                    previous.delay = 0;
                    previous.timeDelay = 0;
                }
            }
#endif

            if((evtPos.delay > 0) || (event.subtype == MidiEvent::ST_ENDTRACK))
            {
                evtPos.absPos = abs_position;
                abs_position += evtPos.delay;
                evtPos.sortEvents(noteStates);
                m_trackData[tk].push_back(evtPos);
                evtPos.clear();
                gotLoopEventInThisRow = false;
            }
        }
        while((trackPtr <= end) && (event.subtype != MidiEvent::ST_ENDTRACK));

        if(ticksSongLength < abs_position)
            ticksSongLength = abs_position;
        // Set the chain of events begin
        if(m_trackData[tk].size() > 0)
            m_currentPosition.track[tk].pos = m_trackData[tk].begin();
    }

    if(gotGlobalLoopStart && !gotGlobalLoopEnd)
    {
        gotGlobalLoopEnd = true;
        loopEndTicks = ticksSongLength;
    }

    // loopStart must be located before loopEnd!
    if(loopStartTicks >= loopEndTicks)
    {
        m_loop.invalidLoop = true;
        if(m_interface->onDebugMessage && (gotGlobalLoopStart || gotGlobalLoopEnd))
        {
            m_interface->onDebugMessage(
                m_interface->onDebugMessage_userData,
                "== Invalid loop detected! [loopEnd is going before loopStart] =="
            );
        }
    }

    buildTimeLine(temposList, loopStartTicks, loopEndTicks);

    return true;
}

void BW_MidiSequencer::buildTimeLine(const std::vector<MidiEvent> &tempos,
                                          uint64_t loopStartTicks,
                                          uint64_t loopEndTicks)
{
    const size_t    trackCount = m_trackData.size();
    /********************************************************************************/
    // Calculate time basing on collected tempo events
    /********************************************************************************/
    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        fraction<uint64_t> currentTempo = m_tempo;
        double  time = 0.0;
        // uint64_t abs_position = 0;
        size_t tempo_change_index = 0;
        MidiTrackQueue &track = m_trackData[tk];
        if(track.empty())
            continue;//Empty track is useless!

#ifdef DEBUG_TIME_CALCULATION
        std::fprintf(stdout, "\n============Track %" PRIuPTR "=============\n", tk);
        std::fflush(stdout);
#endif

        MidiTrackRow *posPrev = &(*(track.begin()));//First element
        for(MidiTrackQueue::iterator it = track.begin(); it != track.end(); it++)
        {
#ifdef DEBUG_TIME_CALCULATION
            bool tempoChanged = false;
#endif
            MidiTrackRow &pos = *it;
            if((posPrev != &pos) && // Skip first event
               (!tempos.empty()) && // Only when in-track tempo events are available
               (tempo_change_index < tempos.size())
              )
            {
                // If tempo event is going between of current and previous event
                if(tempos[tempo_change_index].absPosition <= pos.absPos)
                {
                    // Stop points: begin point and tempo change points are before end point
                    std::vector<TempoChangePoint> points;
                    fraction<uint64_t> t;
                    TempoChangePoint firstPoint = {posPrev->absPos, currentTempo};
                    points.push_back(firstPoint);

                    // Collect tempo change points between previous and current events
                    do
                    {
                        TempoChangePoint tempoMarker;
                        const MidiEvent &tempoPoint = tempos[tempo_change_index];
                        tempoMarker.absPos = tempoPoint.absPosition;
                        tempoMarker.tempo = m_invDeltaTicks * fraction<uint64_t>(readBEint(tempoPoint.data.data(), tempoPoint.data.size()));
                        points.push_back(tempoMarker);
                        tempo_change_index++;
                    }
                    while((tempo_change_index < tempos.size()) &&
                          (tempos[tempo_change_index].absPosition <= pos.absPos));

                    // Re-calculate time delay of previous event
                    time -= posPrev->timeDelay;
                    posPrev->timeDelay = 0.0;

                    for(size_t i = 0, j = 1; j < points.size(); i++, j++)
                    {
                        /* If one or more tempo events are appears between of two events,
                         * calculate delays between each tempo point, begin and end */
                        uint64_t midDelay = 0;
                        // Delay between points
                        midDelay  = points[j].absPos - points[i].absPos;
                        // Time delay between points
                        t = midDelay * currentTempo;
                        posPrev->timeDelay += t.value();

                        // Apply next tempo
                        currentTempo = points[j].tempo;
#ifdef DEBUG_TIME_CALCULATION
                        tempoChanged = true;
#endif
                    }
                    // Then calculate time between last tempo change point and end point
                    TempoChangePoint tailTempo = points.back();
                    uint64_t postDelay = pos.absPos - tailTempo.absPos;
                    t = postDelay * currentTempo;
                    posPrev->timeDelay += t.value();

                    // Store Common time delay
                    posPrev->time = time;
                    time += posPrev->timeDelay;
                }
            }

            fraction<uint64_t> t = pos.delay * currentTempo;
            pos.timeDelay = t.value();
            pos.time = time;
            time += pos.timeDelay;

            // Capture markers after time value calculation
            for(size_t i = 0; i < pos.events.size(); i++)
            {
                MidiEvent &e = pos.events[i];
                if((e.type == MidiEvent::T_SPECIAL) && (e.subtype == MidiEvent::ST_MARKER))
                {
                    MIDI_MarkerEntry marker;
                    marker.label = std::string((char *)e.data.data(), e.data.size());
                    marker.pos_ticks = pos.absPos;
                    marker.pos_time = pos.time;
                    m_musMarkers.push_back(marker);
                }
            }

            // Capture loop points time positions
            if(!m_loop.invalidLoop)
            {
                // Set loop points times
                if(loopStartTicks == pos.absPos)
                    m_loopStartTime = pos.time;
                else if(loopEndTicks == pos.absPos)
                    m_loopEndTime = pos.time;
            }

#ifdef DEBUG_TIME_CALCULATION
            std::fprintf(stdout, "= %10" PRId64 " = %10f%s\n", pos.absPos, pos.time, tempoChanged ? " <----TEMPO CHANGED" : "");
            std::fflush(stdout);
#endif

            // abs_position += pos.delay;
            posPrev = &pos;
        }

        if(time > m_fullSongTimeLength)
            m_fullSongTimeLength = time;
    }

    m_fullSongTimeLength += m_postSongWaitDelay;
    // Set begin of the music
    m_trackBeginPosition = m_currentPosition;
    // Initial loop position will begin at begin of track until passing of the loop point
    m_loopBeginPosition  = m_currentPosition;
    // Set lowest level of the loop stack
    m_loop.stackLevel = -1;

    // Set the count of loops
    m_loop.loopsCount = m_loopCount;
    m_loop.loopsLeft = m_loopCount;

    /********************************************************************************/
    // Find and set proper loop points
    /********************************************************************************/
    if(!m_loop.invalidLoop && !m_currentPosition.track.empty())
    {
        unsigned caughLoopStart = 0;
        bool scanDone = false;
        const size_t  trackCount = m_currentPosition.track.size();
        Position      rowPosition(m_currentPosition);

        while(!scanDone)
        {
            const Position      rowBeginPosition(rowPosition);

            for(size_t tk = 0; tk < trackCount; ++tk)
            {
                Position::TrackInfo &track = rowPosition.track[tk];
                if((track.lastHandledEvent >= 0) && (track.delay <= 0))
                {
                    // Check is an end of track has been reached
                    if(track.pos == m_trackData[tk].end())
                    {
                        track.lastHandledEvent = -1;
                        continue;
                    }

                    for(size_t i = 0; i < track.pos->events.size(); i++)
                    {
                        const MidiEvent &evt = track.pos->events[i];
                        if(evt.type == MidiEvent::T_SPECIAL && evt.subtype == MidiEvent::ST_LOOPSTART)
                        {
                            caughLoopStart++;
                            scanDone = true;
                            break;
                        }
                    }

                    if(track.lastHandledEvent >= 0)
                    {
                        track.delay += track.pos->delay;
                        track.pos++;
                    }
                }
            }

            // Find a shortest delay from all track
            uint64_t shortestDelay = 0;
            bool     shortestDelayNotFound = true;

            for(size_t tk = 0; tk < trackCount; ++tk)
            {
                Position::TrackInfo &track = rowPosition.track[tk];
                if((track.lastHandledEvent >= 0) && (shortestDelayNotFound || track.delay < shortestDelay))
                {
                    shortestDelay = track.delay;
                    shortestDelayNotFound = false;
                }
            }

            // Schedule the next playevent to be processed after that delay
            for(size_t tk = 0; tk < trackCount; ++tk)
                rowPosition.track[tk].delay -= shortestDelay;

            if(caughLoopStart > 0)
            {
                m_loopBeginPosition = rowBeginPosition;
                m_loopBeginPosition.absTimePosition = m_loopStartTime;
                scanDone = true;
            }

            if(shortestDelayNotFound)
                break;
        }
    }

    /********************************************************************************/
    // Resolve "hell of all times" of too short drum notes:
    // move too short percussion note-offs far far away as possible
    /********************************************************************************/
#if 0 // Use this to record WAVEs for comparison before/after implementing of this
    if(m_format == Format_MIDI) // Percussion fix is needed for MIDI only, not for IMF/RSXX or CMF
    {
        //! Minimal real time in seconds
#define DRUM_NOTE_MIN_TIME  0.03
        //! Minimal ticks count
#define DRUM_NOTE_MIN_TICKS 15
        struct NoteState
        {
            double       delay;
            uint64_t     delayTicks;
            bool         isOn;
            char         ___pad[7];
        } drNotes[255];
        size_t banks[16];

        for(size_t tk = 0; tk < trackCount; ++tk)
        {
            std::memset(drNotes, 0, sizeof(drNotes));
            std::memset(banks, 0, sizeof(banks));
            MidiTrackQueue &track = m_trackData[tk];
            if(track.empty())
                continue; // Empty track is useless!

            for(MidiTrackQueue::iterator it = track.begin(); it != track.end(); it++)
            {
                MidiTrackRow &pos = *it;

                for(ssize_t e = 0; e < (ssize_t)pos.events.size(); e++)
                {
                    MidiEvent *et = &pos.events[(size_t)e];

                    /* Set MSB/LSB bank */
                    if(et->type == MidiEvent::T_CTRLCHANGE)
                    {
                        uint8_t ctrlno = et->data[0];
                        uint8_t value =  et->data[1];
                        switch(ctrlno)
                        {
                        case 0: // Set bank msb (GM bank)
                            banks[et->channel] = (value << 8) | (banks[et->channel] & 0x00FF);
                            break;
                        case 32: // Set bank lsb (XG bank)
                            banks[et->channel] = (banks[et->channel] & 0xFF00) | (value & 0x00FF);
                            break;
                        default:
                            break;
                        }
                        continue;
                    }

                    bool percussion = (et->channel == 9) ||
                                      banks[et->channel] == 0x7E00 || // XG SFX1/SFX2 channel (16128 signed decimal)
                                      banks[et->channel] == 0x7F00;   // XG Percussion channel (16256 signed decimal)
                    if(!percussion)
                        continue;

                    if(et->type == MidiEvent::T_NOTEON)
                    {
                        uint8_t     note = et->data[0] & 0x7F;
                        NoteState   &ns = drNotes[note];
                        ns.isOn = true;
                        ns.delay = 0.0;
                        ns.delayTicks = 0;
                    }
                    else if(et->type == MidiEvent::T_NOTEOFF)
                    {
                        uint8_t note = et->data[0] & 0x7F;
                        NoteState &ns = drNotes[note];
                        if(ns.isOn)
                        {
                            ns.isOn = false;
                            if(ns.delayTicks < DRUM_NOTE_MIN_TICKS || ns.delay < DRUM_NOTE_MIN_TIME) // If note is too short
                            {
                                //Move it into next event position if that possible
                                for(MidiTrackQueue::iterator itNext = it;
                                    itNext != track.end();
                                    itNext++)
                                {
                                    MidiTrackRow &posN = *itNext;
                                    if(ns.delayTicks > DRUM_NOTE_MIN_TICKS && ns.delay > DRUM_NOTE_MIN_TIME)
                                    {
                                        // Put note-off into begin of next event list
                                        posN.events.insert(posN.events.begin(), pos.events[(size_t)e]);
                                        // Renive this event from a current row
                                        pos.events.erase(pos.events.begin() + (int)e);
                                        e--;
                                        break;
                                    }
                                    ns.delay += posN.timeDelay;
                                    ns.delayTicks += posN.delay;
                                }
                            }
                            ns.delay = 0.0;
                            ns.delayTicks = 0;
                        }
                    }
                }

                //Append time delays to sustaining notes
                for(size_t no = 0; no < 128; no++)
                {
                    NoteState &ns = drNotes[no];
                    if(ns.isOn)
                    {
                        ns.delay        += pos.timeDelay;
                        ns.delayTicks   += pos.delay;
                    }
                }
            }
        }
#undef DRUM_NOTE_MIN_TIME
#undef DRUM_NOTE_MIN_TICKS
    }
#endif

}

bool BW_MidiSequencer::processEvents(bool isSeek)
{
    if(m_currentPosition.track.size() == 0)
        m_atEnd = true; // No MIDI track data to play
    if(m_atEnd)
        return false;   // No more events in the queue

    m_loop.caughtEnd = false;
    const size_t        trackCount = m_currentPosition.track.size();
    const Position      rowBeginPosition(m_currentPosition);
    bool     doLoopJump = false;
    unsigned caughLoopStart = 0;
    unsigned caughLoopStackStart = 0;
    unsigned caughLoopStackEnds = 0;
    double   caughLoopStackEndsTime = 0.0;
    unsigned caughLoopStackBreaks = 0;

#ifdef DEBUG_TIME_CALCULATION
    double maxTime = 0.0;
#endif

    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        Position::TrackInfo &track = m_currentPosition.track[tk];
        if((track.lastHandledEvent >= 0) && (track.delay <= 0))
        {
            // Check is an end of track has been reached
            if(track.pos == m_trackData[tk].end())
            {
                track.lastHandledEvent = -1;
                break;
            }

            // Handle event
            for(size_t i = 0; i < track.pos->events.size(); i++)
            {
                const MidiEvent &evt = track.pos->events[i];
#ifdef ENABLE_BEGIN_SILENCE_SKIPPING
                if(!m_currentPosition.began && (evt.type == MidiEvent::T_NOTEON))
                    m_currentPosition.began = true;
#endif
                if(isSeek && (evt.type == MidiEvent::T_NOTEON))
                    continue;
                handleEvent(tk, evt, track.lastHandledEvent);

                if(m_loop.caughtStart)
                {
                    if(m_interface->onloopStart) // Loop Start hook
                        m_interface->onloopStart(m_interface->onloopStart_userData);

                    caughLoopStart++;
                    m_loop.caughtStart = false;
                }

                if(m_loop.caughtStackStart)
                {
                    if(m_interface->onloopStart && (m_loopStartTime >= track.pos->time)) // Loop Start hook
                        m_interface->onloopStart(m_interface->onloopStart_userData);

                    caughLoopStackStart++;
                    m_loop.caughtStackStart = false;
                }

                if(m_loop.caughtStackBreak)
                {
                    caughLoopStackBreaks++;
                    m_loop.caughtStackBreak = false;
                }

                if(m_loop.caughtEnd || m_loop.isStackEnd())
                {
                    if(m_loop.caughtStackEnd)
                    {
                        m_loop.caughtStackEnd = false;
                        caughLoopStackEnds++;
                        caughLoopStackEndsTime = track.pos->time;
                    }
                    doLoopJump = true;
                    break; // Stop event handling on catching loopEnd event!
                }
            }

#ifdef DEBUG_TIME_CALCULATION
            if(maxTime < track.pos->time)
                maxTime = track.pos->time;
#endif
            // Read next event time (unless the track just ended)
            if(track.lastHandledEvent >= 0)
            {
                track.delay += track.pos->delay;
                track.pos++;
            }

            if(doLoopJump)
                break;
        }
    }

#ifdef DEBUG_TIME_CALCULATION
    std::fprintf(stdout, "                              \r");
    std::fprintf(stdout, "Time: %10f; Audio: %10f\r", maxTime, m_currentPosition.absTimePosition);
    std::fflush(stdout);
#endif

    // Find a shortest delay from all track
    uint64_t shortestDelay = 0;
    bool     shortestDelayNotFound = true;

    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        Position::TrackInfo &track = m_currentPosition.track[tk];
        if((track.lastHandledEvent >= 0) && (shortestDelayNotFound || track.delay < shortestDelay))
        {
            shortestDelay = track.delay;
            shortestDelayNotFound = false;
        }
    }

    // Schedule the next playevent to be processed after that delay
    for(size_t tk = 0; tk < trackCount; ++tk)
        m_currentPosition.track[tk].delay -= shortestDelay;

    fraction<uint64_t> t = shortestDelay * m_tempo;

#ifdef ENABLE_BEGIN_SILENCE_SKIPPING
    if(m_currentPosition.began)
#endif
        m_currentPosition.wait += t.value();

    if(caughLoopStart > 0 && m_loopBeginPosition.absTimePosition <= 0.0)
        m_loopBeginPosition = rowBeginPosition;

    if(caughLoopStackStart > 0)
    {
        while(caughLoopStackStart > 0)
        {
            m_loop.stackUp();
            LoopStackEntry &s = m_loop.getCurStack();
            s.startPosition = rowBeginPosition;
            caughLoopStackStart--;
        }
        return true;
    }

    if(caughLoopStackBreaks > 0)
    {
        while(caughLoopStackBreaks > 0)
        {
            LoopStackEntry &s = m_loop.getCurStack();
            s.loops = 0;
            s.infinity = false;
            // Quit the loop
            m_loop.stackDown();
            caughLoopStackBreaks--;
        }
    }

    if(caughLoopStackEnds > 0)
    {
        while(caughLoopStackEnds > 0)
        {
            LoopStackEntry &s = m_loop.getCurStack();
            if(s.infinity)
            {
                if(m_interface->onloopEnd && (m_loopEndTime >= caughLoopStackEndsTime)) // Loop End hook
                {
                    m_interface->onloopEnd(m_interface->onloopEnd_userData);
                    if(m_loopHooksOnly) // Stop song on reaching loop end
                    {
                        m_atEnd = true; // Don't handle events anymore
                        m_currentPosition.wait += m_postSongWaitDelay; // One second delay until stop playing
                    }
                }

                m_currentPosition = s.startPosition;
                m_loop.skipStackStart = true;

                for(uint8_t i = 0; i < 16; i++)
                    m_interface->rt_controllerChange(m_interface->rtUserData, i, 123, 0);

                return true;
            }
            else
            if(s.loops >= 0)
            {
                s.loops--;
                if(s.loops > 0)
                {
                    m_currentPosition = s.startPosition;
                    m_loop.skipStackStart = true;

                    for(uint8_t i = 0; i < 16; i++)
                        m_interface->rt_controllerChange(m_interface->rtUserData, i, 123, 0);

                    return true;
                }
                else
                {
                    // Quit the loop
                    m_loop.stackDown();
                }
            }
            else
            {
                // Quit the loop
                m_loop.stackDown();
            }
            caughLoopStackEnds--;
        }

        return true;
    }

    if(shortestDelayNotFound || m_loop.caughtEnd)
    {
        if(m_interface->onloopEnd) // Loop End hook
            m_interface->onloopEnd(m_interface->onloopEnd_userData);

        for(uint8_t i = 0; i < 16; i++)
            m_interface->rt_controllerChange(m_interface->rtUserData, i, 123, 0);

        // Loop if song end or loop end point has reached
        m_loop.caughtEnd         = false;
        shortestDelay = 0;

        if(!m_loopEnabled || (shortestDelayNotFound && m_loop.loopsCount >= 0 && m_loop.loopsLeft < 1) || m_loopHooksOnly)
        {
            m_atEnd = true; // Don't handle events anymore
            m_currentPosition.wait += m_postSongWaitDelay; // One second delay until stop playing
            return true; // We have caugh end here!
        }

        if(m_loop.temporaryBroken)
        {
            m_currentPosition = m_trackBeginPosition;
            m_loop.temporaryBroken = false;
        }
        else if(m_loop.loopsCount < 0 || m_loop.loopsLeft >= 1)
        {
            m_currentPosition = m_loopBeginPosition;
            if(m_loop.loopsCount >= 1)
                m_loop.loopsLeft--;
        }
    }

    return true; // Has events in queue
}

BW_MidiSequencer::MidiEvent BW_MidiSequencer::parseEvent(const uint8_t **pptr, const uint8_t *end, int &status)
{
    const uint8_t *&ptr = *pptr;
    BW_MidiSequencer::MidiEvent evt;

    if(ptr + 1 > end)
    {
        // When track doesn't ends on the middle of event data, it's must be fine
        evt.type = MidiEvent::T_SPECIAL;
        evt.subtype = MidiEvent::ST_ENDTRACK;
        return evt;
    }

    unsigned char byte = *(ptr++);
    bool ok = false;

    if(byte == MidiEvent::T_SYSEX || byte == MidiEvent::T_SYSEX2) // Ignore SysEx
    {
        uint64_t length = readVarLenEx(pptr, end, ok);
        if(!ok || (ptr + length > end))
        {
            m_parsingErrorsString += "parseEvent: Can't read SysEx event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.type = MidiEvent::T_SYSEX;
        evt.data.clear();
        evt.data.push_back(byte);
        std::copy(ptr, ptr + length, std::back_inserter(evt.data));
        ptr += (size_t)length;
        return evt;
    }

    if(byte == MidiEvent::T_SPECIAL)
    {
        // Special event FF
        uint8_t  evtype = *(ptr++);
        uint64_t length = readVarLenEx(pptr, end, ok);
        if(!ok || (ptr + length > end))
        {
            m_parsingErrorsString += "parseEvent: Can't read Special event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        std::string data(length ? (const char *)ptr : NULL, (size_t)length);
        ptr += (size_t)length;

        evt.type = byte;
        evt.subtype = evtype;
        evt.data.insert(evt.data.begin(), data.begin(), data.end());

#if 0 /* Print all tempo events */
        if(evt.subtype == MidiEvent::ST_TEMPOCHANGE)
        {
            if(hooks.onDebugMessage)
                hooks.onDebugMessage(hooks.onDebugMessage_userData, "Temp Change: %02X%02X%02X", evt.data[0], evt.data[1], evt.data[2]);
        }
#endif

        /* TODO: Store those meta-strings separately and give ability to read them
         * by external functions (to display song title and copyright in the player) */
        if(evt.subtype == MidiEvent::ST_COPYRIGHT)
        {
            if(m_musCopyright.empty())
            {
                m_musCopyright = std::string((const char *)evt.data.data(), evt.data.size());
                m_musCopyright.push_back('\0'); /* ending fix for UTF16 strings */
                if(m_interface->onDebugMessage)
                    m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Music copyright: %s", m_musCopyright.c_str());
            }
            else if(m_interface->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Extra copyright event: %s", str.c_str());
            }
        }
        else if(evt.subtype == MidiEvent::ST_SQTRKTITLE)
        {
            if(m_musTitle.empty())
            {
                m_musTitle = std::string((const char *)evt.data.data(), evt.data.size());
                m_musTitle.push_back('\0'); /* ending fix for UTF16 strings */
                if(m_interface->onDebugMessage)
                    m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Music title: %s", m_musTitle.c_str());
            }
            else
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                m_musTrackTitles.push_back(str);
                if(m_interface->onDebugMessage)
                    m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Track title: %s", str.c_str());
            }
        }
        else if(evt.subtype == MidiEvent::ST_INSTRTITLE)
        {
            if(m_interface->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Instrument: %s", str.c_str());
            }
        }
        else if(evt.subtype == MidiEvent::ST_MARKER)
        {
            // To lower
            for(size_t i = 0; i < data.size(); i++)
            {
                if(data[i] <= 'Z' && data[i] >= 'A')
                    data[i] = static_cast<char>(data[i] - ('Z' - 'z'));
            }

            if(data == "loopstart")
            {
                // Return a custom Loop Start event instead of Marker
                evt.subtype = MidiEvent::ST_LOOPSTART;
                evt.data.clear(); // Data is not needed
                return evt;
            }

            if(data == "loopend")
            {
                // Return a custom Loop End event instead of Marker
                evt.subtype = MidiEvent::ST_LOOPEND;
                evt.data.clear(); // Data is not needed
                return evt;
            }

            if(data.substr(0, 10) == "loopstart=")
            {
                evt.type = MidiEvent::T_SPECIAL;
                evt.subtype = MidiEvent::ST_LOOPSTACK_BEGIN;
                uint8_t loops = static_cast<uint8_t>(atoi(data.substr(10).c_str()));
                evt.data.clear();
                evt.data.push_back(loops);

                if(m_interface->onDebugMessage)
                {
                    m_interface->onDebugMessage(
                        m_interface->onDebugMessage_userData,
                        "Stack Marker Loop Start at %d to %d level with %d loops",
                        m_loop.stackLevel,
                        m_loop.stackLevel + 1,
                        loops
                    );
                }
                return evt;
            }

            if(data.substr(0, 8) == "loopend=")
            {
                evt.type = MidiEvent::T_SPECIAL;
                evt.subtype = MidiEvent::ST_LOOPSTACK_END;
                evt.data.clear();

                if(m_interface->onDebugMessage)
                {
                    m_interface->onDebugMessage(
                        m_interface->onDebugMessage_userData,
                        "Stack Marker Loop %s at %d to %d level",
                        (evt.subtype == MidiEvent::ST_LOOPSTACK_END ? "End" : "Break"),
                        m_loop.stackLevel,
                        m_loop.stackLevel - 1
                    );
                }
                return evt;
            }
        }

        if(evtype == MidiEvent::ST_ENDTRACK)
            status = -1; // Finalize track

        return evt;
    }

    // Any normal event (80..EF)
    if(byte < 0x80)
    {
        byte = static_cast<uint8_t>(status | 0x80);
        ptr--;
    }

    // Sys Com Song Select(Song #) [0-127]
    if(byte == MidiEvent::T_SYSCOMSNGSEL)
    {
        if(ptr + 1 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read System Command Song Select event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        return evt;
    }

    // Sys Com Song Position Pntr [LSB, MSB]
    if(byte == MidiEvent::T_SYSCOMSPOSPTR)
    {
        if(ptr + 2 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read System Command Position Pointer event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));
        return evt;
    }

    uint8_t midCh = byte & 0x0F, evType = (byte >> 4) & 0x0F;
    status = byte;
    evt.channel = midCh;
    evt.type = evType;

    switch(evType)
    {
    case MidiEvent::T_NOTEOFF: // 2 byte length
    case MidiEvent::T_NOTEON:
    case MidiEvent::T_NOTETOUCH:
    case MidiEvent::T_CTRLCHANGE:
    case MidiEvent::T_WHEEL:
        if(ptr + 2 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read regular 2-byte event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }

        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));

        if((evType == MidiEvent::T_NOTEON) && (evt.data[1] == 0))
        {
            evt.type = MidiEvent::T_NOTEOFF; // Note ON with zero velocity is Note OFF!
        }
        else
        if(evType == MidiEvent::T_CTRLCHANGE)
        {
            // 111'th loopStart controller (RPG Maker and others)
            if(m_format == Format_MIDI)
            {
                switch(evt.data[0])
                {
                case 110:
                    if(m_loopFormat == Loop_Default)
                    {
                        // Change event type to custom Loop Start event and clear data
                        evt.type = MidiEvent::T_SPECIAL;
                        evt.subtype = MidiEvent::ST_LOOPSTART;
                        evt.data.clear();
                        m_loopFormat = Loop_HMI;
                    }
                    else if(m_loopFormat == Loop_HMI)
                    {
                        // Repeating of 110'th point is BAD practice, treat as EMIDI
                        m_loopFormat = Loop_EMIDI;
                    }
                    break;

                case 111:
                    if(m_loopFormat == Loop_HMI)
                    {
                        // Change event type to custom Loop End event and clear data
                        evt.type = MidiEvent::T_SPECIAL;
                        evt.subtype = MidiEvent::ST_LOOPEND;
                        evt.data.clear();
                    }
                    else if(m_loopFormat != Loop_EMIDI)
                    {
                        // Change event type to custom Loop Start event and clear data
                        evt.type = MidiEvent::T_SPECIAL;
                        evt.subtype = MidiEvent::ST_LOOPSTART;
                        evt.data.clear();
                    }
                    break;

                case 113:
                    if(m_loopFormat == Loop_EMIDI)
                    {
                        // EMIDI does using of CC113 with same purpose as CC7
                        evt.data[0] = 7;
                    }
                    break;
#if 0 //WIP
                case 116:
                    if(m_loopFormat == Loop_EMIDI)
                    {
                        evt.type = MidiEvent::T_SPECIAL;
                        evt.subtype = MidiEvent::ST_LOOPSTACK_BEGIN;
                        evt.data[0] = evt.data[1];
                        evt.data.pop_back();

                        if(m_interface->onDebugMessage)
                        {
                            m_interface->onDebugMessage(
                                m_interface->onDebugMessage_userData,
                                "Stack EMIDI Loop Start at %d to %d level with %d loops",
                                m_loop.stackLevel,
                                m_loop.stackLevel + 1,
                                evt.data[0]
                            );
                        }
                    }
                    break;

                case 117:  // Next/Break Loop Controller
                    if(m_loopFormat == Loop_EMIDI)
                    {
                        evt.type = MidiEvent::T_SPECIAL;
                        evt.subtype = MidiEvent::ST_LOOPSTACK_END;
                        evt.data.clear();

                        if(m_interface->onDebugMessage)
                        {
                            m_interface->onDebugMessage(
                                m_interface->onDebugMessage_userData,
                                "Stack EMIDI Loop End at %d to %d level",
                                m_loop.stackLevel,
                                m_loop.stackLevel - 1
                            );
                        }
                    }
                    break;
#endif
                }
            }

            if(m_format == Format_XMIDI)
            {
                switch(evt.data[0])
                {
                case 116:  // For Loop Controller
                    evt.type = MidiEvent::T_SPECIAL;
                    evt.subtype = MidiEvent::ST_LOOPSTACK_BEGIN;
                    evt.data[0] = evt.data[1];
                    evt.data.pop_back();

                    if(m_interface->onDebugMessage)
                    {
                        m_interface->onDebugMessage(
                            m_interface->onDebugMessage_userData,
                            "Stack XMI Loop Start at %d to %d level with %d loops",
                            m_loop.stackLevel,
                            m_loop.stackLevel + 1,
                            evt.data[0]
                        );
                    }
                    break;

                case 117:  // Next/Break Loop Controller
                    evt.type = MidiEvent::T_SPECIAL;
                    evt.subtype = evt.data[1] < 64 ?
                                MidiEvent::ST_LOOPSTACK_BREAK :
                                MidiEvent::ST_LOOPSTACK_END;
                    evt.data.clear();

                    if(m_interface->onDebugMessage)
                    {
                        m_interface->onDebugMessage(
                            m_interface->onDebugMessage_userData,
                            "Stack XMI Loop %s at %d to %d level",
                            (evt.subtype == MidiEvent::ST_LOOPSTACK_END ? "End" : "Break"),
                            m_loop.stackLevel,
                            m_loop.stackLevel - 1
                        );
                    }
                    break;

                case 119:  // Callback Trigger
                    evt.type = MidiEvent::T_SPECIAL;
                    evt.subtype = MidiEvent::ST_CALLBACK_TRIGGER;
                    evt.data.assign(1, evt.data[1]);
                    break;
                }
            }
        }

        return evt;
    case MidiEvent::T_PATCHCHANGE: // 1 byte length
    case MidiEvent::T_CHANAFTTOUCH:
        if(ptr + 1 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read regular 1-byte event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.data.push_back(*(ptr++));
        return evt;
    default:
        break;
    }

    return evt;
}

void BW_MidiSequencer::handleEvent(size_t track, const BW_MidiSequencer::MidiEvent &evt, int32_t &status)
{
    if(track == 0 && m_smfFormat < 2 && evt.type == MidiEvent::T_SPECIAL &&
       (evt.subtype == MidiEvent::ST_TEMPOCHANGE || evt.subtype == MidiEvent::ST_TIMESIGNATURE))
    {
        /* never reject track 0 timing events on SMF format != 2
           note: multi-track XMI convert to format 2 SMF */
    }
    else
    {
        if(m_trackSolo != ~static_cast<size_t>(0) && track != m_trackSolo)
            return;
        if(m_trackDisable[track])
            return;
    }

    if(m_interface->onEvent)
    {
        m_interface->onEvent(m_interface->onEvent_userData,
                             evt.type, evt.subtype, evt.channel,
                             evt.data.data(), evt.data.size());
    }

    if(evt.type == MidiEvent::T_SYSEX || evt.type == MidiEvent::T_SYSEX2) // Ignore SysEx
    {
        m_interface->rt_systemExclusive(m_interface->rtUserData, evt.data.data(), evt.data.size());
        return;
    }

    if(evt.type == MidiEvent::T_SPECIAL)
    {
        // Special event FF
        uint_fast16_t  evtype = evt.subtype;
        uint64_t length = static_cast<uint64_t>(evt.data.size());
        const char *data(length ? reinterpret_cast<const char *>(evt.data.data()) : "\0\0\0\0\0\0\0\0");

        if(m_interface->rt_metaEvent) // Meta event hook
            m_interface->rt_metaEvent(m_interface->rtUserData, evtype, reinterpret_cast<const uint8_t*>(data), size_t(length));

        if(evtype == MidiEvent::ST_ENDTRACK) // End Of Track
        {
            status = -1;
            return;
        }

        if(evtype == MidiEvent::ST_TEMPOCHANGE) // Tempo change
        {
            m_tempo = m_invDeltaTicks * fraction<uint64_t>(readBEint(evt.data.data(), evt.data.size()));
            return;
        }

        if(evtype == MidiEvent::ST_MARKER) // Meta event
        {
            // Do nothing! :-P
            return;
        }

        if(evtype == MidiEvent::ST_DEVICESWITCH)
        {
            if(m_interface->onDebugMessage)
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Switching another device: %s", data);
            if(m_interface->rt_deviceSwitch)
                m_interface->rt_deviceSwitch(m_interface->rtUserData, track, data, size_t(length));
            return;
        }

        // Turn on Loop handling when loop is enabled
        if(m_loopEnabled && !m_loop.invalidLoop)
        {
            if(evtype == MidiEvent::ST_LOOPSTART) // Special non-spec MIDI loop Start point
            {
                m_loop.caughtStart = true;
                return;
            }

            if(evtype == MidiEvent::ST_LOOPEND) // Special non-spec MIDI loop End point
            {
                m_loop.caughtEnd = true;
                return;
            }

            if(evtype == MidiEvent::ST_LOOPSTACK_BEGIN)
            {
                if(m_loop.skipStackStart)
                {
                    m_loop.skipStackStart = false;
                    return;
                }

                char x = data[0];
                size_t slevel = static_cast<size_t>(m_loop.stackLevel + 1);
                while(slevel >= m_loop.stack.size())
                {
                    LoopStackEntry e;
                    e.loops = x;
                    e.infinity = (x == 0);
                    e.start = 0;
                    e.end = 0;
                    m_loop.stack.push_back(e);
                }

                LoopStackEntry &s = m_loop.stack[slevel];
                s.loops = static_cast<int>(x);
                s.infinity = (x == 0);
                m_loop.caughtStackStart = true;
                return;
            }

            if(evtype == MidiEvent::ST_LOOPSTACK_END)
            {
                m_loop.caughtStackEnd = true;
                return;
            }

            if(evtype == MidiEvent::ST_LOOPSTACK_BREAK)
            {
                m_loop.caughtStackBreak = true;
                return;
            }
        }

        if(evtype == MidiEvent::ST_CALLBACK_TRIGGER)
        {
#if 0 /* Print all callback triggers events */
            if(m_interface->onDebugMessage)
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Callback Trigger: %02X", evt.data[0]);
#endif
            if(m_triggerHandler)
                m_triggerHandler(m_triggerUserData, static_cast<unsigned>(data[0]), track);
            return;
        }

        if(evtype == MidiEvent::ST_RAWOPL) // Special non-spec ADLMIDI special for IMF playback: Direct poke to AdLib
        {
            if(m_interface->rt_rawOPL)
                m_interface->rt_rawOPL(m_interface->rtUserData, static_cast<uint8_t>(data[0]), static_cast<uint8_t>(data[1]));
            return;
        }

        if(evtype == MidiEvent::ST_SONG_BEGIN_HOOK)
        {
            if(m_interface->onSongStart)
                m_interface->onSongStart(m_interface->onSongStart_userData);
            return;
        }

        return;
    }

    if(evt.type == MidiEvent::T_SYSCOMSNGSEL ||
       evt.type == MidiEvent::T_SYSCOMSPOSPTR)
        return;

    size_t midCh = evt.channel;
    if(m_interface->rt_currentDevice)
        midCh += m_interface->rt_currentDevice(m_interface->rtUserData, track);
    status = evt.type;

    switch(evt.type)
    {
    case MidiEvent::T_NOTEOFF: // Note off
    {
        if(midCh < 16 && m_channelDisable[midCh])
            break; // Disabled channel
        uint8_t note = evt.data[0];
        uint8_t vol = evt.data[1];
        if(m_interface->rt_noteOff)
            m_interface->rt_noteOff(m_interface->rtUserData, static_cast<uint8_t>(midCh), note);
        if(m_interface->rt_noteOffVel)
            m_interface->rt_noteOffVel(m_interface->rtUserData, static_cast<uint8_t>(midCh), note, vol);
        break;
    }

    case MidiEvent::T_NOTEON: // Note on
    {
        if(midCh < 16 && m_channelDisable[midCh])
            break; // Disabled channel
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        m_interface->rt_noteOn(m_interface->rtUserData, static_cast<uint8_t>(midCh), note, vol);
        break;
    }

    case MidiEvent::T_NOTETOUCH: // Note touch
    {
        uint8_t note = evt.data[0];
        uint8_t vol =  evt.data[1];
        m_interface->rt_noteAfterTouch(m_interface->rtUserData, static_cast<uint8_t>(midCh), note, vol);
        break;
    }

    case MidiEvent::T_CTRLCHANGE: // Controller change
    {
        uint8_t ctrlno = evt.data[0];
        uint8_t value =  evt.data[1];
        m_interface->rt_controllerChange(m_interface->rtUserData, static_cast<uint8_t>(midCh), ctrlno, value);
        break;
    }

    case MidiEvent::T_PATCHCHANGE: // Patch change
    {
        m_interface->rt_patchChange(m_interface->rtUserData, static_cast<uint8_t>(midCh), evt.data[0]);
        break;
    }

    case MidiEvent::T_CHANAFTTOUCH: // Channel after-touch
    {
        uint8_t chanat = evt.data[0];
        m_interface->rt_channelAfterTouch(m_interface->rtUserData, static_cast<uint8_t>(midCh), chanat);
        break;
    }

    case MidiEvent::T_WHEEL: // Wheel/pitch bend
    {
        uint8_t a = evt.data[0];
        uint8_t b = evt.data[1];
        m_interface->rt_pitchBend(m_interface->rtUserData, static_cast<uint8_t>(midCh), b, a);
        break;
    }

    default:
        break;
    }//switch
}

double BW_MidiSequencer::Tick(double s, double granularity)
{
    assert(m_interface); // MIDI output interface must be defined!

    s *= m_tempoMultiplier;
#ifdef ENABLE_BEGIN_SILENCE_SKIPPING
    if(CurrentPositionNew.began)
#endif
        m_currentPosition.wait -= s;
    m_currentPosition.absTimePosition += s;

    int antiFreezeCounter = 10000; // Limit 10000 loops to avoid freezing
    while((m_currentPosition.wait <= granularity * 0.5) && (antiFreezeCounter > 0))
    {
        if(!processEvents())
            break;
        if(m_currentPosition.wait <= 0.0)
            antiFreezeCounter--;
    }

    if(antiFreezeCounter <= 0)
        m_currentPosition.wait += 1.0; /* Add extra 1 second when over 10000 events
                                          with zero delay are been detected */

    if(m_currentPosition.wait < 0.0) // Avoid negative delay value!
        return 0.0;

    return m_currentPosition.wait;
}


double BW_MidiSequencer::seek(double seconds, const double granularity)
{
    if(seconds < 0.0)
        return 0.0; // Seeking negative position is forbidden! :-P
    const double granualityHalf = granularity * 0.5,
                 s = seconds; // m_setup.delay < m_setup.maxdelay ? m_setup.delay : m_setup.maxdelay;

    /* Attempt to go away out of song end must rewind position to begin */
    if(seconds > m_fullSongTimeLength)
    {
        this->rewind();
        return 0.0;
    }

    bool loopFlagState = m_loopEnabled;
    // Turn loop pooints off because it causes wrong position rememberin on a quick seek
    m_loopEnabled = false;

    /*
     * Seeking search is similar to regular ticking, except of next things:
     * - We don't processsing arpeggio and vibrato
     * - To keep correctness of the state after seek, begin every search from begin
     * - All sustaining notes must be killed
     * - Ignore Note-On events
     */
    this->rewind();

    /*
     * Set "loop Start" to false to prevent overwrite of loopStart position with
     * seek destinition position
     *
     * TODO: Detect & set loopStart position on load time to don't break loop while seeking
     */
    m_loop.caughtStart   = false;

    m_loop.temporaryBroken = (seconds >= m_loopEndTime);

    while((m_currentPosition.absTimePosition < seconds) &&
          (m_currentPosition.absTimePosition < m_fullSongTimeLength))
    {
        m_currentPosition.wait -= s;
        m_currentPosition.absTimePosition += s;
        int antiFreezeCounter = 10000; // Limit 10000 loops to avoid freezing
        double dstWait = m_currentPosition.wait + granualityHalf;
        while((m_currentPosition.wait <= granualityHalf)/*&& (antiFreezeCounter > 0)*/)
        {
            // std::fprintf(stderr, "wait = %g...\n", CurrentPosition.wait);
            if(!processEvents(true))
                break;
            // Avoid freeze because of no waiting increasing in more than 10000 cycles
            if(m_currentPosition.wait <= dstWait)
                antiFreezeCounter--;
            else
            {
                dstWait = m_currentPosition.wait + granualityHalf;
                antiFreezeCounter = 10000;
            }
        }
        if(antiFreezeCounter <= 0)
            m_currentPosition.wait += 1.0;/* Add extra 1 second when over 10000 events
                                             with zero delay are been detected */
    }

    if(m_currentPosition.wait < 0.0)
        m_currentPosition.wait = 0.0;

    if(m_atEnd)
    {
        this->rewind();
        m_loopEnabled = loopFlagState;
        return 0.0;
    }

    m_time.reset();
    m_time.delay = m_currentPosition.wait;

    m_loopEnabled = loopFlagState;
    return m_currentPosition.wait;
}

double BW_MidiSequencer::tell()
{
    return m_currentPosition.absTimePosition;
}

double BW_MidiSequencer::timeLength()
{
    return m_fullSongTimeLength;
}

double BW_MidiSequencer::getLoopStart()
{
    return m_loopStartTime;
}

double BW_MidiSequencer::getLoopEnd()
{
    return m_loopEndTime;
}

void BW_MidiSequencer::rewind()
{
    m_currentPosition   = m_trackBeginPosition;
    m_atEnd             = false;

    m_loop.loopsCount = m_loopCount;
    m_loop.reset();
    m_loop.caughtStart  = true;
    m_loop.temporaryBroken = false;
    m_time.reset();
}

void BW_MidiSequencer::setTempo(double tempo)
{
    m_tempoMultiplier = tempo;
}

bool BW_MidiSequencer::loadMIDI(const std::string &filename)
{
    FileAndMemReader file;
    file.openFile(filename.c_str());
    if(!loadMIDI(file))
        return false;
    return true;
}

bool BW_MidiSequencer::loadMIDI(const void *data, size_t size)
{
    FileAndMemReader file;
    file.openData(data, size);
    return loadMIDI(file);
}

template<class T>
class BufferGuard
{
    T *m_ptr;
public:
    BufferGuard() : m_ptr(NULL)
    {}

    ~BufferGuard()
    {
        set();
    }

    void set(T *p = NULL)
    {
        if(m_ptr)
            free(m_ptr);
        m_ptr = p;
    }
};

/**
 * @brief Detect the EA-MUS file format
 * @param head Header part
 * @param fr Context with opened file data
 * @return true if given file was identified as EA-MUS
 */
static bool detectRSXX(const char *head, FileAndMemReader &fr)
{
    char headerBuf[7] = "";
    bool ret = false;

    // Try to identify RSXX format
    if(head[0] >= 0x5D && fr.fileSize() > reinterpret_cast<const uint8_t*>(head)[0])
    {
        fr.seek(head[0] - 0x10, FileAndMemReader::SET);
        fr.read(headerBuf, 1, 6);
        if(std::memcmp(headerBuf, "rsxx}u", 6) == 0)
            ret = true;
    }

    fr.seek(0, FileAndMemReader::SET);
    return ret;
}

/**
 * @brief Detect the Id-software Music File format
 * @param head Header part
 * @param fr Context with opened file data
 * @return true if given file was identified as IMF
 */
static bool detectIMF(const char *head, FileAndMemReader &fr)
{
    uint8_t raw[4];
    size_t end = static_cast<size_t>(head[0]) + 256 * static_cast<size_t>(head[1]);

    if(end & 3)
        return false;

    size_t backup_pos = fr.tell();
    int64_t sum1 = 0, sum2 = 0;
    fr.seek((end > 0 ? 2 : 0), FileAndMemReader::SET);

    for(size_t n = 0; n < 16383; ++n)
    {
        if(fr.read(raw, 1, 4) != 4)
            break;
        int64_t value1 = raw[0];
        value1 += raw[1] << 8;
        sum1 += value1;
        int64_t value2 = raw[2];
        value2 += raw[3] << 8;
        sum2 += value2;
    }

    fr.seek(static_cast<long>(backup_pos), FileAndMemReader::SET);

    return (sum1 > sum2);
}

bool BW_MidiSequencer::loadMIDI(FileAndMemReader &fr)
{
    size_t  fsize = 0;
    BW_MidiSequencer_UNUSED(fsize);
    m_parsingErrorsString.clear();

    assert(m_interface); // MIDI output interface must be defined!

    if(!fr.isValid())
    {
        m_errorString = "Invalid data stream!\n";
#ifndef _WIN32
        m_errorString += std::strerror(errno);
#endif
        return false;
    }

    m_atEnd            = false;
    m_loop.fullReset();
    m_loop.caughtStart = true;

    m_format = Format_MIDI;
    m_smfFormat = 0;

    m_cmfInstruments.clear();
    m_rawSongsData.clear();

    const size_t headerSize = 4 + 4 + 2 + 2 + 2; // 14
    char headerBuf[headerSize] = "";

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }


    if(std::memcmp(headerBuf, "MThd\0\0\0\6", 8) == 0)
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseSMF(fr);
    }

    if(std::memcmp(headerBuf, "RIFF", 4) == 0)
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseRMI(fr);
    }

    if(std::memcmp(headerBuf, "GMF\x1", 4) == 0)
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseGMF(fr);
    }

#ifndef BWMIDI_DISABLE_MUS_SUPPORT
    if(std::memcmp(headerBuf, "MUS\x1A", 4) == 0)
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseMUS(fr);
    }
#endif

#ifndef BWMIDI_DISABLE_XMI_SUPPORT
    if((std::memcmp(headerBuf, "FORM", 4) == 0) && (std::memcmp(headerBuf + 8, "XDIR", 4) == 0))
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseXMI(fr);
    }
#endif

    if(std::memcmp(headerBuf, "CTMF", 4) == 0)
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseCMF(fr);
    }

    if(detectIMF(headerBuf, fr))
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseIMF(fr);
    }

    if(detectRSXX(headerBuf, fr))
    {
        fr.seek(0, FileAndMemReader::SET);
        return parseRSXX(fr);
    }

    m_errorString = "Unknown or unsupported file format";
    return false;
}


bool BW_MidiSequencer::parseIMF(FileAndMemReader &fr)
{
    const size_t    deltaTicks = 1;
    const size_t    trackCount = 1;
    const uint32_t  imfTempo = 1428;
    size_t          imfEnd = 0;
    uint64_t        abs_position = 0;
    uint8_t         imfRaw[4];

    MidiTrackRow    evtPos;
    MidiEvent       event;

    std::vector<MidiEvent> temposList;

    m_format = Format_IMF;

    buildSmfSetupReset(trackCount);

    m_invDeltaTicks = fraction<uint64_t>(1, 1000000l * static_cast<uint64_t>(deltaTicks));
    m_tempo = fraction<uint64_t>(1, static_cast<uint64_t>(deltaTicks) * 2);

    fr.seek(0, FileAndMemReader::SET);
    if(fr.read(imfRaw, 1, 2) != 2)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    imfEnd = static_cast<size_t>(imfRaw[0]) + 256 * static_cast<size_t>(imfRaw[1]);

    // Define the playing tempo
    event.type = MidiEvent::T_SPECIAL;
    event.subtype = MidiEvent::ST_TEMPOCHANGE;
    event.absPosition = 0;
    event.data.resize(4);
    event.data[0] = static_cast<uint8_t>((imfTempo >> 24) & 0xFF);
    event.data[1] = static_cast<uint8_t>((imfTempo >> 16) & 0xFF);
    event.data[2] = static_cast<uint8_t>((imfTempo >> 8) & 0xFF);
    event.data[3] = static_cast<uint8_t>((imfTempo & 0xFF));
    evtPos.events.push_back(event);
    temposList.push_back(event);

    // Define the draft for IMF events
    event.type = MidiEvent::T_SPECIAL;
    event.subtype = MidiEvent::ST_RAWOPL;
    event.absPosition = 0;
    event.data.resize(2);

    fr.seek((imfEnd > 0) ? 2 : 0, FileAndMemReader::SET);

    if(imfEnd == 0) // IMF Type 0 with unlimited file length
        imfEnd = fr.fileSize();

    while(fr.tell() < imfEnd && !fr.eof())
    {
        if(fr.read(imfRaw, 1, 4) != 4)
            break;

        event.data[0] = imfRaw[0]; // port index
        event.data[1] = imfRaw[1]; // port value
        event.absPosition = abs_position;
        event.isValid = 1;

        evtPos.events.push_back(event);
        evtPos.delay = static_cast<uint64_t>(imfRaw[2]) + 256 * static_cast<uint64_t>(imfRaw[3]);

        if(evtPos.delay > 0)
        {
            evtPos.absPos = abs_position;
            abs_position += evtPos.delay;
            m_trackData[0].push_back(evtPos);
            evtPos.clear();
        }
    }

    // Add final row
    evtPos.absPos = abs_position;
    abs_position += evtPos.delay;
    m_trackData[0].push_back(evtPos);

    if(!m_trackData[0].empty())
        m_currentPosition.track[0].pos = m_trackData[0].begin();

    buildTimeLine(temposList);

    return true;
}

bool BW_MidiSequencer::parseRSXX(FileAndMemReader &fr)
{
    const size_t headerSize = 14;
    char headerBuf[headerSize] = "";
    size_t fsize = 0;
    size_t deltaTicks = 192, trackCount = 1;
    std::vector<std::vector<uint8_t> > rawTrackData;

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    // Try to identify RSXX format
    char start = headerBuf[0];
    if(start < 0x5D)
    {
        m_errorString = "RSXX song too short!\n";
        return false;
    }
    else
    {
        fr.seek(start - 0x10, FileAndMemReader::SET);
        fr.read(headerBuf, 1, 6);
        if(std::memcmp(headerBuf, "rsxx}u", 6) == 0)
        {
            m_format = Format_RSXX;
            fr.seek(start, FileAndMemReader::SET);
            trackCount = 1;
            deltaTicks = 60;
        }
        else
        {
            m_errorString = "Invalid RSXX header!\n";
            return false;
        }
    }

    rawTrackData.clear();
    rawTrackData.resize(trackCount, std::vector<uint8_t>());
    m_invDeltaTicks = fraction<uint64_t>(1, 1000000l * static_cast<uint64_t>(deltaTicks));
    m_tempo         = fraction<uint64_t>(1,            static_cast<uint64_t>(deltaTicks));

    size_t totalGotten = 0;

    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        // Read track header
        size_t trackLength;

        size_t pos = fr.tell();
        fr.seek(0, FileAndMemReader::END);
        trackLength = fr.tell() - pos;
        fr.seek(static_cast<long>(pos), FileAndMemReader::SET);

        // Read track data
        rawTrackData[tk].resize(trackLength);
        fsize = fr.read(&rawTrackData[tk][0], 1, trackLength);
        if(fsize < trackLength)
        {
            m_errorString = fr.fileName() + ": Unexpected file ending while getting raw track data!\n";
            return false;
        }
        totalGotten += fsize;

        //Finalize raw track data with a zero
        rawTrackData[tk].push_back(0);
    }

    for(size_t tk = 0; tk < trackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if(totalGotten == 0)
    {
        m_errorString = fr.fileName() + ": Empty track data";
        return false;
    }

    // Build new MIDI events table
    if(!buildSmfTrackData(rawTrackData))
    {
        m_errorString = fr.fileName() + ": MIDI data parsing error has occouped!\n" + m_parsingErrorsString;
        return false;
    }

    m_smfFormat = 0;
    m_loop.stackLevel   = -1;

    return true;
}

bool BW_MidiSequencer::parseCMF(FileAndMemReader &fr)
{
    const size_t headerSize = 14;
    char headerBuf[headerSize] = "";
    size_t fsize = 0;
    size_t deltaTicks = 192, trackCount = 1;
    std::vector<std::vector<uint8_t> > rawTrackData;

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "CTMF", 4) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format, CTMF signature is not found!\n";
        return false;
    }

    m_format = Format_CMF;

    //unsigned version   = ReadLEint(HeaderBuf+4, 2);
    uint64_t ins_start = readLEint(headerBuf + 6, 2);
    uint64_t mus_start = readLEint(headerBuf + 8, 2);
    //unsigned deltas    = ReadLEint(HeaderBuf+10, 2);
    uint64_t ticks     = readLEint(headerBuf + 12, 2);
    // Read title, author, remarks start offsets in file
    fsize = fr.read(headerBuf, 1, 6);
    if(fsize < 6)
    {
        fr.close();
        m_errorString = "Unexpected file ending on attempt to read CTMF header!";
        return false;
    }

    //uint64_t notes_starts[3] = {readLEint(headerBuf + 0, 2), readLEint(headerBuf + 0, 4), readLEint(headerBuf + 0, 6)};
    fr.seek(16, FileAndMemReader::CUR); // Skip the channels-in-use table
    fsize = fr.read(headerBuf, 1, 4);
    if(fsize < 4)
    {
        fr.close();
        m_errorString = "Unexpected file ending on attempt to read CMF instruments block header!";
        return false;
    }

    uint64_t ins_count =  readLEint(headerBuf + 0, 2);
    fr.seek(static_cast<long>(ins_start), FileAndMemReader::SET);

    m_cmfInstruments.reserve(static_cast<size_t>(ins_count));
    for(uint64_t i = 0; i < ins_count; ++i)
    {
        CmfInstrument inst;
        fsize = fr.read(inst.data, 1, 16);
        if(fsize < 16)
        {
            fr.close();
            m_errorString = "Unexpected file ending on attempt to read CMF instruments raw data!";
            return false;
        }
        m_cmfInstruments.push_back(inst);
    }

    fr.seeku(mus_start, FileAndMemReader::SET);
    trackCount = 1;
    deltaTicks = (size_t)ticks;

    rawTrackData.clear();
    rawTrackData.resize(trackCount, std::vector<uint8_t>());
    m_invDeltaTicks = fraction<uint64_t>(1, 1000000l * static_cast<uint64_t>(deltaTicks));
    m_tempo         = fraction<uint64_t>(1,            static_cast<uint64_t>(deltaTicks));

    size_t totalGotten = 0;

    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        // Read track header
        size_t trackLength;
        size_t pos = fr.tell();
        fr.seek(0, FileAndMemReader::END);
        trackLength = fr.tell() - pos;
        fr.seek(static_cast<long>(pos), FileAndMemReader::SET);

        // Read track data
        rawTrackData[tk].resize(trackLength);
        fsize = fr.read(&rawTrackData[tk][0], 1, trackLength);
        if(fsize < trackLength)
        {
            m_errorString = fr.fileName() + ": Unexpected file ending while getting raw track data!\n";
            return false;
        }
        totalGotten += fsize;
    }

    for(size_t tk = 0; tk < trackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if(totalGotten == 0)
    {
        m_errorString = fr.fileName() + ": Empty track data";
        return false;
    }

    // Build new MIDI events table
    if(!buildSmfTrackData(rawTrackData))
    {
        m_errorString = fr.fileName() + ": MIDI data parsing error has occouped!\n" + m_parsingErrorsString;
        return false;
    }

    return true;
}

bool BW_MidiSequencer::parseGMF(FileAndMemReader &fr)
{
    const size_t headerSize = 14;
    char headerBuf[headerSize] = "";
    size_t fsize = 0;
    size_t deltaTicks = 192, trackCount = 1;
    std::vector<std::vector<uint8_t> > rawTrackData;

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "GMF\x1", 4) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format, GMF\\x1 signature is not found!\n";
        return false;
    }

    fr.seek(7 - static_cast<long>(headerSize), FileAndMemReader::CUR);

    rawTrackData.clear();
    rawTrackData.resize(trackCount, std::vector<uint8_t>());
    m_invDeltaTicks = fraction<uint64_t>(1, 1000000l * static_cast<uint64_t>(deltaTicks));
    m_tempo         = fraction<uint64_t>(1,            static_cast<uint64_t>(deltaTicks) * 2);
    static const unsigned char EndTag[4] = {0xFF, 0x2F, 0x00, 0x00};
    size_t totalGotten = 0;

    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        // Read track header
        size_t trackLength;
        size_t pos = fr.tell();
        fr.seek(0, FileAndMemReader::END);
        trackLength = fr.tell() - pos;
        fr.seek(static_cast<long>(pos), FileAndMemReader::SET);

        // Read track data
        rawTrackData[tk].resize(trackLength);
        fsize = fr.read(&rawTrackData[tk][0], 1, trackLength);
        if(fsize < trackLength)
        {
            m_errorString = fr.fileName() + ": Unexpected file ending while getting raw track data!\n";
            return false;
        }
        totalGotten += fsize;
        // Note: GMF does include the track end tag.
        rawTrackData[tk].insert(rawTrackData[tk].end(), EndTag + 0, EndTag + 4);
    }

    for(size_t tk = 0; tk < trackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if(totalGotten == 0)
    {
        m_errorString = fr.fileName() + ": Empty track data";
        return false;
    }

    // Build new MIDI events table
    if(!buildSmfTrackData(rawTrackData))
    {
        m_errorString = fr.fileName() + ": MIDI data parsing error has occouped!\n" + m_parsingErrorsString;
        return false;
    }

    return true;
}

bool BW_MidiSequencer::parseSMF(FileAndMemReader &fr)
{
    const size_t headerSize = 14; // 4 + 4 + 2 + 2 + 2
    char headerBuf[headerSize] = "";
    size_t fsize = 0;
    size_t deltaTicks = 192, TrackCount = 1;
    unsigned smfFormat = 0;
    std::vector<std::vector<uint8_t> > rawTrackData;

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "MThd\0\0\0\6", 8) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format, MThd signature is not found!\n";
        return false;
    }

    smfFormat  = static_cast<unsigned>(readBEint(headerBuf + 8,  2));
    TrackCount = static_cast<size_t>(readBEint(headerBuf + 10, 2));
    deltaTicks = static_cast<size_t>(readBEint(headerBuf + 12, 2));

    if(smfFormat > 2)
        smfFormat = 1;

    rawTrackData.clear();
    rawTrackData.resize(TrackCount, std::vector<uint8_t>());
    m_invDeltaTicks = fraction<uint64_t>(1, 1000000l * static_cast<uint64_t>(deltaTicks));
    m_tempo         = fraction<uint64_t>(1,            static_cast<uint64_t>(deltaTicks) * 2);

    size_t totalGotten = 0;

    for(size_t tk = 0; tk < TrackCount; ++tk)
    {
        // Read track header
        size_t trackLength;

        fsize = fr.read(headerBuf, 1, 8);
        if((fsize < 8) || (std::memcmp(headerBuf, "MTrk", 4) != 0))
        {
            m_errorString = fr.fileName() + ": Invalid format, MTrk signature is not found!\n";
            return false;
        }
        trackLength = (size_t)readBEint(headerBuf + 4, 4);

        // Read track data
        rawTrackData[tk].resize(trackLength);
        fsize = fr.read(&rawTrackData[tk][0], 1, trackLength);
        if(fsize < trackLength)
        {
            m_errorString = fr.fileName() + ": Unexpected file ending while getting raw track data!\n";
            return false;
        }

        totalGotten += fsize;
    }

    for(size_t tk = 0; tk < TrackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if(totalGotten == 0)
    {
        m_errorString = fr.fileName() + ": Empty track data";
        return false;
    }

    // Build new MIDI events table
    if(!buildSmfTrackData(rawTrackData))
    {
        m_errorString = fr.fileName() + ": MIDI data parsing error has occouped!\n" + m_parsingErrorsString;
        return false;
    }

    m_smfFormat = smfFormat;
    m_loop.stackLevel   = -1;

    return true;
}

bool BW_MidiSequencer::parseRMI(FileAndMemReader &fr)
{
    const size_t headerSize = 4 + 4 + 2 + 2 + 2; // 14
    char headerBuf[headerSize] = "";

    size_t fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "RIFF", 4) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format, RIFF signature is not found!\n";
        return false;
    }

    m_format = Format_MIDI;

    fr.seek(6l, FileAndMemReader::CUR);
    return parseSMF(fr);
}

#ifndef BWMIDI_DISABLE_MUS_SUPPORT
bool BW_MidiSequencer::parseMUS(FileAndMemReader &fr)
{
    const size_t headerSize = 14;
    char headerBuf[headerSize] = "";
    size_t fsize = 0;
    BufferGuard<uint8_t> cvt_buf;

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "MUS\x1A", 4) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format, MUS\\x1A signature is not found!\n";
        return false;
    }

    size_t mus_len = fr.fileSize();

    fr.seek(0, FileAndMemReader::SET);
    uint8_t *mus = (uint8_t *)malloc(mus_len);
    if(!mus)
    {
        m_errorString = "Out of memory!";
        return false;
    }
    fsize = fr.read(mus, 1, mus_len);
    if(fsize < mus_len)
    {
        m_errorString = "Failed to read MUS file data!\n";
        return false;
    }

    // Close source stream
    fr.close();

    uint8_t *mid = NULL;
    uint32_t mid_len = 0;
    int m2mret = Convert_mus2midi(mus, static_cast<uint32_t>(mus_len),
                                  &mid, &mid_len, 0);
    if(mus)
        free(mus);

    if(m2mret < 0)
    {
        m_errorString = "Invalid MUS/DMX data format!";
        return false;
    }
    cvt_buf.set(mid);

    // Open converted MIDI file
    fr.openData(mid, static_cast<size_t>(mid_len));

    return parseSMF(fr);
}
#endif // BWMIDI_DISABLE_MUS_SUPPORT

#ifndef BWMIDI_DISABLE_XMI_SUPPORT
bool BW_MidiSequencer::parseXMI(FileAndMemReader &fr)
{
    const size_t headerSize = 14;
    char headerBuf[headerSize] = "";
    size_t fsize = 0;
//    BufferGuard<uint8_t> cvt_buf;
    std::vector<std::vector<uint8_t > > song_buf;
    bool ret;

    (void)Convert_xmi2midi; /* Shut up the warning */

    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "FORM", 4) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format, FORM signature is not found!\n";
        return false;
    }

    if(std::memcmp(headerBuf + 8, "XDIR", 4) != 0)
    {
        m_errorString = fr.fileName() + ": Invalid format\n";
        return false;
    }

    size_t mus_len = fr.fileSize();
    fr.seek(0, FileAndMemReader::SET);

    uint8_t *mus = (uint8_t*)std::malloc(mus_len + 20);
    if(!mus)
    {
        m_errorString = "Out of memory!";
        return false;
    }

    std::memset(mus, 0, mus_len + 20);

    fsize = fr.read(mus, 1, mus_len);
    if(fsize < mus_len)
    {
        m_errorString = "Failed to read XMI file data!\n";
        return false;
    }

    // Close source stream
    fr.close();

//    uint8_t *mid = NULL;
//    uint32_t mid_len = 0;
    int m2mret = Convert_xmi2midi_multi(mus, static_cast<uint32_t>(mus_len + 20),
                                        song_buf, XMIDI_CONVERT_NOCONVERSION);
    if(mus)
        free(mus);
    if(m2mret < 0)
    {
        song_buf.clear();
        m_errorString = "Invalid XMI data format!";
        return false;
    }

    if(m_loadTrackNumber >= (int)song_buf.size())
        m_loadTrackNumber = song_buf.size() - 1;

    for(size_t i = 0; i < song_buf.size(); ++i)
    {
        m_rawSongsData.push_back(song_buf[i]);
    }

    song_buf.clear();

    // cvt_buf.set(mid);
    // Open converted MIDI file
    fr.openData(m_rawSongsData[m_loadTrackNumber].data(),
                m_rawSongsData[m_loadTrackNumber].size());
    // Set format as XMIDI
    m_format = Format_XMIDI;

    ret = parseSMF(fr);

    return ret;
}
#endif
