#ifndef MULTI_SFX_ITEM_H
#define MULTI_SFX_ITEM_H

#include <QWidget>

#include "SDL.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

namespace Ui {
class MultiSfxItem;
}

class MultiSfxItem : public QWidget
{
    Q_OBJECT

    int lastChannel = -1;
    Mix_Chunk *m_sfx = nullptr;
    QString m_sfxPath;

    friend class MultiSfxTester;

public:
    explicit MultiSfxItem(QString sfx, QWidget *parent = nullptr);
    ~MultiSfxItem();

    QString path() const;

    int channel() const;
    void setChannel(int c);

    int fadeDelay() const;
    void setFadeDelay(int f);

    int initVolume() const;
    void setInitVolume(int v);

signals:
    void wannaClose(MultiSfxItem *self);

private slots:
    void on_closeSfx_clicked();

    void on_play_clicked();
    void on_fadeIn_clicked();
    void on_fadeOut_clicked();
    void on_stop_clicked();

public slots:
    void sfxStoppedHook(int channel);

private:
    void closeSfx();
    void openSfx();
    void reOpenSfx();

private:
    Ui::MultiSfxItem *ui;
};

#endif // MULTI_SFX_ITEM_H
