#include <QtDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSlider>
#include <QDateTime>
#include <QTime>
#include <QSettings>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QMoveEvent>
#include <QToolTip>
#include <cmath>

#include "ui_player_main.h"
#include "musplayer_qt.h"
#include "multi_music_test.h"
#include "multi_sfx_test.h"
#include "musicfx.h"
#include "track_muter.h"
#include "../Player/mus_player.h"
#include "../AssocFiles/assoc_files.h"
#include <math.h>
#include "../version.h"

#include "sfx_tester.h"
#include "setup_midi.h"
#include "setup_audio.h"
#include "echo_tune.h"
#include "reverb_tune.h"
#include "seek_bar.h"

#include "qfile_dialogs_default_options.hpp"

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
#   define qAsConst(x)  x
#endif

static QString timePosToString(double pos, const QString &format = "hh:mm:ss")
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    return QDateTime::fromSecsSinceEpoch((uint)std::floor(pos), Qt::UTC).toString(format);
#else
    return QDateTime::fromTime_t((uint)std::floor(pos)).toUTC().toString(format);
#endif
}


MusPlayer_Qt::MusPlayer_Qt(QWidget *parent) : QMainWindow(parent),
    MusPlayerBase(),
    ui(new Ui::MusPlayerQt)
{
    ui->setupUi(this);
    PGE_MusicPlayer::setMainWindow(this);
    PGE_MusicPlayer::initHooks();
#ifdef Q_OS_MAC
    this->setWindowIcon(QIcon(":/cat_musplay.icns"));
#endif
#ifdef Q_OS_WIN
    this->setWindowIcon(QIcon(":/cat_musplay.ico"));
#endif

    QString title = windowTitle();
    /* Append library version to the title */
    const SDL_version *mixer_ver = Mix_Linked_Version();
#if defined(SDL_MIXER_X)
    title += QString(" (SDL Mixer X %1.%2.%3)")
             .arg(mixer_ver->major)
             .arg(mixer_ver->minor)
             .arg(mixer_ver->patch);
#else
    title += QString(" (SDL2_mixer %1.%2.%3)")
             .arg(mixer_ver->major)
             .arg(mixer_ver->minor)
             .arg(mixer_ver->patch);
#endif
    setWindowTitle(title);

#ifndef SDL_MIXER_X
    ui->actionMidiSetup->setEnabled(false);
#endif

    m_sfxTester = new SfxTester(this);
    m_sfxTester->setModal(false);

    m_multiMusicTest = new MultiMusicTest(this);
    m_multiMusicTest->setModal(false);

    m_multiSfxTest = new MultiSfxTester(this);
    m_multiSfxTest->setModal(false);

    m_musicFx = new MusicFX(this);
    m_musicFx->setModal(false);

    m_trackMuter = new TrackMuter(this);
    m_trackMuter->setModal(false);

    m_setupMidi = new SetupMidi(this);
    m_setupMidi->setModal(false);

    m_echoTune = new EchoTune(this);
    m_echoTune->setModal(false);

    m_reverbTune = new ReverbTune(this);
    m_reverbTune->setModal(false);

    connect(m_setupMidi, &SetupMidi::songRestartNeeded, this, &MusPlayer_Qt::restartMusic);

    m_seekBar = new SeekBar(this);
    ui->gridLayout->removeWidget(ui->musicPosition);
    ui->gridLayout->addWidget(m_seekBar, 5, 0, 1, 2);
    m_seekBar->setLength(100);
    m_seekBar->setVisible(true);
    ui->musicPosition->setVisible(false);

    ui->centralWidget->window()->setWindowFlags(
        Qt::WindowTitleHint |
        Qt::WindowSystemMenuHint |
        Qt::WindowCloseButtonHint |
        Qt::WindowMinimizeButtonHint |
        Qt::MSWindowsFixedSizeDialogHint);
    QObject::connect(&m_blinker, SIGNAL(timeout()), this, SLOT(_blink_red()));
    QObject::connect(&m_positionWatcher, SIGNAL(timeout()), this, SLOT(updatePositionSlider()));
    QObject::connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
    QObject::connect(ui->volume, &QSlider::valueChanged, [this](int x)
    {
        on_volume_valueChanged(x);
        QToolTip::showText(QCursor::pos(), QString("%1").arg(x), this);
    });

    QObject::connect(m_seekBar, &SeekBar::positionSeeked, this, &MusPlayer_Qt::musicPosition_seeked);

    ui->actionTuneReverb->setEnabled(PGE_MusicPlayer::reverbEnabled);
    ui->actionTuneEcho->setEnabled(PGE_MusicPlayer::echoEnabled);

    QSettings setup;
    m_seekBar->setEnabled(false);
    ui->isLooping->setVisible(false);

    ui->volume->setValue(setup.value("Volume", 128).toInt());
    m_prevTrackID = ui->trackID->value();
    ui->gme_setup->setEnabled(false);
    ui->tempoFrame->setEnabled(false);
    ui->speedFrame->setEnabled(false);
    ui->pitchFrame->setEnabled(false);

    m_currentMusic = setup.value("RecentMusic", "").toString();
    restoreGeometry(setup.value("Window-Geometry").toByteArray());
    layout()->activate();
    adjustSize();

    m_setupMidi->loadSetup();
}

