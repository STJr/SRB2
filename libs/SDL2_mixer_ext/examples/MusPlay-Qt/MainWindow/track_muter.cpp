#include <QCheckBox>
#include <QScrollArea>
#include <QGridLayout>

#include "flowlayout.h"

#include "track_muter.h"
#include "ui_track_muter.h"
#include "../Player/mus_player.h"

TrackMuter::TrackMuter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TrackMuter)
{
    ui->setupUi(this);
    m_tracksLayout = new FlowLayout(ui->tracks);
    ui->tracks->setLayout(m_tracksLayout);
}

TrackMuter::~TrackMuter()
{
    delete ui;
}

void TrackMuter::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TrackMuter::setDstMusic(Mix_Music *mus)
{
    m_dstMusic = mus;
    buildTracksList();
}

void TrackMuter::setTitle(const QString &tit)
{
    setWindowTitle(QString("Music Tracks - %1").arg(tit));
}

void TrackMuter::buildTracksList()
{
    for(auto *i : m_items)
    {
        m_tracksLayout->removeWidget(i);
        delete i;
    }
    m_items.clear();

    auto *m = getMus();
    if(!m)
        return;

    int tracks = Mix_GetMusicTracks(m);
    if(tracks < 1)
        return;

    for(int i = 0; i < tracks; ++i)
    {
        QCheckBox *t = new QCheckBox();
        t->setText(tr("Track %1").arg(i));
        t->setChecked(true);
        QObject::connect(t, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                         [this, i](bool s)->void
        {
            auto *m = getMus();
            if(m)
                Mix_SetMusicTrackMute(m, i, s ? 0 : 1);
        });
        m_items.push_back(t);
        m_tracksLayout->addWidget(t);
    }
}

Mix_Music *TrackMuter::getMus()
{
    if(m_dstMusic)
        return m_dstMusic;
    else
        return PGE_MusicPlayer::s_playMus;
}
