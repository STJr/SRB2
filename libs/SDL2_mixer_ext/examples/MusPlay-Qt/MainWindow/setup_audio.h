#ifndef SETUP_AUDIO_H
#define SETUP_AUDIO_H

#include <QDialog>

namespace Ui {
class SetupAudio;
}

class SetupAudio : public QDialog
{
    Q_OBJECT

public:
    explicit SetupAudio(QWidget *parent = nullptr);
    ~SetupAudio();

private slots:
    void on_restoreDefaults_clicked();

private slots:
    void on_confirm_accepted();

private:
    Ui::SetupAudio *ui;
};

#endif // SETUP_AUDIO_H