MusPlayer_Qt::~MusPlayer_Qt()
{
    on_stop_clicked();

    m_sfxTester->close();
    m_multiMusicTest->close();

    m_setupMidi->close();
    m_setupMidi->saveSetup();

    Mix_CloseAudio();

    QSettings setup;
    setup.setValue("Window-Geometry", saveGeometry());
    setup.setValue("Volume", ui->volume->value());
    setup.setValue("RecentMusic", m_currentMusic);
    setup.sync();

    delete ui;
}

void MusPlayer_Qt::dropEvent(QDropEvent *e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    if(ui->recordWav->isChecked())
        return;

    auto l = e->mimeData()->urls();

    for(const QUrl &url : qAsConst(l))
    {
        const QString &fileName = url.toLocalFile();
        m_currentMusic = fileName;
    }

    ui->recordWav->setEnabled(!m_currentMusic.endsWith(".wav", Qt::CaseInsensitive));//Avoid self-trunkling!
    PGE_MusicPlayer::stopMusic();
    qApp->processEvents();
    on_play_clicked();
    this->raise();
    e->accept();
}

void MusPlayer_Qt::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MusPlayer_Qt::moveEvent(QMoveEvent *event)
{
    if(m_oldWindowPos.isNull())
        m_oldWindowPos = event->oldPos();

    int deltaX = event->pos().x() - m_oldWindowPos.x();
    int deltaY = event->pos().y() - m_oldWindowPos.y();
    {
        QRect g = m_sfxTester->frameGeometry();
        m_sfxTester->move(g.x() + deltaX, g.y() + deltaY);
    }
    {
        QRect g = m_setupMidi->frameGeometry();
        m_setupMidi->move(g.x() + deltaX, g.y() + deltaY);
    }
    {
        QRect g = m_reverbTune->frameGeometry();
        m_reverbTune->move(g.x() + deltaX, g.y() + deltaY);
    }
    {
        QRect g = m_echoTune->frameGeometry();
        m_echoTune->move(g.x() + deltaX, g.y() + deltaY);
    }
    {
        QRect g = m_multiMusicTest->frameGeometry();
        m_multiMusicTest->move(g.x() + deltaX, g.y() + deltaY);
    }
    {
        QRect g = m_musicFx->frameGeometry();
        m_musicFx->move(g.x() + deltaX, g.y() + deltaY);
    }
    {
        QRect g = m_trackMuter->frameGeometry();
        m_trackMuter->move(g.x() + deltaX, g.y() + deltaY);
    }

    m_oldWindowPos = event->pos();
}

