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
#ifndef BW_MIDI_SEQUENCER_HHHHPPP
#define BW_MIDI_SEQUENCER_HHHHPPP

#include <list>
#include <vector>

#include "fraction.hpp"
#include "file_reader.hpp"
#include "midi_sequencer.h"

//! Helper for unused values
#define BW_MidiSequencer_UNUSED(x) (void)x;

class BW_MidiSequencer
{
    /**
     * @brief MIDI Event utility container
     */
    class MidiEvent
    {
    public:
        MidiEvent();
        /**
         * @brief Main MIDI event types
         */
        enum Types
        {
            //! Unknown event
            T_UNKNOWN       = 0x00,
            //! Note-Off event
            T_NOTEOFF       = 0x08,//size == 2
            //! Note-On event
            T_NOTEON        = 0x09,//size == 2
            //! Note After-Touch event
            T_NOTETOUCH     = 0x0A,//size == 2
            //! Controller change event
            T_CTRLCHANGE    = 0x0B,//size == 2
            //! Patch change event
            T_PATCHCHANGE   = 0x0C,//size == 1
            //! Channel After-Touch event
            T_CHANAFTTOUCH  = 0x0D,//size == 1
            //! Pitch-bend change event
            T_WHEEL         = 0x0E,//size == 2

            //! System Exclusive message, type 1
            T_SYSEX         = 0xF0,//size == len
            //! Sys Com Song Position Pntr [LSB, MSB]
            T_SYSCOMSPOSPTR = 0xF2,//size == 2
            //! Sys Com Song Select(Song #) [0-127]
            T_SYSCOMSNGSEL  = 0xF3,//size == 1
            //! System Exclusive message, type 2
            T_SYSEX2        = 0xF7,//size == len
            //! Special event
            T_SPECIAL       = 0xFF
        };
        /**
         * @brief Special MIDI event sub-types
         */
        enum SubTypes
        {
            //! Sequension number
            ST_SEQNUMBER    = 0x00,//size == 2
            //! Text label
            ST_TEXT         = 0x01,//size == len
            //! Copyright notice
            ST_COPYRIGHT    = 0x02,//size == len
            //! Sequence track title
            ST_SQTRKTITLE   = 0x03,//size == len
            //! Instrument title
            ST_INSTRTITLE   = 0x04,//size == len
            //! Lyrics text fragment
            ST_LYRICS       = 0x05,//size == len
            //! MIDI Marker
            ST_MARKER       = 0x06,//size == len
            //! Cue Point
            ST_CUEPOINT     = 0x07,//size == len
            //! [Non-Standard] Device Switch
            ST_DEVICESWITCH = 0x09,//size == len <CUSTOM>
            //! MIDI Channel prefix
            ST_MIDICHPREFIX = 0x20,//size == 1

            //! End of Track event
            ST_ENDTRACK     = 0x2F,//size == 0
            //! Tempo change event
            ST_TEMPOCHANGE  = 0x51,//size == 3
            //! SMPTE offset
            ST_SMPTEOFFSET  = 0x54,//size == 5
            //! Time signature
            ST_TIMESIGNATURE = 0x55, //size == 4
            //! Key signature
            ST_KEYSIGNATURE = 0x59,//size == 2
            //! Sequencer specs
            ST_SEQUENCERSPEC = 0x7F, //size == len

            /* Non-standard, internal ADLMIDI usage only */
            //! [Non-Standard] Loop Start point
            ST_LOOPSTART    = 0xE1,//size == 0 <CUSTOM>
            //! [Non-Standard] Loop End point
            ST_LOOPEND      = 0xE2,//size == 0 <CUSTOM>
            //! [Non-Standard] Raw OPL data
            ST_RAWOPL       = 0xE3,//size == 0 <CUSTOM>

            //! [Non-Standard] Loop Start point with support of multi-loops
            ST_LOOPSTACK_BEGIN = 0xE4,//size == 1 <CUSTOM>
            //! [Non-Standard] Loop End point with support of multi-loops
            ST_LOOPSTACK_END   = 0xE5,//size == 0 <CUSTOM>
            //! [Non-Standard] Loop End point with support of multi-loops
            ST_LOOPSTACK_BREAK = 0xE6,//size == 0 <CUSTOM>
            //! [Non-Standard] Callback Trigger
            ST_CALLBACK_TRIGGER = 0xE7,//size == 1 <CUSTOM>

