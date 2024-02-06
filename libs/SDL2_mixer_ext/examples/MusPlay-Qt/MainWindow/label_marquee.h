#ifndef LABEL_MARQUEE_H
#define LABEL_MARQUEE_H

#include <QWidget>
#include <QTimer>
#include <QLabel>

class LabelMarquee : public QLabel
{
    Q_OBJECT
public:
    explicit LabelMarquee(QWidget *parent = nullptr);
    explicit LabelMarquee(QString text, QWidget *parent = nullptr);
    virtual ~LabelMarquee();

    void setText(const QString &text);
    QString text();

protected:
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);

private slots:
    void processMarquee();

private:
    void init();
    bool marqueeNeeded();
    QString m_text;
    int m_offset = 0;
    int m_textWidth = 0;
    int m_textHeight = 0;
    QTimer  m_ticker;
};

#endif // LABEL_MARQUEE_H
