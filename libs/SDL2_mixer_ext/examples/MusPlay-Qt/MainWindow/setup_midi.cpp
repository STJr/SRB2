#include "setup_midi.h"
#include "ui_setup_midi.h"
#include "../Player/mus_player.h"

#include "qfile_dialogs_default_options.hpp"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QSettings>


static int tristateToInt(Qt::CheckState state)
{
    switch(state)
    {
    case Qt::Checked:
        return 1;
    case Qt::Unchecked:
        return 0;
    case Qt::PartiallyChecked:
        return -1;
    }
    return Qt::Unchecked;
}

SetupMidi::SetupMidi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetupMidi)
{
    ui->setupUi(this);

    layout()->activate();
    adjustSize();

    ui->adl_bankId->clear();

#ifdef SDL_MIXER_X
    int totalBakns = Mix_ADLMIDI_getTotalBanks();
    const char *const *names = Mix_ADLMIDI_getBankNames();

    for(int i = 0; i < totalBakns; i++)
        ui->adl_bankId->addItem(QString("%1 = %2").arg(i).arg(names[i]));
#endif

    m_numChipsAdlPrev = ui->adlNumChips->value();
    m_numChipsOpnPrev = ui->opnNumChips->value();

    ui->adl_tremolo->setCheckState(Qt::PartiallyChecked);
    ui->adl_vibrato->setCheckState(Qt::PartiallyChecked);
    ui->adl_scalableModulation->setCheckState(Qt::PartiallyChecked);
}

SetupMidi::~SetupMidi()
{
    delete ui;
}

void SetupMidi::loadSetup()
{
#ifdef SDL_MIXER_X
    QSettings setup;

    ui->mididevice->setCurrentIndex(setup.value("MIDI-Device", 0).toInt());
    ui->adl_bankId->setCurrentIndex(setup.value("ADLMIDI-Bank-ID", 58).toInt());
    ui->adlVolumeModel->setCurrentIndex(setup.value("ADLMIDI-VolumeModel", 0).toInt());
    ui->adlChanAlloc->setCurrentIndex(setup.value("ADLMIDI-ChanAlloc", 0).toInt());
    ui->adlEmulator->setCurrentIndex(setup.value("ADLMIDI-Emulator", 0).toInt());
    ui->adlNumChips->setValue(setup.value("ADLMIDI-NumChips", 4).toInt());
    ui->adl_tremolo->setCheckState((Qt::CheckState)setup.value("ADLMIDI-Tremolo", Qt::PartiallyChecked).toInt());
    ui->adl_vibrato->setCheckState((Qt::CheckState)setup.value("ADLMIDI-Vibrato", Qt::PartiallyChecked).toInt());
    ui->adl_scalableModulation->setCheckState((Qt::CheckState)setup.value("ADLMIDI-Scalable-Modulation", Qt::Unchecked).toInt());
    ui->adl_autoArpeggio->setCheckState((Qt::CheckState)setup.value("ADLMIDI-AutoArpeggio", Qt::Checked).toInt());
    ui->adl_bank->setText(setup.value("ADLMIDI-Bank", QString()).toString());
    ui->adl_use_custom->setChecked(setup.value("ADLMIDI-Bank-UseCustom", true).toBool());

    ui->opnEmulator->setCurrentIndex(setup.value("OPNMIDI-Emulator", 0).toInt());
    ui->opnNumChips->setValue(setup.value("OPNMIDI-NumChips", 8).toInt());
    ui->opnVolumeModel->setCurrentIndex(setup.value("OPNMIDI-VolumeModel", 0).toInt());
    ui->opnChanAlloc->setCurrentIndex(setup.value("OPNMIDI-ChanAlloc", 0).toInt());
    ui->opn_autoArpeggio->setCheckState((Qt::CheckState)setup.value("OPNMIDI-AutoArpeggio", Qt::Checked).toInt());
    ui->opn_bank->setText(setup.value("OPNMIDI-Bank", QString()).toString());
    ui->opn_use_custom->setChecked(setup.value("OPNMIDI-Bank-UseCustom", true).toBool());
    ui->timidityCfgPath->setText(setup.value("Timidity-Config-Path", QString()).toString());
    ui->fluidSynthSF2Paths->setText(setup.value("FluidSynth-SoundFonts", QString()).toString());

    m_numChipsAdlPrev = ui->adlNumChips->value();
    m_numChipsOpnPrev = ui->opnNumChips->value();

    sendSetup();
#endif

    updateWindowLayout();
}

