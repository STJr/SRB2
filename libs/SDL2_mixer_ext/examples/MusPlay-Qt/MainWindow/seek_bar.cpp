#include "seek_bar.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

static inline int64_t fromDouble(double in)
{
    return static_cast<int64_t>(in * 1000.0);
}

static inline double toDouble(int64_t in)
{
    return static_cast<double>(in) / 1000.0;
}

SeekBar::SeekBar(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(20);
    setMaximumHeight(20);
}

SeekBar::~SeekBar()
{}

void SeekBar::setLength(double length)
{
    m_length = fromDouble(length);
    adjustPositionAndUpdate();
}

void SeekBar::setPosition(double pos)
{
    int64_t old = m_position;
    m_position = fromDouble(pos);
    adjustPosition();
    if(old != m_position)
        repaint();
}

void SeekBar::setLoopPoints(double start, double end)
{
    m_globalLoopStart = fromDouble(start);
    m_globalLoopEnd = fromDouble(end);
    repaint();
}

void SeekBar::clearLoopPoints()
{
    m_globalLoopStart = -1;
    m_globalLoopEnd = -1;
    update();
    repaint();
}

void SeekBar::mousePressEvent(QMouseEvent * e)
{
    if(e->button() != Qt::LeftButton)
        return;

    m_mouseHeld = true;
    QRect r = rect();
    r.setLeft(r.left() + m_offsetLeft);
    r.setRight(r.right() - m_offsetRight);

    m_position = mapFromWidget(e->pos().x());
    adjustPosition();
    update();
    repaint();

    emit positionSeeked(toDouble(m_position));
}

void SeekBar::mouseMoveEvent(QMouseEvent * e)
{
    if(m_mouseHeld)
    {
        QRect r = rect();
        r.setLeft(r.left() + m_offsetLeft);
        r.setRight(r.right() - m_offsetRight);

        m_position = mapFromWidget(e->pos().x());
        adjustPosition();
        update();
        repaint();

        emit positionSeeked(toDouble(m_position));
    }
}

void SeekBar::mouseReleaseEvent(QMouseEvent * e)
{
    if(m_mouseHeld)
    {
        if(e->button() == Qt::LeftButton)
            m_mouseHeld = false;
    }
}

void SeekBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect r = rect();

    int pos = mapToWidget(m_position);

    r.setLeft(r.left() + m_offsetLeft);
    r.setRight(r.right() - m_offsetRight);
    r.setTop(r.top() + 2);
    r.setBottom(r.bottom() - 2);

    if(!isEnabled())
    {
        painter.setPen(QPen(QBrush(Qt::green), 1));
        painter.setBrush(Qt::transparent);
        painter.drawRect(r);
        return;
    }

    painter.setPen(QPen(QBrush(Qt::yellow), 1));
    painter.setBrush(Qt::transparent);
    painter.drawRect(r);

    if((m_globalLoopStart > 0) && (m_globalLoopEnd > 0))
    {
        int loopStartPoint = mapToWidget(m_globalLoopStart);
        int loopEndPoint = mapToWidget(m_globalLoopEnd);
        painter.setPen(QPen(Qt::darkBlue));
        painter.setBrush(QBrush(Qt::darkBlue));
        painter.drawRect(loopStartPoint,
                         r.top() + 1,
                         loopEndPoint - loopStartPoint,
                         (r.bottom() - (r.top() + 1)));
    }

    painter.setPen(QPen(Qt::red));
    painter.drawLine(pos, r.top() + 1, pos, r.bottom());
}

void SeekBar::adjustPosition()
{
    if(m_position > m_length)
        m_position = m_length;
    if(m_position < 0)
        m_position = 0;
}

void SeekBar::adjustPositionAndUpdate()
{
    int64_t old = m_position;
    adjustPosition();
    if(old != m_position)
    {
        update();
        repaint();
    }
}

int SeekBar::mapToWidget(int64_t in)
{
    int width = rect().width() - (m_offsetLeft + m_offsetRight);
    return int(double(in) / double(m_length) * double(width)) + m_offsetLeft;
}

int64_t SeekBar::mapFromWidget(int in)
{
    int width = rect().width() - (m_offsetLeft + m_offsetRight);
    return int64_t(double(in - m_offsetLeft) / double(width) * double(m_length));
}
