#include "mus_player.h"

#include <QMessageBox>
#include <QSettings>
#include "../MainWindow/musplayer_qt.h"
#include "../Effects/spc_echo.h"
#include "../Effects/reverb.h"

#include "../wave_writer.h"

/*!
 *  SDL Mixer wrapper
 */
namespace PGE_MusicPlayer
{
    static MusPlayer_Qt *mw = nullptr;

    Mix_Music *s_playMus = nullptr;
    Mix_MusicType type  = MUS_NONE;
    bool reverbEnabled = false;
    bool echoEnabled = false;
    SpcEcho *effectEcho = nullptr;
    FxReverb *effectReverb = nullptr;

    static bool g_playlistMode = false;
    static int  g_loopsCount = -1;
    static bool g_hooksDisabled = false;

    static int      g_sample_rate = MIX_DEFAULT_FREQUENCY;
    static QString  g_output_type;
    static Uint16   g_sample_format = MIX_DEFAULT_FORMAT;
    static int      g_channels = MIX_DEFAULT_CHANNELS;

    static void musicStoppedHook();

    static const char *format_to_string(Uint16 format)
    {
        switch(format)
        {
        case AUDIO_U8:
            return "U8";
        case AUDIO_S8:
            return "S8";
        case AUDIO_U16LSB:
            return "U16LSB";
        case AUDIO_S16LSB:
            return "S16LSB";
        case AUDIO_U16MSB:
            return "U16MSB";
        case AUDIO_S16MSB:
            return "S16MSB";
        case AUDIO_S32MSB:
            return "S32MSB";
        case AUDIO_S32LSB:
            return "S32LSB";
        case AUDIO_F32MSB:
            return "F32MSB";
        case AUDIO_F32LSB:
            return "F32LSB";
        default:
            return "Unknown";
        }
    }

    void loadAudioSettings()
    {
        QSettings setup;
        g_sample_rate = setup.value("Audio-SampleRate", 44100).toInt();
        g_sample_format = static_cast<Uint16>(setup.value("Audio-SampleFormat", AUDIO_F32).toUInt());
        g_channels = setup.value("Audio-Channels", 2).toInt();
        g_output_type = setup.value("Audio-Output", QString()).toString();
    }

    void saveAudioSettings()
    {
        QSettings setup;
        setup.setValue("Audio-SampleRate", g_sample_rate);
        setup.setValue("Audio-SampleFormat", g_sample_format);
        setup.setValue("Audio-Channels", g_channels);
        setup.setValue("Audio-Output", g_output_type);
        setup.sync();
    }

    int getSampleRate()
    {
        return g_sample_rate;
    }

    Uint16 getSampleFormat()
    {
        return g_sample_format;
    }

    int getChannels()
    {
        return g_channels;
    }

    QString getOutputType()
    {
        return g_output_type;
    }

    void setSpec(int rate, Uint16 format, int channels, const QString &outputType)
    {
        g_sample_rate = rate;
        g_sample_format = format;
        g_channels = channels;
        g_output_type = outputType;
    }

    bool openAudio(QString &error)
    {
        return openAudioWithSpec(error, g_sample_rate, g_sample_format, g_channels, g_output_type);
    }

    bool openAudioWithSpec(QString &error, int rate, Uint16 format, int channels, const QString &output)
    {
#ifdef _WIN32 // FIXME: make sound output type menu on other platforms. Until, just rely on environment
        if(!output.isEmpty())
            SDL_setenv("SDL_AUDIODRIVER", output.toUtf8(), 1);
        else
            SDL_setenv("SDL_AUDIODRIVER", "", 1);
#else
        Q_UNUSED(output)
#endif

        if(SDL_InitSubSystem(SDL_INIT_AUDIO) == -1)
        {
            error = QString("Failed to initialize audio: ") + SDL_GetError();
            return false;
        }

        if(Mix_Init(MIX_INIT_FLAC | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MOD | MIX_INIT_MID ) == -1)
        {
            error = QString("Failed to initialize mixer: ") + Mix_GetError();
            return false;
        }

        if(Mix_OpenAudio(rate, format, channels, 512) == -1)
        {
            error = QString("Failed to open audio stream: %1\n\n"
                            "Given spec:\n"
                            "- Sample rate %2\n"
                            "- Sample format %3\n"
                            "- Channels count %4\n")
                    .arg(Mix_GetError())
                    .arg(rate)
                    .arg(format_to_string(format))
                    .arg(channels);
            return false;
        }

        // Retrieve back the actual characteristics
        Mix_QuerySpec(&g_sample_rate, &g_sample_format, &g_channels);

        Mix_AllocateChannels(16);

        return true;
    }