void SetupMidi::saveSetup()
{
#ifdef SDL_MIXER_X
    QSettings setup;

    setup.setValue("MIDI-Device", ui->mididevice->currentIndex());

    setup.setValue("ADLMIDI-Bank-ID", ui->adl_bankId->currentIndex());
    setup.setValue("ADLMIDI-Emulator", ui->adlEmulator->currentIndex());
    setup.setValue("ADLMIDI-NumChips", ui->adlNumChips->value());
    setup.setValue("ADLMIDI-VolumeModel", ui->adlVolumeModel->currentIndex());
    setup.setValue("ADLMIDI-ChanAlloc", ui->adlChanAlloc->currentIndex());
    setup.setValue("ADLMIDI-Tremolo", ui->adl_tremolo->checkState());
    setup.setValue("ADLMIDI-Vibrato", ui->adl_vibrato->checkState());
    setup.setValue("ADLMIDI-Scalable-Modulation", ui->adl_scalableModulation->checkState());
    setup.setValue("ADLMIDI-AutoArpeggio", ui->adl_autoArpeggio->checkState());

    setup.setValue("ADLMIDI-Bank", ui->adl_bank->text());
    setup.setValue("ADLMIDI-Bank-UseCustom", ui->adl_use_custom->isChecked());

    setup.setValue("OPNMIDI-Emulator", ui->opnEmulator->currentIndex());
    setup.setValue("OPNMIDI-NumChips", ui->opnNumChips->value());
    setup.setValue("OPNMIDI-VolumeModel", ui->opnVolumeModel->currentIndex());
    setup.setValue("OPNMIDI-ChanAlloc", ui->opnChanAlloc->currentIndex());
    setup.setValue("OPNMIDI-AutoArpeggio", ui->opn_autoArpeggio->checkState());

    setup.setValue("OPNMIDI-Bank", ui->opn_bank->text());
    setup.setValue("OPNMIDI-Bank-UseCustom", ui->opn_use_custom->isChecked());

    setup.setValue("Timidity-Config-Path", ui->timidityCfgPath->text());
    setup.setValue("FluidSynth-SoundFonts", ui->fluidSynthSF2Paths->text());

    setup.sync();
#endif
}

void SetupMidi::sendSetup()
{
#ifdef SDL_MIXER_X
    on_mididevice_currentIndexChanged(ui->mididevice->currentIndex());
    Mix_ADLMIDI_setBankID(ui->adl_bankId->currentIndex());
    Mix_ADLMIDI_setVolumeModel(ui->adlVolumeModel->currentIndex());
    Mix_ADLMIDI_setChannelAllocMode(ui->adlChanAlloc->currentIndex() - 1);
    Mix_ADLMIDI_setEmulator(ui->adlEmulator->currentIndex());
    Mix_ADLMIDI_setChipsCount(ui->adlNumChips->value());
    Mix_ADLMIDI_setTremolo(tristateToInt(ui->adl_tremolo->checkState()));
    Mix_ADLMIDI_setVibrato(tristateToInt(ui->adl_vibrato->checkState()));
    Mix_ADLMIDI_setScaleMod(tristateToInt(ui->adl_scalableModulation->checkState()));
    Mix_ADLMIDI_setAutoArpeggio(tristateToInt(ui->adl_autoArpeggio->checkState()));

    ui->adl_bank->setModified(true);
    on_adl_bank_editingFinished();

    Mix_OPNMIDI_setEmulator(ui->opnEmulator->currentIndex());
    Mix_OPNMIDI_setChipsCount(ui->opnNumChips->value());
    Mix_OPNMIDI_setVolumeModel(ui->opnVolumeModel->currentIndex());
    Mix_OPNMIDI_setChannelAllocMode(ui->opnChanAlloc->currentIndex() - 1);
    Mix_OPNMIDI_setAutoArpeggio(tristateToInt(ui->opn_autoArpeggio->checkState()));

    ui->opn_bank->setModified(true);
    on_opn_bank_editingFinished();

    ui->timidityCfgPath->setModified(true);
    on_timidityCfgPath_editingFinished();

    ui->fluidSynthSF2Paths->setModified(true);
    on_fluidSynthSF2Paths_editingFinished();
#endif
}

