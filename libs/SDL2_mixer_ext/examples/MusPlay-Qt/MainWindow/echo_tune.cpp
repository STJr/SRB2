#include "echo_tune.h"
#include "ui_echo_tune.h"
#include <QSettings>
#include <QTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#   include <QRegExp>
#else
#   include <QRegularExpression>
#endif
#include <QClipboard>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QtDebug>
#include "../Player/mus_player.h"
#include "../Effects/spc_echo.h"
#include "snes_spc/spc.h"

#include "qfile_dialogs_default_options.hpp"



EchoTune::EchoTune(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EchoTune)
{
    ui->setupUi(this);

    QObject::connect(ui->echo_fir0, &QSlider::valueChanged, [this](int val)->void{
        ui->fir0_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir1, &QSlider::valueChanged, [this](int val)->void{
        ui->fir1_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir2, &QSlider::valueChanged, [this](int val)->void{
        ui->fir2_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir3, &QSlider::valueChanged, [this](int val)->void{
        ui->fir3_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir4, &QSlider::valueChanged, [this](int val)->void{
        ui->fir4_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir5, &QSlider::valueChanged, [this](int val)->void{
        ui->fir5_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir6, &QSlider::valueChanged, [this](int val)->void{
        ui->fir6_val->setText(QString("%1").arg(val));
    });
    QObject::connect(ui->echo_fir7, &QSlider::valueChanged, [this](int val)->void{
        ui->fir7_val->setText(QString("%1").arg(val));
    });
}

EchoTune::~EchoTune()
{
    delete ui;
}

void EchoTune::saveSetup()
{
    QSettings set;

    set.beginGroup("spc-echo");

    set.setValue("eon", ui->echo_eon->isChecked());
    set.setValue("edl", ui->echo_edl->value());
    set.setValue("efb", ui->echo_efb->value());

    set.setValue("mvoll", ui->echo_mvoll->value());
    set.setValue("mvolr", ui->echo_mvolr->value());
    set.setValue("evoll", ui->echo_evoll->value());
    set.setValue("evolr", ui->echo_evolr->value());

    set.setValue("fir0", ui->echo_fir0->value());
    set.setValue("fir1", ui->echo_fir1->value());
    set.setValue("fir2", ui->echo_fir2->value());
    set.setValue("fir3", ui->echo_fir3->value());
    set.setValue("fir4", ui->echo_fir4->value());
    set.setValue("fir5", ui->echo_fir5->value());
    set.setValue("fir6", ui->echo_fir6->value());
    set.setValue("fir7", ui->echo_fir7->value());
    set.endGroup();

    set.sync();
}

void EchoTune::loadSetup()
{
    QSettings set;

    set.beginGroup("spc-echo");

    ui->echo_eon->blockSignals(true);
    ui->echo_eon->setChecked(set.value("eon", PGE_MusicPlayer::echoGetReg(ECHO_EON) != 0).toBool());
    ui->echo_eon->blockSignals(false);

    ui->echo_edl->blockSignals(true);
    ui->echo_edl->setValue(set.value("edl", PGE_MusicPlayer::echoGetReg(ECHO_EDL)).toInt());
    ui->echo_edl->blockSignals(false);

    ui->echo_efb->blockSignals(true);
    ui->echo_efb->setValue(set.value("efb", PGE_MusicPlayer::echoGetReg(ECHO_EFB)).toInt());
    ui->echo_efb->blockSignals(false);

    ui->echo_mvoll->blockSignals(true);
    ui->echo_mvoll->setValue(set.value("mvoll", PGE_MusicPlayer::echoGetReg(ECHO_MVOLL)).toInt());
    ui->echo_mvoll->blockSignals(false);

    ui->echo_mvolr->blockSignals(true);
    ui->echo_mvolr->setValue(set.value("mvolr", PGE_MusicPlayer::echoGetReg(ECHO_MVOLR)).toInt());
    ui->echo_mvolr->blockSignals(false);

    ui->echo_evoll->blockSignals(true);
    ui->echo_evoll->setValue(set.value("evoll", PGE_MusicPlayer::echoGetReg(ECHO_EVOLL)).toInt());
    ui->echo_evoll->blockSignals(false);

    ui->echo_evolr->blockSignals(true);
    ui->echo_evolr->setValue(set.value("evolr", PGE_MusicPlayer::echoGetReg(ECHO_EVOLR)).toInt());
    ui->echo_evolr->blockSignals(false);

    ui->echo_fir0->setValue(set.value("fir0", PGE_MusicPlayer::echoGetReg(ECHO_FIR0)).toInt());
    ui->echo_fir1->setValue(set.value("fir1", PGE_MusicPlayer::echoGetReg(ECHO_FIR1)).toInt());
    ui->echo_fir2->setValue(set.value("fir2", PGE_MusicPlayer::echoGetReg(ECHO_FIR2)).toInt());
    ui->echo_fir3->setValue(set.value("fir3", PGE_MusicPlayer::echoGetReg(ECHO_FIR3)).toInt());
    ui->echo_fir4->setValue(set.value("fir4", PGE_MusicPlayer::echoGetReg(ECHO_FIR4)).toInt());
    ui->echo_fir5->setValue(set.value("fir5", PGE_MusicPlayer::echoGetReg(ECHO_FIR5)).toInt());
    ui->echo_fir6->setValue(set.value("fir6", PGE_MusicPlayer::echoGetReg(ECHO_FIR6)).toInt());
    ui->echo_fir7->setValue(set.value("fir7", PGE_MusicPlayer::echoGetReg(ECHO_FIR7)).toInt());

    set.endGroup();
}

void EchoTune::sendAll()
{
    PGE_MusicPlayer::echoSetReg(ECHO_EON, ui->echo_eon->isChecked() ? 1 : 0);
    PGE_MusicPlayer::echoSetReg(ECHO_EDL, ui->echo_edl->value());
    PGE_MusicPlayer::echoSetReg(ECHO_EFB, ui->echo_efb->value());
    PGE_MusicPlayer::echoSetReg(ECHO_MVOLL, ui->echo_mvoll->value());
    PGE_MusicPlayer::echoSetReg(ECHO_MVOLR, ui->echo_mvolr->value());
    PGE_MusicPlayer::echoSetReg(ECHO_EVOLL, ui->echo_evoll->value());
    PGE_MusicPlayer::echoSetReg(ECHO_EVOLR, ui->echo_evolr->value());

    PGE_MusicPlayer::echoSetReg(ECHO_FIR0, ui->echo_fir0->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR1, ui->echo_fir1->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR2, ui->echo_fir2->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR3, ui->echo_fir3->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR4, ui->echo_fir4->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR5, ui->echo_fir5->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR6, ui->echo_fir6->value());
    PGE_MusicPlayer::echoSetReg(ECHO_FIR7, ui->echo_fir7->value());
}

static void state_save(unsigned char** out, void* in, size_t size)
{
    memcpy(*out, in, size);
    *out += size;
}

void EchoTune::loadSpcFile(const QString &file)
{
    SNES_SPC* snes_spc;
    unsigned char state[spc_state_size];
    unsigned char* out = state;

    QFile in(file);

    if(!in.open(QIODevice::ReadOnly))
    {
        qWarning() << "Can't open file" << file << "Because of error:" << in.errorString();
        return;
    }

    if(in.size() > 70000)
    {
        qWarning() << "File" << file << "is too big";
        return; // Too fat file
    }

    QByteArray inData = in.readAll();

    snes_spc = spc_new();
    if(!snes_spc)
    {
        qWarning() << "Out of memory";
        return; // Out of memory
    }

    spc_err_t err = spc_load_spc(snes_spc, inData.data(), inData.size());
    if(err) // whoops
    {
        qWarning() << "Failed to load SPC file" << file << ":" << err;
        spc_delete(snes_spc);
        return;
    }

    spc_clear_echo(snes_spc);

    spc_skip(snes_spc, 10240 * 2);

    spc_copy_state(snes_spc, &out, state_save);

    // Global registers
    enum {
        r_mvoll = 0x0C, r_mvolr = 0x1C,
        r_evoll = 0x2C, r_evolr = 0x3C,
        r_kon   = 0x4C, r_koff  = 0x5C,
        r_flg   = 0x6C, r_endx  = 0x7C,
        r_efb   = 0x0D, r_pmon  = 0x2D,
        r_non   = 0x3D, r_eon   = 0x4D,
        r_dir   = 0x5D, r_esa   = 0x6D,
        r_edl   = 0x7D,
        r_fir   = 0x0F // 8 coefficients at 0x0F, 0x1F ... 0x7F
    };

    uint8_t *dspReg = state + 65568;
    PGE_MusicPlayer::echoSetReg(ECHO_EON, dspReg[r_eon] != 0 ? 1 : 0);
    PGE_MusicPlayer::echoSetReg(ECHO_EDL, (uint8_t)dspReg[r_edl]);
    PGE_MusicPlayer::echoSetReg(ECHO_EFB, (int8_t)dspReg[r_efb]);

    PGE_MusicPlayer::echoSetReg(ECHO_MVOLL, (int8_t)dspReg[r_mvoll]);
    PGE_MusicPlayer::echoSetReg(ECHO_MVOLR, (int8_t)dspReg[r_mvolr]);
    PGE_MusicPlayer::echoSetReg(ECHO_EVOLL, (int8_t)dspReg[r_evoll]);
    PGE_MusicPlayer::echoSetReg(ECHO_EVOLR, (int8_t)dspReg[r_evolr]);

    PGE_MusicPlayer::echoSetReg(ECHO_FIR0, (int8_t)dspReg[r_fir]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR1, (int8_t)dspReg[r_fir + 0x10]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR2, (int8_t)dspReg[r_fir + 0x20]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR3, (int8_t)dspReg[r_fir + 0x30]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR4, (int8_t)dspReg[r_fir + 0x40]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR5, (int8_t)dspReg[r_fir + 0x50]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR6, (int8_t)dspReg[r_fir + 0x60]);
    PGE_MusicPlayer::echoSetReg(ECHO_FIR7, (int8_t)dspReg[r_fir + 0x70]);

    spc_delete(snes_spc);

    on_echo_reload_clicked();
}

void EchoTune::dropEvent(QDropEvent *e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    for(const QUrl& url : e->mimeData()->urls())
    {
        const QString& fileName = url.toLocalFile();
        loadSpcFile(fileName);
    }
}

void EchoTune::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void EchoTune::on_save_clicked()
{
    saveSetup();
}

void EchoTune::on_sendAll_clicked()
{
    sendAll();
}

void EchoTune::on_echo_reload_clicked()
{
    ui->echo_eon->blockSignals(true);
    ui->echo_eon->setChecked(PGE_MusicPlayer::echoGetReg(ECHO_EON) != 0);
    ui->echo_eon->blockSignals(false);

    ui->echo_edl->blockSignals(true);
    ui->echo_edl->setValue(PGE_MusicPlayer::echoGetReg(ECHO_EDL));
    ui->echo_edl->blockSignals(false);

    ui->echo_efb->blockSignals(true);
    ui->echo_efb->setValue(PGE_MusicPlayer::echoGetReg(ECHO_EFB));
    ui->echo_efb->blockSignals(false);

    ui->echo_mvoll->blockSignals(true);
    ui->echo_mvoll->setValue(PGE_MusicPlayer::echoGetReg(ECHO_MVOLL));
    ui->echo_mvoll->blockSignals(false);

    ui->echo_mvolr->blockSignals(true);
    ui->echo_mvolr->setValue(PGE_MusicPlayer::echoGetReg(ECHO_MVOLR));
    ui->echo_mvolr->blockSignals(false);

    ui->echo_evoll->blockSignals(true);
    ui->echo_evoll->setValue(PGE_MusicPlayer::echoGetReg(ECHO_EVOLL));
    ui->echo_evoll->blockSignals(false);

    ui->echo_evolr->blockSignals(true);
    ui->echo_evolr->setValue(PGE_MusicPlayer::echoGetReg(ECHO_EVOLR));
    ui->echo_evolr->blockSignals(false);

    ui->echo_fir0->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR0));
    ui->echo_fir1->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR1));
    ui->echo_fir2->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR2));
    ui->echo_fir3->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR3));
    ui->echo_fir4->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR4));
    ui->echo_fir5->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR5));
    ui->echo_fir6->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR6));
    ui->echo_fir7->setValue(PGE_MusicPlayer::echoGetReg(ECHO_FIR7));
}

