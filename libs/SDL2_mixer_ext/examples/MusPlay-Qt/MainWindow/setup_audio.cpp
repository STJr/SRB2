#include <QMessageBox>

#include "setup_audio.h"
#include "ui_setup_audio.h"

#include "../Player/mus_player.h"

SetupAudio::SetupAudio(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetupAudio)
{
    ui->setupUi(this);
    ui->sampleFormat->addItem("U8", AUDIO_U8);
    ui->sampleFormat->addItem("S8", AUDIO_S8);
    ui->sampleFormat->addItem("U16MSB", AUDIO_U16MSB);
    ui->sampleFormat->addItem("S16MSB", AUDIO_S16MSB);
    ui->sampleFormat->addItem("U16LSB", AUDIO_U16LSB);
    ui->sampleFormat->addItem("S16LSB", AUDIO_S16LSB);
    ui->sampleFormat->addItem("S32MSB", AUDIO_S32MSB);
    ui->sampleFormat->addItem("S32LSB", AUDIO_S32LSB);
    ui->sampleFormat->addItem("F32MSB", AUDIO_F32MSB);
    ui->sampleFormat->addItem("F32LSB", AUDIO_F32LSB);

#ifdef _WIN32
    ui->output->clear();
    ui->output->addItem("Default", QString());
    ui->output->addItem("WASAPI", "wasapi");
    ui->output->addItem("DirectSound", "directsound");
    ui->output->addItem("WinMM", "winmm");
#else
    ui->output_label->setVisible(false);
    ui->output->setVisible(false);
#endif

    ui->sampleRate->setValue(PGE_MusicPlayer::getSampleRate());
    int i = ui->sampleFormat->findData(PGE_MusicPlayer::getSampleFormat());
    ui->sampleFormat->setCurrentIndex(i);
    ui->channels->setValue(PGE_MusicPlayer::getChannels());
    i = ui->output->findData(PGE_MusicPlayer::getOutputType());
    ui->output->setCurrentIndex(i);
}

SetupAudio::~SetupAudio()
{
    delete ui;
}

void SetupAudio::on_confirm_accepted()
{
    QString error;
    int rate = ui->sampleRate->value();
    Uint16 format = static_cast<Uint16>(ui->sampleFormat->currentData().toUInt());
    int channels = ui->channels->value();
    QString output = ui->output->currentData().toString();

    PGE_MusicPlayer::closeAudio();

    if(!PGE_MusicPlayer::openAudioWithSpec(error, rate, format, channels, output))
    {
        QMessageBox::warning(this, tr("Audio loading error"), tr("Failed to load audio: %1").arg(error));
        if(!PGE_MusicPlayer::openAudio(error))
            QMessageBox::warning(this, tr("Audio loading error"), tr("Failed to re-load audio: %1").arg(error));
        return;
    }

    PGE_MusicPlayer::setSpec(rate, format, channels, output);
    PGE_MusicPlayer::saveAudioSettings();
    accept();
}

void SetupAudio::on_restoreDefaults_clicked()
{
    ui->sampleRate->setValue(MIX_DEFAULT_FREQUENCY);
    ui->channels->setValue(MIX_DEFAULT_CHANNELS);
    int i = ui->sampleFormat->findData(MIX_DEFAULT_FORMAT);
    ui->sampleFormat->setCurrentIndex(i);
    ui->output->setCurrentIndex(0);
}
