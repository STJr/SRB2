#ifndef TRACK_MUTER_H
#define TRACK_MUTER_H

#include <QDialog>
#include <QVector>

namespace Ui {
class TrackMuter;
}

class QCheckBox;
class FlowLayout;

typedef struct _Mix_Music Mix_Music;

class TrackMuter : public QDialog
{
    Q_OBJECT

    FlowLayout *m_tracksLayout = nullptr;
    QVector<QCheckBox*> m_items;
    Mix_Music *m_dstMusic = nullptr;

public:
    explicit TrackMuter(QWidget *parent = nullptr);
    ~TrackMuter();

    void buildTracksList();

protected:
    void changeEvent(QEvent *e);

public slots:
    void setDstMusic(Mix_Music *mus);
    void setTitle(const QString &tit);

private:
    Ui::TrackMuter *ui;

    Mix_Music *getMus();
};

#endif // TRACK_MUTER_H
