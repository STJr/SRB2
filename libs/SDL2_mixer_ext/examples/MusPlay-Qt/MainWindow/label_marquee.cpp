#include "label_marquee.h"
#include <QMouseEvent>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QStyle>

#include <QtDebug>

LabelMarquee::LabelMarquee(QWidget *parent) : QLabel(parent)
{
    init();
}

LabelMarquee::LabelMarquee(QString text, QWidget *parent) : QLabel(parent)
{
    m_text = text;
    init();
}

LabelMarquee::~LabelMarquee()
{}

void LabelMarquee::init()
{
    setMouseTracking(true);
    m_ticker.setInterval(50);
    connect(&m_ticker, &QTimer::timeout, this, &LabelMarquee::processMarquee);
}

bool LabelMarquee::marqueeNeeded()
{
    int width = geometry().width();
    return (m_textWidth > width);
}


void LabelMarquee::enterEvent(QEvent * /*event*/)
{
    if(!m_ticker.isActive() && marqueeNeeded())
        m_ticker.start();
}

void LabelMarquee::leaveEvent(QEvent * /*event*/)
{
    if(m_ticker.isActive())
        m_ticker.stop();
    m_offset = 0;
    repaint();
}

void LabelMarquee::resizeEvent(QResizeEvent * /*event*/)
{
    if(!marqueeNeeded())
        m_offset = 0;
    repaint();
}

void LabelMarquee::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    QRect r = rect();
    // FIXME: Fix the chopping of the right side of the string while drawing shorter strings
    p.drawText(QRect(-m_offset, (r.height() / 2) - (m_textHeight / 2), m_textWidth, m_textHeight),
               Qt::AlignLeft|Qt::AlignTop, m_text);
}


void LabelMarquee::setText(const QString &text)
{
    m_text = text;
    QFontMetrics metrik(this->font());
    this->setMinimumHeight(metrik.height());
    QRect r = metrik.boundingRect(m_text);
    m_textWidth = r.width();
    m_textHeight = r.height();
    m_offset = 0;
    update();
    repaint();
}

QString LabelMarquee::text()
{
    return m_text;
}

void LabelMarquee::processMarquee()
{
    if(!marqueeNeeded())
    {
        m_offset = 0;
        repaint();
        return; // Marquee is not neede here
    }

    m_offset += 4;
    if(m_offset > m_textWidth)
        m_offset = -m_textWidth;
    repaint();
}
