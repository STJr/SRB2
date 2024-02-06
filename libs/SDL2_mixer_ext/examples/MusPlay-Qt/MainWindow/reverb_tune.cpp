#include "reverb_tune.h"
#include "ui_reverb_tune.h"
#include <QSettings>
#include <QTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#   include <QRegExp>
#else
#   include <QRegularExpression>
#endif
#include <QClipboard>
#include "../Player/mus_player.h"
#include "../Effects/reverb.h"

ReverbTune::ReverbTune(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReverbTune)
{
    ui->setupUi(this);
}

ReverbTune::~ReverbTune()
{
    delete ui;
}

void ReverbTune::saveSetup()
{
    QSettings set;

    set.beginGroup("reverb");

    set.setValue("mode", ui->revMode->currentIndex());
    set.setValue("roomsize", ui->revRoomSize->value());
    set.setValue("damping", ui->revDamping->value());
    set.setValue("wet", ui->revWet->value());
    set.setValue("dry", ui->revDry->value());
    set.setValue("width", ui->revWidth->value());

    set.endGroup();

    set.sync();
}

void ReverbTune::loadSetup()
{
    QSettings set;
    ReverbSetup def;

    set.beginGroup("reverb");

    ui->revMode->blockSignals(true);
    ui->revMode->setCurrentIndex(set.value("mode", def.mode > 0.5f ? 1 : 0).toInt());
    ui->revMode->blockSignals(false);

    ui->revRoomSize->blockSignals(true);
    ui->revRoomSize->setValue(set.value("roomsize", def.roomSize).toDouble());
    ui->revRoomSize->blockSignals(false);

    ui->revDamping->blockSignals(true);
    ui->revDamping->setValue(set.value("damping", def.damping).toDouble());
    ui->revDamping->blockSignals(false);

    ui->revWet->blockSignals(true);
    ui->revWet->setValue(set.value("wet", def.wetLevel).toDouble());
    ui->revWet->blockSignals(false);

    ui->revDry->blockSignals(true);
    ui->revDry->setValue(set.value("dry", def.dryLevel).toDouble());
    ui->revDry->blockSignals(false);

    ui->revWidth->blockSignals(true);
    ui->revWidth->setValue(set.value("width", def.width).toDouble());
    ui->revWidth->blockSignals(false);

    set.endGroup();
}

void ReverbTune::sendAll()
{
    ReverbSetup set;
    set.mode = ui->revMode->currentIndex();
    set.roomSize = (float)ui->revRoomSize->value();
    set.damping = (float)ui->revDamping->value();
    set.wetLevel = (float)ui->revWet->value();
    set.dryLevel = (float)ui->revDry->value();
    set.width = (float)ui->revWidth->value();
    PGE_MusicPlayer::reverbUpdateSetup(&set);
}

void ReverbTune::on_reverb_reload_clicked()
{
    ReverbSetup set;
    PGE_MusicPlayer::reverbGetSetup(&set);

    ui->revMode->blockSignals(true);
    ui->revMode->setCurrentIndex(set.mode > 0.5f ? 1 : 0);
    ui->revMode->blockSignals(false);

    ui->revRoomSize->blockSignals(true);
    ui->revRoomSize->setValue(set.roomSize);
    ui->revRoomSize->blockSignals(false);

    ui->revDamping->blockSignals(true);
    ui->revDamping->setValue(set.damping);
    ui->revDamping->blockSignals(false);

    ui->revWet->blockSignals(true);
    ui->revWet->setValue(set.wetLevel);
    ui->revWet->blockSignals(false);

    ui->revDry->blockSignals(true);
    ui->revDry->setValue(set.dryLevel);
    ui->revDry->blockSignals(false);

    ui->revWidth->blockSignals(true);
    ui->revWidth->setValue(set.width);
    ui->revWidth->blockSignals(false);
}

void ReverbTune::on_reset_clicked()
{
    ReverbSetup set;
    PGE_MusicPlayer::reverbUpdateSetup(&set);
    on_reverb_reload_clicked();
}

void ReverbTune::on_sendAll_clicked()
{
    sendAll();
}

void ReverbTune::on_save_clicked()
{
    saveSetup();
}

void ReverbTune::on_copySetup_clicked()
{
    ReverbSetup set;
    PGE_MusicPlayer::reverbGetSetup(&set);

    QString out;
    out += "fx = reverb\n";
    out += QString("mode = %1\n").arg(set.mode);
    out += QString("room-size = %1\n").arg(set.roomSize);
    out += QString("damping = %1\n").arg(set.damping);
    out += QString("wet-level = %1\n").arg(set.wetLevel);
    out += QString("dry-level = %1\n").arg(set.dryLevel);
    out += QString("width = %1\n").arg(set.width);

    QApplication::clipboard()->setText(out);

    QString oldT = ui->copySetup->text();
    QString newT = tr("Copied!");
    ui->copySetup->setText(newT);
    if(oldT != newT)
    {
        auto *c = ui->copySetup;
        QTimer::singleShot(1000, this, [c, oldT]()->void
        {
            c->setText(oldT);
        });
    }
}

static void textToReg(const QString &text, const QString &expr, float &reg)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QRegExp regex(expr);
    if(regex.indexIn(text) >= 0)
        PGE_MusicPlayer::echoSetReg(reg, regex.cap(1).toInt());
#else
    QRegularExpression regex(expr);
    QRegularExpressionMatch m = regex.match(text);
    if(m.capturedStart(1) >= 0)
        PGE_MusicPlayer::echoSetReg(reg, m.captured(1).toInt());
#endif
}

void ReverbTune::on_pasteSetup_clicked()
{
    const QString text = QApplication::clipboard()->text();
    if(text.isEmpty())
        return; // Nothing!

    ReverbSetup set;

    textToReg(text, "mode *= *(-?\\d+\\.?\\d*)", set.mode);
    textToReg(text, "room-size *= *(-?\\d+\\.?\\d*)", set.roomSize);
    textToReg(text, "damping *= *(-?\\d+\\.?\\d*)", set.damping);
    textToReg(text, "wet-level *= *(-?\\d+\\.?\\d*)", set.wetLevel);
    textToReg(text, "dry-level *= *(-?\\d+\\.?\\d*)", set.dryLevel);
    textToReg(text, "width *= *(-?\\d+\\.?\\d*)", set.width);

    PGE_MusicPlayer::reverbUpdateSetup(&set);

    // Reload all values
    on_reverb_reload_clicked();
}

void ReverbTune::on_revMode_currentIndexChanged(int index)
{
    PGE_MusicPlayer::reverbSetMode(index);
}

void ReverbTune::on_revRoomSize_valueChanged(double arg1)
{
    PGE_MusicPlayer::reverbSetRoomSize((float)arg1);
}

void ReverbTune::on_revDamping_valueChanged(double arg1)
{
    PGE_MusicPlayer::reverbSetDamping((float)arg1);
}

void ReverbTune::on_revWet_valueChanged(double arg1)
{
    PGE_MusicPlayer::reverbSetWet((float)arg1);
}

void ReverbTune::on_revDry_valueChanged(double arg1)
{
    PGE_MusicPlayer::reverbSetDry((float)arg1);
}

void ReverbTune::on_revWidth_valueChanged(double arg1)
{
    PGE_MusicPlayer::reverbSetWidth((float)arg1);
}
