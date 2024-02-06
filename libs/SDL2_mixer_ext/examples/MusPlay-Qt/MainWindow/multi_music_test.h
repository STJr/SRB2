#ifndef MULTI_MUSIC_TEST_H
#define MULTI_MUSIC_TEST_H

#include <QDialog>
#include <QVector>

class MultiMusicItem;

namespace Ui {
class MultiMusicTest;
}

class MultiMusicTest : public QDialog
{
    Q_OBJECT

    QVector<MultiMusicItem*> m_items;
    QString m_lastMusic;

public:
    explicit MultiMusicTest(QWidget *parent = nullptr);
    ~MultiMusicTest();

    void openMusicByArg(QString musPath);

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);
    /*!
     * \brief Drop event, triggers when some object was dropped to main window
     * \param e Event parameters
     */
    void dropEvent(QDropEvent *e);

    /*!
     * \brief Drop enter event, triggers when mouse came to window with holden objects
     * \param e Event parameters
     */
    void dragEnterEvent(QDragEnterEvent *e);

private slots:
    void on_generalVolume_sliderMoved(int position);

    void on_stopAll_clicked();
    void on_playAll_clicked();
    void on_open_clicked();
    void wannaClose(MultiMusicItem*item);

private:
    Ui::MultiMusicTest *ui;
};

#endif // MULTI_MUSIC_TEST_H