QString SetupMidi::getRawMidiArgs()
{
    return ui->midiRawArgs->text();
}

void SetupMidi::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch(e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SetupMidi::dropEvent(QDropEvent *e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    QStringList sf2List;

    for(const QUrl& url : e->mimeData()->urls())
    {
        const QString& fileName = url.toLocalFile();

        if(fileName.endsWith(".sf2", Qt::CaseInsensitive) || fileName.endsWith(".sf3", Qt::CaseInsensitive))
            sf2List.push_back(fileName);

        else if(fileName.endsWith(".cfg", Qt::CaseInsensitive))
        {
            ui->timidityCfgPath->setText(fileName);
            ui->timidityCfgPath->setModified(true);
            on_timidityCfgPath_editingFinished();
        }

        else if(fileName.endsWith(".wopl", Qt::CaseInsensitive))
        {
            ui->adl_bank->setText(fileName);
            ui->adl_bank->setModified(true);
            on_adl_bank_editingFinished();
        }

        else if(fileName.endsWith(".wopn", Qt::CaseInsensitive))
        {
            ui->opn_bank->setText(fileName);
            ui->opn_bank->setModified(true);
            on_opn_bank_editingFinished();
        }
    }

    if(!sf2List.isEmpty())
    {
        QString path = sf2List.join(';');
        ui->fluidSynthSF2Paths->setText(path);
        ui->fluidSynthSF2Paths->setModified(true);
        on_fluidSynthSF2Paths_editingFinished();
    }
}

void SetupMidi::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

#ifdef SDL_MIXER_X
/**
 * @brief Convert menu ID into actual emulator ID
 * @param index index of menu
 * @return actual index of emulator
 */
static int toOpnEmu(int index)
{
    switch(index)
    {
    case 0:
        index = OPNMIDI_OPN2_EMU_MAME_OPN2;
        break;
    case 1:
        index = OPNMIDI_OPN2_EMU_NUKED_YM3438;
        break;
    case 2:
        index = OPNMIDI_OPN2_EMU_GENS;
        break;
    case 3:
        index = OPNMIDI_OPN2_EMU_YMFM_OPN2;
        break;
    case 4:
        index = OPNMIDI_OPN2_EMU_NP2;
        break;
    case 5:
        index = OPNMIDI_OPN2_EMU_MAME_OPNA;
        break;
    case 6:
        index = OPNMIDI_OPN2_EMU_YMFM_OPNA;
        break;
    case 7:
        index = OPNMIDI_OPN2_EMU_NUKED_YM2612;
        break;
    }
    return index;
}
#endif

void SetupMidi::on_opnEmulator_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_OPNMIDI_setEmulator(toOpnEmu(index));
    restartForOpn();
    updateAutoArgs();
#else
    Q_UNUSED(index);
#endif
}

void SetupMidi::on_opnVolumeModel_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_OPNMIDI_setVolumeModel(index);
    restartForOpn();
    updateAutoArgs();
#else
    Q_UNUSED(index);
#endif
}

