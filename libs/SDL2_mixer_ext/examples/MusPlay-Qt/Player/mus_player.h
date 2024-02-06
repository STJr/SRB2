#ifndef MUS_PLAYER_H
#define MUS_PLAYER_H

#include "SDL.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

#if defined(SDL_MIXER_X) && \
    ((SDL_MIXER_MAJOR_VERSION > 2) || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 1))
#   define SDL_MIXER_GE21 // SD MixerX Version 2.1 and higher
#endif

#include <QString>
#include <QDebug>
#define DebugLog(msg) qDebug() << msg;

#if !defined(SDL_MIXER_X) // Fallback for into raw SDL2_mixer
#   define Mix_PlayingMusicStream(music) Mix_PlayingMusic()
#   define Mix_PausedMusicStream(music) Mix_PausedMusic()
#   define Mix_ResumeMusicStream(music) Mix_ResumeMusic()
#   define Mix_PauseMusicStream(music) Mix_PauseMusic()
#   define Mix_HaltMusicStream(music) Mix_HaltMusic()
#   define Mix_VolumeMusicStream(music, volume) Mix_VolumeMusic(volume)

#   define Mix_SetMusicStreamPosition(music, value) Mix_SetMusicPosition(value)
#   define Mix_GetMusicTotalTime(music) -1.0
#   define Mix_GetMusicPosition(music) -1.0
#   define Mix_GetMusicLoopStartTime(music) -1.0
#   define Mix_GetMusicLoopEndTime(music) -1.0
#   define Mix_GetMusicTempo(music) -1.0

inline int Mix_PlayChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ticks, int volume)
{
    int ret = Mix_PlayChannelTimed(which, chunk, loops, ticks);
    if(ret >= 0)
        Mix_Volume(which, volume);
    return ret;
}

inline int Mix_FadeInChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ms, int ticks, int volume)
{
    int ret = Mix_FadeInChannelTimed(which, chunk, loops, ms, ticks);
    if(ret >= 0)
        Mix_Volume(which, volume);
    return ret;
}
#endif

#ifndef SDL_MIXER_GE21
#   define Mix_GetMusicTitle(mus) "[No music]"
#   define Mix_GetMusicArtistTag(mus) "[Unknown Artist]"
#   define Mix_GetMusicAlbumTag(mus) "[Unknown Album]"
#   define Mix_GetMusicCopyrightTag(mus) "[Empty tag]"
#endif

struct SpcEcho;
struct ReverbSetup;

namespace PGE_MusicPlayer
{
    extern Mix_Music *s_playMus;
    extern Mix_MusicType type;
    extern bool reverbEnabled;
    extern bool echoEnabled;

    extern SpcEcho *effectEcho;

    extern void loadAudioSettings();
    extern void saveAudioSettings();

    extern int      getSampleRate();
    extern Uint16   getSampleFormat();
    extern int      getChannels();
    extern QString  getOutputType();

    extern void setSpec(int rate, Uint16 format, int channels, const QString &outputType);

    extern bool openAudio(QString &error);
    extern bool openAudioWithSpec(QString &error, int rate, Uint16 format, int channels, const QString &output);
    extern void closeAudio();

    extern bool reloadAudio(QString &error);

    extern void initHooks();

    extern void setMainWindow(void *mwp);

    extern const char *musicTypeC();

    extern QString musicType();

    extern Mix_MusicType musicTypeI();

    /*!
     * \brief Stop music playing
     */
    extern void stopMusic();

    extern void disableHooks();

    extern void enableHooks();

    /*!
     * \brief Get music title of current track
     * \return music title of current music file
     */
    extern QString getMusTitle();

    /*!
     * \brief Get music artist tag text of current music track
     * \return music artist tag text of current music track
     */
    extern QString getMusArtist();

    /*!
     * \brief Get music album tag text of current music track
     * \return music ablum tag text of current music track
     */
    extern QString getMusAlbum();

    /*!
     * \brief Get music copyright tag text of current music track
     * \return music copyright tag text of current music track
     */
    extern QString getMusCopy();

    extern void setMusicLoops(int loops);
    /*!
     * \brief Start playing of currently opened music track
     */
    extern bool playMusic();
    /*!
     * \brief Sets volume level of current music stream
     * \param volume level of volume from 0 tp 128
     */
    extern void changeVolume(int volume);
    /*!
     * \brief Open music file
     * \param musFile Full path to music file
     * \return true if music file was successfully opened, false if loading was failed
     */
    extern bool openFile(QString musFile);

    extern void startWavRecording(QString target);

    extern void stopWavRecording();

    extern bool isWavRecordingWorks();


    extern void echoEabled(bool enabled);
    extern void echoEffectDone(int chan, void *context);
    extern void echoResetFir();
    extern void echoResetDefaults();
    extern void echoSetReg(int key, int val);
    extern int  echoGetReg(int key);

    extern void reverbEabled(bool enabled);
    extern void reverbEffectDone(int, void *context);

    extern void reverbUpdateSetup(ReverbSetup *setup);
    extern void reverbGetSetup(ReverbSetup *setup);

    extern void reverbSetMode(int mode);
    extern void reverbSetRoomSize(float val);
    extern void reverbSetDamping(float val);
    extern void reverbSetWet(float val);
    extern void reverbSetDry(float val);
    extern void reverbSetWidth(float val);
}

#endif // MUS_PLAYER_H
