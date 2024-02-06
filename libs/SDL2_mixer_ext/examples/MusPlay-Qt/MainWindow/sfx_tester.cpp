#include "sfx_tester.h"
#include "ui_sfx_tester.h"
#include <QFocusEvent>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "qfile_dialogs_default_options.hpp"

#include "../Player/mus_player.h"

SfxTester::SfxTester(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SfxTester)
{
    ui->setupUi(this);

    layout()->activate();
    adjustSize();

    QSettings setup;
    m_testSfxDir = setup.value("RecentSfxDir", "").toString();
}

SfxTester::~SfxTester()
{
    delete ui;
}

void SfxTester::reloadSfx()
{
    if(!m_recentSfxFile.isEmpty())
        openSfx(m_recentSfxFile);
}

void SfxTester::dropEvent(QDropEvent *e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    QString sfxFile;

    for(const QUrl &url : e->mimeData()->urls())
    {
        const QString &fileName = url.toLocalFile();
        sfxFile = fileName;
    }

    if(!sfxFile.isEmpty())
        openSfx(sfxFile);

    e->accept();
}

void SfxTester::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void SfxTester::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SfxTester::closeEvent(QCloseEvent *)
{
    if(m_testSfx)
        Mix_FreeChunk(m_testSfx);
    m_testSfx = nullptr;

    ui->sfx_file->clear();

    QSettings setup;
    setup.setValue("RecentSfxDir", m_testSfxDir);
    setup.setValue("RecentSfxFile", m_recentSfxFile);
    setup.sync();

    m_recentSfxFile.clear();
}

void SfxTester::on_sfx_open_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open SFX file"),
                   (m_testSfxDir.isEmpty() ? QApplication::applicationDirPath() : m_testSfxDir),
                   "All (*.*)", nullptr, c_fileDialogOptions);

    if(file.isEmpty())
        return;
    openSfx(file);
}

void SfxTester::openSfx(const QString &path)
{
    if(m_testSfx)
    {
        Mix_HaltChannel(0);
        Mix_FreeChunk(m_testSfx);
        m_testSfx = nullptr;
    }

    m_testSfx = Mix_LoadWAV(path.toUtf8().data());
    if(!m_testSfx)
        QMessageBox::warning(this, "SFX open error!", QString("Mix_LoadWAV: ") + Mix_GetError());
    else
    {
        QFileInfo f(path);
        m_testSfxDir = f.absoluteDir().absolutePath();
        ui->sfx_file->setText(f.fileName());
        m_recentSfxFile = path;
    }
}


void SfxTester::on_sfx_play_clicked()
{
    if(!m_testSfx)
        return;

    Mix_HaltChannel(0);
    if(!m_sendPanning)
        updatePositionEffect();
    else
        updatePanningEffect();

    updateChannelsFlip();

    if(Mix_PlayChannelTimedVolume(0,
                                  m_testSfx,
                                  ui->sfx_loops->value(),
                                  ui->sfx_timed->value(),
                                  ui->sfx_volume->value()) == -1)
    {
        QMessageBox::warning(this, "SFX play error!", QString("Mix_PlayChannelTimedVolume: ") + Mix_GetError());
    }
}

void SfxTester::on_sfx_fadeIn_clicked()
{
    if(!m_testSfx)
        return;

    Mix_HaltChannel(0);
    if(!m_sendPanning)
        updatePositionEffect();
    else
        updatePanningEffect();
    updateChannelsFlip();

    if(Mix_FadeInChannelTimedVolume(0,
                                    m_testSfx,
                                    ui->sfx_loops->value(),
                                    ui->sfx_fadems->value(),
                                    ui->sfx_timed->value(),
                                    ui->sfx_volume->value()) == -1)
    {
        QMessageBox::warning(this,
                             "SFX play error!",
                             QString("Mix_PlayChannelTimedVolume: %1").arg(Mix_GetError()));
    }
}

void SfxTester::on_sfx_stop_clicked()
{
    if(!m_testSfx)
        return;
    Mix_HaltChannel(0);
}

void SfxTester::on_sfx_fadeout_clicked()
{
    if(!m_testSfx)
        return;
    Mix_FadeOutChannel(0, ui->sfx_fadems->value());
}

void SfxTester::on_positionAngle_sliderMoved(int value)
{
    m_angle = (Sint16)(value);
    qDebug() << "Angle" << m_angle;
    updatePositionEffect();
    m_sendPanning = false;
}

void SfxTester::on_positionDistance_sliderMoved(int value)
{
    m_distance = (Uint8)value;
    qDebug() << "Distance" << m_distance;
    updatePositionEffect();
    m_sendPanning = false;
}

void SfxTester::on_positionReset_clicked()
{
    m_angle = 0;
    m_distance = 0;
    ui->positionAngle->setValue(0);
    ui->positionDistance->setValue(0);
    updatePositionEffect();
    m_sendPanning = false;
}

void SfxTester::on_panningLeft_valueChanged(int value)
{
    m_panLeft = (Uint8)value;
    updatePanningEffect();
    m_sendPanning = true;
}

void SfxTester::on_panningRight_valueChanged(int value)
{
    m_panRight = (Uint8)value;
    updatePanningEffect();
    m_sendPanning = true;
}

void SfxTester::on_stereoFlip_clicked(bool value)
{
    m_channelFlip = value ? 1 : 0;
    updateChannelsFlip();
}

void SfxTester::on_panningReset_clicked()
{
    m_channelFlip = 0;
    m_panLeft = 255;
    m_panRight = 255;
    ui->panningLeft->blockSignals(true);
    ui->panningLeft->setValue(255);
    ui->panningLeft->blockSignals(false);
    ui->panningRight->blockSignals(true);
    ui->panningRight->setValue(255);
    ui->panningRight->blockSignals(false);
    ui->stereoFlip->blockSignals(true);
    ui->stereoFlip->setChecked(false);
    ui->stereoFlip->blockSignals(false);
    updatePanningEffect();
    updateChannelsFlip();
    m_sendPanning = true;
}

void SfxTester::updatePositionEffect()
{
    Mix_SetPosition(0, m_angle, m_distance);
}

void SfxTester::updatePanningEffect()
{
    Mix_SetPanning(0, m_panLeft, m_panRight);
}

void SfxTester::updateChannelsFlip()
{
    Mix_SetReverseStereo(0, m_channelFlip);
}
