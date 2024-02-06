#ifndef MULTI_MUSIC_ITEM_H
#define MULTI_MUSIC_ITEM_H

#include <QWidget>
#include <QTimer>

#include "SDL.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif


class SeekBar;
class MusicFX;
class TrackMuter;

namespace Ui {
class MultiMusicItem;
}

class MultiMusicItem : public QWidget
{
    Q_OBJECT
    Mix_Music *m_curMus = nullptr;
    QString m_curMusPath;
    SeekBar *m_seekBar = nullptr;
    MusicFX *m_musicFX = nullptr;
    TrackMuter *m_trackMuter = nullptr;

    friend class MultiMusicTest;

public:
    explicit MultiMusicItem(QString music, QWidget *parent = nullptr);
    ~MultiMusicItem();

signals:
    void wannaClose(MultiMusicItem *self);

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_showMusicFX_clicked();
    void on_showTracksOnOff_clicked();

    void on_tempo_sliderMoved(int position);
    void on_musicVolume_sliderMoved(int position);
    void on_pathArgs_editingFinished();
    void on_closeMusic_clicked();
    void on_playpause_clicked();
    void on_stop_clicked();
    void on_stopFadeOut_clicked();
    void on_playFadeIn_clicked();

    void updatePositionSlider();
    void musicPosition_seeked(double value);
    void musicStoppedSlot();

private:
    void closeMusic();
    void openMusic();
    void reOpenMusic();

    static void musicStoppedHook(Mix_Music *mus, void *self);

    Ui::MultiMusicItem *ui;

    QTimer m_positionWatcher;
    bool   m_positionWatcherLock = false;
};

#endif // MULTI_MUSIC_ITEM_H