void EchoTune::on_reset_clicked()
{
    PGE_MusicPlayer::echoResetDefaults();
    on_echo_reload_clicked();
}

void EchoTune::on_echo_eon_clicked(bool checked)
{
    PGE_MusicPlayer::echoSetReg(ECHO_EON, checked ? 1 : 0);
}

void EchoTune::on_echo_edl_valueChanged(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_EDL, arg1);
}

void EchoTune::on_echo_efb_valueChanged(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_EFB, arg1);
}

void EchoTune::on_echo_mvoll_valueChanged(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_MVOLL, arg1);
}

void EchoTune::on_echo_mvolr_valueChanged(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_MVOLR, arg1);
}

void EchoTune::on_echo_evoll_valueChanged(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_EVOLL, arg1);
}

void EchoTune::on_echo_evolr_valueChanged(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_EVOLR, arg1);
}

void EchoTune::on_echo_fir0_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR0, arg1);
}

void EchoTune::on_echo_fir1_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR1, arg1);
}

void EchoTune::on_echo_fir2_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR2, arg1);
}

void EchoTune::on_echo_fir3_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR3, arg1);
}

void EchoTune::on_echo_fir4_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR4, arg1);
}

void EchoTune::on_echo_fir5_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR5, arg1);
}

void EchoTune::on_echo_fir6_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR6, arg1);
}

