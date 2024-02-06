Attribute VB_Name = "SDL2_mixer_ext_vb6"
Option Explicit

Const SDL_AUDIO_DRIVER_DISK As Long = 1
Const SDL_AUDIO_DRIVER_DUMMY As Long = 1
Const SDL_AUDIO_DRIVER_DSOUND As Long = 1
Const SDL_AUDIO_DRIVER_WINMM As Long = 1

Const AUDIO_U8 As Long = &H8
Const AUDIO_S8 As Long = &H8008
Const AUDIO_U16LSB As Long = &H10
Const AUDIO_S16LSB As Long = &H8010
Const AUDIO_U16MSB As Long = &H1010
Const AUDIO_S16MSB As Long = &H9010
Const AUDIO_S32LSB As Long = &H8020
Const AUDIO_S32MSB As Long = &H9020
Const AUDIO_F32LSB As Long = &H8120
Const AUDIO_F32MSB As Long = &H9120

Const SDL_AUDIO_ALLOW_FREQUENCY_CHANGE As Long = &H1
Const SDL_AUDIO_ALLOW_FORMAT_CHANGE As Long = &H2
Const SDL_AUDIO_ALLOW_CHANNELS_CHANGE As Long = &H4

Const SDL_MIX_MAXVOLUME As Long = 128
Const SDL_MAJOR_VERSION As Long = 2
Const SDL_MINOR_VERSION As Long = 0
Const SDL_PATCHLEVEL As Long = 2
Const SDL_MIXER_MAJOR_VERSION As Long = 2
Const SDL_MIXER_MINOR_VERSION As Long = 0
Const SDL_MIXER_PATCHLEVEL As Long = 0
Const MIX_CHANNELS As Long = 8
Const MIX_DEFAULT_FREQUENCY As Long = 22050
Const MIX_DEFAULT_CHANNELS As Long = 2
Const MIX_MAX_VOLUME As Long = 128
Const MIX_EFFECTSMAXSPEED As String = "MIX_EFFECTSMAXSPEED"

Const RW_SEEK_SET As Long = 0 '/**< Seek from the beginning of data */
Const RW_SEEK_CUR As Long = 1 '/**< Seek relative to current read point */
Const RW_SEEK_END As Long = 2 '/**< Seek relative to the end of data */

Public Type SDL_AudioSpec
        Freq As Long
        Format As Long
        Channels As Long
        Silence As Long
        Samples As Long
        Padding As Long
        Size As Long
        Callback As Long
        UserData As Long
End Type

Public Type Mix_Chunk
        Allocated As Long
        Abuf As Long
        Alen As Long
        Volume As Long
End Type

Public Enum Mix_Fading
    MIX_NO_FADING = 0
    MIX_FADING_OUT
    MIX_FADING_IN
End Enum

Public Enum Mix_MusicType
    MUS_NONE = 0
    MUS_CMD
    MUS_WAV
    MUS_MOD
    MUS_MID
    MUS_OGG
    MUS_MP3
    MUS_MP3_MAD_UNUSED
    MUS_FLAC
    MUS_MODPLUG_UNUSED
    MUS_GME
    MUS_ADLMIDI
End Enum

Public Enum Mix_MIDI_Device
    MIDI_ADLMIDI = 0
    MIDI_Native
    MIDI_Timidity
    MIDI_OPNMIDI
    MIDI_Fluidsynth
    MIDI_ANY
    MIDI_KnuwnDevices
End Enum

Public Enum Mix_ADLMIDI_VolumeModel
    ADLMIDI_VM_AUTO = 0
    ADLMIDI_VM_GENERIC
    ADLMIDI_VM_CMF
    ADLMIDI_VM_DMX
    ADLMIDI_VM_APOGEE
    ADLMIDI_VM_9X
End Enum

'int Mix_Init(int flags);
'Loads dynamic libraries and prepares them for use.  Flags should be
'one or more flags from MIX_InitFlags OR'd together.
'It returns the flags successfully initialized, or 0 on failure.
Public Declare Function Mix_Init Lib "SDL2MixerVB.dll" Alias "Mix_InitVB6" () As Long
'Public Declare Function Mix_Init Lib "SDL2MixerVB.dll" (ByVal flags As Long) As Long