    void closeAudio()
    {
        if(mw)
            QMetaObject::invokeMethod(mw, "musicStopped", Qt::QueuedConnection);
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    bool reloadAudio(QString &error)
    {
        closeAudio();
        return openAudio(error);
    }

    void initHooks()
    {
        Mix_HookMusicFinished(&musicStoppedHook);
    }

    void setMainWindow(void *mwp)
    {
        mw = reinterpret_cast<MusPlayer_Qt*>(mwp);
    }

    const char *musicTypeC()
    {
        return (
                    type == MUS_NONE ? "No Music" :
                    type == MUS_CMD ? "CMD" :
                    type == MUS_WAV ? "PCM Wave" :
                    type == MUS_MOD ? "Tracker" :
                    type == MUS_MID ? "MIDI" :
                    type == MUS_OGG ? "OGG" :
                    type == MUS_MP3 ? "MP3" :
                    type == MUS_FLAC ? "FLAC" :
                    type == MUS_WAVPACK ? "WAVPACK" :
                    type == MUS_GME ? "GME Chiptune" :
#ifdef SDL_MIXER_X
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 2)
                    type == MUS_OPUS ? "OPUS" :
#   endif
                    type == MUS_ADLMIDI ? "IMF/MUS/XMI" :
                    type == MUS_OPNMIDI ? "MUS/XMI(OPN)" :
                    type == MUS_EDMIDI ? "MUS/XMI(ED)" :
                    type == MUS_FLUIDLITE ? "MUS/XMI(Fluid)" :
                    type == MUS_NATIVEMIDI ? "MUS/XMI(Native)" :
                    type == MUS_FFMPEG ? "FFMPEG" :
                    type == MUS_PXTONE ? "PXTONE" :
#else
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION > 0) || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION == 0 && SDL_MIXER_PATCHLEVEL >= 4)
                    type == MUS_OPUS ? "OPUS" :
#   endif
#endif
                   "<Unknown>");
    }
    QString musicType()
    {
        return QString(musicTypeC());
    }

    Mix_MusicType musicTypeI()
    {
        return type;
    }


    /*!
     * \brief Spawn warning message box with specific text
     * \param msg text to spawn in message box
     */
    static void error(QString msg)
    {
        QMessageBox::warning(nullptr,
                             "SDL2 Mixer ext error",
                             msg,
                             QMessageBox::Ok);
    }

    void stopMusic()
    {
        Mix_HaltMusicStream(PGE_MusicPlayer::s_playMus);
    }

    void disableHooks()
    {
        g_hooksDisabled = true;
    }

    void enableHooks()
    {
        g_hooksDisabled = false;
    }

    QString getMusTitle()
    {
        if(s_playMus)
            return QString(Mix_GetMusicTitle(s_playMus));
        else
            return QString("[No music]");
    }

    QString getMusArtist()
    {
        if(s_playMus)
            return QString(Mix_GetMusicArtistTag(s_playMus));
        else
            return QString("[Unknown Artist]");
    }

    QString getMusAlbum()
    {
        if(s_playMus)
            return QString(Mix_GetMusicAlbumTag(s_playMus));
        else
            return QString("[Unknown Album]");
    }

    QString getMusCopy()
    {
        if(s_playMus)
            return QString(Mix_GetMusicCopyrightTag(s_playMus));
        else
            return QString("");
    }

    static void musicStoppedHook()
    {
        if(g_hooksDisabled)
            return;
        if(mw)
            QMetaObject::invokeMethod(mw, "musicStopped", Qt::QueuedConnection);
    }

    void setMusicLoops(int loops)
    {
        g_loopsCount = loops;
    }

    bool playMusic()
    {
        if(s_playMus)
        {
            if(Mix_PlayMusic(s_playMus, g_playlistMode ? 0 : g_loopsCount) == -1)
            {
                error(QString("Mix_PlayMusic: ") + Mix_GetError());
                return false;
                // well, there's no music, but most games don't break without music...
            }
        }
        else
        {
            error(QString("Play nothing: Mix_PlayMusic: ") + Mix_GetError());
            return false;
        }
        return true;
    }

    void changeVolume(int volume)
    {
        Mix_VolumeMusicStream(PGE_MusicPlayer::s_playMus, volume);
        DebugLog(QString("Mix_VolumeMusic: %1\n").arg(volume));
    }

    bool openFile(QString musFile)
    {
        type = MUS_NONE;
        if(s_playMus != nullptr)
        {
            Mix_FreeMusic(s_playMus);
            s_playMus = nullptr;
        }

        QByteArray p = musFile.toUtf8();
        s_playMus = Mix_LoadMUS(p.data());
        if(!s_playMus)
        {
            error(QString("Mix_LoadMUS(\"" + QString(musFile) + "\"): ") + Mix_GetError());
            return false;
        }
        type = Mix_GetMusicType(s_playMus);
        DebugLog(QString("Music type: %1").arg(musicType()));
        return true;
    }




    static void* s_wavCtx = nullptr;

    // make a music play function
    // it expects udata to be a pointer to an int
    static void myMusicPlayer(void *udata, Uint8 *stream, int len)
    {
        ctx_wave_write(udata, stream, len);
    }

    void startWavRecording(QString target)
    {
        if(s_wavCtx)
            return;
        if(!s_playMus)
            return;

        int format = WAVE_FORMAT_PCM;
        int sampleSize = 2;
        int hasSign = 1;
        int isBigEndian = 0;

        switch(g_sample_format)
        {
        case AUDIO_U8:
            sampleSize = 1;
            format = WAVE_FORMAT_PCM;
            hasSign = 0;
            isBigEndian = 0;
            break;
        case AUDIO_S8:
            sampleSize = 1;
            format = WAVE_FORMAT_PCM;
            hasSign = 1;
            isBigEndian = 0;
            break;
        case AUDIO_U16LSB:
            sampleSize = 2;
            format = WAVE_FORMAT_PCM;
            hasSign = 0;
            isBigEndian = 0;
            break;
        default:
        case AUDIO_S16LSB:
            sampleSize = 2;
            format = WAVE_FORMAT_PCM;
            hasSign = 1;
            isBigEndian = 0;
            break;
        case AUDIO_U16MSB:
            sampleSize = 2;
            format = WAVE_FORMAT_PCM;
            hasSign = 0;
            isBigEndian = 1;
            break;
        case AUDIO_S16MSB:
            sampleSize = 2;
            format = WAVE_FORMAT_PCM;
            hasSign = 1;
            isBigEndian = 1;
            break;
        case AUDIO_S32MSB:
            sampleSize = 4;
            format = WAVE_FORMAT_PCM;
            hasSign = 1;
            isBigEndian = 1;
            break;
        case AUDIO_S32LSB:
            sampleSize = 4;
            format = WAVE_FORMAT_PCM;
            hasSign = 1;
            isBigEndian = 0;
            break;
        case AUDIO_F32MSB:
            sampleSize = 4;
            format = WAVE_FORMAT_IEEE_FLOAT;
            hasSign = 1;
            isBigEndian = 1;
            break;
        case AUDIO_F32LSB:
            sampleSize = 4;
            format = WAVE_FORMAT_IEEE_FLOAT;
            hasSign = 1;
            isBigEndian = 0;
            break;
        }

        s_wavCtx = ctx_wave_open(g_channels,
                                 g_sample_rate,
                                 sampleSize,
                                 format,
                                 hasSign,
                                 isBigEndian,
                                 target.toLocal8Bit().data());
        Mix_SetPostMix(myMusicPlayer, s_wavCtx);
    }

    void stopWavRecording()
    {
        if(!s_wavCtx)
            return;
        Mix_SetPostMix(nullptr, nullptr);
        ctx_wave_close(s_wavCtx);
        s_wavCtx = nullptr;
    }

    bool isWavRecordingWorks()
    {
        return (s_wavCtx != nullptr);
    }


    void echoEabled(bool enabled)
    {
        if(enabled && !effectEcho)
        {
            effectEcho = echoEffectInit(g_sample_rate, g_sample_format, g_channels);
            Mix_RegisterEffect(MIX_CHANNEL_POST, spcEchoEffect, echoEffectDone, effectEcho);
        }
        else if(!enabled && effectEcho)
        {
            Mix_UnregisterEffect(MIX_CHANNEL_POST, spcEchoEffect);
            echoEffectFree(effectEcho);
            effectEcho = nullptr;
        }
    }

    void echoEffectDone(int, void *context)
    {
        SpcEcho *out = reinterpret_cast<SpcEcho *>(context);
        if(out == effectEcho)
        {
            echoEffectFree(effectEcho);
            effectEcho = nullptr;
        }
    }

    void echoResetFir()
    {
        if(!effectEcho)
            return;

        SDL_LockAudio();
        echoEffectResetFir(effectEcho);
        SDL_UnlockAudio();
    }

    void echoResetDefaults()
    {
        if(!effectEcho)
            return;

        SDL_LockAudio();
        echoEffectResetDefaults(effectEcho);
        SDL_UnlockAudio();
    }

    void echoSetReg(int key, int val)
    {
        if(!effectEcho)
            return;

        SDL_LockAudio();
        echoEffectSetReg(effectEcho, (EchoSetup)key, val);
        SDL_UnlockAudio();
    }

    int echoGetReg(int key)
    {
        if(!effectEcho)
            return 0;
        return echoEffectGetReg(effectEcho, (EchoSetup)key);
    }


    void reverbEabled(bool enabled)
    {
        if(enabled && !effectReverb)
        {
            effectReverb = reverbEffectInit(g_sample_rate, g_sample_format, g_channels);
            Mix_RegisterEffect(MIX_CHANNEL_POST, reverbEffect, reverbEffectDone, effectReverb);
        }
        else if(!enabled && effectReverb)
        {
            Mix_UnregisterEffect(MIX_CHANNEL_POST, reverbEffect);
            reverbEffectFree(effectReverb);
            effectReverb = nullptr;
        }
    }

    void reverbEffectDone(int, void *context)
    {
        FxReverb *out = reinterpret_cast<FxReverb *>(context);
        if(out == effectReverb)
        {
            reverbEffectFree(effectReverb);
            effectReverb = nullptr;
        }
    }

    void reverbUpdateSetup(ReverbSetup *setup)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateSetup(effectReverb, *setup);
        SDL_UnlockAudio();
    }

    void reverbGetSetup(ReverbSetup *setup)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbGetSetup(effectReverb, *setup);
        SDL_UnlockAudio();
    }

    void reverbSetMode(int mode)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateMode(effectReverb, (float)mode);
        SDL_UnlockAudio();
    }

    void reverbSetRoomSize(float val)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateRoomSize(effectReverb, val);
        SDL_UnlockAudio();
    }

    void reverbSetDamping(float val)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateDamping(effectReverb, val);
        SDL_UnlockAudio();
    }

    void reverbSetWet(float val)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateWetLevel(effectReverb, val);
        SDL_UnlockAudio();
    }

    void reverbSetDry(float val)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateDryLevel(effectReverb, val);
        SDL_UnlockAudio();
    }

    void reverbSetWidth(float val)
    {
        if(!effectReverb)
            return;

        SDL_LockAudio();
        ::reverbUpdateWidth(effectReverb, val);
        SDL_UnlockAudio();
    }
}