            // Built-in hooks
            ST_SONG_BEGIN_HOOK    = 0x101
        };
        //! Main type of event
        uint_fast16_t type;
        //! Sub-type of the event
        uint_fast16_t subtype;
        //! Targeted MIDI channel
        uint_fast16_t channel;
        //! Is valid event
        uint_fast16_t isValid;
        //! Reserved 5 bytes padding
        uint_fast16_t __padding[4];
        //! Absolute tick position (Used for the tempo calculation only)
        uint64_t absPosition;
        //! Raw data of this event
        std::vector<uint8_t> data;
    };

    /**
     * @brief A track position event contains a chain of MIDI events until next delay value
     *
     * Created with purpose to sort events by type in the same position
     * (for example, to keep controllers always first than note on events or lower than note-off events)
     */
    class MidiTrackRow
    {
    public:
        MidiTrackRow();
        //! Clear MIDI row data
        void clear();
        //! Absolute time position in seconds
        double time;
        //! Delay to next event in ticks
        uint64_t delay;
        //! Absolute position in ticks
        uint64_t absPos;
        //! Delay to next event in seconds
        double timeDelay;
        //! List of MIDI events in the current row
        std::vector<MidiEvent> events;
        /**
         * @brief Sort events in this position
         * @param noteStates Buffer of currently pressed/released note keys in the track
         */
        void sortEvents(bool *noteStates = NULL);
    };

    /**
     * @brief Tempo change point entry. Used in the MIDI data building function only.
     */
    struct TempoChangePoint
    {
        uint64_t absPos;
        fraction<uint64_t> tempo;
    };
    //P.S. I declared it here instead of local in-function because C++98 can't process templates with locally-declared structures

    typedef std::list<MidiTrackRow> MidiTrackQueue;

    /**
     * @brief Song position context
     */
    struct Position
    {
        //! Was track began playing
        bool began;
        //! Reserved
        char __padding[7];
        //! Waiting time before next event in seconds
        double wait;
        //! Absolute time position on the track in seconds
        double absTimePosition;
        //! Track information
        struct TrackInfo
        {
            //! Delay to next event in a track
            uint64_t delay;
            //! Last handled event type
            int32_t lastHandledEvent;
            //! Reserved
            char    __padding2[4];
            //! MIDI Events queue position iterator
            MidiTrackQueue::iterator pos;

            TrackInfo() :
                delay(0),
                lastHandledEvent(0)
            {}
        };
        std::vector<TrackInfo> track;
        Position():
            began(false),
            wait(0.0),
            absTimePosition(0.0),
            track()
        {
            for(size_t i = 0; i < 7; ++i)
                __padding[i] = 0;
        }
    };

    //! MIDI Output interface context
    const BW_MidiRtInterface *m_interface;

    /**
     * @brief Prepare internal events storage for track data building
     * @param trackCount Count of tracks
     */
    void buildSmfSetupReset(size_t trackCount);

    /**
     * @brief Build MIDI track data from the raw track data storage
     * @return true if everything successfully processed, or false on any error
     */
    bool buildSmfTrackData(const std::vector<std::vector<uint8_t> > &trackData);

    /**
     * @brief Build the time line from off loaded events
     * @param tempos Pre-collected list of tempo events
     * @param loopStartTicks Global loop start tick (give zero if no global loop presented)
     * @param loopEndTicks Global loop end tick (give zero if no global loop presented)
     */
    void buildTimeLine(const std::vector<MidiEvent> &tempos,
                       uint64_t loopStartTicks = 0,
                       uint64_t loopEndTicks = 0);

    /**
     * @brief Parse one event from raw MIDI track stream
     * @param [_inout] ptr pointer to pointer to current position on the raw data track
     * @param [_in] end address to end of raw track data, needed to validate position and size
     * @param [_inout] status status of the track processing
     * @return Parsed MIDI event entry
     */
    MidiEvent parseEvent(const uint8_t **ptr, const uint8_t *end, int &status);

    /**
     * @brief Process MIDI events on the current tick moment
     * @param isSeek is a seeking process
     * @return returns false on reaching end of the song
     */
    bool processEvents(bool isSeek = false);

    /**
     * @brief Handle one event from the chain
     * @param tk MIDI track
     * @param evt MIDI event entry
     * @param status Recent event type, -1 returned when end of track event was handled.
     */
    void handleEvent(size_t tk, const MidiEvent &evt, int32_t &status);

public:
    /**
     * @brief MIDI marker entry
     */
    struct MIDI_MarkerEntry
    {
        //! Label
        std::string     label;
        //! Position time in seconds
        double          pos_time;
        //! Position time in MIDI ticks
        uint64_t        pos_ticks;
    };

    /**
     * @brief Container of one raw CMF instrument
     */
    struct CmfInstrument
    {
        //! Raw CMF instrument data
        uint8_t data[16];
    };