void MusPlayer_Qt::contextMenu(const QPoint &pos)
{
    QMenu x;
    QAction *open        = x.addAction("Open");
    QAction *playpause   = x.addAction("Play/Pause");
    QAction *stop        = x.addAction("Stop");
    x.addSeparator();
    QAction *reverb       = x.addAction("Reverb");
    QAction *reverbTuner  = x.addAction("Reverb tuner...");
    QAction *echo         = x.addAction("Echo");
    QAction *echoTuner    = x.addAction("Echo tuner...");
    QAction *musicFX      = x.addAction("Music FX...");
    QAction *resetTempo   = x.addAction("Reset tempo");
    QAction *resetSpeed   = x.addAction("Reset speed");
    QAction *resetPitch   = x.addAction("Reset pitch");
    resetTempo->setEnabled(ui->tempoFrame->isEnabled());
    resetSpeed->setEnabled(ui->speedFrame->isEnabled());
    resetPitch->setEnabled(ui->pitchFrame->isEnabled());
    reverb->setCheckable(true);
    reverb->setChecked(PGE_MusicPlayer::reverbEnabled);
    reverbTuner->setEnabled(PGE_MusicPlayer::reverbEnabled);
    echo->setCheckable(true);
    echo->setChecked(PGE_MusicPlayer::echoEnabled);
    echoTuner->setEnabled(PGE_MusicPlayer::echoEnabled);
    QAction *assoc_files = x.addAction("Associate files");
    QAction *sfx_testing_show = x.addAction("Show SFX testing");
    sfx_testing_show->setEnabled(!m_sfxTester->isVisible());

#ifdef SDL_MIXER_X
    QAction *midi_setup_show = x.addAction("Show MIDI setup");
    midi_setup_show->setEnabled(!m_setupMidi->isVisible());
#endif

    x.addSeparator();
    QMenu   *about       = x.addMenu("About");
    QAction *version     = about->addAction("SDL Mixer X Music Player v." V_FILE_VERSION);
    version->setEnabled(false);
    QAction *license     = about->addAction("Licensed under GNU GPLv3 license");
    about->addSeparator();
    QAction *source      = about->addAction("Get source code");
    QAction *ret = x.exec(this->mapToGlobal(pos));

    if(open == ret)
        on_open_clicked();
    else if(playpause == ret)
        on_play_clicked();
    else if(stop == ret)
        on_stop_clicked();
    else if(reverb == ret)
    {
        ui->actionEnableReverb->setChecked(reverb->isChecked());
        on_actionEnableReverb_triggered(reverb->isChecked());
    }
    else if(echo == ret)
    {
        ui->actionEnableEcho->setChecked(echo->isChecked());
        on_actionEnableEcho_triggered(echo->isChecked());
    }
    else if(reverbTuner == ret)
        on_actionTuneReverb_triggered();
    else if(echoTuner == ret)
        on_actionTuneEcho_triggered();
    else if(musicFX == ret)
        on_actionMusicFX_triggered();
    else if(resetTempo == ret)
        ui->tempo->setValue(0);
    else if(resetSpeed == ret)
        ui->speed->setValue(0);
    else if(resetPitch == ret)
        ui->pitch->setValue(0);
    else if(assoc_files == ret)
        on_actionFileAssoc_triggered();
    else if(ret == sfx_testing_show)
        on_actionSfxTesting_triggered();
#ifdef SDL_MIXER_X
    else if(ret == midi_setup_show)
        on_actionMidiSetup_triggered();
#endif
    else if(ret == license)
        on_actionHelpLicense_triggered();
    else if(ret == source)
        on_actionHelpGitHub_triggered();
}

void MusPlayer_Qt::openMusicByArg(QString musPath)
{
    if(ui->recordWav->isChecked())
        return;

    m_currentMusic = musPath;
    PGE_MusicPlayer::stopMusic();
    qApp->processEvents();
    on_play_clicked();
}

void MusPlayer_Qt::restartMusic()
{
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        PGE_MusicPlayer::stopMusic();
        qApp->processEvents();
        on_play_clicked();
    }
}

void MusPlayer_Qt::musicStopped()
{
    m_positionWatcher.stop();
    ui->playingTimeLabel->setText("--:--:--");
    m_seekBar->setPosition(0.0);
    m_seekBar->setEnabled(false);
    ui->play->setToolTip(tr("Play"));
    ui->play->setIcon(QIcon(":/buttons/play.png"));

    if(PGE_MusicPlayer::isWavRecordingWorks())
    {
        ui->recordWav->setChecked(false);
        PGE_MusicPlayer::stopWavRecording();
        ui->open->setEnabled(true);
        ui->play->setEnabled(true);
        m_blinker.stop();
        ui->recordWav->setStyleSheet("");
    }
}

