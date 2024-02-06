#ifndef MUSPLAYER_QT_H
#define MUSPLAYER_QT_H

#include "musplayer_base.h"

#include <QMainWindow>

#include <QUrl>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>

class SeekBar;
class SfxTester;
class SetupMidi;
class EchoTune;
class ReverbTune;
class MultiMusicTest;
class MultiSfxTester;
class MusicFX;
class TrackMuter;

namespace Ui {
class MusPlayerQt;
}

struct Mix_Chunk;

class MusPlayer_Qt : public QMainWindow, public MusPlayerBase
{
    Q_OBJECT
public:
    explicit MusPlayer_Qt(QWidget *parent = nullptr);
    virtual ~MusPlayer_Qt();

protected:
    /*!
     * \brief Drop event, triggers when some object was dropped to main window
     * \param e Event parameters
     */
    void dropEvent(QDropEvent *e);

    /*!
     * \brief Drop enter event, triggers when mouse came to window with holden objects
     * \param e Event parameters
     */
    void dragEnterEvent(QDragEnterEvent *e);

    void moveEvent(QMoveEvent *event);

public slots:
    /*!
     * \brief Context menu
     * \param pos Mouse cursor position
     */
    void contextMenu(const QPoint &pos);

    void openMusicByArg(QString musPath);

    //void setPlayListMode(bool plMode);

    //void playList_pushCurrent(bool x = false);
    //void playList_popCurrent(bool x = false);
    //void playListNext();

private slots:
    void on_actionTuneReverb_triggered();
    void on_actionTuneEcho_triggered();
    void on_actionAudioSetup_triggered();
    void restartMusic();

    void musicStopped();

    void on_open_clicked();
    void on_stop_clicked();
    void on_play_clicked();

    void on_trackID_editingFinished();
    void on_trackPrev_clicked();
    void on_trackNext_clicked();
    void on_disableSpcEcho_clicked(bool checked);

    void on_tempo_valueChanged(int tempo);
    void on_speed_valueChanged(int speed);
    void on_pitch_valueChanged(int pitch);
    void on_volumeGeneral_valueChanged(int position);

    void on_recordWav_clicked(bool checked);

    /*!
     * \brief Changes color of "Recording WAV" label between black and red
     */
    void _blink_red();

    void updatePositionSlider();
    void musicPosition_seeked(double value);

    void on_actionOpen_triggered();
    void on_actionQuit_triggered();
    void on_actionHelpLicense_triggered();
    void on_actionHelpAbout_triggered();
    void on_actionHelpGitHub_triggered();
    void on_actionMidiSetup_triggered();
    void on_actionSfxTesting_triggered();
    void on_actionMultiMusicTesting_triggered();
    void on_actionMultiSFXTesting_triggered();
    void on_actionMusicFX_triggered();
    void on_actionTracksMuter_triggered();
    void on_actionEnableReverb_triggered(bool checked);
    void on_actionEnableEcho_triggered(bool checked);
    void on_actionFileAssoc_triggered();
    void on_actionPause_the_audio_stream_triggered(bool checked);

    void cleanLoopChecks();
    void on_actionLoopForever_triggered();
    void on_actionPlay1Time_triggered();
    void on_actionPlay2Times_triggered();
    void on_actionPlay3Times_triggered();

    void refreshMetaData();

private:
    bool playListMode = false;
    //PlayListModel playList;
    //! Controlls blinking of the wav-recording label
    QTimer m_blinker;
    QTimer m_positionWatcher;
    bool   m_positionWatcherLock = false;
    //! UI form class pointer
    Ui::MusPlayerQt *ui = nullptr;

    QPoint    m_oldWindowPos;

    SeekBar   *m_seekBar = nullptr;
    SfxTester *m_sfxTester = nullptr;
    SetupMidi *m_setupMidi = nullptr;
    EchoTune  *m_echoTune = nullptr;
    ReverbTune  *m_reverbTune = nullptr;
    MultiMusicTest *m_multiMusicTest = nullptr;
    MultiSfxTester *m_multiSfxTest = nullptr;
    MusicFX *m_musicFx = nullptr;
    TrackMuter *m_trackMuter = nullptr;
};

#endif // MUSPLAYER_QT_H
