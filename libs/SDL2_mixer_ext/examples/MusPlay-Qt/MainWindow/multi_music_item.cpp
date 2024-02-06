#include <QMessageBox>
#include <QToolTip>
#include <QtDebug>
#include <QDateTime>
#include <QFileInfo>
#include <cmath>

#include "multi_music_item.h"
#include "ui_multi_music_item.h"
#include "seek_bar.h"
#include "musicfx.h"
#include "track_muter.h"


static const char *musicTypeC(Mix_Music *mus)
{
    Mix_MusicType type  = MUS_NONE;
    if(mus)
        type = Mix_GetMusicType(mus);

    return (
               type == MUS_NONE ? "No Music" :
               type == MUS_CMD ? "CMD" :
               type == MUS_WAV ? "PCM Wave" :
               type == MUS_MOD ? "Tracker music" :
               type == MUS_MID ? "MIDI" :
               type == MUS_OGG ? "OGG" :
               type == MUS_MP3 ? "MP3" :
               type == MUS_FLAC ? "FLAC" :
#ifdef SDL_MIXER_X
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
(SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 2)
               type == MUS_OPUS ? "OPUS" :
#   endif
               type == MUS_ADLMIDI ? "IMF/MUS/XMI" :
               type == MUS_OPNMIDI ? "MUS/XMI(OPN)" :
               type == MUS_FLUIDLITE ? "MUS/XMI(Fluid)" :
               type == MUS_NATIVEMIDI ? "MUS/XMI(Native)" :
               type == MUS_GME ? "GME Chiptune" :
#else
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
(SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION > 0) || \
(SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION == 0 && SDL_MIXER_PATCHLEVEL >= 4)
               type == MUS_OPUS ? "OPUS" :
#   endif
#endif
               "<Unknown>");
}

static QString timePosToString(double pos, const QString &format = "hh:mm:ss")
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    return QDateTime::fromSecsSinceEpoch((uint)std::floor(pos), Qt::UTC).toString(format);
#else
    return QDateTime::fromTime_t((uint)std::floor(pos)).toUTC().toString(format);
#endif
}

MultiMusicItem::MultiMusicItem(QString music, QWidget *parent) :
    QWidget(parent),
    m_curMusPath(music),
    ui(new Ui::MultiMusicItem)
{
    ui->setupUi(this);
    ui->musicTitle->setText(QString("[No music]"));
    ui->musicType->setText(QString(musicTypeC(nullptr)));

    m_seekBar = new SeekBar(this);
    ui->gridLayout->removeWidget(ui->musicPosition);
    ui->gridLayout->addWidget(m_seekBar, 1, 0, 1, 9);
    m_seekBar->setLength(100);
    m_seekBar->setVisible(true);
    ui->musicPosition->setVisible(false);

    QObject::connect(&m_positionWatcher, SIGNAL(timeout()), this, SLOT(updatePositionSlider()));
    QObject::connect(m_seekBar, &SeekBar::positionSeeked, this, &MultiMusicItem::musicPosition_seeked);

    ui->tempo->setVisible(false);
    m_seekBar->setEnabled(false);

    ui->showMusicFX->setEnabled(false);
}

MultiMusicItem::~MultiMusicItem()
{
    on_stop_clicked();
    closeMusic();
    delete ui;
}

void MultiMusicItem::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MultiMusicItem::on_stop_clicked()
{
    ui->playpause->setIcon(QIcon(":/buttons/play.png"));
    ui->playpause->setToolTip(tr("Play"));

    if(!m_curMus)
        return;

    if(Mix_PlayingMusicStream(m_curMus))
    {
        qDebug() << "Stopping music" << m_curMusPath;
        Mix_HaltMusicStream(m_curMus);
    }
}