void SetupMidi::on_opnChanAlloc_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_OPNMIDI_setChannelAllocMode(index - 1);
    restartForOpn();
    updateAutoArgs();
#else
    Q_UNUSED(index);
#endif
}

void SetupMidi::on_opn_use_custom_clicked(bool checked)
{
    ui->opn_bank->setModified(true);
    on_opn_bank_editingFinished();
    if(!checked)
        restartForOpn();
}

void SetupMidi::on_opn_bank_browse_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
                   tr("Select WOPN bank file"),
                   ui->opn_bank->text(),
                   "OPN bank file by Wohlstand (*.wopn);;"
                   "All Files (*.*)", nullptr,
                   c_fileDialogOptions);
    if(!path.isEmpty())
    {
        ui->opn_bank->setText(path);
        ui->opn_bank->setModified(true);
        on_opn_bank_editingFinished();
    }
}

void SetupMidi::on_opn_bank_editingFinished()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    if(ui->opn_bank->isModified())
    {
        QString file = ui->opn_bank->text();
        if(!file.isEmpty() && QFile::exists(file) && ui->opn_use_custom->isChecked())
        {
            Mix_OPNMIDI_setCustomBankFile(file.toUtf8().data());
            restartForOpn();
        }
        else
        {
            Mix_OPNMIDI_setCustomBankFile(nullptr);
        }
        ui->opn_bank->setModified(false);
    }
    updateAutoArgs();
#endif
}

void SetupMidi::on_adl_bankId_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    Mix_ADLMIDI_setBankID(index);
    if(ui->adl_bank->text().isEmpty() || !ui->adl_use_custom->isChecked())
        restartForAdl();
    updateAutoArgs();
#else
    Q_UNUSED(index);
#endif
}

void SetupMidi::on_adl_use_custom_clicked(bool checked)
{
    ui->adl_bank->setModified(true);
    on_adl_bank_editingFinished();
    if(!checked)
        restartForAdl();
}

void SetupMidi::updateWindowLayout()
{
    ui->adlmidi_extra->setVisible(ui->mididevice->currentIndex() == MIDI_ADLMIDI);
    ui->opnmidi_extra->setVisible(ui->mididevice->currentIndex() == MIDI_OPNMIDI);
    ui->timidity_extra->setVisible(ui->mididevice->currentIndex() == MIDI_Timidity);
    ui->fluidsynth_extra->setVisible(ui->mididevice->currentIndex() == MIDI_Fluidsynth);
    adjustSize();
    update();
}

void SetupMidi::on_adl_bank_browse_clicked()
{
#ifdef SDL_MIXER_X
    QString path = QFileDialog::getOpenFileName(this,
                   tr("Select WOPL bank file"),
                   ui->adl_bank->text(),
                   "OPL3 bank file by Wohlstand (*.wopl);;"
                   "All Files (*.*)", nullptr,
                   c_fileDialogOptions);
    if(!path.isEmpty())
    {
        ui->adl_bank->setText(path);
        ui->adl_bank->setModified(true);
        on_adl_bank_editingFinished();
    }
#endif
}

void SetupMidi::on_adl_bank_editingFinished()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    if(ui->adl_bank->isModified())
    {
        QString file = ui->adl_bank->text();
        if(!file.isEmpty() && QFile::exists(file) && ui->adl_use_custom->isChecked())
        {
            Mix_ADLMIDI_setCustomBankFile(file.toUtf8().data());
            restartForAdl();
        }
        else
            Mix_ADLMIDI_setCustomBankFile(nullptr);

        ui->adl_bank->setModified(false);
        updateAutoArgs();
    }
#endif
}

void SetupMidi::on_adlEmulator_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setEmulator(index);
    restartForAdl();
    updateAutoArgs();
#else
    Q_UNUSED(index);
#endif

}

void SetupMidi::on_adlVolumeModel_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setVolumeModel(index);
    restartForAdl();
    updateAutoArgs();
