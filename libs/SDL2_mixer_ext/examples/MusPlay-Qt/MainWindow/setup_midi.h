#ifndef SETUP_MIDI_H
#define SETUP_MIDI_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class SetupMidi;
}

class SetupMidi : public QDialog
{
    Q_OBJECT

public:
    explicit SetupMidi(QWidget *parent = nullptr);
    ~SetupMidi();

    void loadSetup();
    void saveSetup();

    void sendSetup();

    QString getRawMidiArgs();

protected:
    void changeEvent(QEvent *e);
    void dropEvent(QDropEvent* e);
    void dragEnterEvent(QDragEnterEvent* e);

private slots:
    void on_opnEmulator_currentIndexChanged(int index);
    void on_opnVolumeModel_currentIndexChanged(int index);
    void on_opnChanAlloc_currentIndexChanged(int index);
    void on_opn_bank_browse_clicked();
    void on_opn_bank_editingFinished();

    void on_adl_bankId_currentIndexChanged(int index);
    void on_adl_bank_browse_clicked();
    void on_adl_bank_editingFinished();

    void on_adlEmulator_currentIndexChanged(int index);
    void on_adlVolumeModel_currentIndexChanged(int index);
    void on_adlChanAlloc_currentIndexChanged(int index);

    void on_adl_tremolo_clicked();
    void on_adl_vibrato_clicked();
    void on_adl_scalableModulation_clicked();

    void on_adl_autoArpeggio_clicked();
    void on_opn_autoArpeggio_clicked();

    void on_opnNumChips_editingFinished();
    void on_adlNumChips_editingFinished();

    void on_timidityCfgPathBrowse_clicked();
    void on_timidityCfgPath_editingFinished();

    void on_fluidSynthSF2PathsBrowse_clicked();
    void on_fluidSynthSF2Paths_editingFinished();

    void on_mididevice_currentIndexChanged(int index);

    void on_resetDefaultADLMIDI_clicked();

    void on_midiRawArgs_editingFinished();

    void on_opn_use_custom_clicked(bool checked);
    void on_adl_use_custom_clicked(bool checked);

signals:
    void songRestartNeeded();

private:
    Ui::SetupMidi *ui;
    void updateWindowLayout();
    void updateAutoArgs();

    void restartForAdl();
    void restartForOpn();
    void restartForTimidity();
    void restartForFluidSynth();
    bool m_setupLock = false;
    int  m_numChipsAdlPrev = 0;
    int  m_numChipsOpnPrev = 0;
};

#endif // SETUP_MIDI_H
