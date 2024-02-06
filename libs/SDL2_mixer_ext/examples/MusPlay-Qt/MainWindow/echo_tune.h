#ifndef ECHO_TUNE_H
#define ECHO_TUNE_H

#include <QDialog>

namespace Ui {
class EchoTune;
}

class EchoTune : public QDialog
{
    Q_OBJECT

public:
    explicit EchoTune(QWidget *parent = nullptr);
    ~EchoTune();

    void saveSetup();
    void loadSetup();
    void sendAll();

    void loadSpcFile(const QString &file);

protected:
    void dropEvent(QDropEvent* e);
    void dragEnterEvent(QDragEnterEvent* e);

public slots:
    void on_echo_reload_clicked();

private slots:
    void on_sendAll_clicked();
    void on_save_clicked();
    void on_reset_clicked();

    void on_echo_eon_clicked(bool checked);
    void on_echo_edl_valueChanged(int arg1);
    void on_echo_efb_valueChanged(int arg1);
    void on_echo_mvoll_valueChanged(int arg1);
    void on_echo_mvolr_valueChanged(int arg1);
    void on_echo_evoll_valueChanged(int arg1);
    void on_echo_evolr_valueChanged(int arg1);
    void on_echo_fir0_sliderMoved(int arg1);
    void on_echo_fir1_sliderMoved(int arg1);
    void on_echo_fir2_sliderMoved(int arg1);
    void on_echo_fir3_sliderMoved(int arg1);
    void on_echo_fir4_sliderMoved(int arg1);
    void on_echo_fir5_sliderMoved(int arg1);
    void on_echo_fir6_sliderMoved(int arg1);
    void on_echo_fir7_sliderMoved(int arg1);
    void on_resetFir_clicked();

    void on_pasteSetup_clicked();
    void on_copySetup_clicked();

    void on_readFromSPC_clicked();

private:
    Ui::EchoTune *ui;
};

#endif // ECHO_TUNE_H