void MusPlayer_Qt::on_open_clicked()
{
    QString file = QFileDialog::getOpenFileName(this,
                                                tr("Open music file"),
                                                (m_currentMusic.isEmpty() ?
                                                QApplication::applicationDirPath() :
                                                m_currentMusic),
                                                "All (*.*)", nullptr,
                                                c_fileDialogOptions);

    if(file.isEmpty())
        return;

    m_currentMusic = file;
    PGE_MusicPlayer::stopMusic();
    qApp->processEvents();
    on_play_clicked();
}

void MusPlayer_Qt::on_stop_clicked()
{
    musicStopped();
    PGE_MusicPlayer::stopMusic();
}

void MusPlayer_Qt::on_play_clicked()
{
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        if(Mix_PausedMusicStream(PGE_MusicPlayer::s_playMus))
        {
            Mix_ResumeMusicStream(PGE_MusicPlayer::s_playMus);
            ui->play->setToolTip(tr("Pause"));
            ui->play->setIcon(QIcon(":/buttons/pause.png"));
            return;
        }
        else
        {
            Mix_PauseMusicStream(PGE_MusicPlayer::s_playMus);
            ui->play->setToolTip(tr("Resume"));
            ui->play->setIcon(QIcon(":/buttons/play.png"));
            return;
        }
    }

    ui->play->setToolTip(tr("Play"));
    ui->play->setIcon(QIcon(":/buttons/play.png"));
    ui->isLooping->setVisible(false);
    m_prevTrackID = ui->trackID->value();

    bool playSuccess = false;

    QString musicPath = m_currentMusic;
#ifdef SDL_MIXER_GE21
    QString midiRawArgs = m_setupMidi->getRawMidiArgs();
    if(ui->gme_setup->isEnabled())
        musicPath += "|" + ui->trackID->text() + ";" + midiRawArgs;
    else if(PGE_MusicPlayer::type == MUS_PXTONE || PGE_MusicPlayer::type == MUS_OGG)
        musicPath += "|" + midiRawArgs;
    else if((PGE_MusicPlayer::type == MUS_MID || PGE_MusicPlayer::type == MUS_ADLMIDI))
    {
        if(midiRawArgs.isEmpty())
            Mix_SetLockMIDIArgs(1);
        else
        {
            Mix_SetLockMIDIArgs(0);
            musicPath += "|" + midiRawArgs;
        }
    }
#else
    Q_UNUSED(m_currentMusic);
#endif

    if(PGE_MusicPlayer::openFile(musicPath))
    {
        ui->tempo->blockSignals(true);
        ui->tempo->setValue(0);
        ui->tempo->blockSignals(false);
        ui->tempoFrame->setEnabled(Mix_GetMusicTempo(PGE_MusicPlayer::s_playMus) >= 0.0);

        ui->speed->blockSignals(true);
        ui->speed->setValue(0);
        ui->speed->blockSignals(false);
        ui->speedFrame->setEnabled(Mix_GetMusicSpeed(PGE_MusicPlayer::s_playMus) >= 0.0);

        ui->pitch->blockSignals(true);
        ui->pitch->setValue(0);
        ui->pitch->blockSignals(false);
        ui->pitchFrame->setEnabled(Mix_GetMusicPitch(PGE_MusicPlayer::s_playMus) >= 0.0);

        PGE_MusicPlayer::changeVolume(ui->volume->value());
        playSuccess = PGE_MusicPlayer::playMusic();
        ui->play->setToolTip(tr("Pause"));
        ui->play->setIcon(QIcon(":/buttons/pause.png"));
        m_musicFx->resetAll();
        m_trackMuter->buildTracksList();
    }

    m_positionWatcher.stop();
    m_seekBar->setEnabled(false);
    ui->playingTimeLabel->setText("--:--:--");
    ui->playingTimeLenghtLabel->setText("/ --:--:--");

    if(playSuccess)
    {
        refreshMetaData();

        ui->gme_setup->setEnabled(false);
        ui->trackID->setEnabled(false);
        ui->trackNext->setEnabled(false);
        ui->trackPrev->setEnabled(false);
        ui->disableSpcEcho->setEnabled(false);
        ui->trackID->setValue(0);
        m_prevTrackID = 0;

        ui->smallInfo->setText(PGE_MusicPlayer::musicType());
        ui->gridLayout->update();

#ifdef SDL_MIXER_X
        int songsNum = Mix_GetNumTracks(PGE_MusicPlayer::s_playMus);
        if(songsNum > 1)
        {
            ui->gme_setup->setEnabled(true);
            ui->trackID->setEnabled(true);
            ui->trackID->setMaximum(songsNum - 1);
            ui->trackNext->setEnabled(true);
            ui->trackPrev->setEnabled(true);
        }

        if(PGE_MusicPlayer::type == MUS_GME)
        {
            ui->gme_setup->setEnabled(true);
            ui->disableSpcEcho->setEnabled(true);
            int echoDisabled = Mix_GME_GetSpcEchoDisabled(PGE_MusicPlayer::s_playMus);
            ui->disableSpcEcho->setEnabled(echoDisabled >= 0);

            switch(echoDisabled)
            {
            default:
            case 0:
                ui->disableSpcEcho->setChecked(false);
                break;
            case 1:
                ui->disableSpcEcho->setChecked(true);
                break;
            }
        }
#endif
    }
    else
    {
        ui->musTitle->setText("[Unknown]");
        ui->musArtist->setText("[Unknown]");
        ui->musAlbum->setText("[Unknown]");
        ui->musCopyright->setText("[Unknown]");
    }
}