void MultiMusicItem::updatePositionSlider()
{
    if(!m_curMus)
        return;

    const double pos = Mix_GetMusicPosition(m_curMus);
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

void MultiMusicItem::closeMusic()
{
    if(m_curMus)
    {
        m_positionWatcher.stop();
        on_stop_clicked();
        Mix_FreeMusic(m_curMus);
        m_curMus = nullptr;

        ui->musicTitle->setText(QString("[No music]"));
        ui->playingTimeLabel->setText("--:--:--");
        ui->playingTimeLenghtLabel->setText("/ --:--:--");
        m_seekBar->setPosition(0.0);
        m_seekBar->setEnabled(false);
    }

    ui->showMusicFX->setEnabled(false);

    if(m_musicFX)
    {
        m_musicFX->hide();
        m_musicFX->setDstMusic(m_curMus);
    }
}

void MultiMusicItem::openMusic()
{
    if(m_curMus)
        return; // Already open

    qDebug() << "Opening" << m_curMusPath << ui->pathArgs->text();
    Mix_SetLockMIDIArgs(ui->pathArgs->text().isEmpty() ? 1 : 0);
    m_curMus = Mix_LoadMUS_RW_ARG(SDL_RWFromFile(m_curMusPath.toUtf8().data(), "rb"),
                                  1,
                                  ui->pathArgs->text().toUtf8().data());

    if(!m_curMus)
        QMessageBox::warning(this, tr("Can't open music"), tr("Can't open music: %1").arg(Mix_GetError()));
    else
    {
        Mix_HookMusicStreamFinished(m_curMus, musicStoppedHook, this);
        Mix_SetMusicFileName(m_curMus, m_curMusPath.toUtf8().data());

        auto *tit = Mix_GetMusicTitle(m_curMus);
        if(tit)
            ui->musicTitle->setText(QString(tit));
        else
            ui->musicTitle->setText(QFileInfo(m_curMusPath).fileName());

        ui->musicType->setText(QString(musicTypeC(m_curMus)));

        m_positionWatcher.stop();
        m_seekBar->setEnabled(false);
        ui->playingTimeLabel->setText("--:--:--");
        ui->playingTimeLenghtLabel->setText("/ --:--:--");

        double total = Mix_MusicDuration(m_curMus);
        double curPos = Mix_GetMusicPosition(m_curMus);

        double loopStart = Mix_GetMusicLoopStartTime(m_curMus);
        double loopEnd = Mix_GetMusicLoopEndTime(m_curMus);

        ui->tempo->setVisible(Mix_GetMusicTempo(m_curMus) >= 0.0);

        if(total > 0.0 && curPos >= 0.0)
        {
            m_seekBar->setEnabled(true);
            m_seekBar->setLength(total);
            m_seekBar->setPosition(0.0);
            m_seekBar->setLoopPoints(loopStart, loopEnd);
            ui->playingTimeLenghtLabel->setText(timePosToString(total, "/ hh:mm:ss"));
            m_positionWatcher.start(128);
        }

        ui->showMusicFX->setEnabled(true);
        if(m_musicFX)
        {
            m_musicFX->setDstMusic(m_curMus);
            m_musicFX->setTitle(ui->musicTitle->text());
            m_musicFX->resetAll();
        }
    }
}

void MultiMusicItem::reOpenMusic()
{
    if(m_curMus)
        closeMusic();
    openMusic();
}

void MultiMusicItem::on_playpause_clicked()
{
    openMusic();

    if(!m_curMus)
        return;

    if(Mix_PlayingMusicStream(m_curMus))
    {
        if(Mix_PausedMusicStream(m_curMus))
        {
            qDebug() << "Resuming music" << m_curMusPath;
            Mix_ResumeMusicStream(m_curMus);
            ui->playpause->setIcon(QIcon(":/buttons/pause.png"));
            ui->playpause->setToolTip(tr("Pause"));
        }
        else
        {
            qDebug() << "Pausing music" << m_curMusPath;
            Mix_PauseMusicStream(m_curMus);
            ui->playpause->setIcon(QIcon(":/buttons/play.png"));
            ui->playpause->setToolTip(tr("Resume"));
        }
    }
    else
    {
        qDebug() << "Play music" << m_curMusPath;
        int ret = Mix_PlayMusicStream(m_curMus, -1);

        if(ret < 0)
        {
            QMessageBox::warning(this, tr("Can't play song"), tr("Error has occured: %1").arg(Mix_GetError()));
            return;
        }

        double total = Mix_MusicDuration(m_curMus);
        double curPos = Mix_GetMusicPosition(m_curMus);
        if(total > 0.0 && curPos >= 0.0)
            m_positionWatcher.start(128);

        ui->playpause->setIcon(QIcon(":/buttons/pause.png"));
        ui->playpause->setToolTip(tr("Pause"));
    }
}


void MultiMusicItem::on_closeMusic_clicked()
{
    closeMusic();
    emit wannaClose(this);
}

void MultiMusicItem::on_pathArgs_editingFinished()
{
    if(ui->pathArgs->isModified())
    {
        reOpenMusic();
        ui->pathArgs->setModified(false);
    }
}

void MultiMusicItem::on_musicVolume_sliderMoved(int position)
{
    if(m_curMus)
    {
        Mix_VolumeMusicStream(m_curMus, position);
        QToolTip::showText(QCursor::pos(), QString("%1").arg(position), this);
    }
}


void MultiMusicItem::on_tempo_sliderMoved(int position)
{
    if(m_curMus)
    {
        double tempoFactor = 1.0 + 0.01 * double(position);
        Mix_SetMusicTempo(m_curMus, tempoFactor);
        QToolTip::showText(QCursor::pos(), QString("%1").arg(tempoFactor), this);
    }
}

void MultiMusicItem::musicPosition_seeked(double value)
{
    if(m_positionWatcherLock)
        return;

    qDebug() << "Seek to: " << value;
    if(Mix_PlayingMusicStream(m_curMus))
    {
        Mix_SetMusicPositionStream(m_curMus, value);
        ui->playingTimeLabel->setText(timePosToString(value));
    }
}

void MultiMusicItem::musicStoppedSlot()
{
    m_positionWatcher.stop();
    ui->playingTimeLabel->setText("00:00:00");
    m_seekBar->setPosition(0.0);
    ui->playpause->setToolTip(tr("Play"));
    ui->playpause->setIcon(QIcon(":/buttons/play.png"));
}

void MultiMusicItem::musicStoppedHook(Mix_Music *, void *self)
{
    MultiMusicItem *me = reinterpret_cast<MultiMusicItem*>(self);
    Q_ASSERT(me);
    QMetaObject::invokeMethod(me, "musicStoppedSlot", Qt::QueuedConnection);
}

void MultiMusicItem::on_playFadeIn_clicked()
{
    openMusic();

    if(!m_curMus)
        return;

    if(!Mix_PlayingMusicStream(m_curMus))
    {
        double total = Mix_MusicDuration(m_curMus);
        double curPos = Mix_GetMusicPosition(m_curMus);
        if(total > 0.0 && curPos >= 0.0)
            m_positionWatcher.start(128);
    }

    if(!Mix_PlayingMusicStream(m_curMus) || Mix_FadingMusicStream(m_curMus))
    {
        if(Mix_FadeInMusicStream(m_curMus, -1, 4000) < 0)
            qWarning() << "Mix_FadeInMusicStream" << Mix_GetError();
    }
}


void MultiMusicItem::on_stopFadeOut_clicked()
{
    if(!m_curMus)
        return;

    if(Mix_PlayingMusicStream(m_curMus))
    {
        if(Mix_FadeOutMusicStream(m_curMus, 4000) < 0)
            qWarning() << "Mix_FadeOutMusicStream" << Mix_GetError();
    }
}

void MultiMusicItem::on_showMusicFX_clicked()
{
    if(!m_curMus)
        return;

    if(!m_musicFX)
    {
        m_musicFX = new MusicFX(this);
        m_musicFX->setModal(false);
    }

    m_musicFX->show();
    auto g = QCursor::pos();
    m_musicFX->move(g.x(), g.y());
    m_musicFX->update();
    m_musicFX->repaint();
    m_musicFX->setDstMusic(m_curMus);
    m_musicFX->setTitle(ui->musicTitle->text());
}


void MultiMusicItem::on_showTracksOnOff_clicked()
{
    if(!m_curMus)
        return;

    if(!m_trackMuter)
    {
        m_trackMuter = new TrackMuter(this);
        m_trackMuter->setModal(false);
    }

    m_trackMuter->show();
    auto g = QCursor::pos();
    m_trackMuter->move(g.x(), g.y());
    m_trackMuter->update();
    m_trackMuter->repaint();
    m_trackMuter->setDstMusic(m_curMus);
    m_trackMuter->setTitle(ui->musicTitle->text());
}