#else
    Q_UNUSED(index);    
#endif
}

void SetupMidi::on_adlChanAlloc_currentIndexChanged(int index)
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setChannelAllocMode(index - 1);
    restartForAdl();
    updateAutoArgs();
#else
    Q_UNUSED(index);
#endif
}

void SetupMidi::on_adl_tremolo_clicked()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setTremolo(tristateToInt(ui->adl_tremolo->checkState()));
    updateAutoArgs();
#endif
}

void SetupMidi::on_adl_vibrato_clicked()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setVibrato(tristateToInt(ui->adl_vibrato->checkState()));
    updateAutoArgs();
#endif
}

void SetupMidi::on_adl_scalableModulation_clicked()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setScaleMod(tristateToInt(ui->adl_scalableModulation->checkState()));
    updateAutoArgs();
#endif
}

void SetupMidi::on_adlNumChips_editingFinished()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    if(m_numChipsAdlPrev != ui->adlNumChips->value())
    {
        m_numChipsAdlPrev = ui->adlNumChips->value();
        Mix_ADLMIDI_setChipsCount(ui->adlNumChips->value());
        restartForAdl();
        updateAutoArgs();
    }
#endif
}


void SetupMidi::on_opnNumChips_editingFinished()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    if(m_numChipsOpnPrev != ui->opnNumChips->value())
    {
        m_numChipsOpnPrev = ui->opnNumChips->value();
        Mix_OPNMIDI_setChipsCount(ui->opnNumChips->value());
        restartForOpn();
        updateAutoArgs();
    }
#endif
}


void SetupMidi::on_opn_autoArpeggio_clicked()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_OPNMIDI_setAutoArpeggio(tristateToInt(ui->opn_autoArpeggio->checkState()));
    restartForOpn();
    updateAutoArgs();
#endif
}


void SetupMidi::on_adl_autoArpeggio_clicked()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    Mix_ADLMIDI_setAutoArpeggio(tristateToInt(ui->adl_autoArpeggio->checkState()));
    restartForAdl();
    updateAutoArgs();
#endif
}

void SetupMidi::on_timidityCfgPathBrowse_clicked()
{
#ifdef SDL_MIXER_X
    QString path = QFileDialog::getOpenFileName(this,
                   tr("Select Timidity config file"),
                   ui->timidityCfgPath->text(),
                   "Timidity config files (*.cfg);;"
                   "All Files (*.*)", nullptr,
                   c_fileDialogOptions);
    if(!path.isEmpty())
    {
        ui->timidityCfgPath->setText(path);
        ui->timidityCfgPath->setModified(true);
        on_timidityCfgPath_editingFinished();
    }
#endif
}

void SetupMidi::on_timidityCfgPath_editingFinished()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    if(ui->timidityCfgPath->isModified())
    {
        QString file = ui->timidityCfgPath->text();
        if(!file.isEmpty() && QFile::exists(file))
            Mix_SetTimidityCfg(file.toUtf8().data());
        else
            Mix_SetTimidityCfg(nullptr);

        restartForTimidity();
        ui->timidityCfgPath->setModified(false);
    }
#endif
}


void SetupMidi::on_fluidSynthSF2PathsBrowse_clicked()
{
#ifdef SDL_MIXER_X
    QString path = QFileDialog::getOpenFileName(this,
                   tr("Select SoundFont bank for FluidSynth"),
                   ui->fluidSynthSF2Paths->text(),
                   "SoundFont files (*.sf2 *.sf3);;"
                   "All Files (*.*)", nullptr,
                   c_fileDialogOptions);
    if(!path.isEmpty())
    {
        ui->fluidSynthSF2Paths->setText(path);
        ui->fluidSynthSF2Paths->setModified(true);
        on_fluidSynthSF2Paths_editingFinished();
    }
#endif
}

static bool sfExists(QString paths)
{
    QStringList p = paths.split(";");

    for(QString &file : p)
        if(!QFile::exists(file))
            return false;
    return true;
}

