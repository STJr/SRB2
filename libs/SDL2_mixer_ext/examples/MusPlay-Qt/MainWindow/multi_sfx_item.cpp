#include <QFileInfo>
#include <QMessageBox>
#include <QtDebug>
#include "multi_sfx_item.h"
#include "ui_multi_sfx_item.h"


MultiSfxItem::MultiSfxItem(QString sfx, QWidget *parent) :
    QWidget(parent),
    m_sfxPath(sfx),
    ui(new Ui::MultiSfxItem)
{
    ui->setupUi(this);
    openSfx();
}

MultiSfxItem::~MultiSfxItem()
{
    delete ui;
}

QString MultiSfxItem::path() const
{
    return m_sfxPath;
}

int MultiSfxItem::channel() const
{
    return ui->playChannel->value();
}

void MultiSfxItem::setChannel(int c)
{
    ui->playChannel->setValue(c);
}

int MultiSfxItem::fadeDelay() const
{
    return ui->fadeLen->value();
}

void MultiSfxItem::setFadeDelay(int f)
{
    ui->fadeLen->setValue(f);
}

int MultiSfxItem::initVolume() const
{
    return ui->initialVolume->value();
}

void MultiSfxItem::setInitVolume(int v)
{
    ui->initialVolume->setValue(v);
}

void MultiSfxItem::on_closeSfx_clicked()
{
    closeSfx();
    emit wannaClose(this);
}

void MultiSfxItem::on_play_clicked()
{
    openSfx();

    if(!m_sfx)
        return;

    lastChannel = Mix_PlayChannelTimedVolume(ui->playChannel->value(),
                                             m_sfx,
                                             0,
                                             0,
                                             ui->initialVolume->value());
}

void MultiSfxItem::on_fadeIn_clicked()
{
    openSfx();

    if(!m_sfx)
        return;

    lastChannel = Mix_FadeInChannelTimedVolume(ui->playChannel->value(),
                                               m_sfx,
                                               0,
                                               ui->fadeLen->value(),
                                               0,
                                               ui->initialVolume->value());
}

void MultiSfxItem::on_fadeOut_clicked()
{
    if(lastChannel < 0)
        return;

    Mix_FadeOutChannel(lastChannel, ui->fadeLen->value());
}

void MultiSfxItem::on_stop_clicked()
{
    if(lastChannel < 0)
        return;

    Mix_HaltChannel(lastChannel);
}

void MultiSfxItem::sfxStoppedHook(int channel)
{
    if(channel == lastChannel)
        lastChannel = -1;
}

void MultiSfxItem::closeSfx()
{
    if(!m_sfx)
        return; // Already closed

    if(lastChannel >= 0)
        Mix_HaltChannel(lastChannel);
    lastChannel = -1;

    Mix_FreeChunk(m_sfx);
    m_sfx = nullptr;
}

void MultiSfxItem::openSfx()
{
    if(m_sfx)
        return; // Already open
    qDebug() << "Opening" << m_sfxPath;
    m_sfx = Mix_LoadWAV(m_sfxPath.toUtf8().data());

    if(!m_sfx)
        qWarning() << tr("Can't open SFX: %1").arg(Mix_GetError());
    else
        ui->sfxFileName->setText(QFileInfo(m_sfxPath).fileName());
}

void MultiSfxItem::reOpenSfx()
{
    if(m_sfx)
        closeSfx();
    openSfx();
}