void EchoTune::on_echo_fir7_sliderMoved(int arg1)
{
    PGE_MusicPlayer::echoSetReg(ECHO_FIR7, arg1);
}

void EchoTune::on_resetFir_clicked()
{
    PGE_MusicPlayer::echoResetFir();
    on_echo_reload_clicked();
}

void EchoTune::on_copySetup_clicked()
{
    QString out;
    out += "fx = echo\n";
    out += QString("echo-on = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_EON));
    out += QString("delay = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_EDL));
    out += QString("feedback = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_EFB));
    out += QString("main-volume-left = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_MVOLL));
    out += QString("main-volume-right = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_MVOLR));
    out += QString("echo-volume-left = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_EVOLL));
    out += QString("echo-volume-right = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_EVOLR));
    out += QString("fir-0 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR0));
    out += QString("fir-1 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR1));
    out += QString("fir-2 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR2));
    out += QString("fir-3 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR3));
    out += QString("fir-4 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR4));
    out += QString("fir-5 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR5));
    out += QString("fir-6 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR6));
    out += QString("fir-7 = %1\n").arg(PGE_MusicPlayer::echoGetReg(ECHO_FIR7));

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

static void textToReg(const QString &text, const QString &expr, int reg)
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

void EchoTune::on_pasteSetup_clicked()
{
    const QString text = QApplication::clipboard()->text();
    if(text.isEmpty())
        return; // Nothing!

    textToReg(text, "echo-on *= *(-?\\d+)", ECHO_EON);
    textToReg(text, "delay *= *(-?\\d+)", ECHO_EDL);
    textToReg(text, "feedback *= *(-?\\d+)", ECHO_EFB);
    textToReg(text, "main-volume-left *= *(-?\\d+)", ECHO_MVOLL);
    textToReg(text, "main-volume-right *= *(-?\\d+)", ECHO_MVOLR);
    textToReg(text, "echo-volume-left *= *(-?\\d+)", ECHO_EVOLL);
    textToReg(text, "echo-volume-right *= *(-?\\d+)", ECHO_EVOLR);
    textToReg(text, "fir-0 *= *(-?\\d+)", ECHO_FIR0);
    textToReg(text, "fir-1 *= *(-?\\d+)", ECHO_FIR1);
    textToReg(text, "fir-2 *= *(-?\\d+)", ECHO_FIR2);
    textToReg(text, "fir-3 *= *(-?\\d+)", ECHO_FIR3);
    textToReg(text, "fir-4 *= *(-?\\d+)", ECHO_FIR4);
    textToReg(text, "fir-5 *= *(-?\\d+)", ECHO_FIR5);
    textToReg(text, "fir-6 *= *(-?\\d+)", ECHO_FIR6);
    textToReg(text, "fir-7 *= *(-?\\d+)", ECHO_FIR7);

    // Reload all values
    on_echo_reload_clicked();
}

void EchoTune::on_readFromSPC_clicked()
{
    QString in = QFileDialog::getOpenFileName(this,
                                              tr("Import echo setup from SPC file"),
                                              QString(), "SPC files (*.spc);;All files (*.*)",
                                              nullptr, c_fileDialogOptions);
    if(!in.isEmpty() && QFile::exists(in))
        loadSpcFile(in);
}