void MusPlayer_Qt::on_trackID_editingFinished()
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        if(m_prevTrackID != ui->trackID->value())
        {
            Mix_StartTrack(PGE_MusicPlayer::s_playMus, ui->trackID->value());
            m_prevTrackID = ui->trackID->value();
            refreshMetaData();
        }
    }
#endif
}

void MusPlayer_Qt::refreshMetaData()
{
    double total = Mix_MusicDuration(PGE_MusicPlayer::s_playMus);
    double curPos = Mix_GetMusicPosition(PGE_MusicPlayer::s_playMus);

    double loopStart = Mix_GetMusicLoopStartTime(PGE_MusicPlayer::s_playMus);
    double loopEnd = Mix_GetMusicLoopEndTime(PGE_MusicPlayer::s_playMus);

    qDebug() << "Duration:" << total
             << "Position:" << curPos
             << "loopStart:" << loopStart
             << "loopEnd:" << loopEnd;

    m_seekBar->clearLoopPoints();
    m_seekBar->setEnabled(false);

    if(total > 0.0 && curPos >= 0.0)
    {
        m_seekBar->setEnabled(true);
        m_seekBar->setLength(total);
        m_seekBar->setPosition(0.0);
        m_seekBar->setLoopPoints(loopStart, loopEnd);
        ui->playingTimeLenghtLabel->setText(timePosToString(total, "/ hh:mm:ss"));
        m_positionWatcher.start(128);
    }
    // ui->musicPosition->setVisible(ui->musicPosition->isEnabled());

    if(loopStart >= 0.0)
        ui->isLooping->setVisible(true);

    ui->musTitle->setText(PGE_MusicPlayer::getMusTitle());
    ui->musArtist->setText(PGE_MusicPlayer::getMusArtist());
    ui->musAlbum->setText(PGE_MusicPlayer::getMusAlbum());
    ui->musCopyright->setText(PGE_MusicPlayer::getMusCopy());

    ui->gridLayout->update();
}

void MusPlayer_Qt::on_trackPrev_clicked()
{
    ui->trackID->stepDown();
    on_trackID_editingFinished();
}

void MusPlayer_Qt::on_trackNext_clicked()
{
    ui->trackID->stepUp();
    on_trackID_editingFinished();
}

void MusPlayer_Qt::on_disableSpcEcho_clicked(bool checked)
{
    Mix_GME_SetSpcEchoDisabled(PGE_MusicPlayer::s_playMus, checked ? 1 : 0);
}

void MusPlayer_Qt::on_tempo_valueChanged(int tempo)
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        double tempoFactor = 1.0 + 0.01 * double(tempo);
        Mix_SetMusicTempo(PGE_MusicPlayer::s_playMus, tempoFactor);
        qDebug() << "Changed tempo factor: " << tempoFactor;
        QToolTip::showText(QCursor::pos(), QString("%1").arg(tempoFactor), this);
    }
#else
    Q_UNUSED(tempo);
#endif
}

