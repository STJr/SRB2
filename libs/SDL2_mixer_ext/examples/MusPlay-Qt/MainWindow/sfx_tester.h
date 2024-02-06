#ifndef SFX_TESTER_H
#define SFX_TESTER_H

#include <QDialog>

namespace Ui {
class SfxTester;
}

struct Mix_Chunk;

class SfxTester : public QDialog
{
    Q_OBJECT

public:
    explicit SfxTester(QWidget *parent = nullptr);
    ~SfxTester();

public slots:
    void reloadSfx();

private slots:
    void on_sfx_open_clicked();
    void openSfx(const QString &path);
    void on_sfx_play_clicked();
    void on_sfx_fadeIn_clicked();
    void on_sfx_stop_clicked();
    void on_sfx_fadeout_clicked();
    void on_positionAngle_sliderMoved(int value);
    void on_positionDistance_sliderMoved(int value);
    void on_positionReset_clicked();

    void on_panningLeft_valueChanged(int value);
    void on_panningRight_valueChanged(int value);
    void on_stereoFlip_clicked(bool value);
    void on_panningReset_clicked();

    void updatePositionEffect();
    void updatePanningEffect();
    void updateChannelsFlip();

protected:
    void dropEvent(QDropEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private:
    Ui::SfxTester *ui;

    Mix_Chunk *m_testSfx = nullptr;
    QString    m_testSfxDir;
    QString    m_recentSfxFile;
    bool       m_sendPanning = false;
    int16_t    m_angle = 0;
    uint8_t    m_distance = 0;
    uint8_t    m_panLeft = 255;
    uint8_t    m_panRight = 255;
    uint8_t    m_channelFlip = 0;
};

#endif // SFX_TESTER_H
