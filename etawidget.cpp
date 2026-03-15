#include "etawidget.h"
#include <QPainter>
#include <QLinearGradient>

EtaWidget::EtaWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setContentsMargins(0,0,0,0);
}

void EtaWidget::setTimes(int elapsedSecs, int remainSecs)
{
    m_elapsed = elapsedSecs;
    m_remain  = remainSecs;
    update();
}

void EtaWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const double w = width();
    const double h = height();

    int eh = m_elapsed/3600, em=(m_elapsed%3600)/60, es=m_elapsed%60;
    QString elapsedStr = QString("%1:%2:%3")
                             .arg(eh,2,10,QChar('0')).arg(em,2,10,QChar('0')).arg(es,2,10,QChar('0'));

    int rm = m_remain/60, rs = m_remain%60;
    QString remainStr = QString("%1:%2").arg(rm,2,10,QChar('0')).arg(rs,2,10,QChar('0'));

    double topH    = h * 0.56;   // elapsed gets more vertical space
    double bottomH = h - topH;

    // ── Top: ELAPSED label (dim) + big value ─────────────────────
    QFont labelF("Courier New");
    labelF.setPixelSize(qMax(8, (int)(h * 0.175)));
    labelF.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    p.setFont(labelF);
    p.setPen(QColor(0, 130, 170, 110));
    p.drawText(QRectF(0, 0, w, topH),
               Qt::AlignLeft | Qt::AlignVCenter, "ELAPSED");

    QFont bigF("Courier New");
    bigF.setPixelSize(qMax(16, (int)(h * 0.40)));
    bigF.setBold(true);
    bigF.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    p.setFont(bigF);

    // Glow
    p.setPen(QColor(0, 220, 255, 30));
    for (int d : {-1,0,1})
        p.drawText(QRectF(d, d, w, topH),
                   Qt::AlignRight | Qt::AlignVCenter, elapsedStr);
    p.setPen(QColor(0, 225, 255, 255));
    p.drawText(QRectF(0, 0, w, topH),
               Qt::AlignRight | Qt::AlignVCenter, elapsedStr);

    // ── Divider ──────────────────────────────────────────────────
    QLinearGradient div(0, topH, w, topH);
    div.setColorAt(0.0, Qt::transparent);
    div.setColorAt(0.2, QColor(0, 140, 190, 45));
    div.setColorAt(0.8, QColor(0, 140, 190, 45));
    div.setColorAt(1.0, Qt::transparent);
    p.setPen(QPen(div, 0.8));
    p.drawLine(QPointF(0, topH), QPointF(w, topH));

    // ── Bottom: TO DSTN label + small countdown ───────────────────
    QFont smallF("Courier New");
    smallF.setPixelSize(qMax(8, (int)(h * 0.175)));
    smallF.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    p.setFont(smallF);
    p.setPen(QColor(0, 130, 170, 110));
    p.drawText(QRectF(0, topH, w * 0.5, bottomH),
               Qt::AlignLeft | Qt::AlignVCenter, "TO DSTN");

    QFont medF("Courier New");
    medF.setPixelSize(qMax(10, (int)(h * 0.255)));
    medF.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    p.setFont(medF);

    QColor countCol = (m_remain == 0)   ? QColor(0, 220, 130, 230)
                      : (m_remain < 300)  ? QColor(255, 150, 0,  230)
                                         :                     QColor(0,  190, 220, 180);
    p.setPen(countCol);
    p.drawText(QRectF(0, topH, w, bottomH),
               Qt::AlignRight | Qt::AlignVCenter, remainStr);
}