void MusPlayer_Qt::on_speed_valueChanged(int speed)
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        double speedFactor = 1.0 + 0.01 * double(speed);
        Mix_SetMusicSpeed(PGE_MusicPlayer::s_playMus, speedFactor);
        qDebug() << "Changed speed factor: " << speedFactor;
        QToolTip::showText(QCursor::pos(), QString("%1").arg(speedFactor), this);
    }
#else
    Q_UNUSED(speed);
#endif
}

void MusPlayer_Qt::on_pitch_valueChanged(int pitch)
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        double pitchFactor = 1.0 + 0.01 * double(pitch);
        Mix_SetMusicPitch(PGE_MusicPlayer::s_playMus, pitchFactor);
        qDebug() << "Changed pitch factor: " << pitchFactor;
        QToolTip::showText(QCursor::pos(), QString("%1").arg(pitchFactor), this);
    }
#else
    Q_UNUSED(speed);
#endif
}

void MusPlayer_Qt::on_volumeGeneral_valueChanged(int volume)
{
    Mix_MasterVolume(volume);
    qDebug() << "Changed general volume: " << volume;
    QToolTip::showText(QCursor::pos(), QString("%1").arg(volume), this);
}

void MusPlayer_Qt::on_recordWav_clicked(bool checked)
{
    if(checked)
    {
        PGE_MusicPlayer::disableHooks();
        PGE_MusicPlayer::stopMusic();
        PGE_MusicPlayer::enableHooks();
        ui->play->setText(tr("Play"));
        QFileInfo twav(m_currentMusic);
        PGE_MusicPlayer::stopWavRecording();
        QString wavPathBase = twav.absoluteDir().absolutePath() + "/" + twav.baseName();
        QString fileSuffix = twav.suffix();

        switch(PGE_MusicPlayer::musicTypeI())
        {
        case MUS_GME:
            fileSuffix += QString("-t%1").arg(ui->trackID->value());
            break;

        case MUS_MID:
            {
                int synth = Mix_GetMidiPlayer();
                fileSuffix += QString("-s%1").arg(synth);

                if(synth == MIDI_ADLMIDI)
                {
                    int t = Mix_ADLMIDI_getTremolo();
                    int v = Mix_ADLMIDI_getVibrato();
                    int m = Mix_ADLMIDI_getScaleMod();
                    int l = Mix_ADLMIDI_getVolumeModel();
                    int e = Mix_ADLMIDI_getEmulator();
                    int c = Mix_ADLMIDI_getChipsCount();
                    int o = Mix_ADLMIDI_getChannelAllocMode();
                    fileSuffix += QString("-b%1").arg(Mix_ADLMIDI_getBankID());

                    if(t >= 0)
                        fileSuffix += QString("-t%1").arg(t);

                    if(v >= 0)
                        fileSuffix += QString("-v%1").arg(v);

                    if(m >= 0)
                        fileSuffix += QString("-m%1").arg(m);

                    if(l > 0)
                        fileSuffix += QString("-l%1").arg(l);

                    if(o > 0)
                        fileSuffix += QString("-o%1").arg(o);

                    fileSuffix += QString("-e%1").arg(e);
                    fileSuffix += QString("-c%1").arg(c);
                }

                if(synth == MIDI_OPNMIDI)
                {
                    int l = Mix_OPNMIDI_getVolumeModel();
                    int e = Mix_OPNMIDI_getEmulator();
                    int c = Mix_OPNMIDI_getChipsCount();
                    int o = Mix_OPNMIDI_getChannelAllocMode();

                    if(l > 0)
                        fileSuffix += QString("-l%1").arg(l);

                    if(o > 0)
                        fileSuffix += QString("-o%1").arg(o);

                    fileSuffix += QString("-e%1").arg(e);
                    fileSuffix += QString("-c%1").arg(c);
                }
            }
            break;

        case MUS_ADLMIDI:
            fileSuffix += "-s0";
            break;
        case MUS_OPNMIDI:
            fileSuffix += "-s3";
            break;
        case MUS_EDMIDI:
            fileSuffix += "-s5";
            break;
        case MUS_FLUIDLITE:
            fileSuffix += "-s4";
            break;
        case MUS_NATIVEMIDI:
            fileSuffix += "-s2";
            break;
        default:
            break; // Do Nothing
        }

        QString wavPath = wavPathBase + "-" + fileSuffix + ".wav";

        int count = 1;
        while(QFile::exists(wavPath))
            wavPath = wavPathBase + "-" + fileSuffix + QString("-%1.wav").arg(count++);
        PGE_MusicPlayer::startWavRecording(wavPath);
        on_play_clicked();
        ui->open->setEnabled(false);
        ui->play->setEnabled(false);
        m_blinker.start(500);
    }
    else
    {
        on_stop_clicked();
        PGE_MusicPlayer::stopWavRecording();
        ui->open->setEnabled(true);
        ui->play->setEnabled(true);
        m_blinker.stop();
        ui->recordWav->setStyleSheet("");
    }
}

