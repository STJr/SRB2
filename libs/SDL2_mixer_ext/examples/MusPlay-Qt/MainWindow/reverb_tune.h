#ifndef REVERB_TUNE_H
#define REVERB_TUNE_H

#include <QDialog>

namespace Ui {
class ReverbTune;
}

class ReverbTune : public QDialog
{
    Q_OBJECT

public:
    explicit ReverbTune(QWidget *parent = nullptr);
    ~ReverbTune();

    void saveSetup();
    void loadSetup();
    void sendAll();

public slots:
    void on_reverb_reload_clicked();

private slots:
    void on_reset_clicked();
    void on_sendAll_clicked();
    void on_save_clicked();
    void on_copySetup_clicked();
    void on_pasteSetup_clicked();

    void on_revMode_currentIndexChanged(int index);
    void on_revRoomSize_valueChanged(double arg1);
    void on_revDamping_valueChanged(double arg1);
    void on_revWet_valueChanged(double arg1);
    void on_revDry_valueChanged(double arg1);
    void on_revWidth_valueChanged(double arg1);

private:
    Ui::ReverbTune *ui;
};

#endif // REVERB_TUNE_H