    /**
     * @brief The FileFormat enum
     */
    enum FileFormat
    {
        //! MIDI format
        Format_MIDI,
        //! CMF format
        Format_CMF,
        //! Id-Software Music File
        Format_IMF,
        //! EA-MUS format
        Format_RSXX,
        //! AIL's XMIDI format (act same as MIDI, but with exceptions)
        Format_XMIDI
    };

    /**
     * @brief Format of loop points implemented by CC events
     */
    enum LoopFormat
    {
        Loop_Default,
        Loop_RPGMaker = 1,
        Loop_EMIDI,
        Loop_HMI
    };

private:
    //! Music file format type. MIDI is default.
    FileFormat m_format;
    //! SMF format identifier.
    unsigned m_smfFormat;
    //! Loop points format
    LoopFormat m_loopFormat;

    //! Current position
    Position m_currentPosition;
    //! Track begin position
    Position m_trackBeginPosition;
    //! Loop start point
    Position m_loopBeginPosition;

    //! Is looping enabled or not
    bool    m_loopEnabled;
    //! Don't process loop: trigger hooks only if they are set
    bool    m_loopHooksOnly;

    //! Full song length in seconds
    double m_fullSongTimeLength;
    //! Delay after song playd before rejecting the output stream requests
    double m_postSongWaitDelay;

    //! Global loop start time
    double m_loopStartTime;
    //! Global loop end time
    double m_loopEndTime;

    //! Pre-processed track data storage
    std::vector<MidiTrackQueue > m_trackData;

    //! CMF instruments
    std::vector<CmfInstrument> m_cmfInstruments;

    //! Title of music
    std::string m_musTitle;
    //! Copyright notice of music
    std::string m_musCopyright;
    //! List of track titles
    std::vector<std::string> m_musTrackTitles;
    //! List of MIDI markers
    std::vector<MIDI_MarkerEntry> m_musMarkers;

    //! Time of one tick
    fraction<uint64_t> m_invDeltaTicks;
    //! Current tempo
    fraction<uint64_t> m_tempo;

    //! Tempo multiplier factor
    double  m_tempoMultiplier;
    //! Is song at end
    bool    m_atEnd;

    //! Set the number of loops limit. Lesser than 0 - loop infinite
    int     m_loopCount;

    //! The number of track of multi-track file (for exmaple, XMI) to load
    int     m_loadTrackNumber;

    //! The XMI-specific list of raw songs, converted into SMF format
    std::vector<std::vector<uint8_t > > m_rawSongsData;

    /**
     * @brief Loop stack entry
     */
    struct LoopStackEntry
    {
        LoopStackEntry() :
            infinity(false),
            loops(0),
            start(0),
            end(0)
        {}

        //! is infinite loop
        bool infinity;
        //! Count of loops left to break. <0 - infinite loop
        int loops;
        //! Start position snapshot to return back
        Position startPosition;
        //! Loop start tick
        uint64_t start;
        //! Loop end tick
        uint64_t end;
    };

    struct LoopState
    {
        //! Loop start has reached
        bool    caughtStart;
        //! Loop end has reached, reset on handling
        bool    caughtEnd;

        //! Loop start has reached
        bool    caughtStackStart;
        //! Loop next has reached, reset on handling
        bool    caughtStackEnd;
        //! Loop break has reached, reset on handling
        bool    caughtStackBreak;
        //! Skip next stack loop start event handling
        bool    skipStackStart;

        //! Are loop points invalid?
        bool    invalidLoop; /*Loop points are invalid (loopStart after loopEnd or loopStart and loopEnd are on same place)*/

        //! Is look got temporarily broken because of post-end seek?
        bool    temporaryBroken;

        //! How much times the loop should start repeat? For example, if you want to loop song twice, set value 1
        int     loopsCount;

        //! how many loops left until finish the song
        int     loopsLeft;

        //! Stack of nested loops
        std::vector<LoopStackEntry> stack;
        //! Current level on the loop stack (<0 - out of loop, 0++ - the index in the loop stack)
        int                         stackLevel;

        /**
         * @brief Reset loop state to initial
         */
        void reset()
        {
            caughtStart = false;
            caughtEnd = false;
            caughtStackStart = false;
            caughtStackEnd = false;
            caughtStackBreak = false;
            skipStackStart = false;
            loopsLeft = loopsCount;
        }

        void fullReset()
        {
            loopsCount = -1;
            reset();
            invalidLoop = false;
            temporaryBroken = false;
            stack.clear();
            stackLevel = -1;
        }