void SetupMidi::on_fluidSynthSF2Paths_editingFinished()
{
#ifdef SDL_MIXER_X
    if(m_setupLock)
        return;
    if(ui->fluidSynthSF2Paths->isModified())
    {
        QString files = ui->fluidSynthSF2Paths->text();
        if(!files.isEmpty() && sfExists(files))
            Mix_SetSoundFonts(files.toUtf8().data());
        else
            Mix_SetSoundFonts(QString(qApp->applicationDirPath() + "/gm.sf2").toUtf8().data());

        restartForFluidSynth();
        ui->fluidSynthSF2Paths->setModified(false);
        updateAutoArgs();
    }
#endif
}

void SetupMidi::on_resetDefaultADLMIDI_clicked()
{
#ifdef SDL_MIXER_X
    m_setupLock = true;
    ui->adl_bankId->setCurrentIndex(58);
    ui->adl_tremolo->setCheckState(Qt::PartiallyChecked);
    ui->adl_vibrato->setCheckState(Qt::PartiallyChecked);
    ui->adl_scalableModulation->setCheckState(Qt::PartiallyChecked);
    ui->adlVolumeModel->setCurrentIndex(0);
    ui->adl_autoArpeggio->setChecked(true);
    ui->adlEmulator->setCurrentIndex(0);
    ui->adlNumChips->setValue(4);
    Mix_ADLMIDI_setTremolo(tristateToInt(ui->adl_tremolo->checkState()));
    Mix_ADLMIDI_setVibrato(tristateToInt(ui->adl_vibrato->checkState()));
    Mix_ADLMIDI_setScaleMod(tristateToInt(ui->adl_scalableModulation->checkState()));
    Mix_ADLMIDI_setVolumeModel(ui->adlVolumeModel->currentIndex());
    Mix_ADLMIDI_setBankID(ui->adl_bankId->currentIndex());
    Mix_ADLMIDI_setEmulator(ui->adlEmulator->currentIndex());
    Mix_ADLMIDI_setChipsCount(ui->adlNumChips->value());
    m_setupLock = false;
    updateAutoArgs();
    restartForAdl();
#endif
}

void SetupMidi::updateAutoArgs()
{
    QString args;
    args += QString("s%1;").arg(ui->mididevice->currentIndex());

    switch(ui->mididevice->currentIndex())
    {
    case 0:
        if(ui->adl_bankId->currentIndex() != 58)
            args += QString("b%1;").arg(ui->adl_bankId->currentIndex());
        if(ui->adlNumChips->value() != 4)
            args += QString("c%1;").arg(ui->adlNumChips->value());
        if(ui->adl_tremolo->checkState() != Qt::PartiallyChecked)
            args += QString("t%1;").arg(tristateToInt(ui->adl_tremolo->checkState()));
        if(ui->adl_vibrato->checkState() != Qt::PartiallyChecked)
            args += QString("v%1;").arg(tristateToInt(ui->adl_vibrato->checkState()));
        if(ui->adl_scalableModulation->checkState() != Qt::PartiallyChecked)
            args += QString("m%1;").arg(tristateToInt(ui->adl_scalableModulation->checkState()));
        if(ui->adlVolumeModel->currentIndex() != 0)
            args += QString("l%1;").arg(ui->adlVolumeModel->currentIndex());
        if(ui->adlChanAlloc->currentIndex() != 0)
            args += QString("o%1;").arg(ui->adlChanAlloc->currentIndex() - 1);
        if(ui->adl_autoArpeggio->checkState() == Qt::Checked)
            args += QString("j%1;").arg(tristateToInt(ui->adl_autoArpeggio->checkState()));
        if(ui->adl_use_custom->checkState() == Qt::Checked && !ui->adl_bank->text().isEmpty())
            args += QString("x=%1;").arg(ui->adl_bank->text());
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        if(ui->opnVolumeModel->currentIndex() != 0)
            args += QString("l%1;").arg(ui->opnVolumeModel->currentIndex());
        if(ui->opnNumChips->value() != 8)
            args += QString("c%1;").arg(ui->opnNumChips->value());
        if(ui->opn_autoArpeggio->checkState() == Qt::Checked)
            args += QString("j%1;").arg(tristateToInt(ui->opn_autoArpeggio->checkState()));
        if(ui->opnChanAlloc->currentIndex() != 0)
            args += QString("o%1;").arg(ui->opnChanAlloc->currentIndex() - 1);
        if(ui->opn_use_custom->checkState() == Qt::Checked && !ui->opn_bank->text().isEmpty())
            args += QString("x=%1;").arg(ui->opn_bank->text());
        break;
    case 4:
        if(!ui->fluidSynthSF2Paths->text().isEmpty())
            args += QString("x=%1;").arg(ui->fluidSynthSF2Paths->text());
        break;
    case 5:
        break;
    }
    ui->autoArguments->setText(args);
}

