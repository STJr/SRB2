#include "musplayer_base.h"
#include "../Player/mus_player.h"

MusPlayerBase::MusPlayerBase() :
    m_blink_state(false),
    m_prevTrackID(0)
{}

MusPlayerBase::~MusPlayerBase()
{}

void MusPlayerBase::openMusicByArg(QString)
{}

void MusPlayerBase::on_open_clicked()
{}

void MusPlayerBase::on_stop_clicked()
{}

void MusPlayerBase::on_play_clicked()
{}

void MusPlayerBase::on_volume_valueChanged(int value)
{
    PGE_MusicPlayer::changeVolume(value);
}

void MusPlayerBase::on_trackID_editingFinished() {}

void MusPlayerBase::on_recordWav_clicked(bool) {}

void MusPlayerBase::_blink_red()
{}

void MusPlayerBase::on_resetDefaultADLMIDI_clicked()
{}