void MusPlayer_Qt::_blink_red()
{
    m_blink_state = !m_blink_state;

    if(m_blink_state)
        ui->recordWav->setStyleSheet("background-color : red; color : black;");
    else
        ui->recordWav->setStyleSheet("background-color : black; color : red;");
}

void MusPlayer_Qt::updatePositionSlider()
{
    const double pos = Mix_GetMusicPosition(PGE_MusicPlayer::s_playMus);
    m_positionWatcherLock = true;
    if(pos < 0.0)
    {
        m_seekBar->setEnabled(false);
        m_positionWatcher.stop();
    }
    else
    {
        m_seekBar->setPosition(pos);
        ui->playingTimeLabel->setText(timePosToString(pos));
    }
    m_positionWatcherLock = false;
}

void MusPlayer_Qt::musicPosition_seeked(double value)
{
    if(m_positionWatcherLock)
        return;

    qDebug() << "Seek to: " << value;
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus))
    {
        Mix_SetMusicPositionStream(PGE_MusicPlayer::s_playMus, value);
        ui->playingTimeLabel->setText(timePosToString(value));
    }
}


void MusPlayer_Qt::on_actionOpen_triggered()
{
    on_open_clicked();
}

void MusPlayer_Qt::on_actionQuit_triggered()
{
    this->close();
}

void MusPlayer_Qt::on_actionHelpLicense_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.gnu.org/licenses/gpl"));
}

void MusPlayer_Qt::on_actionHelpAbout_triggered()
{
    QString library;
    /* Append library version to the title */
    const SDL_version *mixer_ver = Mix_Linked_Version();
#if defined(SDL_MIXER_X)
    library += QString("SDL Mixer X %1.%2.%3")
             .arg(mixer_ver->major)
             .arg(mixer_ver->minor)
             .arg(mixer_ver->patch);
#else
    library += QString("SDL2_mixer %1.%2.%3")
             .arg(mixer_ver->major)
             .arg(mixer_ver->minor)
             .arg(mixer_ver->patch);
#endif

    QMessageBox::about(this, tr("SDL Mixer X Music Player"),
        tr("SDL Mixer X Music Player\n\n"
           "Version %1\n\n"
           "Linked library: %2")
            .arg(V_FILE_VERSION)
            .arg(library)
    );
}

void MusPlayer_Qt::on_actionHelpGitHub_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/WohlSoft/PGE-Project"));
}

void MusPlayer_Qt::on_actionMidiSetup_triggered()
{
    m_setupMidi->show();
    QRect g = this->frameGeometry();
    m_setupMidi->move(g.right(), g.top());
    m_setupMidi->update();
    m_setupMidi->repaint();
}

void MusPlayer_Qt::on_actionAudioSetup_triggered()
{
    SetupAudio as(this);
    if(as.exec() == QDialog::Accepted)
    {
        on_actionEnableReverb_triggered(PGE_MusicPlayer::reverbEnabled);
        on_actionEnableEcho_triggered(PGE_MusicPlayer::echoEnabled);
        m_setupMidi->sendSetup();
        m_sfxTester->reloadSfx();
    }
}

void MusPlayer_Qt::on_actionTuneReverb_triggered()
{
    m_reverbTune->show();
    QRect g = this->frameGeometry();
    m_reverbTune->move(g.right(), g.top());
    m_reverbTune->on_reverb_reload_clicked();
    m_reverbTune->update();
    m_reverbTune->repaint();
}