''''''''''''''''''''''''''''SDL Functions'''''''''''''''''''''''''''''''''''''''''
'This function calls SDL_Init(SDL_INIT_AUDIO)
Public Declare Function SDL_InitAudio Lib "SDL2MixerVB.dll" () As Long
Public Declare Sub SDL_Quit Lib "SDL2MixerVB.dll" Alias "SDL_QuitVB6" ()
Public Declare Function SDL_GetError Lib "SDL2MixerVB.dll" Alias "SDL_GetErrorVB6" () As String
''''''''''''''''''''''''''''SDL Functions'''END'''''''''''''''''''''''''''''''''''

'void Mix_Quit(void);
'Unloads libraries loaded with Mix_Init
Public Declare Sub Mix_Quit Lib "SDL2MixerVB.dll" ()

'Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
'Open the mixer with a certain audio format
Public Declare Function Mix_OpenAudio Lib "SDL2MixerVB.dll" _
                Alias "Mix_OpenAudioVB6" _
                (ByVal Frequency As Long, _
                 ByVal Format As Long, _
                 ByVal Channels As Long, _
                 ByVal ChunkSize As Long) As Integer
'Public Declare Function Mix_OpenAudio Lib "SDL2MixerVB.dll" (ByVal frequency As Long, ByVal format As Integer, ByVal channels As Long, ByVal chunksize As Long) As Long

'extern DECLSPEC int SDLCALL Mix_AllocateChannels(int numchans);
'Dynamically change the number of channels managed by the mixer.
'If decreasing the number of channels, the upper channels are stopped.
'This function returns the new number of allocated channels.
Public Declare Function Mix_AllocateChannels Lib "SDL2MixerVB.dll" (ByVal numchans As Long) As Long

'Mix_Music * Mix_LoadMUS(const char *file);
'Load a wave file or a music (.mod .s3m .it .xm) file
'List of all supported musics are here: http://engine.wohlnet.ru/pgewiki/SDL2_mixer
Public Declare Function Mix_LoadMUS Lib "SDL2MixerVB.dll" (ByVal file As String) As Long

'Mix_Chunk * Mix_LoadWAV(const char* file);
Public Declare Function Mix_LoadWAV Lib "SDL2MixerVB.dll" Alias "Mix_LoadWAV_VB6" (ByVal file As String) As Long

'Mix_Chunk * Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);
'       rwops - an SDL_rwops structure pointer (in VB is a Long value!)
'       freesrc - close and destroy SDL_rwpos structure (you no need free it after!)
Public Declare Function Mix_LoadWAV_RW Lib "SDL2MixerVB.dll" (ByVal RWops As Long, ByVal freesrc As Long) As Long

'/* Load a music file from an SDL_RWop object (Ogg and MikMod specific currently)
' Matt Campbell (matt@campbellhome.dhs.org) April 2000 */
'extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUS_RW(SDL_RWops *src, int freesrc);
'       rwops - an SDL_rwops structure pointer (in VB is a Long value!)
'       freesrc - close and destroy SDL_rwpos structure (you no need free it after!)
Public Declare Function Mix_LoadMUS_RW Lib "SDL2MixerVB.dll" _
                        (ByVal RWops As Long, ByVal freesrc As Long) As Long

'extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUS_RW_ARG(SDL_RWops *src, int freesrc, char *args);
'Load a music file from an SDL_RWop object with custom arguments (trackID for GME or settings for a MIDI playing)
'Arguments are taking no effect for file formats which are not supports extra arguments.
Public Declare Function Mix_LoadMUS_RW_ARG Lib "SDL2MixerVB.dll" _
                        (ByVal RWops As Long, ByVal freesrc As Long, Optional ByVal args As String = "") As Long

'extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUS_RW_GME(SDL_RWops *src, int freesrc, int trackID=0);
'       rwops - an SDL_rwops structure pointer (in VB is a Long value!)
'       freesrc - close and destroy SDL_rwpos structure (you no need free it after!)
'       trackID - ID of a track inside NES,HES,GBM,etc. file. Doesn't takes effect on any other regular formats.
Public Declare Function Mix_LoadMUS_RW_GME Lib "SDL2MixerVB.dll" _
                        (ByVal RWops As Long, ByVal freesrc As Long, Optional ByVal trackID As Long = 0) As Long

'/* Load a music file from an SDL_RWop object assuming a specific format */
'extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUSType_RW(SDL_RWops *src, Mix_MusicType type, int freesrc);
Public Declare Function Mix_LoadMUSType_RW Lib "SDL2MixerVB.dll" _
                        (ByVal RWops As Long, ByVal mustype As Mix_MusicType, ByVal freesrc As Long) As Long

'extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUSType_RW_ARG(SDL_RWops *src, Mix_MusicType type, int freesrc, const char *args);
Public Declare Function Mix_LoadMUSType_RW_ARG Lib "SDL2MixerVB.dll" _
                        (ByVal RWops As Long, ByVal mustype As Mix_MusicType, ByVal freesrc As Long, Optional ByVal args As String = "") As Long

'/* Load a wave file of the mixer format from a memory buffer */
'extern DECLSPEC Mix_Chunk * SDLCALL Mix_QuickLoad_WAV(Uint8 *mem);

'/* Load raw audio data of the mixer format from a memory buffer */
'extern DECLSPEC Mix_Chunk * SDLCALL Mix_QuickLoad_RAW(Uint8 *mem, Uint32 len);


'Free an audio chunk previously loaded
'void Mix_FreeChunk(Mix_Chunk *chunk);
Public Declare Sub Mix_FreeChunk Lib "SDL2MixerVB.dll" (ByVal Chunk As Long)
'void Mix_FreeMusic(Mix_Music *music);
Public Declare Sub Mix_FreeMusic Lib "SDL2MixerVB.dll" (ByVal music As Long)


'Get a list of chunk/music decoders that this build of SDL_mixer provides.
'This list can change between builds AND runs of the program, if external
'libraries that add functionality become available.
'You must successfully call Mix_OpenAudio() before calling these functions.
'This API is only available in SDL_mixer 1.2.9 and later.
'
'// usage...
'int i;
'const int total = Mix_GetNumChunkDecoders();
'for (i = 0; i < total; i++)
'    printf("Supported chunk decoder: [%s]\n", Mix_GetChunkDecoder(i));
'
'Appearing in this list doesn't promise your specific audio file will
'decode...but it's handy to know if you have, say, a functioning Timidity
'install.
'
'These return values are static, read-only data; do not modify or free it.
'The pointers remain valid until you call Mix_CloseAudio().

'int Mix_GetNumChunkDecoders(void);
Public Declare Function Mix_GetNumChunkDecoders Lib "SDL2MixerVB.dll" () As Long
'const char * Mix_GetChunkDecoder(int index);
Public Declare Function Mix_GetChunkDecoder Lib "SDL2MixerVB.dll" (ByVal index As Long) As String
'extern DECLSPEC int SDLCALL Mix_GetNumMusicDecoders(void);
Public Declare Function Mix_GetNumMusicDecoders Lib "SDL2MixerVB.dll" () As Long
'extern DECLSPEC const char * SDLCALL Mix_GetMusicDecoder(int index);
Public Declare Function Mix_GetMusicDecoder Lib "SDL2MixerVB.dll" (ByVal index As Long) As String

'Mix_MusicType Mix_GetMusicType(const Mix_Music *music);
'Find out the music format of a mixer music, or the currently playing
'music, if 'music' is NULL.
Public Declare Function Mix_GetMusicType Lib "SDL2MixerVB.dll" (ByVal music As Long) As Long

'/* Get music title from meta-tag if possible. If title tag is empty, filename will be returned */
'const char* Mix_GetMusicTitle(const Mix_Music *music);
Public Declare Function Mix_GetMusicTitle Lib "SDL2MixerVB.dll" (ByVal music As Long) As String

'/* Get music title from meta-tag if possible */
'const char* Mix_GetMusicTitleTag(const Mix_Music *music);
Public Declare Function Mix_GetMusicTitleTag Lib "SDL2MixerVB.dll" (ByVal music As Long) As String

'/* Get music artist from meta-tag if possible */
'const char* Mix_GetMusicArtistTag(const Mix_Music *music);
Public Declare Function Mix_GetMusicArtistTag Lib "SDL2MixerVB.dll" (ByVal music As Long) As String

'/* Get music album from meta-tag if possible */
'const char* Mix_GetMusicAlbumTag(const Mix_Music *music);
Public Declare Function Mix_GetMusicAlbumTag Lib "SDL2MixerVB.dll" (ByVal music As Long) As String

'/* Get music copyright from meta-tag if possible */
'const char* Mix_GetMusicCopyrightTag(const Mix_Music *music);
Public Declare Function Mix_GetMusicCopyrightTag Lib "SDL2MixerVB.dll" (ByVal music As Long) As String


'/* Set the panning of a channel. The left and right channels are specified
'int Mix_SetPanning(int channel, Uint8 left, Uint8 right);
Public Declare Function Mix_SetPanning Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Left As Integer, ByVal Right As Integer) As Long

'/* Set the position of a channel. (angle) is an integer from 0 to 360
'int Mix_SetPosition(int channel, Sint16 angle, Uint8 distance);
Public Declare Function Mix_SetPosition Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Angle As Integer, ByVal Distance As Integer) As Long

'/* Set the "distance" of a channel. (distance) is an integer from 0 to 255
'int Mix_SetDistance(int channel, Uint8 distance);
Public Declare Function Mix_SetDistance Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Distance As Integer) As Long

'/* Causes a channel to reverse its stereo.
'int Mix_SetReverseStereo(int channel, int flip);
Public Declare Function Mix_SetReverseStereo Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal flip As Long) As Long



'/* Reserve the first channels (0 -> n-1) for the application, i.e. don't allocate
'   them dynamically to the next sample if requested with a -1 value below.
'   Returns the number of reserved channels.
' */
'int Mix_ReserveChannels(int num);
Public Declare Function Mix_ReserveChannels Lib "SDL2MixerVB.dll" (ByVal Num As Long) As Long

'/* Attach a tag to a channel. A tag can be assigned to several mixer
'   channels, to form groups of channels.
'   If 'tag' is -1, the tag is removed (actually -1 is the tag used to
'   represent the group of all the channels).
'   Returns true if everything was OK.
' */
'int Mix_GroupChannel(int which, int tag);
Public Declare Function Mix_GroupChannel Lib "SDL2MixerVB.dll" (ByVal which As Long, ByVal tag As Long) As Long

'/* Assign several consecutive channels to a group */
'int Mix_GroupChannels(int from, int to, int tag);
Public Declare Function Mix_GroupChannels Lib "SDL2MixerVB.dll" (ByVal groupfrom As Long, ByVal groupto As Long, ByVal tag As Long) As Long

'/* Finds the first available channel in a group of channels,
'   returning -1 if none are available.
' */
'extern DECLSPEC int SDLCALL Mix_GroupAvailable(int tag);
Public Declare Function Mix_GroupAvailable Lib "SDL2MixerVB.dll" (ByVal tag As Long) As Long

'/* Returns the number of channels in a group. This is also a subtle
'   way to get the total number of channels when 'tag' is -1
' */
'int Mix_GroupCount(int tag);
Public Declare Function Mix_GroupCount Lib "SDL2MixerVB.dll" (ByVal tag As Long) As Long

'/* Finds the "oldest" sample playing in a group of channels */
'int Mix_GroupOldest(int tag);
Public Declare Function Mix_GroupOldest Lib "SDL2MixerVB.dll" (ByVal tag As Long) As Long

'/* Finds the "most recent" (i.e. last) sample playing in a group of channels */
'int Mix_GroupNewer(int tag);
Public Declare Function Mix_GroupNewer Lib "SDL2MixerVB.dll" (ByVal tag As Long) As Long

'/* Play an audio chunk on a specific channel.
'   If the specified channel is -1, play on the first free channel.
'   If 'loops' is greater than zero, loop the sound that many times.
'   If 'loops' is -1, loop inifinitely (~65000 times).
'   Returns which channel was used to play the sound.
'*/
'#define Mix_PlayChannel(channel,chunk,loops) Mix_PlayChannelTimed(channel,chunk,loops,-1)
'int Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
Public Declare Function Mix_PlayChannel Lib "SDL2MixerVB.dll" Alias "Mix_PlayChannelTimed" (ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, Optional ByVal ticks As Long = -1) As Long
Public Declare Function Mix_PlayChannelTimed Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, ByVal ticks As Long) As Long
'extern DECLSPEC int SDLCALL Mix_PlayChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ticks, int volume);/*MIXER-X*/
Public Declare Function Mix_PlayChannelTimedVolume Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, ByVal ticks As Long, ByVal Volume As Long) As Long
'#define Mix_PlayChannelVol(channel,chunk,loops,vol) Mix_PlayChannelTimedVolume(channel,chunk,loops,-1,vol)/*MIXER-X*/
'==== See Function Delcarison of 'Mix_PlayChannelVol' in bottom

'int Mix_PlayMusic(Mix_Music *music, int loops);
Public Declare Function Mix_PlayMusic Lib "SDL2MixerVB.dll" (ByVal music As Long, ByVal loops As Long) As Long

'/* Fade in music or a channel over "ms" milliseconds, same semantics as the "Play" functions */
'int Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
Public Declare Function Mix_FadeInMusic Lib "SDL2MixerVB.dll" (ByVal music As Long, ByVal loops As Long, ByVal milliseconds As Long) As Long
'int Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position);
Public Declare Function Mix_FadeInMusicPos Lib "SDL2MixerVB.dll" (ByVal music As Long, ByVal loops As Long, ByVal milliseconds As Long, ByVal position As Double) As Long

'#define Mix_FadeInChannel(channel,chunk,loops,ms) Mix_FadeInChannelTimed(channel,chunk,loops,ms,-1)
Public Declare Function Mix_FadeInChannel Lib "SDL2MixerVB.dll" Alias "Mix_FadeInChannelTimed" (ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, ByVal milliseconds As Long, Optional ByVal ticks As Long = -1) As Long
'extern DECLSPEC int SDLCALL Mix_FadeInChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ms, int ticks);
Public Declare Function Mix_FadeInChannelTimed Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, ByVal milliseconds As Long, ByVal ticks As Long) As Long
'extern DECLSPEC int SDLCALL Mix_FadeInChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ms, int ticks, int volume);/*MIXER-X*/
Public Declare Function Mix_FadeInChannelTimedVolume Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, ByVal milliseconds As Long, ByVal ticks As Long, ByVal Volume As Long) As Long

'/* Set the volume in the range of 0-128 of a specific channel or chunk.
'   If the specified channel is -1, set volume for all channels.
'   Returns the original volume.
'   If the specified volume is -1, just return the current volume.
'*/
'int Mix_Volume(int channel, int volume);
Public Declare Function Mix_Volume Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal Volume As Long) As Long

'int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
Public Declare Function Mix_VolumeChunk Lib "SDL2MixerVB.dll" (ByVal Chunk As Long, ByVal Volume As Long) As Long

'int Mix_VolumeMusic(int volume);
Public Declare Function Mix_VolumeMusic Lib "SDL2MixerVB.dll" (ByVal Volume As Long) As Long


'/* Halt playing of a particular channel */
'extern DECLSPEC int SDLCALL Mix_HaltChannel(int channel);
Public Declare Function Mix_HaltChannel Lib "SDL2MixerVB.dll" (ByVal Channel As Long) As Long

'extern DECLSPEC int SDLCALL Mix_HaltGroup(int tag);
Public Declare Function Mix_HaltGroup Lib "SDL2MixerVB.dll" (ByVal tag As Long) As Long

'extern DECLSPEC int SDLCALL Mix_HaltMusic(void);
Public Declare Function Mix_HaltMusic Lib "SDL2MixerVB.dll" () As Long

'/* Change the expiration delay for a particular channel.
'   The sample will stop playing after the 'ticks' milliseconds have elapsed,
'   or remove the expiration if 'ticks' is -1
'*/
'int Mix_ExpireChannel(int channel, int ticks);
Public Declare Function Mix_ExpireChannel Lib "SDL2MixerVB.dll" (ByVal Channel As Long, ByVal ticks As Long) As Long

'/* Halt a channel, fading it out progressively till it's silent
'   The ms parameter indicates the number of milliseconds the fading
'   will take.
' */
'int Mix_FadeOutChannel(int which, int ms);
Public Declare Function Mix_FadeOutChannel Lib "SDL2MixerVB.dll" (ByVal which As Long, ByVal milliseconds As Long) As Long

'int Mix_FadeOutGroup(int tag, int ms);
Public Declare Function Mix_FadeOutGroup Lib "SDL2MixerVB.dll" (ByVal tag As Long, ByVal milliseconds As Long) As Long

'int Mix_FadeOutMusic(int ms);
Public Declare Function Mix_FadeOutMusic Lib "SDL2MixerVB.dll" (ByVal milliseconds As Long) As Long

'/* Query the fading status of a channel */
'Mix_Fading Mix_FadingMusic(void); //Mix_Fading - enum!
Public Declare Function Mix_FadingMusic Lib "SDL2MixerVB.dll" () As Long

'Mix_Fading Mix_FadingChannel(int which); //Mix_Fading - enum!
Public Declare Function Mix_FadingChannel Lib "SDL2MixerVB.dll" (ByVal which As Long) As Long


'/* Pause/Resume a particular channel */
'void Mix_Pause(int channel);
Public Declare Sub Mix_Pause Lib "SDL2MixerVB.dll" (ByVal Channel As Long)

'void Mix_Resume(int channel);
Public Declare Sub Mix_Resume Lib "SDL2MixerVB.dll" (ByVal Channel As Long)

'int Mix_Paused(int channel);
Public Declare Function Mix_Paused Lib "SDL2MixerVB.dll" (ByVal Channel As Long) As Long


'/* Pause/Resume the music stream */
'void Mix_PauseMusic(void);
Public Declare Sub Mix_PauseMusic Lib "SDL2MixerVB.dll" ()

'void Mix_ResumeMusic(void);
Public Declare Sub Mix_ResumeMusic Lib "SDL2MixerVB.dll" ()

'void Mix_RewindMusic(void);
Public Declare Sub Mix_RewindMusic Lib "SDL2MixerVB.dll" ()

'int  Mix_PausedMusic(void);
Public Declare Function Mix_PausedMusic Lib "SDL2MixerVB.dll" () As Long


'/* Set the current position in the music stream.
'   This returns 0 if successful, or -1 if it failed or isn't implemented.
'   This function is only implemented for MOD music formats (set pattern
'   order number) and for OGG, FLAC, MP3_MAD, and MODPLUG music (set
'   position in seconds), at the moment.
'*/
'int Mix_SetMusicPosition(double position);
Public Declare Function Mix_SetMusicPosition Lib "SDL2MixerVB.dll" (ByVal position As Double) As Long


'/* Check the status of a specific channel.
'   If the specified channel is -1, check all channels.
'*/
'int Mix_Playing(int channel);
Public Declare Function Mix_Playing Lib "SDL2MixerVB.dll" (ByVal Channel As Long) As Long

'int Mix_PlayingMusic(void);
Public Declare Function Mix_PlayingMusic Lib "SDL2MixerVB.dll" () As Long

'/* Get the Mix_Chunk currently associated with a mixer channel
'    Returns NULL if it's an invalid channel, or there's no chunk associated.'
'*/
'Mix_Chunk * Mix_GetChunk(int channel);
Public Declare Function Mix_GetChunk Lib "SDL2MixerVB.dll" (ByVal Channel As Long) As Long


'extern DECLSPEC void SDLCALL Mix_CloseAudio(void);
Public Declare Sub Mix_CloseAudio Lib "SDL2MixerVB.dll" ()

'/* Add additional Timidity bank path */
'void MIX_Timidity_addToPathList(const char *path);
Public Declare Sub MIX_Timidity_addToPathList Lib "SDL2MixerVB.dll" (ByVal Path As String)


'/* ADLMIDI Setup functions */
'int    MIX_ADLMIDI_getTotalBanks();
Public Declare Function MIX_ADLMIDI_getTotalBanks Lib "SDL2MixerVB.dll" () As Long
'const char *const* MIX_ADLMIDI_getBankNames();
'const char *MIX_ADLMIDI_getBankName(int bankID)
Public Declare Function MIX_ADLMIDI_getBankName Lib "SDL2MixerVB.dll" (ByVal bankID As Long) As String

'int  MIX_ADLMIDI_getBankID();
Public Declare Function MIX_ADLMIDI_getBankID Lib "SDL2MixerVB.dll" () As Long
'void MIX_ADLMIDI_setBankID(int bnk);
Public Declare Sub MIX_ADLMIDI_setBankID Lib "SDL2MixerVB.dll" (ByVal bankID As Long)

'int  MIX_ADLMIDI_getTremolo();
Public Declare Function MIX_ADLMIDI_getTremolo Lib "SDL2MixerVB.dll" () As Long
'void MIX_ADLMIDI_setTremolo(int tr);
Public Declare Sub MIX_ADLMIDI_setTremolo Lib "SDL2MixerVB.dll" (ByVal tr As Long)

'int  MIX_ADLMIDI_getVibrato();
Public Declare Function MIX_ADLMIDI_getVibrato Lib "SDL2MixerVB.dll" () As Long
'void MIX_ADLMIDI_setVibrato(int vib);
Public Declare Sub MIX_ADLMIDI_setVibrato Lib "SDL2MixerVB.dll" (ByVal vib As Long)

'int  MIX_ADLMIDI_getScaleMod();
Public Declare Function MIX_ADLMIDI_getScaleMod Lib "SDL2MixerVB.dll" () As Long
'void MIX_ADLMIDI_setScaleMod(int sc);
Public Declare Sub MIX_ADLMIDI_setScaleMod Lib "SDL2MixerVB.dll" (ByVal sc As Long)

'int  Mix_ADLMIDI_getAdLibMode();
Public Declare Function Mix_ADLMIDI_getAdLibMode Lib "SDL2MixerVB.dll" () As Long
'void Mix_ADLMIDI_setAdLibMode(int al);
Public Declare Sub Mix_ADLMIDI_setAdLibMode Lib "SDL2MixerVB.dll" (ByVal sc As Long)

'int  Mix_ADLMIDI_getLogarithmicVolumes();
Public Declare Function Mix_ADLMIDI_getLogarithmicVolumes Lib "SDL2MixerVB.dll" () As Long
'void Mix_ADLMIDI_setLogarithmicVolumes(int lv);
Public Declare Sub Mix_ADLMIDI_setLogarithmicVolumes Lib "SDL2MixerVB.dll" (ByVal sc As Long)

'int  Mix_ADLMIDI_getVolumeModel();
Public Declare Function Mix_ADLMIDI_getVolumeModel Lib "SDL2MixerVB.dll" () As Long
'void Mix_ADLMIDI_setVolumeModel(int vm);
Public Declare Sub Mix_ADLMIDI_setVolumeModel Lib "SDL2MixerVB.dll" (ByVal sc As Long)

'extern DECLSPEC void SDLCALL MIX_ADLMIDI_setSetDefaults();
'Sets all ADLMIDI preferences to default state
Public Declare Sub MIX_ADLMIDI_setSetDefaults Lib "SDL2MixerVB.dll" ()

'/* Sets WOPL bank file for ADLMIDI playing device, affects on MIDI file reopen */
'extern DECLSPEC void SDLCALL Mix_ADLMIDI_setCustomBankFile(const char *bank_wonl_path);
Public Declare Sub Mix_ADLMIDI_setCustomBankFile Lib "SDL2MixerVB.dll" (ByVal bank_wonl_path As String)

'/* Sets WOPN bank file for OPNMIDI playing device, affects on MIDI file reopen */
'extern DECLSPEC void SDLCALL Mix_OPNMIDI_setCustomBankFile(const char *bank_wonp_path);
Public Declare Sub Mix_OPNMIDI_setCustomBankFile Lib "SDL2MixerVB.dll" (ByVal bank_wonp_path As String)

'int MIX_SetMidiDevice(int device);
' Allows you to toggle MIDI Device (change applying only on reopening of MIDI file)
Public Declare Function MIX_SetMidiDevice Lib "SDL2MixerVB.dll" (ByVal MIDIDevice As Mix_MIDI_Device) As Long

'void MIX_SetLockMIDIArgs(int lock_midiargs);
'Allows to lock/unlock ADLMIDI flags capturing from Mix_LoadMUS function
Public Declare Function MIX_SetLockMIDIArgs Lib "SDL2MixerVB.dll" (ByVal lockMidiargs As Long)

'/*****************SDL RWops AIP*****************/

'SDL_RWops * SDL_RWFromFileVB6(const char *file, const char *mode)
Public Declare Function SDL_RWFromFile Lib "SDL2MixerVB.dll" Alias "SDL_RWFromFileVB6" _
                    (ByVal FileName As String, ByVal Mode As String) As Long

'SDL_RWops * SDL_RWFromMemVB6(void *mem, int size)
Public Declare Function SDL_RWFromMem Lib "SDL2MixerVB.dll" Alias "SDL_RWFromMemVB6" _
                    (ByRef Mem As Any, ByVal Size As Long) As Long
                    
'SDL_RWops * SDL_AllocRWVB6(void)
Public Declare Function SDL_AllocRW Lib "SDL2MixerVB.dll" Alias "SDL_AllocRWVB6" () As Long

'void SDL_FreeRWVB6(SDL_RWops * area)
Public Declare Sub SDL_FreeRW Lib "SDL2MixerVB.dll" Alias "SDL_FreeRWVB6" _
                    (ByVal RWops As Long)

'int SDL_RWsizeVB6(SDL_RWops * ctx)
Public Declare Function SDL_RWsize Lib "SDL2MixerVB.dll" Alias "SDL_RWseekVB6" _
                    (ByVal RWops As Long) As Long
                    
'int SDL_RWseekVB6(SDL_RWops * ctx, int offset, int whence)
Public Declare Function SDL_RWseek Lib "SDL2MixerVB.dll" Alias "SDL_RWseekVB6" _
                    (ByVal RWops As Long, ByVal Offset As Long, ByVal Whence As Long) As Long
                    
'int SDL_RWtellVB6(SDL_RWops * ctx)
Public Declare Function SDL_RWtell Lib "SDL2MixerVB.dll" Alias "SDL_RWtellVB6" _
                    (ByVal RWops As Long) As Long

'int SDL_RWreadVB6(SDL_RWops * ctx, void*ptr, int size, int maxnum)
Public Declare Function SDL_RWread Lib "SDL2MixerVB.dll" Alias "SDL_RWreadVB6" _
                    (ByVal RWops As Long, ByRef Ptr As Any, ByVal Size As Long, ByVal max As Long) As Long

'int SDL_RWwriteVB6(SDL_RWops * ctx, void* ptr, int size, int maxnum)
Public Declare Function SDL_RWwrite Lib "SDL2MixerVB.dll" Alias "SDL_RWwriteVB6" _
                    (ByVal RWops As Long, ByRef Ptr As Any, ByVal Size As Long, ByVal max As Long) As Long

'int SDL_RWcloseVB6(SDL_RWops * ctx)
Public Declare Function SDL_RWclose Lib "SDL2MixerVB.dll" Alias "SDL_RWcloseVB6" _
                    (ByVal RWops As Long) As Long


Public Function Mix_PlayChannelVol(ByVal Channel As Long, ByVal Chunk As Long, ByVal loops As Long, ByVal Volume As Long)
    Mix_PlayChannelVol = Mix_PlayChannelTimedVolume(Channel, Chunk, loops, -1, Volume)
End Function

