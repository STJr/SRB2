#ifndef SEEK_BAR_H
#define SEEK_BAR_H

#include <QWidget>

class SeekBar : public QWidget
{
    Q_OBJECT
public:
    explicit SeekBar(QWidget *parent = nullptr);
    virtual ~SeekBar();

public slots:
    void setLength(double length);
    void setPosition(double pos);

    void setLoopPoints(double start, double end);
    void clearLoopPoints();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *);

signals:
    void positionSeeked(double pos);

private:
    void adjustPosition();
    void adjustPositionAndUpdate();

    bool m_mouseHeld = false;

    int64_t m_length  = 0;
    int64_t m_position = 0;

    int mapToWidget(int64_t in);
    int64_t mapFromWidget(int in);

    int64_t m_globalLoopStart = -1;
    int64_t m_globalLoopEnd = -1;

    int m_offsetLeft = 5;
    int m_offsetRight = 5;
};

#endif // SEEK_BAR_H