void MusPlayer_Qt::on_actionTuneEcho_triggered()
{
    m_echoTune->show();
    QRect g = this->frameGeometry();
    m_echoTune->move(g.right(), g.top());
    m_echoTune->on_echo_reload_clicked();
    m_echoTune->update();
    m_echoTune->repaint();
}


void MusPlayer_Qt::on_actionSfxTesting_triggered()
{
    m_sfxTester->show();
    QRect g = this->frameGeometry();
    m_sfxTester->move(g.left(), g.bottom());
    m_sfxTester->update();
    m_sfxTester->repaint();
}


void MusPlayer_Qt::on_actionMultiMusicTesting_triggered()
{
    m_multiMusicTest->show();
    QRect g = this->frameGeometry();
    m_multiMusicTest->move(g.left(), g.bottom());
    m_multiMusicTest->update();
    m_multiMusicTest->repaint();
}

void MusPlayer_Qt::on_actionMultiSFXTesting_triggered()
{
    m_multiSfxTest->show();
    QRect g = this->frameGeometry();
    m_multiSfxTest->move(g.left(), g.bottom());
    m_multiSfxTest->update();
    m_multiSfxTest->repaint();
}

void MusPlayer_Qt::on_actionMusicFX_triggered()
{
    m_musicFx->show();
    QRect g = this->frameGeometry();
    m_musicFx->move(g.right(), g.top());
    m_musicFx->update();
    m_musicFx->repaint();
}


void MusPlayer_Qt::on_actionTracksMuter_triggered()
{
    m_trackMuter->show();
    QRect g = this->frameGeometry();
    m_trackMuter->move(g.right(), g.top());
    m_trackMuter->update();
    m_trackMuter->repaint();
}

void MusPlayer_Qt::on_actionEnableReverb_triggered(bool checked)
{
    PGE_MusicPlayer::reverbEnabled = checked;
    PGE_MusicPlayer::reverbEabled(PGE_MusicPlayer::reverbEnabled);
    ui->actionTuneReverb->setEnabled(PGE_MusicPlayer::reverbEnabled);

    if(PGE_MusicPlayer::reverbEnabled)
    {
        m_reverbTune->loadSetup();
        m_reverbTune->sendAll();
    }
}

void MusPlayer_Qt::on_actionEnableEcho_triggered(bool checked)
{
    PGE_MusicPlayer::echoEnabled = checked;
    PGE_MusicPlayer::echoEabled(PGE_MusicPlayer::echoEnabled);
    ui->actionTuneEcho->setEnabled(PGE_MusicPlayer::echoEnabled);

    if(PGE_MusicPlayer::echoEnabled)
    {
        m_echoTune->loadSetup();
        m_echoTune->sendAll();
    }
}

void MusPlayer_Qt::on_actionFileAssoc_triggered()
{
    AssocFiles af(this);
    af.setWindowModality(Qt::WindowModal);
    af.exec();
}

void MusPlayer_Qt::on_actionPause_the_audio_stream_triggered(bool checked)
{
    Mix_PauseAudio(checked);
}

void MusPlayer_Qt::cleanLoopChecks()
{
    ui->actionLoopForever->setChecked(false);
    ui->actionPlay1Time->setChecked(false);
    ui->actionPlay2Times->setChecked(false);
    ui->actionPlay3Times->setChecked(false);
}

void MusPlayer_Qt::on_actionLoopForever_triggered()
{
    cleanLoopChecks();
    ui->actionLoopForever->setChecked(true);
    PGE_MusicPlayer::setMusicLoops(-1);
    restartMusic();
}

void MusPlayer_Qt::on_actionPlay1Time_triggered()
{
    cleanLoopChecks();
    ui->actionPlay1Time->setChecked(true);
    PGE_MusicPlayer::setMusicLoops(0);
    restartMusic();
}

void MusPlayer_Qt::on_actionPlay2Times_triggered()
{
    cleanLoopChecks();
    ui->actionPlay2Times->setChecked(true);
    PGE_MusicPlayer::setMusicLoops(2);
    restartMusic();
}

void MusPlayer_Qt::on_actionPlay3Times_triggered()
{
    cleanLoopChecks();
    ui->actionPlay3Times->setChecked(true);
    PGE_MusicPlayer::setMusicLoops(3);
    restartMusic();
}