        bool isStackEnd()
        {
            if(caughtStackEnd && (stackLevel >= 0) && (stackLevel < static_cast<int>(stack.size())))
            {
                const LoopStackEntry &e = stack[static_cast<size_t>(stackLevel)];
                if(e.infinity || (!e.infinity && e.loops > 0))
                    return true;
            }
            return false;
        }

        void stackUp(int count = 1)
        {
            stackLevel += count;
        }

        void stackDown(int count = 1)
        {
            stackLevel -= count;
        }

        LoopStackEntry &getCurStack()
        {
            if((stackLevel >= 0) && (stackLevel < static_cast<int>(stack.size())))
                return stack[static_cast<size_t>(stackLevel)];
            if(stack.empty())
            {
                LoopStackEntry d;
                d.loops = 0;
                d.infinity = 0;
                d.start = 0;
                d.end = 0;
                stack.push_back(d);
            }
            return stack[0];
        }
    } m_loop;

    //! Whether the nth track has playback disabled
    std::vector<bool> m_trackDisable;
    //! Index of solo track, or max for disabled
    size_t m_trackSolo;
    //! MIDI channel disable (exception for extra port-prefix-based channels)
    bool m_channelDisable[16];

    /**
     * @brief Handler of callback trigger events
     * @param userData Pointer to user data (usually, context of something)
     * @param trigger Value of the event which triggered this callback.
     * @param track Identifier of the track which triggered this callback.
     */
    typedef void (*TriggerHandler)(void *userData, unsigned trigger, size_t track);

    //! Handler of callback trigger events
    TriggerHandler m_triggerHandler;
    //! User data of callback trigger events
    void *m_triggerUserData;

    //! File parsing errors string (adding into m_errorString on aborting of the process)
    std::string m_parsingErrorsString;
    //! Common error string
    std::string m_errorString;

    struct SequencerTime
    {
        //! Time buffer
        double   timeRest;
        //! Sample rate
        uint32_t sampleRate;
        //! Size of one frame in bytes
        uint32_t frameSize;
        //! Minimum possible delay, granuality
        double minDelay;
        //! Last delay
        double delay;

        void init()
        {
            sampleRate = 44100;
            frameSize = 2;
            reset();
        }

        void reset()
        {
            timeRest = 0.0;
            minDelay = 1.0 / static_cast<double>(sampleRate);
            delay = 0.0;
        }
    } m_time;

public:
    BW_MidiSequencer();
    virtual ~BW_MidiSequencer();

    /**
     * @brief Sets the RT interface
     * @param intrf Pre-Initialized interface structure (pointer will be taken)
     */
    void setInterface(const BW_MidiRtInterface *intrf);

    /**
     * @brief Runs ticking in a sync with audio streaming. Use this together with onPcmRender hook to easily play MIDI.
     * @param stream pointer to the output PCM stream
     * @param length length of the buffer in bytes
     * @return Count of recorded data in bytes
     */
    int playStream(uint8_t *stream, size_t length);

    /**
     * @brief Returns file format type of currently loaded file
     * @return File format type enumeration
     */
    FileFormat getFormat();

    /**
     * @brief Returns the number of tracks
     * @return Track count
     */
    size_t getTrackCount() const;

    /**
     * @brief Sets whether a track is playing
     * @param track Track identifier
     * @param enable Whether to enable track playback
     * @return true on success, false if there was no such track
     */
    bool setTrackEnabled(size_t track, bool enable);

    /**
     * @brief Disable/enable a channel is sounding
     * @param channel Channel number from 0 to 15
     * @param enable Enable the channel playback
     * @return true on success, false if there was no such channel
     */
    bool setChannelEnabled(size_t channel, bool enable);

    /**
     * @brief Enables or disables solo on a track
     * @param track Identifier of solo track, or max to disable
     */
    void setSoloTrack(size_t track);

    /**
     * @brief Set the song number of a multi-song file (such as XMI)
     * @param trackNumber Identifier of the song to load (or -1 to mix all songs as one song)
     */
    void setSongNum(int track);

    /**
     * @brief Retrive the number of songs in a currently opened file
     * @return Number of songs in the file. If 1 or less, means, the file has only one song inside.
     */
    int getSongsCount();

    /**
     * @brief Defines a handler for callback trigger events
     * @param handler Handler to invoke from the sequencer when triggered, or NULL.
     * @param userData Instance of the library
     */
    void setTriggerHandler(TriggerHandler handler, void *userData);

    /**
     * @brief Get the list of CMF instruments (CMF only)
     * @return Array of raw CMF instruments entries
     */
    const std::vector<CmfInstrument> getRawCmfInstruments();

    /**
     * @brief Get string that describes reason of error
     * @return Error string
     */
    const std::string &getErrorString();