void SetupMidi::on_mididevice_currentIndexChanged(int index)
{
    if(m_setupLock)
        return;

#ifdef SDL_MIXER_GE21
    switch(index)
    {
    default:
    case 0:
        Mix_SetMidiPlayer(MIDI_ADLMIDI);
        break;
    case 1:
        Mix_SetMidiPlayer(MIDI_Native);
        break;
    case 2:
        Mix_SetMidiPlayer(MIDI_Timidity);
        break;
    case 3:
        Mix_SetMidiPlayer(MIDI_OPNMIDI);
        break;
    case 4:
        Mix_SetMidiPlayer(MIDI_Fluidsynth);
        break;
    case 5:
        Mix_SetMidiPlayer(MIDI_EDMIDI);
        break;
    }
#else
    Q_UNUSED(index);
#endif

    updateAutoArgs();
    updateWindowLayout();

    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus) && (PGE_MusicPlayer::type == MUS_MID))
        emit songRestartNeeded();
}

void SetupMidi::on_midiRawArgs_editingFinished()
{
#ifdef SDL_MIXER_X
    if(ui->midiRawArgs->isModified())
    {
        if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus) &&
          (PGE_MusicPlayer::type == MUS_MID ||
           PGE_MusicPlayer::type == MUS_ADLMIDI ||
           PGE_MusicPlayer::type == MUS_GME ||
           PGE_MusicPlayer::type == MUS_PXTONE ||
           PGE_MusicPlayer::type == MUS_OGG))
        {
            emit songRestartNeeded();
        }
        ui->midiRawArgs->setModified(false);
    }
#endif
}

void SetupMidi::restartForAdl()
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus) &&
        (
            (PGE_MusicPlayer::type == MUS_MID && Mix_GetMidiPlayer() == MIDI_ADLMIDI) ||
            (PGE_MusicPlayer::type == MUS_ADLMIDI)
        )
    )
    {
        emit songRestartNeeded();
    }
#endif
}

void SetupMidi::restartForOpn()
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus) &&
      (PGE_MusicPlayer::type == MUS_MID) &&
      (Mix_GetMidiPlayer() == MIDI_OPNMIDI)
    )
    {
        emit songRestartNeeded();
    }
#endif
}

void SetupMidi::restartForTimidity()
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus) &&
      (PGE_MusicPlayer::type == MUS_MID) &&
      (Mix_GetMidiPlayer() == MIDI_Timidity)
    )
    {
        emit songRestartNeeded();
    }
#endif
}

void SetupMidi::restartForFluidSynth()
{
#ifdef SDL_MIXER_X
    if(Mix_PlayingMusicStream(PGE_MusicPlayer::s_playMus) &&
      (PGE_MusicPlayer::type == MUS_MID) &&
      (Mix_GetMidiPlayer() == MIDI_Fluidsynth)
    )
    {
        emit songRestartNeeded();
    }
#endif
}