    /**
     * @brief Check is loop enabled
     * @return true if loop enabled
     */
    bool getLoopEnabled();

    /**
     * @brief Switch loop on/off
     * @param enabled Enable loop
     */
    void setLoopEnabled(bool enabled);

    /**
     * @brief Get the number of loops set
     * @return number of loops or -1 if loop infinite
     */
    int getLoopsCount();

    /**
     * @brief How many times song should loop
     * @param loops count or -1 to loop infinite
     */
    void setLoopsCount(int loops);

    /**
     * @brief Switch loop hooks-only mode on/off
     * @param enabled Don't loop: trigger hooks only without loop
     */
    void setLoopHooksOnly(bool enabled);

    /**
     * @brief Get music title
     * @return music title string
     */
    const std::string &getMusicTitle();

    /**
     * @brief Get music copyright notice
     * @return music copyright notice string
     */
    const std::string &getMusicCopyright();

    /**
     * @brief Get list of track titles
     * @return array of track title strings
     */
    const std::vector<std::string> &getTrackTitles();

    /**
     * @brief Get list of MIDI markers
     * @return Array of MIDI marker structures
     */
    const std::vector<MIDI_MarkerEntry> &getMarkers();

    /**
     * @brief Is position of song at end
     * @return true if end of song was reached
     */
    bool positionAtEnd();

    /**
     * @brief Load MIDI file from path
     * @param filename Path to file to open
     * @return true if file successfully opened, false on any error
     */
    bool loadMIDI(const std::string &filename);

    /**
     * @brief Load MIDI file from a memory block
     * @param data Pointer to memory block with MIDI data
     * @param size Size of source memory block
     * @return true if file successfully opened, false on any error
     */
    bool loadMIDI(const void *data, size_t size);

    /**
     * @brief Load MIDI file by using FileAndMemReader interface
     * @param fr FileAndMemReader context with opened source file
     * @return true if file successfully opened, false on any error
     */
    bool loadMIDI(FileAndMemReader &fr);

    /**
     * @brief Periodic tick handler.
     * @param s seconds since last call
     * @param granularity don't expect intervals smaller than this, in seconds
     * @return desired number of seconds until next call
     */
    double Tick(double s, double granularity);

    /**
     * @brief Change current position to specified time position in seconds
     * @param granularity don't expect intervals smaller than this, in seconds
     * @param seconds Absolute time position in seconds
     * @return desired number of seconds until next call of Tick()
     */
    double seek(double seconds, const double granularity);

    /**
     * @brief Gives current time position in seconds
     * @return Current time position in seconds
     */
    double  tell();

    /**
     * @brief Gives time length of current song in seconds
     * @return Time length of current song in seconds
     */
    double  timeLength();

    /**
     * @brief Gives loop start time position in seconds
     * @return Loop start time position in seconds or -1 if song has no loop points
     */
    double  getLoopStart();

    /**
     * @brief Gives loop end time position in seconds
     * @return Loop end time position in seconds or -1 if song has no loop points
     */
    double  getLoopEnd();

    /**
     * @brief Return to begin of current song
     */
    void    rewind();

    /**
     * @brief Get current tempor multiplier value
     * @return
     */
    double getTempoMultiplier();

    /**
     * @brief Set tempo multiplier
     * @param tempo Tempo multiplier: 1.0 - original tempo. >1 - faster, <1 - slower
     */
    void   setTempo(double tempo);

private:
    /**
     * @brief Load file as Id-software-Music-File (Wolfenstein)
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseIMF(FileAndMemReader &fr);

    /**
     * @brief Load file as EA MUS
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseRSXX(FileAndMemReader &fr);

    /**
     * @brief Load file as Creative Music Format
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseCMF(FileAndMemReader &fr);

    /**
     * @brief Load file as GMD/MUS files (ScummVM)
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseGMF(FileAndMemReader &fr);

    /**
     * @brief Load file as Standard MIDI file
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseSMF(FileAndMemReader &fr);

    /**
     * @brief Load file as RIFF MIDI
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseRMI(FileAndMemReader &fr);

#ifndef BWMIDI_DISABLE_MUS_SUPPORT
    /**
     * @brief Load file as DMX MUS file (Doom)
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseMUS(FileAndMemReader &fr);
#endif

#ifndef BWMIDI_DISABLE_XMI_SUPPORT
    /**
     * @brief Load file as AIL eXtended MIdi
     * @param fr Context with opened file
     * @return true on successful load
     */
    bool parseXMI(FileAndMemReader &fr);
#endif

};

#endif /* BW_MIDI_SEQUENCER_HHHHPPP */
